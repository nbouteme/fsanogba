#pragma once
#include "imports.h"

void myRenderGBAContent(astruct_4* self,
                        int port,
                        void* cameraData,      // called camera data, but it's actually gba link's state, including displayed frame and position in the gba space, hence why it was initially called camera
                        void* viewport,        // the area of the gba to compute, typically the gba size
                        int* targetRectangle,  // where to put it on the screen
                        char renderBGFlag,
                        int unk2,
                        char unk3);
void myDrawWholeLayer(astruct_4* param_1, uint param_2, int param_3, int param_4, int param_5,
                      char param_6, char param_7, int param_8);

void myDrawBGQuad(void* quad, void* color);