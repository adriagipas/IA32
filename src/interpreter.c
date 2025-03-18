/*
 * Copyright 2015-2025 Adrià Giménez Pastor.
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
 *  interpreter.c - Implementació de l'intèrpret d'instruccions.
 *
 */
/*
 *  NOTES:
 *
 * - Respecte als prefixes, estic inpedint anidaments de nivell 1,
 *   però així i tot es podrien produir anidaments de nivell 2. De
 *   qualsevol manera el codi deuria d'estar bé.
 *
 */


#include <assert.h>
#include <fenv.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "IA32.h"




/**********/
/* MACROS */
/**********/

#define PROTO_INTERP IA32_Interpreter *interpreter
#define INTERP (interpreter)
#define CPU (INTERP->cpu)

#define WW (INTERP->warning)
#define UDATA (INTERP->udata)
#define LOCK (INTERP->lock) (UDATA); INTERP->_locked= true
#define UNLOCK (INTERP->unlock) (UDATA); INTERP->_locked= false
#define QLOCKED (INTERP->_locked==true)
#ifdef __IA32_LOCK_FLOAT__
#define LOCK_FLOAT LOCK
#define UNLOCK_FLOAT UNLOCK_FLOAT
#else
#define LOCK_FLOAT
#define UNLOCK_FLOAT
#endif
#define SET_DATA_SEG(SEG) (INTERP->_data_seg= (SEG))
#define UNSET_DATA_SEG (INTERP->_data_seg= NULL)
#define OVERRIDE_SEG(TO)        				\
  if ( INTERP->_data_seg != NULL ) (TO)= INTERP->_data_seg


/* EXCEPCIONS *****************************************************************/
#define MAX_NESTED_INT 20

#define EXCP_DE 0
#define EXCP_DB 1
#define EXCP_BP 2
#define EXCP_OF 4
#define EXCP_BR 5
#define EXCP_UD 6
#define EXCP_NM 7
#define EXCP_DF 8
#define EXCP_CO 9
#define EXCP_TS 10
#define EXCP_NP 11
#define EXCP_SS 12
#define EXCP_GP 13
#define EXCP_PF 14
#define EXCP_MF 16
#define EXCP_AC 17
#define EXCP_MC 18
#define EXCP_XM 19
#define EXCP_VE 20
#define EXCP_IS_FAULT(EXC)                                              \
  ((EXC)==EXCP_DE ||                                                    \
   (EXC)==EXCP_DB || /* No queda clar*/                                 \
   (EXC)==EXCP_BR ||                                                    \
   (EXC)==EXCP_UD ||                                                    \
   (EXC)==EXCP_NM ||                                                    \
   (EXC)==EXCP_CO ||                                                    \
   (EXC)==EXCP_TS ||                                                    \
   (EXC)==EXCP_NP ||                                                    \
   (EXC)==EXCP_SS ||                                                    \
   (EXC)==EXCP_GP ||                                                    \
   (EXC)==EXCP_PF ||                                                    \
   (EXC)==EXCP_MF ||                                                    \
   (EXC)==EXCP_AC ||                                                    \
   (EXC)==EXCP_XM ||                                                    \
   (EXC)==EXCP_VE)

/* Quan es supossa valor és equivalent a ficar un 0. */
#define EXCEPTION(VECTOR) {                                             \
    if ( !INTERP->_ignore_exceptions )                                  \
      {                                                                 \
        if ( EXCP_IS_FAULT ( (VECTOR) ) ) EIP= INTERP->_old_EIP;        \
        if ( interruption ( INTERP, INTERRUPTION_TYPE_UNK,              \
                            (VECTOR), 0, false ) )                      \
          {                                                             \
            WW ( UDATA, "no s'ha pogut executar l'excepció" );          \
          }                                                             \
      }                                                                 \
    }

#define EXCEPTION0(VECTOR)                      \
  EXCEPTION_ERROR_CODE ( (VECTOR), 0 )

// NOTA!!! CREC QUE ESTE EN REALITAT ES POT TRANSFORMAR EN
// EXCEPTION_ERROR_CODE EMPRANT int_error_code
#define EXCEPTION_SEL(VECTOR,SEL) {                                     \
    if ( !INTERP->_ignore_exceptions )                                  \
      {                                                                 \
        if ( EXCP_IS_FAULT ( (VECTOR) ) ) EIP= INTERP->_old_EIP;        \
        if ( interruption ( INTERP, INTERRUPTION_TYPE_UNK,              \
                            (VECTOR), (SEL)&0xFFFC, true ) )            \
          {                                                             \
            WW ( UDATA, "no s'ha pogut executar l'excepció" );          \
          }                                                             \
      }                                                                 \
    }


#define EXCEPTION_ERROR_CODE(VECTOR,ERROR) {                            \
    if ( !INTERP->_ignore_exceptions )                                  \
      {                                                                 \
        if ( EXCP_IS_FAULT ( (VECTOR) ) ) EIP= INTERP->_old_EIP;        \
        if ( interruption ( INTERP, INTERRUPTION_TYPE_UNK,              \
                            (VECTOR), (ERROR), true ) )                 \
          {                                                             \
            WW ( UDATA, "no s'ha pogut executar l'excepció" );          \
          }                                                             \
      }                                                                 \
    }


#define MODEL (CPU->model)

/* REGISTRES ******************************************************************/
#define EAX (CPU->eax.v)
#define EBX (CPU->ebx.v)
#define ECX (CPU->ecx.v)
#define EDX (CPU->edx.v)
#define EBP (CPU->ebp.v)
#define ESI (CPU->esi.v)
#define EDI (CPU->edi.v)
#define ESP (CPU->esp.v)
#define AX  (CPU->eax.w.v0)
#define BX  (CPU->ebx.w.v0)
#define CX  (CPU->ecx.w.v0)
#define DX  (CPU->edx.w.v0)
#define BP  (CPU->ebp.w.v0)
#define SI  (CPU->esi.w.v0)
#define DI  (CPU->edi.w.v0)
#define SP  (CPU->esp.w.v0)
#define AH  (CPU->eax.b.v1)
#define AL  (CPU->eax.b.v0)
#define BH  (CPU->ebx.b.v1)
#define BL  (CPU->ebx.b.v0)
#define CH  (CPU->ecx.b.v1)
#define CL  (CPU->ecx.b.v0)
#define DH  (CPU->edx.b.v1)
#define DL  (CPU->edx.b.v0)

#define EFLAGS (CPU->eflags)

#define EFLAGS_1S 0x00000002

#define CF_FLAG   0x00000001
#define PF_FLAG   0x00000004
#define AF_FLAG   0x00000010
#define ZF_FLAG   0x00000040
#define SF_FLAG   0x00000080
#define TF_FLAG   0x00000100
#define IF_FLAG   0x00000200
#define DF_FLAG   0x00000400
#define OF_FLAG   0x00000800
#define IOPL_FLAG 0x00003000
#define NT_FLAG   0x00004000
#define RF_FLAG   0x00010000
#define VM_FLAG   0x00020000
#define AC_FLAG   0x00040000
#define VIF_FLAG  0x00080000
#define VIP_FLAG  0x00100000
#define ID_FLAG   0x00200000

#define IOPL (((EFLAGS)&(IOPL_FLAG))>>12)

#define EIP (CPU->eip)

#define P_CS (&(CPU->cs))
#define P_DS (&(CPU->ds))
#define P_SS (&(CPU->ss))
#define P_ES (&(CPU->es))
#define P_FS (&(CPU->fs))
#define P_GS (&(CPU->gs))

#define CPL (P_CS->h.pl)

#define IDTR (CPU->idtr)
#define GDTR (CPU->gdtr)
#define LDTR (CPU->ldtr)

#define TR (CPU->tr)

#define CR0 (CPU->cr0)
#define CR2 (CPU->cr2)
#define CR3 (CPU->cr3)
#define CR4 (CPU->cr4)

#define CR0_PE 0x00000001
#define CR0_MP 0x00000002
#define CR0_EM 0x00000004
#define CR0_TS 0x00000008
#define CR0_ET 0x00000010
#define CR0_NE 0x00000020
#define CR0_WP 0x00010000
#define CR0_AM 0x00040000
#define CR0_NW 0x20000000
#define CR0_CD 0x40000000
#define CR0_PG 0x80000000
#define CR0_MASK                                                        \
  (CR0_PE|CR0_MP|CR0_EM|CR0_TS|CR0_ET|CR0_NE|CR0_WP|CR0_AM|CR0_NW|CR0_CD|CR0_PG)
#define CR0_RESERVED (~(CR0_MASK))

#define CR3_PWT 0x00000008
#define CR3_PCD 0x00000010
#define CR3_PDB 0xFFFFF000      
#define CR3_MASK (CR3_PWT|CR3_PCD|CR3_PDB)
#define CR3_RESERVED (~(CR3_MASK))

#define CR4_VME        0x00000001
#define CR4_PVI        0x00000002
#define CR4_DE         0x00000008
#define CR4_PSE        0x00000010
#define CR4_PAE        0x00000020
#define CR4_OSFXSR     0x00000200
#define CR4_OSXMMEXCPT 0x00000400
#define CR4_SMEP       0x00100000
#define CR4_SMAP       0x00200000

#define DR0 (CPU->dr0)
#define DR1 (CPU->dr1)
#define DR2 (CPU->dr2)
#define DR3 (CPU->dr3)
#define DR4 (CPU->dr4)
#define DR5 (CPU->dr5)
#define DR6 (CPU->dr6)
#define DR7 (CPU->dr7)

#define DR7_L0   0x00000001
#define DR7_G0   0x00000002
#define DR7_L1   0x00000004
#define DR7_G1   0x00000008
#define DR7_L2   0x00000010
#define DR7_G2   0x00000020
#define DR7_L3   0x00000040
#define DR7_G3   0x00000080
#define DR7_LE   0x00000100
#define DR7_GE   0x00000200
#define DR7_RTM  0x00000800
#define DR7_GD   0x00002000
#define DR7_RW0  0x00030000
#define DR7_LEN0 0x000C0000
#define DR7_RW1  0x00300000
#define DR7_LEN1 0x00C00000
#define DR7_RW2  0x03000000
#define DR7_LEN2 0x0C000000
#define DR7_RW3  0x30000000
#define DR7_LEN3 0xC0000000
#define DR7_MASK                                                        \
  (DR7_L0|DR7_G0|DR7_L1|DR7_G1|DR7_L2|DR7_G2|DR7_L3|DR7_G3|DR7_LE|DR7_GE| \
   DR7_RTM|DR7_GD|DR7_RW0|DR7_LEN0|DR7_RW1|DR7_LEN1|DR7_RW2|DR7_LEN2|   \
   DR7_RW3|DR7_LEN3)
#define DR7_RESERVED_SET1 0x00000400

#define XMM0 (CPU->xmm0)
#define XMM1 (CPU->xmm1)
#define XMM2 (CPU->xmm2)
#define XMM3 (CPU->xmm3)
#define XMM4 (CPU->xmm4)
#define XMM5 (CPU->xmm5)
#define XMM6 (CPU->xmm6)
#define XMM7 (CPU->xmm7)

#define MXCSR (CPU->mxcsr)

#define MXCSR_IE 0x00000001
#define MXCSR_DE 0x00000002
#define MXCSR_ZE 0x00000004
#define MXCSR_OE 0x00000008
#define MXCSR_UE 0x00000010
#define MXCSR_PE 0x00000020

#define MXCSR_IM 0x00000080
#define MXCSR_DM 0x00000100
#define MXCSR_ZM 0x00000200
#define MXCSR_OM 0x00000400
#define MXCSR_UM 0x00000800
#define MXCSR_PM 0x00001000

// NOTA!!! TOP HO MODELE A PART
#define FPU_TOP (CPU->fpu.top)
#define FPU_STATUS (CPU->fpu.status)
#define FPU_STATUS_TOP (((FPU_STATUS)&0xC7FF)|((FPU_TOP)<<11))
#define FPU_CONTROL (CPU->fpu.control)
#define FPU_DPTR (CPU->fpu.dptr)
#define FPU_IPTR (CPU->fpu.iptr)
#define FPU_OPCODE (CPU->fpu.opcode)
#define FPU_REG(IND) ((CPU->fpu.regs)[(IND)])

#define FPU_DPTR_MASK 0x0000FFFFFFFFFFFF
#define FPU_IPTR_MASK 0x0000FFFFFFFFFFFF
#define FPU_OPCODE_MASK 0x97FF

#define FPU_STATUS_IE  0x0001
#define FPU_STATUS_DE  0x0002
#define FPU_STATUS_ZE  0x0004
#define FPU_STATUS_OE  0x0008
#define FPU_STATUS_UE  0x0010
#define FPU_STATUS_PE  0x0020
#define FPU_STATUS_SF  0x0040
#define FPU_STATUS_ES  0x0080
#define FPU_STATUS_C0  0x0100
#define FPU_STATUS_C1  0x0200
#define FPU_STATUS_C2  0x0400
#define FPU_STATUS_C3  0x4000
#define FPU_STATUS_B   0x8000

#define FPU_STATUS_TOP_MASK 0x3800
#define FPU_STATUS_TOP_SHIFT 11

#define FPU_CONTROL_IM 0x0001
#define FPU_CONTROL_DM 0x0002
#define FPU_CONTROL_ZM 0x0004
#define FPU_CONTROL_OM 0x0008
#define FPU_CONTROL_UM 0x0010
#define FPU_CONTROL_PM 0x0020
#define FPU_CONTROL_PC_MASK 0x0300
#define FPU_CONTROL_RC_MASK 0x0C00
#define FPU_CONTROL_RC_SHIFT 10
#define FPU_CONTROL_X  0x1000


/* MEMÒRIA ********************************************************************/
#define READB(P_REGSEG,OFFSET,P_DST,ISDATA,ACTION_ON_ERROR)        	\
  if ( INTERP->_mem_read8 ( INTERP, P_REGSEG, OFFSET, P_DST, ISDATA ) != 0 ) \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READB_INST(P_REGSEG,OFFSET,P_DST,ACTION_ON_ERROR)        \
  READB(P_REGSEG,OFFSET,P_DST,false,ACTION_ON_ERROR)
#define READB_DATA(P_REGSEG,OFFSET,P_DST,ACTION_ON_ERROR)        \
  READB(P_REGSEG,OFFSET,P_DST,true,ACTION_ON_ERROR)
#define READW(P_REGSEG,OFFSET,P_DST,ISDATA,ACTION_ON_ERROR)             \
  if ( INTERP->_mem_read16 ( INTERP, P_REGSEG, OFFSET,                  \
                             P_DST, ISDATA ) != 0 )                     \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READW_INST(P_REGSEG,OFFSET,P_DST,ACTION_ON_ERROR)       \
  READW(P_REGSEG,OFFSET,P_DST,false,ACTION_ON_ERROR)
#define READW_DATA(P_REGSEG,OFFSET,P_DST,ACTION_ON_ERROR)       \
  READW(P_REGSEG,OFFSET,P_DST,true,ACTION_ON_ERROR)
#define READD(P_REGSEG,OFFSET,P_DST,ISDATA,ACTION_ON_ERROR)        	\
  if ( INTERP->_mem_read32 ( INTERP, P_REGSEG, OFFSET, P_DST, ISDATA ) != 0 ) \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READD_INST(P_REGSEG,OFFSET,P_DST,ACTION_ON_ERROR)        \
  READD(P_REGSEG,OFFSET,P_DST,false,ACTION_ON_ERROR)
#define READD_DATA(P_REGSEG,OFFSET,P_DST,ACTION_ON_ERROR)        \
  READD(P_REGSEG,OFFSET,P_DST,true,ACTION_ON_ERROR)
#define READQ(P_REGSEG,OFFSET,P_DST,ACTION_ON_ERROR)        		\
  if ( INTERP->_mem_read64 ( INTERP, P_REGSEG, OFFSET, P_DST ) != 0 )        \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READDQ(P_REGSEG,OFFSET,DST,ISSSE,ACTION_ON_ERROR)        	\
  if ( INTERP->_mem_read128 ( INTERP, P_REGSEG, OFFSET, DST, ISSSE ) != 0 ) \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READDQ_SSE(P_REGSEG,OFFSET,DST,ACTION_ON_ERROR)        \
  READDQ(P_REGSEG,OFFSET,DST,true,ACTION_ON_ERROR)

#define WRITEB(P_REGSEG,OFFSET,BYTE,ACTION_ON_ERROR)        		\
  if ( INTERP->_mem_write8 ( INTERP, P_REGSEG, OFFSET, BYTE ) != 0 )        \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define WRITEW(P_REGSEG,OFFSET,WORD,ACTION_ON_ERROR)        		\
  if ( INTERP->_mem_write16 ( INTERP, P_REGSEG, OFFSET, WORD ) != 0 )        \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define WRITED(P_REGSEG,OFFSET,DWORD,ACTION_ON_ERROR)        		\
  if ( INTERP->_mem_write32 ( INTERP, P_REGSEG, OFFSET, DWORD ) != 0 )        \
    {        								\
      ACTION_ON_ERROR;        						\
    }

static int
push32 (
        PROTO_INTERP,
        const uint32_t data
        );

static int
push16 (
        PROTO_INTERP,
        const uint16_t data
        );

#define PUSHW(WORD,ACTION_ON_ERROR)        				\
  if ( push16 ( INTERP, WORD ) != 0 )                                   \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define PUSHD(DWORD,ACTION_ON_ERROR)        				\
  if ( push32 ( INTERP, DWORD ) != 0 )                                  \
    {        								\
      ACTION_ON_ERROR;        						\
    }

#define POPW(WORD_PTR,ACTION_ON_ERROR)        				\
  if ( pop16 ( INTERP, WORD_PTR ) != 0 )                                \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define POPD(DWORD_PTR,ACTION_ON_ERROR)                                 \
  if ( pop32 ( INTERP, DWORD_PTR ) != 0 )                               \
    {        								\
      ACTION_ON_ERROR;        						\
    }

#define IB(P_DST,ACTION_ON_ERROR)        		\
  do {        						\
    READB_INST ( P_CS, EIP, P_DST, ACTION_ON_ERROR );        \
    ++EIP;        					\
  } while(0)
#define IW(P_DST,ACTION_ON_ERROR)        		\
  do {        						\
    READW_INST ( P_CS, EIP, P_DST, ACTION_ON_ERROR );        \
    EIP+= 2;        					\
  } while(0)
#define ID(P_DST,ACTION_ON_ERROR)        		\
  do {        						\
    READD_INST ( P_CS, EIP, P_DST, ACTION_ON_ERROR );        \
    EIP+= 4;        					\
  } while(0)

/* Macros per a llegir del 'linear-address space'. */
#define READLB(ADDR,P_DST,IS_DATA,ACTION_ON_ERROR)                      \
  if ( INTERP->_mem_readl8 ( INTERP, ADDR, P_DST, IS_DATA ) != 0 )      \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READLW(ADDR,P_DST,IS_DATA,ISVM,ACTION_ON_ERROR)                 \
  if ( INTERP->_mem_readl16 ( INTERP, ADDR, P_DST, IS_DATA, ISVM ) != 0 ) \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READLD(ADDR,P_DST,IS_DATA,ISVM,ACTION_ON_ERROR)                 \
  if ( INTERP->_mem_readl32 ( INTERP, ADDR, P_DST, IS_DATA, ISVM ) != 0 ) \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READLQ(ADDR,P_DST,ACTION_ON_ERROR)                              \
  if ( INTERP->_mem_readl64 ( INTERP, ADDR, P_DST ) != 0 )              \
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define READLDQ(ADDR,P_DST,ACTION_ON_ERROR)                             \
  if ( INTERP->_mem_readl128 ( INTERP, ADDR, P_DST ) != 0 )             \
    {        								\
      ACTION_ON_ERROR;        						\
    }

/* Macros per a escriure en el 'linear-address-space'. */
#define WRITELB(ADDR,BYTE,ACTION_ON_ERROR)        			\
  if ( INTERP->_mem_writel8 ( INTERP, ADDR, BYTE ) != 0 )        	\
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define WRITELW(ADDR,WORD,ACTION_ON_ERROR)        			\
  if ( INTERP->_mem_writel16 ( INTERP, ADDR, WORD ) != 0 )        	\
    {        								\
      ACTION_ON_ERROR;        						\
    }
#define WRITELD(ADDR,DWORD,ACTION_ON_ERROR)        			\
  if ( INTERP->_mem_writel32 ( INTERP, ADDR, DWORD ) != 0 )        	\
    {        								\
      ACTION_ON_ERROR;        						\
    }

// Macros I/O
#define PORT_READ8(PORT) INTERP->port_read8 ( INTERP->udata, PORT )
#define PORT_READ16(PORT) INTERP->port_read16 ( INTERP->udata, PORT )
#define PORT_READ32(PORT) INTERP->port_read32 ( INTERP->udata, PORT )
#define PORT_WRITE8(PORT,DATA) INTERP->port_write8 ( INTERP->udata, PORT, DATA )
#define PORT_WRITE16(PORT,DATA) \
  INTERP->port_write16 ( INTERP->udata, PORT, DATA )
#define PORT_WRITE32(PORT,DATA) \
  INTERP->port_write32 ( INTERP->udata, PORT, DATA )

/* Macros per a llegir de la memòria. */
#define READU8(ADDR) INTERP->mem_read8 ( INTERP->udata, ADDR )
#define READU16(ADDR) INTERP->mem_read16 ( INTERP->udata, ADDR )
#define READU32(ADDR) INTERP->mem_read32 ( INTERP->udata, ADDR )
#define READU64(ADDR) INTERP->mem_read64 ( INTERP->udata, ADDR )
#define WRITEU8(ADDR,DATA) INTERP->mem_write8 ( INTERP->udata, ADDR, DATA )
#define WRITEU16(ADDR,DATA) INTERP->mem_write16 ( INTERP->udata, ADDR, DATA )
#define WRITEU32(ADDR,DATA) INTERP->mem_write32 ( INTERP->udata, ADDR, DATA )

// Nega valors
#define NEG_VARU8(VARU8) ((uint8_t) (-((int8_t) (VARU8))))
#define NEG_VARU16(VARU16) ((uint16_t) (-((int16_t) (VARU16))))
#define NEG_VARU32(VARU32) ((uint32_t) (-((int32_t) (VARU32))))


/* MODE ***********************************************************************/
#define PROTECTED_MODE_ACTIVATED ((CR0&CR0_PE)!=0)


/* OPERAND SIZE ***************************************************************/
#define OPERAND_SIZE_IS_32        					\
  ((PROTECTED_MODE_ACTIVATED&&(!(EFLAGS&VM_FLAG))&&P_CS->h.is32)        \
   ^(INTERP->_switch_op_size))
#define SWITCH_OPERAND_SIZE INTERP->_switch_op_size= 1
#define DONT_SWITCH_OPERAND_SIZE INTERP->_switch_op_size= 0


/* ADDRESS SIZE ***************************************************************/
#define ADDRESS_SIZE_IS_32        					\
  ((PROTECTED_MODE_ACTIVATED&&(!(EFLAGS&VM_FLAG))&&P_CS->h.is32)        \
   ^(INTERP->_switch_addr_size))
#define SWITCH_ADDRESS_SIZE INTERP->_switch_addr_size= 1
#define DONT_SWITCH_ADDRESS_SIZE INTERP->_switch_addr_size= 0


/* REPNE_REPNZ ****************************************************************/
#define ENABLE_REPNE_REPNZ        				\
  INTERP->_repne_repnz_enabled= true
#define DISABLE_REPNE_REPNZ        				\
  INTERP->_repne_repnz_enabled= false


/* REPE_REPZ ****************************************************************/
#define ENABLE_REPE_REPZ        			\
  INTERP->_repe_repz_enabled= true
#define DISABLE_REPE_REPZ        			\
  INTERP->_repe_repz_enabled= false
#define TRY_REP                                 \
  {                                             \
    if ( INTERP->_repe_repz_enabled )           \
      run_rep ( INTERP );                       \
  }
#define TRY_REP_MS                                                    \
  {                                                                   \
    if ( INTERP->_repe_repz_enabled || INTERP->_repne_repnz_enabled ) \
      run_rep ( INTERP );                                             \
  }
#define REP_ENABLED (INTERP->_repe_repz_enabled)
#define TRY_REPE_REPNE                          \
  {                                             \
    if ( INTERP->_repe_repz_enabled )           \
      run_repe ( INTERP );                      \
    else if ( INTERP->_repne_repnz_enabled )    \
      run_repne ( INTERP );                     \
  }
#define REPE_REPNE_ENABLED \
  (INTERP->_repe_repz_enabled || INTERP->_repne_repnz_enabled)
#define REP_COUNTER_IS_0                        \
  (ADDRESS_SIZE_IS_32 ? (ECX==0) : (CX==0))


/* UTILS **********************************************************************/
#define SETU8_SF_ZF_PF_FLAGS(VALU8)        			\
  EFLAGS&= ~(SF_FLAG|ZF_FLAG|PF_FLAG);        			\
  EFLAGS|=        						\
    (((VALU8)&0x80) ? SF_FLAG : 0) |        			\
    (((VALU8)==0) ? ZF_FLAG : 0) |        			\
    PFLAG[(VALU8)]

/* INTERRUPCIONS **************************************************************/
#define INTERRUPTION_TYPE_IMM  0
#define INTERRUPTION_TYPE_INT3 1
#define INTERRUPTION_TYPE_INTO 2
#define INTERRUPTION_TYPE_UNK 3

static int
interruption (
              PROTO_INTERP,
              const int      type,
              const uint8_t  vec,
              const uint16_t error_code,
              const bool     use_error_code
              );

#define INTERRUPTION(VEC,TYPE,ACTION_ON_ERROR)                          \
  if ( interruption ( INTERP, (TYPE), (VEC), 0, false ) != 0 )          \
    {        								\
      ACTION_ON_ERROR;        						\
    }

#define INTERRUPTION_ERR(VEC,TYPE,ERROR,ACTION_ON_ERROR)                \
  if ( interruption ( INTERP, (TYPE), (VEC), (ERROR), true ) != 0 )     \
    {        								\
      ACTION_ON_ERROR;        						\
    }

/* UTILS SEGMENT DESCRIPTORS **************************************************/
#define SEG_DESC_GET_STYPE(DESC) (((DESC).w0>>8)&0x1F)
#define SEG_DESC_IS_PRESENT(DESC) (((DESC).w0&0x00008000)!=0)
#define SEG_DESC_GET_DPL(DESC) (((DESC).w0>>13)&0x3)
#define SEG_DESC_G_IS_SET(DESC) (((DESC).w0&0x00800000)!=0)
#define SEG_DESC_B_IS_SET(DESC) (((DESC).w0&0x00400000)!=0)
#define CG_DESC_GET_SELECTOR(DESC) ((uint16_t) ((DESC).w1>>16))
#define CG_DESC_GET_OFFSET(DESC)        		\
  (((DESC).w0&0xFFFF0000) | ((DESC).w1&0x0000FFFF))
#define CG_DESC_GET_PARAMCOUNT(DESC) ((DESC).w0&0x1F)
#define TG_DESC_GET_SELECTOR(DESC) ((uint16_t) ((DESC).w1>>16))
#define TSS_DESC_IS_BUSY(DESC) (((DESC).w0&0x200)!=0)
#define TSS_DESC_IS_AVAILABLE(DESC) (((DESC).w0&0x100000)!=0)




/*********/
/* TIPUS */
/*********/

/* Address. */
typedef struct
{
  IA32_SegmentRegister *seg;
  uint32_t              off;
} addr_t;


/* Effective address r8. */
typedef struct
{
  bool is_addr;
  union {
    addr_t   addr;
    uint8_t *reg;
  }    v;
} eaddr_r8_t;


/* Effective address r16. */
typedef struct
{
  bool is_addr;
  union {
    addr_t    addr;
    uint16_t *reg;
  }    v;
} eaddr_r16_t;


/* Effective address r32. */
typedef struct
{
  bool is_addr;
  union {
    addr_t    addr;
    uint32_t *reg;
  }    v;
} eaddr_r32_t;


/* Effective address XMM register. */
typedef struct
{
  bool is_addr;
  union {
    addr_t            addr;
    IA32_XMMRegister *reg;
  }    v;
} eaddr_rxmm_t;


/* Descriptor de segment. */
typedef struct
{
  uint32_t addr;     /* Adreça de la primera paraula. */
  uint32_t w0,w1;    /* Paraules. */
} seg_desc_t;





/*************/
/* CONSTANTS */
/*************/

static const uint32_t PFLAG[256]=
  {
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, PF_FLAG, 0, 0, PF_FLAG,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, PF_FLAG, 0, 0, PF_FLAG,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, PF_FLAG, 0, 0, PF_FLAG,
    0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0, PF_FLAG, 0, 0, PF_FLAG,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, PF_FLAG, 0, 0, PF_FLAG,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0,
    0, PF_FLAG, PF_FLAG, 0, PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, PF_FLAG, 0, 0, PF_FLAG,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG, 0, PF_FLAG, PF_FLAG, 0, 0, PF_FLAG, PF_FLAG, 0,
    PF_FLAG, 0, 0, PF_FLAG
  };




/*********************/
/* FUNCIONS PRIVADES */
/*********************/

/* REPE_REPZ ******************************************************************/
static void
run_rep (
         PROTO_INTERP
         )
{

  if ( ADDRESS_SIZE_IS_32 )
    {
      if ( --ECX != 0 ) EIP= INTERP->_old_EIP;
    }
  else
    {
      if ( --CX != 0 ) EIP= INTERP->_old_EIP;
    }
  
} // end run_rep


static void
run_repe (
          PROTO_INTERP
          )
{
  
  if ( ADDRESS_SIZE_IS_32 )
    {
      if ( --ECX != 0 && (EFLAGS&ZF_FLAG)!=0 ) EIP= INTERP->_old_EIP;
    }
  else
    {
      if ( --CX != 0 && (EFLAGS&ZF_FLAG)!=0 ) EIP= INTERP->_old_EIP;
    }
  
} // end run_repe


static void
run_repne (
           PROTO_INTERP
           )
{
  
  if ( ADDRESS_SIZE_IS_32 )
    {
      if ( --ECX != 0 && (EFLAGS&ZF_FLAG)==0 ) EIP= INTERP->_old_EIP;
    }
  else
    {
      if ( --CX != 0 && (EFLAGS&ZF_FLAG)==0 ) EIP= INTERP->_old_EIP;
    }
  
} // end run_repne


/* SEGMENT REGISTERS **********************************************************/
// Torna 0 si tot ha anat bé, -1 en cas contrari.
static int
read_segment_descriptor (
        		 PROTO_INTERP,
        		 const uint16_t  selector,
                         seg_desc_t     *desc,
                         const bool      excp_lim
        		 )
{

  uint32_t off;

  
  // Comprovacions i fixa adreça.
  off= (uint32_t) (selector&0xFFF8);
  if ( selector&0x4 ) // LDT
    {
      if ( LDTR.h.isnull && excp_lim )
        {
          EXCEPTION0 ( EXCP_GP );
          return -1;
        }
      if ( off < LDTR.h.lim.firstb || (off+7) > LDTR.h.lim.lastb )
        {
          if ( excp_lim ) { EXCEPTION_SEL ( EXCP_GP, selector ); }
          return -1;
        }
      desc->addr= LDTR.h.lim.addr + off;
    }
  else // GDT
    {
      if ( off < GDTR.firstb || (off+7) > GDTR.lastb )
        {
          if ( excp_lim ) { EXCEPTION_SEL ( EXCP_GP, selector ); }
          return -1;
        }
      desc->addr= GDTR.addr + off;
    }

  // Llig descriptor.
  READLD ( desc->addr, &(desc->w1), true, true, return -1 );
  READLD ( desc->addr+4, &(desc->w0), true, true, return -1 );
  
  return 0;
  
} // end read_segment_descriptor


// Carrega la part privada del registre.
static void
load_segment_descriptor (
        		 IA32_SegmentRegister *reg,
        		 const uint32_t        selector,
        		 const seg_desc_t     *desc
        		 )
{
  
  uint32_t stype,elimit;


  // Visibles.
  reg->v= selector;
  
  // Camps bàsics.
  stype= SEG_DESC_GET_STYPE ( *desc );
  reg->h.executable= ((stype&0x18)==0x18); // No especial i codi.
  //   -> No especial i, Data o Codi-readable.
  reg->h.readable= (((stype&0x18)==0x10) || ((stype&0x1A)==0x1A));
  reg->h.writable= ((stype&0x1A)==0x12); // No especial, data i writable.
  reg->h.is32= SEG_DESC_B_IS_SET ( *desc ); // AÇÒ ESTà BÉ ?!!!!
  reg->h.isnull= ((selector&0xFFFC) == 0);
  reg->h.data_or_nonconforming= (((stype&0x18)==0x10) || ((stype&0x1C)==0x18));
  /*   -> Sols per a TSS. */
  reg->h.tss_is32= ((stype&0x1D)==0x09);
  
  // Privilege levels.
  reg->h.pl= (uint8_t) (selector&0x3);
  reg->h.dpl= (uint8_t) SEG_DESC_GET_DPL ( *desc );
  
  // Limits.
  reg->h.lim.addr=
    (desc->w1>>16) | ((desc->w0<<16)&0x00FF0000) | (desc->w0&0xFF000000);
  elimit= (desc->w0&0x000F0000) | (desc->w1&0x0000FFFF);
  if ( SEG_DESC_G_IS_SET ( *desc ) )
    {
      elimit<<= 12;
      elimit|= 0xFFF;
    }
  if ( (stype&0x1C) == 0x04 ) // Si és No especial, Data i expand-down.
    {
      printf ( "[EE] load_segment_descriptor, REPASAR AQUESTA PART!!!! sospite que quan granularity està activa açò no està bé\n" );
      exit ( EXIT_FAILURE );
      if ( reg->h.is32 )
        {
          reg->h.lim.firstb= elimit+1;
          reg->h.lim.lastb= 0xFFFFFFFF;
        }
      else
        {
          reg->h.lim.firstb= (elimit+1)&0xFFFF;
          reg->h.lim.lastb= 0xFFFF;
        }
    }
  else // Normal.
    {
      reg->h.lim.firstb= 0;
      reg->h.lim.lastb= elimit;
    }

} // end load_segment_descriptor


// Aquesta funció fixa el bit del descriptor (data/codi) i el torna a
// escriure en memòria.
static void
set_segment_descriptor_access_bit (
        			   PROTO_INTERP,
        			   seg_desc_t *desc
        			   )
{

  // ATENCIÓ!!! En principi WRITELD no deuria de generar excepció. De
  // fet si s'ha aplegat ací no té sentit que es produeixca una
  // excepció al fer un WRITELD. Per tant imprimixc un missatge d'avís
  // i au.
  
  if ( (desc->w0&0x00000100) == 0 )
    {
      desc->w0|= 0x00000100; // Set Access bit.
      WRITELD ( desc->addr+4, desc->w0,
        	fprintf ( stderr, "Avís: excepció en WRITELD en"
        		  " ''set_segment_descriptor_access_bit\n" ) );
    }
  
} // end set_segment_descriptor_access_bit


static void
set_tss_segment_descriptor_busy_bit (
                                     PROTO_INTERP,
                                     seg_desc_t *desc
                                     )
{

  uint32_t old;

  
  // ATENCIÓ!!! En principi WRITELD no deuria de generar excepció. De
  // fet si s'ha aplegat ací no té sentit que es produeixca una
  // excepció al fer un WRITELD. Per tant imprimixc un missatge d'avís
  // i au.

  old= desc->w0;
  desc->w0|= 0x00000200; // Set Busy bit
  if ( old != desc->w0 )
    {
      WRITELD ( desc->addr+4, desc->w0,
                fprintf ( stderr, "Avís: excepció en WRITELD en"
                          " ''set_tss_segment_descriptor_busy_bit\n" ) );
    }
  
} // end set_tss_segment_descriptor_bussy_bit


static int
set_sreg (
          PROTO_INTERP,
          const uint16_t        selector,
          IA32_SegmentRegister *reg
          )
{

  seg_desc_t desc;
  uint32_t stype,dpl,cpl,rpl;

  
  // Carrega en mode protegit
  if ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG) )
    {
      // --> No es pot carregar el CS
      if ( reg == P_CS ) goto ud_exc;
      else if ( reg == P_SS ) // El SS és especial
        {
          if ( (selector&0xFFFC) == 0 ) goto excp_gp_sel; // Null Segment
          if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
            return -1;
          // Type checking and protecttion
          dpl= SEG_DESC_GET_DPL ( desc );
          cpl= CPL;
          rpl= selector&0x3;
          stype= SEG_DESC_GET_STYPE ( desc );
          if (
              // --> is not a writable data segment
              ((stype&0x1A) != 0x12) ||
              // -->  RPL ≠ CPL
              (rpl != cpl) ||
              // --> DPL ≠ CPL
              (dpl != cpl)
              )
            goto  excp_gp_sel;
          // Segment-present flag.
          if ( !SEG_DESC_IS_PRESENT ( desc ) )
            {
              EXCEPTION_SEL ( EXCP_SS, selector );
              return -1;
            }
          // Carrega
          load_segment_descriptor ( P_SS, selector, &desc );
          set_segment_descriptor_access_bit ( INTERP, &desc );
        }
      else
        {
          if ( (selector&0xFFFC) == 0 ) // Null segment
            {
              reg->v= selector;
              reg->h.isnull= true; // Està bé ??????
            }
          else 
            {
              if ( read_segment_descriptor ( INTERP, selector,
                                             &desc, true ) != 0 )
                return -1;
              // Type checking and protecttion
              dpl= SEG_DESC_GET_DPL ( desc );
              cpl= CPL;
              rpl= selector&0x3;
              stype= SEG_DESC_GET_STYPE ( desc );
              if (
                  // --> is not a data or readable code segment
                  !(((stype&0x18) == 0x10) || ((stype&0x1A) == 0x1A) ) ||
                  // --> ((segment is a ¿¿data?? or nonconforming code segment)
                  //      and ((RPL > DPL) or (CPL > DPL)))
                  ((((stype&0x18) == 0x10) || ((stype&0x1C) == 0x18)) &&
                   ((rpl > dpl) || (cpl > dpl)))
                  )
                goto  excp_gp_sel;
              // Segment-present flag.
              if ( !SEG_DESC_IS_PRESENT ( desc ) )
                {
                  EXCEPTION_SEL ( EXCP_NP, selector );
                  return -1;
                }
              // Carrega
              load_segment_descriptor ( reg, selector, &desc );
              set_segment_descriptor_access_bit ( INTERP, &desc );
            }
        }
    }
  
  // Sense protecció.
  else
    {
      reg->v= selector;
      reg->h.lim.addr= ((uint32_t) selector)<<4;
      reg->h.lim.firstb= 0;
      if ( reg == P_CS )
        {
          reg->h.lim.lastb= 0xFFFF;
          reg->h.is32= false;
          reg->h.readable= true;
          reg->h.writable= true;
          reg->h.executable= true;
          reg->h.isnull= false;
          reg->h.data_or_nonconforming= false;
        }
    }
  
  // Ignora interrupcions en la següent instrucció.
  if ( reg == P_SS ) INTERP->_inhibit_interrupt= true;
  
  return 0;
  
 ud_exc:
  EXCEPTION ( EXCP_UD );
  return -1;
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return -1;
} // end set_sreg


static uint16_t
int_error_code (
                const uint16_t num,
                const uint8_t  idt,
                const uint8_t  ext
                )
{

  uint16_t ret;

  
  if ( idt == 0 ) ret= num&0xFC;
  else            ret= (num<<3) | 0x2 | ((uint16_t) ext);

  return ret;
  
} // end int_error_code


static int
int_read_segment_descriptor (
                             PROTO_INTERP,
                             const uint16_t  selector,
                             seg_desc_t     *desc,
                             const uint8_t   ext,
                             const bool      is_TS
                             )
{

  uint32_t off;
  
  
  // NULL selector
  if ( (selector&0xFFFC) == 0 ) goto excp_gp_error;
  
  // Comprovacions i fixa adreça.
  off= (uint32_t) (selector&0xFFF8);
  if ( selector&0x4 ) // LDT
    {
      if ( LDTR.h.isnull )
        {
          EXCEPTION0 ( EXCP_GP );
          return -1;
        }
      if ( off < LDTR.h.lim.firstb || (off+7) > LDTR.h.lim.lastb )
        goto excp_gp_error;
      desc->addr= LDTR.h.lim.addr + off;
    }
  else // GDT
    {
      if ( off < GDTR.firstb || (off+7) > GDTR.lastb )
        goto excp_gp_error;
      desc->addr= GDTR.addr + off;
    }
  
  // Llig descriptor.
  READLD ( desc->addr, &(desc->w1), true, true, return -1 );
  READLD ( desc->addr+4, &(desc->w0), true, true, return -1 );
  
  return 0;
  
 excp_gp_error:
  EXCEPTION_ERROR_CODE ( is_TS ? EXCP_TS : EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
  
} // end int_read_segment_descriptor


static int
switch_task_set_sreg (
                      PROTO_INTERP,
                      const uint16_t        selector,
                      IA32_SegmentRegister *reg,
                      const uint8_t         ext
                      )
{

  seg_desc_t desc;
  uint32_t stype,dpl,rpl,cpl;
  

  if ( reg == P_CS )
    {
      // Comprovacions
      if ( (selector&0xFFFC) == 0 ) // Null segment
        goto excp_ts_error;
      if ( int_read_segment_descriptor ( INTERP, selector,
                                         &desc, ext, true ) != 0 )
        return -1;
      dpl= SEG_DESC_GET_DPL ( desc );
      rpl= selector&0x3;
      stype= SEG_DESC_GET_STYPE ( desc );
      // Nota! No tinc 100% clar que la comprovació del dpl != rpl
      // siga correcta. Em base en aquesta condició taula 7.1:
      //
      //  Code segment DPL matches segment selector RPL.
      if ( (stype&0x18) != 0x18 || dpl != rpl )
        goto excp_ts_error;
      if ( !SEG_DESC_IS_PRESENT ( desc ) ) goto excp_np_error;
      // Carrega
      load_segment_descriptor ( reg, selector, &desc );
      set_segment_descriptor_access_bit ( INTERP, &desc );
    }
  else if ( reg == P_SS ) // El SS és especial
    {
      // Comprovacions
      if ( (selector&0xFFFC) == 0 ) // Null segment
        goto excp_ts_error;
      if ( int_read_segment_descriptor ( INTERP, selector,
                                         &desc, ext, true ) != 0 )
        return -1;
      dpl= SEG_DESC_GET_DPL ( desc );
      cpl= CPL;
      rpl= selector&0x3;
      stype= SEG_DESC_GET_STYPE ( desc );
      // Nota! No tinc 100% clar que la comprovació des permisos siga
      // correcta. Em base en aquestes condicions taula 7.1:
      //
      //  Stack segment DPL matches CPL
      //  Stack segment DPL matches selector RPL
      if (
          // --> is not a writable data segment
          ((stype&0x1A) != 0x12) ||
          // -->  RPL ≠ CPL
          (rpl != cpl) ||
          // --> DPL ≠ CPL
          (dpl != cpl)
          )
        goto  excp_ts_error;
      if ( !SEG_DESC_IS_PRESENT ( desc ) ) goto excp_ss_error;
      // Carrega
      load_segment_descriptor ( reg, selector, &desc );
      set_segment_descriptor_access_bit ( INTERP, &desc );
    }
  else
    {
      if ( (selector&0xFFFC) == 0 ) // Null segment
        {
          reg->v= selector;
          reg->h.isnull= true;
        }
      else 
        {
          if ( int_read_segment_descriptor ( INTERP, selector,
                                             &desc, ext, true ) != 0 )
            return -1;
          // NOTA!!! ENTENC QUE NO CAL COMPROVAR CPL I DPL (O SÍ DEL
          //         NOU CPL?????)
          // Type checking and protecttion
          dpl= SEG_DESC_GET_DPL ( desc );
          cpl= CPL;
          rpl= selector&0x3;
          stype= SEG_DESC_GET_STYPE ( desc );
          if (
              // --> is not a data or readable code segment
              !(((stype&0x18) == 0x10) || ((stype&0x1A) == 0x1A))
              ||
              // --> ((segment is a ¿¿data?? or nonconforming code segment)
              //      and ((RPL > DPL) or (CPL > DPL)))
              ((((stype&0x18) == 0x10) || ((stype&0x1C) == 0x18)) &&
               ((rpl > dpl) || (cpl > dpl)))
              )
            goto excp_ts_error;
          // Segment-present flag.
          if ( !SEG_DESC_IS_PRESENT ( desc ) ) goto excp_np_error;
          // Carrega
          load_segment_descriptor ( reg, selector, &desc );
          set_segment_descriptor_access_bit ( INTERP, &desc );
        }
    }
  
  return 0;

 excp_ts_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
 excp_np_error:
  EXCEPTION_ERROR_CODE ( EXCP_NP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
 excp_ss_error:
  EXCEPTION_ERROR_CODE ( EXCP_SS,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
  
} // end switch_task_set_sreg  


#define SWITCH_TASK_JMP  0x01
#define SWITCH_TASK_CALL 0x02
#define SWITCH_TASK_IRET 0x04
// Excepció
#define SWITCH_TASK_EXC  0x08
// Interrupció Software
#define SWITCH_TASK_INT  0x10

static int
switch_task (
             PROTO_INTERP,
             const int      type,
             const uint16_t selector,
             const uint8_t  ext
             )
{
  
  uint8_t dpl,tmp8;
  uint16_t tmp16;
  uint32_t stype,stype_tmp,off,rpl,tmp32,addr,eflags_tmp,elimit;
  seg_desc_t desc,desc_tmp;
  IA32_SegmentRegister tmp_seg;
  
  
  // TSS Descriptor
  off= (uint32_t) (selector&0xFFF8);
  if ( off < GDTR.firstb || (off+7) > GDTR.lastb )
    {
      if ( type&SWITCH_TASK_IRET ) goto excp_ts_error;
      else                         goto excp_gp_error;
    }
  desc.addr= GDTR.addr + off;
  READLD ( desc.addr, &(desc.w1), true, true, return -1 );
  READLD ( desc.addr+4, &(desc.w0), true, true, return -1 );
  stype= SEG_DESC_GET_STYPE ( desc );
  // En cap llo diu que es tinga que comprovar que el tipus de
  // selector en un IRT siga codi o qualsevol altra cosa.
  if ( type != SWITCH_TASK_IRET &&
       (stype&0x17) != 0x01 ) goto excp_gp_error; // Comprova és TSS???
  
  // Comprovacions TSS descriptor
  // --> Permissos
  if ( type&(SWITCH_TASK_JMP|SWITCH_TASK_CALL|SWITCH_TASK_INT) )
    {
      dpl= SEG_DESC_GET_DPL(desc);
      rpl= selector&0x3;
      if ( dpl < CPL || rpl < CPL ) goto excp_ts_error;
    }
  // --> TSS present i límit
  if ( !SEG_DESC_IS_PRESENT(desc) ) goto excp_np_error;
  load_segment_descriptor ( &tmp_seg, selector, &desc );
  tmp32= tmp_seg.h.lim.lastb - tmp_seg.h.lim.firstb + 1;
  if ( tmp32 < 0x67 ) goto excp_ts_error;
  // --> Comprova available/busy
  if ( type&(SWITCH_TASK_JMP|SWITCH_TASK_CALL|SWITCH_TASK_INT|SWITCH_TASK_EXC) )
    {
      if ( (stype&0x02) == 0x02 ) goto excp_gp_error; // Busy
    }
  else // IRET
    {
      if ( (stype&0x02) != 0x02 ) goto excp_gp_error; // Available
    }
  
  // Checks that the current (old) TSS, new TSS, and all segment
  // descriptors used in the task switch are paged into system memory. ?????
  // NOTA!! No tinc clar què he de fer ací.
  // --> old TSS
  READLB ( TR.h.lim.addr + TR.h.lim.firstb, &tmp8, true, return -1 );
  READLB ( TR.h.lim.addr + TR.h.lim.lastb, &tmp8, true, return -1 );
  // --> new TSS
  READLB ( tmp_seg.h.lim.addr + tmp_seg.h.lim.firstb, &tmp8, true, return -1 );
  READLB ( tmp_seg.h.lim.addr + tmp_seg.h.lim.lastb, &tmp8, true, return -1 );
  
  // Canvia valor Busy TSS Descriptor vell
  addr= GDTR.addr + (uint32_t) (TR.v&0xFFF8);
  READLD ( addr+4, &tmp32, true, true, return -1 );
  if ( type&(SWITCH_TASK_JMP|SWITCH_TASK_IRET) )
    tmp32&= ~(0x00000200); // --> Available
  else // INT, EXC o CALL
    tmp32|= 0x00000200;    // --> Busy
  WRITELD ( addr+4, tmp32, return -1 );
  
  // NT Flag en EFLAGS que es salva
  eflags_tmp= EFLAGS;
  if ( type&SWITCH_TASK_IRET )
    eflags_tmp&= ~NT_FLAG;
  
  // Desa estat en TSS
  assert ( TR.h.lim.firstb == 0 ); // En TR no tendria sentit que no
                                   // ho fora, crec.....
  assert ( TR.h.tss_is32 ); // No sé què fer si és de 16 bits.
  WRITELD ( TR.h.lim.addr + 32, EIP, return -1 );
  WRITELD ( TR.h.lim.addr + 36, eflags_tmp, return -1 );
  WRITELD ( TR.h.lim.addr + 40, EAX, return -1 );
  WRITELD ( TR.h.lim.addr + 44, ECX, return -1 );
  WRITELD ( TR.h.lim.addr + 48, EDX, return -1 );
  WRITELD ( TR.h.lim.addr + 52, EBX, return -1 );
  WRITELD ( TR.h.lim.addr + 56, ESP, return -1 );
  WRITELD ( TR.h.lim.addr + 60, EBP, return -1 );
  WRITELD ( TR.h.lim.addr + 64, ESI, return -1 );
  WRITELD ( TR.h.lim.addr + 68, EDI, return -1 );
  WRITELW ( TR.h.lim.addr + 72, P_ES->v, return -1 );
  WRITELW ( TR.h.lim.addr + 76, P_CS->v, return -1 );
  WRITELW ( TR.h.lim.addr + 80, P_SS->v, return -1 );
  WRITELW ( TR.h.lim.addr + 84, P_DS->v, return -1 );
  WRITELW ( TR.h.lim.addr + 88, P_FS->v, return -1 );
  WRITELW ( TR.h.lim.addr + 92, P_GS->v, return -1 );

  // Llig EFLAGS, fixa NT del nou TSS i desa previous link
  assert ( tmp_seg.h.lim.firstb == 0 ); // En TR no tendria sentit que
                                        // no ho fora, crec.....
  assert ( tmp_seg.h.tss_is32 ); // No sé què fer si és de 16 bits.
  READLD ( tmp_seg.h.lim.addr + 36, &eflags_tmp, true, true, return -1 );
  if ( type&(SWITCH_TASK_CALL|SWITCH_TASK_EXC|SWITCH_TASK_INT) )
    {
      WRITELW ( tmp_seg.h.lim.addr, TR.v, return -1 );
      eflags_tmp|= NT_FLAG;
    }
  
  // Canvia valor Busy TSS Descriptor nou
  if ( type&(SWITCH_TASK_CALL|SWITCH_TASK_JMP|SWITCH_TASK_EXC|SWITCH_TASK_INT) )
    {
      desc.w0|= 0x00000200; // --> Busy
      WRITELD ( desc.addr+4, desc.w0, return -1 );
    }
  
  // Fixa TR i carrega TSS
  TR= tmp_seg;
  // --> LDTR
  READLW ( TR.h.lim.addr + 96, &tmp16, true, true, return -1 );
  if ( (tmp16&0xFFFC) == 0 ) { LDTR.h.isnull= true; LDTR.v= tmp16; }
  else
    {
      if ( int_read_segment_descriptor ( INTERP, tmp16, &desc_tmp,
                                         ext, true ) != 0 )
        return -1;
      LDTR.h.isnull= false;
      /* <-- No faig comprovacions adiccionals
      cpl= CPL;
      if ( cpl != 0 ) goto gp_exc;
      */
      stype_tmp= SEG_DESC_GET_STYPE ( desc_tmp );
      // NOTA!!! no comprova SegmentDescriptor(DescriptorType) (flag
      // S) sols comprova SegmentDescriptor(Type)
      if ( (stype_tmp&0xf) != 0x02 ) goto excp_gp_sel_ldtr;
      if ( !SEG_DESC_IS_PRESENT ( desc_tmp ) )
        {
          printf("TODO 0 - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
          EXCEPTION_SEL ( EXCP_NP, tmp16 );
          return -1;
        }
      // Carrega (No faig el que es fa normalment perquè es LDTR, sols
      // el rellevant per a LDTR).
      LDTR.h.lim.addr=
        (desc_tmp.w1>>16) |
        ((desc_tmp.w0<<16)&0x00FF0000) |
        (desc_tmp.w0&0xFF000000);
      elimit= (desc_tmp.w0&0x000F0000) | (desc_tmp.w1&0x0000FFFF);
      if ( SEG_DESC_G_IS_SET ( desc_tmp ) )
        {
          elimit<<= 12;
          elimit|= 0xFFF;
        }
      LDTR.v= tmp16;
      LDTR.h.lim.firstb= 0;
      LDTR.h.lim.lastb= elimit;
    }
  READLD ( TR.h.lim.addr + 32, &EIP, true, true, return -1 );
  // Ja ho faig abans i ho assigne al final
  //READLD ( TR.h.lim.addr + 36, &EFLAGS, true, true, return -1 );
  READLD ( TR.h.lim.addr + 40, &EAX, true, true, return -1 );
  READLD ( TR.h.lim.addr + 44, &ECX, true, true, return -1 );
  READLD ( TR.h.lim.addr + 48, &EDX, true, true, return -1 );
  READLD ( TR.h.lim.addr + 52, &EBX, true, true, return -1 );
  READLD ( TR.h.lim.addr + 56, &ESP, true, true, return -1 );
  READLD ( TR.h.lim.addr + 60, &EBP, true, true, return -1 );
  READLD ( TR.h.lim.addr + 64, &ESI, true, true, return -1 );
  READLD ( TR.h.lim.addr + 68, &EDI, true, true, return -1 );
  // Primer CS (per si es fan comprovacions)
  READLW ( TR.h.lim.addr + 76, &tmp16, true, true, return -1 );
  if ( switch_task_set_sreg ( INTERP, tmp16, P_CS, ext ) != 0 )
    return -1;
  READLW ( TR.h.lim.addr + 80, &tmp16, true, true, return -1 );
  if ( switch_task_set_sreg ( INTERP, tmp16, P_SS, ext ) != 0 )
    return -1;
  READLW ( TR.h.lim.addr + 72, &tmp16, true, true, return -1 );
  if ( switch_task_set_sreg ( INTERP, tmp16, P_ES, ext ) != 0 )
    return -1;
  READLW ( TR.h.lim.addr + 84, &tmp16, true, true, return -1 );
  if ( switch_task_set_sreg ( INTERP, tmp16, P_DS, ext ) != 0 )
    return -1;
  READLW ( TR.h.lim.addr + 88, &tmp16, true, true, return -1 );
  if ( switch_task_set_sreg ( INTERP, tmp16, P_FS, ext ) != 0 )
    return -1;
  READLW ( TR.h.lim.addr + 92, &tmp16, true, true, return -1 );
  if ( switch_task_set_sreg ( INTERP, tmp16, P_GS, ext ) != 0 )
    return -1;
  
  // CR3 (Sols es carrega si està activada la paginació)
  READLD ( TR.h.lim.addr + 28, &tmp32, true, true, return -1 );
  if ( CR0&CR0_PG )
    CR3= (CR3&(~CR3_PDB)) | (tmp32&CR3_PDB);

  // Consolida coses
  EFLAGS= eflags_tmp | EFLAGS_1S;
  
  return 0;

 excp_gp_sel_ldtr:
  printf("TODO 1 - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
  EXCEPTION_SEL ( EXCP_GP, tmp16 );
  return -1;
 excp_np_error:
  printf("TODO 2 - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
  EXCEPTION_ERROR_CODE ( EXCP_NP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
 excp_gp_error:
  printf("TODO 3 - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
  EXCEPTION_ERROR_CODE ( EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
 excp_ts_error:
  printf("TODO 4 - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
  
} // end switch_task


static int
interruption_real (
                   PROTO_INTERP,
                   const uint8_t vec
                   )
{

  uint32_t offset,aux1,aux2,aux3;
  uint16_t tmp16;
  
  
  offset= vec<<2;
  if ( offset+3 > IDTR.lastb ) { EXCEPTION ( EXCP_GP ); return -1; }
  // Comprovacions abans de fer push de res
  if ( P_SS->h.is32 )
    {
      aux1= (ESP-2);
      aux2= (ESP-4);
      aux3= (ESP-6);
    }
  else
    {
      aux1= (uint32_t) ((uint16_t) (SP-2));
      aux2= (uint32_t) ((uint16_t) (SP-4));
      aux3= (uint32_t) ((uint16_t) (SP-6));
    }
  if ( aux1 > 0xFFFE || aux2 > 0xFFFE || aux3 > 0xFFFE )
    { EXCEPTION ( EXCP_SS ); return -1; }
  PUSHW ( ((uint16_t) EFLAGS), return -1 );
  EFLAGS&= ~(IF_FLAG|TF_FLAG|AC_FLAG);
  PUSHW ( P_CS->v, return -1 );
  PUSHW ( ((uint16_t) (EIP)), return -1 );
  offset+= IDTR.addr;
  READLW ( offset, &tmp16, true, true, return -1 );
  EIP= (uint32_t) tmp16;
  READLW ( offset+2, &tmp16, true, true, return -1 );
  P_CS->v= tmp16;
  P_CS->h.lim.addr= ((uint32_t) tmp16)<<4;
  P_CS->h.lim.firstb= 0;
  P_CS->h.lim.lastb= 0xFFFF;
  P_CS->h.is32= false;
  P_CS->h.readable= true;
  P_CS->h.writable= true;
  P_CS->h.executable= true;
  P_CS->h.isnull= false;
  P_CS->h.data_or_nonconforming= false;
  
  return 0;
  
} // end interruption_real


static int
interruption_protected_intra_pl (
                                 PROTO_INTERP,
                                 const uint32_t    idt_w0,
                                 const uint32_t    idt_w1,
                                 const uint16_t    selector,
                                 seg_desc_t       *desc,
                                 const uint8_t     ext,
                                 const uint16_t    error_code,
                                 const bool        use_error_code
                                 )
{

  int nbytes;
  uint32_t offset,idt_offset;
  IA32_SegmentRegister tmp_seg;
  bool idt_is32,idt_is_intgate;
  
  
  // NOTA!!! Assumisc EFER.LMA==0, aquest bi sols té sentit en 64bits.

  // Comprova espai en la pila.
  if ( SEG_DESC_B_IS_SET(*desc) ) // 32-bit gate
    nbytes= use_error_code ? 16 : 12;
  else // 16-bit gate
    nbytes= use_error_code ? 8 : 6;
  offset= P_SS->h.is32 ? (ESP-nbytes) : ((uint32_t) ((uint16_t) (SP-nbytes)));
  if ( offset < P_SS->h.lim.firstb || offset > (P_SS->h.lim.lastb-(nbytes-1)) )
    { EXCEPTION_ERROR_CODE ( EXCP_SS, ext ); return -1; }

  // Carrega descriptor codi i comprova IDT
  load_segment_descriptor ( &tmp_seg, (selector&0xFFFC)|CPL, desc );
  idt_offset= (idt_w1&0xFFFF0000) | (idt_w0&0x0000FFFF);
  if ( idt_offset < tmp_seg.h.lim.firstb || idt_offset > tmp_seg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return -1; }

  // Desa valors en la pila
  idt_is32= (idt_w1&0x800)!=0;
  if ( idt_is32 )
    {
      // NOTA!! En principi no deuria fallar el PUSH perquè ja he
      // comprovat abans
      PUSHD ( EFLAGS, return -1 );
      PUSHD ( (uint32_t) P_CS->v, return -1 );
      PUSHD ( EIP, return -1 );
      set_segment_descriptor_access_bit ( INTERP, desc );
      *P_CS= tmp_seg;
      EIP= idt_offset;
      if ( use_error_code ) { PUSHD ( (uint32_t) error_code, return -1 ); }
    }
  else
    {
      PUSHW ( (uint16_t) EFLAGS, return -1 );
      PUSHW ( P_CS->v, return -1 );
      PUSHW ( (uint16_t) EIP, return -1 );
      set_segment_descriptor_access_bit ( INTERP, desc );
      *P_CS= tmp_seg;
      EIP= idt_offset&0xFFFF; // <-- He de descartar la part de dalt???
      if ( use_error_code ) { PUSHW ( error_code, return -1 ); }
    }

  // Flags
  idt_is_intgate= (idt_w1&0x7E0)==0x600;
  if ( idt_is_intgate ) { EFLAGS&= ~(IF_FLAG|TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  else { EFLAGS&= ~(TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  
  return 0;
  
} // end interruption_protected_intra_pl


static int
interruption_protected_from_vm86 (
                                  PROTO_INTERP,
                                  const uint32_t    idt_w0,
                                  const uint32_t    idt_w1,
                                  const uint16_t    selector,
                                  seg_desc_t       *desc,
                                  const uint8_t     ext,
                                  const uint16_t    error_code,
                                  const bool        use_error_code
                                  )
{

  int nbytes;
  uint8_t new_SS_dpl;
  uint16_t new_SS_selector,tmp16,new_SP,tmp_SS_selector;
  uint32_t new_ESP,new_SS_stype,offset,idt_offset,tmp_eflags,tmp_ESP;
  seg_desc_t new_SS_desc;
  int new_SS_rpl;
  IA32_SegmentRegister new_SS,tmp_seg;
  bool idt_is_intgate,idt_is32;
  
  
  // Identify stack-segment selector for privilege level 0 in current TSS
  if ( TR.h.tss_is32 )
    {
      if ( TR.h.lim.lastb < 9 ) goto excp_ts_error;
      READLW ( TR.h.lim.addr + 8, &new_SS_selector, true, true, return -1);
      READLD ( TR.h.lim.addr + 4, &new_ESP, true, true, return -1);
    }
  else // TSS is 16-bit
    {
      if ( TR.h.lim.lastb < 5 ) goto excp_ts_error;
      READLW ( TR.h.lim.addr + 4, &new_SS_selector, true, true, return -1);
      READLW ( TR.h.lim.addr + 2, &tmp16, true, true, return -1);
      new_ESP= (uint32_t) tmp16;
    }
  new_SP= (uint16_t) (new_ESP&0xFFFF);
  
  // Llig newSS
  if ( int_read_segment_descriptor ( INTERP, new_SS_selector,
                                     &new_SS_desc, ext, true ) != 0 )
    return -1;
  // Comprovacions newSS addicionals i carrega'l
  new_SS_rpl= new_SS_selector&0x3;
  if ( new_SS_rpl != 0 ) goto excp_ts_new_ss_error;
  new_SS_dpl= SEG_DESC_GET_DPL(new_SS_desc);
  new_SS_stype= SEG_DESC_GET_STYPE ( new_SS_desc );
  if ( new_SS_dpl != 0 ||
       (new_SS_stype&0x1A) != 0x12 || // no és data, writable
       !SEG_DESC_IS_PRESENT(new_SS_desc) )
    goto excp_ts_new_ss_error;
  load_segment_descriptor ( &new_SS, new_SS_selector, &new_SS_desc );
  
  // Comprova espai en la nova pila.
  if ( SEG_DESC_B_IS_SET(*desc) ) // 32-bit gate
    nbytes= use_error_code ? 40 : 36;
  else // 16-bit gate
    nbytes= use_error_code ? 20 : 18;
  offset= new_SS.h.is32 ?
    (new_ESP-nbytes) : ((uint32_t) ((uint16_t) (new_SP-nbytes)));
  if ( offset < new_SS.h.lim.firstb ||
       offset > (new_SS.h.lim.lastb-(nbytes-1)) )
    {
      EXCEPTION_ERROR_CODE ( EXCP_SS,
                             int_error_code ( new_SS_selector&0xFFFC,
                                              0, ext ) );
      return -1;
    }
  
  // Carrega descriptor codi i comprova IDT
  load_segment_descriptor ( &tmp_seg, selector, desc );
  idt_offset= (idt_w1&0xFFFF0000) | (idt_w0&0x0000FFFF);
  if ( idt_offset < tmp_seg.h.lim.firstb || idt_offset > tmp_seg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return -1; }
  
  // Flags
  tmp_eflags= EFLAGS;
  idt_is_intgate= (idt_w1&0x7E0)==0x600;
  if ( idt_is_intgate ) { EFLAGS&= ~(IF_FLAG|TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  else { EFLAGS&= ~(TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  
  // Fixa nova pila.
  tmp_SS_selector= P_SS->v;
  tmp_ESP= ESP;
  *(P_SS)= new_SS;
  ESP= new_ESP;
  
  // Desa valors en la pila
  idt_is32= (idt_w1&0x800)!=0;
  if ( idt_is32 )
    {
      // NOTA!! En principi no deuria fallar el PUSH perquè ja he
      // comprovat abans
      PUSHD ( (uint32_t) P_GS->v, return -1 );
      PUSHD ( (uint32_t) P_FS->v, return -1 );
      PUSHD ( (uint32_t) P_DS->v, return -1 );
      PUSHD ( (uint32_t) P_ES->v, return -1 );
      PUSHD ( (uint32_t) tmp_SS_selector, return -1 );
      PUSHD ( tmp_ESP, return -1 );
      PUSHD ( tmp_eflags, return -1 );
      PUSHD ( (uint32_t) P_CS->v, return -1 );
      PUSHD ( EIP, return -1 );
      if ( use_error_code ) { PUSHD ( (uint32_t) error_code, return -1 ); }
      /*
      if ( use_error_code )
        {
          printf("[EE] interruption_protected_from_vm86 (B) - CAL IMPLEMENTAR USER_ERROR_CODE????\n");
          exit(EXIT_FAILURE);
        }
      */
    }
  else
    {
      printf("[EE] interruption_protected_from_vm86 (A) - 16!!!!\n");
      exit(EXIT_FAILURE);
    }

  // Fixa a 0 selectors dades
  P_GS->v= 0x0000; P_GS->h.isnull= true;
  P_FS->v= 0x0000; P_FS->h.isnull= true;
  P_DS->v= 0x0000; P_DS->h.isnull= true;
  P_ES->v= 0x0000; P_ES->h.isnull= true;

  // Fixa el CS
  *P_CS= tmp_seg;
  EIP= idt_offset;
  if ( !OPERAND_SIZE_IS_32 ) EIP&= 0xFFFF;
  
  return 0;

 excp_ts_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( TR.v&0xFFFC, 0, ext ) );
  return -1;
 excp_ts_new_ss_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( new_SS_selector&0xFFFC, 0, ext ) );
  return -1;
  
} // end interruption_protected_from_vm86


static int
interruption_protected_inter_pl (
                                 PROTO_INTERP,
                                 const uint32_t    idt_w0,
                                 const uint32_t    idt_w1,
                                 const uint16_t    selector,
                                 seg_desc_t       *desc,
                                 const uint8_t     ext,
                                 const uint16_t    error_code,
                                 const bool        use_error_code
                                 )
{

  int nbytes;
  uint8_t dpl,new_SS_rpl,new_SS_dpl;
  uint16_t new_SS_selector,new_SP,tmp_SS_selector,tmp16;
  uint32_t offset,new_ESP,new_SS_stype,idt_offset,tmp_ESP;
  seg_desc_t new_SS_desc;
  IA32_SegmentRegister new_SS,tmp_seg;
  bool idt_is_intgate,idt_is32;
  
  
  // Identify stack-segment selector for new privilege level in current TSS.
  dpl= SEG_DESC_GET_DPL(*desc);
  assert ( dpl < 3 );
  if ( TR.h.tss_is32 )
    {
      offset= (dpl<<3) + 4;
      if ( (offset+5) > TR.h.lim.lastb ) goto excp_ts_error;
      READLW ( TR.h.lim.addr+offset+4, &new_SS_selector, true, true, return -1);
      READLD ( TR.h.lim.addr+offset, &new_ESP, true, true, return -1 );
    }
  else // TSS is 16-bit
    {
      offset= (dpl<<2) + 2;
      if ( (offset+3) > TR.h.lim.lastb ) goto excp_ts_error;
      READLW ( TR.h.lim.addr+offset+2, &new_SS_selector, true, true, return -1);
      READLW ( TR.h.lim.addr+offset, &tmp16, true, true, return -1);
      new_ESP= (uint32_t) tmp16;
    }
  new_SP= (uint16_t) (new_ESP&0xFFFF);
  
  // Llig newSS
  if ( (new_SS_selector&0xFFFC) == 0 ) // Null segment selector..
    { EXCEPTION_ERROR_CODE ( EXCP_TS, ext ); return -1; };
  if ( int_read_segment_descriptor ( INTERP, new_SS_selector,
                                     &new_SS_desc, ext, true ) != 0 )
    return -1;
  // Comprovacions newSS addicionals i carrega'l
  new_SS_rpl= new_SS_selector&0x3;
  if ( new_SS_rpl != dpl ) goto excp_ts_new_ss_error;
  new_SS_dpl= SEG_DESC_GET_DPL(new_SS_desc);
  new_SS_stype= SEG_DESC_GET_STYPE ( new_SS_desc );
  if ( new_SS_dpl != dpl ||
       (new_SS_stype&0x1A) != 0x12 || // no és data, writable
       !SEG_DESC_IS_PRESENT(new_SS_desc) )
    goto excp_ts_new_ss_error;
  load_segment_descriptor ( &new_SS, new_SS_selector, &new_SS_desc );

  // Comprova espai en la nova pila.
  if ( SEG_DESC_B_IS_SET(*desc) ) // 32-bit gate
    nbytes= use_error_code ? 24 : 20;
  else // 16-bit gate
    nbytes= use_error_code ? 12 : 10;
  offset= new_SS.h.is32 ?
    (new_ESP-nbytes) : ((uint32_t) ((uint16_t) (new_SP-nbytes)));
  if ( offset < new_SS.h.lim.firstb ||
       offset > (new_SS.h.lim.lastb-(nbytes-1)) )
    {
      EXCEPTION_ERROR_CODE ( EXCP_SS,
                             int_error_code ( new_SS_selector&0xFFFC,
                                              0, ext ) );
      return -1;
    }

  // Carrega descriptor codi i comprova IDT
  load_segment_descriptor ( &tmp_seg, selector, desc );
  idt_offset= (idt_w1&0xFFFF0000) | (idt_w0&0x0000FFFF);
  if ( idt_offset < tmp_seg.h.lim.firstb || idt_offset > tmp_seg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return -1; }

  // Fixa nova pila.
  tmp_SS_selector= P_SS->v;
  tmp_ESP= ESP;
  *(P_SS)= new_SS;
  ESP= new_ESP;
  
  // Desa valors en la pila
  idt_is32= (idt_w1&0x800)!=0;
  if ( idt_is32 )
    {
      // NOTA!! En principi no deuria fallar el PUSH perquè ja he
      // comprovat abans
      PUSHD ( (uint32_t) tmp_SS_selector, return -1 );
      PUSHD ( tmp_ESP, return -1 );
      PUSHD ( EFLAGS, return -1 );
      PUSHD ( (uint32_t) P_CS->v, return -1 );
      PUSHD ( EIP, return -1 );
      if ( use_error_code ) { PUSHD ( (uint32_t) error_code, return -1 ); }
    }
  else
    {
      PUSHW ( tmp_SS_selector, return -1 );
      PUSHW ( (uint16_t) tmp_ESP, return -1 );
      PUSHW ( (uint16_t) EFLAGS, return -1 );
      PUSHW ( P_CS->v, return -1 );
      PUSHW ( (uint16_t) EIP, return -1 );
      if ( use_error_code ) { PUSHW ( error_code, return -1 ); }
    }

  // Fixa el CS, EIP i EFLAGS.
  tmp_seg.h.pl= dpl; // CPL ← new code-segment DPL;
  tmp_seg.v= (tmp_seg.v&0xFFFC) | dpl; // CS(RPL) ← CPL;
  idt_is_intgate= (idt_w1&0x7E0)==0x600;
  if ( idt_is_intgate ) { EFLAGS&= ~(IF_FLAG|TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  else { EFLAGS&= ~(TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  *P_CS= tmp_seg;
  EIP= idt_offset;
  if ( !OPERAND_SIZE_IS_32 ) EIP&= 0xFFFF;
  
  return 0;
  
 excp_ts_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( TR.v&0xFFFC, 0, ext ) );
  return -1;
 excp_ts_new_ss_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( new_SS_selector&0xFFFC, 0, ext ) );
  return -1;
  
} // end interruption_protected_inter_pl


static int
interruption_protected_trap_int_gate (
                                      PROTO_INTERP,
                                      const int      type,
                                      const uint8_t  vec,
                                      const uint32_t d_w0,
                                      const uint32_t d_w1,
                                      const uint8_t  ext,
                                      const uint16_t error_code,
                                      const bool     use_error_code
                                      )
{

  uint8_t dpl;
  uint16_t selector;
  uint32_t stype;
  seg_desc_t desc;
  
  
  // Llig descriptor
  selector= (uint16_t) (d_w0>>16);
  if ( int_read_segment_descriptor ( INTERP, selector,
                                     &desc, ext, false ) != 0 )
    return -1;

  // Comprovacions preliminars
  dpl= SEG_DESC_GET_DPL(desc);
  stype= SEG_DESC_GET_STYPE ( desc );
  if ( (stype&0x18) != 0x18 || dpl > CPL )
    goto excp_gp_error;
  if ( !SEG_DESC_IS_PRESENT(desc) ) goto excp_np_error;
  
  // En funció del tipus...
  // IF new code segment is non-conforming with DPL < CPL
  if ( (stype&0x04) == 0 && dpl < CPL )
    {
      // PE = 1, VM = 0, interrupt or trap gate, nonconforming code
      // segment, DPL < CPL *)
      if ( (EFLAGS&VM_FLAG) == 0 )
        {
          if ( interruption_protected_inter_pl ( INTERP, d_w0, d_w1,
                                                 selector, &desc, ext,
                                                 error_code,
                                                 use_error_code ) != 0 )
            return -1;
        }
      else // VM==1
        { 
          if ( dpl != 0 ) goto excp_gp_error;
          else
            {
              if ( interruption_protected_from_vm86 ( INTERP, d_w0, d_w1,
                                                      selector, &desc, ext,
                                                      error_code,
                                                      use_error_code ) != 0 )
                return -1;
            }
        }
    }
  // ELSE (* PE = 1, interrupt or trap gate, DPL ≥ CPL *)
  else
    {
      if ( (EFLAGS&VM_FLAG) != 0 ) goto excp_gp_error;
      // IF new code segment is conforming or new code-segment DPL = CPL
      if ( (stype&0x04) != 0 || dpl == CPL )
        {
          if ( interruption_protected_intra_pl ( INTERP, d_w0, d_w1,
                                                 selector, &desc, ext,
                                                 error_code,
                                                 use_error_code ) != 0 )
            return -1;
        }
      // PE = 1, interrupt or trap gate, nonconforming code segment, DPL > CPL
      else goto excp_gp_error;
    }
  
  return 0;

 excp_gp_error:
  EXCEPTION_ERROR_CODE ( EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
 excp_np_error:
  EXCEPTION_ERROR_CODE ( EXCP_NP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
  
} // end interruption_protected_trap_int_gate


static int
interruption_protected_task_gate (
                                  PROTO_INTERP,
                                  const int      type,
                                  const uint8_t  vec,
                                  const uint32_t d_w0,
                                  const uint32_t d_w1,
                                  const uint8_t  ext,
                                  const uint16_t error_code,
                                  const bool     use_error_code
                                  )
{

  uint16_t selector;
  
  
  // TSS Selector
  selector= (uint16_t) (d_w0>>16);
  if ( (selector&0xFFFC) == 0 ) goto excp_gp_error;
  if ( selector&0x4 ) goto excp_gp_error; // LDT

  // Switch task (Inclou comprovació bit busy i si està o no present)
  if ( switch_task ( INTERP,
                     ext==0 ? SWITCH_TASK_INT : SWITCH_TASK_EXC,
                     selector, ext ) != 0 )
    return -1;

  // Comprovacions finals.
  if ( use_error_code ) { PUSHD ( (uint32_t) error_code, return -1 ); }
  if ( EIP < P_CS->h.lim.firstb || EIP > P_CS->h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return -1; }
  
  return 0;
  
 excp_gp_error:
  EXCEPTION_ERROR_CODE ( EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return -1;
  
} // end interruption_protected_task_gate


static int
interruption_protected (
                        PROTO_INTERP,
                        const int      type,
                        const uint8_t  vec,
                        const uint16_t error_code,
                        const bool     use_error_code
                        )
{

  uint32_t offset,d_w0,d_w1;
  uint8_t ext,dpl;
  
  
  if ( INTERRUPTION_TYPE_IMM && (EFLAGS&VM_FLAG) && IOPL < 3 )
    goto excp_gp;
  // NOTA!!! Assumisc EFER.LMA==0, aquest bi sols té sentit en 64bits.
  
  // Valor ext per defecte
  ext=
    ((type==INTERRUPTION_TYPE_IMM) ||
     (type==INTERRUPTION_TYPE_INT3) ||
     (type==INTERRUPTION_TYPE_INTO)) ?
    0 : 1;
  
  // Llig descriptor
  offset= vec<<3;
  if ( offset+7 > IDTR.lastb ) goto excp_gp_error;
  offset+= IDTR.addr;
  READLD ( offset, &d_w0, true, true, return -1 );
  READLD ( offset+4, &d_w1, true, true, return -1 );
  // --> Comprova tipus
  if ( !(((d_w1&0x1F00) == 0x0500) || // Task Gate
        ((d_w1&0x17E0) == 0x0600) || // Interrupt gate
        ((d_w1&0x17E0) == 0x0700)) // Trap Gate
       )
    goto excp_gp_error;
  
  // Comprovacions adicionals software interrupts.
  if ( ext == 0 ) // Software interrupt
    {
      dpl= (d_w1>>13)&0x3;
      if ( dpl < CPL ) goto excp_gp_error;
    }
  
  // Check gate no present
  if ( (d_w1&0x00008000) == 0 ) goto excp_np_error;
  
  // Depen del gate fes
  if ( (d_w1&0x1F00) == 0x0500 )
    {
      if ( interruption_protected_task_gate ( INTERP, type, vec,
                                              d_w0, d_w1, ext,
                                              error_code,
                                              use_error_code ) != 0 )
        return -1;
    }
  else
    {
      if ( interruption_protected_trap_int_gate ( INTERP, type, vec,
                                                  d_w0, d_w1, ext,
                                                  error_code,
                                                  use_error_code ) != 0 )
        return -1;
    }
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
 excp_gp_error:
  EXCEPTION_ERROR_CODE ( EXCP_GP, int_error_code ( vec, 1, ext ) );
  return -1;
 excp_np_error:
  EXCEPTION_ERROR_CODE ( EXCP_NP, int_error_code ( vec, 1, ext ) );
  return -1;
  
} // end interruption_protected


static int
interruption (
              PROTO_INTERP,
              const int      type,
              const uint8_t  vec,
              const uint16_t error_code,
              const bool     use_error_code
              )
{

  int ret;


  ret= 0;
  if ( (INTERP)->_int_counter > MAX_NESTED_INT )
    {
      WW ( UDATA, "ignorant interrupció [evoc:%02X error:%04X (%d)]:"
           " s'ha assolit el nombre màxim d'interrupcions",
           vec, error_code, use_error_code );
      return 0;
    }
  ++((INTERP)->_int_counter);
  INTERP->_halted= false;
  if ( !PROTECTED_MODE_ACTIVATED )
    {
      ret= interruption_real ( INTERP, vec );
      --((INTERP)->_int_counter);
    }
  else
    {
      ret= interruption_protected ( INTERP, type, vec,
                                    error_code, use_error_code );
      --((INTERP)->_int_counter);
    }
  
  return ret;
  
} // end interruption


/* IO *************************************************************************/
// Torna cert si té permís.
static bool
check_io_permission_bit_map (
                             PROTO_INTERP,
                             const uint16_t port,
                             const int      nbits
                             )
{

  uint32_t addr;
  uint16_t offset;
  uint8_t bmap;
  int n,bit;
  
  
  // I/O Map Base
  if ( !TR.h.tss_is32 )
    {
      WW ( UDATA, "check_io_permission_bit_map - TSS 16bit no"
           " suporta I/O permission bit map" );
      return false;
    }
  if ( TR.h.lim.lastb < 103 ) return false;
  READLW ( TR.h.lim.addr + 102, &offset, true, true, return false);

  // Comprova els bits
  bit= port&0x7;
  addr= ((uint32_t) offset) + (port>>3);
  if ( addr < TR.h.lim.firstb || addr > TR.h.lim.lastb  ) return false;
  READLB ( TR.h.lim.addr + addr, &bmap, true, return false);
  for ( n= 0; n < nbits; ++n )
    {
      if ( bmap&(1<<bit) ) return false; // bit 1 indica que no es pot
      if ( ++bit == 8 && n != nbits-1 )
        {
          ++addr;
          bit= 0;
          if ( addr < TR.h.lim.firstb || addr > TR.h.lim.lastb  ) return false;
          READLB ( TR.h.lim.addr + addr, &bmap, true, return false);
        }
    }

  return true;
  
} // end check_io_permission_bit_map




/* MEMÒRIA ********************************************************************/
/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_readl8 (
            PROTO_INTERP,
            const uint32_t  addr,
            uint8_t        *dst,
            const bool      reading_data
            )
{

  *dst= READU8 ( (uint64_t) addr );

  return 0;
  
} /* end mem_readl8 */


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_writel8 (
             PROTO_INTERP,
             const uint32_t addr,
             const uint8_t  data
             )
{

  WRITEU8 ( (uint64_t) addr, data );
  
  return 0;
  
} /* end mem_writel8 */


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_readl16 (
             PROTO_INTERP,
             const uint32_t  addr,
             uint16_t       *dst,
             const bool      reading_data,
             const bool      implicit_svm
             )
{

  uint64_t laddr;


  laddr= (uint64_t) addr;
  if ( laddr&0x1 )
    *dst=
      ((uint16_t) READU8 ( laddr )) |
      (((uint16_t) READU8 ( laddr+1 ))<<8);
  else *dst= READU16 ( laddr );

  return 0;
  
} /* end mem_readl16 */


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_writel16 (
              PROTO_INTERP,
              const uint32_t addr,
              const uint16_t data
              )
{

  uint64_t laddr;


  laddr= (uint64_t) addr;
  if ( laddr&0x1 )
    {
      WRITEU8 ( laddr, data&0xFF );
      WRITEU8 ( laddr+1, data>>8 );
    }
  else WRITEU16 ( laddr, data );
  
  return 0;
  
} /* end mem_writel16 */


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_readl32 (
             PROTO_INTERP,
             const uint32_t  addr,
             uint32_t       *dst,
             const bool      reading_data,
             const bool      implicit_svm
             )
{

  uint64_t laddr;


  laddr= (uint64_t) addr;
  if ( laddr&0x3 )
    {
      if ( laddr&0x1 )
        *dst=
          ((uint32_t) READU8 ( laddr )) |
          (((uint32_t) READU8 ( laddr+1 ))<<8) |
          (((uint32_t) READU8 ( laddr+2 ))<<16) |
          (((uint32_t) READU8 ( laddr+3 ))<<24);
      else
        *dst=
          ((uint32_t) READU16 ( laddr )) |
          (((uint32_t) READU16 ( laddr+2 ))<<16);
    }
  else *dst= READU32 ( laddr );

  return 0;
  
} // end mem_readl32


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_writel32 (
              PROTO_INTERP,
              const uint32_t addr,
              const uint32_t data
              )
{

  uint64_t laddr;


  laddr= (uint64_t) addr;
  if ( laddr&0x3 )
    {
      if ( laddr&0x1 )
        {
          WRITEU8 ( laddr, data&0xFF );
          WRITEU8 ( laddr+1, (data>>8)&0xFF );
          WRITEU8 ( laddr+2, (data>>16)&0xFF );
          WRITEU8 ( laddr+3, (data>>24)&0xFF );
        }
      else
        {
          WRITEU16 ( laddr, data&0xFFFF );
          WRITEU16 ( laddr+2, data>>16 );
        }
    }
  else WRITEU32 ( laddr, data );
  
  return 0;
  
} /* end mem_writel32 */


static uint64_t
readu64 (
         PROTO_INTERP,
         const uint64_t addr
         )
{
  
  uint64_t ret;
  
  
  if ( addr&0x7 )
    {
      if ( addr&0x3 )
        {
          if ( addr&0x1 )
            ret=
              ((uint64_t) READU8 ( addr )) |
              (((uint64_t) READU8 ( addr+1 ))<<8) |
              (((uint64_t) READU8 ( addr+2 ))<<16) |
              (((uint64_t) READU8 ( addr+3 ))<<24) |
              (((uint64_t) READU8 ( addr+4 ))<<32) |
              (((uint64_t) READU8 ( addr+5 ))<<40) |
              (((uint64_t) READU8 ( addr+6 ))<<48) |
              (((uint64_t) READU8 ( addr+7 ))<<56);
          else
            ret=
              ((uint64_t) READU16 ( addr )) |
              (((uint64_t) READU16 ( addr+2 ))<<16) |
              (((uint64_t) READU16 ( addr+4 ))<<32) |
              (((uint64_t) READU16 ( addr+6 ))<<48);
        }
      else
        ret=
          ((uint64_t) READU32 ( addr )) |
          (((uint64_t) READU32 ( addr+4 ))<<32);
    }
  else ret= READU64 ( addr );
  
  return ret;
  
} /* end readu64 */


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_readl64 (
             PROTO_INTERP,
             const uint32_t  addr,
             uint64_t       *dst
             )
{

  *dst= readu64 ( INTERP, (uint64_t) addr );
  
  return 0;
  
} /* end mem_readl64 */


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_readl128 (
              PROTO_INTERP,
              const uint32_t      addr,
              IA32_DoubleQuadword dst
              )
{

#ifdef IA32_LE
  dst[0]= readu64 ( INTERP, (uint64_t) addr );
  dst[1]= readu64 ( INTERP, ((uint64_t) addr) + (uint64_t) 8 );
#else
  dst[1]= readu64 ( INTERP, (uint64_t) addr );
  dst[0]= readu64 ( INTERP, ((uint64_t) addr) + (uint64_t) 8 );
#endif
  
  return 0;
  
} /* end mem_readl128 */


#define PDE_P       0x00000001
#define PDE_RW      0x00000002
#define PDE_US      0x00000004
#define PDE_PS      0x00000080
#define PDE_PTEADDR 0xFFFFF000

#define PTE_P       0x00000001
#define PTE_RW      0x00000002
#define PTE_US      0x00000004

#define ECODE_P    0x0001
#define ECODE_WR   0x0002
#define ECODE_US   0x0004
#define ECODE_RSVD 0x0008
#define ECODE_ID   0x0010
#define ECODE_PK   0x0020

// ecode ha d'estat inicialitzat. Torna 0 si tot ha anat bé
static int
page32_check_access (
                     PROTO_INTERP,
                     const bool  explicit_svm,
                     const bool  implicit_svm,
                     const bool  svm_addr,
                     const bool  writing_allowed,
                     const bool  ifetch,
                     const bool  writing
                     )
{

  int ret;

  
  // Supervidor-mode access
  if ( explicit_svm || implicit_svm )
    {
      
      if ( ifetch ) // Instruction fetch
        {
          if ( svm_addr ) ret= 0;
          else ret= (CR4&CR4_SMEP) ? -1 : 0;
        }
      
      else if ( writing ) // Escriptura dades
        {
          if ( svm_addr ) // supervisor-mode address
            {
              if ( CR0&CR0_WP ) ret= writing_allowed ? 0 : -1;
              else ret= 0;
            }
          else // user-mode address
            {
              if ( CR0&CR0_WP )
                {
                  if ( !writing_allowed ) ret= -1;
                  else
                    {
                      if ( (CR4&CR4_SMAP) != 0 )
                        {
                          if ( (EFLAGS&AC_FLAG) && explicit_svm ) ret= 0;
                          else                                    ret= -1;
                        }
                      else ret= 0;
                    }
                }
              else
                {
                  if ( (CR4&CR4_SMAP) != 0 )
                    {
                      if ( (EFLAGS&AC_FLAG) && explicit_svm ) ret= 0;
                      else                                    ret= -1;
                    }
                  else ret= 0;
                }
            }
        }
      else // Lectura dades
        {
          // Data may be read (implicitly or explicitly) from any
          // supervisor-mode address.
          if ( svm_addr ) ret= 0;
          else // user-mode address
            {
              if ( (CR4&CR4_SMAP) != 0 )
                {
                  if ( (EFLAGS&AC_FLAG) && explicit_svm ) ret= 0;
                  else ret= -1;
                }
              // If CR4.SMAP = 0, data may be read from any user-mode
              // address with a protection key for which read access
              // is permitted.
              else ret= 0;
            }
        }
    }
  
  // User-mode access
  else
    {
      if ( ifetch ) ret= 0;
      else if ( writing )
        {
          if ( svm_addr ) ret= -1;
          else if ( !writing_allowed ) ret= -1;
          else ret= 0;
        }
      else // Lectura
        {
          if ( svm_addr ) ret= -1;
          else ret= 0;
        }
    }

  return ret;
  
} // end page32_check_access


// 0 si tot ha anat bé, -1 en cas contrari.
static int
page32_translate_addr (
                       PROTO_INTERP,
                       const uint32_t  addr,
                       uint64_t       *laddr,
                       bool            writing,
                       bool            ifetch,
                       bool            implicit_svm
                       )
{

  uint64_t pde_addr,ret,pte_addr;
  uint32_t pde,pte;
  uint16_t ecode;
  bool pse_enabled,explicit_svm,svm_addr,writing_allowed;
  
  
  // Preparacions.
  ecode= 0;
  explicit_svm= CPL < 3;
  
  // Obté pde i comprovacions
  pde_addr= (uint64_t) ((CR3&CR3_PDB) | ((addr>>22)<<2));
  pde= READU32 ( pde_addr );
  if ( (pde&PDE_P) == 0 ) { ecode= 0; goto error; }
  pse_enabled= (CR4&CR4_PSE)!=0;
  svm_addr= (pde&PDE_US)==0;
  writing_allowed= (pde&PDE_RW)!=0;
  
  // Accedeix en funció grandària.
  if (  pse_enabled && (pde&PDE_PS) != 0 ) // 4MB
    {
      // NOTA!!! ACÍ CAL COMPROVAR RESERVED BITS. ELS RESERVED BITS EN
      // AQUEST CAS ÉS VARIABLE DEPENENT DE LES CARACTERÍSTIQUES DE LA
      // CPU!!!!
      // TODO!!!!!!!
      fprintf ( stderr, "[CAL_IMPLEMENTAR] page32_translate_addr - 4MB\n" );
      exit ( EXIT_FAILURE );
    }
  else // 4KB
    {
      
      // Comprovació reserved bits.
      if ( pse_enabled ) // i PAT 
        {
          fprintf ( stderr, "[CAL_IMPLEMENTAR] page32_translate_addr"
                    " - 4KB comprovació reserved bits\n" );
          exit ( EXIT_FAILURE );
        }

      // Obté pte
      pte_addr= (uint64_t) ((pde&PDE_PTEADDR) | ((addr>>10)&0x00000FFC));
      pte= READU32 ( pte_addr );
      if ( (pte&PTE_P) == 0 ) { ecode= 0; goto error; }
      if ( (pte&PTE_US) == 0 ) svm_addr= true;
      if ( (pte&PTE_RW) == 0 ) writing_allowed= false;
      
      // Comprova permisos d'access
      if ( page32_check_access ( INTERP, explicit_svm, implicit_svm,
                                 svm_addr, writing_allowed, ifetch,
                                 writing ) != 0 )
        {
          ecode= ECODE_P; // page-level protection violation
          goto error;
        }
      
      // Adreça final
      ret= (uint64_t) ((pte&0xFFFFF000) | (addr&0x00000FFF));
      
    }
  *laddr= ret;

  return 0;

 error:
  CR2= addr;
  if ( writing ) ecode|= ECODE_WR;
  if ( ifetch ) ecode|= ECODE_ID;
  if ( !implicit_svm && !explicit_svm ) ecode|= ECODE_US;
  EXCEPTION_ERROR_CODE ( EXCP_PF, ecode );
  return -1;
  
} // end page32_translate_addr


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read8 (
               PROTO_INTERP,
               const uint32_t  addr,
               uint8_t        *dst,
               const bool      reading_data
               )
{

  uint64_t laddr;


  laddr= 0;
  if ( page32_translate_addr ( INTERP, addr, &laddr,
                               false, !reading_data, false ) != 0 )
    return -1;
  *dst= READU8 ( laddr );

  return 0;
  
} // end mem_p32_read8


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_write8 (
                PROTO_INTERP,
                const uint32_t addr,
                const uint8_t  data
                )
{

  uint64_t laddr;


  laddr= 0;
  if ( page32_translate_addr ( INTERP, addr, &laddr, true, false, false ) != 0 )
    return -1;
  WRITEU8 ( laddr, data );
  
  return 0;
  
} // end mem_p32_write8


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read16 (
                PROTO_INTERP,
                const uint32_t  addr,
                uint16_t       *dst,
                const bool      reading_data,
                const bool      implicit_svm
                )
{
  
  uint64_t laddr;
  uint8_t tmp;
  uint16_t data;
  int ret;
  
  
  if ( (addr&0x1) == 0 )
    {
      laddr= 0;
      if ( page32_translate_addr ( INTERP, addr, &laddr,
                                   false, !reading_data,
                                   implicit_svm ) != 0 )
        return -1;
      ret= mem_readl16 ( INTERP, laddr, dst, reading_data, implicit_svm );
    }
  else
    {
      
      // Llig valors
      if ( mem_p32_read8 ( INTERP, addr, &tmp, reading_data ) != 0 )
        return -1;
      data= (uint16_t) tmp;
      if ( mem_p32_read8 ( INTERP, addr+1, &tmp, reading_data ) != 0 )
        return -1;
      data|= ((uint16_t) tmp)<<8;

      // Assigna
      *dst= data;
      ret= 0;
      
    }
  
  return ret;
  
} // end mem_p32_read16


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_write16 (
                 PROTO_INTERP,
                 const uint32_t addr,
                 const uint16_t data
                 )
{

  uint64_t laddr;
  int ret;
  
  
  if ( (addr&0x1) == 0 )
    {
      laddr= 0;
      if ( page32_translate_addr ( INTERP, addr, &laddr,
                                   true, false, false ) != 0 )
        return -1;
      ret= mem_writel16 ( INTERP, laddr, data );
    }
  else
    {
      if ( mem_p32_write8 ( INTERP, addr, (uint8_t) data ) != 0 )
        return -1;
      if ( mem_p32_write8 ( INTERP, addr+1, (uint8_t) (data>>8) ) != 0 )
        return -1;
      ret= 0;
    }

  return ret;
  
} // end mem_p32_write16


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read32 (
                PROTO_INTERP,
                const uint32_t  addr,
                uint32_t       *dst,
                const bool      reading_data,
                const bool      implicit_svm
                )
{

  uint64_t laddr;
  uint16_t tmp;
  uint32_t data;
  int ret;
  

  if ( (addr&0x3) == 0 )
    {
      laddr= 0;
      if ( page32_translate_addr ( INTERP, addr, &laddr,
                                   false, !reading_data,
                                   implicit_svm ) != 0 )
        return -1;
      ret= mem_readl32 ( INTERP, laddr, dst, reading_data, implicit_svm );
    }
  else
    {

      // Llig
      if ( mem_p32_read16 ( INTERP, addr, &tmp,
                            reading_data, implicit_svm ) != 0 )
        return -1;
      data= (uint32_t) tmp;
      if ( mem_p32_read16 ( INTERP, addr+2, &tmp,
                            reading_data, implicit_svm ) != 0 )
        return -1;
      data|= (((uint32_t) tmp)<<16);

      // Assigna
      *dst= data;
      ret= 0;
      
    }
  
  return ret;
  
} // end mem_p32_read32


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_write32 (
                 PROTO_INTERP,
                 const uint32_t addr,
                 const uint32_t data
                 )
{

  uint64_t laddr;
  int ret;
  

  if ( (addr&0x3) == 0 )
    {
      laddr= 0;
      if ( page32_translate_addr ( INTERP, addr, &laddr, true,
                                   false, false ) != 0 )
        return -1;
      ret= mem_writel32 ( INTERP, laddr, data );
    }
  else
    {
      if ( mem_p32_write16 ( INTERP, addr, (uint16_t) data ) != 0 )
        return -1;
      if ( mem_p32_write16 ( INTERP, addr+2, (uint16_t) (data>>16) ) != 0 )
        return -1;
      ret= 0;
    }

  return ret;
  
} // end mem_p32_write32


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read64 (
                PROTO_INTERP,
                const uint32_t  addr,
                uint64_t       *dst
                )
{

  uint64_t laddr;


  laddr= 0;
  if ( page32_translate_addr ( INTERP, addr, &laddr,
                               false, false, false ) != 0 )
    return -1;
  return mem_readl64 ( INTERP, laddr, dst );
  
} // end mem_p32_read64


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read128 (
                 PROTO_INTERP,
                 const uint32_t      addr,
                 IA32_DoubleQuadword dst
                 )
{

  uint64_t laddr;


  laddr= 0;
  if ( page32_translate_addr ( INTERP, addr, &laddr,
                               false, false, false ) != 0 )
    return -1;
  return mem_readl128 ( INTERP, laddr, dst );
  
} // end mem_p32_read128


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read8_protected (
        	     PROTO_INTERP,
        	     IA32_SegmentRegister *seg,
        	     const uint32_t        offset,
        	     uint8_t              *dst,
        	     const bool            reading_data
        	     )
{

  uint32_t addr;

  
  // Null segment selector checking.
  if ( seg->h.isnull &&
       (!reading_data || (seg!=P_SS && seg!=P_CS) ) )
    goto excp_gp;
  
  // Type checking.
  if ( (reading_data && !seg->h.readable) ||
       (!reading_data && !seg->h.executable) )
    goto excp_gp;
  
  // Limit checking.
  if ( offset < seg->h.lim.firstb || offset > seg->h.lim.lastb )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  // if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ??
  READLB ( addr, dst, reading_data, return -1 );
  
  return 0;

 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read8_protected


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_write8_protected (
        	      PROTO_INTERP,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      const uint8_t         data
        	      )
{

  uint32_t addr;
  
  
  /* Null segment selector and type checking. */
  if ( seg->h.isnull || !seg->h.writable )
    { EXCEPTION0 ( EXCP_GP ); return -1; }
  
  /* Limit checking. */
  if ( offset < seg->h.lim.firstb || offset > seg->h.lim.lastb )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  /* Tradueix i llig. */
  addr= seg->h.lim.addr + offset;
  /* if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ?? */ 
  WRITELB ( addr, data, return -1 );
  
  return 0;
  
} /* end mem_write8_protected */


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read16_protected (
        	      PROTO_INTERP,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint16_t             *dst,
        	      const bool            reading_data
        	      )
{

  uint32_t addr;
  
  
  // Null segment selector and type checking.
  if ( seg->h.isnull &&
       (!reading_data || (seg!=P_SS && seg!=P_CS) ) )
    goto excp_gp;
  
  // Type checking.
  if ( (reading_data && !seg->h.readable) ||
       (!reading_data && !seg->h.executable) )
    goto excp_gp;
  
  // Alignment checking.
  if ( (offset&0x1) && (CR0&CR0_AM) && (EFLAGS&AC_FLAG) && CPL==3 )
    {
      EXCEPTION ( EXCP_AC );
      return -1;
    }
  
  // Limit checking.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-1) )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  // if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ??
  READLW ( addr, dst, reading_data, false, return -1 );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read16_protected


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_write16_protected (
        	       PROTO_INTERP,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       const uint16_t        data
        	       )
{

  uint32_t addr;
  
  
  /* Null segment selector and type checking. */
  if ( seg->h.isnull || !seg->h.writable ) goto excp_gp;
  
  /* Alignment checking. */
  if ( (offset&0x1) && (CR0&CR0_AM) && (EFLAGS&AC_FLAG) && CPL==3 )
    {
      EXCEPTION ( EXCP_AC );
      return -1;
    }
  
  /* Limit checking. */
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-1) )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  /* Tradueix i llig. */
  addr= seg->h.lim.addr + offset;
  /* if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ?? */ 
  WRITELW ( addr, data, return -1 );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} /* end mem_write16_protected */


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read32_protected (
        	      PROTO_INTERP,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint32_t             *dst,
        	      const bool            reading_data
        	      )
{

  uint32_t addr;
  
  
  // Null segment selector and type checking.
  if ( seg->h.isnull &&
       (!reading_data || (seg!=P_SS && seg!=P_CS) ))
    goto excp_gp;

  // Type checking.
  if ( (reading_data && !seg->h.readable) ||
       (!reading_data && !seg->h.executable) )
    goto excp_gp;
  
  // Alignment checking.
  if ( (offset&0x3) && (CR0&CR0_AM) && (EFLAGS&AC_FLAG) && CPL==3 )
    {
      EXCEPTION ( EXCP_AC );
      return -1;
    }
  
  // Limit checking.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-3) )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  // if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ??
  READLD ( addr, dst, reading_data, false, return -1 );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read32_protected


static int
mem_write32_protected (
        	       PROTO_INTERP,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       const uint32_t        data
        	       )
{

  uint32_t addr;
  
  
  /* Null segment selector and type checking. */
  if ( seg->h.isnull || !seg->h.writable ) goto excp_gp;
  
  /* Alignment checking. */
  if ( (offset&0x3) && (CR0&CR0_AM) && (EFLAGS&AC_FLAG) && CPL==3 )
    {
      EXCEPTION ( EXCP_AC );
      return -1;
    }
  
  /* Limit checking. */
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-3) )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  /* Tradueix i llig. */
  addr= seg->h.lim.addr + offset;
  /* if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ?? */ 
  WRITELD ( addr, data, return -1 );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} /* end mem_write32_protected */


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read64_protected (
        	      PROTO_INTERP,
        	      IA32_SegmentRegister *seg,
        	      const uint32_t        offset,
        	      uint64_t             *dst
        	      )
{

  uint32_t addr;
  
  
  // Null segment selector and type checking.
  if ( seg->h.isnull || !seg->h.readable ) goto excp_gp;
  
  // Alignment checking.
  if ( (offset&0x7) && (CR0&CR0_AM) && (EFLAGS&AC_FLAG) && CPL==3 )
    {
      EXCEPTION ( EXCP_AC );
      return -1;
    }
  
  // Limit checking.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-7) )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  // if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ??
  READLQ ( addr, dst, return -1 );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read64_protected


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_read128_protected (
        	       PROTO_INTERP,
        	       IA32_SegmentRegister *seg,
        	       const uint32_t        offset,
        	       IA32_DoubleQuadword   dst,
        	       const bool            isSSE
        	       )
{

  uint32_t addr;
  
  
  /* Null segment selector and type checking. */
  if ( seg->h.isnull || !seg->h.readable ) goto excp_gp;
  
  /* Alignment checking. */
  if ( isSSE )
    {
      if ( offset&0xF ) { EXCEPTION0 ( EXCP_GP ); return -1; }
    }
  else if ( (offset&0xF) && (CR0&CR0_AM) &&
            (EFLAGS&AC_FLAG) && CPL==3 )
    {
      EXCEPTION ( EXCP_AC );
      return -1;
    }
  
  /* Limit checking. */
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-15) )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  /* Tradueix i llig. */
  addr= seg->h.lim.addr + offset;
  /* if ( !ADDRESS_SIZE_IS_32 ) addr&= 0xFFFF; ¿¿ Açò cal fer-ho ?? */ 
  READLDQ ( addr, dst, return -1 );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} /* end mem_read128_protected */


static int
mem_read8_real (
        	PROTO_INTERP,
        	IA32_SegmentRegister *seg,
        	const uint32_t        offset,
        	uint8_t              *dst,
        	const bool            reading_data
        	)
{

  uint32_t addr;

  
  // NOTA!! Importa la part oculta. D'aquesta manera s'habilitat el
  // unreal mode.
  if ( offset < seg->h.lim.firstb || offset > seg->h.lim.lastb )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig. */
  addr= seg->h.lim.addr + offset;
  READLB ( addr, dst, reading_data, return -1 );
  
  return 0;
  
} // end mem_read8_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_write8_real (
        	 PROTO_INTERP,
        	 IA32_SegmentRegister *seg,
        	 const uint32_t        offset,
        	 const uint8_t         data
        	 )
{

  uint32_t addr;
  

  // Excepció.
  if ( offset < seg->h.lim.firstb || offset > seg->h.lim.lastb )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  WRITELB ( addr, data, return -1 );
  
  return 0;
  
} // end mem_write8_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read16_real (
        	 PROTO_INTERP,
        	 IA32_SegmentRegister *seg,
        	 const uint32_t        offset,
        	 uint16_t             *dst,
        	 const bool            reading_data
        	 )
{

  uint32_t addr;
  

  // NOTA!!! En la documentació no queda 100% clar però en en algunes
  // pàgines pareixia que si no estava alinia i eixia del límit
  // generava excepció. Però NAVCOM 6 i DSOBOX donen a entendre que
  // no.  
  // Excepció.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb/*-1*/) )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  READLW ( addr, dst, reading_data, false, return -1 );
  
  return 0;
  
} // end mem_read16_real


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_write16_real (
        	  PROTO_INTERP,
        	  IA32_SegmentRegister *seg,
        	  const uint32_t        offset,
        	  const uint16_t        data
        	  )
{

  uint32_t addr;
  

  // NOTA!!! En la documentació no queda 100% clar però en en algunes
  // pàgines pareixia que si no estava alinia i eixia del límit
  // generava excepció. Però NAVCOM 6 i DSOBOX donen a entendre que
  // no.  
  // Excepció.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb/*-1*/) )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  WRITELW ( addr, data, return -1 );
  
  return 0;
  
} // end mem_write16_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read32_real (
        	 PROTO_INTERP,
        	 IA32_SegmentRegister *seg,
        	 const uint32_t        offset,
        	 uint32_t             *dst,
        	 const bool            reading_data
        	 )
{

  uint32_t addr;
  
  
  // Excepció.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-3) )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  READLD ( addr, dst, reading_data, false, return -1 );
  
  return 0;
  
} // end mem_read32_real


static int
mem_write32_real (
        	  PROTO_INTERP,
        	  IA32_SegmentRegister *seg,
        	  const uint32_t        offset,
        	  const uint32_t        data
        	  )
{

  uint32_t addr;
  

  // Excepció.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-3) )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  WRITELD ( addr, data, return -1 );
  
  return 0;
  
} // end mem_write32_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read64_real (
        	 PROTO_INTERP,
        	 IA32_SegmentRegister *seg,
        	 const uint32_t        offset,
        	 uint64_t             *dst
        	 )
{

  uint32_t addr;
  

  // Excepció.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-7) )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  READLQ ( addr, dst, return -1 );
  
  return 0;
  
} // end mem_read64_real


/* Torna 0 si tot ha anat bé, -1 en cas d'excepció. */
static int
mem_read128_real (
        	  PROTO_INTERP,
        	  IA32_SegmentRegister *seg,
        	  const uint32_t        offset,
        	  IA32_DoubleQuadword   dst,
        	  const bool            isSSE
        	  )
{

  uint32_t addr;
  

  // Excepcions.
  if ( isSSE && offset&0xF ) { EXCEPTION ( EXCP_GP ); return -1; }
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-15) )
    {
      if ( seg == P_SS ) { EXCEPTION ( EXCP_SS ); }
      else               { EXCEPTION ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  READLDQ ( addr, dst, return -1 );
  
  return 0;
  
} // end mem_read128_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
push16 (
        PROTO_INTERP,
        const uint16_t data
        )
{

  uint32_t addr,offset;
  

  // Calcula offset i comprova.
  offset= P_SS->h.is32 ? (ESP-2) : ((uint32_t) ((uint16_t) (SP-2)));
  if ( offset < P_SS->h.lim.firstb || offset > (P_SS->h.lim.lastb-1) )
    {
      EXCEPTION0 ( EXCP_SS );
      return -1;
    }
  
  // Tradueix, escriu i actualitza.
  addr= P_SS->h.lim.addr + offset;
  WRITELW ( addr, data, return -1 );
  if ( P_SS->h.is32 ) ESP-= 2;
  else                SP-= 2;
  
  return 0;
  
} // end push16


static int
push32 (
        PROTO_INTERP,
        const uint32_t data
        )
{

  uint32_t addr, offset;
  

  // Calcula offset i comprova.
  offset= P_SS->h.is32 ? (ESP-4) : ((uint32_t) ((uint16_t) (SP-4)));
  if ( offset < P_SS->h.lim.firstb || offset > (P_SS->h.lim.lastb-3) )
    {
      EXCEPTION0 ( EXCP_SS );
      return -1;
    }
  
  // Tradueix, escriu i actualitza.
  addr= P_SS->h.lim.addr + offset;
  WRITELD ( addr, data, return -1 );
  if ( P_SS->h.is32 ) ESP-= 4;
  else                SP-= 4;
  
  return 0;
  
} // end push32


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
pop16 (
       PROTO_INTERP,
       uint16_t *data
       )
{

  
  uint32_t offset,last_byte,addr;

  
  // Calcula offset i comprova.
  offset= P_SS->h.is32 ? ESP : ((uint32_t) ((uint16_t) SP));
  last_byte= P_SS->h.is32 ? (ESP+2-1) : ((uint32_t) ((uint16_t) (SP+2-1)));
  if ( last_byte < P_SS->h.lim.firstb || last_byte > P_SS->h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_SS );
      return -1;
    }

  // Tradueix, llig i actualitza.
  addr= P_SS->h.lim.addr + offset;
  READLW ( addr, data, true, false, return -1 );
  if ( P_SS->h.is32 ) ESP+= 2;
  else                SP+= 2;
  
  return 0;
  
} // end pop16


static int
pop32 (
       PROTO_INTERP,
       uint32_t *data
       )
{

  uint32_t offset,last_byte,addr;

  
  // Calcula offset i comprova.
  offset= P_SS->h.is32 ? ESP : ((uint32_t) ((uint16_t) SP));
  last_byte= P_SS->h.is32 ? (ESP+4-1) : ((uint32_t) ((uint16_t) (SP+4-1)));
  if ( last_byte < P_SS->h.lim.firstb || last_byte > P_SS->h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_SS );
      return -1;
    }

  // Tradueix, llig i actualitza.
  addr= P_SS->h.lim.addr + offset;
  READLD ( addr, data, true, false, return -1 );
  if ( P_SS->h.is32 ) ESP+= 4;
  else                SP+= 4;
  
  return 0;
  
} // end pop32


static void
update_mem_callbacks (
        	      PROTO_INTERP
        	      )
{

  // NOTA!!! En aquest punt podria ser que la part pública de INTERP
  // no estiga inicialitzada.
  //
  // FALTA VIRTUAL MODE !!!!
  
  if ( CPU != NULL && PROTECTED_MODE_ACTIVATED ) // Protected mode.
    {

      // Memòria.
      INTERP->_mem_read8= mem_read8_protected;
      INTERP->_mem_read16= mem_read16_protected;
      INTERP->_mem_read32= mem_read32_protected;
      INTERP->_mem_read64= mem_read64_protected;
      INTERP->_mem_read128= mem_read128_protected;
      INTERP->_mem_write8= mem_write8_protected;
      INTERP->_mem_write16= mem_write16_protected;
      INTERP->_mem_write32= mem_write32_protected;

      // Memòria linial.
      if ( CR0&CR0_PG )
        {
          if ( (CR4&CR4_PAE) == 0 )
            {
              INTERP->_mem_readl8= mem_p32_read8;
              INTERP->_mem_readl16= mem_p32_read16;
              INTERP->_mem_readl32= mem_p32_read32;
              INTERP->_mem_readl64= mem_p32_read64;
              INTERP->_mem_readl128= mem_p32_read128;
              INTERP->_mem_writel8= mem_p32_write8;
              INTERP->_mem_writel16= mem_p32_write16;
              INTERP->_mem_writel32= mem_p32_write32;
            }
          else
            {
              fprintf ( stderr, "[CAL_IMPLEMENTAR] Paginació PAE=1!!!\n" );
              exit ( EXIT_FAILURE );
            }
        }
      else
        {
          INTERP->_mem_readl8= mem_readl8;
          INTERP->_mem_readl16= mem_readl16;
          INTERP->_mem_readl32= mem_readl32;
          INTERP->_mem_readl64= mem_readl64;
          INTERP->_mem_readl128= mem_readl128;
          INTERP->_mem_writel8= mem_writel8;
          INTERP->_mem_writel16= mem_writel16;
          INTERP->_mem_writel32= mem_writel32;
        }
      
    }
  else // Real address mode.
    {

      // Memòria.
      INTERP->_mem_read8= mem_read8_real;
      INTERP->_mem_read16= mem_read16_real;
      INTERP->_mem_read32= mem_read32_real;
      INTERP->_mem_read64= mem_read64_real;
      INTERP->_mem_read128= mem_read128_real;
      INTERP->_mem_write8= mem_write8_real;
      INTERP->_mem_write16= mem_write16_real;
      INTERP->_mem_write32= mem_write32_real;
      
      // Memòria linial.
      INTERP->_mem_readl8= mem_readl8;
      INTERP->_mem_readl16= mem_readl16;
      INTERP->_mem_readl32= mem_readl32;
      INTERP->_mem_readl64= mem_readl64;
      INTERP->_mem_readl128= mem_readl128;
      INTERP->_mem_writel8= mem_writel8;
      INTERP->_mem_writel16= mem_writel16;
      INTERP->_mem_writel32= mem_writel32;
      
    }
  
} // end update_mem_callbacks


/* ADRESSING MODES ************************************************************/
/* Torna -1 en cas d'excepció, 0 si tot ha anat bé. */
static int
get_addr_16bit (
        	PROTO_INTERP,
        	const uint8_t  modrm,
        	addr_t        *addr
        	)
{

  int8_t disp8;
  uint16_t disp16;
  
  
  switch ( modrm )
    {

      /* Mod 00 */
    case 0x00: /* [BX+SI] */
      addr->seg= P_DS;
      addr->off= (BX+SI)&0xFFFF;
      break;
    case 0x01: /* [BX+DI] */
      addr->seg= P_DS;
      addr->off= (BX+DI)&0xFFFF;
      break;
    case 0x02: /* [BP+SI] */
      addr->seg= P_SS;
      addr->off= (BP+SI)&0xFFFF;
      break;
    case 0x03: /* [BP+DI] */
      addr->seg= P_SS;
      addr->off= (BP+DI)&0xFFFF;
      break;
    case 0x04: /* [SI] */
      addr->seg= P_DS;
      addr->off= SI;
      break;
    case 0x05: /* [DI] */
      addr->seg= P_DS;
      addr->off= DI;
      break;
    case 0x06: /* disp16. */
      addr->seg= P_DS;
      IW ( &disp16, return -1 );
      addr->off= disp16;
      break;
    case 0x07: /* [BX] */
      addr->seg= P_DS;
      addr->off= BX;
      break;

      /* Mod 01 */
    case 0x08: /* [BX+SI]+disp8 */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= (BX+SI+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;
    case 0x09: /* [BX+DI]+disp8 */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= (BX+DI+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;
    case 0x0A: /* [BP+SI]+disp8 */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_SS;
      addr->off= (BP+SI+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;
    case 0x0B: /* [BP+DI]+disp8 */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_SS;
      addr->off= (BP+DI+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;
    case 0x0C: /* [SI]+disp8 */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= (SI+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;
    case 0x0D: /* [DI]+disp8 */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= (DI+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;
    case 0x0E: /* [BP]+disp8. */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_SS;
      addr->off= (BP+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;
    case 0x0F: /* [BX]+disp8 */
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= (BX+(uint16_t)((int16_t) disp8))&0xFFFF;
      break;

      /* Mod 01 */
    case 0x10: /* [BX+SI]+disp16 */
      IW ( &disp16, return -1 );
      addr->seg= P_DS;
      addr->off= (BX+SI+disp16)&0xFFFF;
      break;
    case 0x11: /* [BX+DI]+disp16 */
      IW ( &disp16, return -1 );
      addr->seg= P_DS;
      addr->off= (BX+DI+disp16)&0xFFFF;
      break;
    case 0x12: /* [BP+SI]+disp16 */
      IW ( &disp16, return -1 );
      addr->seg= P_SS;
      addr->off= (BP+SI+disp16)&0xFFFF;
      break;
    case 0x13: /* [BP+DI]+disp16 */
      IW ( &disp16, return -1 );
      addr->seg= P_SS;
      addr->off= (BP+DI+disp16)&0xFFFF;
      break;
    case 0x14: /* [SI]+disp16 */
      IW ( &disp16, return -1 );
      addr->seg= P_DS;
      addr->off= (SI+disp16)&0xFFFF;
      break;
    case 0x15: /* [DI]+disp16 */
      IW ( &disp16, return -1 );
      addr->seg= P_DS;
      addr->off= (DI+disp16)&0xFFFF;
      break;
    case 0x16: /* [BP]+disp16. */
      IW ( &disp16, return -1 );
      addr->seg= P_SS;
      addr->off= (BP+disp16)&0xFFFF;
      break;
    case 0x17: /* [BX]+disp16 */
      IW ( &disp16, return -1 );
      addr->seg= P_DS;
      addr->off= (BX+disp16)&0xFFFF;
      break;
      
      /* Mod 11 */
    default: break;
      
    }

  /* Sobreescriu el segment. */
  OVERRIDE_SEG ( addr->seg );

  return 0;
  
} /* end get_addr_16bit */


/* Torna -1 en cas d'excepció, 0 si tot ha anat bé. */
static int
get_addr_sib (
              PROTO_INTERP,
              const uint8_t  modrm,
              addr_t        *addr
              )
{

  uint8_t sib;
  
  
  IB ( &sib, return -1 );
  
  /* Base */
  switch ( sib&0x7 )
    {
    case 0:
      addr->seg= P_DS;
      addr->off= EAX;
      break;
    case 1:
      addr->seg= P_DS;
      addr->off= ECX;
      break;
    case 2:
      addr->seg= P_DS;
      addr->off= EDX;
      break;
    case 3:
      addr->seg= P_DS;
      addr->off= EBX;
      break;
    case 4:
      addr->seg= P_SS;
      addr->off= ESP;
      break;
    case 5:
      switch ( modrm>>3 )
        {
        case 0: // MOD 00
          addr->seg= P_DS;
          ID ( &(addr->off), return -1 );
          break;
        case 1: // MOD 01, MOD10
        case 2: 
          addr->seg= P_SS;
          addr->off= EBP;
          break;
        default: break;
        }
      break;
    case 6:
      addr->seg= P_DS;
      addr->off= ESI;
      break;
    case 7:
      addr->seg= P_DS;
      addr->off= EDI;
      break;
    }

  /* Índex. */
  switch ( sib>>3 )
    {
      
      /* SS 00 */
    case 0x00: addr->off+= EAX; break;
    case 0x01: addr->off+= ECX; break;
    case 0x02: addr->off+= EDX; break;
    case 0x03: addr->off+= EBX; break;
    case 0x04: break;
    case 0x05: addr->off+= EBP; break;
    case 0x06: addr->off+= ESI; break;
    case 0x07: addr->off+= EDI; break;

      /* SS 01 */
    case 0x08: addr->off+= (EAX<<1); break;
    case 0x09: addr->off+= (ECX<<1); break;
    case 0x0A: addr->off+= (EDX<<1); break;
    case 0x0B: addr->off+= (EBX<<1); break;
    case 0x0C: break;
    case 0x0D: addr->off+= (EBP<<1); break;
    case 0x0E: addr->off+= (ESI<<1); break;
    case 0x0F: addr->off+= (EDI<<1); break;

      /* SS 10 */
    case 0x10: addr->off+= (EAX<<2); break;
    case 0x11: addr->off+= (ECX<<2); break;
    case 0x12: addr->off+= (EDX<<2); break;
    case 0x13: addr->off+= (EBX<<2); break;
    case 0x14: break;
    case 0x15: addr->off+= (EBP<<2); break;
    case 0x16: addr->off+= (ESI<<2); break;
    case 0x17: addr->off+= (EDI<<2); break;

      /* SS 11 */
    case 0x18: addr->off+= (EAX<<3); break;
    case 0x19: addr->off+= (ECX<<3); break;
    case 0x1A: addr->off+= (EDX<<3); break;
    case 0x1B: addr->off+= (EBX<<3); break;
    case 0x1C: break;
    case 0x1D: addr->off+= (EBP<<3); break;
    case 0x1E: addr->off+= (ESI<<3); break;
    case 0x1F: addr->off+= (EDI<<3); break;
      
    }

  return 0;
  
} // end get_addr_sib


// Torna -1 en cas d'excepció, 0 si tot ha anat bé.
static int
get_addr_32bit (
        	PROTO_INTERP,
        	const uint8_t  modrm,
        	addr_t        *addr
        	)
{

  int8_t disp8;
  uint32_t disp32;
  
  
  switch ( modrm )
    {

      // Mod 00
    case 0x00: // [EAX]
      addr->seg= P_DS;
      addr->off= EAX;
      break;
    case 0x01: // [ECX]
      addr->seg= P_DS;
      addr->off= ECX;
      break;
    case 0x02: // [EDX]
      addr->seg= P_DS;
      addr->off= EDX;
      break;
    case 0x03: // [EBX]
      addr->seg= P_DS;
      addr->off= EBX;
      break;
    case 0x04: // [--][--]
      if ( get_addr_sib ( INTERP, modrm, addr ) != 0 )
        return -1;
      break;
    case 0x05: // disp32
      addr->seg= P_DS;
      ID ( &(addr->off), return -1 );
      break;
    case 0x06: // [ESI]
      addr->seg= P_DS;
      addr->off= ESI;
      break;
    case 0x07: // [EDI]
      addr->seg= P_DS;
      addr->off= EDI;
      break;

      // Mod 01
    case 0x08: // [EAX]+disp8
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= EAX + (uint32_t) ((int32_t) disp8);
      break;
    case 0x09: // [ECX]+disp8
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= ECX + (uint32_t) ((int32_t) disp8);
      break;
    case 0x0A: // [EDX]+disp8
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= EDX + (uint32_t) ((int32_t) disp8);
      break;
    case 0x0B: // [EBX]+disp8
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= EBX + (uint32_t) ((int32_t) disp8);
      break;
    case 0x0C: // [--][--]+disp8
      if ( get_addr_sib ( INTERP, modrm, addr ) != 0 )
        return -1;
      IB ( (uint8_t *) &disp8, return -1 );
      addr->off+= (uint32_t) ((int32_t) disp8);
      break;
    case 0x0D: // [EBP]+disp8
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_SS;
      addr->off= EBP + (uint32_t) ((int32_t) disp8);
      break;
    case 0x0E: // [ESI]+disp8
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= ESI + (uint32_t) ((int32_t) disp8);
      break;
    case 0x0F: // [EDI]+disp8
      IB ( (uint8_t *) &disp8, return -1 );
      addr->seg= P_DS;
      addr->off= EDI + (uint32_t) ((int32_t) disp8);
      break;

      // Mod 10
    case 0x10: // [EAX]+disp32
      ID ( &disp32, return -1 );
      addr->seg= P_DS;
      addr->off= EAX + disp32;
      break;
    case 0x11: // [ECX]+disp32
      ID ( &disp32, return -1 );
      addr->seg= P_DS;
      addr->off= ECX + disp32;
      break;
    case 0x12: // [EDX]+disp32
      ID ( &disp32, return -1 );
      addr->seg= P_DS;
      addr->off= EDX + disp32;
      break;
    case 0x13: // [EBX]+disp32
      ID ( &disp32, return -1 );
      addr->seg= P_DS;
      addr->off= EBX + disp32;
      break;
    case 0x14: // [--][--]+disp32
      if ( get_addr_sib ( INTERP, modrm, addr ) != 0 )
        return -1;
      ID ( &disp32, return -1 );
      addr->off+= disp32;
      break;
    case 0x15: // [EBP]+disp32
      ID ( &disp32, return -1 );
      addr->seg= P_SS;
      addr->off= EBP + disp32;
      break;
    case 0x16: // [ESI]+disp32
      ID ( &disp32, return -1 );
      addr->seg= P_DS;
      addr->off= ESI + disp32;
      break;
    case 0x17: // [EDI]+disp32
      ID ( &disp32, return -1 );
      addr->seg= P_DS;
      addr->off= EDI + disp32;
      break;

      // Mod 11
    default: break;
      
    }

  // Sobreescriu el segment.
  OVERRIDE_SEG ( addr->seg );

  return 0;
  
} // end get_addr_32bit


// Torna -1 si excepció, 0 si tot ha anat bé.
static int
get_addr_mode_r8 (
        	  PROTO_INTERP,
        	  const uint8_t   modrmbyte,
        	  eaddr_r8_t     *eaddr,
        	  uint8_t       **reg
        	  )
{

  uint8_t modrm;
  int ret;


  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= &AL; break;
      case 1: *reg= &CL; break;
      case 2: *reg= &DL; break;
      case 3: *reg= &BL; break;
      case 4: *reg= &AH; break;
      case 5: *reg= &CH; break;
      case 6: *reg= &DH; break;
      case 7: *reg= &BH; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &AL; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &CL; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &DL; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &BL; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &AH; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &CH; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &DH; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &BH; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_r8


// Torna -1 si excepció, 0 si tot ha anat bé.
static int
get_addr_mode_rm8_r16 (
                       PROTO_INTERP,
                       const uint8_t   modrmbyte,
                       eaddr_r8_t     *eaddr,
                       uint16_t      **reg
                       )
{

  uint8_t modrm;
  int ret;


  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= &AX; break;
      case 1: *reg= &CX; break;
      case 2: *reg= &DX; break;
      case 3: *reg= &BX; break;
      case 4: *reg= &SP; break;
      case 5: *reg= &BP; break;
      case 6: *reg= &SI; break;
      case 7: *reg= &DI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &AL; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &CL; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &DL; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &BL; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &AH; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &CH; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &DH; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &BH; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_rm8_r16


// Torna -1 en cas d'excepció, 0 si tot ha anat bé.
static int
get_addr_mode_r16 (
        	   PROTO_INTERP,
        	   const uint8_t   modrmbyte,
        	   eaddr_r16_t    *eaddr,
        	   uint16_t      **reg
        	   )
{

  uint8_t modrm;
  int ret;


  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= &AX; break;
      case 1: *reg= &CX; break;
      case 2: *reg= &DX; break;
      case 3: *reg= &BX; break;
      case 4: *reg= &SP; break;
      case 5: *reg= &BP; break;
      case 6: *reg= &SI; break;
      case 7: *reg= &DI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &AX; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &CX; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &DX; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &BX; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &SP; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &BP; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &SI; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &DI; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_r16


// Torna -1 en cas d'excepció, 0 si tot ha anat bé.
static int
get_addr_mode_sreg16 (
                      PROTO_INTERP,
                      const uint8_t          modrmbyte,
                      eaddr_r16_t           *eaddr,
                      IA32_SegmentRegister **reg
                      )
{

  uint8_t modrm;
  int ret;


  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= P_ES; break;
      case 1: *reg= P_CS; break;
      case 2: *reg= P_SS; break;
      case 3: *reg= P_DS; break;
      case 4: *reg= P_FS; break;
      case 5: *reg= P_GS; break;
      default:
        *reg= NULL;
        EXCEPTION ( EXCP_UD );
        return -1;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &AX; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &CX; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &DX; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &BX; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &SP; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &BP; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &SI; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &DI; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_sreg16


// Torna -1 en cas d'excepció, 0 si tot ha anat bé.
static int
get_addr_mode_sreg32 (
                      PROTO_INTERP,
                      const uint8_t          modrmbyte,
                      eaddr_r32_t           *eaddr,
                      IA32_SegmentRegister **reg
                      )
{

  uint8_t modrm;
  int ret;


  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= P_ES; break;
      case 1: *reg= P_CS; break;
      case 2: *reg= P_SS; break;
      case 3: *reg= P_DS; break;
      case 4: *reg= P_FS; break;
      case 5: *reg= P_GS; break;
      default:
        *reg= NULL;
        EXCEPTION ( EXCP_UD );
        return -1;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &EAX; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &ECX; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &EDX; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &EBX; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &ESP; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &EBP; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &ESI; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &EDI; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }
  
  return ret;
  
} // end get_addr_mode_sreg32


// Torna -1 si excepció, 0 si tot ha anat bé.
static int
get_addr_mode_rm8_r32 (
                       PROTO_INTERP,
                       const uint8_t   modrmbyte,
                       eaddr_r8_t     *eaddr,
                       uint32_t      **reg
                       )
{

  uint8_t modrm;
  int ret;


  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= &EAX; break;
      case 1: *reg= &ECX; break;
      case 2: *reg= &EDX; break;
      case 3: *reg= &EBX; break;
      case 4: *reg= &ESP; break;
      case 5: *reg= &EBP; break;
      case 6: *reg= &ESI; break;
      case 7: *reg= &EDI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &AL; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &CL; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &DL; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &BL; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &AH; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &CH; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &DH; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &BH; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_rm8_r32


// Torna -1 si excepció, 0 si tot ha anat bé.
static int
get_addr_mode_rm16_r32 (
                        PROTO_INTERP,
                        const uint8_t   modrmbyte,
                        eaddr_r16_t    *eaddr,
                        uint32_t      **reg
                        )
{

  uint8_t modrm;
  int ret;


  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= &EAX; break;
      case 1: *reg= &ECX; break;
      case 2: *reg= &EDX; break;
      case 3: *reg= &EBX; break;
      case 4: *reg= &ESP; break;
      case 5: *reg= &EBP; break;
      case 6: *reg= &ESI; break;
      case 7: *reg= &EDI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &AX; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &CX; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &DX; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &BX; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &SP; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &BP; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &SI; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &DI; break;
      
    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }
  
  return ret;
  
} // end get_addr_mode_rm16_r32


// Torna -1 en cas d'excepció, 0 si tot ha anat bé.
static int
get_addr_mode_r32 (
        	   PROTO_INTERP,
        	   const uint8_t   modrmbyte,
        	   eaddr_r32_t    *eaddr,
        	   uint32_t      **reg
        	   )
{

  uint8_t modrm;
  int ret;
  

  ret= 0;
  
  // Reg.
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= &EAX; break;
      case 1: *reg= &ECX; break;
      case 2: *reg= &EDX; break;
      case 3: *reg= &EBX; break;
      case 4: *reg= &ESP; break;
      case 5: *reg= &EBP; break;
      case 6: *reg= &ESI; break;
      case 7: *reg= &EDI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      // Mod 11
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &EAX; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &ECX; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &EDX; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &EBX; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &ESP; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &EBP; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &ESI; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &EDI; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_r32


// Torna -1 en cas d'excepció, 0 si tot ha anat bé.
static int
get_addr_moffs (
                PROTO_INTERP,
                addr_t *addr
                )
{

  uint16_t off;
  
  
  if ( ADDRESS_SIZE_IS_32 )
    {
      ID ( &(addr->off), return -1 );
    }
  else
    {
      IW ( &off, return -1 );
      addr->off= (uint32_t) off;
    }
  addr->seg= P_DS;
  OVERRIDE_SEG ( addr->seg );
  
  return 0;
  
} // end get_addr_moffs


static void
get_addr_mode_cr (
                  PROTO_INTERP,
                  const uint8_t   modrmbyte,
                  uint32_t      **reg,
                  int            *cr
                  )
{

  uint8_t modrm;

  // Control Reg.
  *cr= (modrmbyte>>3)&0x7;
  
  // Effective address.
  modrm= (modrmbyte&0x7);
  switch ( modrm )
    {

      // Mod 11
    case 0: *reg= &EAX; break;
    case 1: *reg= &ECX; break;
    case 2: *reg= &EDX; break;
    case 3: *reg= &EBX; break;
    case 4: *reg= &ESP; break;
    case 5: *reg= &EBP; break;
    case 6: *reg= &ESI; break;
    case 7: *reg= &EDI; break;
      
    }
  
} // end get_addr_mode_cr

#if 0
/* Torna -1 en cas de excepció, 0 si tot ha anat bé. */
static int
get_addr_mode_rxmm (
        	    PROTO_INTERP,
        	    const uint8_t      modrmbyte,
        	    eaddr_rxmm_t      *eaddr,
        	    IA32_XMMRegister **reg
        	    )
{

  uint8_t modrm;
  int ret;
  

  ret= 0;
  
  /* Reg. */
  if ( reg != NULL )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: *reg= &XMM0; break;
      case 1: *reg= &XMM1; break;
      case 2: *reg= &XMM2; break;
      case 3: *reg= &XMM3; break;
      case 4: *reg= &XMM4; break;
      case 5: *reg= &XMM5; break;
      case 6: *reg= &XMM6; break;
      case 7: *reg= &XMM7; break;
      }
  
  /* Effective address. */
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {

      /* Mod 11 */
    case 0x18: eaddr->is_addr= false; eaddr->v.reg= &XMM0; break;
    case 0x19: eaddr->is_addr= false; eaddr->v.reg= &XMM1; break;
    case 0x1A: eaddr->is_addr= false; eaddr->v.reg= &XMM2; break;
    case 0x1B: eaddr->is_addr= false; eaddr->v.reg= &XMM3; break;
    case 0x1C: eaddr->is_addr= false; eaddr->v.reg= &XMM4; break;
    case 0x1D: eaddr->is_addr= false; eaddr->v.reg= &XMM5; break;
    case 0x1E: eaddr->is_addr= false; eaddr->v.reg= &XMM6; break;
    case 0x1F: eaddr->is_addr= false; eaddr->v.reg= &XMM7; break;

    default:
      eaddr->is_addr= true;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( INTERP, modrm, &(eaddr->v.addr) );
      else
        ret= get_addr_16bit ( INTERP, modrm, &(eaddr->v.addr) );
      break;
      
    }

  return ret;
  
} /* end get_addr_mode_rxmm */
#endif

static void
get_addrs_dsesi_esedi (
                       PROTO_INTERP,
                       addr_t *src,
                       addr_t *dst
                       )
{

  // Obté adreça
  src->seg= P_DS;
  OVERRIDE_SEG ( src->seg );
  dst->seg= P_ES;
  if ( ADDRESS_SIZE_IS_32 )
    {
      src->off= ESI;
      dst->off= EDI;
    }
  else
    {
      src->off= SI;
      dst->off= DI;
    }
  
} // end get_addrs_dsesi_esedi


static void
get_addr_dsesi (
                PROTO_INTERP,
                addr_t *addr
                )
{
  
  addr->seg= P_DS;
  OVERRIDE_SEG ( addr->seg );
  if ( ADDRESS_SIZE_IS_32 )
    addr->off= ESI;
  else
    addr->off= SI;
  
} // end get_addr_dsesi


/* Decodifica *****************************************************************/
static void
unk_inst (
          PROTO_INTERP,
          const uint8_t  opcode,
          const char    *pref
          )
{
  
  WW ( UDATA, "instrucció desconeguda: %s%02x", pref, opcode );
  fprintf(stderr,"[EE] interpreter.c - CAL IMPLEMENTAR INSTRUCCIÓ!\n");
  exit(EXIT_FAILURE);
  EXCEPTION ( EXCP_UD ); // <-- Ara no s'executa. Genera
                         // stackoverflow. REVISAR!!!

  
} // end unk_inst


#include "interpreter_add.h"
#include "interpreter_bits_bytes.h"
#include "interpreter_cache.h"
#include "interpreter_call_jmp.h"
#include "interpreter_cmp_sub.h"
#include "interpreter_fpu.h"
#include "interpreter_io.h"
#include "interpreter_lop.h"
#include "interpreter_mov.h"
#include "interpreter_mul_div.h"
#include "interpreter_other.h"
#include "interpreter_shift_rols.h"
#include "interpreter_set.h"

static void exec_inst ( PROTO_INTERP, const uint8_t opcode );

static void
inst_80 (
         PROTO_INTERP
         )
{

  uint8_t modrmbyte;


  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return add_rm8_imm8 ( INTERP, modrmbyte );
    case 1: return or_rm8_imm8 ( INTERP, modrmbyte );
    case 2: return adc_rm8_imm8 ( INTERP, modrmbyte );
    case 3: return sbb_rm8_imm8 ( INTERP, modrmbyte );
    case 4: return and_rm8_imm8 ( INTERP, modrmbyte );
    case 5: return sub_rm8_imm8 ( INTERP, modrmbyte );
    case 6: return xor_rm8_imm8 ( INTERP, modrmbyte );
    case 7: return cmp_rm8_imm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "80 " );
    }
  
} // end inst_80


static void
inst_81 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  

  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return add_rm_imm ( INTERP, modrmbyte );
    case 1:return or_rm_imm ( INTERP, modrmbyte );
    case 2: return adc_rm_imm ( INTERP, modrmbyte );
    case 3: return sbb_rm_imm ( INTERP, modrmbyte );
    case 4: return and_rm_imm ( INTERP, modrmbyte );
    case 5: return sub_rm_imm ( INTERP, modrmbyte );
    case 6: return xor_rm_imm ( INTERP, modrmbyte );
    case 7: return cmp_rm_imm ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "81 " );
    }
  
} // end inst_81


static void
inst_83 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  

  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return add_rm_imm8 ( INTERP, modrmbyte );
    case 1: return or_rm_imm8 ( INTERP, modrmbyte );
    case 2: return adc_rm_imm8 ( INTERP, modrmbyte );
    case 3: return sbb_rm_imm8 ( INTERP, modrmbyte );
    case 4: return and_rm_imm8 ( INTERP, modrmbyte );
    case 5: return sub_rm_imm8 ( INTERP, modrmbyte );
    case 6: return xor_rm_imm8 ( INTERP, modrmbyte );
    case 7: return cmp_rm_imm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "83 " );
    }
  
} // end inst_83


static void
inst_8f (
         PROTO_INTERP
         )
{

  uint8_t modrmbyte;


  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return pop_rm ( INTERP, modrmbyte );
      
    default: unk_inst ( INTERP, modrmbyte, "8F " );
    }
  
} // end inst_8f


static void
inst_c0 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return rol_rm8_imm8 ( INTERP, modrmbyte );
    case 1: return ror_rm8_imm8 ( INTERP, modrmbyte );
        
    case 4: return shl_rm8_imm8 ( INTERP, modrmbyte );
    case 5: return shr_rm8_imm8 ( INTERP, modrmbyte );

    case 7: return sar_rm8_imm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "C0 " );
    }
  
} // end inst_c0


static void
inst_c1 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return rol_rm_imm8 ( INTERP, modrmbyte );
    case 1: return ror_rm_imm8 ( INTERP, modrmbyte );
    case 2: return rcl_rm_imm8 ( INTERP, modrmbyte );
    case 3: return rcr_rm_imm8 ( INTERP, modrmbyte );
    case 4: return shl_rm_imm8 ( INTERP, modrmbyte );
    case 5: return shr_rm_imm8 ( INTERP, modrmbyte );

    case 7: return sar_rm_imm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "C1 " );
    }
  
} // end inst_c1


static void
inst_c6 (
         PROTO_INTERP
         )
{

  uint8_t modrmbyte;


  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return mov_rm8_imm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "C6 " );
    }
  
} // end inst_c6


static void
inst_c7 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return mov_rm_imm ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "C7 " );
    }
  
} // end inst_c7


static void
inst_d0 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return rol_rm8_1 ( INTERP, modrmbyte );
    case 1: return ror_rm8_1 ( INTERP, modrmbyte );
    case 2: return rcl_rm8_1 ( INTERP, modrmbyte );
    case 3: return rcr_rm8_1 ( INTERP, modrmbyte );
    case 4: return shl_rm8_1 ( INTERP, modrmbyte );
    case 5: return shr_rm8_1 ( INTERP, modrmbyte );

    case 7: return sar_rm8_1 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "D0 " );
    }
  
} // end inst_d0


static void
inst_d1 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return rol_rm_1 ( INTERP, modrmbyte );
    case 1: return ror_rm_1 ( INTERP, modrmbyte );
    case 2: return rcl_rm_1 ( INTERP, modrmbyte );
    case 3: return rcr_rm_1 ( INTERP, modrmbyte );
    case 4: return shl_rm_1 ( INTERP, modrmbyte );
    case 5: return shr_rm_1 ( INTERP, modrmbyte );

    case 7: return sar_rm_1 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "D1 " );
    }
  
} // end inst_d1


static void
inst_d2 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return rol_rm8_CL ( INTERP, modrmbyte );
    case 1: return ror_rm8_CL ( INTERP, modrmbyte );
    case 2: return rcl_rm8_CL ( INTERP, modrmbyte );
    case 3: return rcr_rm8_CL ( INTERP, modrmbyte );
    case 4: return shl_rm8_CL ( INTERP, modrmbyte );
    case 5: return shr_rm8_CL ( INTERP, modrmbyte );

    case 7: return sar_rm8_CL ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "D2 " );
    }
  
} // end inst_d2


static void
inst_d3 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return rol_rm_CL ( INTERP, modrmbyte );
    case 1: return ror_rm_CL ( INTERP, modrmbyte );
    case 2: return rcl_rm_CL ( INTERP, modrmbyte );
    case 3: return rcr_rm_CL ( INTERP, modrmbyte );
    case 4: return shl_rm_CL ( INTERP, modrmbyte );
    case 5: return shr_rm_CL ( INTERP, modrmbyte );
      
    case 7: return sar_rm_CL ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "D3 " );
    }
  
} // end inst_d3


static void
inst_d8 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: fadd_float ( INTERP, modrmbyte ); break;
        case 1: fmul_float ( INTERP, modrmbyte ); break;
        case 2: fcom_float ( INTERP, modrmbyte, false ); break;
        case 3: fcom_float ( INTERP, modrmbyte, true ); break;
        case 4: fsub_float ( INTERP, modrmbyte ); break;
        case 5: fsubr_float ( INTERP, modrmbyte ); break;
        case 6: fdiv_float ( INTERP, modrmbyte ); break;
        case 7: fdivr_float ( INTERP, modrmbyte ); break;
        default: unk_inst ( INTERP, modrmbyte, "D8 " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7: fadd_0_i ( INTERP, modrmbyte ); break;
        case 0xc8 ... 0xcf: fmul_0_i ( INTERP, modrmbyte ); break;
        case 0xd0 ... 0xd7: fcom ( INTERP, modrmbyte ); break;
        case 0xd8 ... 0xdf: fcomp ( INTERP, modrmbyte ); break;
        case 0xe0 ... 0xe7: fsub_0_i ( INTERP, modrmbyte ); break;
        case 0xe8 ... 0xef: fsubr_0_i ( INTERP, modrmbyte ); break;
        case 0xf0 ... 0xf7: fdiv_0_i ( INTERP, modrmbyte ); break;
        case 0xf8 ... 0xff: fdivr_0_i ( INTERP, modrmbyte ); break;
        default: unk_inst ( INTERP, modrmbyte, "D8 " ); break;
        }
    }
  
} // end inst_d8


static void
inst_d9 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: fld_float ( INTERP, modrmbyte ); break;
          
        case 2: fst_float ( INTERP, modrmbyte, false ); break;
        case 3: fst_float ( INTERP, modrmbyte, true ); break;
          
        case 5: fldcw ( INTERP, modrmbyte ); break;
          
        case 7: fstcw ( INTERP, modrmbyte ); break;
        default: unk_inst ( INTERP, modrmbyte, "D9 " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7: fld_reg ( INTERP, modrmbyte ); break;
        case 0xc8 ... 0xcf: fxch ( INTERP, modrmbyte ); break;

        case 0xe0: fchs ( INTERP ); break;
        case 0xe1: fabs_reg0 ( INTERP ); break;

        case 0xe4: ftst ( INTERP ); break;
        case 0xe5: fxam ( INTERP ); break;
            
        case 0xe8: fld1 ( INTERP ); break;

        case 0xea: fldl2e ( INTERP ); break;

        case 0xed: fldln2 ( INTERP ); break;
        case 0xee: fldz ( INTERP ); break;

        case 0xf0: f2xm1 ( INTERP ); break;
        case 0xf1: fyl2x ( INTERP ); break;
        case 0xf2: fptan ( INTERP ); break;
        case 0xf3: fpatan ( INTERP ); break;

        case 0xf8: fprem ( INTERP ); break;

        case 0xfa: fsqrt ( INTERP ); break;
            
        case 0xfc: frndint ( INTERP ); break;
        case 0xfd: fscale ( INTERP ); break;
        case 0xfe: fsin ( INTERP ); break;
        case 0xff: fcos ( INTERP ); break;
        default: unk_inst ( INTERP, modrmbyte, "D9 " ); break;
        }
    }
  
} // end inst_d9


static void
inst_da (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
          
        case 1: return fmul_int32 ( INTERP, modrmbyte );
          
        default: unk_inst ( INTERP, modrmbyte, "DA " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        default: unk_inst ( INTERP, modrmbyte, "DA " ); break;
        }
    }
  
} // end inst_da


static void
inst_db (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: fild_int32 ( INTERP, modrmbyte ); break;
            
        case 2: fist_int32 ( INTERP, modrmbyte, false ); break;
        case 3: fist_int32 ( INTERP, modrmbyte, true ); break;
          
        case 5: fld_ldouble ( INTERP, modrmbyte ); break;

        case 7: fstp_ldouble ( INTERP, modrmbyte ); break;
        default: unk_inst ( INTERP, modrmbyte, "DB " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
          
        case 0xe2: fclex ( INTERP ); break;
        case 0xe3: finit ( INTERP ); break;
        case 0xe4: break; // FSETPM - Ara funciona com un NOP.
          
        default: unk_inst ( INTERP, modrmbyte, "DB " ); break;
        }
    }
  
} // end inst_db


static void
inst_dc (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: fadd_double ( INTERP, modrmbyte ); break;
        case 1: fmul_double ( INTERP, modrmbyte ); break;
        case 2: fcom_double ( INTERP, modrmbyte, false ); break;
        case 3: fcom_double ( INTERP, modrmbyte, true ); break;
        case 4: fsub_double ( INTERP, modrmbyte ); break;
        case 5: fsubr_double ( INTERP, modrmbyte ); break;
        case 6: fdiv_double ( INTERP, modrmbyte ); break;
        case 7: fdivr_double ( INTERP, modrmbyte ); break;
        default: unk_inst ( INTERP, modrmbyte, "DC " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
          
        case 0xc8 ... 0xcf: fmul_i_0 ( INTERP, modrmbyte ); break;

        case 0xe8 ... 0xef: fsub_i_0 ( INTERP, modrmbyte ); break;
        case 0xf0 ... 0xf7: fdivr_i_0 ( INTERP, modrmbyte ); break;
          
        default: unk_inst ( INTERP, modrmbyte, "DC " ); break;
        }
    }
  
} // end inst_dc


static void
inst_dd (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: fld_double ( INTERP, modrmbyte ); break;

        case 2: fst_double ( INTERP, modrmbyte, false ); break;
        case 3: fst_double ( INTERP, modrmbyte, true ); break;
        case 4: frstor ( INTERP, modrmbyte ); break;

        case 6: fsave ( INTERP, modrmbyte ); break;
        case 7: fstsw ( INTERP, modrmbyte ); break;
        default: unk_inst ( INTERP, modrmbyte, "DD " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7: ffree ( INTERP, modrmbyte ); break;
            
        case 0xd0 ... 0xd7: fst ( INTERP, modrmbyte, false ); break;
        case 0xd8 ... 0xdf: fst ( INTERP, modrmbyte, true ); break;
          
        default: unk_inst ( INTERP, modrmbyte, "DD " ); break;
        }
    }
  
} // end inst_dd


static void
inst_de (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        default: unk_inst ( INTERP, modrmbyte, "DE " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7: faddp_i_0 ( INTERP, modrmbyte ); break;
        case 0xc8 ... 0xcf: fmulp_i_0 ( INTERP, modrmbyte ); break;
          
        case 0xd9: fcompp ( INTERP ); break;

        case 0xe0 ... 0xe7: fsubrp_i_0 ( INTERP, modrmbyte ); break;
        case 0xe8 ... 0xef: fsubp_i_0 ( INTERP, modrmbyte ); break;
        case 0xf0 ... 0xf7: fdivrp_i_0 ( INTERP, modrmbyte ); break;
        case 0xf8 ... 0xff: fdivp_i_0 ( INTERP, modrmbyte ); break;
        default: unk_inst ( INTERP, modrmbyte, "DE " ); break;
        }
    }
  
} // end inst_de


static void
inst_df (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: fild_int16 ( INTERP, modrmbyte ); break;

        case 3: fist_int16 ( INTERP, modrmbyte, true ); break;
            
        case 5: fild_int64 ( INTERP, modrmbyte ); break;
        case 6: fbstp ( INTERP, modrmbyte ); break;
        case 7: fist_int64 ( INTERP, modrmbyte, true ); break;
        default: unk_inst ( INTERP, modrmbyte, "DF " );
        }
    }
  else
    {
      switch ( modrmbyte )
        {

        case 0xe0: fnstsw_AX ( INTERP ); break;
          
        default: unk_inst ( INTERP, modrmbyte, "DF " ); break;
        }
    }
  
} // end inst_df

/*
static void
lock (
      PROTO_INTERP
      )
{

  uint8_t opcode;


  if ( INTERP->_group1_inst ) { EXCEPTION ( EXCP_UD ); return; }
  
  READB_INST ( P_CS, EIP, &opcode, return );
  ++EIP;
  
  INTERP->_group1_inst= true;
  LOCK;
  exec_inst ( INTERP, opcode );
  UNLOCK;
  INTERP->_group1_inst= false;
  
} // end lock
*/

static void
override_seg (
              PROTO_INTERP,
              IA32_SegmentRegister *seg
              )
{

  uint8_t opcode;


  READB_INST ( P_CS, EIP, &opcode, return );
  ++EIP;

  if ( INTERP->_group2_inst ) exec_inst ( INTERP, opcode );
  else
    {
      INTERP->_group2_inst= true;
      SET_DATA_SEG ( seg );
      exec_inst ( INTERP, opcode );
      UNSET_DATA_SEG;
      INTERP->_group2_inst= false;
    }
  
} // end override_seg


static void
inst_0f_ba (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 4: return bt_rm_imm8 ( INTERP, modrmbyte );
    case 5: return bts_rm_imm8 ( INTERP, modrmbyte );
    case 6: return btr_rm_imm8 ( INTERP, modrmbyte );
    case 7: return btc_rm_imm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "0F BA " );
    }
  
} // end inst_0f_ba


static void
inst_0f_00 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return sldt ( INTERP, modrmbyte );
    case 1: return str ( INTERP, modrmbyte );
    case 2: return lldt ( INTERP, modrmbyte );
    case 3: return ltr ( INTERP, modrmbyte );
    case 4: return verr ( INTERP, modrmbyte );
    case 5: return verw ( INTERP, modrmbyte );
      
    default: unk_inst ( INTERP, modrmbyte, "0F 00 " );
    }
  
} // end inst_0f_00


static void
inst_0f_01 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0:
      if ( (modrmbyte>>6) == 0x3 )
        unk_inst ( INTERP, modrmbyte, "0F 01 " );
      else sgdt ( INTERP, modrmbyte );
      break;
    case 1:
      if ( (modrmbyte>>6) == 0x3 )
        unk_inst ( INTERP, modrmbyte, "0F 01 " );
      else sidt ( INTERP, modrmbyte );
      break;
    case 2:
      if ( (modrmbyte>>6) == 0x3 )
        unk_inst ( INTERP, modrmbyte, "0F 01 " );
      else lgdt ( INTERP, modrmbyte );
      break;
    case 3:
      if ( (modrmbyte>>6) == 0x3 )
        unk_inst ( INTERP, modrmbyte, "0F 01 " );
      else lidt ( INTERP, modrmbyte );
      break;
    case 4: return smsw ( INTERP, modrmbyte );
      
    case 6: return lmsw ( INTERP, modrmbyte );
    case 7:
      if ( (modrmbyte>>6) == 0x3 )
        unk_inst ( INTERP, modrmbyte, "0F 01 " );
      else invlpg ( INTERP, modrmbyte );
      break;
    default: unk_inst ( INTERP, modrmbyte, "0F 01 " );
    }
  
} // end inst_0f_01


static void
inst_0f (
         PROTO_INTERP
         )
{

  uint8_t opcode;
  

  READB_INST ( P_CS, EIP, &opcode, return );
  ++EIP;
  switch ( opcode )
    {
    case 0x00: return inst_0f_00 ( INTERP );
    case 0x01: return inst_0f_01 ( INTERP );
    case 0x02: return lar ( INTERP );
    case 0x03: return lsl ( INTERP );

    case 0x06: return clts ( INTERP );

    case 0x09: return wbinvd ( INTERP );
      
    case 0x20: return mov_r32_cr ( INTERP );
    case 0x21: return mov_r32_dr ( INTERP );
    case 0x22: return mov_cr_r32 ( INTERP );
    case 0x23: return mov_dr_r32 ( INTERP );
      
      /*
    case 0x54: return andps_rxmm_rmxmm128 ( INTERP );
    case 0x55: return andnps_rxmm_rmxmm128 ( INTERP );
      
    case 0x58: return addps_rxmm_rmxmm128 ( INTERP );
      */
    case 0x80: return jo_rel ( INTERP );
      
    case 0x82: return jb_rel ( INTERP );
    case 0x83: return jae_rel ( INTERP );
    case 0x84: return je_rel ( INTERP );
    case 0x85: return jne_rel ( INTERP );
    case 0x86: return jna_rel ( INTERP );
    case 0x87: return ja_rel ( INTERP );
    case 0x88: return js_rel ( INTERP );
    case 0x89: return jns_rel ( INTERP );

    case 0x8c: return jl_rel ( INTERP );
    case 0x8d: return jge_rel ( INTERP );
    case 0x8e: return jng_rel ( INTERP );
    case 0x8f: return jg_rel ( INTERP );

    case 0x92: return setb_rm8 ( INTERP );
    case 0x93: return setae_rm8 ( INTERP );
    case 0x94: return sete_rm8 ( INTERP );
    case 0x95: return setne_rm8 ( INTERP );
    case 0x96: return setna_rm8 ( INTERP );
    case 0x97: return seta_rm8 ( INTERP );
    case 0x98: return sets_rm8 ( INTERP );

    case 0x9c: return setl_rm8 ( INTERP );
    case 0x9d: return setge_rm8 ( INTERP );
    case 0x9e: return setng_rm8 ( INTERP );
    case 0x9f: return setg_rm8 ( INTERP );
    case 0xa0: return push_FS ( INTERP );
    case 0xa1: return pop_FS ( INTERP );
    case 0xa2: return cpuid ( INTERP );
    case 0xa3: return bt_rm_r ( INTERP );
    case 0xa4: return shld_rm_r_imm8 ( INTERP );
    case 0xa5: return shld_rm_r_CL ( INTERP );

    case 0xa8: return push_GS ( INTERP );
    case 0xa9: return pop_GS ( INTERP );
        
    case 0xab: return bts_rm_r ( INTERP );
    case 0xac: return shrd_rm_r_imm8 ( INTERP );
    case 0xad: return shrd_rm_r_CL ( INTERP );

    case 0xaf: return imul_r_rm ( INTERP );

    case 0xb2: return lss_r_m16_off ( INTERP );
    case 0xb3: return btr_rm_r ( INTERP );
    case 0xb4: return lfs_r_m16_off ( INTERP );
    case 0xb5: return lgs_r_m16_off ( INTERP );
    case 0xb6: return movzx_r_rm8 ( INTERP );
    case 0xb7: return movzx_r32_rm16 ( INTERP );
      
    case 0xba: return inst_0f_ba ( INTERP );
      /*
    case 0xbb: return btc_rm_r ( INTERP );
      */
    case 0xbc: return bsf_r_rm ( INTERP );
    case 0xbd: return bsr_r_rm ( INTERP );
    case 0xbe: return movsx_r_rm8 ( INTERP );
    case 0xbf: return movsx_r32_rm16 ( INTERP );

    case 0xc8 ... 0xcf: return bswap_r ( INTERP, opcode );
      
    default: unk_inst ( INTERP, opcode, "0F " );
    }
  
} // end inst_0f


static void
switch_op_size_and_other (
        		  PROTO_INTERP
        		  )
{

  uint8_t opcode;


  READB_INST ( P_CS, EIP, &opcode, return );
  ++EIP;

  if ( INTERP->_group3_inst ) exec_inst ( INTERP, opcode );
  else
    {
      INTERP->_group3_inst= true;
      SWITCH_OPERAND_SIZE;
      exec_inst ( INTERP, opcode );
      DONT_SWITCH_OPERAND_SIZE;
      INTERP->_group3_inst= false;
    }
  
} // end switch_op_size_and_other


static void
switch_addr_size (
        	  PROTO_INTERP
        	  )
{

  uint8_t opcode;


  READB_INST ( P_CS, EIP, &opcode, return );
  ++EIP;

  if ( INTERP->_group4_inst ) exec_inst ( INTERP, opcode );
  else
    {
      INTERP->_group4_inst= true;
      SWITCH_ADDRESS_SIZE;
      exec_inst ( INTERP, opcode );
      DONT_SWITCH_ADDRESS_SIZE;
      INTERP->_group4_inst= false;
    }
  
} // end switch_addr_size 


static void
inst_f6 (
         PROTO_INTERP
         )
{

  uint8_t modrmbyte;


  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return test_rm8_imm8 ( INTERP, modrmbyte );

    case 2: return not_rm8 ( INTERP, modrmbyte );
    case 3: return neg_rm8 ( INTERP, modrmbyte );
    case 4: return mul_rm8 ( INTERP, modrmbyte );
    case 5: return imul_rm8 ( INTERP, modrmbyte );
    case 6: return div_rm8 ( INTERP, modrmbyte );
    case 7: return idiv_rm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "F6 " );
    }
  
} // end inst_f6


static void
inst_f7 (
         PROTO_INTERP
         )
{

  uint8_t modrmbyte;


  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return test_rm_imm ( INTERP, modrmbyte );
        
    case 2: return not_rm ( INTERP, modrmbyte );
    case 3: return neg_rm ( INTERP, modrmbyte );
    case 4: return mul_rm ( INTERP, modrmbyte );
    case 5: return imul_rm ( INTERP, modrmbyte );
    case 6: return div_rm ( INTERP, modrmbyte );
    case 7: return idiv_rm ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "F7 " );
    }
  
} // end inst_f7


static void
repne_repnz_and_other (
        	       PROTO_INTERP
        	       )
{

  uint8_t opcode;
  
  
  READB_INST ( P_CS, EIP, &opcode, return );
  ++EIP;

  if ( INTERP->_group1_inst ) exec_inst ( INTERP, opcode );
  else
    {
      INTERP->_group1_inst= true;
      ENABLE_REPNE_REPNZ;
      exec_inst ( INTERP, opcode );
      DISABLE_REPNE_REPNZ;
      INTERP->_group1_inst= false;
    }
  
} // end repne_repnz_and_other


static void
repe_repz_and_other (
        	     PROTO_INTERP
        	     )
{

  uint8_t opcode;
  

  READB_INST ( P_CS, EIP, &opcode, return );
  ++EIP;

  if ( INTERP->_group1_inst ) exec_inst ( INTERP, opcode );
  else
    {
      INTERP->_group1_inst= true;
      ENABLE_REPE_REPZ;
      exec_inst ( INTERP, opcode );
      DISABLE_REPE_REPZ;
      INTERP->_group1_inst= false;
    }
  
} // end repe_repz_and_other


static void
inst_fe (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inc_rm8 ( INTERP, modrmbyte );
    case 1: return dec_rm8 ( INTERP, modrmbyte );
    default: unk_inst ( INTERP, modrmbyte, "FE " );
    }
  
} // end inst_fe


static void
inst_ff (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inc_rm ( INTERP, modrmbyte );
    case 1: return dec_rm ( INTERP, modrmbyte );
    case 2: return call_rm ( INTERP, modrmbyte );
    case 3: return call_m16_off ( INTERP, modrmbyte );
    case 4: return jmp_rm ( INTERP, modrmbyte );
    case 5: return jmp_m16_off ( INTERP, modrmbyte );
    case 6: return push_rm ( INTERP, modrmbyte );
      
    default: unk_inst ( INTERP, modrmbyte, "FF " );
    }
  
} // end inst_ff


static void
exec_inst (
           PROTO_INTERP,
           const uint8_t opcode
           )
{
  
  switch ( opcode )
    {
    case 0x00: return add_rm8_r8 ( INTERP );
    case 0x01: return add_rm_r ( INTERP );
    case 0x02: return add_r8_rm8 ( INTERP );
    case 0x03: return add_r_rm ( INTERP );
    case 0x04: return add_AL_imm8 ( INTERP );
    case 0x05: return add_A_imm ( INTERP );
    case 0x06: return push_ES ( INTERP );
    case 0x07: return pop_ES ( INTERP );
    case 0x08: return or_rm8_r8 ( INTERP );
    case 0x09: return or_rm_r ( INTERP );
    case 0x0a: return or_r8_rm8 ( INTERP );
    case 0x0b: return or_r_rm ( INTERP );
    case 0x0c: return or_AL_imm8 ( INTERP );
    case 0x0d: return or_A_imm ( INTERP );
    case 0x0e: return push_CS ( INTERP );
    case 0x0f: return inst_0f ( INTERP );
    case 0x10: return adc_rm8_r8 ( INTERP );
    case 0x11: return adc_rm_r ( INTERP );
    case 0x12: return adc_r8_rm8 ( INTERP );
    case 0x13: return adc_r_rm ( INTERP );
    case 0x14: return adc_AL_imm8 ( INTERP );
    case 0x15: return adc_A_imm ( INTERP );
    case 0x16: return push_SS ( INTERP );
    case 0x17: return pop_SS ( INTERP );
    case 0x18: return sbb_rm8_r8 ( INTERP );
    case 0x19: return sbb_rm_r ( INTERP );
    case 0x1a: return sbb_r8_rm8 ( INTERP );
    case 0x1b: return sbb_r_rm ( INTERP );
    case 0x1c: return sbb_AL_imm8 ( INTERP );
    case 0x1d: return sbb_A_imm ( INTERP );
    case 0x1e: return push_DS ( INTERP );
    case 0x1f: return pop_DS ( INTERP );
    case 0x20: return and_rm8_r8 ( INTERP );
    case 0x21: return and_rm_r ( INTERP );
    case 0x22: return and_r8_rm8 ( INTERP );
    case 0x23: return and_r_rm ( INTERP );
    case 0x24: return and_AL_imm8 ( INTERP );
    case 0x25: return and_A_imm ( INTERP );
    case 0x26: return override_seg ( INTERP, P_ES );
    case 0x27: return daa ( INTERP );
    case 0x28: return sub_rm8_r8 ( INTERP );
    case 0x29: return sub_rm_r ( INTERP );
    case 0x2a: return sub_r8_rm8 ( INTERP );
    case 0x2b: return sub_r_rm ( INTERP );
    case 0x2c: return sub_AL_imm8 ( INTERP );
    case 0x2d: return sub_A_imm ( INTERP );
    case 0x2e: return override_seg ( INTERP, P_CS );
    case 0x2f: return das ( INTERP );
    case 0x30: return xor_rm8_r8 ( INTERP );
    case 0x31: return xor_rm_r ( INTERP );
    case 0x32: return xor_r8_rm8 ( INTERP );
    case 0x33: return xor_r_rm ( INTERP );
    case 0x34: return xor_AL_imm8 ( INTERP );
    case 0x35: return xor_A_imm ( INTERP );
    case 0x36: return override_seg ( INTERP, P_SS );
      /*
    case 0x37: return aaa ( INTERP );
      */
    case 0x38: return cmp_rm8_r8 ( INTERP );
    case 0x39: return cmp_rm_r ( INTERP );
    case 0x3a: return cmp_r8_rm8 ( INTERP );
    case 0x3b: return cmp_r_rm ( INTERP );
    case 0x3c: return cmp_AL_imm8 ( INTERP );
    case 0x3d: return cmp_A_imm ( INTERP );
    case 0x3e: return override_seg ( INTERP, P_DS );
    case 0x3f: return aas ( INTERP );
    case 0x40 ... 0x47: return inc_r ( INTERP, opcode );
    case 0x48 ... 0x4f: return dec_r ( INTERP, opcode );
    case 0x50 ... 0x57: return push_r ( INTERP, opcode );
    case 0x58 ... 0x5f: return pop_r ( INTERP, opcode );
    case 0x60: return pusha ( INTERP );
    case 0x61: return popa ( INTERP );
    case 0x62: return bound ( INTERP );
      /*
    case 0x63: return arpl ( INTERP );
      */
    case 0x64: return override_seg ( INTERP, P_FS );
    case 0x65: return override_seg ( INTERP, P_GS );
    case 0x66: return switch_op_size_and_other ( INTERP );
    case 0x67: return switch_addr_size ( INTERP );
    case 0x68: return push_imm ( INTERP );
    case 0x69: return imul_r_rm_imm ( INTERP );
    case 0x6a: return push_imm8 ( INTERP );
    case 0x6b: return imul_r_rm_imm8 ( INTERP );
    case 0x6c: return insb ( INTERP );
    case 0x6d: return ins ( INTERP );
    case 0x6e: return outsb ( INTERP );
    case 0x6f: return outs ( INTERP );
    case 0x70: return jo_rel8 ( INTERP );
    case 0x71: return jno_rel8 ( INTERP );
    case 0x72: return jb_rel8 ( INTERP );
    case 0x73: return jae_rel8 ( INTERP );
    case 0x74: return je_rel8 ( INTERP );
    case 0x75: return jne_rel8 ( INTERP );
    case 0x76: return jna_rel8 ( INTERP );
    case 0x77: return ja_rel8 ( INTERP );
    case 0x78: return js_rel8 ( INTERP );
    case 0x79: return jns_rel8 ( INTERP );
    case 0x7a: return jp_rel8 ( INTERP );
    case 0x7b: return jpo_rel8 ( INTERP );
    case 0x7c: return jl_rel8 ( INTERP );
    case 0x7d: return jge_rel8 ( INTERP );
    case 0x7e: return jng_rel8 ( INTERP );
    case 0x7f: return jg_rel8 ( INTERP );
    case 0x80: return inst_80 ( INTERP );
    case 0x81: return inst_81 ( INTERP );
      
    case 0x83: return inst_83 ( INTERP );
    case 0x84: return test_rm8_r8 ( INTERP );
    case 0x85: return test_rm_r ( INTERP );
    case 0x86: return xchg_r8_rm8 ( INTERP );
    case 0x87: return xchg_r_rm ( INTERP );
    case 0x88: return mov_rm8_r8 ( INTERP );
    case 0x89: return mov_rm_r ( INTERP );
    case 0x8a: return mov_r8_rm8 ( INTERP );
    case 0x8b: return mov_r_rm ( INTERP );
    case 0x8c: return mov_rm_sreg ( INTERP );
    case 0x8d: return lea_r_m ( INTERP );
    case 0x8e: return mov_sreg_rm16 ( INTERP );
    case 0x8f: return inst_8f ( INTERP );
    case 0x90: return nop ( INTERP );
    case 0x91 ... 0x97: return xchg_A_r ( INTERP, opcode );
    case 0x98: return cbw_cwde ( INTERP );
    case 0x99: return cwd_cdq ( INTERP );
    case 0x9a: return call_ptr16_16_32 ( INTERP );
    case 0x9b: return fwait ( INTERP );
    case 0x9c: return pushf ( INTERP );
    case 0x9d: return popf ( INTERP );
    case 0x9e: return sahf ( INTERP );
    case 0x9f: return lahf ( INTERP );
    case 0xa0: return mov_AL_moffs8 ( INTERP );
    case 0xa1: return mov_A_moffs ( INTERP );
    case 0xa2: return mov_moffs8_AL ( INTERP );
    case 0xa3: return mov_moffs_A ( INTERP );
    case 0xa4: return movsb ( INTERP );
    case 0xa5: return movs ( INTERP );
    case 0xa6: return cmpsb ( INTERP );
    case 0xa7: return cmps ( INTERP );
    case 0xa8: return test_AL_imm8 ( INTERP );
    case 0xa9: return test_A_imm ( INTERP );
    case 0xaa: return stosb ( INTERP );
    case 0xab: return stos ( INTERP );
    case 0xac: return lodsb ( INTERP );
    case 0xad: return lods ( INTERP );
    case 0xae: return scasb ( INTERP );
    case 0xaf: return scas ( INTERP );
    case 0xb0 ... 0xb7: return mov_r8_imm8 ( INTERP, opcode );
    case 0xb8 ... 0xbf: return mov_r_imm ( INTERP, opcode );
    case 0xc0: return inst_c0 ( INTERP );
    case 0xc1: return inst_c1 ( INTERP );
    case 0xc2: return ret_near_imm16 ( INTERP );
    case 0xc3: return ret_near ( INTERP );
    case 0xc4: return les_r_m16_off ( INTERP );
    case 0xc5: return lds_r_m16_off ( INTERP );
    case 0xc6: return inst_c6 ( INTERP );
    case 0xc7: return inst_c7 ( INTERP );
    case 0xc8: return enter ( INTERP );
    case 0xc9: return leave ( INTERP );
    case 0xca: return ret_far_imm16 ( INTERP );
    case 0xcb: return ret_far ( INTERP );
    case 0xcc: return int3 ( INTERP );
    case 0xcd: return int_imm8 ( INTERP );
    case 0xce: return into ( INTERP );
    case 0xcf: return iret ( INTERP );
    case 0xd0: return inst_d0 ( INTERP );
    case 0xd1: return inst_d1 ( INTERP );
    case 0xd2: return inst_d2 ( INTERP );
    case 0xd3: return inst_d3 ( INTERP );
    case 0xd4: return aam ( INTERP );
    case 0xd5: return aad ( INTERP );

    case 0xd7: return xlatb ( INTERP );
    case 0xd8: return inst_d8 ( INTERP );
    case 0xd9: return inst_d9 ( INTERP );
    case 0xda: return inst_da ( INTERP );
    case 0xdb: return inst_db ( INTERP );
    case 0xdc: return inst_dc ( INTERP );
    case 0xdd: return inst_dd ( INTERP );
    case 0xde: return inst_de ( INTERP );
    case 0xdf: return inst_df ( INTERP );
    case 0xe0: return loopne_rel8 ( INTERP );
    case 0xe1: return loope_rel8 ( INTERP );
    case 0xe2: return loop_rel8 ( INTERP );
    case 0xe3: return jcxz_rel8 ( INTERP );
    case 0xe4: return in_AL_imm8 ( INTERP );
      
    case 0xe6: return out_imm8_AL ( INTERP );
    case 0xe7: return out_imm8_A ( INTERP );
    case 0xe8: return call_rel ( INTERP );
    case 0xe9: return jmp_rel ( INTERP );
    case 0xea: return jmp_ptr16_16_32 ( INTERP );
    case 0xeb: return jmp_rel8 ( INTERP );
    case 0xec: return in_AL_DX ( INTERP );
    case 0xed: return in_A_DX ( INTERP );
    case 0xee: return out_DX_AL ( INTERP );
    case 0xef: return out_DX_A ( INTERP );
      //case 0xf0: return lock ( INTERP );
      
    case 0xf2: return repne_repnz_and_other ( INTERP );
    case 0xf3: return repe_repz_and_other ( INTERP );
    case 0xf4: return hlt ( INTERP );
    case 0xf5: return cmc ( INTERP );
    case 0xf6: return inst_f6 ( INTERP );
    case 0xf7: return inst_f7 ( INTERP );
    case 0xf8: return clc ( INTERP );
    case 0xf9: return stc ( INTERP );
    case 0xfa: return cli ( INTERP );
    case 0xfb: return sti ( INTERP );
    case 0xfc: return cld ( INTERP );
    case 0xfd: return std ( INTERP );
    case 0xfe: return inst_fe ( INTERP );
    case 0xff: return inst_ff ( INTERP );
    default: unk_inst ( INTERP, opcode, "" );
    }
  
} // end exec_inst




/***************/
/* MÈTODES DIS */
/***************/

static bool
dis_mem_read (
              void           *udata,
              const uint64_t  offset,
              uint8_t        *data
              )
{

  PROTO_INTERP;
  bool ret;
  

  ret= true;
  interpreter= (IA32_Interpreter *) udata;
  interpreter->_ignore_exceptions= true;
  READB_INST ( P_CS, ((uint64_t) EIP) + offset, data, ret= false );
  interpreter->_ignore_exceptions= false;
  
  return ret;
  
} // end dis_mem_read


static bool
dis_address_size_is_32 (
                        void *udata
                        )
{
  PROTO_INTERP;
  interpreter= (IA32_Interpreter *) udata;
  return ADDRESS_SIZE_IS_32;
} // end dis_address_size_is_32


static bool
dis_operand_size_is_32 (
                        void *udata
                        )
{
  PROTO_INTERP;
  interpreter= (IA32_Interpreter *) udata;
  return OPERAND_SIZE_IS_32;
} // end dis_operand_size_is_32




/**********************/
/* FUNCIONS PÚBLIQUES */
/**********************/

void
IA32_interpreter_init (
        	       PROTO_INTERP
        	       )
{
  
  INTERP->_locked= false;
  INTERP->_data_seg= NULL;
  INTERP->_switch_op_size= 0;
  INTERP->_switch_addr_size= 0;
  INTERP->_group1_inst= false;
  INTERP->_group2_inst= false;
  INTERP->_group3_inst= false;
  INTERP->_group4_inst= false;
  INTERP->_inhibit_interrupt= false;
  INTERP->_repe_repz_enabled= false;
  INTERP->_repne_repnz_enabled= false;
  INTERP->_intr= false;
  INTERP->_halted= false;
  INTERP->_ignore_exceptions= false;
  INTERP->_int_counter= 0;
  
  // Memòria.
  update_mem_callbacks ( INTERP );
  
} // end IA32_interpreter_init


void
IA32_exec_next_inst (
        	     PROTO_INTERP
        	     )
{

  uint8_t opcode,ivec;


  // Interrupcions.
  if ( !(INTERP->_inhibit_interrupt) )
    {
      if ( INTERP->_intr && (EFLAGS&IF_FLAG)!= 0 )
        {
          ivec= IA32_ack_intr ();
          INTERRUPTION ( ivec, INTERRUPTION_TYPE_UNK, return );
          return;
        }
    }
  else INTERP->_inhibit_interrupt= false;

  if ( !INTERP->_halted )
    {
      INTERP->_old_EIP= EIP;
      READB_INST ( P_CS, EIP, &opcode, return );
      ++EIP;
      exec_inst ( interpreter, opcode );
    }
  
} // end IA32_exec_next_inst


void
IA32_interpreter_init_dis (
                           IA32_Interpreter  *interpreter,
                           IA32_Disassembler *dis
                           )
{

  dis->udata= (void *) interpreter;
  dis->mem_read= dis_mem_read;
  dis->address_size_is_32= dis_address_size_is_32;
  dis->operand_size_is_32= dis_operand_size_is_32;
  
} // end IA32_interpreter_init_dis


void
IA32_set_intr (
               PROTO_INTERP,
               const bool value
               )
{
  INTERP->_intr= value;
} // end IA32_set_intr
