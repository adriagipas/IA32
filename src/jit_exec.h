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
 *  jit_exec.h - Conté la part de 'jit.c' dedicada a executar el
 *               bytecode.
 *
 */

#include <fenv.h>
#include <math.h>




/**********/
/* MACROS */
/**********/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#define GET_EXPONENT_LDOUBLE_U(VAL) ((uint16_t) ((VAL).u16.v4&0x7FFF))

#define FPU_CHECKU32_ZERO(U32) (((U32)&0x7FFFFFFF)==0)
#define FPU_CHECKU32_DENORMAL(U32)                                      \
  ((!FPU_CHECKU32_ZERO(U32)) && (((U32)&0x7F800000)==0x00000000))
#define FPU_CHECKU32_INF(U32) (((U32)&0x7FFFFFFF)==0x7F800000)
#define FPU_CHECKU32_NAN(U32)                                           \
  ((!FPU_CHECKU32_INF(U32)) && (((U32)&0x7F800000)==0x7F800000))
#define FPU_CHECKU32_SPECIAL(U32)                                       \
  (FPU_CHECKU32_INF(U32) ||                                             \
   FPU_CHECKU32_DENORMAL(U32) ||                                        \
   FPU_CHECKU32_NAN(U32))
#define FPU_CHECKU32_SNAN(U32)                                          \
  ((!FPU_CHECKU32_INF(U32)) && (((U32)&0x7FC00000)==0x7F800000))

#define FPU_U32_SNAN2QNAN(U32) ((U32)|0x00400000)


#define FPU_CHECKU64_ZERO(U64) (((U64)&0x7FFFFFFFFFFFFFFF)==0)
#define FPU_CHECKU64_DENORMAL(U64)                                      \
  ((!FPU_CHECKU64_ZERO(U64)) &&                                         \
   (((U64)&0x7FF0000000000000)==0x0000000000000000))
#define FPU_CHECKU64_INF(U64)                           \
  (((U64)&0x7FFFFFFFFFFFFFFF)==0x7FF0000000000000)
#define FPU_CHECKU64_NAN(U64)                                           \
  ((!FPU_CHECKU64_INF(U64)) &&                                          \
   (((U64)&0x7FF0000000000000)==0x7FF0000000000000))
#define FPU_CHECKU64_SPECIAL(U64)                                       \
  (FPU_CHECKU64_INF(U64) ||                                             \
   FPU_CHECKU64_DENORMAL(U64) ||                                        \
   FPU_CHECKU64_NAN(U64))
#define FPU_CHECKU64_SNAN(U64)                                          \
  ((!FPU_CHECKU64_INF(U64)) &&                                          \
   (((U64)&0x7FF8000000000000)==0x7FF0000000000000))

#define FPU_U64_SNAN2QNAN(U64) ((U64)|0x0008000000000000)


#define FPU_CHECKU80_ZERO(U64L,U16H) (((U64L)==0) && (((U16H)&0x7FFF)==0))
#define FPU_CHECKU80_DENORMAL(U64L,U16H)                                \
  ((!FPU_CHECKU80_ZERO(U64L,U16H)) && (((U16H)&0x7FFF)==0x0000))
#define FPU_CHECKU80_INF(U64L,U16H) \
  (((U64L)==0x8000000000000000) && (((U16H)&0x7FFF)==0x7FFF))
#define FPU_CHECKU80_NAN(U64L,U16H)                                     \
  ((!FPU_CHECKU80_INF(U64L,U16H)) && (((U16H)&0x7FFF)==0x7FFF))
#define FPU_CHECKU80_SPECIAL(U64L,U16H)                                 \
  (FPU_CHECKU80_INF(U64L,U16H) ||                                       \
   FPU_CHECKU80_DENORMAL(U64L,U16H) ||                                  \
   FPU_CHECKU80_NAN(U64L,U16H))
#define FPU_CHECKU80_SNAN(U64L,U16H)                                    \
  ((!FPU_CHECKU80_INF(U64L,U16H)) &&                                    \
   (((U16H)&0x7FFF)==0x7FFF) &&                                         \
   (((U64L)&0xC000000000000000)==0x8000000000000000))

#define FPU_U80_SNAN2QNAN(U64L) ((U64L)|0x4000000000000000)


#define FPU_CHECK_STACK_UNDERFLOW(IND,ACTION)           \
  if ( l_FPU_REG(IND).tag == IA32_CPU_FPU_TAG_EMPTY )   \
    {                                                   \
      l_FPU_STATUS|= (FPU_STATUS_IE|FPU_STATUS_SF);     \
      l_FPU_STATUS&= ~FPU_STATUS_C1;                    \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }       \
    }

#define FPU_CHECK_STACK_OVERFLOW(IND,ACTION)                            \
  if ( l_FPU_REG(IND).tag != IA32_CPU_FPU_TAG_EMPTY )                   \
    {                                                                   \
      l_FPU_STATUS|= (FPU_STATUS_IE|FPU_STATUS_SF|FPU_STATUS_C1);       \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }                       \
    }

#define FPU_CHECK_SNAN_VALUE_U32(U32,ACTION)              \
  if ( FPU_CHECKU32_SNAN(U32) )                           \
    {                                                     \
      l_FPU_STATUS|= FPU_STATUS_IE;                       \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )              \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_SNAN_VALUE_U64(U64,ACTION)              \
  if ( FPU_CHECKU64_SNAN(U64) )                           \
    {                                                     \
      l_FPU_STATUS|= FPU_STATUS_IE;                       \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )              \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_DENORMAL_VALUE_U32(U32,ACTION)          \
  if ( FPU_CHECKU32_DENORMAL(U32) )                       \
    {                                                     \
      l_FPU_STATUS|= FPU_STATUS_DE;                       \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )              \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_DENORMAL_VALUE_U64(U64,ACTION)        \
  if ( FPU_CHECKU64_DENORMAL(U64) )                     \
    {                                                     \
      l_FPU_STATUS|= FPU_STATUS_DE;                       \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )              \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_DENORMAL_VALUE_U80(U64L,U16H,ACTION)  \
  if ( FPU_CHECKU80_DENORMAL(U64L,U16H) )               \
    {                                                   \
      l_FPU_STATUS|= FPU_STATUS_DE;                     \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }       \
    }

#define FPU_CHECK_DENORMAL_REG(IND,ACTION)                      \
  if ( l_FPU_REG(IND).tag == IA32_CPU_FPU_TAG_SPECIAL &&        \
       fpclassify ( l_FPU_REG(IND).v ) == FP_SUBNORMAL )        \
    {                                                           \
      l_FPU_STATUS|= FPU_STATUS_DE;                             \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                    \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }               \
    }

#define FPU_CHECK_NUMERIC_OVERFLOW_BOOL(VAL,ACTION)       \
  if ( (VAL) )                                            \
    {                                                     \
      l_FPU_STATUS|= FPU_STATUS_OE;                       \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_OM) )              \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(VAL,ACTION)      \
  if ( (VAL) )                                            \
    {                                                     \
      l_FPU_STATUS|= FPU_STATUS_UE;                       \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_UM) )              \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_FCOM_CHECK_INVALID_OPERAND(IND,ACTION,INT_VAR)      \
  if ( l_FPU_REG(IND).tag == IA32_CPU_FPU_TAG_SPECIAL )         \
    {                                                           \
      (INT_VAR)= fpclassify ( l_FPU_REG(IND).v );               \
      if ( (INT_VAR) == FP_SUBNORMAL )                          \
        {                                                       \
          l_FPU_STATUS|= FPU_STATUS_DE;                         \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }           \
        }                                                       \
      else if ( (INT_VAR) == FP_NAN )                           \
        {                                                       \
          l_FPU_STATUS|= FPU_STATUS_IE;                         \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }           \
        }                                                       \
    }

#define FPU_FCOM_CHECK_INVALID_OPERAND_U32(U32,ACTION)                \
  if ( FPU_CHECKU32_SPECIAL(U32) )                                    \
    {                                                                 \
      if ( FPU_CHECKU32_DENORMAL(U32) )                               \
        {                                                             \
          l_FPU_STATUS|= FPU_STATUS_DE;                               \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                      \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
        }                                                             \
      else if ( FPU_CHECKU32_NAN(U32) )                               \
        {                                                             \
          l_FPU_STATUS|= FPU_STATUS_IE;                               \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                      \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
        }                                                             \
    }

#define FPU_FCOM_CHECK_INVALID_OPERAND_U64(U64,ACTION)              \
  if ( FPU_CHECKU64_SPECIAL(U64) )                                  \
    {                                                               \
      if ( FPU_CHECKU64_DENORMAL(U64) )                             \
        {                                                           \
          l_FPU_STATUS|= FPU_STATUS_DE;                             \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                    \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }               \
        }                                                           \
      else if ( FPU_CHECKU64_NAN(U64) )                             \
        {                                                           \
          l_FPU_STATUS|= FPU_STATUS_IE;                             \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                    \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }               \
        }                                                           \
    }

#define FPU_CHECK_INVALID_OPERAND(IND,ACTION)                   \
  if ( l_FPU_REG(IND).tag == IA32_CPU_FPU_TAG_SPECIAL )         \
    {                                                           \
      if ( fpclassify ( l_FPU_REG(IND).v ) == FP_SUBNORMAL )    \
        {                                                       \
          l_FPU_STATUS|= FPU_STATUS_DE;                         \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }           \
        }                                                       \
      else                                                      \
        {                                                       \
          l_FPU_STATUS|= FPU_STATUS_IE;                         \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }           \
        }                                                       \
    }

#define FPU_CHECK_INVALID_OPERAND_U32(U32,ACTION)                   \
  if ( FPU_CHECKU32_SPECIAL(U32) )                                  \
    {                                                               \
      if ( FPU_CHECKU32_DENORMAL(U32) )                             \
        {                                                           \
          l_FPU_STATUS|= FPU_STATUS_DE;                             \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                    \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }               \
        }                                                           \
      else                                                          \
        {                                                           \
          l_FPU_STATUS|= FPU_STATUS_IE;                             \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                    \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }               \
        }                                                           \
      }

#define FPU_CHECK_INVALID_OPERAND_U64(U64,ACTION)                   \
  if ( FPU_CHECKU64_SPECIAL(U64) )                                  \
    {                                                               \
      if ( FPU_CHECKU64_DENORMAL(U64) )                             \
        {                                                           \
          l_FPU_STATUS|= FPU_STATUS_DE;                             \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                    \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }               \
        }                                                           \
      else                                                          \
        {                                                           \
          l_FPU_STATUS|= FPU_STATUS_IE;                             \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                    \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }               \
        }                                                           \
      }

#define FPU_CHECK_NUMERIC_OVERFLOW_BOOL(VAL,ACTION)     \
  if ( (VAL) )                                          \
    {                                                   \
      l_FPU_STATUS|= FPU_STATUS_OE;                     \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_OM) )            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }       \
    }

#define FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(VAL,ACTION)    \
  if ( (VAL) )                                          \
    {                                                   \
      l_FPU_STATUS|= FPU_STATUS_UE;                     \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_UM) )            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }       \
    }

#define FPU_CHECK_INVALID_OPERATION_BOOL(VAL,ACTION)    \
  if ( (VAL) )                                          \
    {                                                   \
      l_FPU_STATUS|= FPU_STATUS_IE;                     \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }       \
    }

// NOTA! Tenia aquest missatge però l'he llevat.
//
// WW ( UDATA,                                                      
//      "inexact result exception - C1 update not emulated, set to 0" );
#define FPU_CHECK_INEXACT_RESULT_BOOL(VAL,ACTION)                       \
  if ( (VAL) )                                                          \
    {                                                                   \
      l_FPU_STATUS&= ~FPU_STATUS_C1;                                    \
      l_FPU_STATUS|= FPU_STATUS_PE;                                     \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_PM) )                            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }                       \
    }

#define FPU_CHECK_DIVBYZERO_RESULT_BOOL(VAL,ACTION)                     \
  if ( (VAL) )                                                          \
    {                                                                   \
      l_FPU_STATUS|= FPU_STATUS_ZE;                                     \
      if ( !(l_FPU_CONTROL&FPU_CONTROL_ZM) )                            \
        { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }                       \
    }
#pragma GCC diagnostic pop




/*********/
/* TIPUS */
/*********/

typedef union {
  float    f;
  uint32_t u32;
} float_u32_t;

typedef union {
  double                   d;
  uint64_t                 u64;
#ifdef IA32_LE
  struct { uint32_t l,h; } u32;
#else
  struct { uint32_t h,l; } u32;
#endif
} double_u64_t;

typedef union {
  long double ld;
#ifdef IA32_LE
  struct { uint16_t v0,v1,v2,v3,v4,v5,v6,v7; } u16;
  struct { uint32_t v0,v1,v2,v3; } u32;
  struct { uint64_t v0,v1; } u64;
#else
  struct { uint16_t v7,v6,v5,v4,v3,v2,v1,v0; } u16;
  struct { uint32_t v3,v2,v1,v0; } u32;
  struct { uint64_t v1,v0; } u64;
#endif // IA32_LE
} ldouble_u80_t;




/*************/
/* CONSTANTS */
/*************/

static const uint8_t BSF_TABLE[256]=
  {
    8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,
    0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,
    1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,
    0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,
    2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,
    0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,
    1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
  };

static const uint8_t BSR_TABLE[256]=
  {
    8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };




/****************/
/* DECLARACIONS */
/****************/

#define SWITCH_TASK_JMP  0x01
#define SWITCH_TASK_CALL 0x02
#define SWITCH_TASK_IRET 0x04
// Excepció
#define SWITCH_TASK_EXC  0x08
// Interrupció Software
#define SWITCH_TASK_INT  0x10

static bool
switch_task (
             IA32_JIT       *jit,
             const int       type,
             const uint16_t  selector,
             const uint8_t   ext,
             const uint32_t  old_EIP
             );




/************/
/* FUNCIONS */
/************/

static bool
cli (
     IA32_JIT *jit
     )
{

  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( (l_EFLAGS&VM_FLAG) ) // Virtual mode
        {
          if ( l_IOPL == 3 ) { l_EFLAGS&= ~IF_FLAG; }
          else if ( l_IOPL < 3 && (l_CR4&CR4_VME)!=0 ) { l_EFLAGS&= ~VIF_FLAG; }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
        }
      else
        {
          if ( l_IOPL >= l_CPL ) { l_EFLAGS&= ~IF_FLAG; }
          else if ( l_IOPL < l_CPL && l_CPL == 3 && (l_CR4&CR4_PVI)!=0 )
            { l_EFLAGS&= ~VIF_FLAG; }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
        }
    }
  
  // Sense protecció
  else l_EFLAGS&= ~IF_FLAG;

  return true;
  
} // end cli


static bool
sti (
     IA32_JIT *jit
     )
{
  
  if ( !PROTECTED_MODE_ACTIVATED )
    {
      if ( (EFLAGS&IF_FLAG) == 0 )
        jit->_inhibit_interrupt= true;
      EFLAGS|= IF_FLAG;
    }
  else
    {
      if ( EFLAGS&VM_FLAG )
        {
          if ( IOPL == 3 )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                jit->_inhibit_interrupt= true;
              EFLAGS|= IF_FLAG;
            }
          else if ( IOPL < 3 && (EFLAGS&VIP_FLAG)==0 && (CR4&CR4_VME)!=0 )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                jit->_inhibit_interrupt= true;
              EFLAGS|= VIF_FLAG;
            }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
        }
      else
        {
          if ( IOPL >= CPL )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                jit->_inhibit_interrupt= true;
              EFLAGS|= IF_FLAG;
            }
          else if ( IOPL < CPL && CPL==3 && (CR4&CR4_PVI)!=0 )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                jit->_inhibit_interrupt= true;
              EFLAGS|= VIF_FLAG;
            }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
        }
    }

  return true;
  
} // end sti


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
  //   -> Sols per a TSS.
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
      fprintf ( FERROR, "[EE] load_segment_descriptor, REPASAR AQUESTA PART!!!! sospite que quan granularity està activa açò no està bé\n" );
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


static bool
read_segment_descriptor (
                         IA32_JIT       *jit,
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
          return false;
        }
      if ( off < LDTR.h.lim.firstb || (off+7) > LDTR.h.lim.lastb )
        {
          if ( excp_lim ) { EXCEPTION_SEL ( EXCP_GP, selector ); }
          return false;
        }
      desc->addr= LDTR.h.lim.addr + off;
    }
  else // GDT
    {
      if ( off < GDTR.firstb || (off+7) > GDTR.lastb )
        {
          if ( excp_lim ) { EXCEPTION_SEL ( EXCP_GP, selector ); }
          return false;
        }
      desc->addr= GDTR.addr + off;
    }

  // Llig descriptor.
  READLD ( desc->addr, &(desc->w1), return false, true, true );
  READLD ( desc->addr+4, &(desc->w0), return false, true, true );
  
  return true;
  
} // end read_segment_descriptor


// Aquesta funció fixa el bit del descriptor (data/codi) i el torna a
// escriure en memòria.
static void
set_segment_descriptor_access_bit (
                                   IA32_JIT   *jit,
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
      if ( jit->_mem_writel32 ( jit, desc->addr+4, desc->w0 ) != 0 )
        fprintf ( FERROR, "Avís: excepció en WRITELD en"
                  " ''set_segment_descriptor_access_bit\n" );
    }
  
} // end set_segment_descriptor_access_bit


static bool
push32 (
        IA32_JIT       *jit,
        const uint32_t  val
        )
{
  
  uint32_t addr, offset;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // Calcula offset i comprova.
  offset= l_P_SS->h.is32 ? (l_ESP-4) : ((uint32_t) ((uint16_t) (l_SP-4)));
  if ( offset < l_P_SS->h.lim.firstb || offset > (l_P_SS->h.lim.lastb-3) )
    {
      EXCEPTION0 ( EXCP_SS );
      return false;
    }
  
  // Tradueix, escriu i actualitza.
  addr= l_P_SS->h.lim.addr + offset;
  if ( jit->_mem_writel32 ( jit, addr, val ) != 0 )
    return false;
  if ( l_P_SS->h.is32 ) l_ESP-= 4;
  else                  l_SP-= 4;
  
  return true;
  
} // end push32


static bool
push16 (
        IA32_JIT       *jit,
        const uint16_t  val
        )
{
  
  uint32_t addr,offset;
  IA32_CPU *l_cpu;

  
  l_cpu= jit->_cpu;
  
  // Calcula offset i comprova.
  offset= l_P_SS->h.is32 ? (l_ESP-2) : ((uint32_t) ((uint16_t) (l_SP-2)));
  if ( offset < l_P_SS->h.lim.firstb || offset > (l_P_SS->h.lim.lastb-1) )
    {
      EXCEPTION0 ( EXCP_SS );
      return false;
    }
  
  // Tradueix, escriu i actualitza.
  addr= l_P_SS->h.lim.addr + offset;
  if ( jit->_mem_writel16 ( jit, addr, val ) != 0 )
    return false;
  if ( l_P_SS->h.is32 ) l_ESP-= 2;
  else                  l_SP-= 2;
  
  return true;
  
} // end push16


static bool
pop32_protected (
                 IA32_JIT *jit,
                 uint32_t *ret
                 )
{
  
  uint32_t offset,last_byte,addr;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // Calcula offset i comprova.
  offset= l_P_SS->h.is32 ? l_ESP : ((uint32_t) ((uint16_t) l_SP));
  last_byte= l_P_SS->h.is32 ? (l_ESP+4-1) : ((uint32_t)((uint16_t) (l_SP+4-1)));
  if ( last_byte < l_P_SS->h.lim.firstb || last_byte > l_P_SS->h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_SS );
      return false;
    }

  // Tradueix, llig i actualitza.
  addr= l_P_SS->h.lim.addr + offset;
  if ( jit->_mem_readl32 ( jit, addr, ret, true, false ) != 0 )
    return false;
  if ( l_P_SS->h.is32 ) l_ESP+= 4;
  else                  l_SP+= 4;
  
  return true;
  
} // end pop32_protected


static bool
pop32_nonprotected (
                    IA32_JIT *jit,
                    uint32_t *ret
                    )
{
  
  uint32_t offset,addr;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // Calcula offset.
  offset= l_P_SS->h.is32 ? l_ESP : ((uint32_t) l_SP);
  // No estic segur d'aquesta comprovació.
  if ( offset > 0xFFFC ) { EXCEPTION ( EXCP_SS ); return false; }
  
  // Tradueix, llig i actualitza.
  addr= l_P_SS->h.lim.addr + offset;
  if ( jit->_mem_readl32 ( jit, addr, ret, true, false ) != 0 )
    return false;
  if ( l_P_SS->h.is32 ) l_ESP+= 4;
  else                  l_SP+= 4;
  
  return true;
  
} // end pop32_nonprotected


static bool
pop32 (
        IA32_JIT *jit,
        uint32_t *ret
        )
{
  return
    ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG)) ?
    pop32_protected ( jit, ret ) :
    pop32_nonprotected ( jit, ret );
} // end pop32


static bool
pop16_protected (
                 IA32_JIT *jit,
                 uint16_t *ret
                 )
{

  IA32_CPU *l_cpu;
  uint32_t offset,last_byte,addr;


  l_cpu= jit->_cpu;
  
  // Calcula offset i comprova.
  offset= l_P_SS->h.is32 ? l_ESP : ((uint32_t) ((uint16_t) l_SP));
  last_byte=
    l_P_SS->h.is32 ? (l_ESP+2-1) : ((uint32_t) ((uint16_t) (l_SP+2-1)));
  if ( last_byte < l_P_SS->h.lim.firstb || last_byte > l_P_SS->h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_SS );
      return false;
    }

  // Tradueix, llig i actualitza.
  addr= l_P_SS->h.lim.addr + offset;
  if ( jit->_mem_readl16 ( jit, addr, ret, true, false ) != 0 )
    return false;
  if ( l_P_SS->h.is32 ) l_ESP+= 2;
  else                  l_SP+= 2;
  
  return true;
  
} // end pop16_protected


static bool
pop16_nonprotected (
                    IA32_JIT *jit,
                    uint16_t *ret
                    )
{
  
  uint32_t addr, offset;
  IA32_CPU *l_cpu;
  
  l_cpu= jit->_cpu;

  // Calcula offset
  offset= l_P_SS->h.is32 ? l_ESP : ((uint32_t) l_SP);
  // No estic segur d'aquesta comprovació.
  if ( offset > 0xFFFE ) { EXCEPTION ( EXCP_SS ); return false; }
  
  // Tradueix, escriu i actualitza.
  addr= l_P_SS->h.lim.addr + offset;
  if ( jit->_mem_readl16 ( jit, addr, ret, true, false ) != 0 )
    return false;
  if ( l_P_SS->h.is32 ) l_ESP+= 2;
  else                  l_SP+= 2;
  
  return true;
  
} // end pop16_nonprotected


static bool
pop16 (
        IA32_JIT *jit,
        uint16_t *ret
        )
{
  return
    ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG)) ?
    pop16_protected ( jit, ret ) :
    pop16_nonprotected ( jit, ret );
} // end pop16


static bool
pop32_eflags (
              IA32_JIT *jit
              )
{

  uint32_t cpl,mask,val;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  if ( !pop32 ( jit, &val ) ) return false;
  
  // Comprovacions sobre que bits es poden o no actualitzar.
  if ( !(l_EFLAGS&VM_FLAG) ) // Not in Virtual-8086 Mode
    {
      cpl= l_P_CS->h.pl;
      if ( cpl == 0 )
        {
          // All non-reserved flags except RF, VIP, VIF, and VM
          // can be modified
          // VIP, VIF, VM, and all reserved bits are
          // unaffected. RF is cleared.
          mask= (~(RF_FLAG|VIP_FLAG|VIF_FLAG|VM_FLAG))&0x003F7FD5;
          l_EFLAGS= (l_EFLAGS&(~RF_FLAG)&(~mask)) | (val&mask) | EFLAGS_1S;
        }
      else // CPL>0
        {
          if ( cpl > l_IOPL ) // CPL>0, CPL>IOPL OPS32
            {
              // All non-reserved bits except IF, IOPL, VIP, VIF,
              // VM and RF can be modified;
              // IF, IOPL, VIP, VIF, VM and all reserved bits are
              // unaffected; RF is cleared.
              mask= (~(IF_FLAG|IOPL_FLAG|VIP_FLAG|
                       VIF_FLAG|VM_FLAG))&0x003F7FD5;
              l_EFLAGS= (l_EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
              l_EFLAGS&= ~(RF_FLAG);
            }
          else // CPL>0, CPL<=IOPL OPS32
            {
              // All non-reserved bits except IOPL, VIP, VIF, VM
              // and RF can be modified;
              // IOPL, VIP, VIF, VM and all reserved bits are
              // unaffected; RF is cleared.
              mask= (~(IOPL_FLAG|VIP_FLAG|VIF_FLAG|VM_FLAG))&0x003F7FD5;
              l_EFLAGS= (l_EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
              l_EFLAGS&= ~(RF_FLAG);
            }
        }
    }
  else if ( (l_CR4&CR4_VME) != 0 ) // Virtual mode amb VME Enabled
    {
      printf ( "[EE] POPF no implementat en mode virtual VME!!!!\n" );
      exit ( EXIT_FAILURE );
    }
  else // Virtual mode
    {
      if ( l_IOPL == 3 )
        {
          // All non-reserved bits except IOPL, VIP, VIF, VM, and
          // RF can be modified
          // VIP, VIF, VM, IOPL and all reserved bits are
          // unaffected. RF is cleared.
          mask= (~(RF_FLAG|VIP_FLAG|VIF_FLAG|VM_FLAG|IOPL_FLAG))&0x003F7FD5;
          l_EFLAGS= (l_EFLAGS&(~RF_FLAG)&(~mask)) | (val&mask) | EFLAGS_1S;
        }
      else // IOPL < 3
        { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
    }

  return true;
  
} // end pop32_eflags


static bool
pop16_eflags (
              IA32_JIT *jit
              )
{

  uint32_t cpl,mask,val;
  uint16_t tmp16;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  if ( !pop16 ( jit, &tmp16 ) ) return false;
  val= (uint32_t) tmp16;
  
  // Comprovacions sobre que bits es poden o no actualitzar.
  if ( !(l_EFLAGS&VM_FLAG) ) // Not in Virtual-8086 Mode
    {
      cpl= l_P_CS->h.pl;
      if ( cpl == 0 )
        {
          // All non-reserved flags can be modified.
          mask= 0x00007FD5;
          l_EFLAGS= (l_EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
        }
      else // CPL>0
        {
          // All non-reserved bits except IOPL can be modified;
          // IOPL and all reserved bits are unaffected.
          mask= (~IOPL_FLAG)&0x00007FD5;
          l_EFLAGS= (l_EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
        }
    }
  else if ( (l_CR4&CR4_VME) != 0 ) // Virtual mode amb VME Enabled
    {
      printf ( "[EE] POPF no implementat en mode virtual VME!!!!\n" );
      exit ( EXIT_FAILURE );
    }
  else // Virtual mode
    {
      if ( l_IOPL == 3 )
        {
          // All non-reserved bits except IOPL can be modified;
          // IOPL and all reserved bits are unaffected.
          mask= (~IOPL_FLAG)&0x00007FD5;
          l_EFLAGS= (l_EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
        }
      else // IOPL < 3
        { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
    }

  return true;
  
} // end pop16_eflags


// NOTA!! next_EIP sols es rellevant amb els call
static bool
call_jmp_far_protected_code (
                             IA32_JIT       *jit,
                             const uint16_t  selector,
                             const uint32_t  offset,
                             const uint32_t  stype,
                             seg_desc_t     *desc,
                             const bool      op32,
                             const bool      is_call,
                             const uint32_t  next_EIP
                             )
{

  int nbytes;
  uint32_t dpl,cpl,off_stack;
  bool conforming;
  IA32_SegmentRegister tmp_seg;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // Privilegis.
  conforming= (stype&0x04)!=0;
  dpl= SEG_DESC_GET_DPL ( *desc );
  cpl= l_CPL;
  if ( conforming )
    {
      if ( dpl > cpl ) goto excp_gp_sel;
    }
  else
    {
      if ( cpl < (selector&0x3) || dpl != cpl ) goto excp_gp_sel;
      cpl= dpl;
    }
  
  // Segment-present flag.
  if ( !SEG_DESC_IS_PRESENT ( *desc ) )
    {
      EXCEPTION_SEL ( EXCP_NP, selector );
      return false;
    }

  // Comprova espai pila.
  if ( is_call )
    {
      nbytes= op32 ? 8 : 4;
      off_stack= l_P_SS->h.is32 ?
        (l_ESP-nbytes) : ((uint32_t) ((uint16_t) (l_SP-nbytes)));
      if ( off_stack < l_P_SS->h.lim.firstb ||
           off_stack > (l_P_SS->h.lim.lastb-(nbytes-1)) )
        { EXCEPTION_ERROR_CODE ( EXCP_SS, 0 ); return false; }
    }
  
  // Comprova que el offset és vàlid en el segment nou.
  load_segment_descriptor ( &tmp_seg, (selector&0xFFFC) | cpl, desc );
  if ( offset < tmp_seg.h.lim.firstb || offset > tmp_seg.h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_GP );
      return false;
    }

  // Apila adreça retorn
  if ( is_call )
    {
      if ( op32 )
        {
          if ( !push32 ( jit, (uint32_t) l_P_CS->v ) ) return false;
          if ( !push32 ( jit, next_EIP ) ) return false;
        }
      else
        {
          if ( !push16 ( jit, l_P_CS->v ) ) return false;
          if ( !push16 ( jit, (uint16_t) (next_EIP&0xFFFF) ) ) return false;
        }
    }
  
  // Carrega el descriptor.
  set_segment_descriptor_access_bit ( jit, desc );
  *l_P_CS= tmp_seg;
  l_EIP= offset;
  
  return true;
  
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return false;
  
} // end call_jmp_far_protected_code


static bool
call_far_protected_gate_more_pl (
                                 IA32_JIT       *jit,
                                 const uint16_t  selector,
                                 const uint32_t  offset,
                                 seg_desc_t     *desc,
                                 const bool      is32,
                                 const uint8_t   param_count,
                                 const uint32_t  next_EIP
                                 )
{

  int nbytes;
  uint32_t dpl,dpl_new_SS,rpl_new_SS,off,new_ESP,new_SS_stype,tmp_ESP;
  uint16_t new_SS_selector,new_SP,tmp_SS_selector;
  seg_desc_t new_SS_desc;
  IA32_SegmentRegister new_SS,tmp_seg;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // Identify stack-segment selector for new privilege level in current TSS.
  dpl= SEG_DESC_GET_DPL(*desc);
  assert ( dpl < 3 );
  if ( l_TR.h.tss_is32 )
    {
      off= (dpl<<3) + 4;
      if ( (off+5) > l_TR.h.lim.lastb ) goto excp_ts_error;
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr+off+4,
                               &new_SS_selector, true, true ) != 0 )
        return false;
      if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr+off,
                               &new_ESP, true, true ) != 0 )
        return false;
    }
  else // TSS is 16-bit
    {
      printf("[CAL IMPLEMENTAR] call_far_protected_gate_more_pl"
             " - TSS 16bit\n");
      exit(EXIT_FAILURE);
      /*
      TSSstackAddress ← (new code-segment DPL « 2) + 2
      IF (TSSstackAddress + 3) > current TSS limit
         THEN #TS(error_code(current TSS selector)); FI;
         (* idt operand to error_code is 0 because selector is used *)
      NewSS ← 2 bytes loaded from (TSS base + TSSstackAddress + 2);
      NewESP ← 2 bytes loaded from (TSS base + TSSstackAddress);
      */
    }
  new_SP= (uint16_t) new_ESP;
  
  // Llig newSS
  if ( (new_SS_selector&0xFFFC) == 0 ) // Null segment selector..
    { EXCEPTION_SEL ( EXCP_TS, new_SS_selector ); return false; };
  if ( !read_segment_descriptor ( jit, new_SS_selector,
                                  &new_SS_desc, true ) )
    return false;
  rpl_new_SS= new_SS_selector&0x3;
  dpl_new_SS= SEG_DESC_GET_DPL(new_SS_desc);
  new_SS_stype= SEG_DESC_GET_STYPE ( new_SS_desc );
  if ( rpl_new_SS != dpl ||
       dpl_new_SS != dpl ||
       (new_SS_stype&0x1A)!=0x12 // No writable data
       )
    { EXCEPTION_SEL ( EXCP_TS, new_SS_selector ); return false; }
  if ( !SEG_DESC_IS_PRESENT(new_SS_desc) ) goto excp_ss;
  load_segment_descriptor ( &new_SS, new_SS_selector, &new_SS_desc );
  
  // Comprova espai en la nova pila.
  nbytes= is32 ? 16 : 8;
  off= new_SS.h.is32 ?
    (new_ESP-nbytes) : ((uint32_t) ((uint16_t) (new_SP-nbytes)));
  if ( off < new_SS.h.lim.firstb || off > (new_SS.h.lim.lastb-(nbytes-1)) )
    goto excp_ss;

  // Comprova que el offset és vàlid en el segment nou.
  load_segment_descriptor ( &tmp_seg, selector, desc );
  off= is32 ? offset : (offset&0xFFFF);
  if ( offset < tmp_seg.h.lim.firstb || offset > tmp_seg.h.lim.lastb )
    { EXCEPTION0 ( EXCP_GP ); return false; }

  // Fixa pila, apila i nou valors
  set_segment_descriptor_access_bit ( jit, desc );
  set_segment_descriptor_access_bit ( jit, &new_SS_desc );
  tmp_SS_selector= l_P_SS->v;
  tmp_ESP= l_ESP;
  if ( is32 )
    {
      *l_P_SS= new_SS;
      l_ESP= new_ESP;
      if ( !push32 ( jit, (uint32_t) tmp_SS_selector ) ) return false;
      if ( !push32 ( jit, tmp_ESP ) ) return false;
      if ( param_count != 0 )
        {
          printf("[CAL IMPLEMENTAR] "
                 "call_far_protected_gate_more_pl - PARAM_COUNT:%u\n",
                 param_count);
          exit(EXIT_FAILURE);
        }
      if ( !push32 ( jit, (uint32_t) l_P_CS->v ) ) return false;
      if ( !push32 ( jit, next_EIP ) ) return false;
      *l_P_CS= tmp_seg;
      l_EIP= offset;
    }
  else
    {
      printf("[CAL IMPLEMENTAR] "
             "call_far_protected_gate_more_pl - GATE16\n");
      exit(EXIT_FAILURE);
    }
  
  return true;

 excp_ts_error:
  EXCEPTION_SEL ( EXCP_TS, l_TR.v );
  return false;
 excp_ss:
  EXCEPTION_SEL ( EXCP_SS, new_SS_selector );
  return false;
  
} // end call_far_protected_gate_more_pl


static bool
call_far_protected_gate (
                         IA32_JIT       *jit,
                         const uint16_t  selector_gate,
                         seg_desc_t     *desc_gate,
                         const bool      is32,
                         const uint32_t  next_EIP
                         )
{

  uint8_t param_count;
  uint32_t dpl_gate,rpl_gate,stype,dpl,offset;
  uint16_t selector;
  seg_desc_t desc;
  bool conforming;

  
  // Comprovacions gate.
  dpl_gate= SEG_DESC_GET_DPL ( *desc_gate );
  rpl_gate= selector_gate&0x3;
  if ( dpl_gate < CPL || rpl_gate > dpl_gate ) goto excp_gp_gate;
  if ( !SEG_DESC_IS_PRESENT(*desc_gate) )
    { EXCEPTION_SEL ( EXCP_NP, selector_gate ); return false; }
  
  // Comprovacions selector gate.
  selector= CG_DESC_GET_SELECTOR ( *desc_gate );
  if ( (selector&0xFFFC) == 0 ) // Null segment selector..
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; };
  if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
    return false;
  stype= SEG_DESC_GET_STYPE ( desc );
  dpl= SEG_DESC_GET_DPL ( desc );
  if ( (stype&0x18)!=0x18 || // No és codi
       dpl > CPL )
    goto excp_gp_sel;
  if ( !SEG_DESC_IS_PRESENT(desc) )
    { EXCEPTION_SEL ( EXCP_NP, selector ); return false; }
  
  // Comprova privilegi destí.
  conforming= (stype&0x04)!=0;
  param_count= (uint8_t) CG_DESC_GET_PARAMCOUNT ( *desc_gate );
  offset= CG_DESC_GET_OFFSET ( *desc_gate );
  if ( !conforming && dpl < CPL )
    {
      if ( !call_far_protected_gate_more_pl ( jit, selector, offset,
                                              &desc, is32, param_count,
                                              next_EIP ) )
        return false;
    }
  else
    {
      printf("[CAL IMPLEMENTAR] "
             "call_far_protected_gate - SAME PRIVILEGE\n");
      exit(EXIT_FAILURE);
    }
  
  return true;
  
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return false;
 excp_gp_gate:
  EXCEPTION_SEL ( EXCP_GP, selector_gate );
  return false;
  
} // end call_far_protected_gate


static bool
call_jmp_far_protected (
                        IA32_JIT       *jit,
                        const uint16_t  selector,
                        const uint32_t  offset,
                        const bool      op32,
                        const bool      is_call,
                        const uint32_t  next_EIP
                        )
{

  seg_desc_t desc;
  uint32_t stype;

  
  // Null Segment Selector checking.
  if ( (selector&0xFFFC) == 0 )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; };
  
  // Llig.
  if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
    return false;
  
  // Type Checking.
  // NOTA!!! Assumisc EFER.LMA==0, aquest bi sols té sentit en 64bits.
  stype= SEG_DESC_GET_STYPE ( desc );
  if ( (stype&0x18) == 0x18 ) // Segments no especials de codi.
    {
      if ( !call_jmp_far_protected_code ( jit, selector, offset,
                                          stype, &desc, op32, is_call,
                                          next_EIP ) )
        return false;
    }
  else if ( (stype&0x17) == 0x04 ) // Call gates (16 i 32 bits).
    {
      if ( is_call )
        {
          if ( !call_far_protected_gate ( jit, selector,
                                          &desc, stype==0x0c,
                                          next_EIP ) )
            return false;
        }
      else
        {
          printf ( "[CAL IMPLEMENTAR] call_jmp_far_protected - JMP GATE\n" );
          exit ( EXIT_FAILURE );
        }
    }
  else if ( (stype&0x1F) == 0x05 ) // Task Gate.
    {
      printf ( "[EE] jmp_far_protected not implemented for task gates" );
      exit ( EXIT_FAILURE );
    }
  else if ( (stype&0x15) == 0x01 ) // TSS (16 i 32 bits).
    {
      printf ( "[EE] jmp_far_protected not implemented for TSS" );
      exit ( EXIT_FAILURE );
    }
  else goto excp_sel_gp;

  return true;

 excp_sel_gp:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return false;
  
} // end call_jmp_far_protected


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
static bool
jmp_far_nonprotected (
                      IA32_JIT       *jit,
                      const uint16_t  selector,
                      const uint32_t  offset
                      )
{

  if ( offset > 0xFFFF )
    {
      WW ( UDATA, "jmp_far_nonprotected -> no és cumpleix la condició"
           " (offset > 0xFFFF). Aquesta condició és una interpretació"
           " de (IF tempEIP is beyond code segment limit THEN #GP(0); FI;)."
           " Cal revisar" );
      EXCEPTION0 ( EXCP_GP );
      return false;
    }
  P_CS->v= selector;
  P_CS->h.lim.addr= ((uint32_t) selector)<<4;
  P_CS->h.lim.firstb= 0;
  P_CS->h.lim.lastb= 0xFFFF;
  P_CS->h.is32= false;
  P_CS->h.readable= true;
  P_CS->h.writable= true;
  P_CS->h.executable= true;
  P_CS->h.isnull= false;
  P_CS->h.data_or_nonconforming= false;
  EIP= offset; 
  
  return true;
  
} // end jmp_far_nonprotected



static bool
jmp_far (
         IA32_JIT       *jit,
         const uint16_t  selector,
         const uint32_t  offset,
         const bool      op32
         )
{
  return
    ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG)) ?
    call_jmp_far_protected ( jit, selector, offset, op32, false, 0 ) :
    jmp_far_nonprotected ( jit, selector, offset );
} // end jmp_far


static bool
jmp_near (
          IA32_JIT       *jit,
          const uint32_t  offset
          )
{
  
  if ( offset < P_CS->h.lim.firstb || offset > P_CS->h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_GP );
      return false;
    }
  EIP= offset;

  return true;
  
} // end jmp_near


static bool
set_sreg (
          IA32_JIT             *jit,
          const uint16_t        selector,
          IA32_SegmentRegister *reg
          )
{

  uint32_t stype,dpl,cpl,rpl;
  seg_desc_t desc;

  
  // Carrega en mode protegit
  if ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG) )
    {
      // --> No es pot carregar el CS
      if ( reg == P_CS ) goto ud_exc;
      else if ( reg == P_SS ) // El SS és especial
        {
          if ( (selector&0xFFFC) == 0 ) goto excp_gp_sel; // Null Segment
          if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
            return false;
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
            goto excp_gp_sel;
          // Segment-present flag.
          if ( !SEG_DESC_IS_PRESENT ( desc ) )
            {
              EXCEPTION_SEL ( EXCP_SS, selector );
              return false;
            }
          // Carrega
          load_segment_descriptor ( P_SS, selector, &desc );
          set_segment_descriptor_access_bit ( jit, &desc );
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
              if ( !read_segment_descriptor ( jit, selector, &desc,
                                              true ) )
                return false;
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
                goto excp_gp_sel;
              // Segment-present flag.
              if ( !SEG_DESC_IS_PRESENT ( desc ) )
                {
                  EXCEPTION_SEL ( EXCP_NP, selector );
                  return false;
                }
              // Carrega
              load_segment_descriptor ( reg, selector, &desc );
              set_segment_descriptor_access_bit ( jit, &desc );
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
  if ( reg == P_SS ) jit->_inhibit_interrupt= true;
  
  return true;

 ud_exc:
  EXCEPTION ( EXCP_UD );
  return false;
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return false;
  
} // end set_sreg
#pragma GCC diagnostic pop


// Torna cert si té permís.
static bool
check_io_permission_bit_map (
                             IA32_JIT       *jit,
                             const uint16_t  port,
                             const int       nbits
                             )
{

  uint32_t addr;
  uint16_t offset;
  uint8_t bmap;
  int n,bit;
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  // I/O Map Base
  if ( !l_TR.h.tss_is32 )
    {
      WW ( UDATA, "check_io_permission_bit_map - TSS 16bit no"
           " suporta I/O permission bit map" );
      return false;
    }
  if ( l_TR.h.lim.lastb < 103 ) return false;
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 102,
                           &offset, true, true ) != 0 )
    return false;

  // Comprova els bits
  bit= port&0x7;
  addr= ((uint32_t) offset) + (port>>3);
  if ( addr < l_TR.h.lim.firstb || addr > l_TR.h.lim.lastb  ) return false;
  if ( jit->_mem_readl8 ( jit, l_TR.h.lim.addr + addr, &bmap, true ) != 0 )
    return false;
  for ( n= 0; n < nbits; ++n )
    {
      if ( bmap&(1<<bit) ) return false; // bit 1 indica que no es pot
      if ( ++bit == 8 && n != nbits-1 )
        {
          ++addr;
          bit= 0;
          if ( addr < l_TR.h.lim.firstb || addr > l_TR.h.lim.lastb  )
            return false;
          if ( jit->_mem_readl8 ( jit, l_TR.h.lim.addr + addr,
                                  &bmap, true ) != 0 )
            return false;
        }
    }
  
  return true;
  
} // end check_io_permission_bit_map


static bool
io_check_permission (
                     IA32_JIT       *jit,
                     const uint16_t  port,
                     const int       nbits
                     )
{

  bool ret;

  
  if ( PROTECTED_MODE_ACTIVATED && (CPL > IOPL || (EFLAGS&VM_FLAG)) )
    {
      if ( check_io_permission_bit_map ( jit, port, nbits ) )
        ret= true;
      else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); ret= false; }
    }
  else ret= true;
  
  return ret;
  
} // end io_check_permission


static bool
check_seg_level0 (
                  IA32_JIT *jit
                  )
{

  bool ret;

  
  if ( CPL == 0 || // <-- Nivel seguritat 0!!!
       (!(PROTECTED_MODE_ACTIVATED) && !(EFLAGS&VM_FLAG)) )
    ret= true;
  else { EXCEPTION0 ( EXCP_GP ); ret= false; }
  
  return ret;
  
} // end check_seg_level0


static bool
check_seg_level0_novm (
                       IA32_JIT *jit
                       )
{
  
  bool ret;


  if ( !(PROTECTED_MODE_ACTIVATED) || ((EFLAGS&VM_FLAG)==0 && CPL==0) )
    ret= true;
  else { EXCEPTION0 ( EXCP_GP ); ret= false; }
  
  return ret;
  
} // end check_seg_level0_novm


static bool
iret_protected_same_pl (
                        IA32_JIT       *jit,
                        const bool      op32,
                        const uint16_t  selector,
                        const uint32_t  tmp_eflags
                        )
{

  seg_desc_t desc;
  IA32_SegmentRegister sreg;
  uint32_t eflags_mask;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // RETURN-TO-SAME-PRIVILEGE-LEVEL: (* PE = 1, RPL = CPL *)

  // Llig descriptor i comprova limits.
  if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
    return false;
  load_segment_descriptor ( &sreg, selector, &desc );
  if ( l_EIP < sreg.h.lim.firstb || l_EIP > sreg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }

  // Fixa EFLAGS
  eflags_mask= (CF_FLAG|PF_FLAG|AF_FLAG|ZF_FLAG|SF_FLAG|
                TF_FLAG|DF_FLAG|OF_FLAG|NT_FLAG);
  if ( op32 ) eflags_mask|= (RF_FLAG|AC_FLAG|ID_FLAG);
  if ( CPL <= IOPL ) eflags_mask|= IF_FLAG;
  if ( CPL == 0 )
    {
      eflags_mask|= IOPL_FLAG;
      if ( op32 ) eflags_mask|= (VIF_FLAG|VIP_FLAG);
    }
  l_EFLAGS= (l_EFLAGS&(~eflags_mask))|(tmp_eflags&eflags_mask)|EFLAGS_1S;

  // Copia sreg
  *(l_P_CS)= sreg;

  return true;
  
} // end iret_protected_same_pl


static bool
iret_protected_outer_pl (
                         IA32_JIT       *jit,
                         const bool      op32,
                         const uint16_t  selector,
                         const uint32_t  tmp_eflags
                         )
{

  seg_desc_t desc;
  IA32_SegmentRegister sreg;
  uint32_t eflags_mask,tmp32_SS,tmp_ESP,new_SS_stype,rpl,rpl_SS,dpl_SS;
  uint16_t tmp16,new_SS_selector;
  seg_desc_t new_SS_desc;
  IA32_SegmentRegister new_SS;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // IMPORTANT!!! EN LA DOCUMENTACIÓ FALTA UNA PART. LA DE TORNAR A
  // LLEGIR LA PILA.
  if ( op32 )
    {
      if ( !pop32 ( jit, &tmp_ESP ) ) return false;
      if ( !pop32 ( jit, &tmp32_SS ) ) return false;
    }
  else
    {
      if ( !pop16 ( jit, &tmp16 ) ) return false;
      tmp_ESP= (uint16_t) tmp16;
      if ( !pop16 ( jit, &tmp16 ) ) return false;
      tmp32_SS= (uint16_t) tmp16;
    }
  
  // Llig descriptor i comprova limits.
  if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
    return false;
  load_segment_descriptor ( &sreg, selector, &desc );
  if ( l_EIP < sreg.h.lim.firstb || l_EIP > sreg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
  
  // Fixa EFLAGS
  eflags_mask= (CF_FLAG|PF_FLAG|AF_FLAG|ZF_FLAG|SF_FLAG|
                TF_FLAG|DF_FLAG|OF_FLAG|NT_FLAG);
  if ( op32 ) eflags_mask|= (RF_FLAG|AC_FLAG|ID_FLAG);
  if ( CPL <= IOPL ) eflags_mask|= IF_FLAG;
  if ( CPL == 0 )
    {
      eflags_mask|= IOPL_FLAG;
      if ( op32 ) eflags_mask|= (VM_FLAG|VIF_FLAG|VIP_FLAG);
    }
  l_EFLAGS= (l_EFLAGS&(~eflags_mask))|(tmp_eflags&eflags_mask)|EFLAGS_1S;
  
  // Actualitza CPL
  sreg.h.pl= (uint8_t) (selector&0x3); // CPL= rpl

  // Carrega SS (sense comprovacions?????? No té sentit!!!!)
  new_SS_selector= (uint16_t) (tmp32_SS&0xFFFF);
  if ( !read_segment_descriptor ( jit, new_SS_selector, &new_SS_desc, true ) )
    return false;
  // Fique algunes comprovacions que he tret de ret_outer_pl
  new_SS_stype= SEG_DESC_GET_STYPE ( new_SS_desc );
  rpl= selector&0x3;
  rpl_SS= new_SS_selector&0x3;
  dpl_SS= SEG_DESC_GET_DPL ( new_SS_desc );
  if ( rpl != rpl_SS ||
       rpl != dpl_SS ||
       (new_SS_stype&0x1A)!=0x12 // no és data writable
       )
    { EXCEPTION_SEL ( EXCP_GP, selector ); return false; }
  if ( !SEG_DESC_IS_PRESENT ( new_SS_desc ) )
    { EXCEPTION_SEL ( EXCP_SS, new_SS_selector ); return false; }
  /* // CALDRIA COMPROVAR ALGUNA COSA??!!!!!!!
  // Comprovacions newSS addicionals i carrega'l
  new_SS_rpl= new_SS_selector&0x3;
  if ( new_SS_rpl != dpl ) goto excp_ts_new_ss_error;
  new_SS_dpl= SEG_DESC_GET_DPL(new_SS_desc);
  new_SS_stype= SEG_DESC_GET_STYPE ( new_SS_desc );
  if ( new_SS_dpl != dpl ||
       (new_SS_stype&0x1A) != 0x12 || // no és data, writable
       !SEG_DESC_IS_PRESENT(new_SS_desc) )
    goto excp_ts_new_ss_error;
  */
  load_segment_descriptor ( &new_SS, new_SS_selector, &new_SS_desc );
  
  // Fixa nova pila.
  *(l_P_SS)= new_SS;
  l_ESP= tmp_ESP;
  
  // Comprova DPL segments.
  // --> ES
  if ( l_P_ES->h.dpl < sreg.h.pl && l_P_ES->h.data_or_nonconforming )
    { l_P_ES->h.isnull= true; l_P_ES->v&= 0x0003; }
  // --> FS
  if ( l_P_FS->h.dpl < sreg.h.pl && l_P_FS->h.data_or_nonconforming )
    { l_P_FS->h.isnull= true; l_P_FS->v&= 0x0003; }
  // --> GS
  if ( l_P_GS->h.dpl < sreg.h.pl && l_P_GS->h.data_or_nonconforming )
    { l_P_GS->h.isnull= true; l_P_GS->v&= 0x0003; }
  // --> DS
  if ( l_P_DS->h.dpl < sreg.h.pl && l_P_DS->h.data_or_nonconforming )
    { l_P_DS->h.isnull= true; l_P_DS->v&= 0x0003; }
  
  // Copia sreg
  *(l_P_CS)= sreg;

  return true;
  
} // end iret_protected_outer_pl


static bool
iret_to_virtual8086_mode (
                          IA32_JIT       *jit,
                          const uint16_t  selector,
                          const uint32_t  tmp_eflags
                          )
{

  uint32_t tmp32,tmp32_SS,tmp_ESP;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // RETURN-TO-VIRTUAL-8086-MODE:
  // (* Interrupted procedure was in virtual-8086 mode:
  //    PE = 1, CPL=0, VM = 1 in flag image *)

  // NOTA!!!! No queda gens clar si al carregar els selectors es
  // consulta el GDT o directament es carrega com si fora mode
  // real. Vaig a assumir el segon perquè amb el primer tinc errors
  // raros.

  // Fica CS
  if ( l_EIP < l_P_CS->h.lim.firstb || l_EIP > l_P_CS->h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
  l_P_CS->v= (uint16_t) (selector&0xFFFF);
  l_P_CS->h.lim.addr= ((uint32_t) l_P_CS->v)<<4;
  l_P_CS->h.lim.firstb= 0;
  l_P_CS->h.lim.lastb= 0xFFFF;
  l_P_CS->h.pl= 3; // Es fixa
  l_P_CS->h.is32= false;
  l_P_CS->h.readable= true;
  l_P_CS->h.writable= true;
  l_P_CS->h.executable= true;
  l_P_CS->h.isnull= false;
  l_P_CS->h.data_or_nonconforming= false;
  
  // NOTA!!! SI HO HE ENTÈS BÉ ACÍ ENS CARREGUEM EL DESCRIPTOR
  // CACHE. NO VAL EL UNREAL MODE.
  // Fixa EFLAGS i carrega registres
  l_EFLAGS= tmp_eflags | EFLAGS_1S;
  if ( !pop32 ( jit, &tmp_ESP) ) return false;
  // --> SS
  if ( !pop32 ( jit, &tmp32_SS) ) return false;
  // --> ES
  if ( !pop32 ( jit, &tmp32) ) return false;
  l_P_ES->v= (uint16_t) (tmp32&0xFFFF);
  l_P_ES->h.lim.addr= ((uint32_t) l_P_ES->v)<<4;
  l_P_ES->h.lim.firstb= 0;
  l_P_ES->h.lim.lastb= 0xFFFF;
  l_P_ES->h.is32= false;
  l_P_ES->h.readable= true;
  l_P_ES->h.writable= true;
  l_P_ES->h.executable= true;
  l_P_ES->h.isnull= false;
  l_P_ES->h.data_or_nonconforming= true;
  // --> DS
  if ( !pop32 ( jit, &tmp32) ) return false;
  l_P_DS->v= (uint16_t) (tmp32&0xFFFF);
  l_P_DS->h.lim.addr= ((uint32_t) l_P_DS->v)<<4;
  l_P_DS->h.lim.firstb= 0;
  l_P_DS->h.lim.lastb= 0xFFFF;
  l_P_DS->h.is32= false;
  l_P_DS->h.readable= true;
  l_P_DS->h.writable= true;
  l_P_DS->h.executable= true;
  l_P_DS->h.isnull= false;
  l_P_DS->h.data_or_nonconforming= true;
  // --> FS
  if ( !pop32 ( jit, &tmp32) ) return false;
  l_P_FS->v= (uint16_t) (tmp32&0xFFFF);
  l_P_FS->h.lim.addr= ((uint32_t) l_P_FS->v)<<4;
  l_P_FS->h.lim.firstb= 0;
  l_P_FS->h.lim.lastb= 0xFFFF;
  l_P_FS->h.is32= false;
  l_P_FS->h.readable= true;
  l_P_FS->h.writable= true;
  l_P_FS->h.executable= true;
  l_P_FS->h.isnull= false;
  l_P_FS->h.data_or_nonconforming= true;
  // --> GS
  if ( !pop32 ( jit, &tmp32) ) return false;
  l_P_GS->v= (uint16_t) (tmp32&0xFFFF);
  l_P_GS->h.lim.addr= ((uint32_t) l_P_GS->v)<<4;
  l_P_GS->h.lim.firstb= 0;
  l_P_GS->h.lim.lastb= 0xFFFF;
  l_P_GS->h.is32= false;
  l_P_GS->h.readable= true;
  l_P_GS->h.writable= true;
  l_P_GS->h.executable= true;
  l_P_GS->h.isnull= false;
  l_P_GS->h.data_or_nonconforming= true;
  // Acaba SS i ESP
  l_P_SS->v= (uint16_t) (tmp32_SS&0xFFFF);
  l_P_SS->h.lim.addr= ((uint32_t) l_P_SS->v)<<4;
  l_P_SS->h.lim.firstb= 0;
  l_P_SS->h.lim.lastb= 0xFFFF;
  l_P_SS->h.is32= false;
  l_P_SS->h.readable= true;
  l_P_SS->h.writable= true;
  l_P_SS->h.executable= true;
  l_P_SS->h.isnull= false;
  l_P_SS->h.data_or_nonconforming= true;
  l_ESP= tmp_ESP;

  return true;
  
} // end iret_to_virtual8086_mode


static bool
iret_protected (
                IA32_JIT       *jit,
                const bool      op32,
                const uint32_t  old_EIP
                )
{

  uint16_t selector,tmp16;
  uint32_t tmp32,tmp_eflags;
  uint8_t rpl;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // GOTO TASK-RETURN; (* PE = 1, VM = 0, NT = 1 *)
  if ( l_EFLAGS&NT_FLAG )
    {
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr,
                               &selector, true, true ) != 0 )
        return false;
      if ( !switch_task ( jit, SWITCH_TASK_IRET, selector, 0, old_EIP ) )
        return false;
      if ( l_EIP < l_P_CS->h.lim.firstb || l_EIP > l_P_CS->h.lim.lastb )
        { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
    }
  else
    {
      // Recupera EIP i CS
      if ( op32 )
        {
          if ( !pop32 ( jit, &l_EIP) ) return false;
          if ( !pop32 ( jit, &tmp32) ) return false;
          selector= (uint16_t) (tmp32&0xFFFF);
          if ( !pop32 ( jit, &tmp_eflags) ) return false;
        }
      else
        {
          if ( !pop16 ( jit, &tmp16 ) ) return false;
          l_EIP= (uint32_t) tmp16;
          if ( !pop16 ( jit, &selector ) ) return false;
          if ( !pop16 ( jit, &tmp16 ) ) return false;
          tmp_eflags= (uint32_t) tmp16;
        }

      // NOTA!!!! No carregue el selector encara!!! Ho deixe per al final
      // GOTO RETURN-TO-VIRTUAL-8086-MODE;
      if ( tmp_eflags&VM_FLAG && CPL == 0 )
        {
          if ( !iret_to_virtual8086_mode ( jit, selector, tmp_eflags ) )
            return false;
        }
      // GOTO PROTECTED-MODE-RETURN;
      else
        {
          rpl= selector&0x3;
          if ( rpl > CPL )
            {
              if ( !iret_protected_outer_pl ( jit, op32, selector,tmp_eflags ) )
                return false;
            }
          else
            {
              if ( !iret_protected_same_pl ( jit, op32, selector, tmp_eflags ) )
                return false;
            }
        }
    }

  return true;
  
} // end iret_protected


static bool
iret_real (
           IA32_JIT   *jit,
           const bool  op32
           )
{

  uint32_t tmp_eip,tmp32;
  uint16_t tmp16;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;

  // Llig valors
  if ( op32 )
    {
      if ( !pop32 ( jit, &tmp_eip ) ) return false;
      if ( !pop32 ( jit, &tmp32 ) ) return false;
      l_P_CS->v= (uint16_t) (tmp32&0xFFFF);
      l_P_CS->h.lim.addr= ((uint32_t) P_CS->v)<<4;
      if ( !pop32 ( jit, &tmp32 ) ) return false;
      l_EFLAGS= (tmp32&0x257FD5) | (l_EFLAGS&0x1A0000) | EFLAGS_1S;
    }
  else
    {
      if ( !pop16 ( jit, &tmp16 ) ) return false;
      tmp_eip= (uint32_t) tmp16;
      if ( !pop16 ( jit, &(l_P_CS->v) ) ) return false;
      l_P_CS->h.lim.addr= ((uint32_t) l_P_CS->v)<<4;
      if ( !pop16 ( jit, &tmp16 ) ) return false;
      l_EFLAGS= (l_EFLAGS&0xFFFF0000) | ((uint32_t) tmp16) | EFLAGS_1S;
    }
  l_P_CS->h.lim.firstb= 0;
  l_P_CS->h.lim.lastb= 0xFFFF;
  l_P_CS->h.is32= false;
  l_P_CS->h.readable= true;
  l_P_CS->h.writable= true;
  l_P_CS->h.executable= true;
  l_P_CS->h.isnull= false;
  l_P_CS->h.data_or_nonconforming= false;

  // Comprovacions finals.
  if ( tmp_eip > 0xFFFF )
    {
      EXCEPTION ( EXCP_GP );
      return false;
    }
  l_EIP= tmp_eip;
  
  return true;
  
} // end iret_real


static bool
iret_v86 (
          IA32_JIT   *jit,
          const bool  op32
          )
{
  
  uint32_t tmp_eip,tmp32,emask;
  uint16_t tmp16;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;

  // Comprovació IOPL
  if ( l_IOPL != 3 ) { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
  
  // Llig valors
  if ( op32 )
    {
      if ( !pop32 ( jit, &tmp_eip ) ) return false;
      if ( !pop32 ( jit, &tmp32 ) ) return false;
      l_P_CS->v= (uint16_t) (tmp32&0xFFFF);
      l_P_CS->h.lim.addr= ((uint32_t) P_CS->v)<<4;
      if ( !pop32 ( jit, &tmp32 ) ) return false;
      emask= VM_FLAG|IOPL_FLAG|VIP_FLAG|VIF_FLAG;
      l_EFLAGS= (tmp32&(~emask)) | (l_EFLAGS&emask) | EFLAGS_1S;
    }
  else
    {
      if ( !pop16 ( jit, &tmp16 ) ) return false;
      tmp_eip= (uint32_t) tmp16;
      if ( !pop16 ( jit, &(l_P_CS->v) ) ) return false;
      l_P_CS->h.lim.addr= ((uint32_t) l_P_CS->v)<<4;
      if ( !pop16 ( jit, &tmp16 ) ) return false;
      l_EFLAGS=
        (l_EFLAGS&(0xFFFF0000|IOPL_FLAG)) |
        ((uint32_t) (tmp16&(~IOPL_FLAG))) |
        EFLAGS_1S;
    }
  l_P_CS->h.lim.firstb= 0;
  l_P_CS->h.lim.lastb= 0xFFFF;
  l_P_CS->h.is32= false;
  l_P_CS->h.readable= true;
  l_P_CS->h.writable= true;
  l_P_CS->h.executable= true;
  l_P_CS->h.isnull= false;
  l_P_CS->h.data_or_nonconforming= false;

  // Comprovacions finals.
  if (  tmp_eip > 0xFFFF )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
  l_EIP= tmp_eip;

  return true;
  
} // end iret_v86


static bool
iret (
      IA32_JIT       *jit,
      const bool      op32,
      const uint32_t  old_EIP
      )
{

  bool ret;
  IA32_CPU *l_cpu;

  
  l_cpu= jit->_cpu;
  if ( !PROTECTED_MODE_ACTIVATED ) ret= iret_real ( jit, op32 );
  else
    {
      if ( l_EFLAGS&VM_FLAG ) ret= iret_v86 ( jit, op32 );
      else                    ret= iret_protected ( jit, op32, old_EIP );
    }
  
  return ret;
  
} // end iret


static bool
call_far_nonprotected (
                       IA32_JIT       *jit,
                       const uint16_t  selector,
                       const uint32_t  offset,
                       const bool      op32,
                       const uint32_t  next_EIP
                       )
{

  uint32_t tmp;
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  
  
  // NOTA!!! quan operand_size és 16, els bits superiors de offset
  // estan a 0!!
  if ( op32 )
    {
      fprintf(FERROR,"[EE] CAL IMPLEMENTAR call_far32_nonprotected\n");
      exit(EXIT_FAILURE);
      /*
      // Comprove abans de fer cap push (els push ho tornen a
      // comprovar)
      tmp= P_SS->h.is32 ? (ESP-6) : ((uint32_t) (SP-6));
      if ( tmp > 0xFFFA ) { EXCEPTION ( EXCP_SS ); return -1; }
      if ( offset > 0xFFFF ) { EXCEPTION ( EXCP_GP ); return -1; }
      PUSHW ( P_CS->v, return -1 );
      PUSHD ( EIP, return -1 );
      */
    }
  else
    {
      // Comprove abans de fer cap push (els push ho tornen a
      // comprovar)
      tmp= l_P_SS->h.is32 ? (l_ESP-4) : ((uint32_t) ((uint16_t) (l_SP-4)));
      if ( tmp > 0xFFFC ) { EXCEPTION ( EXCP_SS ); return false; }
      if ( !push16 ( jit, l_P_CS->v ) ) return false;
      if ( !push16 ( jit, ((uint16_t) (next_EIP&0xFFFF)) ) ) return false;
    }
  l_P_CS->v= selector;
  l_P_CS->h.lim.addr= ((uint32_t) selector)<<4;
  l_P_CS->h.lim.firstb= 0;
  l_P_CS->h.lim.lastb= 0xFFFF;
  l_P_CS->h.is32= false;
  l_P_CS->h.readable= true;
  l_P_CS->h.writable= true;
  l_P_CS->h.executable= true;
  l_P_CS->h.isnull= false;
  l_P_CS->h.data_or_nonconforming= false;
  l_EIP= offset; 
  
  return true;
  
} // end call_far_nonprotected


static bool
call_far (
          IA32_JIT       *jit,
          const uint16_t  selector,
          const uint32_t  offset,
          const bool      op32,
          const uint32_t  next_EIP
          )
{
  return
    ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG)) ?
    call_jmp_far_protected ( jit, selector, offset, op32, true, next_EIP ) :
    call_far_nonprotected ( jit, selector, offset, op32, next_EIP );
} // end call_far


static bool
ret_far_protected_same_pl (
                           IA32_JIT       *jit,
                           const uint16_t  selector,
                           seg_desc_t      desc,
                           const bool      op32
                           )
{

  uint32_t offset,tmp_eip;
  uint16_t tmp16;
  IA32_SegmentRegister sreg;
  IA32_CPU *l_cpu;

  
  l_cpu= jit->_cpu;

  // Carrega segment, llig EIP sense fer POP i comprova que està en
  // els límits.
  offset= l_P_SS->h.is32 ? l_ESP : ((uint32_t) ((uint16_t) l_SP));
  if ( op32 )
    {
      READLD ( l_P_SS->h.lim.addr + offset, &tmp_eip,
               return false, true, false );
    }
  else
    {
      READLW ( l_P_SS->h.lim.addr + offset, &tmp16, return false, true, false );
      tmp_eip= (uint32_t) tmp16;
    }
  load_segment_descriptor ( &sreg, selector, &desc );
  if ( tmp_eip < sreg.h.lim.firstb || tmp_eip > sreg.h.lim.lastb )
    goto excp_gp;

  // Actualitza pila i valors.
  if ( op32 )
    {
      if ( l_P_SS->h.is32 ) l_ESP+= 8;
      else                  l_SP+= 8;
    }
  else
    {
      if ( P_SS->h.is32 ) l_ESP+= 4;
      else                l_SP+= 4;
    }
  l_EIP= tmp_eip;
  set_segment_descriptor_access_bit ( jit, &desc );
  *(l_P_CS)= sreg;
  
  return true;

 excp_gp:
  EXCEPTION_ERROR_CODE ( EXCP_GP, 0 );
  return false;
      
} // end ret_far_protected_same_pl


static bool
ret_far_protected (
                   IA32_JIT   *jit,
                   const bool  op32
                   )
{

  uint32_t last_byte,tmp,offset,stype,dpl,rpl;
  uint16_t selector;
  seg_desc_t desc;
  IA32_CPU *l_cpu;

  
  l_cpu= jit->_cpu;
  
  // Comprovacions pila i llig selector sense fer POP.
  offset= l_P_SS->h.is32 ? l_ESP : ((uint32_t) ((uint16_t) l_SP));
  if ( op32 )
    {
      last_byte=
        l_P_SS->h.is32 ? (l_ESP+8-1) : ((uint32_t) ((uint16_t) (l_SP+8-1)));
      if ( last_byte < l_P_SS->h.lim.firstb ||
           last_byte > (l_P_SS->h.lim.lastb) )
        goto excp_ss;
      READLD ( l_P_SS->h.lim.addr + offset + 4, &tmp,
               return false, true, false );
      selector= (uint16_t) (tmp&0xFFFF);
    }
  else
    {
      last_byte=
        l_P_SS->h.is32 ? (l_ESP+4-1) : ((uint32_t) ((uint16_t) (l_SP+4-1)));
      if ( last_byte < l_P_SS->h.lim.firstb ||
           last_byte > (l_P_SS->h.lim.lastb) )
        goto excp_ss;
      READLW ( l_P_SS->h.lim.addr + offset + 2, &selector,
               return false, true, false );
    }
  // Comprovacions del descriptor.
  // --> NULL segment
  if ( (selector&0xFFFC) == 0 )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return false; }
  // --> Límits
  if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
    return false;
  // --> Code segment
  stype= SEG_DESC_GET_STYPE ( desc );
  if ( (stype&0x18) != 0x18 ) goto excp_gp_sel;
  // --> Comprovacions PL
  rpl= selector&0x3;
  dpl= SEG_DESC_GET_DPL ( desc );
  if ( rpl < CPL ||
       ((stype&0x1c) == 0x1c && dpl > rpl) || // conforming && DPL > RPL
       ((stype&0x1c) == 0x18 && dpl != rpl) ) // non-conforming && DPL!=RPL
    goto excp_gp_sel;
  // Segment is present
  if ( !SEG_DESC_IS_PRESENT ( desc ) )
    { EXCEPTION_SEL ( EXCP_NP, selector ); return false; }
  
  // Torna en funció privilegi
  if ( rpl > CPL )
    {
      printf("[EE] ret_far_protected - RETURN-OUTER-PRIVILEGE-LEVEL!\n");
      exit ( EXIT_FAILURE );
    }
  else
    {
      if ( !ret_far_protected_same_pl ( jit, selector, desc, op32 ) )
        return -1;
    }
  
  return true;
  
 excp_ss:
  EXCEPTION_ERROR_CODE ( EXCP_SS, 0 );
  return false;
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return false;
  
} // end ret_far_protected


static bool
ret_far_nonprotected (
                      IA32_JIT   *jit,
                      const bool  op32
                      )
{
  
  uint32_t tmp;
  uint16_t selector,tmp16;
  IA32_CPU *l_cpu;


  l_cpu= jit->_cpu;
  
  // NOTA!! FAig comprovacions abans de fer el POP. Encara que són
  // redundants.
  tmp= l_P_SS->h.is32 ? l_ESP : ((uint32_t) l_SP);
  if ( op32 )
    {
      fprintf(FERROR,"[EE] CAL IMPLEMENTAR ret_far32_nonprotected\n");
      exit(EXIT_FAILURE);
      /*
      if ( tmp > 0xFFF8 ) { EXCEPTION ( EXCP_SS ); return -1; }
      POPD ( &EIP, return -1 );
      POPD ( &tmp, return -1 );
      selector= (uint16_t) tmp;
      */
    }
  else
    {
      if ( tmp > 0xFFFC ) { EXCEPTION ( EXCP_SS ); return false; }
      // NOTA!!! Ací la documentació és un poc contradictòria,
      // assumisc que es desa 1 paraula.
      if ( !pop16 ( jit, &tmp16 ) ) return false;
      if ( !pop16 ( jit, &selector ) ) return false;
      // NOTA!! Fa una serie de comprovacions que no tenen molt de
      // sentit. Si carrega 1 paraula no té sentit fer comprovacions.
      l_EIP= (uint32_t) tmp16;
    }
  l_P_CS->v= selector;
  l_P_CS->h.lim.addr= ((uint32_t) selector)<<4;
  l_P_CS->h.lim.firstb= 0;
  l_P_CS->h.lim.lastb= 0xFFFF;
  l_P_CS->h.is32= false;
  l_P_CS->h.readable= true;
  l_P_CS->h.writable= true;
  l_P_CS->h.executable= true;
  l_P_CS->h.isnull= false;
  l_P_CS->h.data_or_nonconforming= false;
  
  return true;
  
} // end call_far_nonprotected


static bool
ret_far (
         IA32_JIT   *jit,
         const bool  op32
         )
{
  return
    ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG)) ?
    ret_far_protected ( jit, op32 ) :
    ret_far_nonprotected ( jit, op32 );
} // end ret_far


static bool
enter16 (
         IA32_JIT       *jit,
         const uint16_t  alloc_size,
         uint16_t        nesting_level
         )
{
  
  IA32_CPU *l_cpu;
  uint16_t frame_tmp,tmp,i;
  
  
  l_cpu= jit->_cpu;
  nesting_level%= 32;

  // Apila EBP
  if ( !push16 ( jit, l_BP ) ) return false;
  frame_tmp= l_SP;

  // Display per a nestinglevel>0
  for ( i= 1; i < nesting_level; ++i )
    {
      if ( l_P_SS->h.is32 )
        {
          l_EBP-= 2;
          if ( jit->_mem_read16 ( jit, l_P_SS, l_EBP, &tmp, true ) != 0 )
            return false;
        }
      else
        {
          l_BP-= 2;
          if ( jit->_mem_read16 ( jit, l_P_SS,
                                  (uint32_t) l_BP, &tmp, true ) != 0 )
            return false;
        }
      if ( !push16 ( jit, tmp ) ) return false;
    }
  if ( nesting_level > 0 )
    {
      if ( !push16 ( jit, frame_tmp ) ) return false;
    }

  // Reserva memòria.
  l_BP= frame_tmp;
  l_SP-= alloc_size;

  return true;
  
} // end enter16


static bool
enter32 (
         IA32_JIT       *jit,
         const uint16_t  alloc_size,
         uint16_t        nesting_level
         )
{
  
  IA32_CPU *l_cpu;
  uint32_t frame_tmp,tmp;
  uint16_t i;
  
  
  l_cpu= jit->_cpu;
  nesting_level%= 32;

  // Apila EBP
  if ( !push32 ( jit, l_EBP ) ) return false;
  frame_tmp= l_ESP;

  // Display per a nestinglevel>0
  for ( i= 1; i < nesting_level; ++i )
    {
      if ( l_P_SS->h.is32 )
        {
          l_EBP-= 4;
          if ( jit->_mem_read32 ( jit, l_P_SS, l_EBP, &tmp, true ) != 0 )
            return false;
        }
      else
        {
          l_BP-= 4;
          if ( jit->_mem_read32 ( jit, l_P_SS,
                                  (uint32_t) l_BP, &tmp, true ) != 0 )
            return false;
        }
      if ( !push32 ( jit, tmp ) ) return false;
    }
  if ( nesting_level > 0 )
    {
      if ( !push32 ( jit, frame_tmp ) ) return false;
    }
  
  // Reserva memòria.
  l_EBP= frame_tmp;
  l_ESP-= (uint32_t) alloc_size;

  return true;
  
} // end enter32


static bool
lar_check_segment_descriptor (
                              IA32_JIT         *jit,
                              const uint16_t    selector,
                              const seg_desc_t  desc
                              )
{

  uint32_t stype;
  uint8_t rpl,dpl;
  bool ret;

  
  stype= SEG_DESC_GET_STYPE ( desc );
  rpl= selector&0x3;
  dpl= SEG_DESC_GET_DPL(desc);
  if ( rpl > dpl ) ret= false;
  else
    {
      if ( stype&0x10 ) // Code/Data
        ret= ((stype&0xc0)!=0xc0 && CPL>dpl) ? false : true;
      else // System
        {
          switch ( stype )
            {
            case 0x0:
            case 0x6:
            case 0x7:
            case 0x8:
            case 0xa:
            case 0xd:
            case 0xe:
            case 0xf: ret= false;
            default: return true;
            }
        }
    }
  
  return ret;

} // end lar_check_segment_descriptor


static bool
lsl_check_segment_descriptor (
                              IA32_JIT         *jit,
                              const uint16_t    selector,
                              const seg_desc_t  desc
                              )
{

  uint32_t stype;
  uint8_t rpl,dpl;
  bool ret;
  
  
  stype= SEG_DESC_GET_STYPE ( desc );
  rpl= selector&0x3;
  dpl= SEG_DESC_GET_DPL(desc);
  if ( rpl > dpl ) ret= false;
  else
    {
      if ( stype&0x10 ) // Code/Data
        ret= ((stype&0xc0)!=0xc0 && CPL>dpl) ? false : true;
      else // System
        {
          // M'estranya que no coincidisca amb LAR, de fet en una
          // pàgina web fica que deuria de ser com LAR... No
          // entenc. Vaig a fer car al manual.
          switch ( stype )
            {
            case 0x0:
            case 0x4:
            case 0x5:
            case 0x6:
            case 0x7:
            case 0x8:
            case 0xa:
            case 0xc:
            case 0xd:
            case 0xe:
            case 0xf: ret= false;
            default: return true;
            }
        }
    }
  
  return ret;
  
} // end lsl_check_segment_descriptor


static bool
lldt (
      IA32_JIT       *jit,
      const uint16_t  selector
      )
{

  IA32_CPU *l_cpu;
  seg_desc_t desc;
  uint32_t cpl,stype,elimit;
  

  l_cpu= jit->_cpu;
  
  // ATENCIÓ!!!! Es supossa que l'ordre de comprovacions seria:
  // 1.- Limits
  // 2.- Si és vàlid (no NULL), si no ho és parem d'executar
  // 3.- Lectura (Pot generar més execpcions)
  //
  // No tinc clar què fer ací, llançaré un GP.
  if ( selector&0x04 ) goto gp_exc;
  // Però per comoditat vaig a fer 1-3-2.
  if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
    return false;
  if ( (selector&0xFFFC) == 0 ) l_LDTR.h.isnull= true;
  else
    {
      l_LDTR.h.isnull= false;
      cpl= CPL;
      if ( cpl != 0 ) goto gp_exc;
      stype= SEG_DESC_GET_STYPE ( desc );
      if ( (stype&0xf) != 0x02 ) goto excp_gp_sel;
      if ( !SEG_DESC_IS_PRESENT ( desc ) )
        {
          EXCEPTION_SEL ( EXCP_NP, selector );
          return false;
        }
      // Carrega (No faig el que es fa normalment perquè es LDTR, sols
      // el rellevant per a LDTR).
      l_LDTR.h.lim.addr=
        (desc.w1>>16) | ((desc.w0<<16)&0x00FF0000) | (desc.w0&0xFF000000);
      elimit= (desc.w0&0x000F0000) | (desc.w1&0x0000FFFF);
      if ( SEG_DESC_G_IS_SET ( desc ) )
        {
          elimit<<= 12;
          elimit|= 0xFFF;
        }
      l_LDTR.v= selector;
      l_LDTR.h.lim.firstb= 0;
      l_LDTR.h.lim.lastb= elimit;
    }
  
  return true;
  
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return false;
 gp_exc:
  EXCEPTION0 ( EXCP_GP );
  return false;
  
} // end lldt


static void
set_tss_segment_descriptor_busy_bit (
                                     IA32_JIT   *jit,
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
                fprintf ( FERROR, "Avís: excepció en WRITELD en"
                          " ''set_tss_segment_descriptor_busy_bit\n" ) );
    }
  
} // end set_tss_segment_descriptor_busy_bit


static bool
ltr (
     IA32_JIT       *jit,
     const uint16_t  selector
     )
{

  IA32_CPU *l_cpu;
  seg_desc_t desc;
  uint32_t cpl,stype,elimit;
  

  l_cpu= jit->_cpu;
  
  // Null selector
  if ( (selector&0xFFFC) == 0 ) goto gp_exc;

  // Tipus no global
  if ( selector&0x04 ) goto excp_gp_sel;

  // Llig
  if ( !read_segment_descriptor ( jit, selector, &desc, true ) )
    return false;
  
  // Ompli valors
  l_TR.h.isnull= false;
  cpl= CPL;
  if ( cpl != 0 ) goto gp_exc;
  stype= SEG_DESC_GET_STYPE ( desc );
  if ( (stype&0x17) != 0x01 ) goto excp_gp_sel; // Available TSS
  if ( !SEG_DESC_IS_PRESENT ( desc ) )
    {
      EXCEPTION_SEL ( EXCP_NP, selector );
      return false ;
    }
  // Carrega (No faig el que es fa normalment perquè es TR, sols
  // el rellevant per a TR).
  l_TR.h.is32= SEG_DESC_B_IS_SET ( desc );
  l_TR.h.tss_is32= ((stype&0x1D)==0x09);
  l_TR.h.pl= (uint8_t) (selector&0x3);
  l_TR.h.lim.addr=
    (desc.w1>>16) | ((desc.w0<<16)&0x00FF0000) | (desc.w0&0xFF000000);
  elimit= (desc.w0&0x000F0000) | (desc.w1&0x0000FFFF);
  if ( SEG_DESC_G_IS_SET ( desc ) )
    {
      elimit<<= 12;
      elimit|= 0xFFF;
    }
  l_TR.v= selector;
  l_TR.h.lim.firstb= 0;
  l_TR.h.lim.lastb= elimit;

  // Fixa busy.
  set_tss_segment_descriptor_busy_bit ( jit, &desc );
  
  return true;
  
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return false;
 gp_exc:
  EXCEPTION0 ( EXCP_GP );
  return false;
  
} // end ltr


static bool
verr_verw (
           IA32_JIT       *jit,
           const uint16_t  selector,
           const bool      check_read
           )
{

  IA32_CPU *l_cpu;
  uint32_t off,dpl,cpl,rpl,stype;
  seg_desc_t desc;
  
  
  l_cpu= jit->_cpu;
  
  // Comprovació
  l_EFLAGS&= ~ZF_FLAG;
  if ( (selector&0xFFFC) == 0 ) return true; // NULL selector
  // --> Comprova offset
  off= (uint32_t) (selector&0xFFF8);
  if ( selector&0x4 ) // LDT
    {
      if ( off < l_LDTR.h.lim.firstb || (off+7) > l_LDTR.h.lim.lastb )
        return true;
      desc.addr= l_LDTR.h.lim.addr + off;
    }
  else // GDT
    {
      if ( off < l_GDTR.firstb || (off+7) > l_GDTR.lastb )
        return true;
      desc.addr= l_GDTR.addr + off;
    }
  // --> Llig descriptor
  READLD ( desc.addr, &(desc.w1), return false, true, true );
  READLD ( desc.addr+4, &(desc.w0), return false, true, true );
  // --> Obté valors protecció
  dpl= SEG_DESC_GET_DPL ( desc );
  cpl= CPL;
  rpl= selector&0x3;
  stype= SEG_DESC_GET_STYPE ( desc );
  // --> System segment
  if ( (stype&0x10) == 0x00 ) return true; // System segment
  // --> Check readable/writable
  if ( (check_read && ((stype&0x1A) == 0x18)) || // Not readable
       (!check_read && ((stype&0x1A) != 0x12)) ) // Not writable
    return true;
  // --> Non conforming code segment
  if ( (stype&0x1C) == 0x18 && (cpl > dpl || rpl > dpl) )
    return true;

  // Fixa valor.
  l_EFLAGS|= ZF_FLAG;
  
  return true;
  
} // end verr_verw


static int
fpu_get_round_mode (
                    IA32_JIT *jit
                    )
{

  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  switch ( (l_FPU_CONTROL&FPU_CONTROL_RC_MASK)>>FPU_CONTROL_RC_SHIFT )
    {
    case 0: return FE_TONEAREST;
    case 1: return FE_DOWNWARD;
    case 2: return FE_UPWARD;
    case 3: return FE_TOWARDZERO;
    default: return -1;
    }
  
} // end fpu_get_round_mode


static uint16_t
fpu_get_tag_word (
                  IA32_JIT *jit
                  )
{

  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  
  return
    ((uint16_t) l_FPU_REG(0).tag) |
    (((uint16_t) l_FPU_REG(1).tag)<<2) |
    (((uint16_t) l_FPU_REG(2).tag)<<4) |
    (((uint16_t) l_FPU_REG(3).tag)<<6) |
    (((uint16_t) l_FPU_REG(4).tag)<<8) |
    (((uint16_t) l_FPU_REG(5).tag)<<10) |
    (((uint16_t) l_FPU_REG(6).tag)<<12) |
    (((uint16_t) l_FPU_REG(7).tag)<<14)
    ;
  
} // end fpu_get_tag_word


static void
fpu_set_tag_word (
                  IA32_JIT *jit,
                  uint16_t  tag_word
                  )
{

  int i;
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  for ( i= 0; i < 8; ++i )
    {
      l_FPU_REG(i).tag= (int) (tag_word&0x3);
      tag_word>>= 2;
    }
  
} // end fpu_set_tag_word


static bool
fpu_check_exceptions (
                      IA32_JIT *jit
                      )
{

  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  if ( (l_FPU_STATUS&FPU_STATUS_ES) != 0 )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fpu_check_exceptions "
                "STATUS:%X CONTROL:%X\n",
                l_FPU_STATUS_TOP, l_FPU_CONTROL );
      exit ( EXIT_FAILURE );
      return false;
    }

  return true;
  
} // end fpu_check_exceptions


static bool
fpu_begin_op (
              IA32_JIT   *jit,
              const bool  check
              )
{

  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  if ( l_CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return false; }
  else if ( check && !fpu_check_exceptions ( jit ) )
    return false;
  
  return true;
  
} // end fpu_begin_op


static bool
fpu_chs (
         IA32_JIT *jit
         )
{
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  
  
  l_cpu= jit->_cpu;
  if ( l_CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return false; }
  l_FPU_STATUS&= ~FPU_STATUS_C0;
  fpu_reg= l_FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return false);
  l_FPU_REG(fpu_reg).v= -l_FPU_REG(fpu_reg).v;
  
  return true;
  
} // end fpu_chs


static bool
fpu_clear_excp (
                IA32_JIT *jit
                )
{
  
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  if ( l_CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return false; }
  l_FPU_STATUS&= ~0x80FF;
  
  return true;
  
} // end fpu_clear_excp


static bool
fpu_init (
          IA32_JIT *jit
          )
{
  
  int i;
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  if ( l_CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return false; }

  l_FPU_CONTROL= 0x37F; // PC=0x3 IM|DM|ZM|OM|UM
  l_FPU_STATUS= 0;
  l_FPU_TOP= 0;
  for ( i= 0; i < 8; ++i )
    l_FPU_REG(i).tag= IA32_CPU_FPU_TAG_EMPTY;
  l_FPU_DPTR.offset= 0; l_FPU_DPTR.selector= 0;
  l_FPU_IPTR.offset= 0; l_FPU_IPTR.selector= 0;
  l_FPU_OPCODE= 0;

  return true;
  
} // end fpu_init


static bool
fpu_load_control_word (
                       IA32_JIT       *jit,
                       const uint16_t  val
                       )
{

  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  if ( l_CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return false; }
  l_FPU_CONTROL= val;
  if ( !fpu_check_exceptions ( jit ) ) return false;
  
  return true;
  
} // end fpu_load_control_word


static void
fpu_push_float_u32 (
                    IA32_JIT    *jit,
                    float_u32_t  val
                    )
{

  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  

  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  fpu_reg= (l_FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  // --> Invalid arithmetic operand (source SNaN)
  if ( FPU_CHECKU32_SNAN(val.u32) )
    {
      l_FPU_STATUS|= FPU_STATUS_IE;
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )
        { l_FPU_STATUS|= FPU_STATUS_ES; return; }
      else val.u32= FPU_U32_SNAN2QNAN ( val.u32 ); // SNaN -> QNan
    }
  // --> Denormal operand
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  
  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg).v= (long double) val.f;
  if ( FPU_CHECKU32_ZERO(val.u32) )
    l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  // El valor denormal es normalitza
  else if ( FPU_CHECKU32_INF(val.u32) || FPU_CHECKU32_NAN(val.u32) )
    l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
  else l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fpu_push_float_u32


static void
fpu_push_double_u64 (
                     IA32_JIT     *jit,
                     double_u64_t  val
                     )
{

  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  

  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  fpu_reg= (l_FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  // --> Invalid arithmetic operand (source SNaN)
  if ( FPU_CHECKU64_SNAN(val.u64) )
    {
      l_FPU_STATUS|= FPU_STATUS_IE;
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )
        { l_FPU_STATUS|= FPU_STATUS_ES; return; }
      else val.u64= FPU_U64_SNAN2QNAN ( val.u64 ); // SNaN -> QNan
    }
  // --> Denormal operand
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  
  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg).v= (long double) val.d;
  if ( FPU_CHECKU64_ZERO(val.u64) )
    l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  // El valor denormal es normalitza
  else if ( FPU_CHECKU64_INF(val.u64) || FPU_CHECKU64_NAN(val.u64) )
    l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
  else l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fpu_push_double_u64


static void
fpu_push_double_u64_as_int64 (
                              IA32_JIT     *jit,
                              double_u64_t  val
                              )
{

  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  

  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  fpu_reg= (l_FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg).v= (long double) ((int64_t) val.u64);
  if ( val.u64 == 0 ) l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  else                l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fpu_push_double_u64_as_int64


static void
fpu_push_ldouble_u80 (
                      IA32_JIT      *jit,
                      ldouble_u80_t  val
                      )
{

  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  

  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  fpu_reg= (l_FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  // --> Invalid arithmetic operand (source SNaN)
  if ( FPU_CHECKU80_SNAN(val.u64.v0,val.u16.v4) )
    {
      l_FPU_STATUS|= FPU_STATUS_IE;
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )
        { l_FPU_STATUS|= FPU_STATUS_ES; return; }
      else val.u64.v0= FPU_U80_SNAN2QNAN ( val.u64.v0 ); // SNaN -> QNan
    }
  
  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg).v= val.ld;
  if ( FPU_CHECKU80_ZERO(val.u64.v0,val.u16.v4) )
    l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  // El valor denormal es normalitza
  else if ( FPU_CHECKU80_INF(val.u64.v0,val.u16.v4) ||
            FPU_CHECKU80_NAN(val.u64.v0,val.u16.v4) )
    l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
  else l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fpu_push_ldouble_u80


static void
fpu_push_reg (
              IA32_JIT  *jit,
              const int  fpu_reg_src
              )
{

  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  

  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  // Prepara i comprova excepcions
  fpu_reg= (l_FPU_TOP-1)&0x7; // nou ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg)= l_FPU_REG(fpu_reg_src);
  
} // end fpu_push_reg


static void
fpu_push_res16 (
                IA32_JIT *jit,
                uint16_t  val
                )
{

  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  
  
  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  fpu_reg= (l_FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg).v= (long double) ((int16_t) val);
  if ( val == 0 ) l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  else            l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fpu_push_res16


static void
fpu_push_res32 (
                IA32_JIT *jit,
                uint32_t  val
                )
{

  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  
  
  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  fpu_reg= (l_FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg).v= (long double) ((int32_t) val);
  if ( val == 0 ) l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  else            l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fpu_push_res32


static void
fpu_push_val (
              IA32_JIT *jit,
              const long double val,
              const int         tag
              )
{
  
  // NOTA!! Si es fica ES actiu es suspen l'operació però l'execucio
  // continua normal.
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  
  
  // Prepara i comprova excepcions
  l_cpu= jit->_cpu;
  fpu_reg= (l_FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);

  // Assigna valor
  l_FPU_TOP= (l_FPU_TOP-1)&0x7;
  l_FPU_REG(fpu_reg).v= val;
  l_FPU_REG(fpu_reg).tag= tag;
  
} // end fpu_push_val

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
// Torna fpu_ok
static bool
fpu_reg2float (
               IA32_JIT  *jit,
               const int  fpu_reg,
               float     *res
               )
{
  
  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc;
  float tmp;

  
  l_cpu= jit->_cpu;
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  tmp= (float) l_FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Inexact-Result (s'actualitza el valor)
  if ( i_exc )
    {
      // Dubtes sobre C1, hi ha contradiccions (Imagine que el segon
      // cas és quan és registre):
      //
      //  If an unmasked numeric overflow or underflow exception
      //  occurs and the destination operand is a memory location
      //  (which can happen only for a floating-point store), the
      //  inexact-result condition is not reported and the C1 flag is
      //  cleared.
      //
      //
      //  Indicates rounding direction of if the floating-point
      //  inexact exception (#P) is generated: 0 ← not roundup; 1 ←
      //  roundup.
      if ( o_exc || u_exc ) l_FPU_STATUS&= ~FPU_STATUS_C1;
      else
        {
          l_FPU_STATUS|= FPU_STATUS_PE;
          if ( !(l_FPU_CONTROL&FPU_CONTROL_PM) )
            { l_FPU_STATUS|= FPU_STATUS_ES; return false; }
        }
    }
  *res= tmp;
  
  return true;
  
} // end fpu_reg2float


// Torna fpu_ok
static bool
fpu_reg2double (
                IA32_JIT  *jit,
                const int  fpu_reg,
                double    *res
                )
{
  
  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc;
  double tmp;

  
  l_cpu= jit->_cpu;
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  tmp= (double) l_FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Inexact-Result (s'actualitza el valor)
  if ( i_exc )
    {
      // Dubtes sobre C1, hi ha contradiccions (Imagine que el segon
      // cas és quan és registre):
      //
      //  If an unmasked numeric overflow or underflow exception
      //  occurs and the destination operand is a memory location
      //  (which can happen only for a floating-point store), the
      //  inexact-result condition is not reported and the C1 flag is
      //  cleared.
      //
      //
      //  Indicates rounding direction of if the floating-point
      //  inexact exception (#P) is generated: 0 ← not roundup; 1 ←
      //  roundup.
      if ( o_exc || u_exc ) l_FPU_STATUS&= ~FPU_STATUS_C1;
      else
        {
          l_FPU_STATUS|= FPU_STATUS_PE;
          if ( !(l_FPU_CONTROL&FPU_CONTROL_PM) )
            { l_FPU_STATUS|= FPU_STATUS_ES; return false; }
        }
    }
  *res= tmp;
  
  return true;
  
} // end fpu_reg2double


// Torna fpu_ok
static bool
fpu_reg2int16 (
               IA32_JIT  *jit,
               const int  fpu_reg,
               uint16_t  *res
               )
{
  
  IA32_CPU *l_cpu;
  bool i_exc;
  uint16_t tmp;
  
  
  l_cpu= jit->_cpu;
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  tmp= (uint16_t) ((int16_t) rintl ( l_FPU_REG(fpu_reg).v ));
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  *res= tmp;
  
  return true;
  
} // end fpu_reg2int16


// Torna fpu_ok
static bool
fpu_reg2int32 (
               IA32_JIT  *jit,
               const int  fpu_reg,
               uint32_t  *res
               )
{
  
  IA32_CPU *l_cpu;
  bool i_exc;
  uint32_t tmp;
  
  
  l_cpu= jit->_cpu;
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  tmp= (uint32_t) ((int32_t) rintl ( l_FPU_REG(fpu_reg).v ));
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  *res= tmp;
  
  return true;
  
} // end fpu_reg2int32


// Torna fpu_ok
static bool
fpu_reg2int64 (
               IA32_JIT  *jit,
               const int  fpu_reg,
               uint64_t  *res
               )
{
  
  IA32_CPU *l_cpu;
  bool i_exc;
  uint64_t tmp;
  
  
  l_cpu= jit->_cpu;
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  tmp= (uint64_t) ((int64_t) rintl ( l_FPU_REG(fpu_reg).v ));
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  *res= tmp;
  
  return true;
  
} // end fpu_reg2int64


static void
fpu_update_tag (
                IA32_JIT  *jit,
                const int  reg
                )
{

  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  switch ( fpclassify ( l_FPU_REG(reg).v ) )
    {
    case FP_NAN:
    case FP_INFINITE:
    case FP_SUBNORMAL:
      l_FPU_REG(reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
      break;
    case FP_ZERO:
      l_FPU_REG(reg).tag= IA32_CPU_FPU_TAG_ZERO;
      break;
    case FP_NORMAL:
    default:
      l_FPU_REG(reg).tag= IA32_CPU_FPU_TAG_VALID;
    }
  
} // end fpu_update_tag


static bool
fpu_2xm1 (
          IA32_JIT  *jit,
          const int  fpu_reg
          )
{
  
  IA32_CPU *l_cpu;
  bool i_exc,u_exc;
  int ftype;
  
  
  l_cpu= jit->_cpu;

  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return false,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_UNDERFLOW|FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v= powl ( 2.0L, l_FPU_REG(fpu_reg).v ) - 1.0L;
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_2xm1


// Torna fpu_ok
static bool
fpu_add (
         IA32_JIT          *jit,
         const int          fpu_reg,
         const long double  src
         )
{

  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc,ia_exc;

  
  l_cpu= jit->_cpu;
    
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v+= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_add


// Torna si hi ha excepció normal
static bool
fpu_bstp (
          IA32_JIT             *jit,
          const int             fpu_reg,
          IA32_SegmentRegister *seg,
          const uint32_t        offset,
          bool                 *fpu_ok
          )
{
  
  IA32_CPU *l_cpu;
  int i;
  long double src;
  bool i_exc;
  uint64_t value,tmp;
  uint8_t buf[10];

  
  // Inicialitza.
  l_cpu= jit->_cpu;
  *fpu_ok= true;
  src= l_FPU_REG(fpu_reg).v;
  
  // Comprovacios prèvies.
  if ( isnan(src) || isinf(src) )
    {
      l_FPU_STATUS|= FPU_STATUS_IE;
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )
        { l_FPU_STATUS|= FPU_STATUS_ES; *fpu_ok= false; }
    }
  if ( !(*fpu_ok) ) return true;

  // Redondeja
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  src= rintl ( src );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,*fpu_ok= false);
  if ( !(*fpu_ok) ) return true;
  
  // Comprovacions posteriors.
  if ( src < -999999999999999999.0L || src > 999999999999999999.0L )
    {
      l_FPU_STATUS|= FPU_STATUS_IE;
      if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )
        { l_FPU_STATUS|= FPU_STATUS_ES; *fpu_ok= false; }
    }
  if ( !(*fpu_ok) ) return true;
  
  // Converteix a BCD (NOTA!! Igual les excepcions no estan activades)
  if ( isnan(src) || isinf(src) ) buf[9]= 0xff;
  else
    {
      if ( src < 0.0 ) { src= -src; buf[9]= 0x80; }
      else             buf[9]= 0x00;
      value= (uint64_t) src;
      for ( i= 0; i < 9; i++ )
        {
          tmp= value%((uint64_t) 100);
          value/= ((uint64_t) 100);
          buf[i]= (tmp%10) | ((tmp/10)<<4);
        }
    }
  
  // Escriu
  for ( i= 0; i < 10; i++ )
    {
      if ( jit->_mem_write8 ( jit, seg, offset, buf[i] ) != 0 )
        return false;
    }
  
  return true;
  
} // end fpu_bstp


// Torna fpu_ok
static bool
fpu_com (
         IA32_JIT          *jit,
         const int          fpu_reg,
         const long double  src
         )
{

  IA32_CPU *l_cpu;
  int tmp;
  
  
  l_cpu= jit->_cpu;
  l_FPU_STATUS|= (FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return false,tmp);
  if ( l_FPU_REG(fpu_reg).v > src )
    {
      l_FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
    }
  else if ( l_FPU_REG(fpu_reg).v < src )
    {
      l_FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
      l_FPU_STATUS|= FPU_STATUS_C0;
    }
  else // l_FPU_REG(fpu_reg).v == src
    {
      l_FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
      l_FPU_STATUS|= FPU_STATUS_C3;
    }

  return true;
  
} // end fpu_com


// NOTA!! No comprove sNaN ni unsupported format
#define FPU_COS_CHECK_INVALID_OPERAND(IND,ACTION,INT_VAR)       \
  if ( l_FPU_REG(IND).tag == IA32_CPU_FPU_TAG_SPECIAL )         \
    {                                                           \
      (INT_VAR)= fpclassify ( l_FPU_REG(IND).v );               \
      if ( (INT_VAR) == FP_SUBNORMAL )                          \
        {                                                       \
          l_FPU_STATUS|= FPU_STATUS_DE;                         \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_DM) )                \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }           \
        }                                                       \
      else if ( (INT_VAR) == FP_INFINITE )                      \
        {                                                       \
          l_FPU_STATUS|= FPU_STATUS_IE;                         \
          if ( !(l_FPU_CONTROL&FPU_CONTROL_IM) )                \
            { l_FPU_STATUS|= FPU_STATUS_ES; ACTION; }           \
        }                                                       \
    }


// Torna fpu_ok
static bool
fpu_cos (
         IA32_JIT  *jit,
         const int  fpu_reg
         )
{
  
  static const long double MAX= 9223372036854775807.0;
  static const long double MIN= -9223372036854775807.0;
  
  IA32_CPU *l_cpu;
  bool i_exc;
  int ftype;
  
  
  l_cpu= jit->_cpu;
  
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg,return false,ftype);
  // --> Operació
  if ( l_FPU_REG(fpu_reg).v >= MIN && l_FPU_REG(fpu_reg).v <= MAX )
    {
      l_FPU_STATUS&= ~FPU_STATUS_C2;
#pragma STDC FENV_ACCESS on
      feclearexcept ( FE_INEXACT );
      fesetround ( fpu_get_round_mode ( jit ) );
      l_FPU_REG(fpu_reg).v= cosl ( l_FPU_REG(fpu_reg).v );
      i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
      // --> Actualitza etiqueta.
      fpu_update_tag ( jit, fpu_reg );
      // --> Inexact result
      FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
    }
  else l_FPU_STATUS|= FPU_STATUS_C2;
  
  return true;
  
} // end fpu_cos


// Torna fpu_ok
static bool
fpu_div (
         IA32_JIT          *jit,
         const int          fpu_reg,
         const long double  src
         )
{
  
  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc,ia_exc;

  
  l_cpu= jit->_cpu;
    
  // --> Divide-by-zero (prèvia a operació)
  if ( src == 0.0 || src == -0.0 )
    {
      l_FPU_STATUS|= FPU_STATUS_ZE;
      if ( !(l_FPU_CONTROL&FPU_CONTROL_ZM) )
        { l_FPU_STATUS|= FPU_STATUS_ES; return false; }
    }
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v/= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_div


// Torna fpu_ok
static bool
fpu_divr (
          IA32_JIT          *jit,
          const int          fpu_reg,
          const long double  src
          )
{

  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc,ia_exc;

  
  l_cpu= jit->_cpu;
    
  // --> Divide-by-zero (prèvia a operació)
  if ( l_FPU_REG(fpu_reg).tag == IA32_CPU_FPU_TAG_ZERO )
    {
      l_FPU_STATUS|= FPU_STATUS_ZE;
      if ( !(l_FPU_CONTROL&FPU_CONTROL_ZM) )
        { l_FPU_STATUS|= FPU_STATUS_ES; return false; }
    }
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v= src / l_FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_divr


// Torna fpu_ok
static bool
fpu_mul (
         IA32_JIT          *jit,
         const int          fpu_reg,
         const long double  src
         )
{
  
  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc,ia_exc;
  
  
  l_cpu= jit->_cpu;
    
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v*= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_mul


static bool
fpu_patan (
           IA32_JIT  *jit,
           const int  fpu_reg0,
           const int  fpu_reg1
           )
{
  
  IA32_CPU *l_cpu;
  bool i_exc,u_exc;
  int ftype;
  
  
  l_cpu= jit->_cpu;

  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg0,return false,ftype); // Me val !
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg1,return false,ftype); // Me val !
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT|FE_UNDERFLOW );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg1).v=
    atan2l ( l_FPU_REG(fpu_reg1).v, l_FPU_REG(fpu_reg0).v );
  i_exc= fetestexcept ( FE_INEXACT );
  u_exc= fetestexcept ( FE_UNDERFLOW );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg1 );
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  
  return true;
  
} // end fpu_patan


static bool
fpu_prem (
          IA32_JIT  *jit,
          const int  fpu_reg0,
          const int  fpu_reg1
          )
{

  int ftype;
  IA32_CPU *l_cpu;
  bool ia_exc,u_exc;
  ldouble_u80_t aux;
  uint16_t exp0,exp1;
  uint64_t q;
  
  
  l_cpu= jit->_cpu;
  
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg0,return false,ftype); // Me val !
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg1,return false,ftype); // Me val !
  // --> Operació
  // NOTA!!! Farem l'operació gastant la funció matemàtica, però per
  // al tema dels flags farem alguns càlculs a partir dels exponents.
  // --> Exponents (C0,C3,C1,C2)
  aux.ld= l_FPU_REG(fpu_reg0).v;
  exp0= GET_EXPONENT_LDOUBLE_U(aux);
  aux.ld= l_FPU_REG(fpu_reg1).v;
  exp1= GET_EXPONENT_LDOUBLE_U(aux);
  if ( (((int) exp0) - ((int) exp1)) < 64 )
    {
      l_FPU_STATUS&= ~(FPU_STATUS_C0|FPU_STATUS_C1|FPU_STATUS_C2|FPU_STATUS_C3);
      fesetround ( FE_TOWARDZERO );
      q= (uint64_t) rintl ( l_FPU_REG(fpu_reg0).v / l_FPU_REG(fpu_reg1).v );
      if ( (q&0x1)!=0  ) l_FPU_STATUS|= FPU_STATUS_C1;
      if ( (q&0x2)!=0  ) l_FPU_STATUS|= FPU_STATUS_C3;
      if ( (q&0x4)!=0  ) l_FPU_STATUS|= FPU_STATUS_C0;
    }
  else { l_FPU_STATUS|= FPU_STATUS_C2; }
  // --> Operació real.
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INVALID|FE_UNDERFLOW );
  fesetround ( fpu_get_round_mode ( jit ) );
  // NOTA!! FPREM1 seria remainder
  l_FPU_REG(fpu_reg0).v= fmodl ( l_FPU_REG(fpu_reg0).v, l_FPU_REG(fpu_reg1).v );
  ia_exc= fetestexcept ( FE_INEXACT );
  u_exc= fetestexcept ( FE_UNDERFLOW );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg0 );
  // --> Inexact result
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  
  return true;
  
} // end fpu_prem


static bool
fpu_ptan (
          IA32_JIT  *jit,
          const int  fpu_reg
          )
{
  
  static const long double MAX= 9223372036854775807.0;
  static const long double MIN= -9223372036854775807.0;
  
  IA32_CPU *l_cpu;
  bool i_exc,u_exc;
  int ftype,fpu_reg_new;
  
  
  l_cpu= jit->_cpu;
  
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg,return false,ftype);
  // --> Operació
  if ( l_FPU_REG(fpu_reg).v >= MIN && l_FPU_REG(fpu_reg).v <= MAX )
    {
      
      // Comprova si es pot ficar el 1.0
      fpu_reg_new= (l_FPU_TOP-1)&0x7; // Nou ST(0)
      FPU_CHECK_STACK_OVERFLOW(fpu_reg_new,return false);
      
      // Calcula tangent
      l_FPU_STATUS&= ~FPU_STATUS_C2;
#pragma STDC FENV_ACCESS on
      feclearexcept ( FE_INEXACT|FE_UNDERFLOW );
      fesetround ( fpu_get_round_mode ( jit ) );
      l_FPU_REG(fpu_reg).v= tanl ( l_FPU_REG(fpu_reg).v );
      i_exc= fetestexcept ( FE_INEXACT );
      u_exc= fetestexcept ( FE_UNDERFLOW );
#pragma STDC FENV_ACCESS off
      // --> Actualitza etiqueta.
      fpu_update_tag ( jit, fpu_reg );
      // --> Inexact result
      FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
      // --> Numeric underflow
      FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
      
      // Assigna valor 1.0
      l_FPU_TOP= fpu_reg_new;
      l_FPU_REG(fpu_reg_new).v= 1.0L;
      l_FPU_REG(fpu_reg_new).tag= IA32_CPU_FPU_TAG_VALID;
      
    }
  else l_FPU_STATUS|= FPU_STATUS_C2;
  
  return true;
  
} // end fpu_ptan


// Torna fpu_ok
static bool
fpu_rndint (
            IA32_JIT  *jit,
            const int  fpu_reg
            )
{
  
  IA32_CPU *l_cpu;
  bool i_exc;
  
  
  l_cpu= jit->_cpu;

  FPU_CHECK_DENORMAL_REG(fpu_reg,return false);
  // NOTA!! Faltaria SNAN i unsupported que no sé com fer-ho
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v= rintl ( l_FPU_REG(fpu_reg).v );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);

  return true;
  
} // end fpu_rndint


// Llig els registres.
static bool
frstor_common (
               IA32_JIT             *jit,
               IA32_SegmentRegister *seg,
               uint32_t              off
               )
{

  int i;
  ldouble_u80_t val;
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  val.u16.v5= 0;
  val.u32.v3= 0;
  for ( i= 0; i < 8; i++ )
    {
      if ( jit->_mem_read32 ( jit, seg, off+i*10, &val.u32.v0, true ) != 0 )
        return false;
      if ( jit->_mem_read32 ( jit, seg, off+i*10+4, &val.u32.v1, true ) != 0 )
        return false;
      if ( jit->_mem_read16 ( jit, seg, off+i*10+8, &val.u16.v4, true ) != 0 )
        return false;
      l_FPU_REG(i).v= val.ld;
    }
  
  return true;
  
} // end frstor_common


static bool
fpu_rstor32 (
             IA32_JIT             *jit,
             IA32_SegmentRegister *seg,
             const uint32_t        off
             )
{
  
  IA32_CPU *l_cpu;
  uint32_t tmp;
  
  
  l_cpu= jit->_cpu;
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( l_EFLAGS&VM_FLAG )
        {
          // EN REALITAT NO EM QUEDA CLAR SI AÇÒ HA DE SER COM EL
          // PROTECTED O COM EL REAL.
          printf("[CAL IMPLEMENTAR] frstor32 - protected mode VM!!!\n");
          exit(EXIT_FAILURE);
        }
      if ( jit->_mem_read32 ( jit, seg, off, &tmp, true ) != 0 )
        return false;
      l_FPU_CONTROL= (uint16_t) tmp;
      if ( jit->_mem_read32 ( jit, seg, off+4, &tmp, true ) != 0 )
        return false;
      l_FPU_STATUS= (uint16_t) tmp;
      if ( jit->_mem_read32 ( jit, seg, off+8, &tmp, true ) != 0 )
        return false;
      fpu_set_tag_word ( jit, (uint16_t) tmp );
      if ( jit->_mem_read32 ( jit, seg, off+12,
                              &(l_FPU_IPTR.offset), true ) != 0 )
        return false;
      if ( jit->_mem_read32 ( jit, seg, off+16, &tmp, true ) != 0 )
        return false;
      l_FPU_IPTR.selector= (uint16_t) tmp;
      l_FPU_OPCODE= (uint16_t) ((tmp>>16)&0x07FF);
      if ( jit->_mem_read32 ( jit, seg, off+20,
                              &(l_FPU_DPTR.offset), true ) != 0 )
        return false;
      if ( jit->_mem_read32 ( jit, seg, off+24, &tmp, true ) != 0 )
        return false;
      l_FPU_DPTR.selector= (uint16_t) tmp;
      if ( !frstor_common ( jit, seg, off+28 ) ) return false;
    }
  else
    {
      printf("[CAL IMPLEMENTAR] frstor32 - real mode!!!\n");
      exit(EXIT_FAILURE);
    }

  return true;
  
} // end fpu_rstor32


// Desa els registres i reinicialitza.
static bool 
fsave_common (
              IA32_JIT             *jit,
              IA32_SegmentRegister *seg,
              uint32_t              off
              )
{

  int i;
  ldouble_u80_t val;
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  // Desa registres
  for ( i= 0; i < 8; i++ )
    {
      val.ld= l_FPU_REG(i).v;
      if ( jit->_mem_write32 ( jit, seg, off+i*10, val.u32.v0 ) != 0 )
        return false;
      if ( jit->_mem_write32 ( jit, seg, off+i*10+4, val.u32.v1 ) != 0 )
        return false;
      if ( jit->_mem_write16 ( jit, seg, off+i*10+8, val.u16.v4 ) != 0 )
        return false;
    }
  
  // Reinicialitza.
  l_FPU_CONTROL= 0x37F; // PC=0x3 IM|DM|ZM|OM|UM
  l_FPU_STATUS= 0;
  l_FPU_TOP= 0;
  for ( i= 0; i < 8; ++i )
    l_FPU_REG(i).tag= IA32_CPU_FPU_TAG_EMPTY;
  l_FPU_DPTR.offset= 0; l_FPU_DPTR.selector= 0;
  l_FPU_IPTR.offset= 0; l_FPU_IPTR.selector= 0;
  //l_FPU_OPCODE= 0; ????? Pareix que l'opcode no es modifica.

  return true;
  
} // end fsave_common


static bool
fpu_save32 (
            IA32_JIT             *jit,
            IA32_SegmentRegister *seg,
            const uint32_t        off
            )
{
  
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( l_EFLAGS&VM_FLAG )
        {
          // EN REALITAT NO EM QUEDA CLAR SI AÇÒ HA DE SER COM EL
          // PROTECTED O COM EL REAL.
          printf("[CAL IMPLEMENTAR] fsave32 - protected mode VM!!!\n");
          exit(EXIT_FAILURE);
        }
      if ( jit->_mem_write32 ( jit, seg, off, (uint32_t) l_FPU_CONTROL ) != 0 )
        return false;
      if ( jit->_mem_write32 ( jit, seg, off+4, (uint32_t) l_FPU_STATUS ) != 0 )
        return false;
      if ( jit->_mem_write32 ( jit, seg, off+8,
                               (uint32_t) fpu_get_tag_word ( jit ) ) != 0 )
        return false;
      if ( jit->_mem_write32 ( jit, seg, off+12, l_FPU_IPTR.offset ) != 0 )
        return false;
      if ( jit->_mem_write32 ( jit, seg, off+16,
                               ((uint32_t) l_FPU_IPTR.selector) |
                               (((uint32_t) (l_FPU_OPCODE&0x07FF))<<16) ) != 0 )
        return false;
      if ( jit->_mem_write32 ( jit, seg, off+20, l_FPU_DPTR.offset ) != 0 )
        return false;
      if ( jit->_mem_write32 ( jit, seg, off+24,
                               (uint32_t) l_FPU_DPTR.selector ) != 0 )
        return false;
      if ( !fsave_common ( jit, seg, off+28 ) ) return false;
    }
  else
    {
      printf("[CAL IMPLEMENTAR] fsave32 - real mode!!!\n");
      exit(EXIT_FAILURE);
    }

  return true;
  
} // end fpu_save32


static bool
fpu_scale (
           IA32_JIT  *jit,
           const int  fpu_reg0,
           const int  fpu_reg1
           )
{
  
  IA32_CPU *l_cpu;
  bool i_exc,u_exc,o_exc;
  long double tmp;
  int ftype;
  
  
  l_cpu= jit->_cpu;

  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg0,return false,ftype); // Me val !
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg1,return false,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT );
  fesetround ( FE_TOWARDZERO );
  tmp= rintl ( l_FPU_REG(fpu_reg1).v );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg0).v*= powl ( 2.0L, tmp );
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg0 );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_scale


// Torna fpu_ok
static bool
fpu_sin (
         IA32_JIT  *jit,
         const int  fpu_reg
         )
{
  
  static const long double MAX= 9223372036854775807.0;
  static const long double MIN= -9223372036854775807.0;
  
  IA32_CPU *l_cpu;
  bool i_exc;
  int ftype;
  
  
  l_cpu= jit->_cpu;
  
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg,return false,ftype);
  // --> Operació
  if ( l_FPU_REG(fpu_reg).v >= MIN && l_FPU_REG(fpu_reg).v <= MAX )
    {
      l_FPU_STATUS&= ~FPU_STATUS_C2;
#pragma STDC FENV_ACCESS on
      feclearexcept ( FE_INEXACT );
      fesetround ( fpu_get_round_mode ( jit ) );
      l_FPU_REG(fpu_reg).v= sinl ( l_FPU_REG(fpu_reg).v );
      i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
      // --> Actualitza etiqueta.
      fpu_update_tag ( jit, fpu_reg );
      // --> Inexact result
      FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
    }
  else l_FPU_STATUS|= FPU_STATUS_C2;
  
  return true;
  
} // end fpu_sin


static bool
fpu_sqrt (
          IA32_JIT  *jit,
          const int  fpu_reg
          )
{
  
  IA32_CPU *l_cpu;
  bool i_exc,ia_exc;
  int ftype;
  
  
  l_cpu= jit->_cpu;
  
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return false,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v= sqrtl ( l_FPU_REG(fpu_reg).v );
  ia_exc= fetestexcept ( FE_INVALID );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  
  return true;
  
} // end fpu_sqrt


// Torna fpu_ok
static bool
fpu_sub (
         IA32_JIT          *jit,
         const int          fpu_reg,
         const long double  src
         )
{

  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc,ia_exc;

  
  l_cpu= jit->_cpu;
    
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v-= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_sub


// Torna fpu_ok
static bool
fpu_subr (
          IA32_JIT          *jit,
          const int          fpu_reg,
          const long double  src
          )
{

  IA32_CPU *l_cpu;
  bool o_exc,u_exc,i_exc,ia_exc;

  
  l_cpu= jit->_cpu;
    
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg).v= src - l_FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  
  return true;
  
} // end fpu_subr


// Torna fpu_ok
static bool
fpu_tst (
         IA32_JIT  *jit,
         const int  fpu_reg
         )
{
  
  IA32_CPU *l_cpu;
  int ftype;
  
  
  l_cpu= jit->_cpu;

  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return false,ftype);
  
  // Compara
  if ( l_FPU_REG(fpu_reg).tag == IA32_CPU_FPU_TAG_SPECIAL &&
       !isinf ( l_FPU_REG(fpu_reg).v ) )
    { l_FPU_STATUS|= (FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0); }
  else if ( l_FPU_REG(fpu_reg).v > 0.0L )
    { l_FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0); }
  else if ( l_FPU_REG(fpu_reg).v < 0.0L )
    {
      l_FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2);
      l_FPU_STATUS|= FPU_STATUS_C0;
    }
  else if ( l_FPU_REG(fpu_reg).v == 0.0L )
    {
      l_FPU_STATUS&= ~(FPU_STATUS_C2|FPU_STATUS_C0);
      l_FPU_STATUS|= FPU_STATUS_C3;
    }
  else
    {
      printf ( "[EE] ftst WTF !!\n" );
      exit ( EXIT_FAILURE );
    }
  
  return true;
  
} // end fpu_tst


static bool
fpu_yl2x (
          IA32_JIT  *jit,
          const int  fpu_reg0,
          const int  fpu_reg1
          )
{
  
  IA32_CPU *l_cpu;
  bool i_exc,u_exc,o_exc,ia_exc,dz_exc;
  int ftype;
  
  
  l_cpu= jit->_cpu;

  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg0,return false,ftype); // Me val !
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg1,return false,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_DIVBYZERO|FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( jit ) );
  l_FPU_REG(fpu_reg1).v*= log2l ( l_FPU_REG(fpu_reg0).v );
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
  dz_exc= fetestexcept ( FE_DIVBYZERO );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( jit, fpu_reg1 );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return false);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return false);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return false);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return false);
  // --> Divide by zero
  FPU_CHECK_DIVBYZERO_RESULT_BOOL(dz_exc,return false);
  
  return true;
  
} // end fpu_yl2x
#pragma GCC diagnostic pop


static bool
fpu_wait (
          IA32_JIT *jit
          )
{
  
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  if ( (l_CR0&CR0_MP) && (l_CR0&CR0_TS) )
    { EXCEPTION ( EXCP_NM ); return false; }
  else if ( !fpu_check_exceptions ( jit ) )
    return false;
  
  return true;
  
} // end fpu_wait


static void
fpu_xam (
         IA32_JIT *jit
         )
{
  
  IA32_CPU *l_cpu;
  int fpu_reg;
  
  
  l_cpu= jit->_cpu;
  
  fpu_reg= l_FPU_TOP; // ST(0)
  switch ( l_FPU_REG(fpu_reg).tag )
    {
    case IA32_CPU_FPU_TAG_VALID:
      l_FPU_STATUS&= ~(FPU_STATUS_C0|FPU_STATUS_C3);
      l_FPU_STATUS|= FPU_STATUS_C2;
      break;
    case IA32_CPU_FPU_TAG_ZERO:
      l_FPU_STATUS&= ~(FPU_STATUS_C0|FPU_STATUS_C2);
      l_FPU_STATUS|= FPU_STATUS_C3;
      break;
    case IA32_CPU_FPU_TAG_SPECIAL:
      switch ( fpclassify ( l_FPU_REG(fpu_reg).v ) )
        {
        case FP_NAN:
          l_FPU_STATUS&= ~(FPU_STATUS_C2|FPU_STATUS_C3);
          l_FPU_STATUS|= FPU_STATUS_C0;
          break;
        case FP_INFINITE:
          l_FPU_STATUS&= ~FPU_STATUS_C3;
          l_FPU_STATUS|= (FPU_STATUS_C0|FPU_STATUS_C2);
          break;
        case FP_SUBNORMAL:
          l_FPU_STATUS&= ~FPU_STATUS_C0;
          l_FPU_STATUS|= (FPU_STATUS_C2|FPU_STATUS_C3);
          break;
        default:
          printf ( "[EE] fxam WTF (B)!!\n" );
          exit ( EXIT_FAILURE );
        }
      break;
    case IA32_CPU_FPU_TAG_EMPTY:
      l_FPU_STATUS&= ~FPU_STATUS_C2;
      l_FPU_STATUS|= (FPU_STATUS_C0|FPU_STATUS_C3);
      break;
    default:
      printf ( "[EE] fxam WTF!!\n" );
      exit ( EXIT_FAILURE );
    }
  
} // end fpu_xam


static bool
translate_addr (
                IA32_JIT *jit,
                uint32_t *addr // Entrada/Eixida
                )
{

  uint32_t ret;
  uint64_t tmp_addr;
  
  
  // Paginació 32 bits
  if ( jit->_mem_readl8 == mem_p32_read8 )
    {
      if ( !paging_32b_translate ( jit, *addr, &tmp_addr, false, true, false ) )
        return false;
      // De moment no soporte >4GB.
      if ( tmp_addr&0xFFFFFFFF00000000 )
        {
          fprintf ( FERROR,
                    "[EE] IA32 JIT - pàgina fora de rang"
                    " (ADDR: %016lX) (>4GB)\n", tmp_addr );
          exit ( EXIT_FAILURE );
        }
      ret= (uint32_t) tmp_addr;
    }
  else if ( jit->_mem_readl8 == mem_readl8 )
    ret= *addr;
  else
    {
      fprintf ( stderr, "[CAL IMPLEMENTAR] translate_addr "
                "- mode no implementat\n" );
      exit ( EXIT_FAILURE );
    }

  // Copia nova adreça
  *addr= ret;

  return true;
  
} // end translate_addr


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


static bool
int_read_segment_descriptor (
                             IA32_JIT       *jit,
                             const uint16_t  selector,
                             seg_desc_t     *desc,
                             const uint8_t   ext,
                             const bool      is_TS
                             )
{

  
  uint32_t off;
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  
  // NULL selector
  if ( (selector&0xFFFC) == 0 ) goto excp_gp_error;
  
  // Comprovacions i fixa adreça.
  off= (uint32_t) (selector&0xFFF8);
  if ( selector&0x4 ) // LDT
    {
      if ( l_LDTR.h.isnull )
        {
          EXCEPTION0 ( EXCP_GP );
          return false;
        }
      if ( off < l_LDTR.h.lim.firstb || (off+7) > l_LDTR.h.lim.lastb )
        goto excp_gp_error;
      desc->addr= l_LDTR.h.lim.addr + off;
    }
  else // GDT
    {
      if ( off < l_GDTR.firstb || (off+7) > l_GDTR.lastb )
        goto excp_gp_error;
      desc->addr= l_GDTR.addr + off;
    }
  
  // Llig descriptor.
  if ( jit->_mem_readl32 ( jit, desc->addr, &(desc->w1), true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, desc->addr+4, &(desc->w0), true, true ) != 0 )
    return false;
  
  return true;
  
 excp_gp_error:
  EXCEPTION_ERROR_CODE ( is_TS ? EXCP_TS : EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
  
} // end int_read_segment_descriptor


static bool
switch_task_set_sreg (
                      IA32_JIT             *jit,
                      const uint16_t        selector,
                      IA32_SegmentRegister *reg,
                      const uint8_t         ext
                      )
{

  seg_desc_t desc;
  uint32_t stype,dpl,rpl,cpl;
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  if ( reg == l_P_CS )
    {
      // Comprovacions
      if ( (selector&0xFFFC) == 0 ) // Null segment
        goto excp_ts_error;
      if ( !int_read_segment_descriptor ( jit, selector, &desc, ext, true ) )
        return false;
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
      set_segment_descriptor_access_bit ( jit, &desc );
    }
  else if ( reg == l_P_SS ) // El SS és especial
    {
      // Comprovacions
      if ( (selector&0xFFFC) == 0 ) // Null segment
        goto excp_ts_error;
      if ( !int_read_segment_descriptor ( jit, selector, &desc, ext, true ) )
        return false;
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
      set_segment_descriptor_access_bit ( jit, &desc );
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
          if ( !int_read_segment_descriptor ( jit, selector,
                                              &desc, ext, true ) )
            return false;
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
          set_segment_descriptor_access_bit ( jit, &desc );
        }
    }
  
  return true;

 excp_ts_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
 excp_np_error:
  EXCEPTION_ERROR_CODE ( EXCP_NP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
 excp_ss_error:
  EXCEPTION_ERROR_CODE ( EXCP_SS,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
  
} // end switch_task_set_sreg


static bool
switch_task (
             IA32_JIT       *jit,
             const int       type,
             const uint16_t  selector,
             const uint8_t   ext,
             const uint32_t  old_EIP
             )
{
  
  uint8_t dpl,tmp8;
  uint16_t tmp16;
  uint32_t stype,off,rpl,tmp32,addr,eflags_tmp;
  seg_desc_t desc;
  IA32_SegmentRegister tmp_seg;
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  // TSS Descriptor
  off= (uint32_t) (selector&0xFFF8);
  if ( off < l_GDTR.firstb || (off+7) > l_GDTR.lastb )
    {
      if ( type&SWITCH_TASK_IRET ) goto excp_ts_error;
      else                         goto excp_gp_error;
    }
  desc.addr= l_GDTR.addr + off;
  if ( jit->_mem_readl32 ( jit, desc.addr, &(desc.w1), true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, desc.addr+4, &(desc.w0), true, true ) != 0 )
    return false;
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
  if ( jit->_mem_readl8 ( jit, l_TR.h.lim.addr + l_TR.h.lim.firstb,
                          &tmp8, true ) != 0 )
    return false;
  if ( jit->_mem_readl8 ( jit, l_TR.h.lim.addr + l_TR.h.lim.lastb,
                          &tmp8, true ) != 0 )
    return false;
  // --> new TSS
  if ( jit->_mem_readl8 ( jit, tmp_seg.h.lim.addr + tmp_seg.h.lim.firstb,
                          &tmp8, true ) != 0 )
    return false;
  if ( jit->_mem_readl8 ( jit, tmp_seg.h.lim.addr + tmp_seg.h.lim.lastb,
                          &tmp8, true ) != 0 )
    return false;
  
  // Canvia valor Busy TSS Descriptor vell
  addr= l_GDTR.addr + (uint32_t) (l_TR.v&0xFFF8);
  if ( jit->_mem_readl32 ( jit, addr+4, &tmp32, true, true ) != 0 )
    return false;
  if ( type&(SWITCH_TASK_JMP|SWITCH_TASK_IRET) )
    tmp32&= ~(0x00000200); // --> Available
  else // INT, EXC o CALL
    tmp32|= 0x00000200;    // --> Busy
  if ( jit->_mem_writel32 ( jit, addr+4, tmp32 ) != 0 )
    return false;
  
  // NT Flag en EFLAGS que es salva
  eflags_tmp= EFLAGS;
  if ( type&SWITCH_TASK_IRET )
    eflags_tmp&= ~NT_FLAG;
  
  // Desa estat en TSS
  assert ( l_TR.h.lim.firstb == 0 ); // En TR no tendria sentit que no
                                     // ho fora, crec.....
  assert ( l_TR.h.tss_is32 ); // No sé què fer si és de 16 bits.
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 32, old_EIP ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 36, eflags_tmp ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 40, l_EAX ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 44, l_ECX ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 48, l_EDX ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 52, l_EBX ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 56, l_ESP ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 60, l_EBP ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 64, l_ESI ) != 0 )
    return false;
  if ( jit->_mem_writel32 ( jit, l_TR.h.lim.addr + 68, l_EDI ) != 0 )
    return false;
  if ( jit->_mem_writel16 ( jit, l_TR.h.lim.addr + 72, l_P_ES->v ) != 0 )
    return false;
  if ( jit->_mem_writel16 ( jit, l_TR.h.lim.addr + 76, l_P_CS->v ) != 0 )
    return false;
  if ( jit->_mem_writel16 ( jit, l_TR.h.lim.addr + 80, l_P_SS->v ) != 0 )
    return false;
  if ( jit->_mem_writel16 ( jit, l_TR.h.lim.addr + 84, l_P_DS->v ) != 0 )
    return false;
  if ( jit->_mem_writel16 ( jit, l_TR.h.lim.addr + 88, l_P_FS->v ) != 0 )
    return false;
  if ( jit->_mem_writel16 ( jit, l_TR.h.lim.addr + 92, l_P_GS->v ) != 0 )
    return false;
  
  // Llig EFLAGS, fixa NT del nou TSS i desa previous link
  assert ( tmp_seg.h.lim.firstb == 0 ); // En TR no tendria sentit que
                                        // no ho fora, crec.....
  assert ( tmp_seg.h.tss_is32 ); // No sé què fer si és de 16 bits.
  if ( jit->_mem_readl32 ( jit, tmp_seg.h.lim.addr + 36,
                           &eflags_tmp, true, true ) != 0 )
    return false;
  if ( type&(SWITCH_TASK_CALL|SWITCH_TASK_EXC|SWITCH_TASK_INT) )
    {
      if ( jit->_mem_writel16 ( jit, tmp_seg.h.lim.addr, l_TR.v ) != 0 )
        return false;
      eflags_tmp|= NT_FLAG;
    }
  
  // Canvia valor Busy TSS Descriptor nou
  if ( type&(SWITCH_TASK_CALL|SWITCH_TASK_JMP|SWITCH_TASK_EXC|SWITCH_TASK_INT) )
    {
      desc.w0|= 0x00000200; // --> Busy
      if ( jit->_mem_writel32 ( jit, desc.addr+4, desc.w0 ) != 0 )
        return false;
    }
  
  // Fixa TR i carrega TSS
  l_TR= tmp_seg;
  // --> LDTR
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 96,
                           &tmp16, true, true ) != 0 )
    return false;
  if ( (tmp16&0xFFFC) == 0 ) { l_LDTR.h.isnull= true; l_LDTR.v= tmp16; }
  else
    {
      printf("TODO SWITCH_TASK LDTR\n");
      exit(EXIT_FAILURE);
      /*
      if ( int_read_segment_descriptor ( INTERP, tmp16, &desc_tmp,
                                         ext, true ) != 0 )
        return -1;
      LDTR.h.isnull= false;
      cpl= CPL;
      if ( cpl != 0 ) goto gp_exc;
      stype= SEG_DESC_GET_STYPE ( desc );
      // NOTA!!! no comprova SegmentDescriptor(DescriptorType) (flag
      // S) sols comprova SegmentDescriptor(Type)
      if ( (stype&0xf) != 0x02 ) goto excp_gp_sel;
      if ( !SEG_DESC_IS_PRESENT ( desc ) )
        {
          EXCEPTION_SEL ( EXCP_NP, selector );
          return;
        }
      // Carrega (No faig el que es fa normalment perquè es LDTR, sols
      // el rellevant per a LDTR).
      LDTR.h.lim.addr=
        (desc.w1>>16) | ((desc.w0<<16)&0x00FF0000) | (desc.w0&0xFF000000);
      elimit= (desc.w0&0x000F0000) | (desc.w1&0x0000FFFF);
      if ( SEG_DESC_G_IS_SET ( desc ) )
        {
          elimit<<= 12;
          elimit|= 0xFFF;
        }
      LDTR.v= selector;
      LDTR.h.lim.firstb= 0;
      LDTR.h.lim.lastb= elimit;
      */
    }
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 32,
                           &l_EIP, true, true ) != 0 )
    return false;
  /* Ja ho faig abans i ho assigne al final.
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 36,
                           &l_EFLAGS, true, true ) != 0 )
    return false;
  */
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 40,
                           &l_EAX, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 44,
                           &l_ECX, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 48,
                           &l_EDX, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 52,
                           &l_EBX, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 56,
                           &l_ESP, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 60,
                           &l_EBP, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 64,
                           &l_ESI, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 68,
                           &l_EDI, true, true ) != 0 )
    return false;
  // Primer CS (per si es fan comprovacions)
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 76,
                           &tmp16, true, true ) != 0 )
    return false;
  if ( !switch_task_set_sreg ( jit, tmp16, l_P_CS, ext ) )
    return false;
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 80, &tmp16,
                           true, true ) != 0 )
    return false;
  if ( !switch_task_set_sreg ( jit, tmp16, l_P_SS, ext ) )
    return false;
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 72,
                           &tmp16, true, true ) != 0 )
    return false;
  if ( !switch_task_set_sreg ( jit, tmp16, l_P_ES, ext ) )
    return false;
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 84,
                           &tmp16, true, true ) != 0 )
    return false;
  if ( !switch_task_set_sreg ( jit, tmp16, l_P_DS, ext ) )
    return false;
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 88,
                           &tmp16, true, true ) != 0 )
    return false;
  if ( !switch_task_set_sreg ( jit, tmp16, l_P_FS, ext ) )
    return false;
  if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 92,
                           &tmp16, true, true ) != 0 )
    return false;
  if ( !switch_task_set_sreg ( jit, tmp16, l_P_GS, ext ) )
    return false;
  
  // CR3 (Sols es carrega si està activada la paginació)
  if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 28,
                           &tmp32, true, true ) != 0 )
    return false;
  if ( l_CR0&CR0_PG )
    {
      l_CR3= (l_CR3&(~CR3_PDB)) | (tmp32&CR3_PDB);
      paging_32b_CR3_changed ( jit );
    }

  // Consolida coses
  l_EFLAGS= eflags_tmp | EFLAGS_1S;
  
  return true;

 excp_np_error:
  printf("TODO - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
  EXCEPTION_ERROR_CODE ( EXCP_NP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
 excp_gp_error:
  printf("TODO - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
  EXCEPTION_ERROR_CODE ( EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
 excp_ts_error:
  printf("TODO - RECOVER EXCEPTION_TASK_SWITCH\n");exit(EXIT_FAILURE);
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
  
} // end switch_task

  
static bool
interruption_protected_intra_pl (
                                 IA32_JIT        *jit,
                                 const uint32_t   idt_w0,
                                 const uint32_t   idt_w1,
                                 const uint16_t   selector,
                                 seg_desc_t      *desc,
                                 const uint8_t    ext,
                                 const uint16_t   error_code,
                                 const bool       use_error_code,
                                 const uint32_t   old_EIP
                                 )
{

  int nbytes;
  uint32_t offset,idt_offset;
  IA32_SegmentRegister tmp_seg;
  bool idt_is32,idt_is_intgate;
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  
  // NOTA!!! Assumisc EFER.LMA==0, aquest bi sols té sentit en 64bits.

  // Comprova espai en la pila.
  if ( SEG_DESC_B_IS_SET(*desc) ) // 32-bit gate
    nbytes= use_error_code ? 16 : 12;
  else // 16-bit gate
    nbytes= use_error_code ? 8 : 6;
  offset=
    l_P_SS->h.is32 ? (l_ESP-nbytes) : ((uint32_t) ((uint16_t) (l_SP-nbytes)));
  if ( offset < l_P_SS->h.lim.firstb ||
       offset > (l_P_SS->h.lim.lastb-(nbytes-1)) )
    { EXCEPTION_ERROR_CODE ( EXCP_SS, ext ); return false; }

  // Carrega descriptor codi i comprova IDT
  load_segment_descriptor ( &tmp_seg, (selector&0xFFFC)|CPL, desc );
  idt_offset= (idt_w1&0xFFFF0000) | (idt_w0&0x0000FFFF);
  if ( idt_offset < tmp_seg.h.lim.firstb || idt_offset > tmp_seg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return false; }

  // Desa valors en la pila
  idt_is32= (idt_w1&0x800)!=0;
  if ( idt_is32 )
    {
      // NOTA!! En principi no deuria fallar el PUSH perquè ja he
      // comprovat abans
      if ( !push32 ( jit, l_EFLAGS ) ) return false;
      if ( !push32 ( jit, (uint32_t) l_P_CS->v ) ) return false;
      if ( !push32 ( jit, old_EIP ) ) return false;
      set_segment_descriptor_access_bit ( jit, desc );
      *l_P_CS= tmp_seg;
      l_EIP= idt_offset;
      if ( use_error_code ) {
        if ( !push32 ( jit, (uint32_t) error_code ) ) return false;
      }
    }
  else
    {
      if ( !push16 ( jit, (uint16_t) l_EFLAGS ) ) return false;
      if ( !push16 ( jit, l_P_CS->v ) ) return false;
      if ( !push16 ( jit, (uint16_t) old_EIP ) ) return false;
      set_segment_descriptor_access_bit ( jit, desc );
      *l_P_CS= tmp_seg;
      l_EIP= idt_offset&0xFFFF; // <-- He de descartar la part de dalt???
      if ( use_error_code ) {
        if ( !push16 ( jit, error_code ) ) return false;
      }
    }
  
  // Flags
  idt_is_intgate= (idt_w1&0x7E0)==0x600;
  if ( idt_is_intgate )
    { l_EFLAGS&= ~(IF_FLAG|TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  else { l_EFLAGS&= ~(TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  
  return true;
  
} // end interruption_protected_intra_pl


static bool
interruption_protected_from_vm86 (
                                  IA32_JIT       *jit,
                                  const uint32_t  old_EIP,
                                  const uint32_t  idt_w0,
                                  const uint32_t  idt_w1,
                                  const uint16_t  selector,
                                  seg_desc_t     *desc,
                                  const uint8_t   ext,
                                  const uint16_t  error_code,
                                  const bool      use_error_code
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
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  // Identify stack-segment selector for privilege level 0 in current TSS
  if ( l_TR.h.tss_is32 )
    {
      if ( l_TR.h.lim.lastb < 9 ) goto excp_ts_error;
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 8,
                               &new_SS_selector, true, true ) != 0 )
        return false;
      if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr + 4,
                               &new_ESP, true, true ) != 0 )
        return false;
    }
  else // TSS is 16-bit
    {
      if ( l_TR.h.lim.lastb < 5 ) goto excp_ts_error;
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 4,
                               &new_SS_selector, true, true ) != 0 )
        return false;
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr + 2,
                               &tmp16, true, true ) != 0 )
        return false;
      new_ESP= (uint32_t) tmp16;
    }
  new_SP= (uint16_t) (new_ESP&0xFFFF);
  
  // Llig newSS
  if ( !int_read_segment_descriptor ( jit, new_SS_selector,
                                      &new_SS_desc, ext, true ) )
    return false;
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
      return false;
    }
  
  // Carrega descriptor codi i comprova IDT
  load_segment_descriptor ( &tmp_seg, selector, desc );
  idt_offset= (idt_w1&0xFFFF0000) | (idt_w0&0x0000FFFF);
  if ( idt_offset < tmp_seg.h.lim.firstb || idt_offset > tmp_seg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return false; }
  
  // Flags
  tmp_eflags= l_EFLAGS;
  idt_is_intgate= (idt_w1&0x7E0)==0x600;
  if ( idt_is_intgate )
    { l_EFLAGS&= ~(IF_FLAG|TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  else { l_EFLAGS&= ~(TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  
  // Fixa nova pila.
  tmp_SS_selector= l_P_SS->v;
  tmp_ESP= l_ESP;
  *(l_P_SS)= new_SS;
  l_ESP= new_ESP;
  
  // Desa valors en la pila
  idt_is32= (idt_w1&0x800)!=0;
  if ( idt_is32 )
    {
      // NOTA!! En principi no deuria fallar el PUSH perquè ja he
      // comprovat abans
      if ( !push32 ( jit, (uint32_t) l_P_GS->v ) ) return false;
      if ( !push32 ( jit, (uint32_t) l_P_FS->v ) ) return false;
      if ( !push32 ( jit, (uint32_t) l_P_DS->v ) ) return false;
      if ( !push32 ( jit, (uint32_t) l_P_ES->v ) ) return false;
      if ( !push32 ( jit, (uint32_t) tmp_SS_selector ) ) return false;
      if ( !push32 ( jit, tmp_ESP ) ) return false;
      if ( !push32 ( jit, tmp_eflags ) ) return false;
      if ( !push32 ( jit, (uint32_t) l_P_CS->v ) ) return false;
      if ( !push32 ( jit, old_EIP ) ) return false;
      if ( use_error_code ) {
        if ( !push32 ( jit, (uint32_t) error_code ) ) return false;
      }
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
  l_P_GS->v= 0x0000; l_P_GS->h.isnull= true;
  l_P_FS->v= 0x0000; l_P_FS->h.isnull= true;
  l_P_DS->v= 0x0000; l_P_DS->h.isnull= true;
  l_P_ES->v= 0x0000; l_P_ES->h.isnull= true;

  // Fixa el CS
  *l_P_CS= tmp_seg;
  l_EIP= idt_offset;
  // NOTA MOLT IMPORTANT!!!! Segons la documentació *IF OperandSize =
  // 32* aleshores es trunca EIP. Fins ahí tot bé, però clar, no queda
  // gents clar a què fa referència eixe OperandSize, el P_CS s'acaba
  // de canviar així com molts flags. Es cacula amb el nou valor o amb
  // el vell? I si es calcula amb el nou i estem parlant d'una
  // instrucció INT, té impacte el switch operand size de la
  // instrucció INT? En la versió no JIT es calcula amb el nou i sí
  // que té impacte el switch. Ací per simplificar asumiré que es
  // calcula amb el nou però no té impacte el switch.
  if ( ((l_CR0&CR0_PE)==0) ||
       ((l_EFLAGS&VM_FLAG) != 0) ||
       !l_P_CS->h.is32 )
    l_EIP&= 0xFFFF;
  
  return true;

 excp_ts_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS, int_error_code ( l_TR.v&0xFFFC, 0, ext ) );
  return false;
 excp_ts_new_ss_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( new_SS_selector&0xFFFC, 0, ext ) );
  return false;
  
} // end interruption_protected_from_vm86


static int
interruption_protected_inter_pl (
                                 IA32_JIT       *jit,
                                 const uint32_t  old_EIP,
                                 const uint32_t  idt_w0,
                                 const uint32_t  idt_w1,
                                 const uint16_t  selector,
                                 seg_desc_t     *desc,
                                 const uint8_t   ext,
                                 const uint16_t  error_code,
                                 const bool      use_error_code
                                 )
{

  int nbytes;
  uint8_t dpl,new_SS_rpl,new_SS_dpl;
  uint16_t new_SS_selector,new_SP,tmp_SS_selector,tmp16;
  uint32_t offset,new_ESP,new_SS_stype,idt_offset,tmp_ESP;
  seg_desc_t new_SS_desc;
  IA32_SegmentRegister new_SS,tmp_seg;
  bool idt_is_intgate,idt_is32;
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  // Identify stack-segment selector for new privilege level in current TSS.
  dpl= SEG_DESC_GET_DPL(*desc);
  assert ( dpl < 3 );
  if ( l_TR.h.tss_is32 )
    {
      offset= (dpl<<3) + 4;
      if ( (offset+5) > l_TR.h.lim.lastb ) goto excp_ts_error;
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr+offset+4,
                               &new_SS_selector, true, true ) != 0 )
        return false;
      if ( jit->_mem_readl32 ( jit, l_TR.h.lim.addr+offset,
                               &new_ESP, true, true ) != 0 )
        return false;
    }
  else // TSS is 16-bit
    {
      offset= (dpl<<2) + 2;
      if ( (offset+3) > l_TR.h.lim.lastb ) goto excp_ts_error;
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr+offset+2,
                               &new_SS_selector, true, true ) != 0 )
        return false;
      if ( jit->_mem_readl16 ( jit, l_TR.h.lim.addr+offset,
                               &tmp16, true, true ) != 0 )
        return false;
      new_ESP= (uint32_t) tmp16;
    }
  new_SP= (uint16_t) (new_ESP&0xFFFF);
  
  // Llig newSS
  if ( (new_SS_selector&0xFFFC) == 0 ) // Null segment selector..
    { EXCEPTION_ERROR_CODE ( EXCP_TS, ext ); return false; };
  if ( !int_read_segment_descriptor ( jit, new_SS_selector,
                                     &new_SS_desc, ext, true ) )
    return false;
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
      return false;
    }

  // Carrega descriptor codi i comprova IDT
  load_segment_descriptor ( &tmp_seg, selector, desc );
  idt_offset= (idt_w1&0xFFFF0000) | (idt_w0&0x0000FFFF);
  if ( idt_offset < tmp_seg.h.lim.firstb || idt_offset > tmp_seg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return false; }

  // Fixa nova pila.
  tmp_SS_selector= l_P_SS->v;
  tmp_ESP= l_ESP;
  *(l_P_SS)= new_SS;
  l_ESP= new_ESP;
  
  // Desa valors en la pila
  idt_is32= (idt_w1&0x800)!=0;
  if ( idt_is32 )
    {
      // NOTA!! En principi no deuria fallar el PUSH perquè ja he
      // comprovat abans
      if ( !push32 ( jit, (uint32_t) tmp_SS_selector ) ) return false;
      if ( !push32 ( jit, tmp_ESP ) ) return false;
      if ( !push32 ( jit, l_EFLAGS ) ) return false;
      if ( !push32 ( jit, (uint32_t) l_P_CS->v ) ) return false;
      if ( !push32 ( jit, old_EIP ) ) return false;
      if ( use_error_code ) {
        if ( !push32 ( jit, (uint32_t) error_code ) ) return false;
      }
    }
  else
    {
      if ( !push16 ( jit, tmp_SS_selector ) ) return false;
      if ( !push16 ( jit, (uint16_t) tmp_ESP ) ) return false;
      if ( !push16 ( jit, (uint16_t) l_EFLAGS ) ) return false;
      if ( !push16 ( jit, l_P_CS->v ) ) return false;
      if ( !push16 ( jit, (uint16_t) old_EIP ) ) return false;
      if ( use_error_code ) {
        if ( !push16 ( jit, error_code ) ) return false;
      }
    }
  
  // Fixa el CS, EIP i EFLAGS.
  tmp_seg.h.pl= dpl; // CPL ← new code-segment DPL;
  tmp_seg.v= (tmp_seg.v&0xFFFC) | dpl; // CS(RPL) ← CPL;
  idt_is_intgate= (idt_w1&0x7E0)==0x600;
  if ( idt_is_intgate )
    { l_EFLAGS&= ~(IF_FLAG|TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  else { l_EFLAGS&= ~(TF_FLAG|NT_FLAG|VM_FLAG|RF_FLAG); }
  *l_P_CS= tmp_seg;
  l_EIP= idt_offset;
  // NOTA MOLT IMPORTANT!!!! Segons la documentació *IF OperandSize =
  // 32* aleshores es trunca EIP. Fins ahí tot bé, però clar, no queda
  // gents clar a què fa referència eixe OperandSize, el P_CS s'acaba
  // de canviar així com molts flags. Es cacula amb el nou valor o amb
  // el vell? I si es calcula amb el nou i estem parlant d'una
  // instrucció INT, té impacte el switch operand size de la
  // instrucció INT? En la versió no JIT es calcula amb el nou i sí
  // que té impacte el switch. Ací per simplificar asumiré que es
  // calcula amb el nou però no té impacte el switch.
  if ( ((l_CR0&CR0_PE)==0) ||
       ((l_EFLAGS&VM_FLAG) != 0) ||
       !l_P_CS->h.is32 )
    l_EIP&= 0xFFFF;
  
  return true;
  
 excp_ts_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( l_TR.v&0xFFFC, 0, ext ) );
  return false;
 excp_ts_new_ss_error:
  EXCEPTION_ERROR_CODE ( EXCP_TS,
                         int_error_code ( new_SS_selector&0xFFFC, 0, ext ) );
  return false;
  
} // end interruption_protected_inter_pl


static bool
interruption_protected_trap_int_gate (
                                      IA32_JIT       *jit,
                                      const int       type,
                                      const uint8_t   vec,
                                      const uint32_t  d_w0,
                                      const uint32_t  d_w1,
                                      const uint8_t   ext,
                                      const uint16_t  error_code,
                                      const bool      use_error_code,
                                      const uint32_t  old_EIP
                                      )
{

  uint8_t dpl;
  uint16_t selector;
  uint32_t stype;
  seg_desc_t desc;
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  // Llig descriptor
  selector= (uint16_t) (d_w0>>16);
  if ( !int_read_segment_descriptor ( jit, selector, &desc, ext, false ) )
    return false;

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
      if ( (l_EFLAGS&VM_FLAG) == 0 )
        {
          if ( !interruption_protected_inter_pl ( jit, old_EIP, d_w0, d_w1,
                                                  selector, &desc, ext,
                                                  error_code,
                                                  use_error_code ))
            return false;
        }
      else // VM==1
        { 
          if ( dpl != 0 ) goto excp_gp_error;
          else
            {
              if ( !interruption_protected_from_vm86 ( jit, old_EIP, d_w0, d_w1,
                                                       selector, &desc, ext,
                                                       error_code,
                                                       use_error_code ))
                return false;
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
          if ( !interruption_protected_intra_pl ( jit, d_w0, d_w1,
                                                  selector, &desc, ext,
                                                  error_code,
                                                  use_error_code, old_EIP ) )
            return false;
        }
      // PE = 1, interrupt or trap gate, nonconforming code segment, DPL > CPL
      else goto excp_gp_error;
    }
  
  return true;

 excp_gp_error:
  EXCEPTION_ERROR_CODE ( EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
 excp_np_error:
  EXCEPTION_ERROR_CODE ( EXCP_NP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
  
} // end interruption_protected_trap_int_gate


static bool
interruption_protected_task_gate (
                                  IA32_JIT       *jit,
                                  const int      type,
                                  const uint8_t  vec,
                                  const uint32_t d_w0,
                                  const uint32_t d_w1,
                                  const uint8_t  ext,
                                  const uint16_t error_code,
                                  const bool     use_error_code,
                                  const uint32_t old_EIP
                                  )
{
  
  uint16_t selector;
  IA32_CPU *l_cpu;
  

  l_cpu= jit->_cpu;
  
  // TSS Selector
  selector= (uint16_t) (d_w0>>16);
  if ( (selector&0xFFFC) == 0 ) goto excp_gp_error;
  if ( selector&0x4 ) goto excp_gp_error; // LDT

  // Switch task (Inclou comprovació bit busy i si està o no present)
  if ( !switch_task ( jit,
                      ext==0 ? SWITCH_TASK_INT : SWITCH_TASK_EXC,
                      selector, ext, old_EIP ) )
    return false;

  // Comprovacions finals.
  if ( use_error_code )
    {
      if ( !push32 ( jit, (uint32_t) error_code ) )
        return false;
    }
  if ( l_EIP < l_P_CS->h.lim.firstb || l_EIP > l_P_CS->h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, ext ); return false; }
  
  return true;
  
 excp_gp_error:
  EXCEPTION_ERROR_CODE ( EXCP_GP,
                         int_error_code ( selector&0xFFFC, 0, ext ) );
  return false;
  
} // end interruption_protected_task_gate


static bool
interruption_protected (
                        IA32_JIT       *jit,
                        const uint32_t  old_EIP,
                        const int       type,
                        const uint8_t   vec,
                        const uint16_t  error_code,
                        const bool      use_error_code
                        )
{

  uint32_t offset,d_w0,d_w1;
  uint8_t ext,dpl;
  
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  
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
  if ( offset+7 > l_IDTR.lastb ) goto excp_gp_error;
  offset+= l_IDTR.addr;
  if ( jit->_mem_readl32 ( jit, offset, &d_w0, true, true ) != 0 )
    return false;
  if ( jit->_mem_readl32 ( jit, offset+4, &d_w1, true, true ) != 0 )
    return false;
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
      if ( !interruption_protected_task_gate ( jit, type, vec,
                                              d_w0, d_w1, ext,
                                              error_code,
                                              use_error_code,
                                              old_EIP ) )
        return false;
    }
  else
    {
      if ( !interruption_protected_trap_int_gate ( jit, type, vec,
                                                   d_w0, d_w1, ext,
                                                   error_code,
                                                   use_error_code,
                                                   old_EIP ) )
        return false;
    }
  
  return true;
  
 excp_gp:
  EXCEPTION0 ( EXCP_GP );
  return false;
 excp_gp_error:
  EXCEPTION_ERROR_CODE ( EXCP_GP, int_error_code ( vec, 1, ext ) );
  return false;
 excp_np_error:
  EXCEPTION_ERROR_CODE ( EXCP_NP, int_error_code ( vec, 1, ext ) );
  return false;
  
} // end interruption_protected


static bool
interruption_nonprotected (
                           IA32_JIT       *jit,
                           const uint32_t  old_EIP,
                           const uint8_t   vec
                           )
{

  uint32_t offset,aux1,aux2,aux3;
  uint16_t tmp16;
  IA32_CPU *l_cpu;
  
  
  l_cpu= jit->_cpu;
  
  offset= vec<<2;
  if ( offset+3 > l_IDTR.lastb ) { EXCEPTION ( EXCP_GP ); return false; }
  // Comprovacions abans de fer push de res
  if ( l_P_SS->h.is32 )
    {
      aux1= (l_ESP-2);
      aux2= (l_ESP-4);
      aux3= (l_ESP-6);
    }
  else
    {
      aux1= (uint32_t) ((uint16_t) (l_SP-2));
      aux2= (uint32_t) ((uint16_t) (l_SP-4));
      aux3= (uint32_t) ((uint16_t) (l_SP-6));
    }
  if ( aux1 > 0xFFFE || aux2 > 0xFFFE || aux3 > 0xFFFE )
    { EXCEPTION ( EXCP_SS ); return false; }
  if ( !push16 ( jit, ((uint16_t) l_EFLAGS) ) ) return false;
  l_EFLAGS&= ~(IF_FLAG|TF_FLAG|AC_FLAG);
  if ( !push16 ( jit, l_P_CS->v ) ) return false;
  if ( !push16 ( jit, ((uint16_t) (old_EIP)) ) ) return false;
  offset+= l_IDTR.addr;
  if ( jit->_mem_readl16 ( jit, offset, &tmp16, true, true ) != 0 )
    return false;
  l_EIP= (uint32_t) tmp16;
  if ( jit->_mem_readl16 ( jit, offset+2, &tmp16, true, true ) != 0 )
    return false;
  l_P_CS->v= tmp16;
  l_P_CS->h.lim.addr= ((uint32_t) tmp16)<<4;
  l_P_CS->h.lim.firstb= 0;
  l_P_CS->h.lim.lastb= 0xFFFF;
  l_P_CS->h.is32= false;
  l_P_CS->h.readable= true;
  l_P_CS->h.writable= true;
  l_P_CS->h.executable= true;
  l_P_CS->h.isnull= false;
  l_P_CS->h.data_or_nonconforming= false;
  
  return true;
  
} // end interruption_nonprotected


static bool
interruption (
              IA32_JIT      *jit,
              const uint32_t old_EIP,
              const int      type,
              const uint8_t  vec,
              const uint16_t error_code,
              const bool     use_error_code
              )
{
  
  return
    ( PROTECTED_MODE_ACTIVATED ) ?
    interruption_protected ( jit, old_EIP, type, vec,
                             error_code, use_error_code ):
    interruption_nonprotected ( jit, old_EIP, vec );
  
} // end interruption


#define MAX_NESTED_EXCEPTIONS 5

static void
exception_body (
                IA32_JIT *jit
                )
{
  
  uint32_t return_eip;
  bool ok;
  int counter;
  
  
  // Prepara
  ok= false;
  counter= 0;
  if ( jit->_exception.vec == -1 )
    {
      fprintf ( stderr, "[EE] jit_exec - exception:  vector a -1 !!!\n");
      exit ( EXIT_FAILURE );
    }

  while ( !ok && counter < MAX_NESTED_EXCEPTIONS )
    {

      ++counter;
      
      // Obté EIP
      // Típicament es crida abans d'actualitar l'EIP.
      if ( EXCP_IS_FAULT ( jit->_exception.vec ) )
        return_eip= EIP; // Encara no està actualitzat
      else
        {
          return_eip= 0;
          fprintf ( FERROR, "[CAL IMPLEMENTAR] exception no és FAULT: %d\n",
                    jit->_exception.vec );
          exit ( EXIT_FAILURE );
        }
      
      if ( jit->_exception.with_selector )
        ok= interruption ( jit, return_eip,
                           INTERRUPTION_TYPE_UNK,
                           jit->_exception.vec,
                           jit->_exception.selector&0xFFFC,
                           true );
      else
        ok= interruption ( jit, return_eip,
                           INTERRUPTION_TYPE_UNK,
                           jit->_exception.vec,
                           jit->_exception.use_error_code ?
                           jit->_exception.error_code : 0,
                           jit->_exception.use_error_code );
      
    }

  if ( ok )
    {
      jit->_exception.vec= -1;
      jit->_exception.with_selector= false;
      jit->_exception.use_error_code= false;
    }
  else
    {
      fprintf ( FERROR, "ASSOLIT MÀXIM NOMBRE D'EXCEPCIÓ IMBRICADES\n" );
      exit ( EXIT_FAILURE );
    }
  
} // end exception_body


static bool
goto_eip_body (
               IA32_JIT *jit
               )
{

  uint32_t addr,page,inst,pos;
  const IA32_JIT_MemMap *mem_map;
  int area;
  IA32_JIT_Page *p;
  

  addr= P_CS->h.lim.addr + (EIP);
  if ( !translate_addr ( jit, &addr ) )
    return false;
  area= 0;
  mem_map= jit->_mem_map;
  do {
    if ( addr >= mem_map->first_addr && addr <= mem_map->last_addr )
      {
        page= (addr-mem_map->first_addr)>>jit->_bits_page;
        inst= addr&(jit->_page_low_mask);
        do {

          // Fixa la pàgina.
          p= jit->_current_page= mem_map->map[page];
          if ( p == NULL )
            {
              p= jit->_current_page=
                mem_map->map[page]=
                get_new_page ( jit );
              p->page_id= page;
              p->area_id= area;
            }

          // Fixa la posició
          pos= jit->_current_pos= p->entries[inst].ind;
          if ( pos != NULL_ENTRY )
            {
              if ( pos != PAD_ENTRY &&
                   p->entries[inst].is32==ADDR_OP_SIZE_IS_32 )
                return true; // FET !!!!!
              else remove_page ( jit, area, page ); // Esborra i repeteix
            }
          else
            {
              if ( !dis_insts ( jit, p, addr, EIP, &jit->_current_pos ) )
                {
                  remove_page ( jit, area, page );
                  return false;
                }
              if ( jit->_current_pos != NULL_ENTRY ) return true; // FET !!!
              else remove_page ( jit, area, page ); // Esborra i repeteix
            }
        } while ( true );
      }
    else { ++area; ++mem_map; }
  } while ( area < jit->_mem_map_size );

  // ERROR
  fprintf ( FERROR,
            "[EE] IA32 JIT - pàgina fora de rang (ADDR: %08X)\n",
            addr );
  exit ( EXIT_FAILURE );
  return false; // CALLA
  
} // end goto_eip_body


static void
exception (
           IA32_JIT *jit
           )
{
  
  do {
    exception_body ( jit );
  } while ( !goto_eip_body ( jit ) );
  
} // end exception


static void
goto_eip (
          IA32_JIT *jit
          )
{
  
  while ( !goto_eip_body ( jit ) )
    exception_body ( jit );
  
} // end goto_eip


static void
exec_inst (
           IA32_JIT *jit
           )
{

  IA32_CPU *l_cpu;
  IA32_JIT_Page *p;
  uint32_t pos;
  // -> Registres adicionals
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
  uint64_t res64,tmp64;
  uint32_t offset,op32[2],res32,next_eip,tmp32;
  uint16_t selector,op16[2],res16,port,tmp16;
  uint8_t op8[2],res8,tmp8;
  bool cond,tmp_bool,fpu_ok;
  int count,fpu_reg,fpu_reg2,tmp_int;
  float_u32_t float_u32;
  double_u64_t double_u64;
  long double ldouble;
  ldouble_u80_t ldouble_u80;
  IA32_FPURegister tmp_fpu_reg;
  seg_desc_t desc;
  
  
  // NOTA!! El valor al final de jit->_current_pos tampoc importa molt
  // si la pàgina actual s'esborra. Si paixa això cal recordar que
  // simplement jit->_current_page serà NULL i per tant el valor de
  // current_pos serà irrellevant.
  
  // Inicialitza current_page si cal
  if ( jit->_current_page == NULL )
    goto_eip ( jit );

  // Prepara iteració.
  jit->_lock_page= p= jit->_current_page;
  
  // Itera
  l_cpu= jit->_cpu;
  pos= jit->_current_pos;;
  do {
    switch ( p->v[pos] )
      {
        
        // FINALS INSTRUCCIÓ
      case BC_GOTO_EIP: goto_eip ( jit ); goto stop; break;
      case BC_INC1_EIP: ++l_EIP; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC2_EIP: l_EIP+= 2; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC3_EIP: l_EIP+= 3; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC4_EIP: l_EIP+= 4; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC5_EIP: l_EIP+= 5; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC6_EIP: l_EIP+= 6; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC7_EIP: l_EIP+= 7; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC8_EIP: l_EIP+= 8; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC9_EIP: l_EIP+= 9; jit->_current_pos= pos+1; goto stop; break;
      case BC_INC10_EIP:
        l_EIP+= 10; jit->_current_pos= pos+1;
        goto stop;
        break;
      case BC_INC11_EIP:
        l_EIP+= 11; jit->_current_pos= pos+1;
        goto stop;
        break;
      case BC_INC12_EIP:
        l_EIP+= 12; jit->_current_pos= pos+1;
        goto stop;
        break;
      case BC_INC14_EIP:
        l_EIP+= 14; jit->_current_pos= pos+1;
        goto stop;
        break;
      case BC_INC1_EIP_NOSTOP: ++l_EIP; break;
      case BC_INC2_EIP_NOSTOP: l_EIP+= 2; break;
      case BC_INC3_EIP_NOSTOP: l_EIP+= 3; break;
      case BC_INC4_EIP_NOSTOP: l_EIP+= 4; break;
      case BC_INC5_EIP_NOSTOP: l_EIP+= 5; break;
      case BC_INC6_EIP_NOSTOP: l_EIP+= 6; break;
      case BC_INC7_EIP_NOSTOP: l_EIP+= 7; break;
      case BC_INC8_EIP_NOSTOP: l_EIP+= 8; break;
      case BC_INC9_EIP_NOSTOP: l_EIP+= 9; break;
      case BC_INC10_EIP_NOSTOP: l_EIP+= 10; break;
      case BC_INC11_EIP_NOSTOP: l_EIP+= 11; break;
      case BC_INC12_EIP_NOSTOP: l_EIP+= 12; break;
      case BC_INC14_EIP_NOSTOP: l_EIP+= 14; break;
      case BC_WRONG_INST:
        jit->_exception.vec= (int) p->v[++pos];
        jit->_exception.with_selector= false;
        exception ( jit );
        goto stop;
        break;
      case BC_INC2_PC_IF_ECX_IS_0: if ( l_ECX == 0 ) pos+= 2; break;
      case BC_INC2_PC_IF_CX_IS_0: if ( l_CX == 0 ) pos+= 2; break;
      case BC_INC4_PC_IF_ECX_IS_0: if ( l_ECX == 0 ) pos+= 4; break;
      case BC_INC4_PC_IF_CX_IS_0: if ( l_CX == 0 ) pos+= 4; break;
      case BC_INCIMM_PC_IF_CX_IS_0:
        tmp16= p->v[++pos];
        if ( l_CX == 0 ) pos+= (uint32_t) tmp16;
        break;
      case BC_INCIMM_PC_IF_ECX_IS_0:
        tmp16= p->v[++pos];
        if ( l_ECX == 0 ) pos+= (uint32_t) tmp16;
        break;
      case BC_DEC1_PC_IF_REP32:
        if ( --l_ECX != 0 ) { jit->_current_pos= pos-1; goto stop; }
        break;
      case BC_DEC1_PC_IF_REP16:
        if ( --l_CX != 0 ) { jit->_current_pos= pos-1; goto stop; }
        break;
      case BC_DEC3_PC_IF_REP32:
        if ( --l_ECX != 0 ) { jit->_current_pos= pos-3; goto stop; }
        break;
      case BC_DEC3_PC_IF_REP16:
        if ( --l_CX != 0 ) { jit->_current_pos= pos-3; goto stop; }
        break;
      case BC_DECIMM_PC_IF_REPE32:
        tmp16= p->v[++pos];
        if ( --l_ECX != 0 && (l_EFLAGS&ZF_FLAG)!=0 )
          { jit->_current_pos= pos-1-((int) tmp16); goto stop;}
        break;
      case BC_DECIMM_PC_IF_REPNE32:
        tmp16= p->v[++pos];
        if ( --l_ECX != 0 && (l_EFLAGS&ZF_FLAG)==0 )
          { jit->_current_pos= pos-1-((int) tmp16); goto stop;}
        break;
      case BC_DECIMM_PC_IF_REPE16:
        tmp16= p->v[++pos];
        if ( --l_CX != 0 && (l_EFLAGS&ZF_FLAG)!=0 )
          { jit->_current_pos= pos-1-((int) tmp16); goto stop;}
        break;
      case BC_DECIMM_PC_IF_REPNE16:
        tmp16= p->v[++pos];
        if ( --l_CX != 0 && (l_EFLAGS&ZF_FLAG)==0 )
          { jit->_current_pos= pos-1-((int) tmp16); goto stop;}
        break;
      case BC_UNK:
        fprintf ( FERROR, "[EE] Instrucció desconeguda %X\n", p->v[++pos] );
        exit ( EXIT_FAILURE );
        goto stop;
        break;
        
        // CARREGA DADES
        // --> Assignació offsets (i selectors)
      case BC_SET32_IMM_SELECTOR_OFFSET:
        selector= p->v[pos+1];
        offset= ((uint32_t) p->v[pos+2]) | (((uint32_t) (p->v[pos+3]))<<16);
        pos+= 3;
        break;
      case BC_SET32_IMM_OFFSET:
        offset= ((uint32_t) p->v[pos+1]) | (((uint32_t) (p->v[pos+2]))<<16);
        pos+= 2;
        break;
      case BC_SET16_IMM_SELECTOR_OFFSET:
        selector= p->v[++pos];
        offset= (uint32_t) (p->v[++pos]);
        break;
      case BC_SET16_IMM_OFFSET: offset= (uint32_t) (p->v[++pos]); break;
      case BC_SET16_IMM_PORT: port= p->v[++pos]; break;
      case BC_SET32_EAX_OFFSET: offset= l_EAX; break;
      case BC_SET32_ECX_OFFSET: offset= l_ECX; break;
      case BC_SET32_EDX_OFFSET: offset= l_EDX; break;
      case BC_SET32_EBX_OFFSET: offset= l_EBX; break;
      case BC_SET32_EBX_AL_OFFSET: offset= l_EBX + ((uint32_t) l_AL); break;
      case BC_SET32_EBP_OFFSET: offset= l_EBP; break;
      case BC_SET32_ESI_OFFSET: offset= l_ESI; break;
      case BC_SET32_EDI_OFFSET: offset= l_EDI; break;
      case BC_SET32_ESP_OFFSET: offset= l_ESP; break;
      case BC_SET16_SI_OFFSET: offset= (uint32_t) l_SI; break;
      case BC_SET16_DI_OFFSET: offset= (uint32_t) l_DI; break;
      case BC_SET16_BP_OFFSET: offset= (uint32_t) l_BP; break;
      case BC_SET16_BP_SI_OFFSET: offset= (uint32_t) (l_BP+l_SI)&0xFFFF; break;
      case BC_SET16_BP_DI_OFFSET: offset= (uint32_t) (l_BP+l_DI)&0xFFFF; break;
      case BC_SET16_BX_OFFSET: offset= (uint32_t) l_BX; break;
      case BC_SET16_BX_AL_OFFSET:
        offset= (uint32_t) (l_BX + (uint16_t) l_AL);
        break;
      case BC_SET16_BX_SI_OFFSET: offset= (uint32_t) (l_BX+l_SI)&0xFFFF; break;
      case BC_SET16_BX_DI_OFFSET: offset= (uint32_t) (l_BX+l_DI)&0xFFFF; break;
      case BC_SET16_DX_PORT: port= l_DX; break;
      case BC_ADD32_BITOFFSETOP1_OFFSET:
        offset+= (uint32_t) (((int32_t) (op32[1]>>5))<<2);
        break;
      case BC_ADD32_IMM_OFFSET:
        offset+= (((uint32_t) p->v[pos+1]) | (((uint32_t) (p->v[pos+2]))<<16));
        pos+= 2;
        break;
      case BC_ADD32_EAX_OFFSET: offset+= l_EAX; break;
      case BC_ADD32_ECX_OFFSET: offset+= l_ECX; break;
      case BC_ADD32_EDX_OFFSET: offset+= l_EDX; break;
      case BC_ADD32_EBX_OFFSET: offset+= l_EBX; break;
      case BC_ADD32_EBP_OFFSET: offset+= l_EBP; break;
      case BC_ADD32_ESI_OFFSET: offset+= l_ESI; break;
      case BC_ADD32_EDI_OFFSET: offset+= l_EDI; break;
      case BC_ADD32_EAX_2_OFFSET: offset+= (l_EAX<<1); break;
      case BC_ADD32_ECX_2_OFFSET: offset+= (l_ECX<<1); break;
      case BC_ADD32_EDX_2_OFFSET: offset+= (l_EDX<<1); break;
      case BC_ADD32_EBX_2_OFFSET: offset+= (l_EBX<<1); break;
      case BC_ADD32_EBP_2_OFFSET: offset+= (l_EBP<<1); break;
      case BC_ADD32_ESI_2_OFFSET: offset+= (l_ESI<<1); break;
      case BC_ADD32_EDI_2_OFFSET: offset+= (l_EDI<<1); break;
      case BC_ADD32_EAX_4_OFFSET: offset+= (l_EAX<<2); break;
      case BC_ADD32_ECX_4_OFFSET: offset+= (l_ECX<<2); break;
      case BC_ADD32_EDX_4_OFFSET: offset+= (l_EDX<<2); break;
      case BC_ADD32_EBX_4_OFFSET: offset+= (l_EBX<<2); break;
      case BC_ADD32_EBP_4_OFFSET: offset+= (l_EBP<<2); break;
      case BC_ADD32_ESI_4_OFFSET: offset+= (l_ESI<<2); break;
      case BC_ADD32_EDI_4_OFFSET: offset+= (l_EDI<<2); break;
      case BC_ADD32_EAX_8_OFFSET: offset+= (l_EAX<<3); break;
      case BC_ADD32_ECX_8_OFFSET: offset+= (l_ECX<<3); break;
      case BC_ADD32_EDX_8_OFFSET: offset+= (l_EDX<<3); break;
      case BC_ADD32_EBX_8_OFFSET: offset+= (l_EBX<<3); break;
      case BC_ADD32_EBP_8_OFFSET: offset+= (l_EBP<<3); break;
      case BC_ADD32_ESI_8_OFFSET: offset+= (l_ESI<<3); break;
      case BC_ADD32_EDI_8_OFFSET: offset+= (l_EDI<<3); break;
      case BC_ADD16_IMM_OFFSET:
        offset+= (uint32_t) ((int32_t) ((int16_t) p->v[++pos]));
        break;
      case BC_ADD16_IMM_OFFSET16:
        offset= (offset +
                 ((uint32_t) ((int32_t) ((int16_t) p->v[++pos]))))&0xFFFF;
        break;
        // --> Condicions
      case BC_SET_A_COND: cond= ((l_EFLAGS&(ZF_FLAG|CF_FLAG))==0); break;
      case BC_SET_AE_COND: cond= ((l_EFLAGS&(CF_FLAG))==0); break;
      case BC_SET_B_COND: cond= ((l_EFLAGS&CF_FLAG)!=0); break;
      case BC_SET_CXZ_COND: cond= (l_CX==0); break;
      case BC_SET_E_COND: cond= ((l_EFLAGS&ZF_FLAG)!=0); break;
      case BC_SET_ECXZ_COND: cond= (l_ECX==0); break;
      case BC_SET_G_COND:
        cond=
          ((l_EFLAGS&ZF_FLAG)==0) &&
          !(((l_EFLAGS&SF_FLAG)==0)^((l_EFLAGS&OF_FLAG)==0));
        break;
      case BC_SET_GE_COND:
        cond= !(((l_EFLAGS&SF_FLAG)==0)^((l_EFLAGS&OF_FLAG)==0));
        break;
      case BC_SET_L_COND:
        cond= (((l_EFLAGS&SF_FLAG)==0)^((l_EFLAGS&OF_FLAG)==0));
        break;
      case BC_SET_NA_COND: cond= ((l_EFLAGS&(ZF_FLAG|CF_FLAG))!=0); break;
      case BC_SET_NE_COND: cond= ((l_EFLAGS&ZF_FLAG)==0); break;
      case BC_SET_NG_COND:
        cond=
          ((l_EFLAGS&ZF_FLAG)!=0) ||
          (((l_EFLAGS&SF_FLAG)==0)^((l_EFLAGS&OF_FLAG)==0));
        break;
      case BC_SET_NO_COND: cond= ((l_EFLAGS&OF_FLAG)==0); break;
      case BC_SET_NS_COND: cond= ((l_EFLAGS&SF_FLAG)==0); break;
      case BC_SET_O_COND: cond= ((l_EFLAGS&OF_FLAG)!=0); break;
      case BC_SET_P_COND: cond= ((l_EFLAGS&PF_FLAG)!=0); break;
      case BC_SET_PO_COND: cond= ((l_EFLAGS&PF_FLAG)==0); break;
      case BC_SET_S_COND: cond= ((l_EFLAGS&SF_FLAG)!=0); break;
      case BC_SET_DEC_ECX_NOT_ZERO_COND: --l_ECX; cond= (l_ECX!=0); break;
      case BC_SET_DEC_ECX_NOT_ZERO_AND_ZF_COND:
        --l_ECX;
        cond= (l_ECX!=0) && ((l_EFLAGS&ZF_FLAG)!=0);
        break;
      case BC_SET_DEC_ECX_NOT_ZERO_AND_NOT_ZF_COND:
        --l_ECX;
        cond= (l_ECX!=0) && ((l_EFLAGS&ZF_FLAG)==0);
        break;
      case BC_SET_DEC_CX_NOT_ZERO_COND: --l_CX; cond= (l_CX!=0); break;
      case BC_SET_DEC_CX_NOT_ZERO_AND_ZF_COND:
        --l_CX;
        cond= (l_CX!=0) && ((l_EFLAGS&ZF_FLAG)!=0);
        break;
      case BC_SET_DEC_CX_NOT_ZERO_AND_NOT_ZF_COND:
        --l_CX;
        cond= (l_CX!=0) && ((l_EFLAGS&ZF_FLAG)==0);
        break;
        // --> Comptador
      case BC_SET_1_COUNT: count= 1; break;
      case BC_SET_CL_COUNT: count= (l_CL&0x1F); break;
      case BC_SET_CL_COUNT_NOMOD: count= l_CL; break;
      case BC_SET_IMM_COUNT: count= p->v[++pos]; break;
        // --> EFLAGS
      case BC_SET32_AD_OF_EFLAGS:
        if (((~(op32[0]^op32[1]))&(op32[0]^res32)&0x80000000))
          l_EFLAGS|=OF_FLAG;
        else l_EFLAGS&= ~OF_FLAG;
        break;
      case BC_SET32_AD_AF_EFLAGS:
        if ( (((op32[0]^op32[1])^res32)&0x10) ) l_EFLAGS|= AF_FLAG;
        else                                    l_EFLAGS&= ~AF_FLAG;
        break;
      case BC_SET32_AD_CF_EFLAGS:
        if ( (((op32[0]&op32[1]) |
               ((op32[0]|op32[1])&(~res32)))&0x80000000) )
          l_EFLAGS|= CF_FLAG;
        else l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET32_MUL_CFOF_EFLAGS:
        if ( ((res32&0x80000000)!=0)^((res64&0x8000000000000000)!=0) )
          l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET32_MUL1_CFOF_EFLAGS:
        if ( l_EDX != 0 ) l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else              l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET32_IMUL1_CFOF_EFLAGS:
        if ( (((uint64_t) ((int64_t) ((int32_t) l_EAX)))!=res64 ) )
          l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET32_NEG_CF_EFLAGS:
        if ( op32[1] == 0 ) l_EFLAGS&= ~CF_FLAG;
        else                l_EFLAGS|= CF_FLAG;
        break;
      case BC_SET32_RCL_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op32[0]&0x80000000)!=0)^((op32[0]&0x40000000)!=0) )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET32_RCL_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( ((op32[0]>>(32-count))&0x1) != 0 ) l_EFLAGS|= CF_FLAG;
            else                                    l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET32_RCR_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op32[0]&0x80000000)!=0)^((l_EFLAGS&CF_FLAG)!=0) )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET32_RCR_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( ((op32[0]>>(count-1))&0x1) != 0 ) l_EFLAGS|= CF_FLAG;
            else                                   l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET32_ROL_CF_EFLAGS:
        if ( (res32&0x01) != 0 ) l_EFLAGS|= CF_FLAG;
        else                     l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET32_ROR_OF_EFLAGS:
        if ( count == 1 )
          {
            if (((op32[0]&0x80000000)^((op32[0]&0x01)<<31))!=0)
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET32_ROR_CF_EFLAGS:
        if ( (res32&0x80000000) != 0 ) l_EFLAGS|= CF_FLAG;
        else                           l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET32_SAR_OF_EFLAGS:
        if ( count == 1 ) { l_EFLAGS&= ~OF_FLAG; }
        break;
      case BC_SET32_SAR_CF_EFLAGS:
        if ( count == 32 )
          {
            if ( op32[0]&0x80000000 ) l_EFLAGS|= CF_FLAG;
            else                      l_EFLAGS&= ~CF_FLAG;
          }
        else if ( count > 0 )
          {
            if ( op32[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                          l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET32_SHIFT_SF_EFLAGS:
        if ( count > 0 && count <= 32 )
          {
            if ( res32&0x80000000 ) l_EFLAGS|= SF_FLAG;
            else                    l_EFLAGS&= ~SF_FLAG;
          }
        break;
      case BC_SET32_SHIFT_ZF_EFLAGS:
        if ( count > 0 && count <= 32 )
          {
            if ( res32 == 0 ) l_EFLAGS|= ZF_FLAG;
            else              l_EFLAGS&= ~ZF_FLAG;
          }
        break;
      case BC_SET32_SHIFT_PF_EFLAGS:
        if ( count > 0 && count <= 32 )
          {
            if ( PFLAG[(uint8_t) (res32&0xff)] ) l_EFLAGS|= PF_FLAG;
            else                                 l_EFLAGS&= ~PF_FLAG;
          }
        break;
      case BC_SET32_SHL_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op32[0]&0x80000000)^((op32[0]&0x40000000)<<1))!=0 )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET32_SHL_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( op32[0]&(0x80000000>>(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                                   l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET32_SHLD_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( (res32^op32[0])&0x80000000 ) l_EFLAGS|= OF_FLAG;
            else                              l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET32_SHLD_CF_EFLAGS:
        if ( count > 0 && count <= 32 )
          {
            if ( op32[0]&(1<<(32-count)) ) l_EFLAGS|= CF_FLAG;
            else                           l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET32_SHR_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( op32[0]&0x80000000 ) l_EFLAGS|= OF_FLAG;
            else                      l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET32_SHR_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( op32[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                          l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET32_SHRD_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( (res32^op32[0])&0x80000000 ) l_EFLAGS|= OF_FLAG;
            else                              l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET32_SHRD_CF_EFLAGS:
        if ( count > 0 && count <= 32 )
          {
            if ( op32[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                          l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET32_SB_OF_EFLAGS:
        if ((((op32[0]^op32[1])&(op32[0]^res32))&0x80000000)) l_EFLAGS|=OF_FLAG;
        else                                                l_EFLAGS&= ~OF_FLAG;
        break;
      case BC_SET32_SB_AF_EFLAGS:
        if ( (((op32[0]^(~op32[1]))^res32)&0x10) ) l_EFLAGS&= ~AF_FLAG;
        else                                       l_EFLAGS|= AF_FLAG;
        break;
      case BC_SET32_SB_CF_EFLAGS:
        if ( (((op32[0]&(~op32[1])) |
               ((op32[0]|(~op32[1]))&(~res32)))&0x80000000) )
          l_EFLAGS&= ~CF_FLAG;
        else l_EFLAGS|= CF_FLAG;
        break;
      case BC_SET32_SF_EFLAGS:
        if ( res32&0x80000000 ) l_EFLAGS|= SF_FLAG;
        else                    l_EFLAGS&= ~SF_FLAG;
        break;
      case BC_SET32_ZF_EFLAGS:
        if ( res32 == 0 ) l_EFLAGS|= ZF_FLAG;
        else              l_EFLAGS&= ~ZF_FLAG;
        break;
      case BC_SET32_PF_EFLAGS:
        if ( PFLAG[(uint8_t) (res32&0xff)] ) l_EFLAGS|= PF_FLAG;
        else                                 l_EFLAGS&= ~PF_FLAG;
        break;
      case BC_SET16_AD_OF_EFLAGS:
        if (((~(op16[0]^op16[1]))&(op16[0]^res16)&0x8000))
          l_EFLAGS|=OF_FLAG;
        else l_EFLAGS&= ~OF_FLAG;
        break;
      case BC_SET16_AD_AF_EFLAGS:
        if ( (((op16[0]^op16[1])^res16)&0x10) ) l_EFLAGS|= AF_FLAG;
        else                                    l_EFLAGS&= ~AF_FLAG;
        break;
      case BC_SET16_AD_CF_EFLAGS:
        if ( (((op16[0]&op16[1]) |
               ((op16[0]|op16[1])&(~res16)))&0x8000) )
          l_EFLAGS|= CF_FLAG;
        else l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET16_MUL_CFOF_EFLAGS:
        if ( ((res16&0x8000)!=0)^((res32&0x80000000)!=0) )
          l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET16_MUL1_CFOF_EFLAGS:
        if ( l_DX != 0 ) l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else             l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET16_IMUL1_CFOF_EFLAGS:
        if ( (((uint32_t) ((int32_t) ((int16_t) l_AX)))!=res32 ) )
          l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET16_NEG_CF_EFLAGS:
        if ( op16[1] == 0 ) l_EFLAGS&= ~CF_FLAG;
        else                l_EFLAGS|= CF_FLAG;
        break;
      case BC_SET16_RCL_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op16[0]&0x8000)!=0)^((op16[0]&0x4000)!=0) )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET16_RCL_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( ((op16[0]>>(16-count))&0x1) != 0 ) l_EFLAGS|= CF_FLAG;
            else                                    l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET16_RCR_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op16[0]&0x8000)!=0)^((l_EFLAGS&CF_FLAG)!=0) )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET16_RCR_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( ((op16[0]>>(count-1))&0x1) != 0 ) l_EFLAGS|= CF_FLAG;
            else                                   l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET16_ROL_CF_EFLAGS:
        if ( (res16&0x01) != 0 ) l_EFLAGS|= CF_FLAG;
        else                     l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET16_ROR_OF_EFLAGS:
        if ( count == 1 )
          {
            if (((op16[0]&0x8000)^((op16[0]&0x01)<<15))!=0) l_EFLAGS|= OF_FLAG;
            else                                            l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET16_ROR_CF_EFLAGS:
        if ( (res16&0x8000) != 0 ) l_EFLAGS|= CF_FLAG;
        else                       l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET16_SAR_CF_EFLAGS:
        if ( count >= 16 )
          {
            if ( op16[0]&0x8000 ) l_EFLAGS|= CF_FLAG;
            else                  l_EFLAGS&= ~CF_FLAG;
          }
        else if ( count > 0 )
          {
            if ( op16[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                          l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET16_SHIFT_SF_EFLAGS:
        if ( count != 0 )
          {
            if ( res16&0x8000 ) l_EFLAGS|= SF_FLAG;
            else                l_EFLAGS&= ~SF_FLAG;
          }
        break;
      case BC_SET16_SHIFT_ZF_EFLAGS:
        if ( count != 0 )
          {
            if ( res16 == 0 ) l_EFLAGS|= ZF_FLAG;
            else              l_EFLAGS&= ~ZF_FLAG;
          }
        break;
      case BC_SET16_SHIFT_PF_EFLAGS:
        if ( count != 0 )
          {
            if ( PFLAG[(uint8_t) (res16&0xff)] ) l_EFLAGS|= PF_FLAG;
            else                                 l_EFLAGS&= ~PF_FLAG;
          }
        break;
      case BC_SET16_SHL_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op16[0]&0x8000)^((op16[0]&0x4000)<<1))!=0 )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET16_SHL_CF_EFLAGS:
        if ( count > 16 ) l_EFLAGS&= ~CF_FLAG;
        else if ( count > 0 )
          {
            if ( op16[0]&(0x8000>>(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                               l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET16_SHLD_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( (res16^op16[0])&0x8000 ) l_EFLAGS|= OF_FLAG;
            else                          l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET16_SHLD_CF_EFLAGS:
        if ( count > 0 && count <= 16 )
          {
            if ( op16[0]&(1<<(16-count)) ) l_EFLAGS|= CF_FLAG;
            else                           l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET16_SHR_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( op16[0]&0x8000 ) l_EFLAGS|= OF_FLAG;
            else                  l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET16_SHR_CF_EFLAGS:
        if ( count > 16 ) l_EFLAGS&= ~CF_FLAG;
        else if ( count > 0 )
          {
            if ( op16[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                          l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET16_SHRD_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( (res16^op16[0])&0x8000 ) l_EFLAGS|= OF_FLAG;
            else                          l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET16_SHRD_CF_EFLAGS:
        if ( count > 0 && count <= 16 )
          {
            if ( op16[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                          l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET16_SB_OF_EFLAGS:
        if ((((op16[0]^op16[1])&(op16[0]^res16))&0x8000)) l_EFLAGS|= OF_FLAG;
        else                                              l_EFLAGS&= ~OF_FLAG;
        break;
      case BC_SET16_SB_AF_EFLAGS:
        if ( (((op16[0]^(~op16[1]))^res16)&0x10) ) l_EFLAGS&= ~AF_FLAG;
        else                                       l_EFLAGS|= AF_FLAG;
        break;
      case BC_SET16_SB_CF_EFLAGS:
        if ( (((op16[0]&(~op16[1])) | ((op16[0]|(~op16[1]))&(~res16)))&0x8000) )
          l_EFLAGS&= ~CF_FLAG;
        else l_EFLAGS|= CF_FLAG;
        break;
      case BC_SET16_SF_EFLAGS:
        if ( res16&0x8000 ) l_EFLAGS|= SF_FLAG;
        else                l_EFLAGS&= ~SF_FLAG;
        break;
      case BC_SET16_ZF_EFLAGS:
        if ( res16 == 0 ) l_EFLAGS|= ZF_FLAG;
        else              l_EFLAGS&= ~ZF_FLAG;
        break;
      case BC_SET16_PF_EFLAGS:
        if ( PFLAG[(uint8_t) (res16&0xff)] ) l_EFLAGS|= PF_FLAG;
        else                                 l_EFLAGS&= ~PF_FLAG;
        break;
      case BC_SET8_AD_OF_EFLAGS:
        if (((~(op8[0]^op8[1]))&(op8[0]^res8)&0x80))
          l_EFLAGS|=OF_FLAG;
        else l_EFLAGS&= ~OF_FLAG;
        break;
      case BC_SET8_AD_AF_EFLAGS:
        if ( (((op8[0]^op8[1])^res8)&0x10) ) l_EFLAGS|= AF_FLAG;
        else                                 l_EFLAGS&= ~AF_FLAG;
        break;
      case BC_SET8_AD_CF_EFLAGS:
        if ( (((op8[0]&op8[1]) | ((op8[0]|op8[1])&(~res8)))&0x80) )
          l_EFLAGS|= CF_FLAG;
        else l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET8_MUL1_CFOF_EFLAGS:
        if ( (l_AX&0xFF00) != 0 ) l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else                      l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET8_IMUL1_CFOF_EFLAGS:
        if ( (((uint16_t) ((int16_t) ((int8_t) (l_AX&0xFF))))!=res16 ) )
          l_EFLAGS|= (CF_FLAG|OF_FLAG);
        else l_EFLAGS&= ~(CF_FLAG|OF_FLAG);
        break;
      case BC_SET8_NEG_CF_EFLAGS:
        if ( op8[1] == 0 ) l_EFLAGS&= ~CF_FLAG;
        else               l_EFLAGS|= CF_FLAG;
        break;
      case BC_SET8_SAR_CF_EFLAGS:
        if ( count >= 8 )
          {
            if ( op8[0]&0x80 ) l_EFLAGS|= CF_FLAG;
            else               l_EFLAGS&= ~CF_FLAG;
          }
        else if ( count > 0 )
          {
            if ( op8[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                         l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET8_RCL_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op8[0]&0x80)!=0)^((op8[0]&0x40)!=0) )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET8_RCL_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( ((op8[0]>>(8-count))&0x1) != 0 ) l_EFLAGS|= CF_FLAG;
            else                                  l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET8_RCR_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op8[0]&0x80)!=0)^((l_EFLAGS&CF_FLAG)!=0) )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET8_RCR_CF_EFLAGS:
        if ( count > 0 )
          {
            if ( ((op8[0]>>(count-1))&0x1) != 0 ) l_EFLAGS|= CF_FLAG;
            else                                  l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET8_ROL_CF_EFLAGS:
        if ( (res8&0x01) != 0 ) l_EFLAGS|= CF_FLAG;
        else                    l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET8_ROR_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op8[0]&0x80)^((op8[0]&0x01)<<7))!=0 ) l_EFLAGS|= OF_FLAG;
            else                                         l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET8_ROR_CF_EFLAGS:
        if ( (res8&0x80) != 0 ) l_EFLAGS|= CF_FLAG;
        else                    l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_SET8_SHIFT_SF_EFLAGS:
        if ( count != 0 )
          {
            if ( res8&0x80 ) l_EFLAGS|= SF_FLAG;
            else             l_EFLAGS&= ~SF_FLAG;
          }
        break;
      case BC_SET8_SHIFT_ZF_EFLAGS:
        if ( count != 0 )
          {
            if ( res8 == 0 ) l_EFLAGS|= ZF_FLAG;
            else             l_EFLAGS&= ~ZF_FLAG;
          }
        break;
      case BC_SET8_SHIFT_PF_EFLAGS:
        if ( count != 0 )
          {
            if ( PFLAG[res8] ) l_EFLAGS|= PF_FLAG;
            else               l_EFLAGS&= ~PF_FLAG;
          }
        break;
      case BC_SET8_SHL_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( ((op8[0]&0x80)^((op8[0]&0x40)<<1))!=0 )
              l_EFLAGS|= OF_FLAG;
            else l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET8_SHL_CF_EFLAGS:
        if ( count > 8 ) l_EFLAGS&= ~CF_FLAG;
        else if ( count > 0 )
          {
            if ( op8[0]&(0x80>>(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                            l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET8_SHR_OF_EFLAGS:
        if ( count == 1 )
          {
            if ( op8[0]&0x80 ) l_EFLAGS|= OF_FLAG;
            else               l_EFLAGS&= ~OF_FLAG;
          }
        break;
      case BC_SET8_SHR_CF_EFLAGS:
        if ( count > 8 ) l_EFLAGS&= ~CF_FLAG;
        else if ( count > 0 )
          {
            if ( op8[0]&(1<<(count-1)) ) l_EFLAGS|= CF_FLAG;
            else                         l_EFLAGS&= ~CF_FLAG;
          }
        break;
      case BC_SET8_SB_OF_EFLAGS:
        if ((((op8[0]^op8[1])&(op8[0]^res8))&0x80)) l_EFLAGS|=OF_FLAG;
        else                                        l_EFLAGS&= ~OF_FLAG;
        break;
      case BC_SET8_SB_AF_EFLAGS:
        if ( (((op8[0]^(~op8[1]))^res8)&0x10) ) l_EFLAGS&= ~AF_FLAG;
        else                                    l_EFLAGS|= AF_FLAG;
        break;
      case BC_SET8_SB_CF_EFLAGS:
        if ( (((op8[0]&(~op8[1])) |
               ((op8[0]|(~op8[1]))&(~res8)))&0x80) )
          l_EFLAGS&= ~CF_FLAG;
        else l_EFLAGS|= CF_FLAG;
        break;
      case BC_SET8_SF_EFLAGS:
        if ( res8&0x80 ) l_EFLAGS|= SF_FLAG;
        else             l_EFLAGS&= ~SF_FLAG;
        break;
      case BC_SET8_ZF_EFLAGS:
        if ( res8 == 0 ) l_EFLAGS|= ZF_FLAG;
        else             l_EFLAGS&= ~ZF_FLAG;
        break;
      case BC_SET8_PF_EFLAGS:
        if ( PFLAG[res8] ) l_EFLAGS|= PF_FLAG;
        else               l_EFLAGS&= ~PF_FLAG;
        break;
      case BC_CLEAR_OF_CF_EFLAGS: l_EFLAGS&= ~(OF_FLAG|CF_FLAG); break;
      case BC_CLEAR_CF_EFLAGS: l_EFLAGS&= ~CF_FLAG; break;
      case BC_CLEAR_DF_EFLAGS: l_EFLAGS&= ~DF_FLAG; break;
      case BC_SET_CF_EFLAGS: l_EFLAGS|= CF_FLAG; break;
      case BC_SET_DF_EFLAGS: l_EFLAGS|= DF_FLAG; break;
        // --> CR0 flags
      case BC_CLEAR_TS_CR0:
        if ( check_seg_level0_novm ( jit ) )
          l_CR0&= ~CR0_TS;
        else { exception ( jit ); goto stop; }
        break;
        // --> Assignació valors immediats a operadors
      case BC_SET32_SIMM_RES:
        res32= (uint32_t) ((int32_t) ((int16_t) (p->v[++pos])));
        break;
      case BC_SET32_SIMM_OP0:
        op32[0]= (uint32_t) ((int32_t) ((int16_t) (p->v[++pos])));
        break;
      case BC_SET32_SIMM_OP1:
        op32[1]= (uint32_t) ((int32_t) ((int16_t) (p->v[++pos])));
        break;
      case BC_SET32_IMM32_RES:
        res32= ((uint32_t) p->v[pos+1]) | (((uint32_t) (p->v[pos+2]))<<16);
        pos+= 2;
        break;
      case BC_SET32_IMM32_OP0:
        op32[0]= ((uint32_t) p->v[pos+1]) | (((uint32_t) (p->v[pos+2]))<<16);
        pos+= 2;
        break;
      case BC_SET32_IMM32_OP1:
        op32[1]= ((uint32_t) p->v[pos+1]) | (((uint32_t) (p->v[pos+2]))<<16);
        pos+= 2;
        break;
      case BC_SET16_IMM_RES: res16= (uint16_t) (p->v[pos+1]); ++pos; break;
      case BC_SET16_IMM_OP0: op16[0]= (uint16_t) (p->v[pos+1]); ++pos; break;
      case BC_SET16_IMM_OP1: op16[1]= (uint16_t) (p->v[pos+1]); ++pos; break;
      case BC_SET8_IMM_RES: res8= (uint8_t) (p->v[pos+1]); ++pos; break;
      case BC_SET8_IMM_OP0: op8[0]= (uint8_t) (p->v[pos+1]); ++pos; break;
      case BC_SET8_IMM_OP1: op8[1]= (uint8_t) (p->v[pos+1]); ++pos; break;
        // --> Assignació registre
      case BC_SET32_1_OP1: op32[1]= 1; break;
      case BC_SET32_OP0_OP1_0_OP0: op32[1]= op32[0]; op32[0]= 0; break;
      case BC_SET32_OP0_RES: res32= op32[0]; break;
      case BC_SET32_OP1_RES: res32= op32[1]; break;
      case BC_SET32_DR7_RES:
        if ( check_seg_level0 ( jit ) ) res32= l_DR7;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_DR7_OP0:
        if ( check_seg_level0 ( jit ) ) op32[0]= l_DR7;
        else                            { exception ( jit ); goto stop; }
        break;        
      case BC_SET32_DR7_OP1:
        if ( check_seg_level0 ( jit ) ) op32[1]= l_DR7;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_CR0_RES_NOCHECK: res32= l_CR0&0x0000FFFF; break;
      case BC_SET32_CR0_RES:
        if ( check_seg_level0 ( jit ) ) res32= l_CR0;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_CR0_OP0:
        if ( check_seg_level0 ( jit ) ) op32[0]= l_CR0;
        else                            { exception ( jit ); goto stop; }
        break;        
      case BC_SET32_CR0_OP1:
        if ( check_seg_level0 ( jit ) ) op32[1]= l_CR0;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_CR2_RES:
        if ( check_seg_level0 ( jit ) ) res32= l_CR2;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_CR2_OP0:
        if ( check_seg_level0 ( jit ) ) op32[0]= l_CR2;
        else                            { exception ( jit ); goto stop; }
        break;        
      case BC_SET32_CR2_OP1:
        if ( check_seg_level0 ( jit ) ) op32[1]= l_CR2;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_CR3_RES:
        if ( check_seg_level0 ( jit ) ) res32= l_CR3;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_CR3_OP0:
        if ( check_seg_level0 ( jit ) ) op32[0]= l_CR3;
        else                            { exception ( jit ); goto stop; }
        break;        
      case BC_SET32_CR3_OP1:
        if ( check_seg_level0 ( jit ) ) op32[1]= l_CR3;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_EAX_RES: res32= l_EAX; break;
      case BC_SET32_EAX_OP0: op32[0]= l_EAX; break;
      case BC_SET32_EAX_OP1: op32[1]= l_EAX; break;
      case BC_SET32_ECX_RES: res32= l_ECX; break;
      case BC_SET32_ECX_OP0: op32[0]= l_ECX; break;
      case BC_SET32_ECX_OP1: op32[1]= l_ECX; break;
      case BC_SET32_EDX_RES: res32= l_EDX; break;
      case BC_SET32_EDX_OP0: op32[0]= l_EDX; break;
      case BC_SET32_EDX_OP1: op32[1]= l_EDX; break;
      case BC_SET32_EBX_RES: res32= l_EBX; break;
      case BC_SET32_EBX_OP0: op32[0]= l_EBX; break;
      case BC_SET32_EBX_OP1: op32[1]= l_EBX; break;
      case BC_SET32_ESP_RES: res32= l_ESP; break;
      case BC_SET32_ESP_OP0: op32[0]= l_ESP; break;
      case BC_SET32_ESP_OP1: op32[1]= l_ESP; break;
      case BC_SET32_EBP_RES: res32= l_EBP; break;
      case BC_SET32_EBP_OP0: op32[0]= l_EBP; break;
      case BC_SET32_EBP_OP1: op32[1]= l_EBP; break;
      case BC_SET32_ESI_RES: res32= l_ESI; break;
      case BC_SET32_ESI_OP0: op32[0]= l_ESI; break;
      case BC_SET32_ESI_OP1: op32[1]= l_ESI; break;
      case BC_SET32_EDI_RES: res32= l_EDI; break;
      case BC_SET32_EDI_OP0: op32[0]= l_EDI; break;
      case BC_SET32_EDI_OP1: op32[1]= l_EDI; break;
      case BC_SET32_ES_RES: res32= (uint32_t) l_P_ES->v; break;
      case BC_SET32_ES_OP0: op32[0]= (uint32_t) l_P_ES->v; break;
      case BC_SET32_ES_OP1: op32[1]= (uint32_t) l_P_ES->v; break;
      case BC_SET32_CS_RES: res32= (uint32_t) l_P_CS->v; break;
      case BC_SET32_CS_OP0: op32[0]= (uint32_t) l_P_CS->v; break;
      case BC_SET32_CS_OP1: op32[1]= (uint32_t) l_P_CS->v; break;
      case BC_SET32_SS_RES: res32= (uint32_t) l_P_SS->v; break;
      case BC_SET32_SS_OP0: op32[0]= (uint32_t) l_P_SS->v; break;
      case BC_SET32_SS_OP1: op32[1]= (uint32_t) l_P_SS->v; break;
      case BC_SET32_DS_RES: res32= (uint32_t) l_P_DS->v; break;
      case BC_SET32_DS_OP0: op32[0]= (uint32_t) l_P_DS->v; break;
      case BC_SET32_DS_OP1: op32[1]= (uint32_t) l_P_DS->v; break;
      case BC_SET32_FS_RES: res32= (uint32_t) l_P_FS->v; break;
      case BC_SET32_FS_OP0: op32[0]= (uint32_t) l_P_FS->v; break;
      case BC_SET32_FS_OP1: op32[1]= (uint32_t) l_P_FS->v; break;
      case BC_SET32_GS_RES: res32= (uint32_t) l_P_GS->v; break;
      case BC_SET32_GS_OP0: op32[0]= (uint32_t) l_P_GS->v; break;
      case BC_SET32_GS_OP1: op32[1]= (uint32_t) l_P_GS->v; break;
      case BC_SET16_FPUCONTROL_RES: res16= l_FPU_CONTROL; break;
      case BC_SET16_FPUSTATUSTOP_RES: res16= l_FPU_STATUS_TOP; break;
      case BC_SET16_1_OP1: op16[1]= 1; break;
      case BC_SET16_OP0_OP1_0_OP0: op16[1]= op16[0]; op16[0]= 0; break;
      case BC_SET16_OP0_RES: res16= op16[0]; break;
      case BC_SET16_OP1_RES: res16= op16[1]; break;
      case BC_SET16_CR0_RES_NOCHECK: res16= (uint16_t) (l_CR0&0xFFFF); break;
      case BC_SET16_LDTR_RES: res16= l_LDTR.v; break;
      case BC_SET16_TR_RES: res16= l_TR.v; break;
      case BC_SET16_AX_RES: res16= l_AX; break;
      case BC_SET16_AX_OP0: op16[0]= l_AX; break;
      case BC_SET16_AX_OP1: op16[1]= l_AX; break;
      case BC_SET16_CX_RES: res16= l_CX; break;
      case BC_SET16_CX_OP0: op16[0]= l_CX; break;
      case BC_SET16_CX_OP1: op16[1]= l_CX; break;
      case BC_SET16_DX_RES: res16= l_DX; break;
      case BC_SET16_DX_OP0: op16[0]= l_DX; break;
      case BC_SET16_DX_OP1: op16[1]= l_DX; break;
      case BC_SET16_BX_RES: res16= l_BX; break;
      case BC_SET16_BX_OP0: op16[0]= l_BX; break;
      case BC_SET16_BX_OP1: op16[1]= l_BX; break;
      case BC_SET16_SP_RES: res16= l_SP; break;
      case BC_SET16_SP_OP0: op16[0]= l_SP; break;
      case BC_SET16_SP_OP1: op16[1]= l_SP; break;
      case BC_SET16_BP_RES: res16= l_BP; break;
      case BC_SET16_BP_OP0: op16[0]= l_BP; break;
      case BC_SET16_BP_OP1: op16[1]= l_BP; break;
      case BC_SET16_SI_RES: res16= l_SI; break;
      case BC_SET16_SI_OP0: op16[0]= l_SI; break;
      case BC_SET16_SI_OP1: op16[1]= l_SI; break;
      case BC_SET16_DI_RES: res16= l_DI; break;
      case BC_SET16_DI_OP0: op16[0]= l_DI; break;
      case BC_SET16_DI_OP1: op16[1]= l_DI; break;
      case BC_SET16_ES_RES: res16= l_P_ES->v; break;
      case BC_SET16_ES_OP0: op16[0]= l_P_ES->v; break;
      case BC_SET16_ES_OP1: op16[1]= l_P_ES->v; break;
      case BC_SET16_CS_RES: res16= l_P_CS->v; break;
      case BC_SET16_CS_OP0: op16[0]= l_P_CS->v; break;
      case BC_SET16_CS_OP1: op16[1]= l_P_CS->v; break;
      case BC_SET16_SS_RES: res16= l_P_SS->v; break;
      case BC_SET16_SS_OP0: op16[0]= l_P_SS->v; break;
      case BC_SET16_SS_OP1: op16[1]= l_P_SS->v; break;
      case BC_SET16_DS_RES: res16= l_P_DS->v; break;
      case BC_SET16_DS_OP0: op16[0]= l_P_DS->v; break;
      case BC_SET16_DS_OP1: op16[1]= l_P_DS->v; break;
      case BC_SET16_FS_RES: res16= l_P_FS->v; break;
      case BC_SET16_FS_OP0: op16[0]= l_P_FS->v; break;
      case BC_SET16_FS_OP1: op16[1]= l_P_FS->v; break;
      case BC_SET16_GS_RES: res16= l_P_GS->v; break;
      case BC_SET16_GS_OP0: op16[0]= l_P_GS->v; break;
      case BC_SET16_GS_OP1: op16[1]= l_P_GS->v; break;
      case BC_SET8_COND_RES: res8= cond ? 0x01 : 0x00; break;
      case BC_SET8_OP0_OP1_0_OP0: op8[1]= op8[0]; op8[0]= 0; break;
      case BC_SET8_1_OP1: op8[1]= 1; break;
      case BC_SET8_OP0_RES: res8= op8[0]; break;
      case BC_SET8_OP1_RES: res8= op8[1]; break;
      case BC_SET8_AL_RES: res8= l_AL; break;
      case BC_SET8_AL_OP0: op8[0]= l_AL; break;
      case BC_SET8_AL_OP1: op8[1]= l_AL; break;
      case BC_SET8_CL_RES: res8= l_CL; break;
      case BC_SET8_CL_OP0: op8[0]= l_CL; break;
      case BC_SET8_CL_OP1: op8[1]= l_CL; break;
      case BC_SET8_DL_RES: res8= l_DL; break;
      case BC_SET8_DL_OP0: op8[0]= l_DL; break;
      case BC_SET8_DL_OP1: op8[1]= l_DL; break;
      case BC_SET8_BL_RES: res8= l_BL; break;
      case BC_SET8_BL_OP0: op8[0]= l_BL; break;
      case BC_SET8_BL_OP1: op8[1]= l_BL; break;
      case BC_SET8_AH_RES: res8= l_AH; break;
      case BC_SET8_AH_OP0: op8[0]= l_AH; break;
      case BC_SET8_AH_OP1: op8[1]= l_AH; break;
      case BC_SET8_CH_RES: res8= l_CH; break;
      case BC_SET8_CH_OP0: op8[0]= l_CH; break;
      case BC_SET8_CH_OP1: op8[1]= l_CH; break;
      case BC_SET8_DH_RES: res8= l_DH; break;
      case BC_SET8_DH_OP0: op8[0]= l_DH; break;
      case BC_SET8_DH_OP1: op8[1]= l_DH; break;
      case BC_SET8_BH_RES: res8= l_BH; break;
      case BC_SET8_BH_OP0: op8[0]= l_BH; break;
      case BC_SET8_BH_OP1: op8[1]= l_BH; break;
        // --> Assignació res
      case BC_SET32_RES_FLOATU32: float_u32.u32= res32; break;
      case BC_SET32_RES_DOUBLEU64L: double_u64.u32.l= res32; break;
      case BC_SET32_RES_DOUBLEU64H: double_u64.u32.h= res32; break;
      case BC_SET32_RES_LDOUBLEU80L: ldouble_u80.u32.v0= res32; break;
      case BC_SET32_RES_LDOUBLEU80M: ldouble_u80.u32.v1= res32; break;
      case BC_SET32_SRES_LDOUBLE:
        ldouble= (long double) ((int32_t) res32);
        break;
      case BC_SET32_RES_SELECTOR: selector= (uint16_t) res32; break;
      case BC_SET32_RES_DR7:
        if ( check_seg_level0 ( jit ) )
          {
            if ( (res32&DR7_MASK) != 0 )
              {
                printf ( "[CAL IMPLEMENTAR] DR7 - VAL: %X!!!\n", res32 );
                exit ( EXIT_FAILURE );
              }
            l_DR7= (res32&DR7_MASK) | DR7_RESERVED_SET1;
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_CR0:
        if ( check_seg_level0 ( jit ) )
          {
            res32= (res32&CR0_MASK)&(~CR0_ET);
            // Comprovacions situaciones absurdes. Faltaran segurament!
            if ( ((CR0_PG&res32)==0 || (CR0_PE&res32)) )
              {
                l_CR0= res32;
                update_mem_callbacks ( jit );
                if ( res32&(CR0_CD|CR0_NW|CR0_AM|
                            CR0_WP|CR0_NE|CR0_TS|CR0_EM|CR0_MP) )
                  WW ( UDATA, "mov_cr_r32 Falten flags en CR0:"
                       " PG,CD,NW,AM,WP,NE,TS,EM,MP" );
              }
            else { EXCEPTION0 ( EXCP_GP ); exception ( jit ); goto stop; }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_CR2:
        if ( check_seg_level0 ( jit ) ) l_CR2= res32;
        else                            { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_CR3:
        if ( check_seg_level0 ( jit ) )
          {
            res32&= CR3_MASK;
            l_CR3= res32;
            if ( res32&(CR3_PCD|CR3_PWT) )
              WW ( UDATA, "mov_cr_r32 Falten flags en CR3:"
                   " PCD,PWT" );
            paging_32b_CR3_changed ( jit );
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_EAX: l_EAX= res32; break;
      case BC_SET32_RES_ECX: l_ECX= res32; break;
      case BC_SET32_RES_EDX: l_EDX= res32; break;
      case BC_SET32_RES_EBX: l_EBX= res32; break;
      case BC_SET32_RES_ESP: l_ESP= res32; break;
      case BC_SET32_RES_EBP: l_EBP= res32; break;
      case BC_SET32_RES_ESI: l_ESI= res32; break;
      case BC_SET32_RES_EDI: l_EDI= res32; break;
      case BC_SET32_RES_ES:
        if ( !set_sreg ( jit, (uint16_t) res32, l_P_ES ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_SS:
        if ( !set_sreg ( jit, (uint16_t) res32, l_P_SS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_DS:
        if ( !set_sreg ( jit, (uint16_t) res32, l_P_DS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_FS:
        if ( !set_sreg ( jit, (uint16_t) res32, l_P_FS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES_GS:
        if ( !set_sreg ( jit, (uint16_t) res32, l_P_GS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET32_RES8_RES: res32= (uint32_t) res8; break;
      case BC_SET32_RES8_RES_SIGNED:
        res32= (uint32_t) ((int32_t) ((int8_t) res8));
        break;
      case BC_SET32_RES16_RES: res32= (uint32_t) res16; break;
      case BC_SET32_RES16_RES_SIGNED:
        res32= (uint32_t) ((int32_t) ((int16_t) res16));
        break;
      case BC_SET32_RES64_RES: res32= (uint32_t) (res64&0xFFFFFFFF); break;
      case BC_SET32_RES64_EAX_EDX:
        l_EAX= (uint32_t) (res64&0xFFFFFFFF);
        l_EDX= (uint32_t) (res64>>32);
        break;
      case BC_SET16_RES_LDOUBLEU80H:
        ldouble_u80.u16.v4= res16;
        ldouble_u80.u16.v5= 0;
        ldouble_u80.u32.v3= 0;
        break;
      case BC_SET16_RES_SELECTOR: selector= res16; break;
      case BC_SET16_RES_CR0: // Sols amb LMSW
        if ( check_seg_level0_novm ( jit ) )
          {
            if ( l_CR0&CR0_PE ) res16|= (uint16_t) CR0_PE;
            l_CR0= (l_CR0&(~0xF)) | ((uint32_t) (res16&0xF));
            update_mem_callbacks ( jit );
            if ( res16&(CR0_TS|CR0_EM|CR0_MP) )
              WW ( UDATA, "lmsw Falten flags en CR0: TS,EM,MP" );
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_SET16_RES_AX: l_AX= res16; break;
      case BC_SET16_RES_CX: l_CX= res16; break;
      case BC_SET16_RES_DX: l_DX= res16; break;
      case BC_SET16_RES_BX: l_BX= res16; break;
      case BC_SET16_RES_SP: l_SP= res16; break;
      case BC_SET16_RES_BP: l_BP= res16; break;
      case BC_SET16_RES_SI: l_SI= res16; break;
      case BC_SET16_RES_DI: l_DI= res16; break;
      case BC_SET16_RES_ES:
        if ( !set_sreg ( jit, res16, l_P_ES ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET16_RES_SS:
        if ( !set_sreg ( jit, res16, l_P_SS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET16_RES_DS:
        if ( !set_sreg ( jit, res16, l_P_DS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET16_RES_FS:
        if ( !set_sreg ( jit, res16, l_P_FS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET16_RES_GS:
        if ( !set_sreg ( jit, res16, l_P_GS ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_SET16_RES8_RES: res16= (uint16_t) res8; break;
      case BC_SET16_RES8_RES_SIGNED:
        res16= (uint16_t) ((int16_t) ((int8_t) res8));
        break;
      case BC_SET16_RES32_RES: res16= (uint16_t) (res32&0xFFFF); break;
      case BC_SET16_RES32_AX_DX:
        l_AX= (uint16_t) (res32&0xFFFF);
        l_DX= (uint16_t) (res32>>16);
        break;
      case BC_SET8_RES_AL: l_AL= res8; break;
      case BC_SET8_RES_CL: l_CL= res8; break;
      case BC_SET8_RES_DL: l_DL= res8; break;
      case BC_SET8_RES_BL: l_BL= res8; break;
      case BC_SET8_RES_AH: l_AH= res8; break;
      case BC_SET8_RES_CH: l_CH= res8; break;
      case BC_SET8_RES_DH: l_DH= res8; break;
      case BC_SET8_RES_BH: l_BH= res8; break;
        // --> Assignació offset
      case BC_SET32_OFFSET_EAX: l_EAX= offset; break;
      case BC_SET32_OFFSET_ECX: l_ECX= offset; break;
      case BC_SET32_OFFSET_EDX: l_EDX= offset; break;
      case BC_SET32_OFFSET_EBX: l_EBX= offset; break;
      case BC_SET32_OFFSET_ESP: l_ESP= offset; break;
      case BC_SET32_OFFSET_EBP: l_EBP= offset; break;
      case BC_SET32_OFFSET_ESI: l_ESI= offset; break;
      case BC_SET32_OFFSET_EDI: l_EDI= offset; break;
      case BC_SET32_OFFSET_RES: res32= offset; break;
      case BC_SET16_OFFSET_AX: l_AX= (uint16_t) offset; break;
      case BC_SET16_OFFSET_CX: l_CX= (uint16_t) offset; break;
      case BC_SET16_OFFSET_DX: l_DX= (uint16_t) offset; break;
      case BC_SET16_OFFSET_BX: l_BX= (uint16_t) offset; break;
      case BC_SET16_OFFSET_SP: l_SP= (uint16_t) offset; break;
      case BC_SET16_OFFSET_BP: l_BP= (uint16_t) offset; break;
      case BC_SET16_OFFSET_SI: l_SI= (uint16_t) offset; break;
      case BC_SET16_OFFSET_DI: l_DI= (uint16_t) offset; break;
      case BC_SET16_OFFSET_RES: res16= (uint16_t) offset; break;
      case BC_SET16_SELECTOR_RES: res16= selector; break;
        // --> Lectura en operadors
      case BC_DS_READ32_RES:
        if ( jit->_mem_read32 ( jit, l_P_DS, offset, &res32, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ32_OP0:
        if ( jit->_mem_read32 ( jit, l_P_DS, offset, &op32[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ32_OP1:
        if ( jit->_mem_read32 ( jit, l_P_DS, offset, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ32_RES:
        if ( jit->_mem_read32 ( jit, l_P_SS, offset, &res32, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ32_OP0:
        if ( jit->_mem_read32 ( jit, l_P_SS, offset, &op32[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ32_OP1:
        if ( jit->_mem_read32 ( jit, l_P_SS, offset, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ32_RES:
        if ( jit->_mem_read32 ( jit, l_P_ES, offset, &res32, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ32_OP0:
        if ( jit->_mem_read32 ( jit, l_P_ES, offset, &op32[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ32_OP1:
        if ( jit->_mem_read32 ( jit, l_P_ES, offset, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ32_RES:
        if ( jit->_mem_read32 ( jit, l_P_CS, offset, &res32, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ32_OP0:
        if ( jit->_mem_read32 ( jit, l_P_CS, offset, &op32[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ32_OP1:
        if ( jit->_mem_read32 ( jit, l_P_CS, offset, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ32_RES:
        if ( jit->_mem_read32 ( jit, l_P_FS, offset, &res32, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ32_OP0:
        if ( jit->_mem_read32 ( jit, l_P_FS, offset, &op32[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ32_OP1:
        if ( jit->_mem_read32 ( jit, l_P_FS, offset, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ32_RES:
        if ( jit->_mem_read32 ( jit, l_P_GS, offset, &res32, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ32_OP0:
        if ( jit->_mem_read32 ( jit, l_P_GS, offset, &op32[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ32_OP1:
        if ( jit->_mem_read32 ( jit, l_P_GS, offset, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ16_RES:
        if ( jit->_mem_read16 ( jit, l_P_DS, offset, &res16, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ16_OP0:
        if ( jit->_mem_read16 ( jit, l_P_DS, offset, &op16[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ16_OP1:
        if ( jit->_mem_read16 ( jit, l_P_DS, offset, &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ16_RES:
        if ( jit->_mem_read16 ( jit, l_P_SS, offset, &res16, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ16_OP0:
        if ( jit->_mem_read16 ( jit, l_P_SS, offset, &op16[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ16_OP1:
        if ( jit->_mem_read16 ( jit, l_P_SS, offset, &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ16_RES:
        if ( jit->_mem_read16 ( jit, l_P_ES, offset, &res16, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ16_OP0:
        if ( jit->_mem_read16 ( jit, l_P_ES, offset, &op16[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ16_OP1:
        if ( jit->_mem_read16 ( jit, l_P_ES, offset, &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ16_RES:
        if ( jit->_mem_read16 ( jit, l_P_CS, offset, &res16, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ16_OP0:
        if ( jit->_mem_read16 ( jit, l_P_CS, offset, &op16[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ16_OP1:
        if ( jit->_mem_read16 ( jit, l_P_CS, offset, &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ16_RES:
        if ( jit->_mem_read16 ( jit, l_P_FS, offset, &res16, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ16_OP0:
        if ( jit->_mem_read16 ( jit, l_P_FS, offset, &op16[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ16_OP1:
        if ( jit->_mem_read16 ( jit, l_P_FS, offset, &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ16_RES:
        if ( jit->_mem_read16 ( jit, l_P_GS, offset, &res16, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ16_OP0:
        if ( jit->_mem_read16 ( jit, l_P_GS, offset, &op16[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ16_OP1:
        if ( jit->_mem_read16 ( jit, l_P_GS, offset, &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ8_RES:
        if ( jit->_mem_read8 ( jit, l_P_DS, offset, &res8, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ8_OP0:
        if ( jit->_mem_read8 ( jit, l_P_DS, offset, &op8[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ8_OP1:
        if ( jit->_mem_read8 ( jit, l_P_DS, offset, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ8_RES:
        if ( jit->_mem_read8 ( jit, l_P_SS, offset, &res8, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ8_OP0:
        if ( jit->_mem_read8 ( jit, l_P_SS, offset, &op8[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ8_OP1:
        if ( jit->_mem_read8 ( jit, l_P_SS, offset, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ8_RES:
        if ( jit->_mem_read8 ( jit, l_P_ES, offset, &res8, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ8_OP0:
        if ( jit->_mem_read8 ( jit, l_P_ES, offset, &op8[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ8_OP1:
        if ( jit->_mem_read8 ( jit, l_P_ES, offset, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ8_RES:
        if ( jit->_mem_read8 ( jit, l_P_CS, offset, &res8, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ8_OP0:
        if ( jit->_mem_read8 ( jit, l_P_CS, offset, &op8[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ8_OP1:
        if ( jit->_mem_read8 ( jit, l_P_CS, offset, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ8_RES:
        if ( jit->_mem_read8 ( jit, l_P_FS, offset, &res8, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ8_OP0:
        if ( jit->_mem_read8 ( jit, l_P_FS, offset, &op8[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_READ8_OP1:
        if ( jit->_mem_read8 ( jit, l_P_FS, offset, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ8_RES:
        if ( jit->_mem_read8 ( jit, l_P_GS, offset, &res8, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ8_OP0:
        if ( jit->_mem_read8 ( jit, l_P_GS, offset, &op8[0], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ8_OP1:
        if ( jit->_mem_read8 ( jit, l_P_GS, offset, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ_SELECTOR_OFFSET:
        if ( jit->_mem_read16 ( jit, l_P_DS, offset, &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read32 ( jit, l_P_DS, offset+2,
                                     &offset, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_READ_SELECTOR_OFFSET:
        if ( jit->_mem_read16 ( jit, l_P_SS, offset, &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read32 ( jit, l_P_SS, offset+2,
                                     &offset, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_READ_SELECTOR_OFFSET:
        if ( jit->_mem_read16 ( jit, l_P_ES, offset, &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read32 ( jit, l_P_ES, offset+2,
                                     &offset, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_READ_SELECTOR_OFFSET:
        if ( jit->_mem_read16 ( jit, l_P_CS, offset, &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read32 ( jit, l_P_CS, offset+2,
                                     &offset, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_READ_SELECTOR_OFFSET:
        if ( jit->_mem_read16 ( jit, l_P_GS, offset, &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read32 ( jit, l_P_GS, offset+2,
                                     &offset, true ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_READ_OFFSET_SELECTOR:
        if ( jit->_mem_read32 ( jit, l_P_DS, offset, &tmp32, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_DS, offset+4,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= tmp32;
        break;
      case BC_DS_READ_OFFSET16_SELECTOR:
        if ( jit->_mem_read16 ( jit, l_P_DS, offset, &tmp16, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_DS, offset+2,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= (uint32_t) tmp16;
        break;
      case BC_SS_READ_OFFSET_SELECTOR:
        if ( jit->_mem_read32 ( jit, l_P_SS, offset, &tmp32, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_SS, offset+4,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= tmp32;
        break;
      case BC_SS_READ_OFFSET16_SELECTOR:
        if ( jit->_mem_read16 ( jit, l_P_SS, offset, &tmp16, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_SS, offset+2,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= (uint32_t) tmp16;
        break;
      case BC_ES_READ_OFFSET_SELECTOR:
        if ( jit->_mem_read32 ( jit, l_P_ES, offset, &tmp32, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_ES, offset+4,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= tmp32;
        break;
      case BC_ES_READ_OFFSET16_SELECTOR:
        if ( jit->_mem_read16 ( jit, l_P_ES, offset, &tmp16, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_ES, offset+2,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= (uint32_t) tmp16;
        break;
      case BC_CS_READ_OFFSET_SELECTOR:
        if ( jit->_mem_read32 ( jit, l_P_CS, offset, &tmp32, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_CS, offset+4,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= tmp32;
        break;
      case BC_CS_READ_OFFSET16_SELECTOR:
        if ( jit->_mem_read16 ( jit, l_P_CS, offset, &tmp16, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_CS, offset+2,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= (uint32_t) tmp16;
        break;
      case BC_FS_READ_OFFSET_SELECTOR:
        if ( jit->_mem_read32 ( jit, l_P_FS, offset, &tmp32, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_FS, offset+4,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= tmp32;
        break;
      case BC_FS_READ_OFFSET16_SELECTOR:
        if ( jit->_mem_read16 ( jit, l_P_FS, offset, &tmp16, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_FS, offset+2,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= (uint32_t) tmp16;
        break;
      case BC_GS_READ_OFFSET_SELECTOR:
        if ( jit->_mem_read32 ( jit, l_P_GS, offset, &tmp32, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_GS, offset+4,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= tmp32;
        break;
      case BC_GS_READ_OFFSET16_SELECTOR:
        if ( jit->_mem_read16 ( jit, l_P_GS, offset, &tmp16, true ) != 0 )
          { exception ( jit ); goto stop; }
        else if ( jit->_mem_read16 ( jit, l_P_GS, offset+2,
                                     &selector, true ) != 0 )
          { exception ( jit ); goto stop; }
        else offset= (uint32_t) tmp16;
        break;
        // --> Escriu res
      case BC_DS_RES_WRITE32:
        if ( jit->_mem_write32 ( jit, l_P_DS, offset, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_RES_WRITE32:
        if ( jit->_mem_write32 ( jit, l_P_SS, offset, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_RES_WRITE32:
        if ( jit->_mem_write32 ( jit, l_P_ES, offset, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_RES_WRITE32:
        if ( jit->_mem_write32 ( jit, l_P_CS, offset, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_RES_WRITE32:
        if ( jit->_mem_write32 ( jit, l_P_FS, offset, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_RES_WRITE32:
        if ( jit->_mem_write32 ( jit, l_P_GS, offset, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_RES_WRITE16:
        if ( jit->_mem_write16 ( jit, l_P_DS, offset, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_RES_WRITE16:
        if ( jit->_mem_write16 ( jit, l_P_SS, offset, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_RES_WRITE16:
        if ( jit->_mem_write16 ( jit, l_P_ES, offset, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_RES_WRITE16:
        if ( jit->_mem_write16 ( jit, l_P_CS, offset, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_RES_WRITE16:
        if ( jit->_mem_write16 ( jit, l_P_FS, offset, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_RES_WRITE16:
        if ( jit->_mem_write16 ( jit, l_P_GS, offset, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_DS_RES_WRITE8:
        if ( jit->_mem_write8 ( jit, l_P_DS, offset, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_SS_RES_WRITE8:
        if ( jit->_mem_write8 ( jit, l_P_SS, offset, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_ES_RES_WRITE8:
        if ( jit->_mem_write8 ( jit, l_P_ES, offset, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_CS_RES_WRITE8:
        if ( jit->_mem_write8 ( jit, l_P_CS, offset, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_FS_RES_WRITE8:
        if ( jit->_mem_write8 ( jit, l_P_FS, offset, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
      case BC_GS_RES_WRITE8:
        if ( jit->_mem_write8 ( jit, l_P_GS, offset, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        break;
        // --> Lectura ports
      case BC_PORT_READ32:
        if ( io_check_permission ( jit, port, 4 ) )
          res32= jit->port_read32 ( jit->udata, port );
        else { exception ( jit ); goto stop; }
        break;
      case BC_PORT_READ16:
        if ( io_check_permission ( jit, port, 2 ) )
          res16= jit->port_read16 ( jit->udata, port );
        else { exception ( jit ); goto stop; }
        break;
      case BC_PORT_READ8:
        if ( io_check_permission ( jit, port, 1 ) )
          res8= jit->port_read8 ( jit->udata, port );
        else { exception ( jit ); goto stop; }
        break;
        // --> Escritura ports
      case BC_PORT_WRITE32:
        if ( io_check_permission ( jit, port, 4 ) )
          jit->port_write32 ( jit->udata, port, res32 );
        else { exception ( jit ); goto stop; }
        break;
      case BC_PORT_WRITE16:
        if ( io_check_permission ( jit, port, 2 ) )
          jit->port_write16 ( jit->udata, port, res16 );
        else { exception ( jit ); goto stop; }
        break;
      case BC_PORT_WRITE8:
        if ( io_check_permission ( jit, port, 1 ) )
          {
            jit->port_write8 ( jit->udata, port, res8 );
            if ( jit->_stop_after_port_write )
              { jit->_stop_after_port_write= false; goto stop; }
          }
        else { exception ( jit ); goto stop; }
        break;
        
        // INSTRUCCIONS (Poden parar algunes)
        // --> Control de fluxe
      case BC_JMP32_FAR:
        if ( jmp_far ( jit, selector, offset, true ) ) goto_eip ( jit );
        else                                           exception ( jit );
        goto stop;
        break;
      case BC_JMP16_FAR:
        if ( jmp_far ( jit, selector, offset, false ) ) goto_eip ( jit );
        else                                            exception ( jit );
        goto stop;
        break;
      case BC_JMP32_NEAR_REL:
        offset= l_EIP +
          ((uint32_t) (p->v[pos+2])) +
          (uint32_t) ((int32_t) ((int16_t) p->v[pos+1]));
        if ( jmp_near ( jit, offset ) ) goto_eip ( jit );
        else                            exception ( jit );
        goto stop;
        break;
      case BC_JMP16_NEAR_REL:
        offset= (l_EIP +
                 ((uint32_t) (p->v[pos+2])) +
                 (uint32_t) ((int32_t) ((int16_t) p->v[pos+1])))&0xFFFF;
        if ( jmp_near ( jit, offset ) ) goto_eip ( jit );
        else                            exception ( jit );
        goto stop;
        break;
      case BC_JMP32_NEAR_REL32:
        offset= l_EIP +
          ((uint32_t) (p->v[pos+3])) +
          (((uint32_t) p->v[pos+1]) | (((uint32_t) p->v[pos+2])<<16));
        if ( jmp_near ( jit, offset ) ) goto_eip ( jit );
        else                            exception ( jit );
        goto stop;
        break;
      case BC_JMP32_NEAR_RES32:
        if ( jmp_near ( jit, res32 ) ) goto_eip ( jit );
        else                           exception ( jit );
        goto stop;
        break;
      case BC_JMP16_NEAR_RES16:
        if ( jmp_near ( jit, (uint32_t) res16 ) ) goto_eip ( jit );
        else                                      exception ( jit );
        goto stop;
        break;
      case BC_CALL32_FAR:
        if ( call_far ( jit, selector, offset, true,
                        l_EIP+((uint32_t) p->v[pos+1]) ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_CALL16_FAR:
        if ( call_far ( jit, selector, offset, false,
                        l_EIP+((uint32_t) p->v[pos+1]) ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_CALL32_NEAR_REL:
        next_eip= l_EIP + (uint32_t) (p->v[pos+2]);
        offset= next_eip +
          (uint32_t) ((int32_t) ((int16_t) p->v[pos+1]));
        if ( jmp_near ( jit, offset ) && push32 ( jit, next_eip ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_CALL16_NEAR_REL:
        next_eip= l_EIP + (uint32_t) (p->v[pos+2]);
        offset= next_eip +
          (uint32_t) ((int32_t) ((int16_t) p->v[pos+1]));
        if ( jmp_near ( jit, offset&0xFFFF ) &&
             push16 ( jit, (uint16_t) (next_eip&0xFFFF) ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_CALL32_NEAR_REL32:
        next_eip= l_EIP + (uint32_t) (p->v[pos+3]);
        offset= next_eip +
          (((uint32_t) p->v[pos+1]) | (((uint32_t) p->v[pos+2])<<16));
        if ( jmp_near ( jit, offset ) && push32 ( jit, next_eip ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_CALL32_NEAR_RES32:
        next_eip= l_EIP + (uint32_t) (p->v[pos+1]);
        if ( jmp_near ( jit, res32 ) && push32 ( jit, next_eip ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_CALL16_NEAR_RES16:
        next_eip= l_EIP + (uint32_t) (p->v[pos+1]);
        if ( jmp_near ( jit, (uint32_t) res16 ) &&
             push16 ( jit, (uint16_t) (next_eip&0xFFFF) ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_BRANCH32:
        if ( cond )
          {
            offset= l_EIP +
              (uint32_t) (p->v[pos+2]) +
              (uint32_t) ((int32_t) ((int16_t) p->v[pos+1]));
            if ( jmp_near ( jit, offset ) ) goto_eip ( jit );
            else                            exception ( jit );
          }
        else { l_EIP+= (uint32_t) (p->v[pos+2]); goto_eip ( jit ); }
        goto stop;
        break;
      case BC_BRANCH32_IMM32:
        if ( cond )
          {
            offset= l_EIP +
              (uint32_t) (p->v[pos+3]) +
              (((uint32_t) p->v[pos+1] ) | (((uint32_t) p->v[pos+2])<<16));
            if ( jmp_near ( jit, offset ) ) goto_eip ( jit );
            else                            exception ( jit );
          }
        else { l_EIP+= (uint32_t) (p->v[pos+3]); goto_eip ( jit ); }
        goto stop;
        break;
      case BC_BRANCH16:
        if ( cond )
          {
            offset= (l_EIP +
                     (uint32_t) (p->v[pos+2]) +
                     (uint32_t) ((int32_t) ((int16_t) p->v[pos+1])))&0xFFFF;
            if ( jmp_near ( jit, offset ) ) goto_eip ( jit );
            else                            exception ( jit );
          }
        else { l_EIP+= (uint32_t) (p->v[pos+2]); goto_eip ( jit ); }
        goto stop;
        break;
      case BC_IRET32:
        if ( iret ( jit, true,
                    l_EIP+(uint32_t)p->v[pos+1] ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_IRET16:
        if ( iret ( jit, false,
                    l_EIP+(uint32_t)p->v[pos+1] ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
        // --> Aritmètiques
      case BC_ADC32: res32= op32[0] + op32[1] + ((l_EFLAGS&CF_FLAG)!=0); break;
      case BC_ADC16: res16= op16[0] + op16[1] + ((l_EFLAGS&CF_FLAG)!=0); break;
      case BC_ADC8: res8= op8[0] + op8[1] + ((l_EFLAGS&CF_FLAG)!=0); break;
      case BC_ADD32: res32= op32[0] + op32[1]; break;
      case BC_ADD16: res16= op16[0] + op16[1]; break;
      case BC_ADD8: res8= op8[0] + op8[1]; break;
      case BC_DIV32_EDX_EAX:
        if ( op32[0] == 0 )
          { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
        else
          {
            tmp64= (((uint64_t) l_EDX)<<32) | ((uint64_t) l_EAX);
            res64= tmp64/((uint64_t) op32[0]);
            if ( res64 > 0xFFFFFFFF )
              { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
            else
              {
                l_EAX= (uint32_t) res64;
                l_EDX= (uint32_t) (tmp64%((uint64_t) op32[0]));
              }
          }
        break;
      case BC_DIV16_DX_AX:
        if ( op16[0] == 0 )
          { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
        else
          {
            tmp32= (((uint32_t) l_DX)<<16) | ((uint32_t) l_AX);
            res32= tmp32/((uint32_t) op16[0]);
            if ( res32 > 0xFFFF )
              { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
            else
              {
                l_AX= (uint16_t) res32;
                l_DX= (uint16_t) (tmp32%((uint32_t) op16[0]));
              }
          }
        break;
      case BC_DIV8_AH_AL:
        if ( op8[0] == 0 )
          { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
        else
          {
            tmp16= (uint16_t) l_AX;
            res16= tmp16/((uint16_t) op8[0]);
            if ( res16 > 0xFF )
              { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
            else
              {
                l_AL= (uint8_t) res16;
                l_AH= (uint8_t) (tmp16%((uint16_t) op8[0]));
              }
          }
        break;
      case BC_IDIV32_EDX_EAX:
        if ( op32[0] == 0 )
          { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
        else
          {
            tmp64= (((uint64_t) l_EDX)<<32) | ((uint64_t) l_EAX);
            res64= ((int64_t) tmp64)/((int64_t) ((int32_t) op32[0]));
            if ( ((int64_t) res64) > ((int64_t) ((int32_t) 0x7fffffff)) ||
                 ((int64_t) res64) < ((int64_t) ((int32_t) 0x80000000)) )
              { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
            else
              {
                l_EAX= (uint32_t) res64;
                l_EDX= (uint32_t)
                  (((int64_t) tmp64)%((int64_t) ((int32_t) op32[0])));
              }
          }
        break;
      case BC_IDIV16_DX_AX:
        if ( op16[0] == 0 )
          { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
        else
          {
            tmp32= (((uint32_t) l_DX)<<16) | ((uint32_t) l_AX);
            res32= ((int32_t) tmp32)/((int32_t) ((int16_t) op16[0]));
            if ( ((int32_t) res32) > ((int32_t) ((int16_t) 0x7fff)) ||
                 ((int32_t) res32) < ((int32_t) ((int16_t) 0x8000)) )
              { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
            else
              {
                l_AX= (uint16_t) res32;
                l_DX= (uint16_t)
                  (((int32_t) tmp32)%((int32_t) ((int16_t) op16[0])));
              }
          }
        break;
      case BC_IDIV8_AH_AL:
        if ( op8[0] == 0 )
          { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
        else
          {
            tmp16= (uint16_t) ((int16_t) ((int8_t) l_AX));
            res16= ((int16_t) tmp16)/((int16_t) ((int8_t) op8[0]));
            if ( ((int16_t) res16) > ((int16_t) ((int8_t) 0x7f)) ||
                 ((int16_t) res16) < ((int16_t) ((int8_t) 0x80)) )
              { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
            else
              {
                l_AL= (uint8_t) res16;
                l_AH= (uint8_t)
                  (((int16_t) tmp16)%((int16_t) ((int8_t) op8[0])));
              }
          }
        break;
      case BC_IMUL32:
        res64= (uint64_t) (((int64_t) ((int32_t) op32[0])) *
                           ((int64_t) ((int32_t) op32[1])));
        break;
      case BC_IMUL16:
        res32= (uint32_t) (((int32_t) ((int16_t) op16[0])) *
                           ((int32_t) ((int16_t) op16[1])));
        break;
      case BC_IMUL8:
        res16= (uint16_t) (((int16_t) ((int8_t) op8[0])) *
                           ((int16_t) ((int8_t) op8[1])));
        break;
      case BC_MUL32:
        res64= ((uint64_t) op32[0]) * ((uint64_t) op32[1]);
        break;
      case BC_MUL16:
        res32= ((uint32_t) op16[0]) * ((uint32_t) op16[1]);
        break;
      case BC_MUL8:
        res16= ((uint16_t) op8[0]) * ((uint16_t) op8[1]);
        break;
      case BC_SBB32:
        res32= op32[0] + ((~op32[1])&0xFFFFFFFF) + ((l_EFLAGS&CF_FLAG)==0);
        break;
      case BC_SBB16:
        res16= op16[0] + ((~op16[1])&0xFFFF) + ((l_EFLAGS&CF_FLAG)==0);
        break;
      case BC_SBB8:
        res8= op8[0] + ((~op8[1])&0xFF) + ((l_EFLAGS&CF_FLAG)==0);
        break;
      case BC_SGDT32: res16= l_GDTR.lastb; res32= l_GDTR.addr; break;
      case BC_SIDT32: res16= l_IDTR.lastb; res32= l_IDTR.addr; break;
      case BC_SIDT16: res16= l_IDTR.lastb; res32= l_IDTR.addr&0x00FFFFFF; break;
      case BC_SUB32:
        res32= (uint32_t) ((int32_t) op32[0] - (int32_t) op32[1]);
        break;
      case BC_SUB16:
        res16= (uint16_t) ((int16_t) op16[0] - (int16_t) op16[1]);
        break;
      case BC_SUB8:
        res8= (uint8_t) ((int8_t) op8[0] - (int8_t) op8[1]);
        break;
        // --> Llògiques
      case BC_AND32: res32= op32[0] & op32[1]; break;
      case BC_AND16: res16= op16[0] & op16[1]; break;
      case BC_AND8: res8= op8[0] & op8[1]; break;
      case BC_NOT32: res32= ~(op32[0]); break;
      case BC_NOT16: res16= ~(op16[0]); break;
      case BC_NOT8: res8= ~(op8[0]); break;
      case BC_OR32: res32= op32[0] | op32[1]; break;
      case BC_OR16: res16= op16[0] | op16[1]; break;
      case BC_OR8: res8= op8[0] | op8[1]; break;
      case BC_RCL32:
        tmp32= ((l_EFLAGS&CF_FLAG)!=0);
        if ( count > 0 )
          {
            if ( count == 1 ) res32= (op32[0]<<1) | tmp32;
            else res32=
                   (op32[0]<<count) |
                   (tmp32<<(count-1)) |
                   (op32[0]>>(33-count));
          }
        else res32= op32[0];
        break;
      case BC_RCL16:
        tmp16= ((l_EFLAGS&CF_FLAG)!=0);
        count%= 17;
        if ( count > 0 )
          {
            if ( count == 16 ) res16= (tmp16<<15) | (op16[0]>>1);
            else if ( count == 1 ) res16= (op16[0]<<1) | tmp16;
            else res16=
                   (op16[0]<<count) |
                   (tmp16<<(count-1)) |
                   (op16[0]>>(17-count));
          }
        else res16= op16[0];
        break;
      case BC_RCL8:
        tmp8= ((l_EFLAGS&CF_FLAG)!=0);
        count%= 9;
        if ( count > 0 )
          {
            if ( count == 8 ) res8= (tmp8<<7) | (op8[0]>>1);
            else if ( count == 1 ) res8= (op8[0]<<1) | tmp8;
            else res8=
                   (op8[0]<<count) |
                   (tmp8<<(count-1)) |
                   (op8[0]>>(9-count));
          }
        else res8= op8[0];
        break;
      case BC_RCR32:
        tmp32= ((l_EFLAGS&CF_FLAG)!=0);
        if ( count > 0 )
          {
            if ( count == 1 ) res32= (op32[0]>>1) | (tmp32<<31);
            else res32=
                   (op32[0]>>count) |
                   (tmp32<<(32-count)) |
                   (op32[0]<<(33-count));
          }
        else res32= op32[0];
        break;
      case BC_RCR16:
        tmp16= ((l_EFLAGS&CF_FLAG)!=0);
        count%= 17;
        if ( count > 0 )
          {
            if ( count == 16 ) res16= tmp16 | (op16[0]<<1);
            else if ( count == 1 ) res16= (op16[0]>>1) | (tmp16<<15);
            else res16=
                   (op16[0]>>count) |
                   (tmp16<<(16-count)) |
                   (op16[0]<<(17-count));
          }
        else res16= op16[0];
        break;
      case BC_RCR8:
        tmp8= ((l_EFLAGS&CF_FLAG)!=0);
        count%= 9;
        if ( count > 0 )
          {
            if ( count == 8 ) res8= tmp8 | (op8[0]<<1);
            else if ( count == 1 ) res8= (op8[0]>>1) | (tmp8<<7);
            else res8=
                   (op8[0]>>count) |
                   (tmp8<<(8-count)) |
                   (op8[0]<<(9-count));
          }
        else res8= op8[0];
        break;
      case BC_ROL32:
        count%= 32;
        if ( count > 0 ) res32= (op32[0]<<count) | (op32[0]>>(32-count));
        else res32= op32[0];
        break;
      case BC_ROL16:
        count%= 16;
        if ( count > 0 ) res16= (op16[0]<<count) | (op16[0]>>(16-count));
        else res16= op16[0];
        break;
      case BC_ROL8:
        count%= 8;
        if ( count > 0 ) res8= (op8[0]<<count) | (op8[0]>>(8-count));
        else res8= op8[0];
        break;
      case BC_ROR32:
        if ( count > 0 ) res32= (op32[0]<<(32-count)) | (op32[0]>>count);
        else res32= op32[0];
        break;
      case BC_ROR16:
        count%= 16;
        if ( count > 0 ) res16= (op16[0]<<(16-count)) | (op16[0]>>count);
        else res16= op16[0];
        break;
      case BC_ROR8:
        count%= 8;
        if ( count > 0 ) res8= (op8[0]<<(8-count)) | (op8[0]>>count);
        else res8= op8[0];
        break;
      case BC_SAR32:
        if ( count == 32  )
          { res32= (op32[0]&0x80000000) ? 0xFFFFFFFF : 0; }
        else if ( count > 0 )
          { res32= (uint32_t) (((int32_t) op32[0])>>count); }
        else res32= op32[0];
        break;
      case BC_SAR16:
        if ( count >= 16  )
          { res16= (op16[0]&0x8000) ? 0xFFFF : 0; }
        else if ( count > 0 )
          { res16= (uint16_t) (((int16_t) op16[0])>>count); }
        else res16= op16[0];
        break;
      case BC_SAR8:
        if ( count >= 8  )
          { res8= (op8[0]&0x80) ? 0xFF : 0; }
        else if ( count > 0 )
          { res8= (uint8_t) (((int8_t) op8[0])>>count); }
        else res8= op8[0];
        break;
      case BC_SHL32:
        if ( count > 0 ) { res32= ((count==32) ? 0 : (op32[0]<<count)); }
        else res32= op32[0];
        break;
      case BC_SHL16:
        if ( count > 0 ) { res16= ((count>=16) ? 0 : (op16[0]<<count)); }
        else res16= op16[0];
        break;
      case BC_SHL8:
        if ( count > 0 ) { res8= ((count>=8) ? 0 : (op8[0]<<count)); }
        else res8= op8[0];
        break;
      case BC_SHLD32:
        if ( count > 0 && count <= 32 )
          {
            res32= (count==32) ?
              op32[1] :
              ((op32[0]<<count) | (op32[1]>>(32-count)));
          }
        else res32= op32[0];
        break;
      case BC_SHLD16:
        if ( count > 0 && count <= 16 )
          {
            res16= (count==16) ?
              op16[1] :
              ((op16[0]<<count) | (op16[1]>>(16-count)));
          }
        else res16= op16[0];
        break;
      case BC_SHR32:
        if ( count > 0 ) { res32= ((count==32) ? 0 : (op32[0]>>count)); }
        else res32= op32[0];
        break;
      case BC_SHR16:
        if ( count > 0 ) { res16= ((count>=16) ? 0 : (op16[0]>>count)); }
        else res16= op16[0];
        break;
      case BC_SHR8:
        if ( count > 0 ) { res8= ((count>=8) ? 0 : (op8[0]>>count)); }
        else res8= op8[0];
        break;
      case BC_SHRD32:
        if ( count > 0 && count <= 32 )
          {
            res32= (count==32) ?
              op32[1] :
              ((op32[0]>>count) | (op32[1]<<(32-count)));
          }
        else res32= op32[0];
        break;
      case BC_SHRD16:
        if ( count > 0 && count <= 16 )
          {
            res16= (count==16) ?
              op16[1] :
              ((op16[0]>>count) | (op16[1]<<(16-count)));
          }
        else res16= op16[0];
        break;
      case BC_XOR32: res32= op32[0] ^ op32[1]; break;
      case BC_XOR16: res16= op16[0] ^ op16[1]; break;
      case BC_XOR8: res8= op8[0] ^ op8[1]; break;
        // --> Altres
      case BC_AAD:
        l_AL= res8= (uint8_t) ((((uint16_t) l_AH)*((uint16_t) op8[0])
                                + ((uint16_t) l_AL))&0xFF);
        l_AH= 0;
        break;
      case BC_AAM:
        if ( op8[0] != 0 )
          {
            tmp8= l_AL;
            l_AH= tmp8/op8[0];
            l_AL= res8= tmp8%op8[0];
          }
        else { EXCEPTION ( EXCP_DE ); exception ( jit ); goto stop; }
        break;
      case BC_BSF32:
        if ( op32[1] == 0 ) { l_EFLAGS|= ZF_FLAG; }
        else
          {
            res32= BSF_TABLE[op32[1]&0xFF];
            if ( res32 == 8 )
              {
                res32+= BSF_TABLE[(op32[1]>>8)&0xFF];
                if ( res32 == 16 )
                  {
                    res32+= BSF_TABLE[(op32[1]>>16)&0xFF];
                    if ( res32 == 24 ) res32+= BSF_TABLE[op32[1]>>24];
                  }
              }
            l_EFLAGS&= ~ZF_FLAG;
          }
        break;
      case BC_BSF16:
        if ( op16[1] == 0 ) { l_EFLAGS|= ZF_FLAG; }
        else
          {
            res16= BSF_TABLE[op16[1]&0xFF];
            if ( res16 == 8 ) res16+= BSF_TABLE[op16[1]>>8];
            l_EFLAGS&= ~ZF_FLAG;
          }
        break;
      case BC_BSR32:
        if ( op32[1] == 0 ) { l_EFLAGS|= ZF_FLAG; }
        else
          {
            res32= 31 - BSR_TABLE[op32[1]>>24];
            if ( res32 == 23 )
              {
                res32-= BSR_TABLE[(op32[1]>>16)&0xFF];
                if ( res32 == 15 )
                  {
                    res32-= BSR_TABLE[(op32[1]>>8)&0xFF];
                    if ( res32 == 7 ) res32-= BSR_TABLE[op32[1]&0xFF];
                  }
              }
            l_EFLAGS&= ~ZF_FLAG;
          }
        break;
      case BC_BSR16:
        if ( op16[1] == 0 ) { l_EFLAGS|= ZF_FLAG; }
        else
          {
            res16= 15 - BSR_TABLE[op16[1]>>8];
            if ( res16 == 7 ) res16-= BSR_TABLE[op16[1]&0xFF];
            l_EFLAGS&= ~ZF_FLAG;
          }
        break;
      case BC_BSWAP:
        res32=
          (op32[0]<<24) |
          ((op32[0]<<8)&0x00FF0000) |
          ((op32[0]>>8)&0x0000FF00) |
          (op32[0]>>24)
          ;
        break;
      case BC_BT32:
        if ( op32[0]&(1<<(op32[1]%32)) ) l_EFLAGS|= CF_FLAG;
        else                             l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_BT16:
        if ( op16[0]&(1<<(op16[1]%32)) ) l_EFLAGS|= CF_FLAG;
        else                             l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_BTC32:
        if ( op32[0]&(1<<(op32[1]%32)) ) l_EFLAGS|= CF_FLAG;
        else                             l_EFLAGS&= ~CF_FLAG;
        res32= op32[0]^(1<<(op32[1]%32));
        break;
      case BC_BTR32:
        if ( op32[0]&(1<<(op32[1]%32)) ) l_EFLAGS|= CF_FLAG;
        else                             l_EFLAGS&= ~CF_FLAG;
        res32= op32[0]&(~(1<<(op32[1]%32)));
        break;
      case BC_BTR16:
        if ( op16[0]&(1<<(op16[1]%16)) ) l_EFLAGS|= CF_FLAG;
        else                             l_EFLAGS&= ~CF_FLAG;
        res16= op16[0]&(~(1<<(op16[1]%16)));
        break;
      case BC_BTS32:
        if ( op32[0]&(1<<(op32[1]%32)) ) l_EFLAGS|= CF_FLAG;
        else                             l_EFLAGS&= ~CF_FLAG;
        res32= op32[0]|(1<<(op32[1]%32));
        break;
      case BC_BTS16:
        if ( op16[0]&(1<<(op16[1]%16)) ) l_EFLAGS|= CF_FLAG;
        else                             l_EFLAGS&= ~CF_FLAG;
        res16= op16[0]|(1<<(op16[1]%16));
        break;
      case BC_CDQ: l_EDX= (l_EAX&0x80000000)!=0 ? 0xFFFFFFFF : 0x00000000;break;
      case BC_CLI: if ( !cli ( jit ) ) { exception ( jit ); goto stop; } break;
      case BC_CMC:
        if ( (l_EFLAGS&CF_FLAG) != 0 ) l_EFLAGS&= ~CF_FLAG;
        else                           l_EFLAGS|= CF_FLAG;
        break;
      case BC_CPUID: IA32_cpu_cpuid ( l_cpu ); break;
      case BC_CWD: l_DX= (l_AX&0x8000)!=0 ? 0xFFFF : 0x0000; break;
      case BC_CWDE: l_EAX= (uint32_t) ((int32_t) ((int16_t) l_AX)); break;
      case BC_DAA:
        tmp8= l_AL;
        tmp_bool= ((l_EFLAGS&CF_FLAG)!=0);
        if ( (l_AL&0xF) > 9 || (l_EFLAGS&AF_FLAG)!=0 )
          {
            res8= l_AL+6;
            if ( res8 < l_AL ) l_EFLAGS|= CF_FLAG;
            l_AL= res8;
            l_EFLAGS|= AF_FLAG;
          }
        else l_EFLAGS&= ~AF_FLAG;
        if ( tmp8 > 0x99 || tmp_bool )
          {
            l_AL+= 0x60;
            l_EFLAGS|= CF_FLAG;
          }
        else l_EFLAGS&= ~CF_FLAG;
        res8= l_AL;
        break;
      case BC_DAS:
        tmp8= l_AL;
        tmp_bool= ((l_EFLAGS&CF_FLAG)!=0);
        if ( (l_AL&0xF) > 9 || (l_EFLAGS&AF_FLAG)!=0 )
          {
            res8= l_AL-6;
            if ( res8 > l_AL ) l_EFLAGS|= CF_FLAG;
            l_AL= res8;
            l_EFLAGS|= AF_FLAG;
          }
        else l_EFLAGS&= ~AF_FLAG;
        if ( tmp8 > 0x99 || tmp_bool )
          {
            l_AL-= 0x60;
            l_EFLAGS|= CF_FLAG;
          }
        else l_EFLAGS&= ~CF_FLAG;
        res8= l_AL;
        break;
      case BC_ENTER16:
        if ( enter16 ( jit, p->v[pos+1], p->v[pos+2] ) ) pos+= 2;
        else { exception ( jit ); goto stop; }
        break;
      case BC_ENTER32:
        if ( enter32 ( jit, p->v[pos+1], p->v[pos+2] ) ) pos+= 2;
        else { exception ( jit ); goto stop; }
        break;
      case BC_HALT:
        // Bucle infinit. Es basa en que la EIP s'ha actualitzat abans.
        if ( check_seg_level0 ( jit ) ) jit->_current_pos= pos;
        else { l_EIP-= p->v[pos+1]; exception ( jit ); }
        goto stop;
        break;
      case BC_INT32: // NOTA!!! Per a què la distinció 32 i 16??????
        if ( interruption ( jit,
                            l_EIP+(uint32_t)p->v[pos+1],
                            INTERRUPTION_TYPE_IMM,
                            p->v[pos+2], 0, false ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_INT16:
        if ( interruption ( jit,
                            l_EIP+(uint32_t)p->v[pos+1],
                            INTERRUPTION_TYPE_IMM,
                            p->v[pos+2], 0, false ) )
          goto_eip ( jit );
        else exception ( jit );
        goto stop;
        break;
      case BC_INTO32:
        if ( (l_EFLAGS&OF_FLAG) != 0 )
          {
            if ( interruption ( jit,
                                l_EIP+(uint32_t)p->v[pos+1],
                                INTERRUPTION_TYPE_INTO,
                                4, 0, false ) )
              goto_eip ( jit );
            else exception ( jit );
          }
        else { l_EIP+= (uint32_t)p->v[pos+1]; goto_eip ( jit ); }
        goto stop;
        break;
      case BC_INTO16:
        if ( (l_EFLAGS&OF_FLAG) != 0 )
          {
            if ( interruption ( jit,
                                l_EIP+(uint32_t)p->v[pos+1],
                                INTERRUPTION_TYPE_INTO,
                                4, 0, false ) )
              goto_eip ( jit );
            else exception ( jit );
          }
        else { l_EIP+= (uint32_t)p->v[pos+1]; goto_eip ( jit ); }
        goto stop;
        break;
      case BC_INVLPG32:
      case BC_INVLPG16: // Ara són iguals
        if ( check_seg_level0_novm ( jit ) ) {} // NO FA RES!!!
        else { exception ( jit ); goto stop; }
        break;
      case BC_LAHF:
        l_AH=
          (((l_EFLAGS&SF_FLAG)!=0) ? 0x80 : 0x00) |
          (((l_EFLAGS&ZF_FLAG)!=0) ? 0x40 : 0x00) |
          (((l_EFLAGS&AF_FLAG)!=0) ? 0x10 : 0x00) |
          (((l_EFLAGS&PF_FLAG)!=0) ? 0x04 : 0x00) |
          0x02 |
          (((l_EFLAGS&CF_FLAG)!=0) ? 0x01 : 0x00)
          ;
        break;
      case BC_LAR32:
        if ( !PROTECTED_MODE_ACTIVATED || (l_EFLAGS&VM_FLAG) )
          { EXCEPTION ( EXCP_UD ); exception ( jit ); goto stop; }
        else
          {
            // Operació. ATENCIÓ! no genera execpció si està fora dels límits.
            if ( read_segment_descriptor ( jit, selector, &desc, false ) &&
                 lar_check_segment_descriptor ( jit, selector, desc ) )
              {
                res32= desc.w0&0x00F0FF00;
                l_EFLAGS|= ZF_FLAG;
              }
            else { l_EFLAGS&= ~ZF_FLAG; }
          }
        break;
      case BC_LAR16:
        if ( !PROTECTED_MODE_ACTIVATED || (l_EFLAGS&VM_FLAG) )
          { EXCEPTION ( EXCP_UD ); exception ( jit ); goto stop; }
        else
          {
            // Operació. ATENCIÓ! no genera execpció si està fora dels límits.
            if ( read_segment_descriptor ( jit, selector, &desc, false ) &&
                 lar_check_segment_descriptor ( jit, selector, desc ) )
              {
                res16= (uint16_t) (desc.w0&0xFF00);
                l_EFLAGS|= ZF_FLAG;
              }
            else { l_EFLAGS&= ~ZF_FLAG; }
          }
        break;
      case BC_LEAVE32:
        if ( l_P_SS->h.is32 ) l_ESP= l_EBP;
        else                  l_SP= l_BP;
        if ( !pop32 ( jit, &l_EBP ) ) { exception ( jit ); goto stop; }
        break;
      case BC_LEAVE16:
        if ( l_P_SS->h.is32 ) l_ESP= l_EBP;
        else                  l_SP= l_BP;
        if ( !pop16 ( jit, &l_BP ) ) { exception ( jit ); goto stop; }
        break;
      case BC_LGDT32:
        if ( check_seg_level0 ( jit ) )
          {
            l_GDTR.addr= offset;
            l_GDTR.firstb= 0;
            l_GDTR.lastb= selector;
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_LGDT16:
        if ( check_seg_level0 ( jit ) )
          {
            l_GDTR.addr= offset&0xFFFFFF;
            l_GDTR.firstb= 0;
            l_GDTR.lastb= selector;
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_LIDT32:
        if ( check_seg_level0 ( jit ) )
          {
            l_IDTR.addr= offset;
            l_IDTR.firstb= 0;
            l_IDTR.lastb= selector;
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_LIDT16:
        if ( check_seg_level0 ( jit ) )
          {
            l_IDTR.addr= offset&0xFFFFFF;
            l_IDTR.firstb= 0;
            l_IDTR.lastb= selector;
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_LLDT:
        if ( !lldt ( jit, res16 ) ) { exception ( jit ); goto stop; }
        break;
      case BC_LSL32:
        if ( !PROTECTED_MODE_ACTIVATED || (l_EFLAGS&VM_FLAG) )
          { EXCEPTION ( EXCP_UD ); exception ( jit ); goto stop; }
        else
          {
            // Operació. ATENCIÓ! no genera execpció si està fora dels límits.
            if ( read_segment_descriptor ( jit, selector, &desc, false ) &&
                 lsl_check_segment_descriptor ( jit, selector, desc ) )
              {
                tmp32= (desc.w0&0x000F0000)|(desc.w1&0xFFFF);
                if ( SEG_DESC_G_IS_SET ( desc ) )
                  tmp32= (tmp32<<12) | 0xFFF;
                res32= tmp32;
                l_EFLAGS|= ZF_FLAG;
              }
            else { l_EFLAGS&= ~ZF_FLAG; }
          }
        break;
      case BC_LSL16:
        if ( !PROTECTED_MODE_ACTIVATED || (l_EFLAGS&VM_FLAG) )
          { EXCEPTION ( EXCP_UD ); exception ( jit ); goto stop; }
        else
          {
            // Operació. ATENCIÓ! no genera execpció si està fora dels límits.
            if ( read_segment_descriptor ( jit, selector, &desc, false ) &&
                 lsl_check_segment_descriptor ( jit, selector, desc ) )
              {
                tmp32= (desc.w0&0x000F0000)|(desc.w1&0xFFFF);
                if ( SEG_DESC_G_IS_SET ( desc ) )
                  tmp32= (tmp32<<12) | 0xFFF;
                res16= (uint16_t) (tmp32&0xFFFF);
                l_EFLAGS|= ZF_FLAG;
              }
            else { l_EFLAGS&= ~ZF_FLAG; }
          }
        break;
      case BC_LTR:
        if ( !ltr ( jit, res16 ) ) { exception ( jit ); goto stop; }
        break;
      case BC_POP32:
        if ( !pop32 ( jit, &res32 ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_POP16:
        if ( !pop16 ( jit, &res16 ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_POP32_EFLAGS:
        if ( !pop32_eflags ( jit ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_POP16_EFLAGS:
        if ( !pop16_eflags ( jit ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_PUSH32:
        if ( !push32 ( jit, res32 ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_PUSH16:
        if ( !push16 ( jit, res16 ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_PUSH32_EFLAGS:
        if ( !push32 ( jit, l_EFLAGS&0x00FCFFFF ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_PUSH16_EFLAGS:
        if ( !push16 ( jit, (uint16_t) (l_EFLAGS&0xFFFF) ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_PUSHA32_CHECK_EXCEPTION:
        if ( !PROTECTED_MODE_ACTIVATED &&
             (l_ESP==7 || l_ESP==9 || l_ESP==11 || l_ESP==13 || l_ESP==15) )
          {
            fprintf ( stderr, "[EE] jit_exec - BC_PUSHA32_CHECK_EXCEPTION:"
                      " cal implementar exception\n" );
            exit ( EXIT_FAILURE );
            exception ( jit );
            goto stop;
          }
        break;
      case BC_PUSHA16_CHECK_EXCEPTION:
        if ( !PROTECTED_MODE_ACTIVATED &&
             (l_SP==7 || l_SP==9 || l_SP==11 || l_SP==13 || l_SP==15) )
          {
            fprintf ( stderr, "[EE] jit_exec - BC_PUSHA16_CHECK_EXCEPTION:"
                      " cal implementar exception\n" );
            exit ( EXIT_FAILURE );
            exception ( jit );
            goto stop;
          }
        break;
      case BC_RET32_FAR:
        if ( ret_far ( jit, true ) ) goto_eip ( jit );
        else                         exception ( jit );
        goto stop;
        break;
      case BC_RET32_FAR_NOSTOP:
        if ( !ret_far ( jit, true ) )  { exception ( jit ); goto stop; }
        break;
      case BC_RET16_FAR:
        if ( ret_far ( jit, false ) ) goto_eip ( jit );
        else                          exception ( jit );
        goto stop;
        break;
      case BC_RET16_FAR_NOSTOP:
        if ( !ret_far ( jit, false ) ) { exception ( jit ); goto stop; }
        break;
        // COMPTE QUE RT16 fa comprovacions
      case BC_RET32_RES: l_EIP= res32; goto_eip ( jit ); goto stop; break;
      case BC_RET32_RES_INC_ESP:
        l_EIP= res32;
        l_ESP+= (uint32_t) (p->v[pos+1]);
        goto_eip ( jit );
        goto stop;
        break;
      case BC_RET32_RES_INC_SP:
        l_EIP= res32;
        l_SP+= p->v[pos+1];
        goto_eip ( jit );
        goto stop;
        break;
      case BC_RET16_RES:
        res32= (uint32_t) res16;
        if ( res32 < P_CS->h.lim.firstb || res32 > P_CS->h.lim.lastb )
          { EXCEPTION0 ( EXCP_GP ); exception ( jit ); }
        else { l_EIP= res32; goto_eip ( jit ); }
        goto stop;
        break;
      case BC_RET16_RES_INC_ESP:
        res32= (uint32_t) res16;
        if ( res32 < P_CS->h.lim.firstb || res32 > P_CS->h.lim.lastb )
          { EXCEPTION0 ( EXCP_GP ); exception ( jit ); }
        else
          { l_EIP= res32; l_ESP+= (uint32_t) (p->v[pos+1]); goto_eip ( jit ); }
        goto stop;
        break;
      case BC_RET16_RES_INC_SP:
        res32= (uint32_t) res16;
        if ( res32 < P_CS->h.lim.firstb || res32 > P_CS->h.lim.lastb )
          { EXCEPTION0 ( EXCP_GP ); exception ( jit ); }
        else
          { l_EIP= res32; l_SP+= p->v[pos+1]; goto_eip ( jit ); }
        goto stop;
        break;
      case BC_INC_ESP_AND_GOTO_EIP:
        l_ESP+= (uint32_t) (p->v[pos+1]);
        goto_eip ( jit );
        goto stop;
        break;
      case BC_INC_SP_AND_GOTO_EIP:
        l_SP+= p->v[pos+1];
        goto_eip ( jit );
        goto stop;
        break;
      case BC_SAHF:
        if ( l_AH&0x80 ) l_EFLAGS|= SF_FLAG;
        else             l_EFLAGS&= ~SF_FLAG;
        if ( l_AH&0x40 ) l_EFLAGS|= ZF_FLAG;
        else             l_EFLAGS&= ~ZF_FLAG;
        if ( l_AH&0x10 ) l_EFLAGS|= AF_FLAG;
        else             l_EFLAGS&= ~AF_FLAG;
        if ( l_AH&0x04 ) l_EFLAGS|= PF_FLAG;
        else             l_EFLAGS&= ~PF_FLAG;
        if ( l_AH&0x01 ) l_EFLAGS|= CF_FLAG;
        else             l_EFLAGS&= ~CF_FLAG;
        break;
      case BC_STI: if ( !sti ( jit ) ) { exception ( jit ); goto stop; } break;
      case BC_VERR:
        if ( !verr_verw ( jit, res16, true ) ) { exception ( jit ); goto stop; }
        break;
      case BC_VERW:
        if ( !verr_verw ( jit, res16, false ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_WBINVD:
        if ( check_seg_level0 ( jit ) )
          {
            printf("[CAL_IMPLEMENTAR] WBINVD\n");
          }
        else { EXCEPTION0 ( EXCP_GP ); exception ( jit ); goto stop; }
        break;
        // --> Strings
      case BC_CMPS32_ADDR32:
        if ( jit->_mem_read32 ( jit, l_P_ES, l_EDI, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            res32= (uint32_t) ((int32_t) op32[0] - (int32_t) op32[1]);
            if ( l_EFLAGS&DF_FLAG ) { l_ESI-=4; l_EDI-=4; }
            else                    { l_ESI+=4; l_EDI+=4; }
          }
        break;
      case BC_CMPS8_ADDR32:
        if ( jit->_mem_read8 ( jit, l_P_ES, l_EDI, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            res8= (uint8_t) ((int8_t) op8[0] - (int8_t) op8[1]);
            if ( l_EFLAGS&DF_FLAG ) { --l_ESI; --l_EDI; }
            else                    { ++l_ESI; ++l_EDI; }
          }
        break;
      case BC_CMPS32_ADDR16:
        if ( jit->_mem_read32 ( jit, l_P_ES, (uint32_t)l_DI,
                               &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            res32= (uint32_t) ((int32_t) op32[0] - (int32_t) op32[1]);
            if ( l_EFLAGS&DF_FLAG ) { l_SI-=4; l_DI-=4; }
            else                    { l_SI+=4; l_DI+=4; }
          }
        break;
      case BC_CMPS16_ADDR16:
        if ( jit->_mem_read16 ( jit, l_P_ES, (uint32_t)l_DI,
                               &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            res16= (uint16_t) ((int16_t) op16[0] - (int16_t) op16[1]);
            if ( l_EFLAGS&DF_FLAG ) { l_SI-=2; l_DI-=2; }
            else                    { l_SI+=2; l_DI+=2; }
          }
        break;
      case BC_CMPS8_ADDR16:
        if ( jit->_mem_read8 ( jit, l_P_ES, (uint32_t)l_DI,
                               &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            res8= (uint8_t) ((int8_t) op8[0] - (int8_t) op8[1]);
            if ( l_EFLAGS&DF_FLAG ) { --l_SI; --l_DI; }
            else                    { ++l_SI; ++l_DI; }
          }
        break;
      case BC_INS16_ADDR32:
        if ( io_check_permission ( jit, l_DX, 2 ) )
          {
            res16= jit->port_read16 ( jit->udata, l_DX );
            if ( jit->_mem_write16 ( jit, l_P_ES, l_EDI, res16 ) != 0 )
              { exception ( jit ); goto stop; }
            else
              {
                if ( l_EFLAGS&DF_FLAG ) { l_EDI-= 2; }
                else                    { l_EDI+= 2; }
              }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_INS8_ADDR32:
        if ( io_check_permission ( jit, l_DX, 1 ) )
          {
            res8= jit->port_read8 ( jit->udata, l_DX );
            if ( jit->_mem_write8 ( jit, l_P_ES, l_EDI, res8 ) != 0 )
              { exception ( jit ); goto stop; }
            else
              {
                if ( l_EFLAGS&DF_FLAG ) { --l_EDI; }
                else                    { ++l_EDI; }
              }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_INS32_ADDR16:
        if ( io_check_permission ( jit, l_DX, 4 ) )
          {
            res32= jit->port_read32 ( jit->udata, l_DX );
            if ( jit->_mem_write32 ( jit, l_P_ES, (uint32_t) l_DI,
                                     res32 ) != 0 )
              { exception ( jit ); goto stop; }
            else
              {
                if ( l_EFLAGS&DF_FLAG ) { l_DI-= 4; }
                else                    { l_DI+= 4; }
              }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_INS16_ADDR16:
        if ( io_check_permission ( jit, l_DX, 2 ) )
          {
            res16= jit->port_read16 ( jit->udata, l_DX );
            if ( jit->_mem_write16 ( jit, l_P_ES, (uint32_t) l_DI,
                                     res16 ) != 0 )
              { exception ( jit ); goto stop; }
            else
              {
                if ( l_EFLAGS&DF_FLAG ) { l_DI-= 2; }
                else                    { l_DI+= 2; }
              }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_LODS32_ADDR32:
        l_EAX= res32;
        if ( l_EFLAGS&DF_FLAG ) { l_ESI-= 4; }
        else                    { l_ESI+= 4; }
        break;
      case BC_LODS16_ADDR32:
        l_AX= res16;
        if ( l_EFLAGS&DF_FLAG ) { l_ESI-= 2; }
        else                    { l_ESI+= 2; }
        break;
      case BC_LODS32_ADDR16:
        l_EAX= res32;
        if ( l_EFLAGS&DF_FLAG ) { l_SI-= 4; }
        else                    { l_SI+= 4; }
        break;
      case BC_LODS16_ADDR16:
        l_AX= res16;
        if ( l_EFLAGS&DF_FLAG ) { l_SI-= 2; }
        else                    { l_SI+= 2; }
        break;
      case BC_LODS8_ADDR32:
        l_AL= res8;
        if ( l_EFLAGS&DF_FLAG ) { --l_ESI; }
        else                    { ++l_ESI; }
        break;
      case BC_LODS8_ADDR16:
        l_AL= res8;
        if ( l_EFLAGS&DF_FLAG ) { --l_SI; }
        else                    { ++l_SI; }
        break;
      case BC_MOVS32_ADDR32:
        if ( jit->_mem_write32 ( jit, l_P_ES, l_EDI, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_ESI-= 4; l_EDI-= 4; }
            else                    { l_ESI+= 4; l_EDI+= 4; }
          }
        break;
      case BC_MOVS16_ADDR32:
        if ( jit->_mem_write16 ( jit, l_P_ES, l_EDI, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_ESI-= 2; l_EDI-= 2; }
            else                    { l_ESI+= 2; l_EDI+= 2; }
          }
        break;
      case BC_MOVS8_ADDR32:
        if ( jit->_mem_write8 ( jit, l_P_ES, l_EDI, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { --l_ESI; --l_EDI; }
            else                    { ++l_ESI; ++l_EDI; }
          }
        break;
      case BC_MOVS32_ADDR16:
        if ( jit->_mem_write32 ( jit, l_P_ES, (uint32_t) l_DI, res32 ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_SI-= 4; l_DI-= 4; }
            else                    { l_SI+= 4; l_DI+= 4; }
          }
        break;
      case BC_MOVS16_ADDR16:
        if ( jit->_mem_write16 ( jit, l_P_ES, (uint32_t) l_DI, res16 ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_SI-= 2; l_DI-= 2; }
            else                    { l_SI+= 2; l_DI+= 2; }
          }
        break;
      case BC_MOVS8_ADDR16:
        if ( jit->_mem_write8 ( jit, l_P_ES, (uint32_t) l_DI, res8 ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { --l_SI; --l_DI; }
            else                    { ++l_SI; ++l_DI; }
          }
        break;
      case BC_OUTS16_ADDR32:
        if ( io_check_permission ( jit, l_DX, 2 ) )
          {
            jit->port_write16 ( jit->udata, l_DX, res16 );
            if ( l_EFLAGS&DF_FLAG ) { l_ESI-= 2; }
            else                    { l_ESI+= 2; }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_OUTS8_ADDR32:
        if ( io_check_permission ( jit, l_DX, 1 ) )
          {
            jit->port_write8 ( jit->udata, l_DX, res8 );
            if ( l_EFLAGS&DF_FLAG ) { --l_ESI; }
            else                    { ++l_ESI; }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_OUTS16_ADDR16:
        if ( io_check_permission ( jit, l_DX, 2 ) )
          {
            jit->port_write16 ( jit->udata, l_DX, res16 );
            if ( l_EFLAGS&DF_FLAG ) { l_SI-= 2; }
            else                    { l_SI+= 2; }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_OUTS8_ADDR16:
        if ( io_check_permission ( jit, l_DX, 1 ) )
          {
            jit->port_write8 ( jit->udata, l_DX, res8 );
            if ( l_EFLAGS&DF_FLAG ) { --l_SI; }
            else                    { ++l_SI; }
          }
        else { exception ( jit ); goto stop; }
        break;
      case BC_SCAS32_ADDR32:
        if ( jit->_mem_read32 ( jit, l_P_ES, l_EDI, &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            op32[0]= l_EAX; // <-- Comparació
            res32= (uint32_t) ((int32_t) l_EAX - (int32_t) op32[1]);
            if ( l_EFLAGS&DF_FLAG ) { l_EDI-= 4; }
            else                    { l_EDI+= 4; }
          }
        break;
      case BC_SCAS16_ADDR32:
        if ( jit->_mem_read16 ( jit, l_P_ES, l_EDI, &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            op16[0]= l_AX;
            res16= (uint16_t) ((int16_t) l_AX - (int16_t) op16[1]);
            if ( l_EFLAGS&DF_FLAG ) { l_EDI-= 2; }
            else                    { l_EDI+= 2; }
          }
        break;
      case BC_SCAS8_ADDR32:
        if ( jit->_mem_read8 ( jit, l_P_ES, l_EDI, &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            op8[0]= l_AL;
            res8= (uint8_t) ((int8_t) l_AL - (int8_t) op8[1]);
            if ( l_EFLAGS&DF_FLAG ) { --l_EDI; }
            else                    { ++l_EDI; }
          }
        break;
      case BC_SCAS32_ADDR16:
        if ( jit->_mem_read32 ( jit, l_P_ES, (uint32_t)l_DI,
                                &op32[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            op32[0]= l_EAX;
            res32= (uint32_t) ((int32_t) l_EAX - (int32_t) op32[1]);
            if ( l_EFLAGS&DF_FLAG ) { l_DI-= 4; }
            else                    { l_DI+= 4; }
          }
        break;
      case BC_SCAS16_ADDR16:
        if ( jit->_mem_read16 ( jit, l_P_ES, (uint32_t)l_DI,
                                &op16[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            op16[0]= l_AX;
            res16= (uint16_t) ((int16_t) l_AX - (int16_t) op16[1]);
            if ( l_EFLAGS&DF_FLAG ) { l_DI-= 2; }
            else                    { l_DI+= 2; }
          }
        break;
      case BC_SCAS8_ADDR16:
        if ( jit->_mem_read8 ( jit, l_P_ES, (uint32_t)l_DI,
                               &op8[1], true ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            op8[0]= l_AL;
            res8= (uint8_t) ((int8_t) l_AL - (int8_t) op8[1]);
            if ( l_EFLAGS&DF_FLAG ) { --l_DI; }
            else                    { ++l_DI; }
          }
        break;
      case BC_STOS32_ADDR32:
        if ( jit->_mem_write32 ( jit, l_P_ES, l_EDI, l_EAX ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_EDI-= 4; }
            else                    { l_EDI+= 4; }
          }
        break;
      case BC_STOS32_ADDR16:
        if ( jit->_mem_write32 ( jit, l_P_ES, (uint32_t)l_DI, l_EAX ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_DI-= 4; }
            else                    { l_DI+= 4; }
          }
        break;
      case BC_STOS16_ADDR32:
        if ( jit->_mem_write16 ( jit, l_P_ES, l_EDI, l_AX ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_EDI-= 2; }
            else                    { l_EDI+= 2; }
          }
        break;
      case BC_STOS16_ADDR16:
        if ( jit->_mem_write16 ( jit, l_P_ES, (uint32_t) l_DI, l_AX ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { l_DI-= 2; }
            else                    { l_DI+= 2; }
          }
        break;
      case BC_STOS8_ADDR32:
        if ( jit->_mem_write8 ( jit, l_P_ES, (uint32_t) l_EDI, l_AL ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { --l_EDI; }
            else                    { ++l_EDI; }
          }
        break;
      case BC_STOS8_ADDR16:
        if ( jit->_mem_write8 ( jit, l_P_ES, (uint32_t) l_DI, l_AL ) != 0 )
          { exception ( jit ); goto stop; }
        else
          {
            if ( l_EFLAGS&DF_FLAG ) { --l_DI; }
            else                    { ++l_DI; }
          }
        break;
        // --> FPU WRITE
      case BC_FPU_DS_FLOATU32_WRITE32:
        if ( fpu_ok )
          {
            if ( jit->_mem_write32 ( jit, l_P_DS, offset, float_u32.u32 ) != 0 )
              { exception ( jit ); goto stop; }
          }
        break;
      case BC_FPU_SS_FLOATU32_WRITE32:
        if ( fpu_ok )
          {
            if ( jit->_mem_write32 ( jit, l_P_SS, offset, float_u32.u32 ) != 0 )
              { exception ( jit ); goto stop; }
          }
        break;
      case BC_FPU_ES_FLOATU32_WRITE32:
        if ( fpu_ok )
          {
            if ( jit->_mem_write32 ( jit, l_P_ES, offset, float_u32.u32 ) != 0 )
              { exception ( jit ); goto stop; }
          }
        break;
      case BC_FPU_DS_DOUBLEU64_WRITE:
        if ( fpu_ok )
          {
            if ( jit->_mem_write32
                 ( jit, l_P_DS, offset, double_u64.u32.l ) != 0 )
              { exception ( jit ); goto stop; }
            else if ( jit->_mem_write32
                      ( jit, l_P_DS, offset+4, double_u64.u32.h ) != 0 )
              { exception ( jit ); goto stop; }
          }
        break;
      case BC_FPU_SS_DOUBLEU64_WRITE:
        if ( fpu_ok )
          {
            if ( jit->_mem_write32
                 ( jit, l_P_SS, offset, double_u64.u32.l ) != 0 )
              { exception ( jit ); goto stop; }
            else if ( jit->_mem_write32
                      ( jit, l_P_SS, offset+4, double_u64.u32.h ) != 0 )
              { exception ( jit ); goto stop; }
          }
        break;
      case BC_FPU_DS_LDOUBLEU80_WRITE:
        if ( fpu_ok )
          {
            if ( jit->_mem_write32
                 ( jit, l_P_DS, offset, ldouble_u80.u32.v0 ) != 0 )
              { exception ( jit ); goto stop; }
            else if ( jit->_mem_write32
                      ( jit, l_P_DS, offset+4, ldouble_u80.u32.v1 ) != 0 )
              { exception ( jit ); goto stop; }
            else if ( jit->_mem_write16
                      ( jit, l_P_DS, offset+8, ldouble_u80.u16.v4 ) != 0 )
              { exception ( jit ); goto stop; }
          }
        break;
      case BC_FPU_SS_LDOUBLEU80_WRITE:
        if ( fpu_ok )
          {
            if ( jit->_mem_write32
                 ( jit, l_P_SS, offset, ldouble_u80.u32.v0 ) != 0 )
              { exception ( jit ); goto stop; }
            else if ( jit->_mem_write32
                      ( jit, l_P_SS, offset+4, ldouble_u80.u32.v1 ) != 0 )
              { exception ( jit ); goto stop; }
            else if ( jit->_mem_write16
                      ( jit, l_P_SS, offset+8, ldouble_u80.u16.v4 ) != 0 )
              { exception ( jit ); goto stop; }
          }
        break;
        // --> FPU
      case BC_FPU_2XM1:
        if ( fpu_ok ) { fpu_ok= fpu_2xm1 ( jit, fpu_reg ); }
        break;
      case BC_FPU_ABS:
        if ( fpu_ok ) { l_FPU_REG(fpu_reg).v= fabsl ( l_FPU_REG(fpu_reg).v ); }
        break;
      case BC_FPU_ADD:
        if ( fpu_ok ) { fpu_ok= fpu_add ( jit, fpu_reg, ldouble ); }
        break;
      case BC_FPU_BEGIN_OP:
        fpu_ok= true;
        if ( !fpu_begin_op ( jit, true ) ) { exception ( jit ); goto stop; }
        break;
      case BC_FPU_BEGIN_OP_CLEAR_C1:
        fpu_ok= true;
        if ( !fpu_begin_op ( jit, true ) ) { exception ( jit ); goto stop; }
        l_FPU_STATUS&= ~FPU_STATUS_C1;
        break;
      case BC_FPU_BEGIN_OP_NOCHECK:
        fpu_ok= true;
        if ( !fpu_begin_op ( jit, false ) ) { exception ( jit ); goto stop; }
        break;
      case BC_FPU_BSTP_SS:
        if ( fpu_ok )
          {
            if ( !fpu_bstp ( jit, fpu_reg, l_P_SS, offset, &fpu_ok ) )
              { exception ( jit ); goto stop; }
          }
        break;
      case BC_FPU_CHS:
        fpu_ok= true;
        if ( !fpu_chs ( jit ) ) { exception ( jit ); goto stop; }
        break;
      case BC_FPU_CLEAR_EXCP:
        fpu_ok= true;
        if ( !fpu_clear_excp ( jit ) ) { exception ( jit ); goto stop; }
        break;
      case BC_FPU_COM:
        if ( fpu_ok ) { fpu_ok= fpu_com ( jit, fpu_reg, ldouble ); }
        break;
      case BC_FPU_COS:
        if ( fpu_ok ) { fpu_ok= fpu_cos ( jit, fpu_reg ); }
        break;
      case BC_FPU_DIV:
        if ( fpu_ok ) { fpu_ok= fpu_div ( jit, fpu_reg, ldouble ); }
        break;
      case BC_FPU_DIVR:
        if ( fpu_ok ) { fpu_ok= fpu_divr ( jit, fpu_reg, ldouble ); }
        break;
      case BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK:
        if ( fpu_ok )
          {
            FPU_CHECK_INVALID_OPERAND_U64(double_u64.u64,fpu_ok= false);
            if ( fpu_ok ) { ldouble= (long double) double_u64.d; }
          }
        break;
      case BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_DENORMAL_SNAN:
        if ( fpu_ok )
          {
            FPU_CHECK_DENORMAL_VALUE_U64(double_u64.u64,fpu_ok= false);
            if ( fpu_ok )
              {
                FPU_CHECK_SNAN_VALUE_U64(double_u64.u64,fpu_ok= false);
                if ( fpu_ok ) { ldouble= (long double) double_u64.d; }
              }
          }
        break;
      case BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_FCOM:
        if ( fpu_ok )
          {
            FPU_FCOM_CHECK_INVALID_OPERAND_U64(double_u64.u64,fpu_ok= false);
            if ( fpu_ok ) { ldouble= (long double) double_u64.d; }
          }
        break;
      case BC_FPU_FLOATU32_LDOUBLE_AND_CHECK:
        if ( fpu_ok )
          {
            FPU_CHECK_INVALID_OPERAND_U32(float_u32.u32,fpu_ok= false);
            if ( fpu_ok ) { ldouble= (long double) float_u32.f; }
          }
        break;
      case BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_DENORMAL_SNAN:
        if ( fpu_ok )
          {
            FPU_CHECK_DENORMAL_VALUE_U32(float_u32.u32,fpu_ok= false);
            if ( fpu_ok )
              {
                FPU_CHECK_SNAN_VALUE_U32(float_u32.u32,fpu_ok= false);
                if ( fpu_ok ) { ldouble= (long double) float_u32.f; }
              }
          }
        break;
      case BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_FCOM:
        if ( fpu_ok )
          {
            FPU_FCOM_CHECK_INVALID_OPERAND_U32(float_u32.u32,fpu_ok= false);
            if ( fpu_ok ) { ldouble= (long double) float_u32.f; }
          }
        break;
      case BC_FPU_FREE:
        if ( fpu_ok )
          {
            fpu_reg= (l_FPU_TOP+p->v[pos+1])&0x7;
            l_FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
          }
        ++pos;
        break;
      case BC_FPU_INIT:
        fpu_ok= true;
        if ( !fpu_init ( jit ) ) { exception ( jit ); goto stop; }
        break;
      case BC_FPU_MUL:
        if ( fpu_ok ) { fpu_ok= fpu_mul ( jit, fpu_reg, ldouble ); }
        break;
      case BC_FPU_PATAN:
        if ( fpu_ok ) { fpu_ok= fpu_patan ( jit, fpu_reg, fpu_reg2 ); }
        break;
      case BC_FPU_POP:
        if ( fpu_ok )
          {
            l_FPU_REG(l_FPU_TOP).tag= IA32_CPU_FPU_TAG_EMPTY;
            l_FPU_TOP= (l_FPU_TOP+1)&0x7;
          }
        break;
      case BC_FPU_PREM:
        if ( fpu_ok ) { fpu_ok= fpu_prem ( jit, fpu_reg, fpu_reg2 ); }
        break;
      case BC_FPU_PTAN:
        if ( fpu_ok ) { fpu_ok= fpu_ptan ( jit, fpu_reg ); }
        break;
      case BC_FPU_PUSH_DOUBLEU64:
        if ( fpu_ok ) fpu_push_double_u64 ( jit, double_u64 );
        break;
      case BC_FPU_PUSH_DOUBLEU64_AS_INT64:
        if ( fpu_ok ) fpu_push_double_u64_as_int64 ( jit, double_u64 );
        break;
      case BC_FPU_PUSH_FLOATU32:
        if ( fpu_ok ) fpu_push_float_u32 ( jit, float_u32 );
        break;
      case BC_FPU_PUSH_L2E:
        if ( fpu_ok )
          fpu_push_val ( jit, 1.4426950408889634074L, IA32_CPU_FPU_TAG_VALID );
        break;
      case BC_FPU_PUSH_LDOUBLEU80:
        if ( fpu_ok ) fpu_push_ldouble_u80 ( jit, ldouble_u80 );
        break;
      case BC_FPU_PUSH_LN2:
        if ( fpu_ok )
          fpu_push_val ( jit, logl ( +2.0L ), IA32_CPU_FPU_TAG_VALID );
        break;
      case BC_FPU_PUSH_ONE:
        if ( fpu_ok ) fpu_push_val ( jit, +1.0L, IA32_CPU_FPU_TAG_VALID );
        break;
      case BC_FPU_PUSH_REG2:
        if ( fpu_ok ) fpu_push_reg ( jit, fpu_reg2 );
        break;
      case BC_FPU_PUSH_RES16:
        if ( fpu_ok ) fpu_push_res16 ( jit, res16 );
        break;
      case BC_FPU_PUSH_RES32:
        if ( fpu_ok ) fpu_push_res32 ( jit, res32 );
        break;
      case BC_FPU_PUSH_ZERO:
        if ( fpu_ok ) fpu_push_val ( jit, +0.0L, IA32_CPU_FPU_TAG_ZERO );
        break;
      case BC_FPU_REG_ST:
        if ( fpu_ok )
          {
            fpu_reg2= (l_FPU_TOP+p->v[pos+1])&0x7;
            l_FPU_REG(fpu_reg2)= l_FPU_REG(fpu_reg);
          }
        ++pos;
        break;
      case BC_FPU_REG_INT16:
        if ( fpu_ok ){fpu_ok= fpu_reg2int16 ( jit, fpu_reg, &res16 );}
        break;
      case BC_FPU_REG_INT32:
        if ( fpu_ok ){fpu_ok= fpu_reg2int32 ( jit, fpu_reg, &float_u32.u32 );}
        break;
      case BC_FPU_REG_INT64:
        if ( fpu_ok ){fpu_ok= fpu_reg2int64 ( jit, fpu_reg, &double_u64.u64 );}
        break;
      case BC_FPU_REG_FLOATU32:
        if ( fpu_ok ) { fpu_ok= fpu_reg2float ( jit, fpu_reg, &float_u32.f ); }
        break;
      case BC_FPU_REG_DOUBLEU64:
        if ( fpu_ok )
          { fpu_ok= fpu_reg2double ( jit, fpu_reg, &double_u64.d ); }
        break;
      case BC_FPU_REG_LDOUBLEU80:
        if ( fpu_ok ) { ldouble_u80.ld= l_FPU_REG(fpu_reg).v; }
        break;
      case BC_FPU_REG2_LDOUBLE_AND_CHECK:
        if ( fpu_ok )
          {
            FPU_CHECK_INVALID_OPERAND(fpu_reg2,fpu_ok= false);
            if ( fpu_ok ) { ldouble= l_FPU_REG(fpu_reg2).v; }
          }
        break;
      case BC_FPU_REG2_LDOUBLE_AND_CHECK_FCOM:
        if ( fpu_ok )
          {
            FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg2,fpu_ok= false,tmp_int);
            if ( fpu_ok ) { ldouble= l_FPU_REG(fpu_reg2).v; }
          }
        break;
      case BC_FPU_RES16_CONTROL_WORD:
        if ( !fpu_load_control_word ( jit, res16 ) )
          { exception ( jit ); goto stop; }
        break;
      case BC_FPU_RNDINT:
        if ( fpu_ok ) { fpu_ok= fpu_rndint ( jit, fpu_reg ); }
        break;
      case BC_FPU_RSTOR32_DS:
        if ( fpu_ok ) { fpu_ok= fpu_rstor32 ( jit, l_P_DS, offset ); }
        break;
      case BC_FPU_SAVE32_DS:
        if ( fpu_ok ) { fpu_ok= fpu_save32 ( jit, l_P_DS, offset ); }
        break;
      case BC_FPU_SCALE:
        if ( fpu_ok ) { fpu_ok= fpu_scale ( jit, fpu_reg, fpu_reg2 ); }
        break;
      case BC_FPU_SELECT_ST0:
        fpu_reg= l_FPU_TOP; // ST(0)
        FPU_CHECK_STACK_UNDERFLOW(fpu_reg,fpu_ok= false);
        break;
      case BC_FPU_SELECT_ST1:
        if ( fpu_ok )
          {
            fpu_reg2= (l_FPU_TOP+1)&0x7; // ST(1)
            FPU_CHECK_STACK_UNDERFLOW(fpu_reg2,fpu_ok= false);
          }
        break;
      case BC_FPU_SELECT_ST_REG:
        if ( fpu_ok )
          {
            fpu_reg= (l_FPU_TOP+p->v[pos+1])&0x7;
            FPU_CHECK_STACK_UNDERFLOW(fpu_reg,fpu_ok= false);
          }
        ++pos;
        break;
      case BC_FPU_SELECT_ST_REG2:
        if ( fpu_ok )
          {
            fpu_reg2= (l_FPU_TOP+p->v[pos+1])&0x7;
            FPU_CHECK_STACK_UNDERFLOW(fpu_reg2,fpu_ok= false);
          }
        ++pos;
        break;
      case BC_FPU_SIN:
        if ( fpu_ok ) { fpu_ok= fpu_sin ( jit, fpu_reg ); }
        break;
      case BC_FPU_SQRT:
        if ( fpu_ok ) { fpu_ok= fpu_sqrt ( jit, fpu_reg ); }
        break;
      case BC_FPU_SUB:
        if ( fpu_ok ) { fpu_ok= fpu_sub ( jit, fpu_reg, ldouble ); }
        break;
      case BC_FPU_SUBR:
        if ( fpu_ok ) { fpu_ok= fpu_subr ( jit, fpu_reg, ldouble ); }
        break;
      case BC_FPU_TST:
        if ( fpu_ok ) { fpu_ok= fpu_tst ( jit, fpu_reg ); }
        break;
      case BC_FPU_WAIT:
        fpu_ok= true;
        if ( !fpu_wait ( jit ) ) { exception ( jit ); goto stop; }
        break;
      case BC_FPU_XAM: if ( fpu_ok ) fpu_xam ( jit ); break;
      case BC_FPU_XCH:
        if ( fpu_ok )
          {
            tmp_fpu_reg= l_FPU_REG(fpu_reg);
            l_FPU_REG(fpu_reg)= l_FPU_REG(fpu_reg2);
            l_FPU_REG(fpu_reg2)= tmp_fpu_reg;
          }
        break;
      case BC_FPU_YL2X:
        if ( fpu_ok ) { fpu_ok= fpu_yl2x ( jit, fpu_reg, fpu_reg2 ); }
        break;
        
      default:
        print_pages ( stderr, jit );
        printf ( "[EE] jit - exec_inst - instrucció desconeguda %u (pos:%d)\n",
                 p->v[pos], pos-2 );
        exit ( EXIT_FAILURE );
      }
    ++pos;
  } while(true);
#pragma GCC diagnostic pop

 stop:
  // Fica la pàgina bloquejada en _free_pages si cal
  if ( jit->_free_lock_page )
    {
      p->prev= NULL;
      p->next= jit->_free_pages;
      jit->_free_pages= p;
      jit->_free_lock_page= false;
    }
  jit->_lock_page= NULL;
  
} // end exec_inst
