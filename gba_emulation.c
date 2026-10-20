#include "gba_emulation.h"

#include "gba.h"
#include "imports.h"

typedef struct gba_behavior gba_behavior;
typedef struct gba_state gba_state;

unsigned GCToGBAInput(gba_state*, int st);

struct gba_behavior {
  int (*on_status)(gba_state*, u8*);
  int (*on_write)(gba_state*, u32*, u8*);
  int (*on_read)(gba_state*, u32*, u8*);
};

struct gba_state {
  u32 time;
  u32 code;
  u32 buffer[8];
  gba_behavior* behavior;
  u32 ptr;
  u32 id;
};

static gba_state GbaState[4];
// right after a reset, gba is expected to
extern gba_behavior post_reset;
// after sending its code, expects the console to send its code
extern gba_behavior post_code_exchange;
// waits for any command (only 0x04000000) implemented
extern gba_behavior post_handshake;
// received 0x04000000, send 32 bytes of internal payload
extern gba_behavior send_32bytes;
// the gamecube will send its current timestamp and expects it to be spit out
// exactly
extern gba_behavior load_timestamp;
// normal loop from the gba thread
extern gba_behavior normal_operation;

// clang-format off
static char fpayload[] = {
    0x01, 0xFF, 0x59, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x56, 0x00, 0x57, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};
// clang-format on

int reset_status(gba_state* self, u8* status) {
  *status = GBA_JSTAT_PSF1 | GBA_JSTAT_SEND;
  return GBA_READY;
}

// write isn't expected in this state
int dummy_rw(gba_state* self, u32* data, u8* status) {
  *status = 0;
  return GBA_NOT_READY;
}

int reset_read(gba_state* self, u32* data, u8* status) {
  *status = 0;
  *data = self->code;
  self->behavior = &post_code_exchange;
  return GBA_READY;
}

int pe_status(gba_state* self, u8* status) {
  *status = GBA_JSTAT_PSF1;
  return GBA_READY;
}

int pe_write(gba_state* self, u32* data, u8* status) {
  // here the client is supposed to check that it got the proper game code, but
  // we don't care
  *status = 0;
  OSReport("Port: %d: Validated code %x, transitionning to post handshake\n", self->id, *data);
  self->behavior = &post_handshake;
  return GBA_READY;
}

int hs_status(gba_state* self, u8* status) {
  *status = GBA_JSTAT_PSF1 | GBA_JSTAT_PSF0;
  return GBA_READY;
}

int hs_write(gba_state* self, u32* data, u8* status) {
  if (*data == 0x04000000) {
    *status = GBA_JSTAT_PSF1 | GBA_JSTAT_PSF0;
    int port = self->id;
    OSReport("Port: %d: received 0x04 command, transitionning to dumping payload\n", port, *data);
    self->behavior = &send_32bytes;
    self->ptr = 0;
  }
  *status = GBA_JSTAT_PSF1 | GBA_JSTAT_PSF0;
  return GBA_READY;
}

int sd_status(gba_state* self, u8* status) {
  *status = GBA_JSTAT_PSF1 | GBA_JSTAT_PSF0 | GBA_JSTAT_SEND;
  return GBA_READY;
}

int sd_read(gba_state* self, u32* data, u8* status) {
  u32* pl = (u32*)fpayload;
  u8* asbytes = (u8*)fpayload;
  asbytes[1] = self->id;  // port
  pl[1] = self->time;
  *data = pl[self->ptr++];
  if (self->ptr == 8) {
    int port = self->id;
    OSReport("Port: %d: finished dumping, transitionning to loading timestamp\n", port, *data);
    self->ptr = 0;
    self->behavior = &load_timestamp;
  }
  return GBA_READY;
}

int ts_status(gba_state* self, u8* status) {
  *status = GBA_JSTAT_PSF1 | GBA_JSTAT_PSF0 | self->ptr;
  return GBA_READY;
}

int ts_write(gba_state* self, u32* data, u8* status) {
  sd_status(self, status);
  self->time = *data;
  self->ptr = GBA_JSTAT_SEND;
  return GBA_READY;
}

int ts_read(gba_state* self, u32* data, u8* status) {
  sd_status(self, status);
  *data = self->time;
  self->behavior = &normal_operation;
  int port = self->id;
  OSReport("Port: %d: finished reading timestamp, transitionning to normal loop\n", port, *data);
  return GBA_READY;
}
int normal_status(gba_state* self, u8* status);
// writing to gba is supposed to send payloads for the tilemap, i think
int normal_write(gba_state* self, u32* data, u8* status) {
  normal_status(self, status);
  // int port = self->id;
  // OSReport("Port: %d: NORMAL WRITE %08X\n", port, data);
  return 0;
}

// reading the gba means reading the buttons
int normal_read(gba_state* self, u32* data, u8* status) {
  normal_status(self, status);
  int port = self->id;
  *data = GCToGBAInput(self, port);
  return 0;
}

int normal_status(gba_state* self, u8* status) {
  *status = GBA_JSTAT_PSF1 | GBA_JSTAT_PSF0;
  return GBA_READY;
}

gba_behavior post_reset = {reset_status, dummy_rw, reset_read};
gba_behavior post_code_exchange = {pe_status, pe_write, dummy_rw};
gba_behavior post_handshake = {hs_status, hs_write, dummy_rw};
gba_behavior send_32bytes = {sd_status, dummy_rw, sd_read};
gba_behavior load_timestamp = {ts_status, ts_write, ts_read};
gba_behavior normal_operation = {normal_status, normal_write, normal_read};

int myGbaJoyboot(int port, int palette, int speed, void* client_data, int size,
                 unsigned char* status) {
  OSReport("Joyboot in port %d\n", port);
  *status = 0;
  u8* data = client_data;
  // the gamecube game writes those in the client binary before uploading it through joybus
  GbaState[port].code = *(u32*)(data + 0xac);
  GbaState[port].time = *(u32*)(data + 0xc8);
  GbaState[port].behavior = &post_reset;
  GbaState[port].id = port;
  return 0;
}

int myGbaRead(int port, u32* data, u8* status) {
  return GbaState[port].behavior->on_read(&GbaState[port], data, status);
}

int myGbaRead2(int port, u32* data, u8* status) {
  int ret = GBAReadAsync(port, (u8*)data, status, (void*)0x80076c60);
  if (ret == 0) {
    int __GBASync(int port);
    ret = __GBASync(port);
    if (*data >> 24 != 0x01) {
      OSReport("%d: READ %08X\n", port, *data);
    }
    u32 d = *data;
    if (d & 0xFC00) {
      OSReport("%d: WHOMST SET %08X\n", port, *data);
    }
  } else
    ret = 2;
  return ret;
}

int myGbaWrite(int port, u32* data, u8* status) {
  return GbaState[port].behavior->on_write(&GbaState[port], data, status);
}

int myGbaStatus(int port, u8* status) {
  return GbaState[port].behavior->on_status(&GbaState[port], status);
}

int myGbaReset(int port, u8* status) {
  OSReport("Resetting in port %d\n", port);
  GbaState[port].behavior = &post_reset;
  *status = 0;
  return 0;
}

astruct_20* getLinkById(int linkId) {
  astruct_20*** gameplayState = (void*)0x8053ac90;
  if (!gameplayState[0]) return 0;
  return gameplayState[0][linkId + 1];
}

// static unsigned long int next = 1;

// int rand_lcg(void) {
//   next = next * 1103515245 + 12345;
//   return (unsigned int)(next / 65536) % 32768;
// }

unsigned GCToGBAInput(gba_state* self, int port) {
  int st = padStatus[port].button;
  int stickx = padStatus[port].stickX;
  int sticky = padStatus[port].stickY;
  int cstickx = padStatus[port].substickX;
  int csticky = padStatus[port].substickY;
  /*
    Command bytes:
    01: nothing special
    02: signal warping is ready?
    03: continue currently pending dialog
  */

  unsigned base = 0x02000000;

  if (st & PAD_TRIGGER_R) base |= 0x00000100;
  if (st & PAD_TRIGGER_L) base |= 0x00000200;
  if (st & PAD_BUTTON_A) {
    // base |= 0x00010000;
    // A button + progress dialog (bit 15) + next dialog (0x03000000)
    base |= 0x01018000;
  }

  if (st & PAD_BUTTON_B) base |= 0x00020000;
  // GC controllers don't have a select button, so we use the x/y buttons
  if (st & (PAD_BUTTON_X | PAD_BUTTON_Y)) base |= 0x00040000;
  if (st & PAD_BUTTON_START) base |= 0x00080000;

  const int threshold = 30;
  if ((st & PAD_BUTTON_RIGHT) || (stickx > threshold)) base |= 0x00100000;
  if ((st & PAD_BUTTON_LEFT) || (stickx < -threshold)) base |= 0x00200000;
  if ((st & PAD_BUTTON_UP) || (sticky > threshold)) base |= 0x00400000;
  if ((st & PAD_BUTTON_DOWN) || (sticky < -threshold)) base |= 0x00800000;

  void* link = getLinkById(port);
  if (link) {
    if (cstickx > threshold) {
      quickSetFormation(link, 0, 1);
    } else if (cstickx < -threshold) {
      quickSetFormation(link, 3, 1);
    } else if (csticky > threshold) {
      quickSetFormation(link, 2, 1);
    } else if (csticky < -threshold) {
      quickSetFormation(link, 1, 1);
    }
  }

  return base | CRC8bits(((base >> 16) & 0xFF) | (base & 0xFF00));
}
