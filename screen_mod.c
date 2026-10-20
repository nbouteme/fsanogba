#include "imports.h"
#include "gba_emulation.h"

enum WinStatus {
  WIN_REDUCED,
  WIN_FULL,
  WIN_TRANSPARENT
};

static char winstatus[] = {WIN_FULL, WIN_FULL, WIN_FULL, WIN_FULL};

static int target_rectangles[4][4] = {
    {0, 0, 304, 224},
    {304, 0, 304, 224},
    {0, 224, 304, 224},
    {304, 224, 304, 224},
};

static int gbaviewport[4] = {0, 0, 0xf0, 0xa0};
int last_button_state[4] = {0, 0, 0, 0};


void myRenderGBAContent(astruct_4* self,
                        int port,
                        void* cameraData,      // called camera data, but it's actually gba link's state, including displayed frame and position in the gba space, hence why it was initially called camera
                        void* viewport,        // the area of the gba to compute, typically the gba size
                        int* targetRectangle,  // where to put it on the screen
                        char renderBGFlag,
                        int unk2,
                        char unk3) {
  int current = padStatus[port].button;
  int pressed = (((last_button_state[port]) ^ (current)) & (current));
  last_button_state[port] = current;

  if (pressed & PAD_TRIGGER_Z) {
    winstatus[port]++;
    if (winstatus[port] > WIN_TRANSPARENT)
      winstatus[port] = WIN_REDUCED;
  }

  if (winstatus[port] != WIN_REDUCED) {
    renderGBAContent(self, port, cameraData, gbaviewport, target_rectangles[port], 1, unk2, unk3);
  } else {  // vanilla path
    renderGBAContent(self, port, cameraData, viewport, targetRectangle, renderBGFlag, unk2, unk3);
  }
}

void myDrawWholeLayer(astruct_4* param_1, uint param_2, int param_3, int param_4, int param_5,
                      char param_6, char param_7, int param_8) {
  register unsigned long portid __asm__("r25");
  if (winstatus[portid] != WIN_TRANSPARENT)
    return drawWholeLayer(param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8);
  return drawWholeLayer(param_1, param_2, param_3, param_4, param_5, 0x7f, 0x7f, param_8);
}

void myDrawBGQuad(void* quad, void* color) {
  register unsigned long portid __asm__("r25");
  if (winstatus[portid] != WIN_TRANSPARENT)
    drawColoredQuad(quad, color);
}