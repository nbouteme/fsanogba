#include "common_types.h"
#include "main.dol.h"

extern PADStatus padStatus[4];

void OSReport(const char* fmt, ...);
void memset2(void* dst, char v, unsigned sz);
void DCFlushRange(u32, int);
void ICInvalidateRange(u32, int);
unsigned CRC8bits(unsigned param_1);

void renderGBAContent(astruct_4* self,
                      int port,
                      void* cameraData,      // called camera data, but it's actually gba link's state
                      void* viewport,        // the area of the gba to compute, typically
                      int* targetRectangle,  // where to put it on the screen
                      char renderBGFlag,
                      int unk2,
                      char unk3);

void drawWholeLayer(astruct_4* param_1, uint param_2, int param_3, int param_4, int param_5,
                    char param_6, char param_7, int param_8);
void drawColoredQuad(void* quad, void* color);

//formation:
// 0 -> horizonal line
// 1 -> vertical line
// 2 -> square
// 3 -> cross
// 4 -> isolate
// 5 -> fetch
void quickSetFormation(astruct_20 *link, int formation, int enableSound);