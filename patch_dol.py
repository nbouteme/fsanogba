#!/usr/bin/env python3
import sys
import struct

def parse_elf(elf_bytes):
    if elf_bytes[:4] != b'\x7fELF':
        raise ValueError("Not a valid ELF file")

    is_32 = (elf_bytes[4] == 1)
    is_be = (elf_bytes[5] == 2)
    if not is_32 or not is_be:
        raise ValueError("Only 32-bit big-endian ELF is supported")

    endian = '>'

    # ELF32 Header from offset 16 to 52 (36 bytes):
    # e_type (2), e_machine (2), e_version (4), e_entry (4), e_phoff (4),
    # e_shoff (4), e_flags (4), e_ehsize (2), e_phentsize (2), e_phnum (2),
    # e_shentsize (2), e_shnum (2), e_shstrndx (2) -> HHIIIIIHHHHHH (36 bytes)
    (
        e_type, e_machine, e_version, e_entry, e_phoff, e_shoff, e_flags,
        e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx
    ) = struct.unpack(endian + 'HHIIIIIHHHHHH', elf_bytes[16:52])

    sections = []
    symtab_sec = None

    # Section headers (40 bytes each)
    sh_fmt = endian + 'IIIIIIIIII'
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        sh = struct.unpack(sh_fmt, elf_bytes[off:off+40])
        sec = {
            'sh_name': sh[0],
            'sh_type': sh[1],
            'sh_flags': sh[2],
            'sh_addr': sh[3],
            'sh_offset': sh[4],
            'sh_size': sh[5],
            'sh_link': sh[6],
            'sh_info': sh[7],
            'sh_addralign': sh[8],
            'sh_entsize': sh[9]
        }
        sections.append(sec)
        if sec['sh_type'] == 2:  # SHT_SYMTAB
            symtab_sec = sec

    start_addr = None
    if symtab_sec:
        strtab_sec = sections[symtab_sec['sh_link']]
        strtab_data = elf_bytes[strtab_sec['sh_offset'] : strtab_sec['sh_offset'] + strtab_sec['sh_size']]

        sym_fmt = endian + 'IIIBBH'
        sym_entsize = symtab_sec['sh_entsize'] or 16
        num_syms = symtab_sec['sh_size'] // sym_entsize
        for i in range(num_syms):
            off = symtab_sec['sh_offset'] + i * sym_entsize
            st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack(sym_fmt, elf_bytes[off:off+16])

            name_end = strtab_data.find(b'\x00', st_name)
            name = strtab_data[st_name:name_end].decode('utf-8', errors='ignore') if name_end != -1 else ""
            if name == "_start":
                start_addr = st_value
                break

    if start_addr is None:
        start_addr = e_entry

    alloc_sections = []
    for sec in sections:
        # SHF_ALLOC (0x2) with size > 0
        if (sec['sh_flags'] & 0x2) and sec['sh_size'] > 0:
            if sec['sh_type'] == 1:  # SHT_PROGBITS
                data = elf_bytes[sec['sh_offset'] : sec['sh_offset'] + sec['sh_size']]
            elif sec['sh_type'] == 8:  # SHT_NOBITS
                data = b'\x00' * sec['sh_size']
            else:
                continue
            alloc_sections.append({
                'addr': sec['sh_addr'],
                'data': data
            })

    return start_addr, alloc_sections


class DOL:
    def __init__(self, dol_bytes):
        self.data = bytearray(dol_bytes)
        self.text_offsets = list(struct.unpack('>7I', self.data[0x00:0x1C]))
        self.data_offsets = list(struct.unpack('>11I', self.data[0x1C:0x48]))
        self.text_addrs   = list(struct.unpack('>7I', self.data[0x48:0x64]))
        self.data_addrs   = list(struct.unpack('>11I', self.data[0x64:0x90]))
        self.text_sizes   = list(struct.unpack('>7I', self.data[0x90:0xAC]))
        self.data_sizes   = list(struct.unpack('>11I', self.data[0xAC:0xD8]))

    def vaddr_to_offset(self, vaddr):
        for off, addr, size in zip(self.text_offsets, self.text_addrs, self.text_sizes):
            if off != 0 and size != 0 and addr <= vaddr < addr + size:
                return off + (vaddr - addr)
        for off, addr, size in zip(self.data_offsets, self.data_addrs, self.data_sizes):
            if off != 0 and size != 0 and addr <= vaddr < addr + size:
                return off + (vaddr - addr)
        return None

    def patch_bytes(self, vaddr, patch_data):
        for i, b in enumerate(patch_data):
            curr_vaddr = vaddr + i
            off = self.vaddr_to_offset(curr_vaddr)
            if off is not None:
                self.data[off] = b
            else:
                raise ValueError(f"Address 0x{curr_vaddr:08x} does not map to any section in the DOL file")


def main():
    if len(sys.argv) < 5:
        print("Usage: patch_dol.py <elf_file> <bl_address> <input_dol> <output_dol>", file=sys.stderr)
        sys.exit(1)

    elf_path = sys.argv[1]
    bl_addr = int(sys.argv[2], 0)
    input_dol_path = sys.argv[3]
    output_dol_path = sys.argv[4]

    with open(elf_path, "rb") as f:
        elf_bytes = f.read()

    start_addr, elf_sections = parse_elf(elf_bytes)
    print(f"Target _start symbol address: 0x{start_addr:08x}")
    print(f"Branch and link location: 0x{bl_addr:08x}")

    with open(input_dol_path, "rb") as f:
        dol_bytes = f.read()

    dol = DOL(dol_bytes)

    # 1. Patch ELF sections into DOL
    for sec in elf_sections:
        print(f"Patching ELF section at 0x{sec['addr']:08x} (size 0x{len(sec['data']):x})")
        dol.patch_bytes(sec['addr'], sec['data'])

    # 2. Write branch and link (bl) instruction at bl_addr targeting start_addr
    rel_offset = start_addr - bl_addr
    bl_instr = 0x48000001 | (rel_offset & 0x03FFFFFC)
    bl_bytes = struct.pack('>I', bl_instr)
    print(f"Writing bl instruction 0x{bl_instr:08x} at 0x{bl_addr:08x}")
    dol.patch_bytes(bl_addr, bl_bytes)

    with open(output_dol_path, "wb") as f:
        f.write(dol.data)

    print(f"Successfully created patched DOL at {output_dol_path}")


if __name__ == "__main__":
    main()
