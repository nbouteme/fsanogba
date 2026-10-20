#!/usr/bin/env python3
import sys
import subprocess
import re

def clean_instruction(addr, instr):
    # Strip comment/symbol notes like <fff> or <gbaThread>
    instr = re.sub(r'<[^>]+>', '', instr).strip()

    # Match any branch instruction (b, bl, beq, bne, ble, bge, blt, bgt, bdnz, etc.)
    # Conditional branches can have register/condition operands before target address, e.g. "beq cr0, 80001234"
    m = re.match(r'^(b[a-z0-9+_-]*)\s+(.*?)(?:0x)?([0-9a-fA-F]{8})$', instr)
    if m:
        op = m.group(1)
        prefix = m.group(2) # E.g., "cr0, " or ""
        target_addr = int(m.group(3), 16)
        rel_offset = target_addr - addr
        if rel_offset < 0:
            offset_hex = f"0x{(rel_offset & 0xFFFFFFFF):x}"
        else:
            offset_hex = f"0x{rel_offset:x}"
        return f"{op} {prefix}{offset_hex}"

    # Normalize whitespace between instruction mnemonic and arguments
    instr = re.sub(r'\s+', ' ', instr)
    return instr

def main():
    if len(sys.argv) < 3:
        print("Usage: generate_patch.py <elf_file> <output_asm>", file=sys.stderr)
        sys.exit(1)

    elf_file = sys.argv[1]
    out_file = sys.argv[2]

    devkit_bin = "/opt/devkitpro/devkitPPC/bin/"

    # Parse PROGBITS/ALLOC section details
    sections = []
    proc = subprocess.run([devkit_bin + "powerpc-eabi-readelf", "-S", elf_file], capture_output=True, text=True, check=True)
    for line in proc.stdout.splitlines():
        # Match lines like: [ 1] .text PROGBITS 80060b88 0000b8 000100 00 AX 0 0 4
        m = re.search(r'\[\s*\d+\]\s+([\.\w]+)\s+(\w+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+\d+\s+([A-Z]*)', line)
        if m:
            sec_name, sec_type, sec_addr, sec_off, sec_size, sec_flags = m.groups()
            addr = int(sec_addr, 16)
            size = int(sec_size, 16)
            # Only include allocated sections with size > 0 at target memory (0x80xxxxxx)
            if 'A' in sec_flags and addr >= 0x80000000 and size > 0:
                sections.append({
                    'name': sec_name,
                    'type': sec_type,
                    'addr': addr,
                    'offset': int(sec_off, 16),
                    'size': size,
                    'exec': 'X' in sec_flags
                })

    sections.sort(key=lambda s: s['addr'])

    # Read ELF raw binary data
    with open(elf_file, "rb") as f:
        elf_data = f.read()

    # Read disassembly for executable ranges
    disasm = {}
    proc_dis = subprocess.run([devkit_bin + "powerpc-eabi-objdump", "-d", elf_file], capture_output=True, text=True, check=True)
    for line in proc_dis.stdout.splitlines():
        m = re.match(r'^\s*([0-9a-fA-F]+):\s+(?:[0-9a-fA-F]{2}\s+)+\s*(.*)$', line)
        if m:
            addr = int(m.group(1), 16)
            raw_instr = m.group(2).strip()
            disasm[addr] = clean_instruction(addr, raw_instr)

    # First line setup (Preserve user's loaded locate/branch header)
    src_addr = 0x801347a4
    target_addr = 0x80060b88
    rel_offset = target_addr - src_addr

    lines = []
    lines.append(".locate 0x801347a4")
    lines.append(f"bl 0x{(rel_offset & 0xFFFFFFFF):x}" if rel_offset < 0 else f"b 0x{rel_offset:x}")
    lines.append("")

    # Iterate over all allocated contiguous section ranges
    for sec in sections:
        lines.append(f".locate 0x{sec['addr']:08x}")
        if sec['exec']:
            # Executable code: emit instructions line-by-line
            for addr in range(sec['addr'], sec['addr'] + sec['size'], 4):
                if addr in disasm:
                    lines.append(disasm[addr])
                else:
                    lines.append("nop")
        else:
            # Data / Read-Only / String Literals section
            data_bytes = elf_data[sec['offset'] : sec['offset'] + sec['size']]
            i = 0
            while i < len(data_bytes):
                remaining = len(data_bytes) - i
                if remaining >= 8 and (i % 8 == 0):
                    val = int.from_bytes(data_bytes[i:i+8], byteorder='big')
                    lines.append(f".8byte 0x{val:016x}")
                    i += 8
                elif remaining >= 4 and (i % 4 == 0):
                    val = int.from_bytes(data_bytes[i:i+4], byteorder='big')
                    lines.append(f".4byte 0x{val:08x}")
                    i += 4
                elif remaining >= 2 and (i % 2 == 0):
                    val = int.from_bytes(data_bytes[i:i+2], byteorder='big')
                    lines.append(f".2byte 0x{val:04x}")
                    i += 2
                else:
                    val = data_bytes[i]
                    lines.append(f".byte 0x{val:02x}")
                    i += 1
        lines.append("")

    with open(out_file, "w") as f:
        f.write("\n".join(lines) + "\n")

if __name__ == "__main__":
    main()
