#ifndef PPC_H
#define PPC_H

#include "common_types.h"

typedef enum {
	_ADDI = 14,
	_ADDIS = 15,
	_CBRANCH = 16,
	_BRANCH = 18,
	_BCCTR = 19,
	_MTSPR = 31
} OPCODES;

typedef enum {
	_BITSET = 12
} BRANCH_OPTIONS;

typedef enum {
	_LT = 8,
	_GT = 9,
	_EQ = 10
} BRANCH_TYPE;

#define SPRID(x) ((x >> 5) | ((x & 0x1f) << 5))

typedef enum {
	_XER = SPRID(1),
	_LR = SPRID(8),
	_CTR = SPRID(9)
} SPR;

// Big Endian
typedef struct {
	union {
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			int LI : 24; // Immediate field (23 bit + sign)
			unsigned AA : 1; // Absolute Address
			unsigned LK : 1; // update LinK
		} I; // Immediate
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned BO : 5; // Branch Option
			unsigned BI : 5; // Branch If
			unsigned BD : 14; // Branch Displacement (13 bits + sign)
			unsigned AA : 1; // Absolute Address
			unsigned LK : 1; // update LinK
		} B; // Branch
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned _r : 14; // Reserved
			unsigned LEVI : 7; // Level
			unsigned _r2 : 3; // Reserved 
			unsigned one : 1; // constant 1
			unsigned _r3 : 1; // Reserved
		} SC; // Supervisor Call
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned RT : 5; // Target
			unsigned RA : 5; // Source or Target
			unsigned D : 16; // Data | SI: Signed Integer			
		} D;
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned RT : 5; // Target
			unsigned RA : 5; // Source or Target
			unsigned DS : 14; // Signed immediate concatenated with 0x0B00
			unsigned XO : 2; // eXtended Opcode			
		} DS; // Data Sign?
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned RT : 5; // Target
			unsigned RA : 5; // Source or Target
			unsigned RB : 5; // Source or Target
			unsigned XO : 10; // eXtended Opcode
			unsigned _r : 1; // Reserved
		} X;
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned BO : 5; // Target
			unsigned BI : 5; // Source or Target
			unsigned _r2 : 3; // Source or Target
			unsigned BH : 2; // Source or Target
			unsigned XO : 10; // eXtended Opcode
			unsigned LK : 1; // Reserved or Rc or 1
		} XL;
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned RT : 5; // Target/Source
			unsigned spr : 10; // Source or Target
			unsigned XO : 10; // eXtended Opcode
			unsigned _reserved : 1; // Reserved
		} XFX;
		struct {
			// 32
			unsigned OPCD : 6; // OPCoDe
			unsigned _r1 : 1; // Reserved
			unsigned FLM : 8; // FLoating Mask
			unsigned _r2 : 1; // Reserved
			unsigned FRB : 5; // Source FPR
			unsigned XO : 10; // eXtended Opcode
			unsigned Rc : 1; // Record
		} XFL;
		struct {
			unsigned OPCD : 6; // OPCoDe
			unsigned RS : 5; // Source
			unsigned RA : 5; // Arg

			unsigned sh : 5; // shift
			unsigned XO : 9; // eXtended Opcode
			unsigned sh2 : 1; // shift
			unsigned Rc : 1; // Record
		} XS;
		struct {
			unsigned OPCD : 6; // OPCoDe
			unsigned RT : 5; // Target
			unsigned RA : 5; // Arg
			unsigned RB : 5; // shift
			unsigned OE : 1; // Overflow Enable
			unsigned XO : 9; // eXtended Opcode
			unsigned Rc : 1; // Record
		} XO;
		struct {
			unsigned OPCD : 6; // OPCoDe
			unsigned FRT : 5; // Target
			unsigned FRA : 5; // Arg
			unsigned FRB : 5; // shift
			unsigned FRC : 5; // Overflow Enable
			unsigned XO : 5; // eXtended Opcode
			unsigned Rc : 1; // Record
		} A;
		struct {
			unsigned OPCD : 6; // OPCoDe
			unsigned RS : 5; // Source
			unsigned RA : 5; // Arg A
			unsigned RB : 5; // Arg B
			unsigned MB : 5; // Mask Begin
			unsigned ME : 5; // Mask End
			unsigned Rc : 1; // Record
		} M;
		struct {
			unsigned OPCD : 6; // OPCoDe
			unsigned RS : 5; // Source
			unsigned RA : 5; // Arg A
			unsigned sh : 5; // Arg B
			unsigned XO : 3; // eXtended Opcode
			unsigned MB : 6; // Mask Begin
			unsigned sh2 : 1; // Mask End
			unsigned Rc : 1; // Record
		} MD;
		struct {
			unsigned OPCD : 6; // OPCoDe
			unsigned RS : 5; // Source
			unsigned RA : 5; // Arg A
			unsigned RB : 5; // Arg B
			unsigned MBE : 3; // Mask Begin/End
			unsigned XO : 1; // Mask End
			unsigned Rc : 1; // Record
		} MDS;
	};
} __attribute__((packed, scalar_storage_order("big-endian"))) ppc_instruction;

#define PPCI(form, opc, ...) (ppc_instruction){form = {.OPCD = opc, __VA_ARGS__}}

#define ADDIS(target, a, i) PPCI(.D,   _ADDIS, .RT = target, .RA = a, .D = (i >> 16) + !!(i & 0x8000))
#define ADDI(target, a, i)  PPCI(.D,   _ADDI,  .RT = target, .RA = a, .D = i & 0xFFFF)
#define MTSPR(n, x)         PPCI(.XFX, _MTSPR, .RT = x, .spr = n, .XO = 467)
#define MTCTR(x)            MTSPR(_CTR, x)

#define BCTRL				PPCI(.XL,  _BCCTR, .BO = 0x14, .BI = 0, .BH = 0, .XO = 528, .LK = 1)
#define BCTR				PPCI(.XL,  _BCCTR, .BO = 0x14, .BI = 0, .BH = 0, .XO = 528, .LK = 0)
#define BL(off)             PPCI(.I, _BRANCH, .LI = off, .AA = 0, .LK = 1);
#define B(off)              PPCI(.I, _BRANCH, .LI = off, .AA = 0, .LK = 0);

#define BEQ(off)            PPCI(.B, _CBRANCH, .BO=_EQ, .BD = off, .AA = 0, .LK = 0);

// Register Transfer & Move
#define MR(dst, src)        PPCI(.X, 31, .RT = src, .RA = dst, .RB = src, .XO = 444)
#define NOP()               PPCI(.D, 24, .RT = 0, .RA = 0, .D = 0)
#define LI(dst, imm)        PPCI(.D, _ADDI, .RT = dst, .RA = 0, .D = imm & 0xFFFF)
#define LIS(dst, imm)       PPCI(.D, _ADDIS, .RT = dst, .RA = 0, .D = (imm >> 16) + !!(imm & 0x8000))

// Memory Access (Load / Store)
#define LWZ(rt, ra, d)      PPCI(.D, 32, .RT = rt, .RA = ra, .D = d & 0xFFFF)
#define STW(rs, ra, d)      PPCI(.D, 36, .RT = rs, .RA = ra, .D = d & 0xFFFF)
#define STWU(rs, ra, d)     PPCI(.D, 37, .RT = rs, .RA = ra, .D = d & 0xFFFF)
#define LWZU(rt, ra, d)     PPCI(.D, 33, .RT = rt, .RA = ra, .D = d & 0xFFFF)

// Special Purpose Registers (LR, CTR)
#define MFSPR(rt, n)        PPCI(.XFX, 31, .RT = rt, .spr = n, .XO = 339)
#define MFLR(rt)            MFSPR(rt, _LR)
#define MTLR(rs)            MTSPR(_LR, rs)
#define MFCTR(rt)           MFSPR(rt, _CTR)

// Control Flow & Returns
#define BLR()               PPCI(.XL, _BCCTR, .BO = 0x14, .BI = 0, .BH = 0, .XO = 16, .LK = 0)
#define BLRL()              PPCI(.XL, _BCCTR, .BO = 0x14, .BI = 0, .BH = 0, .XO = 16, .LK = 1)

void monkeypatch_far(arg addr, arg target); // Inserts a longcall to target
void monkeypatch_close(arg addr, arg target); // Inserts a closecall to target 
void monkeypatchb_close(arg addr, arg tadr);
void monkeypatchbl_close(arg addr, arg tadr);
void monkeypatchbeql_close(arg addr, arg ret, arg tadr);
void monkeypatchb_far(arg addr, arg tadr);

void __InitBATS();

#endif /* PPC_H */
