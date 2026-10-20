#pragma once
#include "common_types.h"

int myGbaJoyboot(int port, int palette, int speed, void* client_data, int size,
                 unsigned char* status);

int myGbaRead(int port, u32* data, u8* status);
int myGbaRead2(int port, u32* data, u8* status);
int myGbaWrite(int port, u32* data, u8* status);

int myGbaStatus(int port, u8* status);
int myGbaReset(int port, u8* status);


#define PAD_BUTTON_LEFT 0x0001
#define PAD_BUTTON_RIGHT 0x0002
#define PAD_BUTTON_DOWN 0x0004
#define PAD_BUTTON_UP 0x0008
#define PAD_TRIGGER_Z 0x0010
#define PAD_TRIGGER_R 0x0020
#define PAD_TRIGGER_L 0x0040
#define PAD_BUTTON_A 0x0100
#define PAD_BUTTON_B 0x0200
#define PAD_BUTTON_X 0x0400
#define PAD_BUTTON_Y 0x0800
#define PAD_BUTTON_MENU 0x1000
#define PAD_BUTTON_START 0x1000

extern int last_button_state[4];
