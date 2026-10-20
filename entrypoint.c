#include "gba.h"
#include "gba_emulation.h"
#include "imports.h"
#include "main.dol.h"
#include "ppc.h"
#include "screen_mod.h"

// Make intellisense not complain about not understanding transparent unions
#ifdef __INTELLISENSE__
#pragma diag_suppress 167
#endif

int mySIProbe(int port) {
  return 0x40000;  // code for GBA
}

__attribute__((section(".gba_thread_sec"))) u32 _start() {
  OSReport("Initializing monkey patching...\n");
  if (1) {
    monkeypatchb_close(0x80076854, myGbaJoyboot);
    monkeypatchb_close(0x80076968, myGbaRead);
    monkeypatchb_close(0x80076ac0, myGbaWrite);
    monkeypatchb_close(0x80075688, myGbaStatus);
    monkeypatchb_close(0x8007577c, myGbaReset);
    monkeypatchb_close(0x80075400, mySIProbe);

    monkeypatchbl_close(0x802fba3c, myDrawWholeLayer);
    monkeypatchbl_close(0x802fb95c, myDrawBGQuad);
    monkeypatchbl_close(0x802ff284, myRenderGBAContent);

    *(ppc_instruction*)0x80419c38 = BLR();  // stub the function that controls the opacity of the corner circles

    const int to_nop[] = {
        // nop the code that draws the ranking and circles
        0x801369bc,

        // nop the instructions that inhibit GBA screen rendering
        0x8041f560,
        0x8041f588,
        0x8041f5b4,
        0x8041f5dc,
        0x80418d9c,

        // disable the screen of shame (skip updating the window state)
        // causes some flickering
        0x802fd8b4,
        0x802fd310,
    };
    for (int i = 0; i < sizeof(to_nop) / sizeof(int); ++i)
      *(ppc_instruction*)to_nop[i] = NOP();

    *(ppc_instruction*)0x8038d30c = LI(3, 1);
    *(ppc_instruction*)0x8038b7b0 = LI(3, 1);  // override a call to pretend it returned 1
  } else {
    monkeypatchb_close(0x80076968, myGbaRead2);
  }
  OSReport("monkey patching done, larping as OSGetCurrentThread now\n");

  return *(u32*)0x800000e4;
}
