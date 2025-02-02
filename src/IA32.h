/*
 * Copyright 2010-2025 Adrià Giménez Pastor.
 *
 * This file is part of adriagipas/IA32.
 *
 * adriagipas/IA32 is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * adriagipas/IA32 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with adriagipas/IA32.  If not, see <https://www.gnu.org/licenses/>.
 */
/*
 *  IA32.h - Simulador d'ún processador amb arquitectura de 32 bits
 *           d'Intel.
 *
 */
/*
 *  FLAGS:
 *   - __LITTLE_ENDIAN__ : Si l'arquitectura és little endian.
 *   - __BIG_ENDIAN__ : Si l'arquitectura és big endian.
 *   - __IA32_LOCK_FLOAT__ : Per a saber si hi han excepcions float el
 *                           simulador empra les funcions fenv,
 *                           definint este flag ens assegurem que els
 *                           mètodes 'lock/unlock' de l'intèrpret es
 *                           criden abans de cada instrucció.
 */

#ifndef __IA32_H__
#define __IA32_H__

#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __LITTLE_ENDIAN__
#define IA32_LE
#elif defined __BIG_ENDIAN__
#define IA32_BE
#else
#error Per favor defineix __LITTLE_ENDIAN__ o __BIG_ENDIAN__
#endif

#ifndef __STDC_IEC_559__
#error IEEE 754 floating point no suportat
#endif

#if LDBL_MANT_DIG != 64
#error long double no és de grandària 128 bits
#endif


/*************/
/* ESTAT UCP */
/*************/

/* Tipus bàsics. */
#ifdef IA32_LE
typedef union
{
  uint16_t                  v;
  struct { uint8_t v0,v1; } b;
} IA32_Word;

typedef union
{
  uint32_t                        v;
  struct { uint16_t v0,v1; }      w;
  struct { uint8_t v0,v1,v2,v3; } b;
} IA32_Doubleword;
#else /* IA32_BE */
typedef union
{
  uint16_t                  v;
  struct { uint8_t v1,v0; } b;
} IA32_Word;

typedef union
{
  uint32_t                        v;
  struct { uint16_t v1,v0; }      w;
  struct { uint8_t v3,v2,v1,v0; } b;
} IA32_Doubleword;
#endif

/* LSB 0 en LITTLE_ENDIAN. */
typedef uint64_t IA32_DoubleQuadword[2];

/* Per a saber on està un segment (o estructures similars) en memòria
 * lineal.
 */
typedef struct
{
  uint32_t addr;     /* Adreça del byte 0. */
  uint32_t firstb;   /* Primer byte (offsets) vàlid dins del segment. */
  uint32_t lastb;    /* Últim byte (offsets) vàlid dins del segment. */
} IA32_SegmentLimits;

/* Regitre de segment. Conté part visible i part privada. */
typedef struct
{
  uint16_t v;    /* Part visible. */
  struct
  {
    IA32_SegmentLimits lim;
    int                is32; /* En un segment de codi indica que les
        			adreces i operacions són de 32bits. En
        			el SS indica que s'ha de gastar ESP
        			(32bit) en comptes de SP (16bit) com a
        			punter de pila. */
    bool               readable;
    bool               writable;
    bool               executable;
    bool               isnull;
    bool               tss_is32; /* Sols emprat en el registre TSS. */
    bool               data_or_nonconforming;
    uint8_t            pl; // Privileg level. Sols útil i modificat en
                           // mode protegit. En mode real este camp no
                           // es modifica mai. És a dir, que sempre
                           // tindrà el valor que tinguera en mode
                           // protegit.
    uint8_t            dpl;
  }        h; /* Part oculta. */
} IA32_SegmentRegister;

/* Registre XMM. En algunes instruccions l'endianisme s'ha de tindre
   en compter. LSB 0 en LITTLE_ENDIAN. */
typedef union
{
  double   f64[2];
  float    f32[4];
  uint64_t i64[2];
  uint32_t i32[4];
  uint16_t i16[8];
  uint8_t  i8[16];
} IA32_XMMRegister;

typedef struct
{
  enum {
    IA32_CPU_FPU_TAG_VALID= 0,
    IA32_CPU_FPU_TAG_ZERO,
    IA32_CPU_FPU_TAG_SPECIAL,
    IA32_CPU_FPU_TAG_EMPTY
  }           tag;
  long double v;
} IA32_FPURegister;

typedef struct
{
  uint32_t offset;
  uint16_t selector;
} IA32_FPUInstructionPointer;

typedef enum
  {
    IA32_CPU_P5_60MHZ=0,
    IA32_CPU_P5_66MHZ,
    IA32_CPU_P54C_75MHZ,
    IA32_CPU_P54C_90MHZ,
    IA32_CPU_P54C_100MHZ
  } IA32_CPU_MODEL;

/* Estat del processador. */
typedef struct
{

  // Model
  IA32_CPU_MODEL model;
  
  // Registres de propòsit general.
  IA32_Doubleword eax;
  IA32_Doubleword ebx;
  IA32_Doubleword ecx;
  IA32_Doubleword edx;
  IA32_Doubleword esi;
  IA32_Doubleword edi;
  IA32_Doubleword ebp;
  IA32_Doubleword esp;

  // Registres de segment.
  IA32_SegmentRegister cs;
  IA32_SegmentRegister ds;
  IA32_SegmentRegister ss;
  IA32_SegmentRegister es;
  IA32_SegmentRegister fs;
  IA32_SegmentRegister gs;

  // Taules de descriptors locals, global i interrupcions.
  IA32_SegmentLimits idtr;
  IA32_SegmentLimits gdtr;
  IA32_SegmentRegister ldtr;

  // Taula amb l'estat de la tasca (TSS).
  IA32_SegmentRegister tr;
  
  // Estat del programa i registres de control.
  uint32_t eflags;
  
  // Punter d'instrucció.
  uint32_t eip;

  // Registres de control.
  uint32_t cr0;
  uint32_t cr2;
  uint32_t cr3;
  uint32_t cr4;

  // Registres de depuració.
  uint32_t dr0;
  uint32_t dr1;
  uint32_t dr2;
  uint32_t dr3;
  uint32_t dr4;
  uint32_t dr5;
  uint32_t dr6;
  uint32_t dr7;
  
  // Registres XMM (SSE/SSE2/SSE3/...)
  IA32_XMMRegister xmm0;
  IA32_XMMRegister xmm1;
  IA32_XMMRegister xmm2;
  IA32_XMMRegister xmm3;
  IA32_XMMRegister xmm4;
  IA32_XMMRegister xmm5;
  IA32_XMMRegister xmm6;
  IA32_XMMRegister xmm7;

  // MXCSR Control and Status Register (SSE/SSE2/SSE3/...).
  uint32_t mxcsr;

  // FPU
  struct
  {
    uint8_t                    top;
    uint16_t                   opcode;
    uint16_t                   status;
    uint16_t                   control;
    IA32_FPUInstructionPointer dptr;
    IA32_FPUInstructionPointer iptr;
    IA32_FPURegister           regs[8];
  } fpu;
  
} IA32_CPU;

typedef enum
  {
   IA32_CPU_POWER_UP=0,
   IA32_CPU_RESET,
   IA32_CPU_INIT,
   IA32_CPU_FINIT
  } IA32_CPU_INIT_MODE;

// Inicialitza els registres
void
IA32_cpu_init (
               IA32_CPU                 *cpu,
               const IA32_CPU_INIT_MODE  mode,
               const IA32_CPU_MODEL      model
               );

// Operació CPUID

void
IA32_cpu_cpuid (
                IA32_CPU *cpu
                );


/****************/
/* DISASSEMBLER */
/****************/

/* Mnemonics. */
typedef enum
  {
    IA32_UNK= 0,
    IA32_AAA,
    IA32_AAD,
    IA32_AAM,
    IA32_AAS,
    IA32_ADC32,
    IA32_ADC16,
    IA32_ADC8,
    IA32_ADD32,
    IA32_ADD16,
    IA32_ADD8,
    IA32_AND32,
    IA32_AND16,
    IA32_AND8,
    IA32_BOUND32,
    IA32_BOUND16,
    IA32_BSF32,
    IA32_BSF16,
    IA32_BSR32,
    IA32_BSR16,
    IA32_BSWAP,
    IA32_BT32,
    IA32_BT16,
    IA32_BTC32,
    IA32_BTC16,
    IA32_BTR32,
    IA32_BTR16,
    IA32_BTS32,
    IA32_BTS16,
    IA32_CALL32_NEAR,
    IA32_CALL16_NEAR,
    IA32_CALL32_FAR,
    IA32_CALL16_FAR,
    IA32_CBW,
    IA32_CDQ,
    IA32_CLC,
    IA32_CLD,
    IA32_CLI,
    IA32_CLTS,
    IA32_CMC,
    IA32_CMP32,
    IA32_CMP16,
    IA32_CMP8,
    IA32_CMPS32,
    IA32_CMPS16,
    IA32_CMPS8,
    IA32_CPUID,
    IA32_CWD,
    IA32_CWDE,
    IA32_DAA,
    IA32_DAS,
    IA32_DEC32,
    IA32_DEC16,
    IA32_DEC8,
    IA32_DIV32,
    IA32_DIV16,
    IA32_DIV8,
    IA32_ENTER16,
    IA32_ENTER32,
    IA32_F2XM1,
    IA32_FABS,
    IA32_FADD32,
    IA32_FADD64,
    IA32_FADD80,
    IA32_FADDP80,
    IA32_FBSTP,
    IA32_FCHS,
    IA32_FCLEX,
    IA32_FCOM32,
    IA32_FCOM64,
    IA32_FCOM80,
    IA32_FCOMP32,
    IA32_FCOMP64,
    IA32_FCOMP80,
    IA32_FCOMPP,
    IA32_FCOS,
    IA32_FDIV32,
    IA32_FDIV64,
    IA32_FDIV80,
    IA32_FDIVP80,
    IA32_FDIVR32,
    IA32_FDIVR64,
    IA32_FDIVR80,
    IA32_FDIVRP80,
    IA32_FFREE,
    IA32_FILD16,
    IA32_FILD32,
    IA32_FILD64,
    IA32_FIMUL32,
    IA32_FINIT,
    IA32_FIST32,
    IA32_FISTP16,
    IA32_FISTP32,
    IA32_FISTP64,
    IA32_FLD1,
    IA32_FLD32,
    IA32_FLD64,
    IA32_FLD80,
    IA32_FLDCW,
    IA32_FLDL2E,
    IA32_FLDLN2,
    IA32_FLDZ,
    IA32_FMUL32,
    IA32_FMUL64,
    IA32_FMUL80,
    IA32_FMULP80,
    IA32_FNSTSW,
    IA32_FPATAN,
    IA32_FPREM,
    IA32_FPTAN,
    IA32_FRNDINT,
    IA32_FRSTOR16,
    IA32_FRSTOR32,
    IA32_FSAVE16,
    IA32_FSAVE32,
    IA32_FSCALE,
    IA32_FSETPM,
    IA32_FSIN,
    IA32_FSQRT,
    IA32_FST32,
    IA32_FST64,
    IA32_FST80,
    IA32_FSTP32,
    IA32_FSTP64,
    IA32_FSTP80,
    IA32_FSTCW,
    IA32_FSTSW,
    IA32_FSUB32,
    IA32_FSUB64,
    IA32_FSUB80,
    IA32_FSUBP80,
    IA32_FSUBR80,
    IA32_FSUBR64,
    IA32_FSUBR32,
    IA32_FSUBRP80,
    IA32_FTST,
    IA32_FXAM,
    IA32_FXCH,
    IA32_FYL2X,
    IA32_FWAIT,
    IA32_HLT,
    IA32_IDIV32,
    IA32_IDIV16,
    IA32_IDIV8,
    IA32_IMUL32,
    IA32_IMUL16,
    IA32_IMUL8,
    IA32_IN,
    IA32_INC32,
    IA32_INC16,
    IA32_INC8,
    IA32_INS32,
    IA32_INS16,
    IA32_INS8,
    IA32_INT32,
    IA32_INT16,
    IA32_INTO32,
    IA32_INTO16,
    IA32_INVLPG32,
    IA32_INVLPG16,
    IA32_IRET32,
    IA32_IRET16,
    IA32_JA32,
    IA32_JA16,
    IA32_JAE32,
    IA32_JAE16,
    IA32_JB32,
    IA32_JB16,
    IA32_JCXZ32,
    IA32_JCXZ16,
    IA32_JE32,
    IA32_JE16,
    IA32_JECXZ32,
    IA32_JECXZ16,
    IA32_JG32,
    IA32_JG16,
    IA32_JGE32,
    IA32_JGE16,
    IA32_JL32,
    IA32_JL16,
    IA32_JMP32_NEAR,
    IA32_JMP16_NEAR,
    IA32_JMP32_FAR,
    IA32_JMP16_FAR,
    IA32_JNA32,
    IA32_JNA16,
    IA32_JNE32,
    IA32_JNE16,
    IA32_JNG32,
    IA32_JNG16,
    IA32_JNO32,
    IA32_JNO16,
    IA32_JNS32,
    IA32_JNS16,
    IA32_JO32,
    IA32_JO16,
    IA32_JP32,
    IA32_JP16,
    IA32_JPO32,
    IA32_JPO16,
    IA32_JS32,
    IA32_JS16,
    IA32_LAHF,
    IA32_LAR32,
    IA32_LAR16,
    IA32_LDS32,
    IA32_LDS16,
    IA32_LEA32,
    IA32_LEA16,
    IA32_LEAVE32,
    IA32_LEAVE16,
    IA32_LES32,
    IA32_LES16,
    IA32_LFS32,
    IA32_LFS16,
    IA32_LGDT32,
    IA32_LGDT16,
    IA32_LGS32,
    IA32_LGS16,
    IA32_LIDT32,
    IA32_LIDT16,
    IA32_LLDT,
    IA32_LMSW,
    IA32_LODS32,
    IA32_LODS16,
    IA32_LODS8,
    IA32_LOOP32,
    IA32_LOOP16,
    IA32_LOOPE32,
    IA32_LOOPE16,
    IA32_LOOPNE32,
    IA32_LOOPNE16,
    IA32_LSL32,
    IA32_LSL16,
    IA32_LSS32,
    IA32_LSS16,
    IA32_LTR,
    IA32_MOV32,
    IA32_MOV16,
    IA32_MOV8,
    IA32_MOVS32,
    IA32_MOVS16,
    IA32_MOVS8,
    IA32_MOVSX32W,
    IA32_MOVSX32B,
    IA32_MOVSX16,
    IA32_MOVZX32W,
    IA32_MOVZX32B,
    IA32_MOVZX16,
    IA32_MUL32,
    IA32_MUL16,
    IA32_MUL8,
    IA32_NEG32,
    IA32_NEG16,
    IA32_NEG8,
    IA32_NOP,
    IA32_NOT32,
    IA32_NOT16,
    IA32_NOT8,
    IA32_OR32,
    IA32_OR16,
    IA32_OR8,
    IA32_OUT,
    IA32_OUTS32,
    IA32_OUTS16,
    IA32_OUTS8,
    IA32_POP32,
    IA32_POP16,
    IA32_POPA32,
    IA32_POPA16,
    IA32_POPF32,
    IA32_POPF16,
    IA32_PUSH32,
    IA32_PUSH16,
    IA32_PUSHA32,
    IA32_PUSHA16,
    IA32_PUSHF32,
    IA32_PUSHF16,
    IA32_RCL32,
    IA32_RCL16,
    IA32_RCL8,
    IA32_RCR32,
    IA32_RCR16,
    IA32_RCR8,
    IA32_RET32_FAR,
    IA32_RET16_FAR,
    IA32_RET32_NEAR,
    IA32_RET16_NEAR,
    IA32_ROL32,
    IA32_ROL16,
    IA32_ROL8,
    IA32_ROR32,
    IA32_ROR16,
    IA32_ROR8,
    IA32_SAHF,
    IA32_SAR32,
    IA32_SAR16,
    IA32_SAR8,
    IA32_SBB32,
    IA32_SBB16,
    IA32_SBB8,
    IA32_SCAS32,
    IA32_SCAS16,
    IA32_SCAS8,
    IA32_SETA,
    IA32_SETAE,
    IA32_SETB,
    IA32_SETE,
    IA32_SETG,
    IA32_SETGE,
    IA32_SETL,
    IA32_SETNA,
    IA32_SETNE,
    IA32_SETNG,
    IA32_SETS,
    IA32_SGDT32,
    IA32_SGDT16,
    IA32_SHL32,
    IA32_SHL16,
    IA32_SHL8,
    IA32_SHLD32,
    IA32_SHLD16,
    IA32_SHR32,
    IA32_SHR16,
    IA32_SHR8,
    IA32_SHRD32,
    IA32_SHRD16,
    IA32_SIDT32,
    IA32_SIDT16,
    IA32_SLDT,
    IA32_SMSW32,
    IA32_SMSW16,
    IA32_STC,
    IA32_STD,
    IA32_STI,
    IA32_STOS32,
    IA32_STOS16,
    IA32_STOS8,
    IA32_STR,
    IA32_SUB32,
    IA32_SUB16,
    IA32_SUB8,
    IA32_TEST32,
    IA32_TEST16,
    IA32_TEST8,
    IA32_VERR,
    IA32_VERW,
    IA32_WBINVD,
    IA32_XCHG32,
    IA32_XCHG16,
    IA32_XCHG8,
    IA32_XOR32,
    IA32_XOR16,
    IA32_XOR8,
    IA32_XLATB32,
    IA32_XLATB16
  } IA32_Mnemonic;

/* Operator instruction type. */
typedef enum
{
  IA32_NONE= 0,
  IA32_DR0,
  IA32_DR1,
  IA32_DR2,
  IA32_DR3,
  IA32_DR4,
  IA32_DR5,
  IA32_DR6,
  IA32_DR7,
  IA32_CR0,
  IA32_CR1,
  IA32_CR2,
  IA32_CR3,
  IA32_CR4,
  IA32_CR8,
  IA32_AL,
  IA32_CL,
  IA32_DL,
  IA32_BL,
  IA32_AH,
  IA32_CH,
  IA32_DH,
  IA32_BH,
  IA32_AX,
  IA32_CX,
  IA32_DX,
  IA32_BX,
  IA32_SP,
  IA32_BP,
  IA32_SI,
  IA32_DI,
  IA32_EAX,
  IA32_ECX,
  IA32_EDX,
  IA32_EBX,
  IA32_ESP,
  IA32_EBP,
  IA32_ESI,
  IA32_EDI,
  IA32_SEG_ES,
  IA32_SEG_CS,
  IA32_SEG_SS,
  IA32_SEG_DS,
  IA32_SEG_FS,
  IA32_SEG_GS,
  IA32_XMM0,
  IA32_XMM1,
  IA32_XMM2,
  IA32_XMM3,
  IA32_XMM4,
  IA32_XMM5,
  IA32_XMM6,
  IA32_XMM7,
  IA32_IMM8,
  IA32_IMM16,
  IA32_IMM32,
  IA32_ADDR16_BX_SI, /* SEG:[BX+SI] */
  IA32_ADDR16_BX_DI, /* SEG:[BX+DI] */
  IA32_ADDR16_BP_SI, /* SEG:[BP+SI] */
  IA32_ADDR16_BP_DI, /* SEG:[BP+DI] */
  IA32_ADDR16_SI, /* SEG:[SI] */
  IA32_ADDR16_DI, /* SEG:[DI] */
  IA32_ADDR16_DISP16, /* SEG:DISP16 */
  IA32_ADDR16_BX, /* SEG:[BX] */
  IA32_ADDR16_BX_SI_DISP8, /* SEG:[BX+SI]+DISP8 */
  IA32_ADDR16_BX_DI_DISP8, /* SEG:[BX+DI]+DISP8 */
  IA32_ADDR16_BP_SI_DISP8, /* SEG:[BP+SI]+DISP8 */
  IA32_ADDR16_BP_DI_DISP8, /* SEG:[BP+DI]+DISP8 */
  IA32_ADDR16_SI_DISP8, /* SEG:[SI]+DISP8 */
  IA32_ADDR16_DI_DISP8, /* SEG:[DI]+DISP8 */
  IA32_ADDR16_BP_DISP8, /* SEG:[BP]+DISP8 */
  IA32_ADDR16_BX_DISP8, /* SEG:[BX]+DISP8 */
  IA32_ADDR16_BX_SI_DISP16, /* SEG:[BX+SI]+DISP16 */
  IA32_ADDR16_BX_DI_DISP16, /* SEG:[BX+DI]+DISP16 */
  IA32_ADDR16_BP_SI_DISP16, /* SEG:[BP+SI]+DISP16 */
  IA32_ADDR16_BP_DI_DISP16, /* SEG:[BP+DI]+DISP16 */
  IA32_ADDR16_SI_DISP16, /* SEG:[SI]+DISP16 */
  IA32_ADDR16_DI_DISP16, /* SEG:[DI]+DISP16 */
  IA32_ADDR16_BP_DISP16, /* SEG:[BP]+DISP16 */
  IA32_ADDR16_BX_DISP16, /* SEG:[BX]+DISP16 */
  IA32_ADDR32_EAX, /* SEG:[EAX] */
  IA32_ADDR32_ECX, /* SEG:[ECX] */
  IA32_ADDR32_EDX, /* SEG:[EDX] */
  IA32_ADDR32_EBX, /* SEG:[EBX] */
  IA32_ADDR32_SIB, /* SEG:SIB(SCALE+VAL) */
  IA32_ADDR32_DISP32, /* SEG:DISP32 */
  IA32_ADDR32_ESI, /* SEG:[ESI] */
  IA32_ADDR32_EDI, /* SEG:[EDI] */
  IA32_ADDR32_EAX_DISP8, /* SEG:[EAX]+DISP8 */
  IA32_ADDR32_ECX_DISP8, /* SEG:[ECX]+DISP8 */
  IA32_ADDR32_EDX_DISP8, /* SEG:[EDX]+DISP8 */
  IA32_ADDR32_EBX_DISP8, /* SEG:[EBX]+DISP8 */
  IA32_ADDR32_SIB_DISP8, /* SEG:SIB(SCALE+VAL)+DISP8 */
  IA32_ADDR32_EBP_DISP8, /* SEG:[EBP]+DISP8 */
  IA32_ADDR32_ESI_DISP8, /* SEG:[ESI]+DISP8 */
  IA32_ADDR32_EDI_DISP8, /* SEG:[EDI]+DISP8 */
  IA32_ADDR32_EAX_DISP32, /* SEG:[EAX]+DISP32 */
  IA32_ADDR32_ECX_DISP32, /* SEG:[ECX]+DISP32 */
  IA32_ADDR32_EDX_DISP32, /* SEG:[EDX]+DISP32 */
  IA32_ADDR32_EBX_DISP32, /* SEG:[EBX]+DISP32 */
  IA32_ADDR32_SIB_DISP32, /* SEG:SIB(SCALE+VAL)+DISP32 */
  IA32_ADDR32_EBP_DISP32, /* SEG:[EBP]+DISP32 */
  IA32_ADDR32_ESI_DISP32, /* SEG:[ESI]+DISP32 */
  IA32_ADDR32_EDI_DISP32, /* SEG:[EDI]+DISP32 */
  IA32_REL8,
  IA32_REL16,
  IA32_REL32,
  IA32_PTR16_16,
  IA32_PTR16_32,
  IA32_MOFFS_OFF32,
  IA32_MOFFS_OFF16,
  IA32_CONSTANT_1,
  IA32_CONSTANT_3,
  IA32_USE_ADDR32, // Indica que l'instrucció s'executa en mode ADDR32
  IA32_USE_ADDR16,  // Indica que l'instrucció s'executa en mode ADDR16
  IA32_FPU_STACK_POS
} IA32_InstOpType;

typedef enum
  {
    IA32_SIB_VAL_EAX, /* EAX */
    IA32_SIB_VAL_ECX, /* ECX */
    IA32_SIB_VAL_EDX, /* EDX */
    IA32_SIB_VAL_EBX, /* EBX */
    IA32_SIB_VAL_ESP, /* ESP */
    IA32_SIB_VAL_DISP32, /* DISP32 */
    IA32_SIB_VAL_EBP, /* EBP */
    IA32_SIB_VAL_ESI, /* [ESI] */
    IA32_SIB_VAL_EDI /* [EDI] */
  } IA32_SibValType;

typedef enum
  {
    IA32_SIB_SCALE_NONE, /* none */
    IA32_SIB_SCALE_EAX, /* [EAX] */
    IA32_SIB_SCALE_ECX, /* [ECX] */
    IA32_SIB_SCALE_EDX, /* [EDX] */
    IA32_SIB_SCALE_EBX, /* [EBX] */
    IA32_SIB_SCALE_EBP, /* [EBP] */
    IA32_SIB_SCALE_ESI, /* [ESI] */
    IA32_SIB_SCALE_EDI, /* [EDI] */
    IA32_SIB_SCALE_EAX_2, /* [EAX*2] */
    IA32_SIB_SCALE_ECX_2, /* [ECX*2] */
    IA32_SIB_SCALE_EDX_2, /* [EDX*2] */
    IA32_SIB_SCALE_EBX_2, /* [EBX*2] */
    IA32_SIB_SCALE_EBP_2, /* [EBP*2] */
    IA32_SIB_SCALE_ESI_2, /* [ESI*2] */
    IA32_SIB_SCALE_EDI_2, /* [EDI*2] */
    IA32_SIB_SCALE_EAX_4, /* [EAX*4] */
    IA32_SIB_SCALE_ECX_4, /* [ECX*4] */
    IA32_SIB_SCALE_EDX_4, /* [EDX*4] */
    IA32_SIB_SCALE_EBX_4, /* [EBX*4] */
    IA32_SIB_SCALE_EBP_4, /* [EBP*4] */
    IA32_SIB_SCALE_ESI_4, /* [ESI*4] */
    IA32_SIB_SCALE_EDI_4, /* [EDI*4] */
    IA32_SIB_SCALE_EAX_8, /* [EAX*8] */
    IA32_SIB_SCALE_ECX_8, /* [ECX*8] */
    IA32_SIB_SCALE_EDX_8, /* [EDX*8] */
    IA32_SIB_SCALE_EBX_8, /* [EBX*8] */
    IA32_SIB_SCALE_EBP_8, /* [EBP*8] */
    IA32_SIB_SCALE_ESI_8, /* [ESI*8] */
    IA32_SIB_SCALE_EDI_8  /* [EDI*8] */
  } IA32_SibScaleType;

typedef enum
  {
    IA32_DS= 0,
    IA32_SS,
    IA32_ES,
    IA32_CS,
    IA32_FS,
    IA32_GS,
    IA32_UNK_SEG
  } IA32_SegmentName;

/* Operator instruction. */
typedef struct
{

  IA32_InstOpType   type;
  IA32_SegmentName  data_seg; // Per a adreces, moffs.
  uint8_t           u8; // Per a disp8, imm8.
  uint16_t          u16; // Per a disp16, imm16, rel16, offset16, moffs32.
  uint32_t          u32; // Per a disp32, imm32, rel32, offset32, moffs16.
  IA32_SibValType   sib_val; // Per a operands amb SIB.
  IA32_SibScaleType sib_scale; // Per a operands amb SIB.
  uint8_t           sib_u8; // Per a disp8 de SIB.
  uint32_t          sib_u32; // Per a disp32 de SIB.
  uint16_t          ptr16; // Per a ptr16.
  int               fpu_stack_pos; // Per a FPU_STACK_POS
  
} IA32_InstOp;

typedef enum
{
  IA32_PREFIX_NONE= 0,
  IA32_PREFIX_REP,
  IA32_PREFIX_REPE,
  IA32_PREFIX_REPNE
} IA32_Prefix;

/* Instruction. */
typedef struct
{

  IA32_Mnemonic name;
  IA32_InstOp   ops[3];
  int           nbytes;
  int           real_nbytes; // En nbytes es descarten prefixes de
                             // grups repetits.
  uint8_t       bytes[15];
  IA32_Prefix   prefix;
  
} IA32_Inst;

/* Disassembler. */
typedef struct
{
  
  /* Punter que es passat a tots els callbacks. */
  void *udata;
  
  /* Callbacks. */
  bool (*mem_read) (void *udata,const uint64_t offset,uint8_t *data);
  bool (*address_size_is_32) (void *udata);
  bool (*operand_size_is_32) (void *udata);

  /* ESTAT PRIVAT. No inicialitzar directament. */
  IA32_SegmentName _data_seg;
  bool _group1_inst;
  bool _group2_inst;
  bool _group3_inst;
  bool _group4_inst;
  bool _switch_operand_size;
  bool _switch_addr_size;
  bool _repe_repz_enabled;
  bool _repne_repnz_enabled;
  
} IA32_Disassembler;

bool
IA32_dis (
          IA32_Disassembler *dis,
          uint64_t           offset,
          IA32_Inst         *inst
          );


/*************/
/* INTERPRET */
/*************/

typedef struct IA32_Interpreter IA32_Interpreter;

// Interpret d'instruccions.
// NOTA!!! Els callbacks es poden modificar en qualsevol moment.
struct IA32_Interpreter
{

  // Punter a l'estat del processador.
  IA32_CPU *cpu;

  // Punter que es passa a tots els callbacks.
  void *udata;

  // Callback per a imprimir avisos.
  void (*warning) (void *udata,const char *format,...);

  // Callback per a tracejar Softwar Interruptions. OPCIONAL!!! Pot ser NULL.
  // Si és distint de NULL es crida cada vegada que es crida a la
  // interrupció INT.
  void (*trace_soft_int) (void *udata,const uint8_t vector,const IA32_CPU *cpu);
  
  // Lock/Unlock. Cridat cada vegada que es fa un lock.
  void (*lock) (void *udata);
  void (*unlock) (void *udata);

  // Callbacks memòria.  S'assumeix que la la memòria està en little
  // endian. L'adreça és de 36bits.
  uint8_t (*mem_read8) (void *udata,const uint64_t addr);
  uint16_t (*mem_read16) (void *udata,const uint64_t addr);
  uint32_t (*mem_read32) (void *udata,const uint64_t addr);
  uint64_t (*mem_read64) (void *udata,const uint64_t addr);
  void (*mem_write8) (void *udata,const uint64_t addr,const uint8_t data);
  void (*mem_write16) (void *udata,const uint64_t addr,const uint16_t data);
  void (*mem_write32) (void *udata,const uint64_t addr,const uint32_t data);

  // Callbacks ports.
  uint8_t (*port_read8) (void *udata,const uint16_t port);
  uint16_t (*port_read16) (void *udata,const uint16_t port);
  uint32_t (*port_read32) (void *udata,const uint16_t port);
  void (*port_write8) (void *udata,const uint16_t port,const uint8_t data);
  void (*port_write16) (void *udata,const uint16_t port,const uint16_t data);
  void (*port_write32) (void *udata,const uint16_t port,const uint32_t data);
  
  // ESTAT PRIVAT. No inicialitzar directament.
  bool _locked;
  IA32_SegmentRegister *_data_seg;
  int _switch_op_size;
  int _switch_addr_size;
  bool _group1_inst;
  bool _group2_inst;
  bool _group3_inst;
  bool _group4_inst;
  bool _inhibit_interrupt;
  bool _repe_repz_enabled;
  bool _repne_repnz_enabled;
  uint32_t _old_EIP;
  bool _intr;
  bool _halted;
  bool _ignore_exceptions; // Emprat en mode traça
  int  _int_counter;
  
  // -> callbacks mem.
  int (*_mem_read8) (IA32_Interpreter *,
        	     IA32_SegmentRegister *seg,
        	     const uint32_t        offset,
        	     uint8_t              *dst,
        	     const bool            reading_data);
  int (*_mem_read16) (IA32_Interpreter *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint16_t             *dst,
        	      const bool            reading_data);
  int (*_mem_read32) (IA32_Interpreter *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint32_t             *dst,
        	      const bool            reading_data);
  int (*_mem_read64) (IA32_Interpreter *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint64_t             *dst);
  int (*_mem_read128) (IA32_Interpreter *,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       IA32_DoubleQuadword   dst,
        	       const bool            isSSE);
  int (*_mem_write8) (IA32_Interpreter *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      const uint8_t         data);
  int (*_mem_write16) (IA32_Interpreter *,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       const uint16_t        data);
  int (*_mem_write32) (IA32_Interpreter *,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       const uint32_t        data);

  // -> callbacks mem linial.
  int (*_mem_readl8) (IA32_Interpreter *,
        	      const uint32_t  addr,
        	      uint8_t        *dst,
                      const bool      reading_data);
  int (*_mem_readl16) (IA32_Interpreter *,
        	       const uint32_t  addr,
        	       uint16_t       *dst,
                       const bool      reading_data,
                       const bool      implicit_svm);
  int (*_mem_readl32) (IA32_Interpreter *,
        	       const uint32_t  addr,
        	       uint32_t       *dst,
                       const bool      reading_data,
                       const bool      implicit_svm);
  int (*_mem_readl64) (IA32_Interpreter *,
        	       const uint32_t  addr,
        	       uint64_t       *dst);
  int (*_mem_readl128) (IA32_Interpreter *,
        		const uint32_t      addr,
        		IA32_DoubleQuadword dst);
  int (*_mem_writel8) (IA32_Interpreter *,
        	       const uint32_t addr,
        	       const uint8_t  data);
  int (*_mem_writel16) (IA32_Interpreter *,
        		const uint32_t addr,
        		const uint16_t data);
  int (*_mem_writel32) (IA32_Interpreter *,
        		const uint32_t addr,
        		const uint32_t data);
  
};

/* Inicialitza l'estat privat de l'intèrpret. L'estat públic, incloent
 * els registres, ha de ser inicialitzat per l'usuari.
 */
void
IA32_interpreter_init (
        	       IA32_Interpreter *interpreter
        	       );

// Inicialitza un Disassembler que consulta l'estat de l'intèrpret. En
// aquest dissambler els 'offset' sempre fa referència al primer byte
// de la següent instrucció.
void
IA32_interpreter_init_dis (
                           IA32_Interpreter  *interpreter,
                           IA32_Disassembler *dis
                           );

// Executa la següent instrucció. 
void
IA32_exec_next_inst (
        	     IA32_Interpreter *interpreter
        	     );

void
IA32_set_intr (
               IA32_Interpreter *interpreter,
               const bool        value
               );

// ATENCIÓ!!! Esta funció no està implementada, s'ha d'implementar
// fora.
// Torna el vector d'interrupció i és l'encarregat de modificar el
// valor de la senyal INTR si és el cas.
uint8_t
IA32_ack_intr (void);


/*******/
/* JIT */
/*******/
// NOTA!! Es diu JIT però no compila a codi màquina, compila a un
// bytecode més òptim.

// El primer nivell implica 10 bits
#define IA32_JIT_P32_L1_SIZE 1024

typedef struct
{

  bool     active;
  bool     error; // Pàgina errònea
  bool     svm_addr;
  bool     writing_allowed;
  uint64_t base_addr;
  
} IA32_JIT_P32_L2_4KB;

typedef struct
{

  // Atributs.
  bool active;
  bool error; // Pàgina errònea
  bool pse_enabled; // Açò és el valor que tenia quan es va mapejar.
  bool svm_addr;
  bool writing_allowed;

  // Pàgines
  bool                 is4KB;
  IA32_JIT_P32_L2_4KB *v4kB;

  // Adreces
  uint64_t base_addr;
  
} IA32_JIT_P32_L1;

typedef struct
{
  
  IA32_JIT_P32_L1 v[IA32_JIT_P32_L1_SIZE];
  int             a[IA32_JIT_P32_L1_SIZE]; // Entrades actives
  int             N; // Nombre d'entrades actives
  uint64_t        addr_min;
  uint64_t        addr_max;
  uint64_t        base_addr; // Adreça base de la taula.
  
} IA32_JIT_Paging32b;

typedef struct
{
  uint32_t ind; // 0 vol dir NULL, i 1 part d'instrucció. 2 o major és
                // una posició d'una instrucció.
  bool     is32;  // Per a cada entrada de map que siga > 1 indica si
                  // el processador estava en mode 32bits quan es va
                  // descodificar o no. (Independentment que desrpés
                  // amb modificadors s'intercanviara el mode)
} IA32_JIT_PageEntry;

typedef struct IA32_JIT_Page IA32_JIT_Page;

struct IA32_JIT_Page
{

  // Entrades pàgina
  IA32_JIT_PageEntry *entries;
  int                 area_id;
  uint32_t            page_id;
  uint32_t            first_entry; // Primera entrada de map que no és 0
  uint32_t            last_entry; // Última entrada de map que no és 0
  int                 overlap_next_page; // Número de bytes de
                                         // l'última instrucció que se
                                         // n'ixen de la pàgina actual
  
  // Instruccions
  uint16_t *v; // El 0 i l'1 estan reservats
  uint32_t  capacity;
  uint32_t  N;
  
  // Per a gestionar-los en una llista
  IA32_JIT_Page *next;
  IA32_JIT_Page *prev;
  
};

typedef struct
{
  IA32_Inst inst;
  uint32_t  addr;
  uint32_t  flags; // Flags que s'han de calcular
} IA32_JIT_DisEntry;

typedef struct
{
  uint32_t        first_addr;
  uint32_t        last_addr;
  IA32_JIT_Page **map;
} IA32_JIT_MemMap;

typedef struct IA32_JIT IA32_JIT;

struct IA32_JIT
{
  
  // Punter que es passa a tots els callbacks.
  void *udata;
  
  // Callback per a imprimir avisos.
  void (*warning) (void *udata,const char *format,...);

  // Callbacks memòria.  S'assumeix que la memòria està en little
  // endian. L'adreça és de 36bits.
  uint8_t (*mem_read8) (void *udata,const uint64_t addr,
                        const bool reading_data);
  uint16_t (*mem_read16) (void *udata,const uint64_t addr);
  uint32_t (*mem_read32) (void *udata,const uint64_t addr);
  uint64_t (*mem_read64) (void *udata,const uint64_t addr);
  void (*mem_write8) (void *udata,const uint64_t addr,const uint8_t data);
  void (*mem_write16) (void *udata,const uint64_t addr,const uint16_t data);
  void (*mem_write32) (void *udata,const uint64_t addr,const uint32_t data);

  // Callbacks ports.
  uint8_t (*port_read8) (void *udata,const uint16_t port);
  uint16_t (*port_read16) (void *udata,const uint16_t port);
  uint32_t (*port_read32) (void *udata,const uint16_t port);
  void (*port_write8) (void *udata,const uint16_t port,const uint8_t data);
  void (*port_write16) (void *udata,const uint16_t port,const uint16_t data);
  void (*port_write32) (void *udata,const uint16_t port,const uint32_t data);

  // ESTAT PRIVAT.
  // -> CPU
  IA32_CPU             *_cpu;
  IA32_SegmentRegister *_seg_regs[6];
  struct {
    int      vec;
    uint16_t error_code;
    bool     use_error_code;
    bool     with_selector;
    uint16_t selector;
    bool     op32;
  }                     _exception;
  int                  *_clock; // Ho incrementa el jit, però es reseteja.
  
  // -> Desenssamblar
  IA32_Disassembler  _dis;
  bool               _optimize_flags;
  IA32_JIT_DisEntry *_dis_v;
  size_t             _dis_capacity;
  
  // -> Pàgines
  int               _bits_page;
  uint32_t          _page_low_mask;
  IA32_JIT_MemMap  *_mem_map;
  int               _mem_map_size;
  IA32_JIT_Page    *_free_pages; // Pàgines que es poden reciclar
  IA32_JIT_Page    *_pages; // Pàgines en ús
  IA32_JIT_Page    *_pages_tail;
  IA32_JIT_Page    *_lock_page; // Impideix que una pàgina que
                                // s'esborra es fique en _free_nodes.
  bool              _free_lock_page;
  IA32_JIT_Page    *_current_page; // Pàgina actual. Pot ser NULL
  uint32_t          _current_pos; // Posició dins de la pàgina
  bool              _stop_after_port_write;

  // Paginació
  IA32_JIT_Paging32b *_pag32;
  
  // Interrupcions
  bool _inhibit_interrupt;
  bool _intr;
  bool _ignore_exceptions; // Per al mode traça
  
  // -> callbacks mem.
  int (*_mem_read8) (IA32_JIT *,
        	     IA32_SegmentRegister *seg,
        	     const uint32_t        offset,
        	     uint8_t              *dst,
        	     const bool            reading_data);
  int (*_mem_read16) (IA32_JIT *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint16_t             *dst,
        	      const bool            reading_data);
  int (*_mem_read32) (IA32_JIT *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint32_t             *dst,
        	      const bool            reading_data);
  int (*_mem_read64) (IA32_JIT *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint64_t             *dst);
  int (*_mem_read128) (IA32_JIT *,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       IA32_DoubleQuadword   dst,
        	       const bool            isSSE);
  int (*_mem_write8) (IA32_JIT *,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      const uint8_t         data);
  int (*_mem_write16) (IA32_JIT *,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       const uint16_t        data);
  int (*_mem_write32) (IA32_JIT *,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       const uint32_t        data);

  // -> callbacks mem linial.
  int (*_mem_readl8) (IA32_JIT *,
        	      const uint32_t  addr,
        	      uint8_t        *dst,
                      const bool      is_data);
  int (*_mem_readl16) (IA32_JIT *,
        	       const uint32_t  addr,
        	       uint16_t       *dst,
                       const bool      is_data,
                       const bool      implicit_svm);
  int (*_mem_readl32) (IA32_JIT *,
        	       const uint32_t  addr,
        	       uint32_t       *dst,
                       const bool      is_data,
                       const bool      implicit_svm);
  int (*_mem_readl64) (IA32_JIT *,
        	       const uint32_t  addr,
        	       uint64_t       *dst,
                       const bool      is_data);
  int (*_mem_readl128) (IA32_JIT *,
        		const uint32_t      addr,
        		IA32_DoubleQuadword dst,
                        const bool          is_data);
  int (*_mem_writel8) (IA32_JIT *,
        	       const uint32_t addr,
        	       const uint8_t  data);
  int (*_mem_writel16) (IA32_JIT *,
        		const uint32_t addr,
        		const uint16_t data);
  int (*_mem_writel32) (IA32_JIT *,
        		const uint32_t addr,
        		const uint32_t data);
  
};

typedef struct
{
  uint32_t addr; // Ha d'estat aliniat amb la grandària de pàgina elegida.
  size_t   size;
} IA32_JIT_MemArea;

// Reserva memòria i inicialitza estat privat. Ningun callback
// s'inicialitza ací.
// Les arees dins de mem_area han d'estar en ordre.
IA32_JIT *
IA32_jit_new (
              IA32_CPU               *cpu,
              const int               bits_page, // 2^bits_page és la
                                                 // grandària de cada
                                                 // pàgina
              const bool              optimize_flags,
              const IA32_JIT_MemArea *mem_areas,
              const int               N
              );

void
IA32_jit_reset (
                IA32_JIT *jit
                );

void
IA32_jit_free (
               IA32_JIT *jit
               );

void
IA32_jit_clear_areas (
                      IA32_JIT *jit
                      );

// Torna cert en cas de pàgina esborrada, false si tot continua igual.
bool
IA32_jit_addr_changed (
                       IA32_JIT       *jit,
                       const uint32_t  addr
                       );

void
IA32_jit_area_remapped (
                        IA32_JIT       *jit,
                        const uint32_t  begin,
                        const uint32_t  last // Inclos
                        );

// Executa la següent instrucció. 
void
IA32_jit_exec_next_inst (
                         IA32_JIT *jit
                         );

void
IA32_jit_set_intr (
                   IA32_JIT   *jit,
                   const bool  value
                   );

// Inicialitza un Disassembler que consulta l'estat de l'intèrpret. En
// aquest dissambler els 'offset' sempre fa referència al primer byte
// de la següent instrucció.
void
IA32_jit_init_dis (
                   IA32_JIT          *jit,
                   IA32_Disassembler *dis
                   );

#endif // __IA32_H__
