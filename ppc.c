#include "ppc.h"
#include "imports.h"

static void flush_instr(arg addr) {
  // https://bugs.dolphin-emu.org/issues/11455#note-46
  // According to delroth, instruction cache invalidation isn't sufficient
  u32 aligned_addr = addr.integer & ~0x1f;  // align to 32 bytes
  // idk how big are cache lines so I'm going to pretend they're 64 bytes, but they're probably 32 bytes
  DCFlushRange(aligned_addr, 0x40);
  ICInvalidateRange(aligned_addr, 0x40);
}

void monkeypatchb_close(arg addr, arg tadr) {
  ppc_instruction* iadr = addr.ptr;
  long diff = tadr.integer - addr.integer;
  diff >>= 2;
  iadr[0] = B(diff);
  flush_instr(addr);
}

void monkeypatchbl_close(arg addr, arg tadr) {
  ppc_instruction* iadr = addr.ptr;
  long diff = tadr.integer - addr.integer;
  diff >>= 2;
  iadr[0] = BL(diff);
  flush_instr(addr);
}