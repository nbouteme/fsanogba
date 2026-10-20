#pragma once
#include "common_types.h"

typedef void (*GBACallback)(s32 chan, s32 ret);

#define GBA_JSTAT_MASK                  0x3a
#define GBA_JSTAT_FLAGS_SHIFT           4
#define GBA_JSTAT_FLAGS_MASK            0x30
#define GBA_JSTAT_PSF1                  0x20
#define GBA_JSTAT_PSF0                  0x10
#define GBA_JSTAT_SEND                  0x08
#define GBA_JSTAT_RECV                  0x02

#define GBA_READY                       0
#define GBA_NOT_READY                   1
#define GBA_BUSY                        2
#define GBA_JOYBOOT_UNKNOWN_STATE       3
#define GBA_JOYBOOT_ERR_INVALID         4

s32 GBAReadAsync(s32 chan, u8* dst, u8* status, GBACallback callback);
