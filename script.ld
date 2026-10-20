/* PowerPC EABI Linker Script (No BSS, No SDA/SDA2) */
OUTPUT_FORMAT("elf32-powerpc", "elf32-powerpc", "elf32-powerpc")
OUTPUT_ARCH(powerpc)
ENTRY(gbaThread)

PROVIDE(padStatus = 0x8052ea28);
PROVIDE(CRC8bits = 0x803006e8);

PROVIDE(DCFlushRange = 0x80040f60);
PROVIDE(ICInvalidateRange = 0x80041044);
PROVIDE(OSReport = 0x80041f34);
PROVIDE(GBAReadAsync = 0x800768fc);
PROVIDE(__GBASync = 0x80076c94);
PROVIDE(renderGBAContent = 0x802fb69c);
PROVIDE(drawWholeLayer = 0x802fac10);
PROVIDE(drawColoredQuad = 0x8013b004);
PROVIDE(quickSetFormation = 0x80248004);

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80060b88, LENGTH = 32M
}

SECTIONS
{
    . = 0x80060b88;

    /* 1. Define gbaThread section at the absolute beginning (0x80060b88) */
    .gba_thread_sec :
    {
        PROVIDE(gbaThread = .);
        *(.gba_thread_sec)
    } > RAM

    /* 2. Text / Executable Code */
    .text :
    {
        *(.text .text.* .gnu.linkonce.t.*)
        *(.init)
        *(.fini)
    } > RAM

    /* 3. Read-Only Data */
    .rodata :
    {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        *(.rodata1)
        *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
    } > RAM

    /* 4. Read-Write Data (Includes all bss/common/small-data) */
    .data :
    {
        *(.data .data.* .gnu.linkonce.d.*)
        *(.data1)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.scommon)
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
    } > RAM

    /* Discard unused metadata sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame)
    }
}
