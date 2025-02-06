/*
 * Copyright 2021-2025 Adrià Giménez Pastor.
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
 *  jit.c - Implementació de 'JIT'.
 *
 */


#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "IA32.h"




/**********/
/* MACROS */
/**********/

#define FERROR stdout

#define WW (jit->warning)
#define UDATA (jit->udata)

#define NULL_ENTRY 0
#define PAD_ENTRY 1

#define l_P_CS (&(l_cpu->cs))
#define l_P_DS (&(l_cpu->ds))
#define l_P_SS (&(l_cpu->ss))
#define l_P_ES (&(l_cpu->es))
#define l_P_FS (&(l_cpu->fs))
#define l_P_GS (&(l_cpu->gs))

#define l_EFLAGS (l_cpu->eflags)
#define l_EIP    (l_cpu->eip)
#define l_CR0    (l_cpu->cr0)
#define l_CR2    (l_cpu->cr2)
#define l_CR3    (l_cpu->cr3)
#define l_CR4    (l_cpu->cr4)

#define l_DR0    (l_cpu->dr0)
#define l_DR1    (l_cpu->dr1)
#define l_DR2    (l_cpu->dr2)
#define l_DR3    (l_cpu->dr3)
#define l_DR4    (l_cpu->dr4)
#define l_DR5    (l_cpu->dr5)
#define l_DR6    (l_cpu->dr6)
#define l_DR7    (l_cpu->dr7)

#define l_EAX (l_cpu->eax.v)
#define l_EBX (l_cpu->ebx.v)
#define l_ECX (l_cpu->ecx.v)
#define l_EDX (l_cpu->edx.v)
#define l_EBP (l_cpu->ebp.v)
#define l_ESI (l_cpu->esi.v)
#define l_EDI (l_cpu->edi.v)
#define l_ESP (l_cpu->esp.v)
#define l_AX  (l_cpu->eax.w.v0)
#define l_BX  (l_cpu->ebx.w.v0)
#define l_CX  (l_cpu->ecx.w.v0)
#define l_DX  (l_cpu->edx.w.v0)
#define l_BP  (l_cpu->ebp.w.v0)
#define l_SI  (l_cpu->esi.w.v0)
#define l_DI  (l_cpu->edi.w.v0)
#define l_SP  (l_cpu->esp.w.v0)
#define l_AH  (l_cpu->eax.b.v1)
#define l_AL  (l_cpu->eax.b.v0)
#define l_BH  (l_cpu->ebx.b.v1)
#define l_BL  (l_cpu->ebx.b.v0)
#define l_CH  (l_cpu->ecx.b.v1)
#define l_CL  (l_cpu->ecx.b.v0)
#define l_DH  (l_cpu->edx.b.v1)
#define l_DL  (l_cpu->edx.b.v0)

#define l_IDTR (l_cpu->idtr)
#define l_GDTR (l_cpu->gdtr)
#define l_LDTR (l_cpu->ldtr)
#define l_TR (l_cpu->tr)

#define CS_ID 0
#define DS_ID 1
#define SS_ID 2
#define ES_ID 3
#define FS_ID 4
#define GS_ID 5

#define P_CS (jit->_seg_regs[CS_ID])
#define P_DS (jit->_seg_regs[DS_ID])
#define P_SS (jit->_seg_regs[SS_ID])
#define P_ES (jit->_seg_regs[ES_ID])
#define P_FS (jit->_seg_regs[FS_ID])
#define P_GS (jit->_seg_regs[GS_ID])

#define CPL (P_CS->h.pl)
#define l_CPL (l_P_CS->h.pl)

#define IDTR (jit->_cpu->idtr)
#define GDTR (jit->_cpu->gdtr)
#define LDTR (jit->_cpu->ldtr)

#define EFLAGS (jit->_cpu->eflags)
#define EIP    (jit->_cpu->eip)
#define CR0    (jit->_cpu->cr0)
#define CR2    (jit->_cpu->cr2)
#define CR3    (jit->_cpu->cr3)
#define CR4    (jit->_cpu->cr4)

#define IOPL (((EFLAGS)&(IOPL_FLAG))>>12)
#define l_IOPL (((l_EFLAGS)&(IOPL_FLAG))>>12)

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
#define CR4_PSE        0x00000010
#define CR4_PAE        0x00000020
#define CR4_OSFXSR     0x00000200
#define CR4_OSXMMEXCPT 0x00000400
#define CR4_SMEP       0x00100000
#define CR4_SMAP       0x00200000

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

#define OSZACP_FLAGS (OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|CF_FLAG|PF_FLAG)

// NOTA!!! TOP HO MODELE A PART
#define l_FPU_TOP (l_cpu->fpu.top)
#define l_FPU_STATUS (l_cpu->fpu.status)
#define l_FPU_STATUS_TOP (((l_FPU_STATUS)&0xC7FF)|((l_FPU_TOP)<<11))
#define l_FPU_CONTROL (l_cpu->fpu.control)
#define l_FPU_DPTR (l_cpu->fpu.dptr)
#define l_FPU_IPTR (l_cpu->fpu.iptr)
#define l_FPU_OPCODE (l_cpu->fpu.opcode)
#define l_FPU_REG(IND) ((l_cpu->fpu.regs)[(IND)])

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

#define EXCEPTION(VECTOR)                                               \
  {                                                                     \
    if ( !jit->_ignore_exceptions )                                     \
      {                                                                 \
        /*WW ( UDATA, "excepció: %d",(VECTOR) );*/                      \
        jit->_exception.vec= (VECTOR);                                  \
        jit->_exception.with_selector= false;                           \
        jit->_exception.use_error_code= false;                          \
      }                                                                 \
    }

#define EXCEPTION0(VECTOR)                      \
  EXCEPTION_ERROR_CODE ( (VECTOR), 0 )

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#define EXCEPTION_SEL(VECTOR,SELECTOR)                                  \
  {                                                                     \
    if ( !jit->_ignore_exceptions )                                     \
      {                                                                 \
        /*WW ( UDATA, "excepció sel: %d:%X", (VECTOR), (SELECTOR) );*/  \
        jit->_exception.vec= (VECTOR);                                  \
        jit->_exception.with_selector= true;                            \
        jit->_exception.selector= (SELECTOR);                           \
        jit->_exception.use_error_code= false;                          \
      }                                                                 \
    }

#define EXCEPTION_ERROR_CODE(VECTOR,ERROR)                              \
  {                                                                     \
    if ( !jit->_ignore_exceptions )                                     \
      {                                                                 \
        /*WW ( UDATA, "excepció error code: %d:%X", (VECTOR), (ERROR) );*/ \
        jit->_exception.vec= (VECTOR);                                  \
        jit->_exception.with_selector= false;                           \
        jit->_exception.use_error_code= true;                           \
        jit->_exception.error_code= (ERROR);                            \
      }                                                                 \
  }
#pragma GCC diagnostic pop

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

#define PROTECTED_MODE_ACTIVATED ((CR0&CR0_PE)!=0)
#define ADDR_OP_SIZE_IS_32                                              \
  ((PROTECTED_MODE_ACTIVATED&&((EFLAGS&VM_FLAG)==0)&&P_CS->h.is32))
 
// Macros per a llegir de la memòria.
#define READU8(ADDR,IS_DATA) jit->mem_read8 ( jit->udata, ADDR, IS_DATA )
#define READU16(ADDR) jit->mem_read16 ( jit->udata, ADDR )
#define READU32(ADDR) jit->mem_read32 ( jit->udata, ADDR )
#define READU64(ADDR) jit->mem_read64 ( jit->udata, ADDR )
#define WRITEU8(ADDR,DATA) jit->mem_write8 ( jit->udata, ADDR, DATA )
#define WRITEU16(ADDR,DATA) jit->mem_write16 ( jit->udata, ADDR, DATA )
#define WRITEU32(ADDR,DATA) jit->mem_write32 ( jit->udata, ADDR, DATA )

// Macros per a llegir del 'linear-address space'.
#define READLB(ADDR,P_DST,ACTION_ON_ERROR,IS_DATA)                      \
  if ( jit->_mem_readl8 ( jit, ADDR, P_DST, IS_DATA ) != 0 )            \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }
#define READLW(ADDR,P_DST,ACTION_ON_ERROR,IS_DATA,ISVM)                 \
  if ( jit->_mem_readl16 ( jit, ADDR, P_DST, IS_DATA, ISVM ) != 0 )     \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }
#define READLD(ADDR,P_DST,ACTION_ON_ERROR,IS_DATA,ISVM)                 \
  if ( jit->_mem_readl32 ( jit, ADDR, P_DST, IS_DATA, ISVM ) != 0 )     \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }
#define READLQ(ADDR,P_DST,ACTION_ON_ERROR,IS_DATA)                      \
  if ( jit->_mem_readl64 ( jit, ADDR, P_DST, IS_DATA ) != 0 )           \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }
#define READLDQ(ADDR,P_DST,ACTION_ON_ERROR,IS_DATA)                     \
  if ( jit->_mem_readl128 ( jit, ADDR, P_DST, IS_DATA ) != 0 )          \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }

// Macros per a escriure en el 'linear-address-space'.
#define WRITELB(ADDR,BYTE,ACTION_ON_ERROR)                              \
  if ( jit->_mem_writel8 ( jit, ADDR, BYTE ) != 0 )                     \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }
#define WRITELW(ADDR,WORD,ACTION_ON_ERROR)                              \
  if ( jit->_mem_writel16 ( jit, ADDR, WORD ) != 0 )                    \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }
#define WRITELD(ADDR,DWORD,ACTION_ON_ERROR)                             \
  if ( jit->_mem_writel32 ( jit, ADDR, DWORD ) != 0 )                   \
    {                                                                   \
      ACTION_ON_ERROR;                                                  \
    }

// UTILS SEGMENT DESCRIPTORS *
#define SEG_DESC_GET_STYPE(DESC) (((DESC).w0>>8)&0x1F)
#define SEG_DESC_IS_PRESENT(DESC) (((DESC).w0&0x00008000)!=0)
#define SEG_DESC_GET_DPL(DESC) (((DESC).w0>>13)&0x3)
#define SEG_DESC_G_IS_SET(DESC) (((DESC).w0&0x00800000)!=0)
#define SEG_DESC_B_IS_SET(DESC) (((DESC).w0&0x00400000)!=0)
#define CG_DESC_GET_SELECTOR(DESC) ((uint16_t) ((DESC).w1>>16))
#define CG_DESC_GET_OFFSET(DESC)                        \
  (((DESC).w0&0xFFFF0000) | ((DESC).w1&0x0000FFFF))
#define CG_DESC_GET_PARAMCOUNT(DESC) ((DESC).w0&0x1F)
#define TG_DESC_GET_SELECTOR(DESC) ((uint16_t) ((DESC).w1>>16))
#define TSS_DESC_IS_BUSY(DESC) (((DESC).w0&0x200)!=0)
#define TSS_DESC_IS_AVAILABLE(DESC) (((DESC).w0&0x100000)!=0)

#define INTERRUPTION_TYPE_IMM  0
#define INTERRUPTION_TYPE_INT3 1
#define INTERRUPTION_TYPE_INTO 2
#define INTERRUPTION_TYPE_UNK 3




/*********/
/* TIPUS */
/*********/

enum {

  // Bytecodes final d'instrucció i altres especials (repeticions, etc.)
  BC_GOTO_EIP= 0,
  BC_INC1_EIP,
  BC_INC2_EIP,
  BC_INC3_EIP,
  BC_INC4_EIP,
  BC_INC5_EIP,
  BC_INC6_EIP,
  BC_INC7_EIP,
  BC_INC8_EIP,
  BC_INC9_EIP,
  BC_INC10_EIP,
  BC_INC11_EIP,
  BC_INC12_EIP,
  BC_INC14_EIP,
  BC_INC1_EIP_NOSTOP,
  BC_INC2_EIP_NOSTOP,
  BC_INC3_EIP_NOSTOP,
  BC_INC4_EIP_NOSTOP,
  BC_INC5_EIP_NOSTOP,
  BC_INC6_EIP_NOSTOP,
  BC_INC7_EIP_NOSTOP,
  BC_INC8_EIP_NOSTOP,
  BC_INC9_EIP_NOSTOP,
  BC_INC10_EIP_NOSTOP,
  BC_INC11_EIP_NOSTOP,
  BC_INC12_EIP_NOSTOP,
  BC_INC14_EIP_NOSTOP,
  BC_WRONG_INST,  // Aquesta instrucció està sempre en bucle
  BC_INC2_PC_IF_ECX_IS_0,
  BC_INC2_PC_IF_CX_IS_0,
  BC_INC4_PC_IF_ECX_IS_0,
  BC_INC4_PC_IF_CX_IS_0,
  BC_INCIMM_PC_IF_CX_IS_0,
  BC_INCIMM_PC_IF_ECX_IS_0,
  BC_DEC1_PC_IF_REP32,
  BC_DEC1_PC_IF_REP16,
  BC_DEC3_PC_IF_REP32,
  BC_DEC3_PC_IF_REP16,
  BC_DECIMM_PC_IF_REPE32,
  BC_DECIMM_PC_IF_REPNE32,
  BC_DECIMM_PC_IF_REPE16,
  BC_DECIMM_PC_IF_REPNE16,
  BC_UNK,
  
  // Bytecodes carrega dades
  // --> Offsets i ports
  BC_SET32_IMM32_EIP,
  BC_SET32_IMM_SELECTOR_OFFSET,
  BC_SET32_IMM_OFFSET,
  BC_SET16_IMM_SELECTOR_OFFSET,
  BC_SET16_IMM_OFFSET,
  BC_SET16_IMM_PORT,
  BC_SET32_EAX_OFFSET,
  BC_SET32_ECX_OFFSET,
  BC_SET32_EDX_OFFSET,
  BC_SET32_EBX_OFFSET,
  BC_SET32_EBX_AL_OFFSET,
  BC_SET32_EBP_OFFSET,
  BC_SET32_ESI_OFFSET,
  BC_SET32_EDI_OFFSET,
  BC_SET32_ESP_OFFSET,
  BC_SET16_SI_OFFSET,
  BC_SET16_DI_OFFSET,
  BC_SET16_BP_OFFSET,
  BC_SET16_BP_SI_OFFSET,
  BC_SET16_BP_DI_OFFSET,
  BC_SET16_BX_OFFSET,
  BC_SET16_BX_AL_OFFSET,
  BC_SET16_BX_SI_OFFSET,
  BC_SET16_BX_DI_OFFSET,
  BC_SET16_DX_PORT,
  BC_ADD32_IMM_OFFSET,
  BC_ADD32_BITOFFSETOP1_OFFSET,
  BC_ADD32_EAX_OFFSET,
  BC_ADD32_ECX_OFFSET,
  BC_ADD32_EDX_OFFSET,
  BC_ADD32_EBX_OFFSET,
  BC_ADD32_EBP_OFFSET,
  BC_ADD32_ESI_OFFSET,
  BC_ADD32_EDI_OFFSET,
  BC_ADD32_EAX_2_OFFSET,
  BC_ADD32_ECX_2_OFFSET,
  BC_ADD32_EDX_2_OFFSET,
  BC_ADD32_EBX_2_OFFSET,
  BC_ADD32_EBP_2_OFFSET,
  BC_ADD32_ESI_2_OFFSET,
  BC_ADD32_EDI_2_OFFSET,
  BC_ADD32_EAX_4_OFFSET,
  BC_ADD32_ECX_4_OFFSET,
  BC_ADD32_EDX_4_OFFSET,
  BC_ADD32_EBX_4_OFFSET,
  BC_ADD32_EBP_4_OFFSET,
  BC_ADD32_ESI_4_OFFSET,
  BC_ADD32_EDI_4_OFFSET,
  BC_ADD32_EAX_8_OFFSET,
  BC_ADD32_ECX_8_OFFSET,
  BC_ADD32_EDX_8_OFFSET,
  BC_ADD32_EBX_8_OFFSET,
  BC_ADD32_EBP_8_OFFSET,
  BC_ADD32_ESI_8_OFFSET,
  BC_ADD32_EDI_8_OFFSET,
  BC_ADD16_IMM_OFFSET,
  BC_ADD16_IMM_OFFSET16,
  // --> Condicions
  BC_SET_A_COND,
  BC_SET_AE_COND,
  BC_SET_B_COND,
  BC_SET_CXZ_COND,
  BC_SET_E_COND,
  BC_SET_ECXZ_COND,
  BC_SET_G_COND,
  BC_SET_GE_COND,
  BC_SET_L_COND,
  BC_SET_NA_COND,
  BC_SET_NE_COND,
  BC_SET_NG_COND,
  BC_SET_NO_COND,
  BC_SET_NS_COND,
  BC_SET_O_COND,
  BC_SET_P_COND,
  BC_SET_PO_COND,
  BC_SET_S_COND,  
  BC_SET_DEC_ECX_NOT_ZERO_COND,
  BC_SET_DEC_ECX_NOT_ZERO_AND_ZF_COND,
  BC_SET_DEC_ECX_NOT_ZERO_AND_NOT_ZF_COND,
  BC_SET_DEC_CX_NOT_ZERO_COND,
  BC_SET_DEC_CX_NOT_ZERO_AND_ZF_COND,
  BC_SET_DEC_CX_NOT_ZERO_AND_NOT_ZF_COND,
  // --> Comptador
  BC_SET_1_COUNT,
  BC_SET_CL_COUNT,
  BC_SET_CL_COUNT_NOMOD,
  BC_SET_IMM_COUNT,
  // --> EFLAGS
  BC_SET32_AD_OF_EFLAGS,
  BC_SET32_AD_AF_EFLAGS,
  BC_SET32_AD_CF_EFLAGS,
  BC_SET32_MUL_CFOF_EFLAGS,
  BC_SET32_MUL1_CFOF_EFLAGS,
  BC_SET32_IMUL1_CFOF_EFLAGS,
  BC_SET32_NEG_CF_EFLAGS,
  BC_SET32_RCL_OF_EFLAGS,
  BC_SET32_RCL_CF_EFLAGS,
  BC_SET32_RCR_OF_EFLAGS,
  BC_SET32_RCR_CF_EFLAGS,
  BC_SET32_ROL_CF_EFLAGS,
  BC_SET32_ROR_OF_EFLAGS,
  BC_SET32_ROR_CF_EFLAGS,
  BC_SET32_SAR_OF_EFLAGS,
  BC_SET32_SAR_CF_EFLAGS,
  BC_SET32_SHIFT_SF_EFLAGS,
  BC_SET32_SHIFT_ZF_EFLAGS,
  BC_SET32_SHIFT_PF_EFLAGS,
  BC_SET32_SHL_OF_EFLAGS,
  BC_SET32_SHL_CF_EFLAGS,
  BC_SET32_SHLD_OF_EFLAGS,
  BC_SET32_SHLD_CF_EFLAGS,
  BC_SET32_SHR_OF_EFLAGS,
  BC_SET32_SHR_CF_EFLAGS,
  BC_SET32_SHRD_OF_EFLAGS,
  BC_SET32_SHRD_CF_EFLAGS,
  BC_SET32_SB_OF_EFLAGS,
  BC_SET32_SB_AF_EFLAGS,
  BC_SET32_SB_CF_EFLAGS,
  BC_SET32_SF_EFLAGS,
  BC_SET32_ZF_EFLAGS,
  BC_SET32_PF_EFLAGS,
  BC_SET16_AD_OF_EFLAGS,
  BC_SET16_AD_AF_EFLAGS,
  BC_SET16_AD_CF_EFLAGS,
  BC_SET16_MUL_CFOF_EFLAGS,
  BC_SET16_MUL1_CFOF_EFLAGS,
  BC_SET16_IMUL1_CFOF_EFLAGS,
  BC_SET16_NEG_CF_EFLAGS,
  BC_SET16_RCL_OF_EFLAGS,
  BC_SET16_RCL_CF_EFLAGS,
  BC_SET16_RCR_OF_EFLAGS,
  BC_SET16_RCR_CF_EFLAGS,
  BC_SET16_ROL_CF_EFLAGS,
  BC_SET16_ROR_OF_EFLAGS,
  BC_SET16_ROR_CF_EFLAGS,
  BC_SET16_SAR_CF_EFLAGS,
  BC_SET16_SHIFT_SF_EFLAGS,
  BC_SET16_SHIFT_ZF_EFLAGS,
  BC_SET16_SHIFT_PF_EFLAGS,
  BC_SET16_SHL_OF_EFLAGS,
  BC_SET16_SHL_CF_EFLAGS,
  BC_SET16_SHLD_OF_EFLAGS,
  BC_SET16_SHLD_CF_EFLAGS,
  BC_SET16_SHR_OF_EFLAGS,
  BC_SET16_SHR_CF_EFLAGS,
  BC_SET16_SHRD_OF_EFLAGS,
  BC_SET16_SHRD_CF_EFLAGS,
  BC_SET16_SB_OF_EFLAGS,
  BC_SET16_SB_AF_EFLAGS,
  BC_SET16_SB_CF_EFLAGS,
  BC_SET16_SF_EFLAGS,
  BC_SET16_ZF_EFLAGS,
  BC_SET16_PF_EFLAGS,
  BC_SET8_AD_OF_EFLAGS,
  BC_SET8_AD_AF_EFLAGS,
  BC_SET8_AD_CF_EFLAGS,
  BC_SET8_MUL1_CFOF_EFLAGS,
  BC_SET8_IMUL1_CFOF_EFLAGS,
  BC_SET8_NEG_CF_EFLAGS,
  BC_SET8_SAR_CF_EFLAGS,
  BC_SET8_RCL_OF_EFLAGS,
  BC_SET8_RCL_CF_EFLAGS,
  BC_SET8_RCR_OF_EFLAGS,
  BC_SET8_RCR_CF_EFLAGS,
  BC_SET8_ROL_CF_EFLAGS,
  BC_SET8_ROR_OF_EFLAGS,
  BC_SET8_ROR_CF_EFLAGS,
  BC_SET8_SHIFT_SF_EFLAGS,
  BC_SET8_SHIFT_ZF_EFLAGS,
  BC_SET8_SHIFT_PF_EFLAGS,
  BC_SET8_SHL_OF_EFLAGS,
  BC_SET8_SHL_CF_EFLAGS,
  BC_SET8_SHR_OF_EFLAGS,
  BC_SET8_SHR_CF_EFLAGS,
  BC_SET8_SB_OF_EFLAGS,
  BC_SET8_SB_AF_EFLAGS,
  BC_SET8_SB_CF_EFLAGS,
  BC_SET8_SF_EFLAGS,
  BC_SET8_ZF_EFLAGS,
  BC_SET8_PF_EFLAGS,
  BC_CLEAR_OF_CF_EFLAGS,
  BC_CLEAR_CF_EFLAGS,
  BC_CLEAR_DF_EFLAGS,
  BC_SET_CF_EFLAGS,
  BC_SET_DF_EFLAGS,
  // --> CR0
  BC_CLEAR_TS_CR0,
  // --> Importa molt l'odre dels res/operadors en tots els que son OP0/OP1
  // --> Assignació IMM a operadors
  BC_SET32_SIMM_RES,
  BC_SET32_SIMM_OP0,
  BC_SET32_SIMM_OP1,
  BC_SET32_IMM32_RES,
  BC_SET32_IMM32_OP0,
  BC_SET32_IMM32_OP1,
  BC_SET16_IMM_RES,
  BC_SET16_IMM_OP0,
  BC_SET16_IMM_OP1,
  BC_SET8_IMM_RES,
  BC_SET8_IMM_OP0,
  BC_SET8_IMM_OP1,
  // --> Assignació registres i constants
  BC_SET32_1_OP1,
  BC_SET32_OP0_OP1_0_OP0,
  BC_SET32_OP0_RES,
  BC_SET32_OP1_RES,
  BC_SET32_DR7_RES,
  BC_SET32_DR7_OP0,
  BC_SET32_DR7_OP1,
  BC_SET32_CR0_RES_NOCHECK, // <-- IMPORTANTÍSSIM QUE ESTIGA ABANS
  BC_SET32_CR0_RES,
  BC_SET32_CR0_OP0,
  BC_SET32_CR0_OP1,
  BC_SET32_CR2_RES,
  BC_SET32_CR2_OP0,
  BC_SET32_CR2_OP1,
  BC_SET32_CR3_RES,
  BC_SET32_CR3_OP0,
  BC_SET32_CR3_OP1,
  BC_SET32_EAX_RES,
  BC_SET32_EAX_OP0,
  BC_SET32_EAX_OP1,
  BC_SET32_ECX_RES,
  BC_SET32_ECX_OP0,
  BC_SET32_ECX_OP1,
  BC_SET32_EDX_RES,
  BC_SET32_EDX_OP0,
  BC_SET32_EDX_OP1,
  BC_SET32_EBX_RES,
  BC_SET32_EBX_OP0,
  BC_SET32_EBX_OP1,
  BC_SET32_ESP_RES,
  BC_SET32_ESP_OP0,
  BC_SET32_ESP_OP1,
  BC_SET32_EBP_RES,
  BC_SET32_EBP_OP0,
  BC_SET32_EBP_OP1,
  BC_SET32_ESI_RES,
  BC_SET32_ESI_OP0,
  BC_SET32_ESI_OP1,
  BC_SET32_EDI_RES,
  BC_SET32_EDI_OP0,
  BC_SET32_EDI_OP1,
  BC_SET32_ES_RES,
  BC_SET32_ES_OP0,
  BC_SET32_ES_OP1,
  BC_SET32_CS_RES,
  BC_SET32_CS_OP0,
  BC_SET32_CS_OP1,
  BC_SET32_SS_RES,
  BC_SET32_SS_OP0,
  BC_SET32_SS_OP1,
  BC_SET32_DS_RES,
  BC_SET32_DS_OP0,
  BC_SET32_DS_OP1,
  BC_SET32_FS_RES,
  BC_SET32_FS_OP0,
  BC_SET32_FS_OP1,
  BC_SET32_GS_RES,
  BC_SET32_GS_OP0,
  BC_SET32_GS_OP1,
  BC_SET16_FPUCONTROL_RES,
  BC_SET16_FPUSTATUSTOP_RES,
  BC_SET16_1_OP1,
  BC_SET16_OP0_OP1_0_OP0,
  BC_SET16_OP0_RES,
  BC_SET16_OP1_RES,
  BC_SET16_CR0_RES_NOCHECK,
  BC_SET16_LDTR_RES,
  BC_SET16_TR_RES,
  BC_SET16_AX_RES,
  BC_SET16_AX_OP0,
  BC_SET16_AX_OP1,
  BC_SET16_CX_RES,
  BC_SET16_CX_OP0,
  BC_SET16_CX_OP1,
  BC_SET16_DX_RES,
  BC_SET16_DX_OP0,
  BC_SET16_DX_OP1,
  BC_SET16_BX_RES,
  BC_SET16_BX_OP0,
  BC_SET16_BX_OP1,
  BC_SET16_SP_RES,
  BC_SET16_SP_OP0,
  BC_SET16_SP_OP1,
  BC_SET16_BP_RES,
  BC_SET16_BP_OP0,
  BC_SET16_BP_OP1,
  BC_SET16_SI_RES,
  BC_SET16_SI_OP0,
  BC_SET16_SI_OP1,
  BC_SET16_DI_RES,
  BC_SET16_DI_OP0,
  BC_SET16_DI_OP1,
  BC_SET16_ES_RES,
  BC_SET16_ES_OP0,
  BC_SET16_ES_OP1,
  BC_SET16_CS_RES,
  BC_SET16_CS_OP0,
  BC_SET16_CS_OP1,
  BC_SET16_SS_RES,
  BC_SET16_SS_OP0,
  BC_SET16_SS_OP1,
  BC_SET16_DS_RES,
  BC_SET16_DS_OP0,
  BC_SET16_DS_OP1,
  BC_SET16_FS_RES,
  BC_SET16_FS_OP0,
  BC_SET16_FS_OP1,
  BC_SET16_GS_RES,
  BC_SET16_GS_OP0,
  BC_SET16_GS_OP1,
  BC_SET8_COND_RES,
  BC_SET8_1_OP1,
  BC_SET8_OP0_OP1_0_OP0,
  BC_SET8_OP0_RES,
  BC_SET8_OP1_RES,
  BC_SET8_AL_RES,
  BC_SET8_AL_OP0,
  BC_SET8_AL_OP1,
  BC_SET8_CL_RES,
  BC_SET8_CL_OP0,
  BC_SET8_CL_OP1,
  BC_SET8_DL_RES,
  BC_SET8_DL_OP0,
  BC_SET8_DL_OP1,
  BC_SET8_BL_RES,
  BC_SET8_BL_OP0,
  BC_SET8_BL_OP1,
  BC_SET8_AH_RES,
  BC_SET8_AH_OP0,
  BC_SET8_AH_OP1,
  BC_SET8_CH_RES,
  BC_SET8_CH_OP0,
  BC_SET8_CH_OP1,
  BC_SET8_DH_RES,
  BC_SET8_DH_OP0,
  BC_SET8_DH_OP1,
  BC_SET8_BH_RES,
  BC_SET8_BH_OP0,
  BC_SET8_BH_OP1,
  // --> Assignació resultat
  BC_SET32_RES_FLOATU32,
  BC_SET32_RES_DOUBLEU64L,
  BC_SET32_RES_DOUBLEU64H,
  BC_SET32_RES_LDOUBLEU80L,
  BC_SET32_RES_LDOUBLEU80M,
  BC_SET32_SRES_LDOUBLE,
  BC_SET32_RES_SELECTOR,
  BC_SET32_RES_DR7,
  BC_SET32_RES_CR0,
  BC_SET32_RES_CR2,
  BC_SET32_RES_CR3,
  BC_SET32_RES_EAX,
  BC_SET32_RES_ECX,
  BC_SET32_RES_EDX,
  BC_SET32_RES_EBX,
  BC_SET32_RES_ESP,
  BC_SET32_RES_EBP,
  BC_SET32_RES_ESI,
  BC_SET32_RES_EDI,
  BC_SET32_RES_ES,
  BC_SET32_RES_SS,
  BC_SET32_RES_DS,
  BC_SET32_RES_FS,
  BC_SET32_RES_GS,
  BC_SET32_RES8_RES,
  BC_SET32_RES8_RES_SIGNED,
  BC_SET32_RES16_RES,
  BC_SET32_RES16_RES_SIGNED,
  BC_SET32_RES64_RES,
  BC_SET32_RES64_EAX_EDX,
  BC_SET16_RES_LDOUBLEU80H,
  BC_SET16_RES_SELECTOR,
  BC_SET16_RES_CR0,
  BC_SET16_RES_AX,
  BC_SET16_RES_CX,
  BC_SET16_RES_DX,
  BC_SET16_RES_BX,
  BC_SET16_RES_SP,
  BC_SET16_RES_BP,
  BC_SET16_RES_SI,
  BC_SET16_RES_DI,
  BC_SET16_RES_ES,
  BC_SET16_RES_SS,
  BC_SET16_RES_DS,
  BC_SET16_RES_FS,
  BC_SET16_RES_GS,
  BC_SET16_RES8_RES,
  BC_SET16_RES8_RES_SIGNED,
  BC_SET16_RES32_RES,
  BC_SET16_RES32_AX_DX,
  BC_SET8_RES_AL,
  BC_SET8_RES_CL,
  BC_SET8_RES_DL,
  BC_SET8_RES_BL,
  BC_SET8_RES_AH,
  BC_SET8_RES_CH,
  BC_SET8_RES_DH,
  BC_SET8_RES_BH,
  // --> Assignació resultat
  BC_SET32_OFFSET_EAX,
  BC_SET32_OFFSET_ECX,
  BC_SET32_OFFSET_EDX,
  BC_SET32_OFFSET_EBX,
  BC_SET32_OFFSET_ESP,
  BC_SET32_OFFSET_EBP,
  BC_SET32_OFFSET_ESI,
  BC_SET32_OFFSET_EDI,
  BC_SET32_OFFSET_RES,
  BC_SET16_OFFSET_AX,
  BC_SET16_OFFSET_CX,
  BC_SET16_OFFSET_DX,
  BC_SET16_OFFSET_BX,
  BC_SET16_OFFSET_SP,
  BC_SET16_OFFSET_BP,
  BC_SET16_OFFSET_SI,
  BC_SET16_OFFSET_DI,
  BC_SET16_OFFSET_RES,
  BC_SET16_SELECTOR_RES,
  // --> Llig en operadors
  BC_DS_READ32_RES,
  BC_DS_READ32_OP0,
  BC_DS_READ32_OP1,
  BC_SS_READ32_RES,
  BC_SS_READ32_OP0,
  BC_SS_READ32_OP1,
  BC_ES_READ32_RES,
  BC_ES_READ32_OP0,
  BC_ES_READ32_OP1,
  BC_CS_READ32_RES,
  BC_CS_READ32_OP0,
  BC_CS_READ32_OP1,
  BC_FS_READ32_RES,
  BC_FS_READ32_OP0,
  BC_FS_READ32_OP1,
  BC_GS_READ32_RES,
  BC_GS_READ32_OP0,
  BC_GS_READ32_OP1,
  BC_DS_READ16_RES,
  BC_DS_READ16_OP0,
  BC_DS_READ16_OP1,
  BC_SS_READ16_RES,
  BC_SS_READ16_OP0,
  BC_SS_READ16_OP1,
  BC_ES_READ16_RES,
  BC_ES_READ16_OP0,
  BC_ES_READ16_OP1,
  BC_CS_READ16_RES,
  BC_CS_READ16_OP0,
  BC_CS_READ16_OP1,
  BC_FS_READ16_RES,
  BC_FS_READ16_OP0,
  BC_FS_READ16_OP1,
  BC_GS_READ16_RES,
  BC_GS_READ16_OP0,
  BC_GS_READ16_OP1,
  BC_DS_READ8_RES,
  BC_DS_READ8_OP0,
  BC_DS_READ8_OP1,
  BC_SS_READ8_RES,
  BC_SS_READ8_OP0,
  BC_SS_READ8_OP1,
  BC_ES_READ8_RES,
  BC_ES_READ8_OP0,
  BC_ES_READ8_OP1,
  BC_CS_READ8_RES,
  BC_CS_READ8_OP0,
  BC_CS_READ8_OP1,
  BC_FS_READ8_RES,
  BC_FS_READ8_OP0,
  BC_FS_READ8_OP1,
  BC_GS_READ8_RES,
  BC_GS_READ8_OP0,
  BC_GS_READ8_OP1,
  BC_DS_READ_SELECTOR_OFFSET,
  BC_SS_READ_SELECTOR_OFFSET,
  BC_ES_READ_SELECTOR_OFFSET,
  BC_CS_READ_SELECTOR_OFFSET,
  BC_GS_READ_SELECTOR_OFFSET,
  BC_DS_READ_OFFSET_SELECTOR,
  BC_DS_READ_OFFSET16_SELECTOR,
  BC_SS_READ_OFFSET_SELECTOR,
  BC_SS_READ_OFFSET16_SELECTOR,
  BC_ES_READ_OFFSET_SELECTOR,
  BC_ES_READ_OFFSET16_SELECTOR,
  BC_CS_READ_OFFSET_SELECTOR,
  BC_CS_READ_OFFSET16_SELECTOR,
  BC_FS_READ_OFFSET_SELECTOR,
  BC_FS_READ_OFFSET16_SELECTOR,
  BC_GS_READ_OFFSET_SELECTOR,
  BC_GS_READ_OFFSET16_SELECTOR,
  // --> Escriu res
  BC_DS_RES_WRITE32,
  BC_SS_RES_WRITE32,
  BC_ES_RES_WRITE32,
  BC_CS_RES_WRITE32,
  BC_FS_RES_WRITE32,
  BC_GS_RES_WRITE32,
  BC_DS_RES_WRITE16,
  BC_SS_RES_WRITE16,
  BC_ES_RES_WRITE16,
  BC_CS_RES_WRITE16,
  BC_FS_RES_WRITE16,
  BC_GS_RES_WRITE16,
  BC_DS_RES_WRITE8,
  BC_SS_RES_WRITE8,
  BC_ES_RES_WRITE8,
  BC_CS_RES_WRITE8,
  BC_FS_RES_WRITE8,
  BC_GS_RES_WRITE8,
  // --> Llig ports
  BC_PORT_READ32,
  BC_PORT_READ16,
  BC_PORT_READ8,
  // --> Escriu ports
  BC_PORT_WRITE32,
  BC_PORT_WRITE16,
  BC_PORT_WRITE8,
  
  // Bytecodes operacions
  // --> Control de fluxe
  BC_JMP32_FAR,
  BC_JMP16_FAR,
  BC_JMP32_NEAR_REL,
  BC_JMP16_NEAR_REL,
  BC_JMP32_NEAR_REL32,
  BC_JMP32_NEAR_RES32,
  BC_JMP16_NEAR_RES16,
  BC_BRANCH32,
  BC_BRANCH32_IMM32,
  BC_BRANCH16,
  BC_CALL32_FAR,
  BC_CALL16_FAR,
  BC_CALL32_NEAR_REL,
  BC_CALL16_NEAR_REL,
  BC_CALL32_NEAR_REL32,
  BC_CALL32_NEAR_RES32,
  BC_CALL16_NEAR_RES16,
  BC_IRET32,
  BC_IRET16,
  // --> Aritmètiques
  BC_ADC32,
  BC_ADC16,
  BC_ADC8,
  BC_ADD32,
  BC_ADD16,
  BC_ADD8,
  BC_DIV32_EDX_EAX,
  BC_DIV16_DX_AX,
  BC_DIV8_AH_AL,
  BC_IDIV32_EDX_EAX,
  BC_IDIV16_DX_AX,
  BC_IDIV8_AH_AL,
  BC_IMUL32,
  BC_IMUL16,
  BC_IMUL8,
  BC_MUL32,
  BC_MUL16,
  BC_MUL8,
  BC_SBB32,
  BC_SBB16,
  BC_SBB8,
  BC_SGDT32,
  BC_SIDT32,
  BC_SIDT16,
  BC_SUB32,
  BC_SUB16,
  BC_SUB8,
  // --> Llògiques
  BC_AND32,
  BC_AND16,
  BC_AND8,
  BC_NOT32,
  BC_NOT16,
  BC_NOT8,
  BC_OR32,
  BC_OR16,
  BC_OR8,
  BC_RCL32,
  BC_RCL16,
  BC_RCL8,
  BC_RCR32,
  BC_RCR16,
  BC_RCR8,
  BC_ROL32,
  BC_ROL16,
  BC_ROL8,
  BC_ROR32,
  BC_ROR16,
  BC_ROR8,
  BC_SAR32,
  BC_SAR16,
  BC_SAR8,
  BC_SHL32,
  BC_SHL16,
  BC_SHL8,
  BC_SHLD32,
  BC_SHLD16,
  BC_SHR32,
  BC_SHR16,
  BC_SHR8,
  BC_SHRD32,
  BC_SHRD16,
  BC_XOR32,
  BC_XOR16,
  BC_XOR8,
  // --> Altres
  BC_AAD,
  BC_AAM,
  BC_BSF32,
  BC_BSF16,
  BC_BSR32,
  BC_BSR16,
  BC_BSWAP,
  BC_BT32,
  BC_BT16,
  BC_BTC32,
  BC_BTR32,
  BC_BTR16,
  BC_BTS32,
  BC_BTS16,
  BC_CDQ,
  BC_CLI,
  BC_CMC,
  BC_CPUID,
  BC_CWD,
  BC_CWDE,
  BC_DAA,
  BC_DAS,
  BC_ENTER16,
  BC_ENTER32,
  BC_HALT,
  BC_INT16,
  BC_INT32,
  BC_INTO16,
  BC_INTO32,
  BC_INVLPG16,
  BC_INVLPG32,
  BC_LAHF,
  BC_LAR32,
  BC_LAR16,
  BC_LEAVE32,
  BC_LEAVE16,
  BC_LGDT32,
  BC_LGDT16,
  BC_LIDT32,
  BC_LIDT16,
  BC_LLDT,
  BC_LSL32,
  BC_LSL16,
  BC_LTR,
  BC_POP32,
  BC_POP16,
  BC_POP32_EFLAGS,
  BC_POP16_EFLAGS,
  BC_PUSH32,
  BC_PUSH16,
  BC_PUSH32_EFLAGS,
  BC_PUSH16_EFLAGS,
  BC_PUSHA32_CHECK_EXCEPTION,
  BC_PUSHA16_CHECK_EXCEPTION,
  BC_RET32_FAR,
  BC_RET32_FAR_NOSTOP,
  BC_RET16_FAR,
  BC_RET16_FAR_NOSTOP,
  BC_RET32_RES,
  BC_RET32_RES_INC_ESP,
  BC_RET32_RES_INC_SP,
  BC_RET16_RES,
  BC_RET16_RES_INC_ESP,
  BC_RET16_RES_INC_SP,
  BC_INC_ESP_AND_GOTO_EIP,
  BC_INC_SP_AND_GOTO_EIP,
  BC_SAHF,
  BC_STI,
  BC_VERR,
  BC_VERW,
  BC_WBINVD,
  // --> String
  BC_CMPS32_ADDR32,
  BC_CMPS8_ADDR32,
  BC_CMPS32_ADDR16,
  BC_CMPS16_ADDR16,
  BC_CMPS8_ADDR16,
  BC_INS16_ADDR32,
  BC_INS8_ADDR32,
  BC_INS32_ADDR16,
  BC_INS16_ADDR16,
  BC_LODS32_ADDR32,
  BC_LODS16_ADDR32,
  BC_LODS32_ADDR16,
  BC_LODS16_ADDR16,
  BC_LODS8_ADDR32,
  BC_LODS8_ADDR16,
  BC_MOVS32_ADDR32,
  BC_MOVS16_ADDR32,
  BC_MOVS8_ADDR32,
  BC_MOVS32_ADDR16,
  BC_MOVS16_ADDR16,
  BC_MOVS8_ADDR16,
  BC_OUTS16_ADDR32,
  BC_OUTS8_ADDR32,
  BC_OUTS16_ADDR16,
  BC_OUTS8_ADDR16,
  BC_SCAS32_ADDR32,
  BC_SCAS16_ADDR32,
  BC_SCAS8_ADDR32,
  BC_SCAS32_ADDR16,
  BC_SCAS16_ADDR16,
  BC_SCAS8_ADDR16,
  BC_STOS32_ADDR32,
  BC_STOS32_ADDR16,
  BC_STOS16_ADDR32,
  BC_STOS16_ADDR16,
  BC_STOS8_ADDR32,
  BC_STOS8_ADDR16,
  // --> FPU WRITE
  BC_FPU_DS_FLOATU32_WRITE32,
  BC_FPU_SS_FLOATU32_WRITE32,
  BC_FPU_ES_FLOATU32_WRITE32,
  BC_FPU_DS_DOUBLEU64_WRITE,
  BC_FPU_SS_DOUBLEU64_WRITE,
  BC_FPU_DS_LDOUBLEU80_WRITE,
  BC_FPU_SS_LDOUBLEU80_WRITE,
  // --> FPU
  BC_FPU_2XM1,
  BC_FPU_ABS,
  BC_FPU_ADD,
  BC_FPU_BEGIN_OP,
  BC_FPU_BEGIN_OP_CLEAR_C1,
  BC_FPU_BEGIN_OP_NOCHECK,
  BC_FPU_BSTP_SS,
  BC_FPU_CHS,
  BC_FPU_CLEAR_EXCP,
  BC_FPU_COM,
  BC_FPU_COS,
  BC_FPU_DIV,
  BC_FPU_DIVR,
  BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK,
  BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_DENORMAL_SNAN,
  BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_FCOM,
  BC_FPU_FLOATU32_LDOUBLE_AND_CHECK,
  BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_DENORMAL_SNAN,
  BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_FCOM,
  BC_FPU_FREE,
  BC_FPU_INIT,
  BC_FPU_MUL,
  BC_FPU_PATAN,
  BC_FPU_POP,
  BC_FPU_PREM,
  BC_FPU_PTAN,
  BC_FPU_PUSH_DOUBLEU64,
  BC_FPU_PUSH_DOUBLEU64_AS_INT64,
  BC_FPU_PUSH_FLOATU32,
  BC_FPU_PUSH_L2E,
  BC_FPU_PUSH_LDOUBLEU80,
  BC_FPU_PUSH_LN2,
  BC_FPU_PUSH_ONE,
  BC_FPU_PUSH_REG2,
  BC_FPU_PUSH_RES16,
  BC_FPU_PUSH_RES32,
  BC_FPU_PUSH_ZERO,
  BC_FPU_REG_ST,
  BC_FPU_REG_INT16,
  BC_FPU_REG_INT32,
  BC_FPU_REG_INT64,
  BC_FPU_REG_FLOATU32,
  BC_FPU_REG_DOUBLEU64,
  BC_FPU_REG_LDOUBLEU80,
  BC_FPU_REG2_LDOUBLE_AND_CHECK,
  BC_FPU_REG2_LDOUBLE_AND_CHECK_FCOM,
  BC_FPU_RES16_CONTROL_WORD,
  BC_FPU_RNDINT,
  BC_FPU_RSTOR32_DS,
  BC_FPU_SAVE32_DS,
  BC_FPU_SCALE,
  BC_FPU_SELECT_ST0,
  BC_FPU_SELECT_ST1,
  BC_FPU_SELECT_ST_REG,
  BC_FPU_SELECT_ST_REG2,
  BC_FPU_SIN,
  BC_FPU_SQRT,
  BC_FPU_SUB,
  BC_FPU_SUBR,
  BC_FPU_TST,
  BC_FPU_WAIT,
  BC_FPU_XAM,
  BC_FPU_XCH,
  BC_FPU_YL2X
  
};


// Descriptor de segment.
typedef struct
{
  uint32_t addr;     // Adreça de la primera paraula.
  uint32_t w0,w1;    // Paraules.
} seg_desc_t;




/*************/
/* CONSTANTS */
/*************/

static const struct
{
  IA32_Mnemonic name;
  uint32_t      req_flags; // els necessita per funcionar bé
  uint32_t      chg_flags; // es modifiquen segur
  bool          branch;
} INSTS_METADATA[]=
  {
    {IA32_UNK,0,0,true},
    {IA32_AAA,AF_FLAG,AF_FLAG|CF_FLAG,false},
    {IA32_AAD,0,SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_AAM,0,SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_AAS,AF_FLAG,AF_FLAG|CF_FLAG,false},
    {IA32_ADC32,CF_FLAG,OSZACP_FLAGS,false},
    {IA32_ADC16,CF_FLAG,OSZACP_FLAGS,false},
    {IA32_ADC8,CF_FLAG,OSZACP_FLAGS,false},
    {IA32_ADD32,0,OSZACP_FLAGS,false},
    {IA32_ADD16,0,OSZACP_FLAGS,false},
    {IA32_ADD8,0,OSZACP_FLAGS,false},
    {IA32_AND32,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_AND16,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_AND8,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_BOUND32,0,0,false},
    {IA32_BOUND16,0,0,false},
    {IA32_BSF32,0,ZF_FLAG,false},
    {IA32_BSF16,0,ZF_FLAG,false},
    {IA32_BSR32,0,ZF_FLAG,false},
    {IA32_BSR16,0,ZF_FLAG,false},
    {IA32_BSWAP,0,0,false},
    {IA32_BT32,0,CF_FLAG,false},
    {IA32_BT16,0,CF_FLAG,false},
    {IA32_BTC32,0,CF_FLAG,false},
    {IA32_BTC16,0,CF_FLAG,false},
    {IA32_BTR32,0,CF_FLAG,false},
    {IA32_BTR16,0,CF_FLAG,false},
    {IA32_BTS32,0,CF_FLAG,false},
    {IA32_BTS16,0,CF_FLAG,false},
    {IA32_CALL32_NEAR,0,0,true},
    {IA32_CALL16_NEAR,0,0,true},
    {IA32_CALL32_FAR,0,0,true}, // Sols fiquem flags que es modifiquen segur!!
    {IA32_CALL16_FAR,0,0,true},
    {IA32_CBW,0,0,false},
    {IA32_CDQ,0,0,false},
    {IA32_CLC,0,CF_FLAG,false},
    {IA32_CLD,0,DF_FLAG,false},
    {IA32_CLI,IOPL_FLAG|VM_FLAG,0,false}, // A vegades es modifica IF
                                          // i altres VIF
    {IA32_CLTS,0,0,false},
    {IA32_CMC,CF_FLAG,CF_FLAG,false},
    {IA32_CMP32,0,OSZACP_FLAGS,false},
    {IA32_CMP16,0,OSZACP_FLAGS,false},
    {IA32_CMP8,0,OSZACP_FLAGS,false},
    {IA32_CMPS32,DF_FLAG,OSZACP_FLAGS,false},
    {IA32_CMPS16,DF_FLAG,OSZACP_FLAGS,false},
    {IA32_CMPS8,DF_FLAG,OSZACP_FLAGS,false},
    {IA32_CPUID,0,0,false},
    {IA32_CWD,0,0,false},
    {IA32_CWDE,0,0,false},
    {IA32_DAA,CF_FLAG|AF_FLAG,AF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_DAS,CF_FLAG|AF_FLAG,AF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_DEC32,0,OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG,false},
    {IA32_DEC16,0,OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG,false},
    {IA32_DEC8,0,OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG,false},
    {IA32_DIV32,0,0,false},
    {IA32_DIV16,0,0,false},
    {IA32_DIV8,0,0,false},
    {IA32_ENTER16,0,0,false},
    {IA32_ENTER32,0,0,false},
    {IA32_F2XM1,0,0,false},
    {IA32_FABS,0,0,false},
    {IA32_FADD32,0,0,false},
    {IA32_FADD64,0,0,false},
    {IA32_FADD80,0,0,false},
    {IA32_FADDP80,0,0,false},
    {IA32_FBSTP,0,0,false},
    {IA32_FCHS,0,0,false},
    {IA32_FCLEX,0,0,false},
    {IA32_FCOM32,0,0,false},
    {IA32_FCOM64,0,0,false},
    {IA32_FCOM80,0,0,false},
    {IA32_FCOMP32,0,0,false},
    {IA32_FCOMP64,0,0,false},
    {IA32_FCOMP80,0,0,false},
    {IA32_FCOMPP,0,0,false},
    {IA32_FCOS,0,0,false},
    {IA32_FDIV32,0,0,false},
    {IA32_FDIV64,0,0,false},
    {IA32_FDIV80,0,0,false},
    {IA32_FDIVP80,0,0,false},
    {IA32_FDIVR32,0,0,false},
    {IA32_FDIVR64,0,0,false},
    {IA32_FDIVR80,0,0,false},
    {IA32_FDIVRP80,0,0,false},
    {IA32_FFREE,0,0,false},
    {IA32_FILD16,0,0,false},
    {IA32_FILD32,0,0,false},
    {IA32_FILD64,0,0,false},
    {IA32_FIMUL32,0,0,false},
    {IA32_FINIT,0,0,false},
    {IA32_FIST32,0,0,false},
    {IA32_FISTP16,0,0,false},
    {IA32_FISTP32,0,0,false},
    {IA32_FISTP64,0,0,false},
    {IA32_FLD1,0,0,false},
    {IA32_FLD32,0,0,false},
    {IA32_FLD64,0,0,false},
    {IA32_FLD80,0,0,false},
    {IA32_FLDCW,0,0,false},
    {IA32_FLDL2E,0,0,false},
    {IA32_FLDLN2,0,0,false},
    {IA32_FLDZ,0,0,false},
    {IA32_FMUL32,0,0,false},
    {IA32_FMUL64,0,0,false},
    {IA32_FMUL80,0,0,false},
    {IA32_FMULP80,0,0,false},
    {IA32_FNSTSW,0,0,false},
    {IA32_FPATAN,0,0,false},
    {IA32_FPREM,0,0,false},
    {IA32_FPTAN,0,0,false},
    {IA32_FRNDINT,0,0,false},
    {IA32_FRSTOR16,0,0,false},
    {IA32_FRSTOR32,0,0,false},
    {IA32_FSAVE16,0,0,false},
    {IA32_FSAVE32,0,0,false},
    {IA32_FSCALE,0,0,false},
    {IA32_FSETPM,0,0,false},
    {IA32_FSIN,0,0,false},
    {IA32_FSQRT,0,0,false},
    {IA32_FST32,0,0,false},
    {IA32_FST64,0,0,false},
    {IA32_FST80,0,0,false},
    {IA32_FSTP32,0,0,false},
    {IA32_FSTP64,0,0,false},
    {IA32_FSTP80,0,0,false},
    {IA32_FSTCW,0,0,false},
    {IA32_FSTSW,0,0,false},
    {IA32_FSUB32,0,0,false},
    {IA32_FSUB64,0,0,false},
    {IA32_FSUB80,0,0,false},
    {IA32_FSUBP80,0,0,false},
    {IA32_FSUBR80,0,0,false},
    {IA32_FSUBR64,0,0,false},
    {IA32_FSUBR32,0,0,false},
    {IA32_FSUBRP80,0,0,false},
    {IA32_FTST,0,0,false},
    {IA32_FXAM,0,0,false},
    {IA32_FXCH,0,0,false},
    {IA32_FYL2X,0,0,false},
    {IA32_FWAIT,0,0,false},
    {IA32_HLT,0,0,true},
    {IA32_IDIV32,0,0,false},
    {IA32_IDIV16,0,0,false},
    {IA32_IDIV8,0,0,false},
    {IA32_IMUL32,0,SF_FLAG|CF_FLAG|OF_FLAG,false},
    {IA32_IMUL16,0,SF_FLAG|CF_FLAG|OF_FLAG,false},
    {IA32_IMUL8,0,SF_FLAG|CF_FLAG|OF_FLAG,false},
    {IA32_IN,IOPL_FLAG|VM_FLAG,0,false},
    {IA32_INC32,0,OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG,false},
    {IA32_INC16,0,OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG,false},
    {IA32_INC8,0,OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG,false},
    {IA32_INS32,IOPL_FLAG|VM_FLAG|DF_FLAG,0,false},
    {IA32_INS16,IOPL_FLAG|VM_FLAG|DF_FLAG,0,false},
    {IA32_INS8,IOPL_FLAG|VM_FLAG|DF_FLAG,0,false},
    {IA32_INT32,IOPL_FLAG|VM_FLAG,0,true}, // Pot ser no es modifique
                                           // res de EFLAGS
    {IA32_INT16,IOPL_FLAG|VM_FLAG,0,true},
    {IA32_INTO32,IOPL_FLAG|VM_FLAG,0,true}, // Pot ser no es modifique
                                            // res de EFLAGS
    {IA32_INTO16,IOPL_FLAG|VM_FLAG,0,true},
    {IA32_INVLPG32,0,0,false},
    {IA32_INVLPG16,0,0,false},
    {IA32_IRET32,IOPL_FLAG|VM_FLAG,0,true}, // No està clar que es
                                            // modifique sempre
    {IA32_IRET16,IOPL_FLAG|VM_FLAG,0,true},
    {IA32_JA32,CF_FLAG|ZF_FLAG,0,true},
    {IA32_JA16,CF_FLAG|ZF_FLAG,0,true},
    {IA32_JAE32,CF_FLAG,0,true},
    {IA32_JAE16,CF_FLAG,0,true},
    {IA32_JB32,CF_FLAG,0,true},
    {IA32_JB16,CF_FLAG,0,true},
    {IA32_JCXZ32,0,0,true},
    {IA32_JCXZ16,0,0,true},
    {IA32_JE32,ZF_FLAG,0,true},
    {IA32_JE16,ZF_FLAG,0,true},
    {IA32_JECXZ32,0,0,true},
    {IA32_JECXZ16,0,0,true},
    {IA32_JG32,ZF_FLAG|SF_FLAG|OF_FLAG,0,true},
    {IA32_JG16,ZF_FLAG|SF_FLAG|OF_FLAG,0,true},
    {IA32_JGE32,SF_FLAG|OF_FLAG,0,true},
    {IA32_JGE16,SF_FLAG|OF_FLAG,0,true},
    {IA32_JL32,SF_FLAG|OF_FLAG,0,true},
    {IA32_JL16,SF_FLAG|OF_FLAG,0,true},
    {IA32_JMP32_NEAR,0,0,true},
    {IA32_JMP16_NEAR,0,0,true},
    {IA32_JMP32_FAR,VM_FLAG,0,true},
    {IA32_JMP16_FAR,VM_FLAG,0,true},
    {IA32_JNA32,CF_FLAG|ZF_FLAG,0,true},
    {IA32_JNA16,CF_FLAG|ZF_FLAG,0,true},
    {IA32_JNE32,ZF_FLAG,0,true},
    {IA32_JNE16,ZF_FLAG,0,true},
    {IA32_JNG32,ZF_FLAG|SF_FLAG|OF_FLAG,0,true},
    {IA32_JNG16,ZF_FLAG|SF_FLAG|OF_FLAG,0,true},
    {IA32_JNO32,OF_FLAG,0,true},
    {IA32_JNO16,OF_FLAG,0,true},
    {IA32_JNS32,SF_FLAG,0,true},
    {IA32_JNS16,SF_FLAG,0,true},
    {IA32_JO32,OF_FLAG,0,true},
    {IA32_JO16,OF_FLAG,0,true},
    {IA32_JP32,PF_FLAG,0,true},
    {IA32_JP16,PF_FLAG,0,true},
    {IA32_JPO32,PF_FLAG,0,true},
    {IA32_JPO16,PF_FLAG,0,true},
    {IA32_JS32,SF_FLAG,0,true},
    {IA32_JS16,SF_FLAG,0,true},
    {IA32_LAHF,SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG|CF_FLAG,0,false},
    {IA32_LAR32,0,ZF_FLAG,false},
    {IA32_LAR16,0,ZF_FLAG,false},
    {IA32_LDS32,VM_FLAG,0,false},
    {IA32_LDS16,VM_FLAG,0,false},
    {IA32_LEA32,0,0,false},
    {IA32_LEA16,0,0,false},
    {IA32_LEAVE32,0,0,false},
    {IA32_LEAVE16,0,0,false},
    {IA32_LES32,VM_FLAG,0,false},
    {IA32_LES16,VM_FLAG,0,false},
    {IA32_LFS32,VM_FLAG,0,false},
    {IA32_LFS16,VM_FLAG,0,false},
    {IA32_LGDT32,0,0,false},
    {IA32_LGDT16,0,0,false},
    {IA32_LGS32,VM_FLAG,0,false},
    {IA32_LGS16,VM_FLAG,0,false},
    {IA32_LIDT32,0,0,false},
    {IA32_LIDT16,0,0,false},
    {IA32_LLDT,0,0,false},
    {IA32_LMSW,0,0,false},
    {IA32_LODS32,DF_FLAG,0,false},
    {IA32_LODS16,DF_FLAG,0,false},
    {IA32_LODS8,DF_FLAG,0,false},
    {IA32_LOOP32,0,0,true},
    {IA32_LOOP16,0,0,true},
    {IA32_LOOPE32,ZF_FLAG,0,true},
    {IA32_LOOPE16,ZF_FLAG,0,true},
    {IA32_LOOPNE32,ZF_FLAG,0,true},
    {IA32_LOOPNE16,ZF_FLAG,0,true},
    {IA32_LSL32,0,ZF_FLAG,false},
    {IA32_LSL16,0,ZF_FLAG,false},
    {IA32_LSS32,VM_FLAG,0,false},
    {IA32_LSS16,VM_FLAG,0,false},
    {IA32_LTR,0,0,false},
    {IA32_MOV32,0,0,false},
    {IA32_MOV16,0,0,false},
    {IA32_MOV8,0,0,false},
    {IA32_MOVS32,DF_FLAG,0,false},
    {IA32_MOVS16,DF_FLAG,0,false},
    {IA32_MOVS8,DF_FLAG,0,false},
    {IA32_MOVSX32W,0,0,false},
    {IA32_MOVSX32B,0,0,false},
    {IA32_MOVSX16,0,0,false},
    {IA32_MOVZX32W,0,0,false},
    {IA32_MOVZX32B,0,0,false},
    {IA32_MOVZX16,0,0,false},
    {IA32_MUL32,0,OF_FLAG|CF_FLAG,false},
    {IA32_MUL16,0,OF_FLAG|CF_FLAG,false},
    {IA32_MUL8,0,OF_FLAG|CF_FLAG,false},
    {IA32_NEG32,0,CF_FLAG,false},
    {IA32_NEG16,0,CF_FLAG,false},
    {IA32_NEG8,0,CF_FLAG,false},
    {IA32_NOP,0,0,false},
    {IA32_NOT32,0,0,false},
    {IA32_NOT16,0,0,false},
    {IA32_NOT8,0,0,false},
    {IA32_OR32,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_OR16,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_OR8,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_OUT,VM_FLAG|IOPL_FLAG,0,false},
    {IA32_OUTS32,DF_FLAG|VM_FLAG|IOPL_FLAG,0,false},
    {IA32_OUTS16,DF_FLAG|VM_FLAG|IOPL_FLAG,0,false},
    {IA32_OUTS8,DF_FLAG|VM_FLAG|IOPL_FLAG,0,false},
    {IA32_POP32,VM_FLAG,0,false},
    {IA32_POP16,VM_FLAG,0,false},
    {IA32_POPA32,VM_FLAG,0,false},
    {IA32_POPA16,VM_FLAG,0,false},
    {IA32_POPF32,VM_FLAG|IOPL_FLAG,0xFFFFFFFF,false},
    {IA32_POPF16,VM_FLAG|IOPL_FLAG,0xFFFFFFFF,false},
    {IA32_PUSH32,0,0,false},
    {IA32_PUSH16,0,0,false},
    {IA32_PUSHA32,0,0,false},
    {IA32_PUSHA16,0,0,false},
    {IA32_PUSHF32,0xFFFFFFFF,0,false},
    {IA32_PUSHF16,0xFFFFFFFF,0,false},
    {IA32_RCL32,CF_FLAG,CF_FLAG,false}, // El OF_FLAG no sempre es modifica
    {IA32_RCL16,CF_FLAG,CF_FLAG,false},
    {IA32_RCL8,CF_FLAG,CF_FLAG,false},
    {IA32_RCR32,CF_FLAG,CF_FLAG,false}, // El OF_FLAG no sempre es modifica
    {IA32_RCR16,CF_FLAG,CF_FLAG,false},
    {IA32_RCR8,CF_FLAG,CF_FLAG,false},
    {IA32_RET32_FAR,VM_FLAG,0,true},
    {IA32_RET16_FAR,VM_FLAG,0,true},
    {IA32_RET32_NEAR,0,0,true},
    {IA32_RET16_NEAR,0,0,true},
    {IA32_ROL32,0,CF_FLAG,false}, // El OF_FLAG no sempre es modifica
    {IA32_ROL16,0,CF_FLAG,false},
    {IA32_ROL8,0,CF_FLAG,false},
    {IA32_ROR32,0,CF_FLAG,false}, // El OF_FLAG no sempre es modifica
    {IA32_ROR16,0,CF_FLAG,false},
    {IA32_ROR8,0,CF_FLAG,false},
    {IA32_SAHF,0,0xFF,false},
    {IA32_SAR32,0,0,false}, // Els flags poden no modificar-se amb count 0
    {IA32_SAR16,0,0,false},
    {IA32_SAR8,0,0,false},
    {IA32_SBB32,CF_FLAG,OSZACP_FLAGS,false},
    {IA32_SBB16,CF_FLAG,OSZACP_FLAGS,false},
    {IA32_SBB8,CF_FLAG,OSZACP_FLAGS,false},
    {IA32_SCAS32,DF_FLAG,OSZACP_FLAGS,false},
    {IA32_SCAS16,DF_FLAG,OSZACP_FLAGS,false},
    {IA32_SCAS8,DF_FLAG,OSZACP_FLAGS,false},
    {IA32_SETA,CF_FLAG|ZF_FLAG,0,false},
    {IA32_SETAE,CF_FLAG,0,false},
    {IA32_SETB,CF_FLAG,0,false},
    {IA32_SETE,ZF_FLAG,0,false},
    {IA32_SETG,ZF_FLAG|SF_FLAG|OF_FLAG,0,false},
    {IA32_SETGE,SF_FLAG|OF_FLAG,0,false},
    {IA32_SETL,SF_FLAG|OF_FLAG,0,false},
    {IA32_SETNA,CF_FLAG|ZF_FLAG,0,false},
    {IA32_SETNE,ZF_FLAG,0,false},
    {IA32_SETNG,ZF_FLAG|SF_FLAG|OF_FLAG,0,false},
    {IA32_SETS,SF_FLAG,0,false},
    {IA32_SGDT32,0,0,false},
    {IA32_SGDT16,0,0,false},
    {IA32_SHL32,0,0,false}, // Els flags poden no modificar-se amb count 0
    {IA32_SHL16,0,0,false},
    {IA32_SHL8,0,0,false},
    {IA32_SHLD32,0,0,false}, // Els flags poden no modificar-se amb count 0
    {IA32_SHLD16,0,0,false},
    {IA32_SHR32,0,0,false}, // Els flags poden no modificar-se amb count 0
    {IA32_SHR16,0,0,false},
    {IA32_SHR8,0,0,false},
    {IA32_SHRD32,0,0,false}, // Els flags poden no modificar-se amb count 0
    {IA32_SHRD16,0,0,false},
    {IA32_SIDT32,0,0,false},
    {IA32_SIDT16,0,0,false},
    {IA32_SLDT,0,0,false},
    {IA32_SMSW32,0,0,false},
    {IA32_SMSW16,0,0,false},
    {IA32_STC,0,CF_FLAG,false},
    {IA32_STD,0,DF_FLAG,false},
    {IA32_STI,0,0,false}, // Pot ser IF o VIF
    {IA32_STOS32,DF_FLAG,0,false},
    {IA32_STOS16,DF_FLAG,0,false},
    {IA32_STOS8,DF_FLAG,0,false},
    {IA32_STR,0,0,false},
    {IA32_SUB32,0,OSZACP_FLAGS,false},
    {IA32_SUB16,0,OSZACP_FLAGS,false},
    {IA32_SUB8,0,OSZACP_FLAGS,false},
    {IA32_TEST32,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_TEST16,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_TEST8,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_VERR,0,ZF_FLAG,false},
    {IA32_VERW,0,ZF_FLAG,false},
    {IA32_WBINVD,0,0,false},
    {IA32_XCHG32,0,0,false},
    {IA32_XCHG16,0,0,false},
    {IA32_XCHG8,0,0,false},
    {IA32_XOR32,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_XOR16,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_XOR8,0,OF_FLAG|CF_FLAG|SF_FLAG|ZF_FLAG|PF_FLAG,false},
    {IA32_XLATB32,0,0,false},
    {IA32_XLATB16,0,0,false}
  };


static const bool PFLAG[256]=
  {
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, false, true,
    true, false, true, false, false, true,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, true, false,
    false, true, false, true, true, false,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, false, true,
    true, false, true, false, false, true,
    true, false, false, true, false, true,
    true, false, true, false, false, true,
    false, true, true, false, false, true,
    true, false, true, false, false, true,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, false, true,
    true, false, true, false, false, true,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, true, false,
    false, true, false, true, true, false,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, true, false,
    false, true, false, true, true, false,
    false, true, true, false, true, false,
    false, true, false, true, true, false,
    true, false, false, true, true, false,
    false, true, false, true, true, false,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, false, true,
    true, false, true, false, false, true,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true, true, false,
    false, true, false, true, true, false,
    true, false, false, true, false, true,
    true, false, false, true, true, false,
    true, false, false, true
  };




/*********************/
/* FUNCIONS PRIVADES */
/*********************/

static void *
malloc__ (
          size_t size
          )
{

  void *ret;


  ret= malloc ( size );
  if ( ret == NULL )
    {
      fprintf ( stderr, "cannot allocate memory" );
      exit ( EXIT_FAILURE );
    }

  return ret;
  
} // malloc__


static void *
realloc__ (
           void   *ptr,
           size_t  size
           )
{

  void *ret;


  ret= realloc ( ptr, size );
  if ( ret == NULL )
    {
      fprintf ( stderr, "cannot allocate memory" );
      exit ( EXIT_FAILURE );
    }

  return ret;
  
} // realloc__


static bool
dis_address_operand_size_is_32 (
                                void *udata
                                )
{
  
  IA32_JIT *jit;
  bool ret;

  
  jit= (IA32_JIT *) udata;
  ret= ((CR0&CR0_PE)!=0 && (!(EFLAGS&VM_FLAG)) && P_CS->h.is32);
  
  return ret;
  
} // end dis_address_operand_size_is_32


static bool
dis_mem_read (
              void           *udata,
              const uint64_t  offset,
              uint8_t        *data
              )
{

  IA32_JIT *jit;
  uint32_t addr;
  
  
  jit= (IA32_JIT *) udata;
  addr= (uint32_t) offset;
  if ( jit->_mem_read8 ( jit, P_CS, addr, data, false ) != 0 )
    {
      jit->warning ( jit->udata,
                     "s'ha produït un error de lectura en %X durant"
                     " la descodificació d'una instrucció",
                     addr );
      return false;
    }

  return true;
  
} // end dis_mem_read


static bool
dis_mem_read_trace (
                    void           *udata,
                    const uint64_t  offset,
                    uint8_t        *data
                    )
{

  IA32_JIT *jit;
  bool ret;
  
  
  jit= (IA32_JIT *) udata;
  jit->_ignore_exceptions= true;
  ret= jit->_mem_read8 ( jit, P_CS, EIP + offset, data, false ) == 0;
  jit->_ignore_exceptions= false;
  
  return ret;
  
} // end dis_mem_read_trace


/* PAGINACIÓ ******************************************************************/

#include "jit_pag.h"




/* MEMÒRIA ********************************************************************/
// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_readl8 (
            IA32_JIT       *jit,
            const uint32_t  addr,
            uint8_t        *dst,
            const bool      is_data
            )
{

  *dst= READU8 ( (uint64_t) addr, is_data );

  return 0;
  
} // end mem_readl8


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_writel8 (
             IA32_JIT       *jit,
             const uint32_t  addr,
             const uint8_t   data
             )
{

  WRITEU8 ( (uint64_t) addr, data );
  
  return 0;
  
} // end mem_writel8 


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_readl16 (
             IA32_JIT       *jit,
             const uint32_t  addr,
             uint16_t       *dst,
             const bool      is_data,
             const bool      implicit_svm
             )
{

  uint64_t laddr;


  laddr= (uint64_t) addr;
  if ( laddr&0x1 )
    *dst=
      ((uint16_t) READU8 ( laddr, is_data )) |
      (((uint16_t) READU8 ( laddr+1, is_data ))<<8);
  else *dst= READU16 ( laddr );

  return 0;
  
} // end mem_readl16


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_writel16 (
              IA32_JIT       *jit,
              const uint32_t  addr,
              const uint16_t  data
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
  
} // end mem_writel16


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_readl32 (
             IA32_JIT       *jit,
             const uint32_t  addr,
             uint32_t       *dst,
             const bool      is_data,
             const bool      implicit_svm
             )
{
  
  uint64_t laddr;
  
  
  laddr= (uint64_t) addr;
  if ( laddr&0x3 )
    {
      if ( laddr&0x1 )
        *dst=
          ((uint32_t) READU8 ( laddr, is_data )) |
          (((uint32_t) READU8 ( laddr+1, is_data ))<<8) |
          (((uint32_t) READU8 ( laddr+2, is_data ))<<16) |
          (((uint32_t) READU8 ( laddr+3, is_data ))<<24);
      else
        *dst=
          ((uint32_t) READU16 ( laddr )) |
          (((uint32_t) READU16 ( laddr+2 ))<<16);
    }
  else *dst= READU32 ( laddr );

  return 0;
  
} // end mem_readl32


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_writel32 (
              IA32_JIT       *jit,
              const uint32_t  addr,
              const uint32_t  data
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
  
} // end mem_writel32


static uint64_t
readu64 (
         IA32_JIT       *jit,
         const uint64_t  addr,
         const bool      is_data
         )
{
  
  uint64_t ret;
  
  
  if ( addr&0x7 )
    {
      if ( addr&0x3 )
        {
          if ( addr&0x1 )
            ret=
              ((uint64_t) READU8 ( addr, is_data )) |
              (((uint64_t) READU8 ( addr+1, is_data ))<<8) |
              (((uint64_t) READU8 ( addr+2, is_data ))<<16) |
              (((uint64_t) READU8 ( addr+3, is_data ))<<24) |
              (((uint64_t) READU8 ( addr+4, is_data ))<<32) |
              (((uint64_t) READU8 ( addr+5, is_data ))<<40) |
              (((uint64_t) READU8 ( addr+6, is_data ))<<48) |
              (((uint64_t) READU8 ( addr+7, is_data ))<<56);
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
  
} // end readu64


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_readl64 (
             IA32_JIT       *jit,
             const uint32_t  addr,
             uint64_t       *dst,
             const bool      is_data
             )
{

  *dst= readu64 ( jit, (uint64_t) addr, is_data );
  
  return 0;
  
} // end mem_readl64


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_readl128 (
              IA32_JIT            *jit,
              const uint32_t       addr,
              IA32_DoubleQuadword  dst,
              const bool           is_data
              )
{

#ifdef IA32_LE
  dst[0]= readu64 ( jit, (uint64_t) addr, is_data );
  dst[1]= readu64 ( jit, ((uint64_t) addr) + (uint64_t) 8, is_data );
#else
  dst[1]= readu64 ( jit, (uint64_t) addr, is_data );
  dst[0]= readu64 ( jit, ((uint64_t) addr) + (uint64_t) 8, is_data );
#endif
  
  return 0;
  
} // end mem_readl128


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read8 (
               IA32_JIT       *jit,
               const uint32_t  addr,
               uint8_t        *dst,
               const bool      reading_data
               )
{

  uint64_t laddr;


  laddr= 0;
  if ( !paging_32b_translate ( jit, addr, &laddr, false,
                               !reading_data, false ) )
    return -1;
  *dst= READU8 ( laddr, reading_data );
  
  return 0;
  
} // end mem_p32_read8


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_write8 (
                IA32_JIT       *jit,
                const uint32_t  addr,
                const uint8_t   data
                )
{

  uint64_t laddr;


  laddr= 0;
  if ( !paging_32b_translate ( jit, addr, &laddr, true, false, false ) )
    return -1;
  WRITEU8 ( laddr, data );
  paging_32b_addr_changed ( jit, laddr );
  
  return 0;
  
} // end mem_p32_write8


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read16 (
                IA32_JIT       *jit,
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
      if ( !paging_32b_translate ( jit, addr, &laddr, false,
                                   !reading_data, implicit_svm ) )
        return -1;
      ret= mem_readl16 ( jit, laddr, dst, reading_data, implicit_svm );
    }
  else
    {
      
      // Llig valors
      if ( mem_p32_read8 ( jit, addr, &tmp, reading_data ) != 0 )
        return -1;
      data= (uint16_t) tmp;
      if ( mem_p32_read8 ( jit, addr+1, &tmp, reading_data ) != 0 )
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
                 IA32_JIT       *jit,
                 const uint32_t  addr,
                 const uint16_t  data
                 )
{

  uint64_t laddr;
  int ret;
  
  
  if ( (addr&0x1) == 0 )
    {
      laddr= 0;
      if ( !paging_32b_translate ( jit, addr, &laddr, true, false, false ) )
        return -1;
      ret= mem_writel16 ( jit, laddr, data );
      paging_32b_addr_changed ( jit, laddr );
    }
  else
    {
      if ( mem_p32_write8 ( jit, addr, (uint8_t) data ) != 0 )
        return -1;
      if ( mem_p32_write8 ( jit, addr+1, (uint8_t) (data>>8) ) != 0 )
        return -1;
      ret= 0;
    }
  
  return ret;
  
} // end mem_p32_write16


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read32 (
                IA32_JIT       *jit,
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
      if ( !paging_32b_translate ( jit, addr, &laddr, false,
                                   !reading_data, implicit_svm ) )
        return -1;
      ret= mem_readl32 ( jit, laddr, dst, reading_data, implicit_svm );
    }
  else
    {

      // Llig
      if ( mem_p32_read16 ( jit, addr, &tmp,
                            reading_data, implicit_svm ) != 0 )
        return -1;
      data= (uint32_t) tmp;
      if ( mem_p32_read16 ( jit, addr+2, &tmp,
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
                 IA32_JIT       *jit,
                 const uint32_t  addr,
                 const uint32_t  data
                 )
{

  uint64_t laddr;
  int ret;
  

  if ( (addr&0x3) == 0 )
    {
      laddr= 0;
      if ( !paging_32b_translate ( jit, addr, &laddr, true, false, false ) )
        return -1;
      ret= mem_writel32 ( jit, laddr, data );
      paging_32b_addr_changed ( jit, laddr );
    }
  else
    {
      if ( mem_p32_write16 ( jit, addr, (uint16_t) data ) != 0 )
        return -1;
      if ( mem_p32_write16 ( jit, addr+2, (uint16_t) (data>>16) ) != 0 )
        return -1;
      ret= 0;
    }
  
  return ret;
  
} // end mem_p32_write32


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read64 (
                IA32_JIT       *jit,
                const uint32_t  addr,
                uint64_t       *dst,
                const bool      reading_data
                )
{

  uint64_t laddr;


  laddr= 0;
  if ( !paging_32b_translate ( jit, addr, &laddr, false,
                               !reading_data, false ) )
    return -1;
  return mem_readl64 ( jit, laddr, dst, reading_data );
  
} // end mem_p32_read64


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_p32_read128 (
                 IA32_JIT            *jit,
                 const uint32_t       addr,
                 IA32_DoubleQuadword  dst,
                 const bool           reading_data
                 )
{

  uint64_t laddr;


  laddr= 0;
  if ( !paging_32b_translate ( jit, addr, &laddr, false,
                               !reading_data, false ) )
    return -1;
  return mem_readl128 ( jit, laddr, dst, reading_data );
  
} // end mem_p32_read128


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read8_protected (
                     IA32_JIT             *jit,
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
  READLB ( addr, dst, return -1, reading_data );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read8_protected


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_write8_protected (
                      IA32_JIT             *jit,
                      IA32_SegmentRegister *seg,
                      const uint32_t        offset,
                      const uint8_t         data
                      )
{

  uint32_t addr;
  
  
  // Null segment selector and type checking.
  if ( seg->h.isnull || !seg->h.writable )
    { EXCEPTION0 ( EXCP_GP ); return -1; }
  
  // Limit checking.
  if ( offset < seg->h.lim.firstb || offset > seg->h.lim.lastb )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  WRITELB ( addr, data, return -1 );
  
  return 0;
  
} // end mem_write8_protected


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read16_protected (
                      IA32_JIT             *jit,
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
  READLW ( addr, dst, return -1, reading_data, false );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read16_protected


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_write16_protected (
                       IA32_JIT             *jit,
                       IA32_SegmentRegister *seg,
                       const uint32_t        offset,
                       const uint16_t        data
                       )
{

  uint32_t addr;
  
  
  // Null segment selector and type checking.
  if ( seg->h.isnull || !seg->h.writable ) goto excp_gp;
  
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
  WRITELW ( addr, data, return -1 );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_write16_protected


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read32_protected (
                      IA32_JIT             *jit,
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
  READLD ( addr, dst, return -1, reading_data, false );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read32_protected


static int
mem_write32_protected (
                       IA32_JIT             *jit,
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
  
} // end mem_write32_protected


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read64_protected (
                      IA32_JIT             *jit,
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
  READLQ ( addr, dst, return -1, true );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read64_protected



// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read128_protected (
                       IA32_JIT             *jit,
                       IA32_SegmentRegister *seg,
                       const uint32_t        offset,
                       IA32_DoubleQuadword   dst,
                       const bool            isSSE
                       )
{

  uint32_t addr;
  
  
  // Null segment selector and type checking.
  if ( seg->h.isnull || !seg->h.readable ) goto excp_gp;
  
  // Alignment checking.
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
  
  // Limit checking.
  if ( offset < seg->h.lim.firstb || offset > (seg->h.lim.lastb-15) )
    {
      if ( seg == P_SS ) { EXCEPTION0 ( EXCP_SS ); }
      else               { EXCEPTION0 ( EXCP_GP ); }
      return -1;
    }
  
  // Tradueix i llig.
  addr= seg->h.lim.addr + offset;
  READLDQ ( addr, dst, return -1, true );
  
  return 0;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return -1;
  
} // end mem_read128_protected


static int
mem_read8_real (
                IA32_JIT             *jit,
                IA32_SegmentRegister *seg,
                const uint32_t        offset,
                uint8_t              *dst,
                const bool            reading_data
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
  READLB ( addr, dst, return -1, reading_data );
  
  return 0;
  
} // end mem_read8_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_write8_real (
                 IA32_JIT             *jit,
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
  //addr= (((uint32_t) seg->v)<<4) + offset;
  WRITELB ( addr, data, return -1 );
  
  return 0;
  
} // end mem_write8_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read16_real (
                 IA32_JIT             *jit,
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
  READLW ( addr, dst, return -1, reading_data, false );
  
  return 0;
  
} // end mem_read16_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_write16_real (
                  IA32_JIT             *jit,
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
                 IA32_JIT             *jit,
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
  //addr= (((uint32_t) seg->v)<<4) + offset;
  READLD ( addr, dst, return -1, reading_data, false );
  
  return 0;
  
} // end mem_read32_real


static int
mem_write32_real (
                  IA32_JIT             *jit,
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
  //addr= (((uint32_t) seg->v)<<4) + offset;
  WRITELD ( addr, data, return -1 );
  
  return 0;
  
} // end mem_write32_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read64_real (
                 IA32_JIT             *jit,
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
  READLQ ( addr, dst, return -1, true );
  
  return 0;
  
} // end mem_read64_real


// Torna 0 si tot ha anat bé, -1 en cas d'excepció.
static int
mem_read128_real (
                  IA32_JIT             *jit,
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
  READLDQ ( addr, dst, return -1, true );
  
  return 0;
  
} // end mem_read128_real


static void
update_mem_callbacks (
                      IA32_JIT *jit
                      )
{

  bool pag32_enabled;

  
  // NOTA!!! En aquest punt podria ser que la part pública de jit
  // no estiga inicialitzada.
  //
  // FALTA VIRTUAL MODE !!!!
  pag32_enabled= jit->_mem_readl8==mem_p32_read8;
  if ( jit->_cpu != NULL && (CR0&CR0_PE)!=0 ) // Protected mode.
    {
      
      // Memòria.
      jit->_mem_read8= mem_read8_protected;
      jit->_mem_read16= mem_read16_protected;
      jit->_mem_read32= mem_read32_protected;
      jit->_mem_read64= mem_read64_protected;
      jit->_mem_read128= mem_read128_protected;
      jit->_mem_write8= mem_write8_protected;
      jit->_mem_write16= mem_write16_protected;
      jit->_mem_write32= mem_write32_protected;

      // Memòria linial.
      if ( CR0&CR0_PG )
        {
          if ( (CR4&CR4_PAE) == 0 )
            {
              // Reseteja si no està ja activat.
              if ( !pag32_enabled ) paging_32b_CR3_changed ( jit );
              jit->_mem_readl8= mem_p32_read8;
              jit->_mem_readl16= mem_p32_read16;
              jit->_mem_readl32= mem_p32_read32;
              jit->_mem_readl64= mem_p32_read64;
              jit->_mem_readl128= mem_p32_read128;
              jit->_mem_writel8= mem_p32_write8;
              jit->_mem_writel16= mem_p32_write16;
              jit->_mem_writel32= mem_p32_write32;
            }
          else
            {
              fprintf ( stderr, "[JIT] [CAL_IMPLEMENTAR] Paginació PAE=1!!!\n");
              exit ( EXIT_FAILURE );
            }
        }
      else
        {
          jit->_mem_readl8= mem_readl8;
          jit->_mem_readl16= mem_readl16;
          jit->_mem_readl32= mem_readl32;
          jit->_mem_readl64= mem_readl64;
          jit->_mem_readl128= mem_readl128;
          jit->_mem_writel8= mem_writel8;
          jit->_mem_writel16= mem_writel16;
          jit->_mem_writel32= mem_writel32;
        }
      
    }
  else // Real address mode.
    {

      // Memòria.
      jit->_mem_read8= mem_read8_real;
      jit->_mem_read16= mem_read16_real;
      jit->_mem_read32= mem_read32_real;
      jit->_mem_read64= mem_read64_real;
      jit->_mem_read128= mem_read128_real;
      jit->_mem_write8= mem_write8_real;
      jit->_mem_write16= mem_write16_real;
      jit->_mem_write32= mem_write32_real;
      
      // Memòria linial.
      jit->_mem_readl8= mem_readl8;
      jit->_mem_readl16= mem_readl16;
      jit->_mem_readl32= mem_readl32;
      jit->_mem_readl64= mem_readl64;
      jit->_mem_readl128= mem_readl128;
      jit->_mem_writel8= mem_writel8;
      jit->_mem_writel16= mem_writel16;
      jit->_mem_writel32= mem_writel32;
      
    }

} // end update_mem_callbacks


/* GESTIÓ PÀGINES *************************************************************/
static void
print_page (
            FILE                *f,
            const IA32_JIT_Page *p
            )
{

  uint32_t e,ind,n;
  bool print_dots;


  if ( p->overlap_next_page > 0 )
    fprintf ( f, "OVERLAP: %d\n", p->overlap_next_page );
  
  // Mapa
  fprintf ( f, "MAP:\n" );
  print_dots= true;
  for ( e= p->first_entry; e <= p->last_entry; ++e )
    {
      ind= p->entries[e].ind;
      if ( ind == NULL_ENTRY )
        {
          if ( print_dots ) { fprintf ( f, "   ...\n"); print_dots= false; }
        }
      else
        {
          print_dots= true;
          if ( ind != PAD_ENTRY )
            fprintf ( f, "  %08X  %u    (%s)\n",
                      e, ind-2, p->entries[e].is32 ? "32bit" : "16bit" );
        }
    }

  // Codi
  fprintf ( f, "CODE:\n" );
  for ( n= 2; n < p->N; ++n )
    {
      fprintf ( f, "  %010d: ", n-2 );
      switch ( p->v[n] )
        {
        case BC_GOTO_EIP: fprintf ( f, "goto EIP // Stop!\n" ); break;
        case BC_INC1_EIP: fprintf ( f, "++EIP // Stop!\n" ); break;
        case BC_INC2_EIP: fprintf ( f, "EIP+= 2 // Stop!\n" ); break;
        case BC_INC3_EIP: fprintf ( f, "EIP+= 3 // Stop!\n" ); break;
        case BC_INC4_EIP: fprintf ( f, "EIP+= 4 // Stop!\n" ); break;
        case BC_INC5_EIP: fprintf ( f, "EIP+= 5 // Stop!\n" ); break;
        case BC_INC6_EIP: fprintf ( f, "EIP+= 6 // Stop!\n" ); break;
        case BC_INC7_EIP: fprintf ( f, "EIP+= 7 // Stop!\n" ); break;
        case BC_INC8_EIP: fprintf ( f, "EIP+= 8 // Stop!\n" ); break;
        case BC_INC9_EIP: fprintf ( f, "EIP+= 9 // Stop!\n" ); break;
        case BC_INC10_EIP: fprintf ( f, "EIP+= 10 // Stop!\n" ); break;
        case BC_INC11_EIP: fprintf ( f, "EIP+= 11 // Stop!\n" ); break;
        case BC_INC12_EIP: fprintf ( f, "EIP+= 12 // Stop!\n" ); break;
        case BC_INC14_EIP: fprintf ( f, "EIP+= 14 // Stop!\n" ); break;
        case BC_INC1_EIP_NOSTOP: fprintf ( f, "++EIP\n" ); break;
        case BC_INC2_EIP_NOSTOP: fprintf ( f, "EIP+= 2\n" ); break;
        case BC_INC3_EIP_NOSTOP: fprintf ( f, "EIP+= 3\n" ); break;
        case BC_INC4_EIP_NOSTOP: fprintf ( f, "EIP+= 4\n" ); break;
        case BC_INC5_EIP_NOSTOP: fprintf ( f, "EIP+= 5\n" ); break;
        case BC_INC6_EIP_NOSTOP: fprintf ( f, "EIP+= 6\n" ); break;
        case BC_INC7_EIP_NOSTOP: fprintf ( f, "EIP+= 7\n" ); break;
        case BC_INC8_EIP_NOSTOP: fprintf ( f, "EIP+= 8\n" ); break;
        case BC_INC9_EIP_NOSTOP: fprintf ( f, "EIP+= 9\n" ); break;
        case BC_INC10_EIP_NOSTOP: fprintf ( f, "EIP+= 10\n" ); break;
        case BC_INC11_EIP_NOSTOP: fprintf ( f, "EIP+= 11\n" ); break;
        case BC_INC12_EIP_NOSTOP: fprintf ( f, "EIP+= 12\n" ); break;
        case BC_INC14_EIP_NOSTOP: fprintf ( f, "EIP+= 14\n" ); break;
          /*
        case BC_INC_IMM_EIP_AND_GOTO_EIP:
          fprintf ( f, "EIP+= %d; goto EIP // Stop!\n", p->v[n+1] );
          n+= 4;
          break;
          */
        case BC_WRONG_INST:
          fprintf ( f, "wrong inst (exception: %d) // Stop!\n", p->v[n+1] );
          n+= 3;
          break;
        case BC_INC2_PC_IF_ECX_IS_0:
          fprintf ( f, "if(ECX==0) {BC_PC+= 2}\n" );
          break;
        case BC_INC2_PC_IF_CX_IS_0:
          fprintf ( f, "if(CX==0) {BC_PC+= 2}\n" );
          break;
        case BC_INC4_PC_IF_ECX_IS_0:
          fprintf ( f, "if(ECX==0) {BC_PC+= 4}\n" );
          break;
        case BC_INC4_PC_IF_CX_IS_0:
          fprintf ( f, "if(CX==0) {BC_PC+= 4}\n" );
          break;
        case BC_INCIMM_PC_IF_CX_IS_0:
          fprintf ( f, "if(CX==0) {BC_PC+= %d}\n", p->v[n+1] );
          ++n;
          break;
        case BC_INCIMM_PC_IF_ECX_IS_0:
          fprintf ( f, "if(ECX==0) {BC_PC+= %d}\n", p->v[n+1] );
          ++n;
          break;
        case BC_DEC1_PC_IF_REP32:
          fprintf ( f, "if(--ECX!=0) {BC_EIP-= 1; STOP}\n" );
          break;
        case BC_DEC1_PC_IF_REP16:
          fprintf ( f, "if(--CX!=0) {BC_EIP-= 1; STOP}\n" );
          break;
        case BC_DEC3_PC_IF_REP32:
          fprintf ( f, "if(--ECX!=0) {BC_EIP-= 3; STOP}\n" );
          break;
        case BC_DEC3_PC_IF_REP16:
          fprintf ( f, "if(--CX!=0) {BC_EIP-= 3; STOP}\n" );
          break;
        case BC_DECIMM_PC_IF_REPE32:
          fprintf ( f, "if(--ECX!=0 && ZF!=0) {BC_EIP-= %d; STOP}\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_DECIMM_PC_IF_REPNE32:
          fprintf ( f, "if(--ECX!=0 && ZF==0) {BC_EIP-= %d; STOP}\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_DECIMM_PC_IF_REPE16:
          fprintf ( f, "if(--CX!=0 && ZF!=0) {BC_EIP-= %d; STOP}\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_DECIMM_PC_IF_REPNE16:
          fprintf ( f, "if(--CX!=0 && ZF==0) {BC_EIP-= %d; STOP}\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_UNK:
          fprintf ( f, "unknown_inst(%X) // Stop !!\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET32_IMM_SELECTOR_OFFSET:
          fprintf ( f, "selector= %04Xh; offset= %08Xh\n",
                    p->v[n+1], ((uint32_t) p->v[n+2]) |
                    (((uint32_t) (p->v[n+3]))<<16) );
          n+= 3;
          break;
        case BC_SET32_IMM_OFFSET:
          fprintf ( f, "offset= %08Xh\n",
                    ((uint32_t) p->v[n+1]) | (((uint32_t) p->v[n+2])<<16) );
          n+= 2;
          break;
        case BC_SET16_IMM_SELECTOR_OFFSET:
          fprintf ( f, "selector= %04Xh; offset= %04Xh\n",
                    p->v[n+1], p->v[n+2] );
          n+= 2;
          break;
        case BC_SET16_IMM_OFFSET:
          fprintf ( f, "offset= %04Xh\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET16_IMM_PORT:
          fprintf ( f, "port= %04Xh\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET32_EAX_OFFSET: fprintf ( f, "offset= EAX\n" ); break;
        case BC_SET32_ECX_OFFSET: fprintf ( f, "offset= ECX\n" ); break;
        case BC_SET32_EDX_OFFSET: fprintf ( f, "offset= EDX\n" ); break;
        case BC_SET32_EBX_OFFSET: fprintf ( f, "offset= EBX\n" ); break;
        case BC_SET32_EBX_AL_OFFSET: fprintf ( f, "offset= EBX+AL\n" ); break;
        case BC_SET32_EBP_OFFSET: fprintf ( f, "offset= EBP\n" ); break;
        case BC_SET32_ESI_OFFSET: fprintf ( f, "offset= ESI\n" ); break;
        case BC_SET32_EDI_OFFSET: fprintf ( f, "offset= EDI\n" ); break;
        case BC_SET32_ESP_OFFSET: fprintf ( f, "offset= ESP\n" ); break;
        case BC_SET16_SI_OFFSET: fprintf ( f, "offset= SI\n" ); break;
        case BC_SET16_DI_OFFSET: fprintf ( f, "offset= DI\n" ); break;
        case BC_SET16_BP_OFFSET: fprintf ( f, "offset= BP\n" ); break;
        case BC_SET16_BP_SI_OFFSET:
          fprintf ( f, "offset= (BP+SI)&FFFFh\n" );
          break;
        case BC_SET16_BP_DI_OFFSET:
          fprintf ( f, "offset= (BP+DI)&FFFFh\n" );
          break;
        case BC_SET16_BX_OFFSET: fprintf ( f, "offset= BX\n" ); break;
        case BC_SET16_BX_AL_OFFSET: fprintf ( f, "offset= BX+AL\n" ); break;
        case BC_SET16_BX_SI_OFFSET:
          fprintf ( f, "offset= (BX+SI)&FFFFh\n" );
          break;
        case BC_SET16_BX_DI_OFFSET:
          fprintf ( f, "offset= (BX+DI)&FFFFh\n" );
          break;
        case BC_SET16_DX_PORT: fprintf ( f, "port= DX\n" ); break;
        case BC_ADD32_IMM_OFFSET:
          fprintf ( f, "offset+= %08Xh\n",
                    ((uint32_t) p->v[n+1]) | (((uint32_t) p->v[n+2])<<16) );
          n+= 2;
          break;
        case BC_ADD32_BITOFFSETOP1_OFFSET:
          fprintf ( f, "offset+= 4*sign_extend(op32[1]>>5)\n" );
          break;
        case BC_ADD32_EAX_OFFSET: fprintf ( f, "offset+= EAX\n" ); break;
        case BC_ADD32_ECX_OFFSET: fprintf ( f, "offset+= ECX\n" ); break;
        case BC_ADD32_EDX_OFFSET: fprintf ( f, "offset+= EDX\n" ); break;
        case BC_ADD32_EBX_OFFSET: fprintf ( f, "offset+= EBX\n" ); break;
        case BC_ADD32_EBP_OFFSET: fprintf ( f, "offset+= EBP\n" ); break;
        case BC_ADD32_ESI_OFFSET: fprintf ( f, "offset+= ESI\n" ); break;
        case BC_ADD32_EDI_OFFSET: fprintf ( f, "offset+= EDI\n" ); break;
        case BC_ADD32_EAX_2_OFFSET: fprintf ( f, "offset+= EAX*2\n" ); break;
        case BC_ADD32_ECX_2_OFFSET: fprintf ( f, "offset+= ECX*2\n" ); break;
        case BC_ADD32_EDX_2_OFFSET: fprintf ( f, "offset+= EDX*2\n" ); break;
        case BC_ADD32_EBX_2_OFFSET: fprintf ( f, "offset+= EBX*2\n" ); break;
        case BC_ADD32_EBP_2_OFFSET: fprintf ( f, "offset+= EBP*2\n" ); break;
        case BC_ADD32_ESI_2_OFFSET: fprintf ( f, "offset+= ESI*2\n" ); break;
        case BC_ADD32_EDI_2_OFFSET: fprintf ( f, "offset+= EDI*2\n" ); break;
        case BC_ADD32_EAX_4_OFFSET: fprintf ( f, "offset+= EAX*4\n" ); break;
        case BC_ADD32_ECX_4_OFFSET: fprintf ( f, "offset+= ECX*4\n" ); break;
        case BC_ADD32_EDX_4_OFFSET: fprintf ( f, "offset+= EDX*4\n" ); break;
        case BC_ADD32_EBX_4_OFFSET: fprintf ( f, "offset+= EBX*4\n" ); break;
        case BC_ADD32_EBP_4_OFFSET: fprintf ( f, "offset+= EBP*4\n" ); break;
        case BC_ADD32_ESI_4_OFFSET: fprintf ( f, "offset+= ESI*4\n" ); break;
        case BC_ADD32_EDI_4_OFFSET: fprintf ( f, "offset+= EDI*4\n" ); break;
        case BC_ADD32_EAX_8_OFFSET: fprintf ( f, "offset+= EAX*8\n" ); break;
        case BC_ADD32_ECX_8_OFFSET: fprintf ( f, "offset+= ECX*8\n" ); break;
        case BC_ADD32_EDX_8_OFFSET: fprintf ( f, "offset+= EDX*8\n" ); break;
        case BC_ADD32_EBX_8_OFFSET: fprintf ( f, "offset+= EBX*8\n" ); break;
        case BC_ADD32_EBP_8_OFFSET: fprintf ( f, "offset+= EBP*8\n" ); break;
        case BC_ADD32_ESI_8_OFFSET: fprintf ( f, "offset+= ESI*8\n" ); break;
        case BC_ADD32_EDI_8_OFFSET: fprintf ( f, "offset+= EDI*8\n" ); break;
        case BC_ADD16_IMM_OFFSET:
          fprintf ( f, "offset+= %d\n", (int32_t) ((int16_t) p->v[n+1]) );
          ++n;
          break;
        case BC_ADD16_IMM_OFFSET16:
          fprintf ( f, "offset= (offset+%d)&FFFFh\n",
                    (int32_t) ((int16_t) p->v[n+1]) );
          ++n;
          break;
        case BC_SET_A_COND: fprintf ( f, "cond= (CF==0 && ZF==0)\n" ); break;
        case BC_SET_AE_COND: fprintf ( f, "cond= (CF==0)\n" ); break;
        case BC_SET_B_COND: fprintf ( f, "cond= (CF==1)\n" ); break;
        case BC_SET_CXZ_COND: fprintf ( f, "cond= (CX==0)\n" ); break;
        case BC_SET_E_COND: fprintf ( f, "cond= (ZF!=0)\n" ); break;
        case BC_SET_ECXZ_COND: fprintf ( f, "cond= (ECX==0)\n" ); break;
        case BC_SET_G_COND: fprintf ( f, "cond= (ZF==0 && SF==OF)\n" ); break;
        case BC_SET_GE_COND: fprintf ( f, "cond= (SF==OF)\n" ); break;
        case BC_SET_L_COND: fprintf ( f, "cond= (SF!=OF)\n" ); break;
        case BC_SET_NA_COND: fprintf ( f, "cond= (CF==1 || ZF==1)\n" ); break;
        case BC_SET_NE_COND: fprintf ( f, "cond= (ZF==0)\n" ); break;
        case BC_SET_NG_COND: fprintf ( f, "cond= (ZF==1 || SF!=OF)\n" ); break;
        case BC_SET_NO_COND: fprintf ( f, "cond= (OF==0)\n" ); break;
        case BC_SET_NS_COND: fprintf ( f, "cond= (SF==0)\n" ); break;
        case BC_SET_O_COND: fprintf ( f, "cond= (OF==1)\n" ); break;
        case BC_SET_P_COND: fprintf ( f, "cond= (PF==1)\n" ); break;
        case BC_SET_PO_COND: fprintf ( f, "cond= (PF==0)\n" ); break;
        case BC_SET_S_COND: fprintf ( f, "cond= (SF==1)\n" ); break;
        case BC_SET_DEC_ECX_NOT_ZERO_COND:
          fprintf ( f, "cond= (--ECX!=0)\n" );
          break;
        case BC_SET_DEC_ECX_NOT_ZERO_AND_ZF_COND:
          fprintf ( f, "cond= (--ECX!=0)&&ZF\n" );
          break;
        case BC_SET_DEC_ECX_NOT_ZERO_AND_NOT_ZF_COND:
          fprintf ( f, "cond= (--ECX!=0)&&!ZF\n" );
          break;
        case BC_SET_DEC_CX_NOT_ZERO_COND:
          fprintf ( f, "cond= (--CX!=0)\n" );
          break;
        case BC_SET_DEC_CX_NOT_ZERO_AND_ZF_COND:
          fprintf ( f, "cond= (--CX!=0)&&ZF\n" );
          break;
        case BC_SET_DEC_CX_NOT_ZERO_AND_NOT_ZF_COND:
          fprintf ( f, "cond= (--CX!=0)&&!ZF\n" );
          break;
        case BC_SET_1_COUNT: fprintf ( f, "count= 1\n" ); break;
        case BC_SET_CL_COUNT: fprintf ( f, "count= CL&1Fh\n" ); break;
        case BC_SET_CL_COUNT_NOMOD: fprintf ( f, "count= CL\n" ); break;
        case BC_SET_IMM_COUNT:
          fprintf ( f, "count= %04Xh\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET32_AD_OF_EFLAGS:
          fprintf ( f, "OF <-- check_overflow_add(res32)\n" );
          break;
        case BC_SET32_AD_AF_EFLAGS:
          fprintf ( f, "AF <-- check_aux_carry(res32)\n" );
          break;
        case BC_SET32_AD_CF_EFLAGS:
          fprintf ( f, "CF <-- check_carry(res32)\n" );
          break;
        case BC_SET32_MUL_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- ((res32&80000000h)!=0)^"
                    "((res64&8000000000000000h)!=0)\n" );
          break;
        case BC_SET32_MUL1_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- EDX!=0\n" );
          break;
        case BC_SET32_IMUL1_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- sign_extend(EAX)!=res64\n" );
          break;
        case BC_SET32_NEG_CF_EFLAGS:
          fprintf ( f, "CF <-- op32[1]!=0\n" );
          break;
        case BC_SET32_RCL_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- (((op32[0]&80000000h)!=0)^"
                    "((op32[0]&40000000h)!=0))}\n" );
          break;
        case BC_SET32_RCL_CF_EFLAGS:
          fprintf ( f,"if(count>0) {CF <-- ((op32[0]>>(32-count))&0x1)!=0}\n" );
          break;
        case BC_SET32_RCR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- ((op32[0]&80000000h)^"
                    "(CF!=0))}\n" );
          break;
        case BC_SET32_RCR_CF_EFLAGS:
          fprintf ( f, "if(count>0) {CF <-- ((op32[0]>>(count-1))&0x1)!=0}\n" );
          break;
        case BC_SET32_ROL_CF_EFLAGS:
          fprintf ( f, "CF <-- (res32&0x1)!=0\n" );
          break;
        case BC_SET32_ROR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- ((op32[0]&80000000h)^"
                    "((op32[0]&01h)<<31))!=0}\n" );
          break;
        case BC_SET32_ROR_CF_EFLAGS:
          fprintf ( f, "CF <-- (res32&80000000h)!=0\n" );
          break;
        case BC_SET32_SAR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- 0}\n" );
          break;
        case BC_SET32_SAR_CF_EFLAGS:
          fprintf ( f, "if(count==32) {CF <-- op32[0]&80000000h} else"
                    " if(count>0) {CF <-- (op32[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET32_SHIFT_SF_EFLAGS:
          fprintf ( f, "if(count>0 && count<=32) {SF <-- is_signed(res32)}\n" );
          break;
        case BC_SET32_SHIFT_ZF_EFLAGS:
          fprintf ( f, "if(count>0 && count<=32) {ZF <-- res32==0}\n");
          break;
        case BC_SET32_SHIFT_PF_EFLAGS:
          fprintf ( f, "if(count>0 && count<=32) {"
                    "PF <-- check_parity(res32&FFh)}\n" );
          break;
        case BC_SET32_SHL_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- "
                    "((op32[0]&80000000h)^((op32[0]&40000000h)<<1))!=0}\n" );
          break;
        case BC_SET32_SHL_CF_EFLAGS:
          fprintf ( f, "if(count>0) "
                    "{CF <-- (op32[0]&(80000000h>>(count-1))!=0}\n" );
          break;
        case BC_SET32_SHLD_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- "
                    "((res32^op32[0])&80000000h) }\n" );
          break;
        case BC_SET32_SHLD_CF_EFLAGS:
          fprintf ( f, "if(count>0 && count<=32) "
                    "{CF <-- (op32[0]&(1<<(32-count)))!=0}\n" );
          break;
        case BC_SET32_SHR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- (op32[0]&80000000h)!=0}\n" );
          break;
        case BC_SET32_SHR_CF_EFLAGS:
          fprintf ( f, "if(count>0) {CF <-- (op32[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET32_SHRD_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- "
                    "((res32^op32[0])&80000000h)!=0}\n" );
          break;
        case BC_SET32_SHRD_CF_EFLAGS:
          fprintf ( f, "if(count>0 && count<=32) {CF <-- "
                    "(op32[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET32_SB_OF_EFLAGS:
          fprintf ( f, "OF <-- check_overflow_sub(res32)\n" );
          break;
        case BC_SET32_SB_AF_EFLAGS:
          fprintf ( f, "AF <-- check_aux_borrow(res32)\n" );
          break;
        case BC_SET32_SB_CF_EFLAGS:
          fprintf ( f, "CF <-- check_borrow(res32)\n" );
          break;
        case BC_SET32_SF_EFLAGS:
          fprintf ( f, "SF <-- is_signed(res32)\n" );
          break;
        case BC_SET32_ZF_EFLAGS:
          fprintf ( f, "ZF <-- res32==0\n");
          break;
        case BC_SET32_PF_EFLAGS:
          fprintf ( f, "PF <-- check_parity(res32&FFh)\n" );
          break;
        case BC_SET16_AD_OF_EFLAGS:
          fprintf ( f, "OF <-- check_overflow_add(res16)\n" );
          break;
        case BC_SET16_AD_AF_EFLAGS:
          fprintf ( f, "AF <-- check_aux_carry(res16)\n" );
          break;
        case BC_SET16_AD_CF_EFLAGS:
          fprintf ( f, "CF <-- check_carry(res16)\n" );
          break;
        case BC_SET16_MUL_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- ((res16&8000h)!=0)^"
                    "((res32&80000000h)!=0)\n" );
          break;
        case BC_SET16_MUL1_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- DX!=0\n" );
          break;
        case BC_SET16_IMUL1_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- sign_extend(AX)!=res32\n" );
          break;
        case BC_SET16_NEG_CF_EFLAGS:
          fprintf ( f, "CF <-- op16[1]!=0\n" );
          break;
        case BC_SET16_RCL_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- (((op16[0]&8000h)!=0)^"
                    "((op16[0]&4000h)!=0))}\n" );
          break;
        case BC_SET16_RCL_CF_EFLAGS:
          fprintf ( f,"if(count>0) {CF <-- ((op16[0]>>(16-count))&0x1)!=0}\n" );
          break;
        case BC_SET16_RCR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- ((op16[0]&8000h)^"
                    "(CF!=0))}\n" );
          break;
        case BC_SET16_RCR_CF_EFLAGS:
          fprintf ( f, "if(count>0) {CF <-- ((op16[0]>>(count-1))&0x1)!=0}\n" );
          break;
        case BC_SET16_ROL_CF_EFLAGS:
          fprintf ( f, "CF <-- (res16&0x1)!=0\n" );
          break;
        case BC_SET16_ROR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- ((op16[0]&8000h)^"
                    "((op16[0]&01h)<<15))!=0}\n" );
          break;
        case BC_SET16_ROR_CF_EFLAGS:
          fprintf ( f, "CF <-- (res16&8000h)!=0\n" );
          break;
        case BC_SET16_SAR_CF_EFLAGS:
          fprintf ( f, "if(count>=16) {CF <-- op16[0]&8000h} else"
                    " if(count>0) {CF <-- (op16[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET16_SHIFT_SF_EFLAGS:
          fprintf ( f, "if(count!=0) {SF <-- is_signed(res16)}\n" );
          break;
        case BC_SET16_SHIFT_ZF_EFLAGS:
          fprintf ( f, "if(count!=0) {ZF <-- res16==0}\n");
          break;
        case BC_SET16_SHIFT_PF_EFLAGS:
          fprintf ( f, "if(count!=0) {PF <-- check_parity(res16&FFh)}\n" );
          break;
        case BC_SET16_SHL_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- "
                    "((op16[0]&8000h)^((op16[0]&4000h)<<1))!=0}\n" );
          break;
        case BC_SET16_SHL_CF_EFLAGS:
          fprintf ( f, "if(count>16) {CF <-- 0} else if(count>0) "
                    "{CF <-- (op16[0]&(8000h>>(count-1))!=0}\n" );
          break;
        case BC_SET16_SHR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- (op16[0]&8000h)!=0}\n" );
          break;
        case BC_SET16_SHR_CF_EFLAGS:
          fprintf ( f, "if(count>16) {CF <-- 0} else if(count>0)"
                    " {CF <-- (op16[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET16_SHRD_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- "
                    "((res16^op16[0])&8000h)!=0}\n" );
          break;
        case BC_SET16_SHRD_CF_EFLAGS:
          fprintf ( f, "if(count>0 && count<=16) {CF <-- "
                    "(op16[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET16_SHLD_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- "
                    "((res16^op16[0])&8000h) }\n" );
          break;
        case BC_SET16_SHLD_CF_EFLAGS:
          fprintf ( f, "if(count>0 && count<=16) "
                    "{CF <-- (op16[0]&(1<<(16-count)))!=0}\n" );
          break;
        case BC_SET16_SB_OF_EFLAGS:
          fprintf ( f, "OF <-- check_overflow_sub(res16)\n" );
          break;
        case BC_SET16_SB_AF_EFLAGS:
          fprintf ( f, "AF <-- check_aux_borrow(res16)\n" );
          break;
        case BC_SET16_SB_CF_EFLAGS:
          fprintf ( f, "CF <-- check_borrow(res16)\n" );
          break;
        case BC_SET16_SF_EFLAGS:
          fprintf ( f, "SF <-- is_signed(res16)\n" );
          break;
        case BC_SET16_ZF_EFLAGS:
          fprintf ( f, "ZF <-- res16==0\n");
          break;
        case BC_SET16_PF_EFLAGS:
          fprintf ( f, "PF <-- check_parity(res16&FFh)\n" );
          break;
        case BC_SET8_AD_OF_EFLAGS:
          fprintf ( f, "OF <-- check_overflow_add(res8)\n" );
          break;
        case BC_SET8_AD_AF_EFLAGS:
          fprintf ( f, "AF <-- check_aux_carry(res8)\n" );
          break;
        case BC_SET8_AD_CF_EFLAGS:
          fprintf ( f, "CF <-- check_carry(res8)\n" );
          break;
        case BC_SET8_MUL1_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- (AX&FF00h)!=0\n" );
          break;
        case BC_SET8_IMUL1_CFOF_EFLAGS:
          fprintf ( f, "CF,OF <-- sign_extend(AX&FFh)!=res16\n" );
          break;
        case BC_SET8_NEG_CF_EFLAGS:
          fprintf ( f, "CF <-- op8[1]!=0\n" );
          break;
        case BC_SET8_RCL_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- (((op8[0]&80h)!=0)^"
                    "((op8[0]&40h)!=0))}\n" );
          break;
        case BC_SET8_RCL_CF_EFLAGS:
          fprintf ( f,"if(count>0) {CF <-- ((op8[0]>>(8-count))&0x1)!=0}\n" );
          break;
        case BC_SET8_RCR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- ((op8[0]&80h)^"
                    "(CF!=0))}\n" );
          break;
        case BC_SET8_RCR_CF_EFLAGS:
          fprintf ( f, "if(count>0) {CF <-- ((op8[0]>>(count-1))&0x1)!=0}\n" );
          break;
        case BC_SET8_ROL_CF_EFLAGS:
          fprintf ( f, "CF <-- (res8&0x1)!=0\n" );
          break;
        case BC_SET8_ROR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- ((op8[0]&80h)^"
                    "((op8[0]&01h)<<7))!=0}\n" );
          break;
        case BC_SET8_ROR_CF_EFLAGS:
          fprintf ( f, "CF <-- (res8&80h)!=0\n" );
          break;
        case BC_SET8_SAR_CF_EFLAGS:
          fprintf ( f, "if(count>=8) {CF <-- op8[0]&80h} else"
                    " if(count>0) {CF <-- (op8[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET8_SHIFT_SF_EFLAGS:
          fprintf ( f, "if(count!=0) {SF <-- is_signed(res8)}\n" );
          break;
        case BC_SET8_SHIFT_ZF_EFLAGS:
          fprintf ( f, "if(count!=0) {ZF <-- res8==0}\n");
          break;
        case BC_SET8_SHIFT_PF_EFLAGS:
          fprintf ( f, "if(count!=0) {PF <-- check_parity(res8)}\n" );
          break;
        case BC_SET8_SHL_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- "
                    "((op8[0]&80h)^((op8[0]&40h)<<1))!=0}\n" );
          break;
        case BC_SET8_SHL_CF_EFLAGS:
          fprintf ( f, "if(count>8) {CF <-- 0} else if(count>0) "
                    "{CF <-- (op8[0]&(80h>>(count-1))!=0}\n" );
          break;
        case BC_SET8_SHR_OF_EFLAGS:
          fprintf ( f, "if(count==1) {OF <-- (op8[0]&80h)!=0}\n" );
          break;
        case BC_SET8_SHR_CF_EFLAGS:
          fprintf ( f, "if(count>8) {CF <-- 0} else if(count>0)"
                    " {CF <-- (op8[0]&(1<<(count-1)))!=0}\n" );
          break;
        case BC_SET8_SB_OF_EFLAGS:
          fprintf ( f, "OF <-- check_overflow_sub(res8)\n" );
          break;
        case BC_SET8_SB_AF_EFLAGS:
          fprintf ( f, "AF <-- check_aux_borrow(res8)\n" );
          break;
        case BC_SET8_SB_CF_EFLAGS:
          fprintf ( f, "CF <-- check_borrow(res8)\n" );
          break;
        case BC_SET8_SF_EFLAGS:
          fprintf ( f, "SF <-- is_signed(res8)\n" );
          break;
        case BC_SET8_ZF_EFLAGS:
          fprintf ( f, "ZF <-- res8==0\n");
          break;
        case BC_SET8_PF_EFLAGS:
          fprintf ( f, "PF <-- check_parity(res8)\n" );
          break;
        case BC_CLEAR_OF_CF_EFLAGS:
          fprintf ( f, "OF <-- 0; CF <-- 0\n" );
          break;
        case BC_CLEAR_CF_EFLAGS: fprintf ( f, "CF <-- 0\n" ); break;
        case BC_CLEAR_DF_EFLAGS: fprintf ( f, "DF <-- 0\n" ); break;
        case BC_SET_CF_EFLAGS: fprintf ( f, "CF <-- 1\n" ); break;
        case BC_SET_DF_EFLAGS: fprintf ( f, "DF <-- 1\n" ); break;
        case BC_CLEAR_TS_CR0: fprintf ( f, "CR0.TS <-- 0\n" ); break;
        case BC_SET32_SIMM_RES:
          fprintf ( f, "res32= sign_extend(%04Xh)\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET32_SIMM_OP0:
          fprintf ( f, "op32[0]= sign_extend(%04Xh)\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET32_SIMM_OP1:
          fprintf ( f, "op32[1]= sign_extend(%04Xh)\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET32_IMM32_RES:
          fprintf ( f, "res32= %08Xh\n",
                    ((uint32_t) p->v[n+1]) | (((uint32_t) (p->v[n+2]))<<16) );
          n+= 2;
          break;
        case BC_SET32_IMM32_OP0:
          fprintf ( f, "op32[0]= %08Xh\n",
                    ((uint32_t) p->v[n+1]) | (((uint32_t) (p->v[n+2]))<<16) );
          n+= 2;
          break;
        case BC_SET32_IMM32_OP1:
          fprintf ( f, "op32[1]= %08Xh\n",
                    ((uint32_t) p->v[n+1]) | (((uint32_t) (p->v[n+2]))<<16) );
          n+= 2;
          break;
        case BC_SET16_IMM_RES:
          fprintf ( f, "res16= %04Xh\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET16_IMM_OP0:
          fprintf ( f, "op16[0]= %04Xh\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET16_IMM_OP1:
          fprintf ( f, "op16[1]= %04Xh\n", p->v[n+1] );
          ++n;
          break;
        case BC_SET8_IMM_RES:
          fprintf ( f, "res8= %02Xh\n", (uint8_t) p->v[n+1] );
          ++n;
          break;
        case BC_SET8_IMM_OP0:
          fprintf ( f, "op8[0]= %02Xh\n", (uint8_t) p->v[n+1] );
          ++n;
          break;
        case BC_SET8_IMM_OP1:
          fprintf ( f, "op8[1]= %02Xh\n", (uint8_t) p->v[n+1] );
          ++n;
          break;
        case BC_SET32_1_OP1: fprintf ( f, "op32[1]= 1\n" ); break;
        case BC_SET32_OP0_OP1_0_OP0:
          fprintf ( f, "op32[1]= op32[0]; op32[0]= 0\n" );
          break;
        case BC_SET32_OP0_RES: fprintf ( f, "res32= op32[0]\n" ); break;
        case BC_SET32_OP1_RES: fprintf ( f, "res32= op32[1]\n" ); break;
        case BC_SET32_DR7_RES: fprintf ( f, "res32= DR7\n" ); break;
        case BC_SET32_DR7_OP0: fprintf ( f, "op32[0]= DR7\n" ); break;
        case BC_SET32_DR7_OP1: fprintf ( f, "op32[1]= DR7\n" ); break;
        case BC_SET32_CR0_RES_NOCHECK:
          fprintf ( f, "res32= CR0&0x0000FFFF (no pl checks)\n" );
          break;
        case BC_SET32_CR0_RES: fprintf ( f, "res32= CR0\n" ); break;
        case BC_SET32_CR0_OP0: fprintf ( f, "op32[0]= CR0\n" ); break;
        case BC_SET32_CR0_OP1: fprintf ( f, "op32[1]= CR0\n" ); break;
        case BC_SET32_CR2_RES: fprintf ( f, "res32= CR2\n" ); break;
        case BC_SET32_CR2_OP0: fprintf ( f, "op32[0]= CR2\n" ); break;
        case BC_SET32_CR2_OP1: fprintf ( f, "op32[1]= CR2\n" ); break;
        case BC_SET32_CR3_RES: fprintf ( f, "res32= CR3\n" ); break;
        case BC_SET32_CR3_OP0: fprintf ( f, "op32[0]= CR3\n" ); break;
        case BC_SET32_CR3_OP1: fprintf ( f, "op32[1]= CR3\n" ); break;
        case BC_SET32_EAX_RES: fprintf ( f, "res32= EAX\n" ); break;
        case BC_SET32_EAX_OP0: fprintf ( f, "op32[0]= EAX\n" ); break;
        case BC_SET32_EAX_OP1: fprintf ( f, "op32[1]= EAX\n" ); break;
        case BC_SET32_ECX_RES: fprintf ( f, "res32= ECX\n" ); break;
        case BC_SET32_ECX_OP0: fprintf ( f, "op32[0]= ECX\n" ); break;
        case BC_SET32_ECX_OP1: fprintf ( f, "op32[1]= ECX\n" ); break;
        case BC_SET32_EDX_RES: fprintf ( f, "res32= EDX\n" ); break;
        case BC_SET32_EDX_OP0: fprintf ( f, "op32[0]= EDX\n" ); break;
        case BC_SET32_EDX_OP1: fprintf ( f, "op32[1]= EDX\n" ); break;
        case BC_SET32_EBX_RES: fprintf ( f, "res32= EBX\n" ); break;
        case BC_SET32_EBX_OP0: fprintf ( f, "op32[0]= EBX\n" ); break;
        case BC_SET32_EBX_OP1: fprintf ( f, "op32[1]= EBX\n" ); break;
        case BC_SET32_ESP_RES: fprintf ( f, "res32= ESP\n" ); break;
        case BC_SET32_ESP_OP0: fprintf ( f, "op32[0]= ESP\n" ); break;
        case BC_SET32_ESP_OP1: fprintf ( f, "op32[1]= ESP\n" ); break;
        case BC_SET32_EBP_RES: fprintf ( f, "res32= EBP\n" ); break;
        case BC_SET32_EBP_OP0: fprintf ( f, "op32[0]= EBP\n" ); break;
        case BC_SET32_EBP_OP1: fprintf ( f, "op32[1]= EBP\n" ); break;
        case BC_SET32_ESI_RES: fprintf ( f, "res32= ESI\n" ); break;
        case BC_SET32_ESI_OP0: fprintf ( f, "op32[0]= ESI\n" ); break;
        case BC_SET32_ESI_OP1: fprintf ( f, "op32[1]= ESI\n" ); break;
        case BC_SET32_EDI_RES: fprintf ( f, "res32= EDI\n" ); break;
        case BC_SET32_EDI_OP0: fprintf ( f, "op32[0]= EDI\n" ); break;
        case BC_SET32_EDI_OP1: fprintf ( f, "op32[1]= EDI\n" ); break;
        case BC_SET32_ES_RES: fprintf ( f, "res32= ES\n" ); break;
        case BC_SET32_ES_OP0: fprintf ( f, "op32[0]= ES\n" ); break;
        case BC_SET32_ES_OP1: fprintf ( f, "op32[1]= ES\n" ); break;
        case BC_SET32_CS_RES: fprintf ( f, "res32= CS\n" ); break;
        case BC_SET32_CS_OP0: fprintf ( f, "op32[0]= CS\n" ); break;
        case BC_SET32_CS_OP1: fprintf ( f, "op32[1]= CS\n" ); break;
        case BC_SET32_SS_RES: fprintf ( f, "res32= SS\n" ); break;
        case BC_SET32_SS_OP0: fprintf ( f, "op32[0]= SS\n" ); break;
        case BC_SET32_SS_OP1: fprintf ( f, "op32[1]= SS\n" ); break;
        case BC_SET32_DS_RES: fprintf ( f, "res32= DS\n" ); break;
        case BC_SET32_DS_OP0: fprintf ( f, "op32[0]= DS\n" ); break;
        case BC_SET32_DS_OP1: fprintf ( f, "op32[1]= DS\n" ); break;
        case BC_SET32_FS_RES: fprintf ( f, "res32= FS\n" ); break;
        case BC_SET32_FS_OP0: fprintf ( f, "op32[0]= FS\n" ); break;
        case BC_SET32_FS_OP1: fprintf ( f, "op32[1]= FS\n" ); break;
        case BC_SET32_GS_RES: fprintf ( f, "res32= GS\n" ); break;
        case BC_SET32_GS_OP0: fprintf ( f, "op32[0]= GS\n" ); break;
        case BC_SET32_GS_OP1: fprintf ( f, "op32[1]= GS\n" ); break;
        case BC_SET16_FPUCONTROL_RES:
          fprintf ( f, "res16= FPU_CONTROL\n" );
          break;
        case BC_SET16_FPUSTATUSTOP_RES:
          fprintf ( f, "res16= FPU_STATUS_TOP\n" );
          break;
        case BC_SET16_1_OP1: fprintf ( f, "op16[1]= 1\n" ); break;
        case BC_SET16_OP0_OP1_0_OP0:
          fprintf ( f, "op16[1]= op16[0]; op16[0]= 0\n" );
          break;
        case BC_SET16_OP0_RES: fprintf ( f, "res16= op16[0]\n" ); break;
        case BC_SET16_OP1_RES: fprintf ( f, "res16= op16[1]\n" ); break;
        case BC_SET16_CR0_RES_NOCHECK:
          fprintf ( f, "res16= CR0&0xFFFF (no pl checks)\n" );
          break;
        case BC_SET16_LDTR_RES: fprintf ( f, "res16= LDTR\n" ); break;
        case BC_SET16_TR_RES: fprintf ( f, "res16= TR\n" ); break;
        case BC_SET16_AX_RES: fprintf ( f, "res16= AX\n" ); break;
        case BC_SET16_AX_OP0: fprintf ( f, "op16[0]= AX\n" ); break;
        case BC_SET16_AX_OP1: fprintf ( f, "op16[1]= AX\n" ); break;
        case BC_SET16_CX_RES: fprintf ( f, "res16= CX\n" ); break;
        case BC_SET16_CX_OP0: fprintf ( f, "op16[0]= CX\n" ); break;
        case BC_SET16_CX_OP1: fprintf ( f, "op16[1]= CX\n" ); break;
        case BC_SET16_DX_RES: fprintf ( f, "res16= DX\n" ); break;
        case BC_SET16_DX_OP0: fprintf ( f, "op16[0]= DX\n" ); break;
        case BC_SET16_DX_OP1: fprintf ( f, "op16[1]= DX\n" ); break;
        case BC_SET16_BX_RES: fprintf ( f, "res16= BX\n" ); break;
        case BC_SET16_BX_OP0: fprintf ( f, "op16[0]= BX\n" ); break;
        case BC_SET16_BX_OP1: fprintf ( f, "op16[1]= BX\n" ); break;
        case BC_SET16_SP_RES: fprintf ( f, "res16= SP\n" ); break;
        case BC_SET16_SP_OP0: fprintf ( f, "op16[0]= SP\n" ); break;
        case BC_SET16_SP_OP1: fprintf ( f, "op16[1]= SP\n" ); break;
        case BC_SET16_BP_RES: fprintf ( f, "res16= BP\n" ); break;
        case BC_SET16_BP_OP0: fprintf ( f, "op16[0]= BP\n" ); break;
        case BC_SET16_BP_OP1: fprintf ( f, "op16[1]= BP\n" ); break;
        case BC_SET16_SI_RES: fprintf ( f, "res16= SI\n" ); break;
        case BC_SET16_SI_OP0: fprintf ( f, "op16[0]= SI\n" ); break;
        case BC_SET16_SI_OP1: fprintf ( f, "op16[1]= SI\n" ); break;
        case BC_SET16_DI_RES: fprintf ( f, "res16= DI\n" ); break;
        case BC_SET16_DI_OP0: fprintf ( f, "op16[0]= DI\n" ); break;
        case BC_SET16_DI_OP1: fprintf ( f, "op16[1]= DI\n" ); break;
        case BC_SET16_ES_RES: fprintf ( f, "res16= ES\n" ); break;
        case BC_SET16_ES_OP0: fprintf ( f, "op16[0]= ES\n" ); break;
        case BC_SET16_ES_OP1: fprintf ( f, "op16[1]= ES\n" ); break;
        case BC_SET16_CS_RES: fprintf ( f, "res16= CS\n" ); break;
        case BC_SET16_CS_OP0: fprintf ( f, "op16[0]= CS\n" ); break;
        case BC_SET16_CS_OP1: fprintf ( f, "op16[1]= CS\n" ); break;
        case BC_SET16_SS_RES: fprintf ( f, "res16= SS\n" ); break;
        case BC_SET16_SS_OP0: fprintf ( f, "op16[0]= SS\n" ); break;
        case BC_SET16_SS_OP1: fprintf ( f, "op16[1]= SS\n" ); break;
        case BC_SET16_DS_RES: fprintf ( f, "res16= DS\n" ); break;
        case BC_SET16_DS_OP0: fprintf ( f, "op16[0]= DS\n" ); break;
        case BC_SET16_DS_OP1: fprintf ( f, "op16[1]= DS\n" ); break;
        case BC_SET16_FS_RES: fprintf ( f, "res16= FS\n" ); break;
        case BC_SET16_FS_OP0: fprintf ( f, "op16[0]= FS\n" ); break;
        case BC_SET16_FS_OP1: fprintf ( f, "op16[1]= FS\n" ); break;
        case BC_SET16_GS_RES: fprintf ( f, "res16= GS\n" ); break;
        case BC_SET16_GS_OP0: fprintf ( f, "op16[0]= GS\n" ); break;
        case BC_SET16_GS_OP1: fprintf ( f, "op16[1]= GS\n" ); break;
        case BC_SET8_COND_RES: fprintf ( f, "res8= cond ? 1 : 0\n" ); break;
        case BC_SET8_OP0_OP1_0_OP0:
          fprintf ( f, "op8[1]= op8[0]; op8[0]= 0\n" );
          break;
        case BC_SET8_1_OP1: fprintf ( f, "op8[1]= 1\n" ); break;
        case BC_SET8_OP0_RES: fprintf ( f, "res8= op8[0]\n" ); break;
        case BC_SET8_OP1_RES: fprintf ( f, "res8= op8[1]\n" ); break;
        case BC_SET8_AL_RES: fprintf ( f, "res8= AL\n" ); break;
        case BC_SET8_AL_OP0: fprintf ( f, "op8[0]= AL\n" ); break;
        case BC_SET8_AL_OP1: fprintf ( f, "op8[1]= AL\n" ); break;
        case BC_SET8_CL_RES: fprintf ( f, "res8= CL\n" ); break;
        case BC_SET8_CL_OP0: fprintf ( f, "op8[0]= CL\n" ); break;
        case BC_SET8_CL_OP1: fprintf ( f, "op8[1]= CL\n" ); break;
        case BC_SET8_DL_RES: fprintf ( f, "res8= DL\n" ); break;
        case BC_SET8_DL_OP0: fprintf ( f, "op8[0]= DL\n" ); break;
        case BC_SET8_DL_OP1: fprintf ( f, "op8[1]= DL\n" ); break;
        case BC_SET8_BL_RES: fprintf ( f, "res8= BL\n" ); break;
        case BC_SET8_BL_OP0: fprintf ( f, "op8[0]= BL\n" ); break;
        case BC_SET8_BL_OP1: fprintf ( f, "op8[1]= BL\n" ); break;
        case BC_SET8_AH_RES: fprintf ( f, "res8= AH\n" ); break;
        case BC_SET8_AH_OP0: fprintf ( f, "op8[0]= AH\n" ); break;
        case BC_SET8_AH_OP1: fprintf ( f, "op8[1]= AH\n" ); break;
        case BC_SET8_CH_RES: fprintf ( f, "res8= CH\n" ); break;
        case BC_SET8_CH_OP0: fprintf ( f, "op8[0]= CH\n" ); break;
        case BC_SET8_CH_OP1: fprintf ( f, "op8[1]= CH\n" ); break;
        case BC_SET8_DH_RES: fprintf ( f, "res8= DH\n" ); break;
        case BC_SET8_DH_OP0: fprintf ( f, "op8[0]= DH\n" ); break;
        case BC_SET8_DH_OP1: fprintf ( f, "op8[1]= DH\n" ); break;
        case BC_SET8_BH_RES: fprintf ( f, "res8= BH\n" ); break;
        case BC_SET8_BH_OP0: fprintf ( f, "op8[0]= BH\n" ); break;
        case BC_SET8_BH_OP1: fprintf ( f, "op8[1]= BH\n" ); break;
        case BC_SET32_RES_FLOATU32:
          fprintf ( f, "float_u32.u32= res32\n" );
          break;
        case BC_SET32_RES_DOUBLEU64L:
          fprintf ( f, "double_u64.u32.l= res32\n" );
          break;
        case BC_SET32_RES_DOUBLEU64H:
          fprintf ( f, "double_u64.u32.h= res32\n" );
          break;
        case BC_SET32_RES_LDOUBLEU80L:
          fprintf ( f, "ldouble_u80.u32.v0= res32\n" );
          break;
        case BC_SET32_RES_LDOUBLEU80M:
          fprintf ( f, "ldouble_u80.u32.v1= res32\n" );
          break;
        case BC_SET32_SRES_LDOUBLE:
          fprintf ( f, "ldouble= (int32_t) res32\n" );
          break;
        case BC_SET32_RES_SELECTOR: fprintf ( f, "selector= res32\n" ); break;
        case BC_SET32_RES_DR7: fprintf ( f, "DR7= res32\n" ); break;
        case BC_SET32_RES_CR0: fprintf ( f, "CR0= res32\n" ); break;
        case BC_SET32_RES_CR2: fprintf ( f, "CR2= res32\n" ); break;
        case BC_SET32_RES_CR3: fprintf ( f, "CR3= res32\n" ); break;
        case BC_SET32_RES_EAX: fprintf ( f, "EAX= res32\n" ); break;
        case BC_SET32_RES_ECX: fprintf ( f, "ECX= res32\n" ); break;
        case BC_SET32_RES_EDX: fprintf ( f, "EDX= res32\n" ); break;
        case BC_SET32_RES_EBX: fprintf ( f, "EBX= res32\n" ); break;
        case BC_SET32_RES_ESP: fprintf ( f, "ESP= res32\n" ); break;
        case BC_SET32_RES_EBP: fprintf ( f, "EBP= res32\n" ); break;
        case BC_SET32_RES_ESI: fprintf ( f, "ESI= res32\n" ); break;
        case BC_SET32_RES_EDI: fprintf ( f, "EDI= res32\n" ); break;
        case BC_SET32_RES_ES: fprintf ( f, "ES= (uint16_t) res32\n" ); break;
        case BC_SET32_RES_SS: fprintf ( f, "SS= (uint16_t) res32\n" ); break;
        case BC_SET32_RES_DS: fprintf ( f, "DS= (uint16_t) res32\n" ); break;
        case BC_SET32_RES_FS: fprintf ( f, "FS= (uint16_t) res32\n" ); break;
        case BC_SET32_RES_GS: fprintf ( f, "GS= (uint16_t) res32\n" ); break;
        case BC_SET32_RES8_RES: fprintf ( f, "res32= res8\n" ); break;
        case BC_SET32_RES8_RES_SIGNED:
          fprintf ( f, "res32= sign_extend(res8)\n" );
          break;
        case BC_SET32_RES16_RES: fprintf ( f, "res32= res16\n" ); break;
        case BC_SET32_RES16_RES_SIGNED:
          fprintf ( f, "res32= sign_extend(res16)\n" );
          break;
        case BC_SET32_RES64_RES:
          fprintf ( f, "res32= res64&FFFFFFFFh\n" );
          break;
        case BC_SET32_RES64_EAX_EDX:
          fprintf ( f, "EAX= res64&FFFFFFFFh; EDX= res64>>32\n" );
          break;
        case BC_SET16_RES_LDOUBLEU80H:
          fprintf ( f, "ldouble_u80.u16.v4= res16; "
                    "ldouble_u80.u16.v5= 0; ldouble_u80.u32.v3= 0\n" );
          break;
        case BC_SET16_RES_SELECTOR: fprintf ( f, "selector= res16\n" ); break;
        case BC_SET16_RES_CR0: fprintf ( f, "CR0= res16\n" ); break;
        case BC_SET16_RES_AX: fprintf ( f, "AX= res16\n" ); break;
        case BC_SET16_RES_CX: fprintf ( f, "CX= res16\n" ); break;
        case BC_SET16_RES_DX: fprintf ( f, "DX= res16\n" ); break;
        case BC_SET16_RES_BX: fprintf ( f, "BX= res16\n" ); break;
        case BC_SET16_RES_SP: fprintf ( f, "SP= res16\n" ); break;
        case BC_SET16_RES_BP: fprintf ( f, "BP= res16\n" ); break;
        case BC_SET16_RES_SI: fprintf ( f, "SI= res16\n" ); break;
        case BC_SET16_RES_DI: fprintf ( f, "DI= res16\n" ); break;
        case BC_SET16_RES_ES: fprintf ( f, "ES= res16\n" ); break;
        case BC_SET16_RES_SS: fprintf ( f, "SS= res16\n" ); break;
        case BC_SET16_RES_DS: fprintf ( f, "DS= res16\n" ); break;
        case BC_SET16_RES_FS: fprintf ( f, "FS= res16\n" ); break;
        case BC_SET16_RES_GS: fprintf ( f, "GS= res16\n" ); break;
        case BC_SET16_RES8_RES: fprintf ( f, "res16= res8\n" ); break;
        case BC_SET16_RES8_RES_SIGNED:
          fprintf ( f, "res16= sign_extended(res8)\n" );
          break;
        case BC_SET16_RES32_RES:
          fprintf ( f, "res16= res32&FFFFh\n" );
          break;
        case BC_SET16_RES32_AX_DX:
          fprintf ( f, "AX= res32&FFFFh; DX= res32>>16\n" );
          break;
        case BC_SET8_RES_AL: fprintf ( f, "AL= res8\n" ); break;
        case BC_SET8_RES_CL: fprintf ( f, "CL= res8\n" ); break;
        case BC_SET8_RES_DL: fprintf ( f, "DL= res8\n" ); break;
        case BC_SET8_RES_BL: fprintf ( f, "BL= res8\n" ); break;
        case BC_SET8_RES_AH: fprintf ( f, "AH= res8\n" ); break;
        case BC_SET8_RES_CH: fprintf ( f, "CH= res8\n" ); break;
        case BC_SET8_RES_DH: fprintf ( f, "DH= res8\n" ); break;
        case BC_SET8_RES_BH: fprintf ( f, "BH= res8\n" ); break;
        case BC_SET32_OFFSET_EAX: fprintf ( f, "EAX= offset\n" ); break;
        case BC_SET32_OFFSET_ECX: fprintf ( f, "ECX= offset\n" ); break;
        case BC_SET32_OFFSET_EDX: fprintf ( f, "EDX= offset\n" ); break;
        case BC_SET32_OFFSET_EBX: fprintf ( f, "EBX= offset\n" ); break;
        case BC_SET32_OFFSET_ESP: fprintf ( f, "ESP= offset\n" ); break;
        case BC_SET32_OFFSET_EBP: fprintf ( f, "EBP= offset\n" ); break;
        case BC_SET32_OFFSET_ESI: fprintf ( f, "ESI= offset\n" ); break;
        case BC_SET32_OFFSET_EDI: fprintf ( f, "EDI= offset\n" ); break;
        case BC_SET32_OFFSET_RES: fprintf ( f, "res32= offset\n" ); break;
        case BC_SET16_OFFSET_AX: fprintf ( f, "AX= offset\n" ); break;
        case BC_SET16_OFFSET_CX: fprintf ( f, "CX= offset\n" ); break;
        case BC_SET16_OFFSET_DX: fprintf ( f, "DX= offset\n" ); break;
        case BC_SET16_OFFSET_BX: fprintf ( f, "BX= offset\n" ); break;
        case BC_SET16_OFFSET_SP: fprintf ( f, "SP= offset\n" ); break;
        case BC_SET16_OFFSET_BP: fprintf ( f, "BP= offset\n" ); break;
        case BC_SET16_OFFSET_SI: fprintf ( f, "SI= offset\n" ); break;
        case BC_SET16_OFFSET_DI: fprintf ( f, "DI= offset\n" ); break;
        case BC_SET16_OFFSET_RES: fprintf ( f, "res16= offset\n" ); break;
        case BC_SET16_SELECTOR_RES: fprintf ( f, "res16= selector\n" ); break;
        case BC_DS_READ32_RES:
          fprintf ( f, "res32= READ32(DS:offset)\n" );
          break;
        case BC_DS_READ32_OP0:
          fprintf ( f, "op32[0]= READ32(DS:offset)\n" );
          break;
        case BC_DS_READ32_OP1:
          fprintf ( f, "op32[1]= READ32(DS:offset)\n" );
          break;
        case BC_SS_READ32_RES:
          fprintf ( f, "res32= READ32(SS:offset)\n" );
          break;
        case BC_SS_READ32_OP0:
          fprintf ( f, "op32[0]= READ32(SS:offset)\n" );
          break;
        case BC_SS_READ32_OP1:
          fprintf ( f, "op32[1]= READ32(SS:offset)\n" );
          break;
        case BC_ES_READ32_RES:
          fprintf ( f, "res32= READ32(ES:offset)\n" );
          break;
        case BC_ES_READ32_OP0:
          fprintf ( f, "op32[0]= READ32(ES:offset)\n" );
          break;
        case BC_ES_READ32_OP1:
          fprintf ( f, "op32[1]= READ32(ES:offset)\n" );
          break;
        case BC_CS_READ32_RES:
          fprintf ( f, "res32= READ32(CS:offset)\n" );
          break;
        case BC_CS_READ32_OP0:
          fprintf ( f, "op32[0]= READ32(CS:offset)\n" );
          break;
        case BC_CS_READ32_OP1:
          fprintf ( f, "op32[1]= READ32(CS:offset)\n" );
          break;
        case BC_FS_READ32_RES:
          fprintf ( f, "res32= READ32(FS:offset)\n" );
          break;
        case BC_FS_READ32_OP0:
          fprintf ( f, "op32[0]= READ32(FS:offset)\n" );
          break;
        case BC_FS_READ32_OP1:
          fprintf ( f, "op32[1]= READ32(FS:offset)\n" );
          break;
        case BC_GS_READ32_RES:
          fprintf ( f, "res32= READ32(GS:offset)\n" );
          break;
        case BC_GS_READ32_OP0:
          fprintf ( f, "op32[0]= READ32(GS:offset)\n" );
          break;
        case BC_GS_READ32_OP1:
          fprintf ( f, "op32[1]= READ32(GS:offset)\n" );
          break;
        case BC_DS_READ16_RES:
          fprintf ( f, "res16= READ16(DS:offset)\n" );
          break;
        case BC_DS_READ16_OP0:
          fprintf ( f, "op16[0]= READ16(DS:offset)\n" );
          break;
        case BC_DS_READ16_OP1:
          fprintf ( f, "op16[1]= READ16(DS:offset)\n" );
          break;
        case BC_SS_READ16_RES:
          fprintf ( f, "res16= READ16(SS:offset)\n" );
          break;
        case BC_SS_READ16_OP0:
          fprintf ( f, "op16[0]= READ16(SS:offset)\n" );
          break;
        case BC_SS_READ16_OP1:
          fprintf ( f, "op16[1]= READ16(SS:offset)\n" );
          break;
        case BC_ES_READ16_RES:
          fprintf ( f, "res16= READ16(ES:offset)\n" );
          break;
        case BC_ES_READ16_OP0:
          fprintf ( f, "op16[0]= READ16(ES:offset)\n" );
          break;
        case BC_ES_READ16_OP1:
          fprintf ( f, "op16[1]= READ16(ES:offset)\n" );
          break;
        case BC_CS_READ16_RES:
          fprintf ( f, "res16= READ16(CS:offset)\n" );
          break;
        case BC_CS_READ16_OP0:
          fprintf ( f, "op16[0]= READ16(CS:offset)\n" );
          break;
        case BC_CS_READ16_OP1:
          fprintf ( f, "op16[1]= READ16(CS:offset)\n" );
          break;
        case BC_FS_READ16_RES:
          fprintf ( f, "res16= READ16(FS:offset)\n" );
          break;
        case BC_FS_READ16_OP0:
          fprintf ( f, "op16[0]= READ16(FS:offset)\n" );
          break;
        case BC_FS_READ16_OP1:
          fprintf ( f, "op16[1]= READ16(FS:offset)\n" );
          break;
        case BC_GS_READ16_RES:
          fprintf ( f, "res16= READ16(GS:offset)\n" );
          break;
        case BC_GS_READ16_OP0:
          fprintf ( f, "op16[0]= READ16(GS:offset)\n" );
          break;
        case BC_GS_READ16_OP1:
          fprintf ( f, "op16[1]= READ16(GS:offset)\n" );
          break;
        case BC_DS_READ8_RES:
          fprintf ( f, "res8= READ8(DS:offset)\n" );
          break;
        case BC_DS_READ8_OP0:
          fprintf ( f, "op8[0]= READ8(DS:offset)\n" );
          break;
        case BC_DS_READ8_OP1:
          fprintf ( f, "op8[1]= READ8(DS:offset)\n" );
          break;
        case BC_SS_READ8_RES:
          fprintf ( f, "res8= READ8(SS:offset)\n" );
          break;
        case BC_SS_READ8_OP0:
          fprintf ( f, "op8[0]= READ8(SS:offset)\n" );
          break;
        case BC_SS_READ8_OP1:
          fprintf ( f, "op8[1]= READ8(SS:offset)\n" );
          break;
        case BC_ES_READ8_RES:
          fprintf ( f, "res8= READ8(ES:offset)\n" );
          break;
        case BC_ES_READ8_OP0:
          fprintf ( f, "op8[0]= READ8(ES:offset)\n" );
          break;
        case BC_ES_READ8_OP1:
          fprintf ( f, "op8[1]= READ8(ES:offset)\n" );
          break;
        case BC_CS_READ8_RES:
          fprintf ( f, "res8= READ8(CS:offset)\n" );
          break;
        case BC_CS_READ8_OP0:
          fprintf ( f, "op8[0]= READ8(CS:offset)\n" );
          break;
        case BC_CS_READ8_OP1:
          fprintf ( f, "op8[1]= READ8(CS:offset)\n" );
          break;
        case BC_FS_READ8_RES:
          fprintf ( f, "res8= READ8(FS:offset)\n" );
          break;
        case BC_FS_READ8_OP0:
          fprintf ( f, "op8[0]= READ8(FS:offset)\n" );
          break;
        case BC_FS_READ8_OP1:
          fprintf ( f, "op8[1]= READ8(FS:offset)\n" );
          break;
        case BC_GS_READ8_RES:
          fprintf ( f, "res8= READ8(GS:offset)\n" );
          break;
        case BC_GS_READ8_OP0:
          fprintf ( f, "op8[0]= READ8(GS:offset)\n" );
          break;
        case BC_GS_READ8_OP1:
          fprintf ( f, "op8[1]= READ8(GS:offset)\n" );
          break;
        case BC_DS_READ_SELECTOR_OFFSET:
          fprintf ( f, "selector= READ16(DS:offset);"
                    " offset= READ32(DS:offset+2)\n" );
          break;
        case BC_SS_READ_SELECTOR_OFFSET:
          fprintf ( f, "selector= READ16(SS:offset);"
                    " offset= READ32(SS:offset+2)\n" );
          break;
        case BC_ES_READ_SELECTOR_OFFSET:
          fprintf ( f, "selector= READ16(ES:offset);"
                    " offset= READ32(ES:offset+2)\n" );
          break;
        case BC_CS_READ_SELECTOR_OFFSET:
          fprintf ( f, "selector= READ16(CS:offset);"
                    " offset= READ32(CS:offset+2)\n" );
          break;
        case BC_GS_READ_SELECTOR_OFFSET:
          fprintf ( f, "selector= READ16(GS:offset);"
                    " offset= READ32(GS:offset+2)\n" );
          break;
        case BC_DS_READ_OFFSET_SELECTOR:
          fprintf ( f, "offset= READ32(DS:offset);"
                    " selector= READ16(DS:offset+4)\n" );
          break;
        case BC_DS_READ_OFFSET16_SELECTOR:
          fprintf ( f, "offset= READ16(DS:offset);"
                    " selector= READ16(DS:offset+2)\n" );
          break;
        case BC_SS_READ_OFFSET_SELECTOR:
          fprintf ( f, "offset= READ32(SS:offset);"
                    " selector= READ16(SS:offset+4)\n" );
          break;
        case BC_SS_READ_OFFSET16_SELECTOR:
          fprintf ( f, "offset= READ16(SS:offset);"
                    " selector= READ16(SS:offset+2)\n" );
          break;
        case BC_ES_READ_OFFSET_SELECTOR:
          fprintf ( f, "offset= READ32(ES:offset);"
                    " selector= READ16(ES:offset+4)\n" );
          break;
        case BC_ES_READ_OFFSET16_SELECTOR:
          fprintf ( f, "offset= READ16(ES:offset);"
                    " selector= READ16(ES:offset+2)\n" );
          break;
        case BC_CS_READ_OFFSET_SELECTOR:
          fprintf ( f, "offset= READ32(CS:offset);"
                    " selector= READ16(CS:offset+4)\n" );
          break;
        case BC_CS_READ_OFFSET16_SELECTOR:
          fprintf ( f, "offset= READ16(CS:offset);"
                    " selector= READ16(CS:offset+2)\n" );
          break;
        case BC_FS_READ_OFFSET_SELECTOR:
          fprintf ( f, "offset= READ32(FS:offset);"
                    " selector= READ16(FS:offset+4)\n" );
          break;
        case BC_FS_READ_OFFSET16_SELECTOR:
          fprintf ( f, "offset= READ16(FS:offset);"
                    " selector= READ16(FS:offset+2)\n" );
          break;
        case BC_GS_READ_OFFSET_SELECTOR:
          fprintf ( f, "offset= READ32(GS:offset);"
                    " selector= READ16(GS:offset+4)\n" );
          break;
        case BC_GS_READ_OFFSET16_SELECTOR:
          fprintf ( f, "offset= READ16(GS:offset);"
                    " selector= READ16(GS:offset+2)\n" );
          break;
        case BC_DS_RES_WRITE32:
          fprintf ( f, "WRITE32(DS:offset,res32)\n" );
          break;
        case BC_SS_RES_WRITE32:
          fprintf ( f, "WRITE32(SS:offset,res32)\n" );
          break;
        case BC_ES_RES_WRITE32:
          fprintf ( f, "WRITE32(ES:offset,res32)\n" );
          break;
        case BC_CS_RES_WRITE32:
          fprintf ( f, "WRITE32(CS:offset,res32)\n" );
          break;
        case BC_FS_RES_WRITE32:
          fprintf ( f, "WRITE32(FS:offset,res32)\n" );
          break;
        case BC_GS_RES_WRITE32:
          fprintf ( f, "WRITE32(GS:offset,res32)\n" );
          break;
        case BC_DS_RES_WRITE16:
          fprintf ( f, "WRITE16(DS:offset,res16)\n" );
          break;
        case BC_SS_RES_WRITE16:
          fprintf ( f, "WRITE16(SS:offset,res16)\n" );
          break;
        case BC_ES_RES_WRITE16:
          fprintf ( f, "WRITE16(ES:offset,res16)\n" );
          break;
        case BC_CS_RES_WRITE16:
          fprintf ( f, "WRITE16(CS:offset,res16)\n" );
          break;
        case BC_FS_RES_WRITE16:
          fprintf ( f, "WRITE16(FS:offset,res16)\n" );
          break;
        case BC_GS_RES_WRITE16:
          fprintf ( f, "WRITE16(GS:offset,res16)\n" );
          break;
        case BC_DS_RES_WRITE8: fprintf ( f, "WRITE8(DS:offset,res8)\n" ); break;
        case BC_SS_RES_WRITE8: fprintf ( f, "WRITE8(SS:offset,res8)\n" ); break;
        case BC_ES_RES_WRITE8: fprintf ( f, "WRITE8(ES:offset,res8)\n" ); break;
        case BC_CS_RES_WRITE8: fprintf ( f, "WRITE8(CS:offset,res8)\n" ); break;
        case BC_FS_RES_WRITE8: fprintf ( f, "WRITE8(FS:offset,res8)\n" ); break;
        case BC_GS_RES_WRITE8: fprintf ( f, "WRITE8(GS:offset,res8)\n" ); break;
        case BC_PORT_READ32: fprintf ( f, "res32= PORT_READ32(port)\n" ); break;
        case BC_PORT_READ16: fprintf ( f, "res16= PORT_READ16(port)\n" ); break;
        case BC_PORT_READ8: fprintf ( f, "res8= PORT_READ8(port)\n" ); break;
        case BC_PORT_WRITE32: fprintf ( f, "PORT_WRITE32(port,res32)\n" );break;
        case BC_PORT_WRITE16: fprintf ( f, "PORT_WRITE16(port,res16)\n" );break;
        case BC_PORT_WRITE8: fprintf ( f, "PORT_WRITE8(port,res8)\n" ); break;
        case BC_JMP32_FAR:
          fprintf ( f, "jmp_far(selector,offset,op32=true) // Stop!\n" );
          break;
        case BC_JMP16_FAR:
          fprintf ( f, "jmp_far(selector,offset,op32=false) // Stop!\n" );
          break;
        case BC_JMP32_NEAR_REL:
          fprintf ( f, "jmp_near(EIP+%d+%d); goto EIP // Stop!\n",
                    p->v[n+2], (int16_t) p->v[n+1] );
          n+= 2;
          break;
        case BC_JMP16_NEAR_REL:
          fprintf ( f, "jmp_near((EIP+%d+%d)&FFFFh); goto EIP // Stop!\n",
                    p->v[n+2], (int16_t) p->v[n+1] );
          n+= 2;
          break;
        case BC_JMP32_NEAR_REL32:
          fprintf ( f, "jmp_near(EIP+%d+%d); goto EIP // Stop!\n",
                    p->v[n+3],
                    (int32_t) ((uint32_t) p->v[n+1]) |
                    (((uint32_t) (p->v[n+2]))<<16) );
          n+= 3;
          break;
        case BC_JMP32_NEAR_RES32:
          fprintf ( f, "jmp_near(res32); goto EIP // Stop!\n" );
          break;
        case BC_JMP16_NEAR_RES16:
          fprintf ( f, "jmp_near(res16); goto EIP // Stop!\n" );
          break;
        case BC_BRANCH32:
          fprintf ( f, "if(cond) { jmp_near(EIP+%d+%d) } else"
                    " { EIP+= %d }; goto EIP // Stop!\n",
                    p->v[n+2], (int16_t) p->v[n+1], p->v[n+2] );
          n+= 2;
          break;
        case BC_BRANCH32_IMM32:
          fprintf ( f, "if(cond) { jmp_near(EIP+%d+%d) } else"
                    " { EIP+= %d }; goto EIP // Stop!\n",
                    p->v[n+3],
                    (int32_t) (((uint32_t) p->v[n+1]) |
                               (((uint32_t) p->v[n+2])<<16)),
                    p->v[n+3] );
          n+= 3;
          break;
        case BC_BRANCH16:
          fprintf ( f, "if(cond) { jmp_near((EIP+%d+%d)&FFFFh) } else"
                    " { EIP+= %d }; goto EIP // Stop!\n",
                    p->v[n+2], (int16_t) p->v[n+1], p->v[n+2] );
          n+= 2;
          break;
        case BC_CALL32_FAR:
          fprintf ( f, "call_far(selector,offset,op32=true) // Stop!\n" );
          ++n;
          break;
        case BC_CALL16_FAR:
          fprintf ( f, "call_far(selector,offset,op32=false) // Stop!\n" );
          ++n;
          break;
        case BC_CALL32_NEAR_REL:
          fprintf ( f,
                    "push32(EIP+%d); "
                    "jmp_near(EIP+%d+%d); goto EIP // Stop!\n",
                    p->v[n+2], p->v[n+2], (int16_t) p->v[n+1] );
          n+= 2;
          break;
        case BC_CALL16_NEAR_REL:
          fprintf ( f,
                    "push16((EIP+%d)&FFFFh); "
                    "jmp_near((EIP+%d+%d)&FFFFh); goto EIP // Stop!\n",
                    p->v[n+2], p->v[n+2], (int16_t) p->v[n+1] );
          n+= 2;
          break;
        case BC_CALL32_NEAR_REL32:
          fprintf ( f,
                    "push32(EIP+%d); jmp_near(EIP+%d+%d); goto EIP // Stop!\n",
                    p->v[n+3], p->v[n+3],
                    (int32_t) ((uint32_t) p->v[n+1]) |
                    (((uint32_t) (p->v[n+2]))<<16) );
          n+= 3;
          break;
        case BC_CALL32_NEAR_RES32:
          fprintf ( f,
                    "push32(EIP+%d); jmp_near(res32); goto EIP // Stop!\n",
                    p->v[n+1] );
          n+= 1;
          break;
        case BC_CALL16_NEAR_RES16:
          fprintf ( f,
                    "push16((EIP+%d)&FFFFh); jmp_near(res16);"
                    " goto EIP // Stop!\n",
                    p->v[n+1] );
          n+= 1;
          break;
        case BC_IRET32: fprintf ( f, "iret32(); // Stop!\n" ); break;
        case BC_IRET16: fprintf ( f, "iret16(); // Stop!\n" ); break;
        case BC_ADC32: fprintf ( f, "res32= op32[0] + op32[1] + CF\n" ); break;
        case BC_ADC16: fprintf ( f, "res16= op16[0] + op16[1] + CF\n" ); break;
        case BC_ADC8: fprintf ( f, "res8= op8[0] + op8[1] + CF\n" ); break;
        case BC_ADD32: fprintf ( f, "res32= op32[0] + op32[1]\n" ); break;
        case BC_ADD16: fprintf ( f, "res16= op16[0] + op16[1]\n" ); break;
        case BC_ADD8: fprintf ( f, "res8= op8[0] + op8[1]\n" ); break;
        case BC_DIV32_EDX_EAX:
          fprintf ( f, "tmp64= ((EDX<<32)|EAX); res64= tmp64/op32[0];"
                    " EAX= res64; EDX= tmp64%%op32[0]\n" );
          break;
        case BC_DIV16_DX_AX:
          fprintf ( f, "tmp32= ((DX<<16)|AX); res32= tmp32/op16[0];"
                    " AX= res32; DX= tmp32%%op16[0]\n" );
          break;
        case BC_DIV8_AH_AL:
          fprintf ( f, "tmp16= AX; res16= tmp16/op8[0];"
                    " AL= res16; AH= tmp16%%op8[0]\n" );
          break;
        case BC_IDIV32_EDX_EAX:
          fprintf ( f, "tmp64= sign_extend((EDX<<32)|EAX); "
                    "res64= tmp64/sign_extend(op32[0]);"
                    " EAX= res64; EDX= tmp64%%sign_extend(op32[0])\n" );
          break;
        case BC_IDIV16_DX_AX:
          fprintf ( f, "tmp32= sign_extend((DX<<16)|AX); "
                    "res32= tmp32/sign_extend(op16[0]);"
                    " AX= res32; DX= tmp32%%sign_extend(op16[0])\n" );
          break;
        case BC_IDIV8_AH_AL:
          fprintf ( f, "tmp16= sign_extend(AX); "
                    "res16= tmp16/sign_extend(op8[0]);"
                    " AL= res16; AH= tmp16%%sign_extend(op8[0])\n" );
          break;
        case BC_IMUL32:
          fprintf ( f, "res64= sign_extend(op32[0]) * sign_extend(op32[1])\n" );
          break;
        case BC_IMUL16:
          fprintf ( f, "res32= sign_extend(op16[0]) * sign_extend(op16[1])\n" );
          break;
        case BC_IMUL8:
          fprintf ( f, "res16= sign_extend(op8[0]) * sign_extend(op8[1])\n" );
          break;
        case BC_MUL32:
          fprintf ( f, "res64= op32[0] * op32[1]\n" );
          break;
        case BC_MUL16:
          fprintf ( f, "res32= op16[0] * op16[1]\n" );
          break;
        case BC_MUL8:
          fprintf ( f, "res16= op8[0] * op8[1]\n" );
          break;
        case BC_SBB32:
          fprintf ( f, "res32= op32[0] + ((~op32[1])&FFFFFFFFh) + ~CF\n" );
          break;
        case BC_SBB16:
          fprintf ( f, "res16= op16[0] + ((~op16[1])&FFFFh) + ~CF\n" );
          break;
        case BC_SBB8:
          fprintf ( f, "res8= op8[0] + ((~op8[1])&FFh) + ~CF\n" );
          break;
        case BC_SGDT32:
          fprintf ( f, "res16= GDTR.lastb; res32= GDTR.addr\n" );
          break;
        case BC_SIDT32:
          fprintf ( f, "res16= IDTR.lastb; res32= IDTR.addr\n" );
          break;
        case BC_SIDT16:
          fprintf ( f, "res16= IDTR.lastb; res32= IDTR.addr&00FFFFFF\n" );
          break;
        case BC_SUB32: fprintf ( f, "res32= op32[0] - op32[1]\n" ); break;
        case BC_SUB16: fprintf ( f, "res16= op16[0] - op16[1]\n" ); break;
        case BC_SUB8: fprintf ( f, "res8= op8[0] - op8[1]\n" ); break;
        case BC_AND32: fprintf ( f, "res32= op32[0] & op32[1]\n" ); break;
        case BC_AND16: fprintf ( f, "res16= op16[0] & op16[1]\n" ); break;
        case BC_AND8: fprintf ( f, "res8= op8[0] & op8[1]\n" ); break;
        case BC_NOT32: fprintf ( f, "res32= ~(op32[0])\n" ); break;
        case BC_NOT16: fprintf ( f, "res16= ~(op16[0])\n" ); break;
        case BC_NOT8: fprintf ( f, "res8= ~(op8[0])\n" ); break;
        case BC_OR32: fprintf ( f, "res32= op32[0] | op32[1]\n" ); break;
        case BC_OR16: fprintf ( f, "res16= op16[0] | op16[1]\n" ); break;
        case BC_OR8: fprintf ( f, "res8= op8[0] | op8[1]\n" ); break;
        case BC_RCL32:
          fprintf ( f, "if(count==1) "
                    "{res32= (op32[0]<<1)|(CF!=0)} else if(count>0)"
                    "{res32= (op32[0]<<count) | ((CF!=0)<<(count-1)) |"
                    " op32[0]>>(33-count)} else res32= op32[0]\n");
          break;
        case BC_RCL16:
          fprintf ( f, "count%%= 17; if(count==16) "
                    "{res16= ((CF!=0)<<15)|(op16[0]>>1)} else if(count==1) "
                    "{res16= (op16[0]<<1)|(CF!=0)} else if(count>0)"
                    "{res16= (op16[0]<<count) | ((CF!=0)<<(count-1)) |"
                    " op16[0]>>(17-count)} else res16= op16[0]\n");
          break;
        case BC_RCL8:
          fprintf ( f, "count%%= 9; if(count==8) "
                    "{res8= ((CF!=0)<<7)|(op8[0]>>1)} else if(count==1) "
                    "{res8= (op8[0]<<1)|(CF!=0)} else if(count>0)"
                    "{res8= (op8[0]<<count) | ((CF!=0)<<(count-1)) |"
                    " op8[0]>>(9-count)} else res8= op8[0]\n");
          break;
        case BC_RCR32:
          fprintf ( f, "if(count==1) "
                    "{res32= (op32[0]>>1)|((CF!=0)<<31)} else if(count>0)"
                    "{res32= (op32[0]>>count) | ((CF!=0)<<(32-count)) |"
                    " op32[0]<<(33-count)} else res32= op32[0]\n");
          break;
        case BC_RCR16:
          fprintf ( f, "count%%= 17; if(count==16) "
                    "{res16= (CF!=0)|(op16[0]<<1)} else if(count==1) "
                    "{res16= (op16[0]>>1)|((CF!=0)<<15)} else if(count>0)"
                    "{res16= (op16[0]>>count) | ((CF!=0)<<(16-count)) |"
                    " op16[0]<<(17-count)} else res16= op16[0]\n");
          break;
        case BC_RCR8:
          fprintf ( f, "count%%= 9; if(count==8) "
                    "{res8= (CF!=0)|(op8[0]<<1)} else if(count==1) "
                    "{res8= (op8[0]>>1)|((CF!=0)<<7)} else "
                    "{res8= (op8[0]>>count) | ((CF!=0)<<(8-count)) |"
                    " op8[0]<<(9-count)} else res8= op8[0]\n");
          break;
        case BC_ROL32:
          fprintf ( f, "count%%= 32; if(count>0){res32= "
                    "(op32[0]<<count)|(op32[0]>>(32-count))} "
                    "else {res32= op32[0]\n}" );
          break;
        case BC_ROL16:
          fprintf ( f, "count%%= 16; if(count>0){res16= "
                    "(op16[0]<<count)|(op16[0]>>(16-count))} "
                    "else {res16= op16[0]\n}" );
          break;
        case BC_ROL8:
          fprintf ( f, "count%%= 8; if(count>0){res8= "
                    "(op8[0]<<count)|(op8[0]>>(8-count))} "
                    "else {res8= op8[0]\n}" );
          break;
        case BC_ROR32:
          fprintf ( f, "if(count>0){res32= "
                    "(op32[0]<<(32-count))|(op32[0]>>count)} "
                    "else {res32= op32[0]\n}" );
          break;
        case BC_ROR16:
          fprintf ( f, "count%%= 16; if(count>0){res16= "
                    "(op16[0]<<(16-count))|(op16[0]>>count)} "
                    "else {res16= op16[0]\n}" );
          break;
        case BC_ROR8:
          fprintf ( f, "count%%= 8; if(count>0){res8= "
                    "(op8[0]<<(8-count))|(op8[0]>>count)} "
                    "else {res8= op8[0]\n}" );
          break;
        case BC_SAR32:
          fprintf ( f, "if(count==32) {res32= (op32[0]&80000000h)?FFFFFFFFh:0}"
                    " else if(count>0){res32= (op32[0]>>count)} "
                    "else {res32= op32[0]}\n" );
          break;
        case BC_SAR16:
          fprintf ( f, "if(count>=16) {res16= (op16[0]&8000h)?FFFFh:0}"
                    " else if(count>0){res16= (op16[0]>>count)} "
                    "else {res16= op16[0]}\n" );
          break;
        case BC_SAR8:
          fprintf ( f, "if(count>=8) {res8= (op8[0]&80h)?FFh:0}"
                    " else if(count>0){res8= (op8[0]>>count)} "
                    "else {res8= op8[0]}\n" );
          break;
        case BC_SHL32:
          fprintf ( f, "if(count>0){res32= "
                    "(count==32) ? 0 : (op32[0]<<count)}"
                    " else {res32= op32[0]}\n" );
          break;
        case BC_SHL16:
          fprintf ( f, "if(count>0){res16= "
                    "(count>=16) ? 0 : (op16[0]<<count)}"
                    " else {res16= op16[0]}\n" );
          break;
        case BC_SHL8:
          fprintf ( f, "if(count>0){res8= "
                    "(count>=8) ? 0 : (op8[0]<<count)}"
                    " else {res8= op8[0]}\n" );
          break;
        case BC_SHLD32:
          fprintf ( f, "if(count>0 && count<=32){res32= "
                    "(count==32) ? op32[1] : ((op32[0]<<count) |"
                    " (op32[1]>>(32-count)))}"
                    " else {res32= op32[0]}\n" );
          break;
        case BC_SHLD16:
          fprintf ( f, "if(count>0 && count<=16){res16= "
                    "(count==16) ? op16[1] : ((op16[0]<<count) |"
                    " (op16[1]>>(16-count)))}"
                    " else {res16= op16[0]}\n" );
          break;
        case BC_SHR32:
          fprintf ( f, "if(count>0){res32= (count==32) ? 0 :"
                    " (op32[0]>>count)} else {res32= op32[0]}\n" );
          break;
        case BC_SHR16:
          fprintf ( f, "if(count>0){res16= (count>=16) ? 0 :"
                    " (op16[0]>>count)} else {res16=op16[0]}\n" );
          break;
        case BC_SHR8:
          fprintf ( f, "if(count>0){res8= (count>=8) ? 0 :"
                    " (op8[0]>>count)} else {res8= op8[0]}\n" );
          break;
        case BC_SHRD32:
          fprintf ( f, "if(count>0 && count<=32){res32= (count==32) ? op32[1] :"
                    " ((op32[0]>>count)|(op32[1]<<(32-count)))} else"
                    " {res32= op32[0]}\n" );
          break;
        case BC_SHRD16:
          fprintf ( f, "if(count>0 && count<=16){res16= (count==16) ? op16[1] :"
                    " ((op16[0]>>count)|(op16[1]<<(16-count)))} else"
                    " {res16= op16[0]}\n" );
          break;
        case BC_XOR32: fprintf ( f, "res32= op32[0] ^ op32[1]\n" ); break;
        case BC_XOR16: fprintf ( f, "res16= op16[0] ^ op16[1]\n" ); break;
        case BC_XOR8: fprintf ( f, "res8= op8[0] ^ op8[1]\n" ); break;
        case BC_AAD:
          fprintf ( f, "AL= (AL+AH*%d)&0xFF; AH= 0\n", p->v[n+1] );
          ++n;
          break;
        case BC_AAM:
          fprintf ( f, "tmp8= AL; AH= tmp8/%d; res8= AL= tmp8%%%d\n",
                    p->v[n+1], p->v[n+1] );
          ++n;
          break;
        case BC_BSF32:
          fprintf ( f, "res32= bit_scan_forward(op32[1])\n" );
          break;
        case BC_BSF16:
          fprintf ( f, "res16= bit_scan_forward(op16[1])\n" );
          break;
        case BC_BSR32:
          fprintf ( f, "res32= bit_scan_reverse(op32[1])\n" );
          break;
        case BC_BSR16:
          fprintf ( f, "res16= bit_scan_reverse(op16[1])\n" );
          break;
        case BC_BSWAP:
          fprintf ( f, "res32= byte_swap(op32[0])\n" );
          break;
        case BC_BT32:
          fprintf ( f, "CF <-- op32[0]&(1<<(op32[1]%%32))\n" );
          break;
        case BC_BT16:
          fprintf ( f, "CF <-- op16[0]&(1<<(op16[1]%%16))\n" );
          break;
        case BC_BTC32:
          fprintf ( f, "CF <-- op32[0]&(1<<(op32[1]%%32));"
                    " res32= op32[0]^(1<<(op32[1]%%32));\n" );
          break;
        case BC_BTR32:
          fprintf ( f, "CF <-- op32[0]&(1<<(op32[1]%%32));"
                    " res32= op32[0]&(~(1<<(op32[1]%%32)));\n" );
          break;
        case BC_BTR16:
          fprintf ( f, "CF <-- op16[0]&(1<<(op16[1]%%16));"
                    " res16= op16[0]&(~(1<<(op16[1]%%16)));\n" );
          break;
        case BC_BTS32:
          fprintf ( f, "CF <-- op32[0]&(1<<(op32[1]%%32));"
                    " res32= op32[0]|(1<<(op32[1]%%32));\n" );
          break;
        case BC_BTS16:
          fprintf ( f, "CF <-- op16[0]&(1<<(op16[1]%%16));"
                    " res16= op16[0]|(1<<(op16[1]%%16));\n" );
          break;
        case BC_CDQ:
          fprintf ( f, "EDX= (EAX&80000000h)!=0 ? FFFFFFFFh : 00000000h\n" );
          break;
        case BC_CLI: fprintf ( f, "cli()\n" ); break;
        case BC_CMC: fprintf ( f, "CF <-- !CF\n"); break;
        case BC_CPUID: fprintf ( f, "cpuid()\n" ); break;
        case BC_CWD:
          fprintf ( f, "DX= (AX&8000h)!=0 ? FFFFh : 0000h\n" );
          break;
        case BC_CWDE:
          fprintf ( f, "EAX= sign_extend(AX)\n" );
          break;
        case BC_DAA: fprintf ( f, "AL= daa(AL)\n" ); break;
        case BC_DAS: fprintf ( f, "AL= das(AL)\n" ); break;
        case BC_ENTER16:
          fprintf ( f, "enter16(alloc_size=%d,nesting_level=%d)\n",
                    p->v[n+1], p->v[n+2] );
          n+= 2;
          break;
        case BC_ENTER32:
          fprintf ( f, "enter32(alloc_size=%d,nesting_level=%d)\n",
                    p->v[n+1], p->v[n+2] );
          n+= 2;
          break;
        case BC_HALT:
          fprintf ( f, "if ( !halt() ) { EIP-= %d } // Stop !\n", p->v[n+1] );
          n++;
          break;
        case BC_INT32:
          fprintf ( f, "interruption32(%02x,old_EIP=EIP+%d,"
                    "type=IMM) // Stop !\n",
                    p->v[n+2], p->v[n+1] );
          n+= 2;
          break;
        case BC_INT16:
          fprintf ( f, "interruption16(%02x,old_EIP=EIP+%d,"
                    "type=INTO) // Stop !\n",
                    p->v[n+2], p->v[n+1] );
          n+= 2;
          break;
        case BC_INTO32:
          fprintf ( f, "if(OF==1){ interruption32(4,old_EIP=EIP+%d,"
                    "type=INTO) } // Stop !\n",
                    p->v[n+1] );
          n++;
          break;
        case BC_INTO16:
          fprintf ( f, "if(OF==1){ interruption16(4,old_EIP=EIP+%d,"
                    "type=IMM) } // Stop !\n",
                    p->v[n+1] );
          n++;
          break;
        case BC_INVLPG32: fprintf ( f, "Invalidate32(RelevantTLB)\n" ); break;
        case BC_INVLPG16: fprintf ( f, "Invalidate16(RelevantTLB)\n" ); break;
        case BC_LAHF: fprintf ( f, "AH <-- {SF,ZF,AF,PF,CF}|0x02\n" ); break;
        case BC_LAR32:
          fprintf ( f, "res32,EFLAGS= "
                    "load_access_rights_byte(selector);\n" );
          break;
        case BC_LAR16:
          fprintf ( f, "res16,EFLAGS= "
                    "load_access_rights_byte(selector);\n" );
          break;
        case BC_LEAVE32: fprintf ( f, "(E)SP= (E)BP; EBP= pop32()\n" ); break;
        case BC_LEAVE16: fprintf ( f, "(E)SP= (E)BP; BP= pop16()\n" ); break;
        case BC_LGDT32:
          fprintf ( f, "GDTR.addr= offset; GDTR.firstb= 0;"
                    " GDTR.lastb= selector\n");
          break;
        case BC_LGDT16:
          fprintf ( f, "GDTR.addr= offset&FFFFFFh; GDTR.firstb= 0;"
                    " GDTR.lastb= selector\n");
          break;
        case BC_LIDT32:
          fprintf ( f, "IDTR.addr= offset; IDTR.firstb= 0;"
                    " IDTR.lastb= selector\n");
          break;
        case BC_LIDT16:
          fprintf ( f, "IDTR.addr= offset&FFFFFFh; IDTR.firstb= 0;"
                    " IDTR.lastb= selector\n");
          break;
        case BC_LLDT:
          fprintf ( f, "lldt(selector= res16)\n" );
          break;
        case BC_LSL32:
          fprintf ( f, "res32,EFLAGS= "
                    "load_segment_limit(selector);\n" );
          break;
        case BC_LSL16:
          fprintf ( f, "res16,EFLAGS= "
                    "load_segment_limit(selector);\n" );
          break;
        case BC_LTR:
          fprintf ( f, "ltr(selector= res16)\n" );
          break;
        case BC_POP32: fprintf ( f, "res32= pop32()\n" ); break;
        case BC_POP16: fprintf ( f, "res16= pop16()\n" ); break;
        case BC_POP32_EFLAGS: fprintf ( f, "EFLAGS= pop32()\n" ); break;
        case BC_POP16_EFLAGS: fprintf ( f, "EFLAGS= pop16()\n" ); break;
        case BC_PUSH32: fprintf ( f, "push32(res32)\n" ); break;
        case BC_PUSH16: fprintf ( f, "push16(res16)\n" ); break;
        case BC_PUSH32_EFLAGS:
          fprintf ( f, "push32(EFLAGS&00FCFFFFh)\n" );
          break;
        case BC_PUSH16_EFLAGS:
          fprintf ( f, "push16(EFLAGS&FFFFh)\n" );
          break;
        case BC_PUSHA32_CHECK_EXCEPTION:
          fprintf ( f,
                    "if(!PROTECTED_MODE && "
                    "(ESP==7||ESP==9||ESP==11||"
                    "ESP==13||ESP==15)){exception();}\n" );
          break;
        case BC_PUSHA16_CHECK_EXCEPTION:
          fprintf ( f,
                    "if(!PROTECTED_MODE && "
                    "(SP==7||SP==9||SP==11||SP==13||SP==15)){exception();}\n" );
          break;
        case BC_RET32_FAR:
          fprintf ( f, "ret32_far() // Stop !\n" );
          break;
        case BC_RET32_FAR_NOSTOP:
          fprintf ( f, "ret32_far()\n" );
          break;
        case BC_RET16_FAR:
          fprintf ( f, "ret16_far() // Stop !\n" );
          break;
        case BC_RET16_FAR_NOSTOP:
          fprintf ( f, "ret16_far()\n" );
          break;
        case BC_RET32_RES:
          fprintf ( f, "EIP= res32; goto EIP // Stop !\n" );
          break;
        case BC_RET32_RES_INC_ESP:
          fprintf ( f, "EIP= res32; ESP+= %d; goto EIP // Stop !\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_RET32_RES_INC_SP:
          fprintf ( f, "EIP= res32; SP+= %d; goto EIP // Stop !\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_RET16_RES:
          fprintf ( f, "EIP= res16; goto EIP // Stop !\n" );
          break;
        case BC_RET16_RES_INC_ESP:
          fprintf ( f, "EIP= res16; ESP+= %d; goto EIP // Stop !\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_RET16_RES_INC_SP:
          fprintf ( f, "EIP= res16; SP+= %d; goto EIP // Stop !\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_INC_ESP_AND_GOTO_EIP:
          fprintf ( f, "ESP+= %d; goto EIP // Stop !\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_INC_SP_AND_GOTO_EIP:
          fprintf ( f, "SP+= %d; goto EIP // Stop !\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_SAHF: fprintf ( f, "SF,ZF,AF,PF,CF <-- AH\n" ); break;
        case BC_STI: fprintf ( f, "sti()\n" ); break;
        case BC_VERR:
          fprintf ( f, "ZF <-- verify_segment_reading(selector= res16)\n" );
          break;
        case BC_VERW:
          fprintf ( f, "ZF <-- verify_segment_writing(selector= res16)\n" );
          break;
        case BC_WBINVD: fprintf ( f, "stwbinvd()\n" ); break;
        case BC_CMPS32_ADDR32:
          fprintf ( f, "res32= op32[0] - READ32(ES:EDI); "
                    "if(DF) {ESI-=4;EDI-=4} else {ESI+=4;EDI+=4}\n");
          break;
        case BC_CMPS8_ADDR32:
          fprintf ( f, "res8= op8[0] - READ8(ES:EDI); "
                    "if(DF) {--ESI;--EDI} else {++ESI;++EDI}\n");
          break;
        case BC_CMPS32_ADDR16:
          fprintf ( f, "res32= op32[0] - READ32(ES:DI); "
                    "if(DF) {SI-=4;DI-=4} else {SI+=4;DI+=4}\n");
          break;
        case BC_CMPS16_ADDR16:
          fprintf ( f, "res16= op16[0] - READ16(ES:DI); "
                    "if(DF) {SI-=2;DI-=2} else {SI+=2;DI+=2}\n");
          break;
        case BC_CMPS8_ADDR16:
          fprintf ( f, "res8= op8[0] - READ8(ES:DI); "
                    "if(DF) {--SI;--DI} else {++SI;++DI}\n");
          break;
        case BC_INS16_ADDR32:
          fprintf ( f, "WRITE16(ES:EDI,PORT_READ16(DX));"
                    " if(DF) {EDI-=2} else {EDI+=2}\n" );
          break;
        case BC_INS8_ADDR32:
          fprintf ( f, "WRITE8(ES:EDI,PORT_READ8(DX));"
                    " if(DF) {--EDI} else {++EDI}\n" );
          break;
        case BC_INS32_ADDR16:
          fprintf ( f, "WRITE32(ES:DI,PORT_READ32(DX));"
                    " if(DF) {DI-=4} else {DI+=4}\n" );
          break;
        case BC_INS16_ADDR16:
          fprintf ( f, "WRITE16(ES:DI,PORT_READ16(DX));"
                    " if(DF) {DI-=2} else {DI+=2}\n" );
          break;
        case BC_LODS32_ADDR32:
          fprintf ( f, "EAX= res32; if(DF) {ESI-=4} else {ESI+=4}\n" );
          break;
        case BC_LODS16_ADDR32:
          fprintf ( f, "AX= res16; if(DF) {ESI-=2} else {ESI+=2}\n" );
          break;
        case BC_LODS32_ADDR16:
          fprintf ( f, "EAX= res32; if(DF) {SI-=4} else {SI+=4}\n" );
          break;
        case BC_LODS16_ADDR16:
          fprintf ( f, "AX= res16; if(DF) {SI-=2} else {SI+=2}\n" );
          break;
        case BC_LODS8_ADDR32:
          fprintf ( f, "AL= res8; if(DF) {--ESI} else {++ESI}\n" );
          break;
        case BC_LODS8_ADDR16:
          fprintf ( f, "AL= res8; if(DF) {--SI} else {++SI}\n" );
          break;
        case BC_MOVS32_ADDR32:
          fprintf ( f, "WRITE32(ES:EDI,res32);"
                    " if(DF) {ESI-=4;EDI-=4} else {ESI+=4;EDI+=4}\n" );
          break;
        case BC_MOVS16_ADDR32:
          fprintf ( f, "WRITE16(ES:EDI,res16);"
                    " if(DF) {ESI-=2;EDI-=2} else {ESI+=2;EDI+=2}\n" );
          break;
        case BC_MOVS8_ADDR32:
          fprintf ( f, "WRITE8(ES:EDI,res8);"
                    " if(DF) {--ESI;--EDI} else {++ESI;++EDI}\n" );
          break;
        case BC_MOVS32_ADDR16:
          fprintf ( f, "WRITE32(ES:DI,res32);"
                    " if(DF) {SI-=4;DI-=4} else {SI+=4;DI+=4}\n" );
          break;
        case BC_MOVS16_ADDR16:
          fprintf ( f, "WRITE16(ES:DI,res16);"
                    " if(DF) {SI-=2;DI-=2} else {SI+=2;DI+=2}\n" );
          break;
        case BC_MOVS8_ADDR16:
          fprintf ( f, "WRITE8(ES:DI,res8);"
                    " if(DF) {--SI;--DI} else {++SI;++DI}\n" );
          break;
        case BC_OUTS16_ADDR32:
          fprintf ( f, "PORT_WRITE16(DX,res16);"
                    " if(DF) {ESI-=2} else {ESI+=2}\n" );
          break;
        case BC_OUTS8_ADDR32:
          fprintf ( f, "PORT_WRITE8(DX,res8);"
                    " if(DF) {--ESI} else {++ESI}\n" );
          break;
        case BC_OUTS16_ADDR16:
          fprintf ( f, "PORT_WRITE16(DX,res16);"
                    " if(DF) {SI-=2} else {SI+=2}\n" );
          break;
        case BC_OUTS8_ADDR16:
          fprintf ( f, "PORT_WRITE8(DX,res8);"
                    " if(DF) {--SI} else {++SI}\n" );
          break;
        case BC_SCAS32_ADDR32:
          fprintf ( f, "res32= EAX - READ32(ES:EDI); "
                    "if(DF) {EDI-=4} else {EDI+=4}\n");
          break;
        case BC_SCAS16_ADDR32:
          fprintf ( f, "res16= AX - READ16(ES:EDI); "
                    "if(DF) {EDI-=2} else {EDI+=2}\n");
          break;
        case BC_SCAS8_ADDR32:
          fprintf ( f, "res8= AL - READ8(ES:EDI); "
                    "if(DF) {--EDI} else {++EDI}\n");
          break;
        case BC_SCAS32_ADDR16:
          fprintf ( f, "res32= EAX - READ32(ES:DI); "
                    "if(DF) {DI-=4} else {DI+=4}\n");
          break;
        case BC_SCAS16_ADDR16:
          fprintf ( f, "res16= AX - READ16(ES:DI); "
                    "if(DF) {DI-=2} else {DI+=2}\n");
          break;
        case BC_SCAS8_ADDR16:
          fprintf ( f, "res8= AL - READ8(ES:DI); "
                    "if(DF) {--DI} else {++DI}\n");
          break;
        case BC_STOS32_ADDR32:
          fprintf ( f, "WRITE32(ES:EDI,EAX);"
                    " if(DF) {EDI-=4} else {EDI+=4}\n" );
          break;
        case BC_STOS32_ADDR16:
          fprintf ( f, "WRITE32(ES:DI,EAX);"
                    " if(DF) {DI-=4} else {DI+=4}\n" );
          break;
        case BC_STOS16_ADDR32:
          fprintf ( f, "WRITE16(ES:EDI,AX);"
                    " if(DF) {EDI-=2} else {EDI+=2}\n" );
          break;
        case BC_STOS16_ADDR16:
          fprintf ( f, "WRITE16(ES:DI,AX);"
                    " if(DF) {DI-=2} else {DI+=2}\n" );
          break;
        case BC_STOS8_ADDR32:
          fprintf ( f, "WRITE8(ES:EDI,AL);"
                    " if(DF) {--EDI} else {++EDI}\n" );
          break;
        case BC_STOS8_ADDR16:
          fprintf ( f, "WRITE8(ES:DI,AL);"
                    " if(DF) {--DI} else {++DI}\n" );
          break;
        case BC_FPU_DS_FLOATU32_WRITE32:
          fprintf ( f, "if(fpu_ok) {WRITE32(DS:offset,float_u32)\n" );
          break;
        case BC_FPU_SS_FLOATU32_WRITE32:
          fprintf ( f, "if(fpu_ok) {WRITE32(SS:offset,float_u32)\n" );
          break;
        case BC_FPU_ES_FLOATU32_WRITE32:
          fprintf ( f, "if(fpu_ok) {WRITE32(ES:offset,float_u32)\n" );
          break;
        case BC_FPU_DS_DOUBLEU64_WRITE:
          fprintf ( f, "if(fpu_ok) {WRITE(DS:offset,double_u64)\n" );
          break;
        case BC_FPU_SS_DOUBLEU64_WRITE:
          fprintf ( f, "if(fpu_ok) {WRITE(SS:offset,double_u64)\n" );
          break;
        case BC_FPU_DS_LDOUBLEU80_WRITE:
          fprintf ( f, "if(fpu_ok) {WRITE(DS:offset,ldouble_u80)\n" );
          break;
        case BC_FPU_SS_LDOUBLEU80_WRITE:
          fprintf ( f, "if(fpu_ok) {WRITE(SS:offset,ldouble_u80)\n" );
          break;
        case BC_FPU_2XM1:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)= "
                    "2^FPU_REG(fpu_reg) - 1}\n" );
          break;
        case BC_FPU_ABS:
          fprintf ( f,
                    "if(fpu_ok) {FPU_REG(fpu_reg)= abs(FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_ADD:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)+= ldouble}\n" );
          break;
        case BC_FPU_BEGIN_OP:
          fprintf ( f, "fpu_begin_op(check=true)\n" );
          break;
        case BC_FPU_BEGIN_OP_CLEAR_C1:
          fprintf ( f, "fpu_begin_op(check=true); C1 <-- 0\n" );
          break;
        case BC_FPU_BEGIN_OP_NOCHECK:
          fprintf ( f, "fpu_begin_op(check=false)\n" );
          break;
        case BC_FPU_BSTP_SS:
          fprintf ( f, "StoreBCDIntegerAndPop(fpu_reg,SS,offset)\n" );
          break;
        case BC_FPU_CHS: fprintf ( f, "C0 <-- 0; ST(0) <-- -ST(0)\n" ); break;
        case BC_FPU_CLEAR_EXCP: fprintf ( f, "fpu_clear_excp()\n" ); break;
        case BC_FPU_COM:
          fprintf ( f, "if(fpu_ok) {C3,C2,C0 <-- "
                    "compare(FPU_REG(fpu_reg),ldouble)}\n" );
          break;
        case BC_FPU_COS:
          fprintf ( f, "if(fpu_ok) {ST(0) <-- cos(ST(0))}\n" );
          break;
        case BC_FPU_DIV:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)/= ldouble}\n" );
          break;
        case BC_FPU_DIVR:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)= ldouble"
                    " / FPU_REG(fpu_reg)}\n" );
          break;
        case BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK:
          fprintf ( f, "if(fpu_ok) {fpu_ok= check_invalid(double_u64);"
                    " if(fpu_ok) {ldouble= double_u64}}\n" );
          break;
        case BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_DENORMAL_SNAN:
          fprintf ( f, "if(fpu_ok) {fpu_ok= !check_denormal(double_u64);"
                    " if(fpu_ok) {fpu_ok= !check_snan(double_u64);"
                    " if(fpu_ok) {ldouble= double_u64}}\n" );
          break;
        case BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_FCOM:
          fprintf ( f, "if(fpu_ok) {fpu_ok= fcom_check_invalid(double_u64);"
                    " if(fpu_ok) {ldouble= double_u64}}\n" );
          break;
        case BC_FPU_FLOATU32_LDOUBLE_AND_CHECK:
          fprintf ( f, "if(fpu_ok) {fpu_ok= check_invalid(float_u32);"
                    " if(fpu_ok) {ldouble= float_u32}}\n" );
          break;
        case BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_DENORMAL_SNAN:
          fprintf ( f, "if(fpu_ok) {fpu_ok= !check_denormal(float_u32);"
                    " if(fpu_ok) {fpu_ok= !check_snan(float_u32);"
                    " if(fpu_ok) {ldouble= float_u32}}\n" );
          break;
        case BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_FCOM:
          fprintf ( f, "if(fpu_ok) {fpu_ok= fcom_check_invalid(float_u32);"
                    " if(fpu_ok) {ldouble= float_u32}}\n" );
          break;
        case BC_FPU_FREE:
          fprintf ( f, "if(fpu_ok) {free(ST(%d))}\n", p->v[n+1] );
          ++n;
          break;
        case BC_FPU_INIT: fprintf ( f, "fpu_init()\n"); break;
        case BC_FPU_MUL:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)*= ldouble}\n" );
          break;
        case BC_FPU_PATAN:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg2)= "
                    "atan(FPU_REG(fpu_reg2)/FPU_REG(fpu_reg))};\n" );
          break;
        case BC_FPU_POP:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg) <-- EMPTY;FPU_TOP++}\n" );
          break;
        case BC_FPU_PREM:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg2)= "
                    "partial_remainder(FPU_REG(fpu_reg),"
                    "FPU_REG(fpu_reg2))};\n" );
          break;
        case BC_FPU_PTAN:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)= tan(FPU_REG(fpu_reg));"
                    "FPU_TOP--;ST(0) <-- 1.0}\n" );
          break;
        case BC_FPU_PUSH_DOUBLEU64:
          fprintf ( f, "if(fpu_ok) {ST() <-- double_u64}\n" );
          break;
        case BC_FPU_PUSH_DOUBLEU64_AS_INT64:
          fprintf ( f, "if(fpu_ok) {ST() <-- (int64_t) double_u64}\n" );
          break;
        case BC_FPU_PUSH_FLOATU32:
          fprintf ( f, "if(fpu_ok) {ST() <-- float_u32}\n" );
          break;
        case BC_FPU_PUSH_L2E:
          fprintf ( f, "if(fpu_ok) {ST() <-- log2(e)}\n" );
          break;
        case BC_FPU_PUSH_LDOUBLEU80:
          fprintf ( f, "if(fpu_ok) {ST() <-- ldouble_u80}\n" );
          break;
        case BC_FPU_PUSH_LN2:
          fprintf ( f, "if(fpu_ok) {ST() <-- log(2.0)}\n" );
          break;
        case BC_FPU_PUSH_ONE:
          fprintf ( f, "if(fpu_ok) {ST() <-- +1.0}\n" );
          break;
        case BC_FPU_PUSH_REG2:
          fprintf ( f, "if(fpu_ok) {ST() <-- FPU_REG(fpu_reg2)}\n" );
          break;
        case BC_FPU_PUSH_RES16:
          fprintf ( f, "if(fpu_ok) {ST() <-- (int16_t) res16}\n" );
          break;
        case BC_FPU_PUSH_RES32:
          fprintf ( f, "if(fpu_ok) {ST() <-- (int32_t) res32}\n" );
          break;
        case BC_FPU_PUSH_ZERO:
          fprintf ( f, "if(fpu_ok) {ST() <-- +0.0}\n" );
          break;
        case BC_FPU_REG_ST:
          fprintf ( f, "if(fpu_ok) {ST(%d)= FPU_REG(fpu_reg)}\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_FPU_REG_INT16:
          fprintf ( f, "if(fpu_ok) {res16= (uint16_t) "
                    "((int16_t) FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_REG_INT32:
          fprintf ( f, "if(fpu_ok) {float_u32.u32= (uint32_t) "
                    "((int32_t) FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_REG_INT64:
          fprintf ( f, "if(fpu_ok) {double_u64.u64= (uint64_t) "
                    "((int64_t) FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_REG_FLOATU32:
          fprintf ( f, "if(fpu_ok) {float_u32= (float) FPU_REG(fpu_reg)}\n" );
          break;
        case BC_FPU_REG_DOUBLEU64:
          fprintf ( f, "if(fpu_ok) {double_u64= (double) FPU_REG(fpu_reg)}\n" );
          break;
        case BC_FPU_REG_LDOUBLEU80:
          fprintf ( f, "if(fpu_ok) {ldouble_u80= FPU_REG(fpu_reg)}\n" );
          break;
        case BC_FPU_REG2_LDOUBLE_AND_CHECK:
          fprintf ( f, "if(fpu_ok) {fpu_ok= check_invalid(fpu_reg2);"
                    " if(fpu_ok) {ldouble= FPU_REG(fpu_reg2)}}\n" );
          break;
        case BC_FPU_REG2_LDOUBLE_AND_CHECK_FCOM:
          fprintf ( f, "if(fpu_ok) {fpu_ok= fcom_check_invalid(fpu_reg2);"
                    " if(fpu_ok) {ldouble= FPU_REG(fpu_reg2)}}\n" );
          break;
        case BC_FPU_RES16_CONTROL_WORD:
          fprintf ( f, "FPU_CONTROL_WORD= res16; update_fpu_exp()\n" );
          break;
        case BC_FPU_RNDINT:
          fprintf ( f,
                    "if(fpu_ok) {FPU_REG(fpu_reg)= int(FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_RSTOR32_DS:
          fprintf ( f, "if(fpu_ok) {fpu_restore_state_32bit(DS:offset)}\n" );
          break;
        case BC_FPU_SAVE32_DS:
          fprintf ( f, "if(fpu_ok) {fpu_save_state_32bit(DS:offset)}\n" );
          break;
        case BC_FPU_SCALE:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)*= "
                    "2^FPU_REG(fpu_reg2)}\n" );
          break;
        case BC_FPU_SELECT_ST0:
          fprintf ( f, "fpu_reg= FPU_TOP; fpu_ok= check_stack_underflow()\n" );
          break;
        case BC_FPU_SELECT_ST1:
          fprintf ( f,
                    "if(fpu_ok){fpu_reg2= FPU_TOP+1; "
                    "fpu_ok= check_stack_underflow();}\n" );
          break;
        case BC_FPU_SELECT_ST_REG:
          fprintf ( f, "if(fpu_ok) {fpu_reg= FPU_TOP+%d; "
                    "fpu_ok= check_stack_underflow()}\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_FPU_SELECT_ST_REG2:
          fprintf ( f, "if(fpu_ok) {fpu_reg2= FPU_TOP+%d; "
                    "fpu_ok= check_stack_underflow()}\n",
                    p->v[n+1] );
          ++n;
          break;
        case BC_FPU_SIN:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)="
                    " sin(FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_SQRT:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)= "
                    "sqrt(FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_SUB:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)-= ldouble}\n" );
          break;
        case BC_FPU_SUBR:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg)="
                    " ldouble - FPU_REG(fpu_reg)}\n" );
          break;
        case BC_FPU_TST:
          fprintf ( f, "if(fpu_ok) {C3,C2,C0 <-- tst(FPU_REG(fpu_reg))}\n" );
          break;
        case BC_FPU_WAIT: fprintf ( f, "fpu_wait()\n"); break;
        case BC_FPU_XAM:
          fprintf ( f, "if(fpu_ok) {C0,C2,C3 <-- ExamineModRM(fpu_reg);}\n" );
          break;
        case BC_FPU_XCH:
          fprintf ( f, "if(fpu_ok) {tmp_fpu_reg= FPU_REG(fpu_reg); "
                    "FPU_REG(fpu_reg)= FPU_REG(fpu_reg2); FPU_REG(fpu_reg2)= "
                    "tmp_fpu_reg;}\n" );
          break;
        case BC_FPU_YL2X:
          fprintf ( f, "if(fpu_ok) {FPU_REG(fpu_reg2)*= "
                    "log2(FPU_REG(fpu_reg))};\n" );
          break;
          
        default:
          fprintf ( stderr,
                    "[EE] jit.c - print_page: bytecode desconegut %d\n",
                    p->v[n] );
          exit ( EXIT_FAILURE );
        }
    }
  
  fflush ( f );
  
} // end print_page


static void
print_pages (
             FILE           *f,
             const IA32_JIT *jit
             )
{

  int i;
  const IA32_JIT_MemMap *mem_map;
  uint32_t page,page_e;
  
  
  for ( i= 0; i < jit->_mem_map_size; ++i )
    {
      mem_map= &(jit->_mem_map[i]);
      page_e= (mem_map->last_addr-mem_map->first_addr)>>jit->_bits_page;
      for ( page= 0; page <= page_e; ++page )
        if ( mem_map->map[page] != NULL )
          {
            fprintf ( f, "Pàgina %08Xh\n",
                      (page<<jit->_bits_page)+mem_map->first_addr );
            fprintf ( f, "----------------\n");
            print_page ( f, mem_map->map[page] );
            fprintf ( f, "\n" );
            fflush ( f );
          }
    }
  
} // end print_pages


#if 0
// NOTA!!! Açò de moment no es gasta, però em permetria ficar un
// número màxim de pàgines actives.
static void
move_page_to_head (
                   IA32_JIT      *jit,
                   IA32_JIT_Page *page
                   )
{

  if ( page != jit->_pages )
    {

      assert ( jit->_pages != NULL );
      
      // Lleva de la llista.
      if ( page->next != NULL ) page->next->prev= page->prev;
      else                      jit->_pages_tail= page->prev;
      assert ( page->prev != NULL );
      page->prev->next= page->next;
      
      // Torna a ficar en la llista.
      page->next= jit->_pages;
      assert ( page->next != NULL );
      page->next->prev= page;
      page->prev= NULL;
      jit->_pages= page;
      
    }
  
} // end move_page_to_head
#endif

// NOTA!! Si vullguera ficar un màxim ací podria fer que si s'ha
// aplegat al màxim en compte de crear una nova es reciclara la més
// vella.
// Torna una pàgina buida i la fica en el cap de la llista pages
static IA32_JIT_Page *
get_new_page (
              IA32_JIT *jit
              )
{

  IA32_JIT_Page *ret;
  uint32_t n;
  
  
  // La agafa de _free_pages
  if ( jit->_free_pages != NULL )
    {

      // Lleva de _free_pages
      ret= jit->_free_pages;
      jit->_free_pages= ret->next;
      
      // Neteja valors antics
      for ( n= ret->first_entry; n <= ret->last_entry; ++n )
        ret->entries[n].ind= NULL_ENTRY;
      ret->first_entry= (uint32_t) -1;
      ret->last_entry= 0;
      ret->overlap_next_page= 0;
      ret->N= 2; // les 2 primeres estan reservades
      
    }
  
  // Crea una nova
  else
    {
      ret= (IA32_JIT_Page *) malloc__ ( sizeof(IA32_JIT_Page) );
      ret->entries=
        (IA32_JIT_PageEntry *) malloc__ ( sizeof(IA32_JIT_PageEntry)*
                                          (jit->_page_low_mask+1) );
      for ( n= 0; n <= jit->_page_low_mask; ++n )
        ret->entries[n].ind= NULL_ENTRY;
      ret->first_entry= (uint32_t) -1;
      ret->last_entry= 0;
      ret->overlap_next_page= 0;
      ret->v= (uint16_t *) malloc__ ( sizeof(uint16_t)*2 );
      ret->capacity= 2;
      ret->N= 2;
      ret->area_id= -1;
      ret->page_id= (uint32_t) -1;
    }

  // Inserta en en el cap de _pages
  ret->prev= NULL;
  if ( jit->_pages != NULL )
    {
      ret->next= jit->_pages;
      ret->next->prev= ret;
      jit->_pages= ret;
    }
  else
    {
      ret->next= NULL;
      jit->_pages= jit->_pages_tail= ret;
    }
  
  return ret;
  
} // end get_new_page


static void
remove_page (
             IA32_JIT       *jit,
             const int       area,
             const uint32_t  page
             )
{

  IA32_JIT_Page *p;

  
  //printf("REMOVE_PAGE area:%d page:%X!!!!\n",area,page);
  // Lleva
  p= jit->_mem_map[area].map[page];
  assert ( p != NULL );
  jit->_mem_map[area].map[page]= NULL;

  // lleva de current si és el cas.
  if ( jit->_current_page == p )
    {
      jit->_current_page= NULL;
      jit->_current_pos= 0; // No pot ser 0
    }

  // Lleva de pages
  if ( p->next != NULL ) p->next->prev= p->prev;
  else                   jit->_pages_tail= p->prev;
  if ( p->prev != NULL ) p->prev->next= p->next;
  else                   jit->_pages= p->next;
  
  // Afegeix a _free_pages
  if ( jit->_lock_page != p )
    {
      p->next= jit->_free_pages;
      jit->_free_pages= p;
      p->prev= NULL;
    }
  else jit->_free_lock_page= true;
  
} // end remove_page


/* COMPILAR *******************************************************************/

#include "jit_compile.h"


// Torna en pos la posició de la primera instrucció descodificada o
// NULL_ENTRY per a indicar que cal esborrar i recompilar tota la
// pàgina. Açò no és considera en realitat un error. En cas d'error
// (cal executar l'excepció) torna false, true si no cal executar cap
// excepció.
static bool
dis_insts (
           IA32_JIT      *jit,
           IA32_JIT_Page *p,
           uint32_t       addr,
           uint32_t       offset,
           uint32_t      *pos
           )
{

  uint32_t last_addr,flags,e,tmp_e,beg_offset;
  size_t n,N,tmp_size,diff;
  int i,nbytes;
  bool is32,end;
  IA32_Mnemonic name;
  
  
  // Última adreça de la pàgina actual. S'assumix que addr és una
  // adreça vàlida de la pàgina.
  last_addr= (addr&(~(jit->_page_low_mask))) | (jit->_page_low_mask);

  // Desenssambla tot el que es puga abans d'aplegar al final,
  // trovar-se una instrucció de control de fluxe, una excepció o un
  // solapament. També pare si el offset pega la volta 0xffffffff -> 0
  beg_offset= offset;
  is32= ADDR_OP_SIZE_IS_32;
  N= 0;
  end= false;
  assert ( jit->_exception.vec == -1 );
  tmp_e= addr&(jit->_page_low_mask);
  if ( tmp_e < p->first_entry ) p->first_entry= tmp_e;
  if ( tmp_e > p->last_entry ) p->last_entry= tmp_e;
  do {

    // Prepara memòria.
    if ( N == jit->_dis_capacity )
      {
        tmp_size= jit->_dis_capacity*2;
        if ( tmp_size <= jit->_dis_capacity )
          {
            fprintf ( stderr, "cannot allocate memory" );
            exit ( EXIT_FAILURE );
          }
        jit->_dis_v=
          (IA32_JIT_DisEntry *) realloc__
          ( jit->_dis_v, tmp_size*sizeof(IA32_JIT_DisEntry) );
        jit->_dis_capacity= tmp_size;
      }

    // Si una instrucció ja està desenssamblada pare (caldrà ficar un
    // goto eip) no passarà mai amb la primera instrucció.
    e= addr&(jit->_page_low_mask);
    if ( p->entries[e].ind != NULL_ENTRY && p->entries[e].ind != PAD_ENTRY )
      {
        assert ( N > 0 );
        break;
      }
            
    // Desenssambla
    IA32_dis ( &(jit->_dis), (uint64_t) offset, &(jit->_dis_v[N].inst) );
    if ( jit->_exception.vec == -1 )
      {
        
        // Actualitza
        //NOTA!! El CR0 és un cas súper especial. Modificar el CR0 pot
        //modificar el valor de is32, per això pare.
        jit->_dis_v[N].addr= addr;
        if ( INSTS_METADATA[jit->_dis_v[N].inst.name].branch ||
             (jit->_dis_v[N].inst.name == IA32_MOV32 &&
              jit->_dis_v[N].inst.ops[0].type == IA32_CR0) )
          end= true;
        nbytes= jit->_dis_v[N].inst.real_nbytes;
        ++N;

        // De moment etiqueta totes les entrades com a PAD i calcula
        // overpadding. Si trova alguna no NULL acava amb
        // error. Actualitza també addr.
        diff= ((size_t) last_addr-addr)+1;
        if ( diff > (size_t) nbytes )
          {
            // NOTA!!! Vaig a assumir que no hi han instruccions que a
            // meitat instrucció peguen la volta (comencen pel final i
            // acaven pel principi) dins del seu segment. Si passara
            // tampoc seria un grandíssim problema, faltarien marcar
            // correctament alguns paddings (també sobrarien) i per
            // tant igual es produirien conflictes d'eliminar i crear
            // pàgines.
            offset+= nbytes;
            if ( offset < beg_offset ) end= true; // PAREM EN AQUESTA SITUACIÓ
            addr+= nbytes;
            tmp_e= e + (nbytes-1);
            if ( tmp_e > p->last_entry ) p->last_entry= tmp_e;
          }
        else // Fi de pàgina (pot ser amb solapament)
          {
            p->overlap_next_page= nbytes - ((int) diff);
            nbytes= (int) diff;
            p->last_entry= jit->_page_low_mask;
            end= true;
          }
        for ( i= 0; i < nbytes; ++i, ++e )
          {
            // Torna amb error indicant que s'ha de tornar a
            // recompilar tota la pàgina.
            if ( p->entries[e].ind != NULL_ENTRY )
              {
                *pos= NULL_ENTRY;
                return true;
              }
            p->entries[e].ind= PAD_ENTRY; // De moment
            p->entries[e].is32= is32;
          }
        // NOTA!!! En cas d'overlap podria comprovar també els bytes
        // de la pàgina següent en cas d'existir. Però com no vaig a
        // gestionar codi entre pàgines, viaig a ser permisiu. Sí que
        // és important esborrar la pàgina si dos instruccions de la
        // mateixa pàgina es solapen.
        
      }
    else end= true;
    
  } while ( !end );

  // Optimització de flags
  if ( jit->_optimize_flags )
    {
      flags= 0xFFFFFFFF;
      for ( n= N-1; n != (size_t) -1; --n )
        {
          jit->_dis_v[n].flags= flags;
          name= jit->_dis_v[n].inst.name;
          //printf("%d %d\n",name,INSTS_METADATA[name].name);
          assert ( name == INSTS_METADATA[name].name);
          if ( jit->_dis_v[n].inst.prefix == IA32_PREFIX_NONE )
            {
              flags&= ~(INSTS_METADATA[name].chg_flags);
              flags|= INSTS_METADATA[name].req_flags;
            }
          // Quan hi ha REP és possible no executar res, per tant
          // chn_flags pot ser 0.
          else 
            {
              flags|= INSTS_METADATA[name].req_flags;
              if ( jit->_dis_v[n].inst.prefix == IA32_PREFIX_REPE ||
                   jit->_dis_v[n].inst.prefix == IA32_PREFIX_REPNE )
                flags|= ZF_FLAG;
            }
        }
    }
  else
    {
      for ( n= 0; n < N; ++n )
        jit->_dis_v[n].flags= 0xFFFFFFFF;
    }

  // Compila a bytecode
  *pos= p->N;
  for ( n= 0; n < N; ++n )
    {
      e= jit->_dis_v[n].addr&(jit->_page_low_mask);
      p->entries[e].ind= p->N;
      compile ( jit, &(jit->_dis_v[n]), p );
    }
  // --> Si s'ha acabat sense excepció ni branch s'afegeix un goto_eip
  //     adicional i es transforma el INC_EIP per un no stop
  if ( jit->_exception.vec == -1 &&
       (N == 0 || !INSTS_METADATA[jit->_dis_v[N-1].inst.name].branch ) )
    {
      if ( p->N > 0 &&
           p->v[p->N-1] >= BC_INC1_EIP && p->v[p->N-1] < BC_INC1_EIP_NOSTOP )
        p->v[p->N-1]+= BC_INC1_EIP_NOSTOP-BC_INC1_EIP;
      add_word ( p, BC_GOTO_EIP );
    }
  
  // En cas d'excepció.
  if ( jit->_exception.vec != -1 )
    {
      // Si s'han pogut descodificar instruccions aleshores ignore
      // l'excepció i force un GOTO_EIP, perquè la pròxima vegada es
      // genera l'excepció al principi.
      if ( N > 0 )
        {
          if ( p->N > 0 &&
               p->v[p->N-1] >= BC_INC1_EIP &&
               p->v[p->N-1] < BC_INC1_EIP_NOSTOP )
            p->v[p->N-1]+= BC_INC1_EIP_NOSTOP-BC_INC1_EIP;
          add_word ( p, BC_GOTO_EIP );
          jit->_exception.vec= -1; // Ignora excepció.
        }
      // En cas contrari l'excepció serà la primera del bloc, i forcem
      // un error perquè s'execute l'excepció.
      else return false;
      /* // CODI ANTINC
      add_word ( p, BC_WRONG_INST );
      add_word ( p, (uint16_t) (jit->_exception.vec) );
      jit->_exception.vec= -1;
      */
    }
  
  return true;
  
} // end dis_insts


/* EXECUTA INSTRUCCIONS *******************************************************/

#include "jit_exec.h"




/**********************/
/* FUNCIONS PÚBLIQUES */
/**********************/

IA32_JIT *
IA32_jit_new (
              IA32_CPU               *cpu,
              const int               bits_page,
              const bool              optimize_flags,
              const IA32_JIT_MemArea *mem_areas,
              const int               N
              )
{
  
  IA32_JIT *ret;
  size_t i,tmp;
  int n;
  
  
  assert ( cpu != NULL );
  assert ( bits_page <= 16 && bits_page >= 4 );
  
  // Reserva i inicialitza a NULL estat públic.
  ret= (IA32_JIT *) malloc__ ( sizeof(IA32_JIT) );
  ret->udata= NULL;
  ret->warning= NULL;
  ret->mem_read8= NULL;
  ret->mem_read16= NULL;
  ret->mem_read32= NULL;
  ret->mem_read64= NULL;
  ret->mem_write8= NULL;
  ret->mem_write16= NULL;
  ret->mem_write32= NULL;
  ret->port_read8= NULL;
  ret->port_read16= NULL;
  ret->port_read32= NULL;
  ret->port_write8= NULL;
  ret->port_write16= NULL;
  ret->port_write32= NULL;

  // CPU
  ret->_cpu= cpu;
  
  // Pàgines
  ret->_bits_page= bits_page;
  ret->_page_low_mask= (1<<bits_page)-1;
  assert ( N > 0 );
  ret->_mem_map= (IA32_JIT_MemMap *) malloc__ ( sizeof(IA32_JIT_MemMap)*N );
  ret->_mem_map_size= N;
  for ( n= 0; n < N; ++n )
    {
      assert ( mem_areas[n].size > 0 );
      tmp= mem_areas[n].size>>bits_page;
      assert ( (mem_areas[n].addr&ret->_page_low_mask) == 0 );
      if ( n > 0 )
        {
          assert ( mem_areas[n].addr > ret->_mem_map[n-1].last_addr );
        }
      ret->_mem_map[n].first_addr= mem_areas[n].addr;
      ret->_mem_map[n].last_addr= mem_areas[n].addr + (mem_areas[n].size-1);
      assert ( ret->_mem_map[n].first_addr <= ret->_mem_map[n].last_addr );
      ret->_mem_map[n].map=
        (IA32_JIT_Page **) malloc__ ( sizeof(IA32_JIT_Page *)*tmp );
      for ( i= 0; i < tmp; ++i )
        ret->_mem_map[n].map[i]= NULL;
    }
  ret->_free_pages= NULL;
  ret->_pages= NULL;
  ret->_pages_tail= NULL;
  ret->_current_page= NULL;
  ret->_current_pos= 0; // 0 no té sentit
  ret->_lock_page= NULL;
  ret->_free_lock_page= false;
  
  // Punters a registres
  ret->_seg_regs[CS_ID]= &(cpu->cs);
  ret->_seg_regs[DS_ID]= &(cpu->ds);
  ret->_seg_regs[SS_ID]= &(cpu->ss);
  ret->_seg_regs[ES_ID]= &(cpu->es);
  ret->_seg_regs[FS_ID]= &(cpu->fs);
  ret->_seg_regs[GS_ID]= &(cpu->gs);
  
  // Inicialitza disassembler
  ret->_optimize_flags= optimize_flags;
  ret->_dis.udata= (void *) ret;
  ret->_dis.mem_read= dis_mem_read;
  ret->_dis.address_size_is_32= dis_address_operand_size_is_32;
  ret->_dis.operand_size_is_32= dis_address_operand_size_is_32;
  ret->_dis_v= (IA32_JIT_DisEntry *) malloc__ ( sizeof(IA32_JIT_DisEntry) );
  ret->_dis_capacity= 1;
  ret->_exception.vec= -1;
  ret->_exception.with_selector= false;
  ret->_exception.use_error_code= false;
  
  // Altres.
  ret->_ignore_exceptions= false;
  ret->_inhibit_interrupt= false;
  ret->_intr= false;
  ret->_stop_after_port_write= false;
  update_mem_callbacks ( ret );
  ret->_pag32= paging_32b_new ();
  
  return ret;
  
} // end IA32_jit_new


void
IA32_jit_reset (
                IA32_JIT *jit
                )
{
  
  jit->_current_page= NULL;
  jit->_current_pos= 0; // 0 no té sentit
  jit->_inhibit_interrupt= false;
  jit->_ignore_exceptions= false;
  jit->_intr= false;
  jit->_stop_after_port_write= false;
  update_mem_callbacks ( jit );
  
} // end IA32_jit_reset


void
IA32_jit_free (
               IA32_JIT *jit
               )
{

  IA32_JIT_Page *p,*q;
  int n;


  // Paginador
  paging_32b_free ( jit->_pag32 );
  
  // Allibera pàgines
  p= jit->_free_pages;
  while ( p != NULL )
    {
      q= p;
      p= p->next;
      free ( q->entries );
      free ( q->v );
      free ( q );
    }
  p= jit->_pages;
  while ( p != NULL )
    {
      q= p;
      p= p->next;
      free ( q->entries );
      free ( q->v );
      free ( q );
    }
  for ( n= 0; n < jit->_mem_map_size; ++n )
    free ( jit->_mem_map[n].map );
  free ( jit->_mem_map );

  // Desessamblar
  free ( jit->_dis_v );
  
  free ( jit );
  
} // end IA32_jit_free


void
IA32_jit_clear_areas (
                      IA32_JIT *jit
                      )
{

  int area;
  const IA32_JIT_MemMap *mem_map;
  uint32_t page,page_e;
  
  
  for ( area= 0; area < jit->_mem_map_size; ++area )
    {
      mem_map= &(jit->_mem_map[area]);
      page_e= (mem_map->last_addr-mem_map->first_addr)>>jit->_bits_page;
      for ( page= 0; page <= page_e; ++page )
        if ( mem_map->map[page] != NULL )
          remove_page ( jit, area, page );
    }
  
} // end IA32_jit_clear_areas


bool
IA32_jit_addr_changed (
                       IA32_JIT       *jit,
                       const uint32_t  addr
                       )
{

  bool ret;
  int area;
  uint32_t page,inst;
  IA32_JIT_Page *p,*q;
  

  ret= false;
  
  for ( area= 0;
        area < jit->_mem_map_size &&
          (addr < jit->_mem_map[area].first_addr ||
           addr > jit->_mem_map[area].last_addr);
        ++area );
  if ( area != jit->_mem_map_size )
    {
      page= (addr-jit->_mem_map[area].first_addr)>>jit->_bits_page;
      p= jit->_mem_map[area].map[page];
      if ( p != NULL )
        {
          inst= (addr)&(jit->_page_low_mask);
          // Elimina la pàgina anterior si té overlapping en eixa zona
          if ( page > 0 &&
               inst < 16 &&
               (q= jit->_mem_map[area].map[page-1])!=NULL &&
               inst < (uint32_t) (q->overlap_next_page) )
            remove_page ( jit, area, page-1 );
          // Elimina pàgina actual si la zona està desensamblada
          if ( p->entries[inst].ind != NULL_ENTRY )
            {
              remove_page ( jit, area, page );
              ret= true;
            }
        }
    }

  return ret;
  
} // end IA32_jit_addr_changed


void
IA32_jit_area_remapped (
                        IA32_JIT       *jit,
                        const uint32_t  begin,
                        const uint32_t  last
                        )
{
  
  uint32_t page_b,page_e,inst_b,i,tmp_b;
  IA32_JIT_Page *p;
  int area;
  
  
  tmp_b= begin;
  for ( area= 0; area < jit->_mem_map_size && tmp_b <= last; ++area )
    if ( tmp_b >= jit->_mem_map[area].first_addr &&
         tmp_b <= jit->_mem_map[area].last_addr )
      {

        // Pàgines dins d'esta àrea
        page_b= (tmp_b-jit->_mem_map[area].first_addr)>>jit->_bits_page;
        if ( last > jit->_mem_map[area].last_addr )
          page_e= (jit->_mem_map[area].last_addr-
                   jit->_mem_map[area].first_addr)>>jit->_bits_page;
        else
          page_e= (last-jit->_mem_map[area].first_addr)>>jit->_bits_page;
        
        // Elimina pàgina anterior si hi ha overlapping
        inst_b= (tmp_b)&(jit->_page_low_mask);
        if ( page_b > 0 &&
             inst_b < 16 &&
             (p= jit->_mem_map[area].map[page_b-1])!=NULL &&
             inst_b < (uint32_t) (p->overlap_next_page) )
          remove_page ( jit, area, page_b-1 );
        
        // Elimina totes les pàgines en el rang
        for ( i= page_b; i <= page_e; ++i )
          if ( jit->_mem_map[area].map[i] != NULL )
            remove_page ( jit, area, i );
        
        // Fica tmp_b al primer element de la següent pàgina
        if ( area+1 < jit->_mem_map_size )
          tmp_b= jit->_mem_map[area+1].first_addr;
        
      }
  
} // end IA32_jit_area_remapped


void
IA32_jit_exec_next_inst (
                         IA32_JIT *jit
                         )
{
  
  uint8_t ivec;
  
  
  if ( !jit->_intr ||
       jit->_inhibit_interrupt ||
       (EFLAGS&IF_FLAG) == 0 )
    {
      jit->_inhibit_interrupt= false;
      exec_inst ( jit );
    }
  else
    {
      ivec= IA32_ack_intr ();
      if ( interruption ( jit, EIP,
                          INTERRUPTION_TYPE_UNK, ivec,
                          0, false ) )
        goto_eip ( jit );
      else
        {
          WW ( UDATA, "s'ha produït un error inesperat mentre es"
               " gestionava una interrupció" );
        }
    }

} // end IA32_jit_exec_next_inst


void
IA32_jit_set_intr (
                   IA32_JIT   *jit,
                   const bool  value
                   )
{
  jit->_intr= value;
} // end IA32_jit_set_intr


void
IA32_jit_init_dis (
                   IA32_JIT          *jit,
                   IA32_Disassembler *dis
                   )
{

  dis->udata= (void *) jit;
  dis->mem_read= dis_mem_read_trace;
  dis->address_size_is_32= dis_address_operand_size_is_32;
  dis->operand_size_is_32= dis_address_operand_size_is_32;
  
} // end IA32_jit_init_dis
