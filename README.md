# FSA No GBA — Four Swords Adventures GBA-link patch

A PowerPC code-injection mod for **The Legend of Zelda: Four Swords Adventures** (GameCube).
It hooks the game's GBA link layer and its GBA screen renderer, so you can play without physical GBAs and see the four GBA displays on the TV.

Concretely, the injected code:

* **Emulates four GBA "joyboot" clients in software.** `gba_emulation.c` fakes the whole
  joybus handshake (reset → code exchange → handshake → payload dump → timestamp → normal
  loop), so the game believes four GBAs are connected, while GameCube pad input is translated
  into the GBA button/timestamp protocol the game expects. This is what lets the game run
  without hardware GBAs attached.
* **Reroutes GBA screen rendering.** `screen_mod.c` re-targets the GBA viewports to larger
  on-screen rectangles (up to four full GBA screens), and pressing **Z** cycles each port's
  window between reduced (vanilla), partially and fully transparent.
* **Patches out the vanilla UI** that gets in the way (corner circles, ranking display, and
  the instructions that inhibit GBA screen rendering) by stubbing or NOP-ing them in
  `entrypoint.c`.

## Layout

| File | Purpose |
| --- | --- |
| `entrypoint.c` | Injection entry point; installs all monkeypatches, NOPs and overrides. |
| `gba_emulation.c` | Software GBA-link state machine + GameCube→GBA input mapping. |
| `screen_mod.c` | GBA viewport/layout changes and Z-button window mode cycling. |
| `ppc.h`, `ppc.c` | PowerPC instruction encoding helpers (`NOP()`, `BLR()`, `LI()`, …). |
| `imports.h`, `main.dol.h` | Declarations and decompiled structs for the game's own functions. |
| `script.ld` | Linker script placing everything at `0x80060b88` (no BSS/SDA). |
| `patch_dol.py` | Injects the built ELF's sections plus a `bl` at the entrypoint into a DOL. |
| `generate_patch.py` | Emits `fsa2.S`, a `.locate`-annotated assembly patch for Dolphin. |

## Building

Requires [devkitPPC](https://devkitpro.org/) (`/opt/devkitpro/devkitPPC` by default, override
with `DEVKITPPC=`) and Python 3.

```sh
make            # builds gba_thread.elf and fsa2.S
make main.dol   # also produces a patched main.dol from orig.dol
make clean
```

`make main.dol` needs an unmodified `orig.dol` of the matching game version in the repo root;
it is the only input not checked in. The resulting `main.dol` is the patched executable, and
`fsa2.S` can be fed to Dolphin's assembly-patch/injector mechanism instead.

## Notes

* Addresses are hardcoded for the USA DOL this was reversed from; another revision will need new addresses.
* Debug output goes through `OSReport`, so watch the Dolphin log to see the link handshake.
* The game's tile renderer runs in immediate mode, so certain area are fairly slow to render and can cause frame dips.
* AI was used to generate this README, linker script and Makefile. The rest is human authored.
