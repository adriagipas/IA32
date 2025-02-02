/*
 * Copyright 2022-2025 Adrià Giménez Pastor.
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
 *  interpreter_fpu.h - Implementa instruccions coprocessador matemàtic.
 *
 */

#include <assert.h>
#include <float.h>
#include <fenv.h>
#include <stdint.h>




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
} ldouble_u_t;




/*********************/
/* FUNCIONS I MACROS */
/*********************/

static void
fpu_update_exceptions (
                       PROTO_INTERP
                       )
{

  if ( (FPU_STATUS&(~FPU_CONTROL))&0x3F )
    { FPU_STATUS|= FPU_STATUS_ES; }
  else { FPU_STATUS&= ~FPU_STATUS_ES; }
  
} // end fpu_update_exceptions


static void
fpu_check_exception (
                     PROTO_INTERP
                     )
{

  /*
    NOTA!! CAL REPASSAR AMB CALMA, PERÒ PROBABLEMENT AÇÒ IMPLICA
    EMULAR FERR# (com un callback de la CPU???) i IGNNE# (com una
    funció de la cpu???)
   */
  assert ( (FPU_STATUS&FPU_STATUS_ES)!=0 );
  fprintf ( stderr, "[CAL_IMPLEMENTAR] FPU_CHECK_EXCPTION "
            "STATUS:%X CONTROL:%X\n",
            FPU_STATUS_TOP, FPU_CONTROL );
  exit ( EXIT_FAILURE );
      
} // end fpu_check_exception


static int
fpu_get_round_mode (
                    PROTO_INTERP
                    )
{
  switch ( (FPU_CONTROL&FPU_CONTROL_RC_MASK)>>FPU_CONTROL_RC_SHIFT )
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
                  PROTO_INTERP
                  )
{
  return
    ((uint16_t) FPU_REG(0).tag) |
    (((uint16_t) FPU_REG(1).tag)<<2) |
    (((uint16_t) FPU_REG(2).tag)<<4) |
    (((uint16_t) FPU_REG(3).tag)<<6) |
    (((uint16_t) FPU_REG(4).tag)<<8) |
    (((uint16_t) FPU_REG(5).tag)<<10) |
    (((uint16_t) FPU_REG(6).tag)<<12) |
    (((uint16_t) FPU_REG(7).tag)<<14)
    ;
} // end fpu_get_tag_word


static void
fpu_set_tag_word (
                  PROTO_INTERP,
                  uint16_t tag_word
                  )
{

  int i;


  for ( i= 0; i < 8; ++i )
    {
      FPU_REG(i).tag= (int) (tag_word&0x3);
      tag_word>>= 2;
    }
  
} // end fpu_set_tag_word


static void
fpu_update_tag (
                PROTO_INTERP,
                const int reg
                )
{

  switch ( fpclassify ( FPU_REG(reg).v ) )
    {
    case FP_NAN:
    case FP_INFINITE:
    case FP_SUBNORMAL:
      FPU_REG(reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
      break;
    case FP_ZERO:
      FPU_REG(reg).tag= IA32_CPU_FPU_TAG_ZERO;
      break;
    case FP_NORMAL:
    default:
      FPU_REG(reg).tag= IA32_CPU_FPU_TAG_VALID;
    }
  
} // end fpu_update_tag


#define GET_EXPONENT_LDOUBLE_U(VAL) ((uint16_t) ((VAL).u16.v4&0x7FFF))

#define FPU_CHECK_EXCEPTIONS(ACTION)                            \
  if ( (FPU_STATUS&FPU_STATUS_ES) != 0 )                        \
    { fpu_check_exception ( INTERP ); ACTION; }

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

#define FPU_CHECK_STACK_UNDERFLOW(IND,ACTION)           \
  if ( FPU_REG(IND).tag == IA32_CPU_FPU_TAG_EMPTY )     \
    {                                                   \
      FPU_STATUS|= (FPU_STATUS_IE|FPU_STATUS_SF);       \
      FPU_STATUS&= ~FPU_STATUS_C1;                      \
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_STACK_OVERFLOW(IND,ACTION)                    \
  if ( FPU_REG(IND).tag != IA32_CPU_FPU_TAG_EMPTY )             \
    {                                                           \
      FPU_STATUS|= (FPU_STATUS_IE|FPU_STATUS_SF|FPU_STATUS_C1); \
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )                      \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
    }

#define FPU_CHECK_SNAN_VALUE_U32(U32,ACTION)            \
  if ( FPU_CHECKU32_SNAN(U32) )                         \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_IE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_SNAN_VALUE_U64(U64,ACTION)            \
  if ( FPU_CHECKU64_SNAN(U64) )                         \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_IE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_DENORMAL_VALUE_U32(U32,ACTION)        \
  if ( FPU_CHECKU32_DENORMAL(U32) )                     \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_DE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_DM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_DENORMAL_VALUE_U64(U64,ACTION)        \
  if ( FPU_CHECKU64_DENORMAL(U64) )                     \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_DE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_DM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_DENORMAL_VALUE_U80(U64L,U16H,ACTION)  \
  if ( FPU_CHECKU80_DENORMAL(U64L,U16H) )               \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_DE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_DM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_DENORMAL_REG(IND,ACTION)                      \
  if ( FPU_REG(IND).tag == IA32_CPU_FPU_TAG_SPECIAL &&          \
       fpclassify ( FPU_REG(IND).v ) == FP_SUBNORMAL )          \
    {                                                           \
      FPU_STATUS|= FPU_STATUS_DE;                               \
      if ( !(FPU_CONTROL&FPU_CONTROL_DM) )                      \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
    }

#define FPU_CHECK_NUMERIC_OVERFLOW_BOOL(VAL,ACTION)     \
  if ( (VAL) )                                          \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_OE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_OM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(VAL,ACTION)    \
  if ( (VAL) )                                          \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_UE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_UM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

#define FPU_CHECK_INVALID_OPERATION_BOOL(VAL,ACTION)    \
  if ( (VAL) )                                          \
    {                                                   \
      FPU_STATUS|= FPU_STATUS_IE;                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }         \
    }

//  NOTA!! Tenia aquest missatge però l'he llevat
//
// WW ( UDATA,                                                       
//      "inexact result exception - C1 update not emulated, set to 0" ); 
#define FPU_CHECK_INEXACT_RESULT_BOOL(VAL,ACTION)                       \
  if ( (VAL) )                                                          \
    {                                                                   \
      FPU_STATUS&= ~FPU_STATUS_C1;                                      \
      FPU_STATUS|= FPU_STATUS_PE;                                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_PM) )                              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                         \
    }

#define FPU_CHECK_DIVBYZERO_RESULT_BOOL(VAL,ACTION)                     \
  if ( (VAL) )                                                          \
    {                                                                   \
      FPU_STATUS|= FPU_STATUS_ZE;                                       \
      if ( !(FPU_CONTROL&FPU_CONTROL_ZM) )                              \
        { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                         \
    }

#define FPU_FCOM_CHECK_INVALID_OPERAND(IND,ACTION,INT_VAR)      \
  if ( FPU_REG(IND).tag == IA32_CPU_FPU_TAG_SPECIAL )           \
    {                                                           \
      (INT_VAR)= fpclassify ( FPU_REG(IND).v );                 \
      if ( (INT_VAR) == FP_SUBNORMAL )                          \
        {                                                       \
          FPU_STATUS|= FPU_STATUS_DE;                           \
          if ( !(FPU_CONTROL&FPU_CONTROL_DM) )                  \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }             \
        }                                                       \
      else if ( (INT_VAR) == FP_NAN )                           \
        {                                                       \
          FPU_STATUS|= FPU_STATUS_IE;                           \
          if ( !(FPU_CONTROL&FPU_CONTROL_IM) )                  \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }             \
        }                                                       \
    }




/***************************/
/* F2XM1 - Compute 2^x - 1 */
/***************************/

static void
f2xm1 (
       PROTO_INTERP
       )
{
  
  int fpu_reg,ftype;
  bool i_exc,u_exc;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_UNDERFLOW|FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v= powl ( 2.0L, FPU_REG(fpu_reg).v ) - 1.0L;
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  
} // end f2xm1




/*************************/
/* FABS - Absolute Value */
/*************************/

static void
fabs_reg0 (
           PROTO_INTERP
           )
{

  int fpu_reg0;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);

  FPU_STATUS&= ~FPU_STATUS_C1;
  fpu_reg0= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_REG(fpu_reg0).v= fabsl ( FPU_REG(fpu_reg0).v );
  
} // end fabs_reg0




/**************************/
/* FADD/FADDP/FIADD - Add */
/**************************/

static int
fadd_common (
             PROTO_INTERP,
             const int         fpu_reg,
             const long double src
             )
{
  
  bool o_exc,u_exc,i_exc,ia_exc;

  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v+= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return -1);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return -1);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return -1);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return -1);
  
  return 0;
  
} // end fadd_common


static void
fadd_float (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return ); }
  else val.u32= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  
  // Operació
  if ( fadd_common ( INTERP, fpu_reg, (long double) val.f ) != 0 )
    return;
  
} // end fadd_float


static void
fadd_double (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fadd_double - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  
  // Operació
  if ( fadd_common ( INTERP, fpu_reg, (long double) val.d ) != 0 )
    return;
  
} // end fadd_double


static void
fadd80_i_0 (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{
  
  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fadd_common ( INTERP, fpu_regi, FPU_REG(fpu_reg0).v ) != 0 )
    return;
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fadd80_i_0


static void
fadd_0_i (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fadd_common ( INTERP, fpu_reg0, FPU_REG(fpu_regi).v ) != 0 )
    return;
  
} // end fadd_0_i


static void
faddp_i_0 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  fadd80_i_0 ( INTERP, modrmbyte, true );
} // end faddp_i_0




/*************************************/
/* FBSTP - Store BCD Integer and Pop */
/*************************************/

static void
fbstp (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{
  
  eaddr_r32_t eaddr;
  int fpu_reg,i;
  long double src;
  bool i_exc;
  uint64_t value,tmp;
  uint8_t buf[10];
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;

  // IMPORTANT!! No tinc clar que m80bcd siga get_addr_mode_r32
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      // NO SÉ QUÈ FER ACÍ!!!
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fbstp - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }

  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  src= FPU_REG(fpu_reg).v;

  // Comprovacios prèvies.
  if ( isnan(src) || isinf(src) )
    {
      FPU_STATUS|= FPU_STATUS_IE;
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )
        { FPU_STATUS|= FPU_STATUS_ES; return; }
    }

  // Redondeja
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  src= rintl ( src );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  
  // Comprovacions posteriors.
  if ( src < -999999999999999999.0L || src > 999999999999999999.0L )
    {
      FPU_STATUS|= FPU_STATUS_IE;
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )
        { FPU_STATUS|= FPU_STATUS_ES; return; }
    }

  // Converteix a BCD (NOTA!! Igual les excepcions no estan activades)
  if ( isnan(src) || isinf(src) ) buf[9]= 0xff;
  else
    {
      if ( src < 0.0 ) { src= -src; buf[9]= 0x80; }
      else             buf[9]= 0x00;
      value= (uint64_t) src;
      for ( int i= 0; i < 9; i++ )
        {
          tmp= value%((uint64_t) 100);
          value/= ((uint64_t) 100);
          buf[i]= (tmp%10) | ((tmp/10)<<4);
        }
    }

  // Escriu
  for ( i= 0; i < 10; i++ )
    {
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+i, buf[i], return );
    }
  
  // Pop
  FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_TOP= (FPU_TOP+1)&0x7;
  
} // end fbstp




/**********************/
/* FCHS - Change Sign */
/**********************/

static void
fchs (
      PROTO_INTERP
      )
{

  int fpu_reg;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }

  FPU_STATUS&= ~FPU_STATUS_C0;
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_REG(fpu_reg).v= -FPU_REG(fpu_reg).v;
  
} // end fchs




/***********************************/
/* FCLEX/FNCLEX - Clear Exceptions */
/***********************************/

static void
fclex (
       PROTO_INTERP
       )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }

  // The PE, UE, OE, ZE, DE, IE, ES, SF, and B flags in the FPU status
  // word are cleared. The C0, C1, C2, and C3 flags are undefined.
  // NOTA!! Com no afecta a TOP no cal modificar top.
  FPU_STATUS&= ~0x80FF;
  
} // end fclex




/*****************************************************/
/* FCOM/FCOMP/FCOMPP - Compare Floating Point Values */
/*****************************************************/

#define FPU_FCOM_CHECK_INVALID_OPERAND_U32(U32,ACTION)              \
  if ( FPU_CHECKU32_SPECIAL(U32) )                                  \
    {                                                               \
      if ( FPU_CHECKU32_DENORMAL(U32) )                             \
        {                                                           \
          FPU_STATUS|= FPU_STATUS_DE;                               \
          if ( !(FPU_CONTROL&FPU_CONTROL_DM) )                      \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
        }                                                           \
      else if ( FPU_CHECKU32_NAN(U32) )                             \
        {                                                           \
          FPU_STATUS|= FPU_STATUS_IE;                               \
          if ( !(FPU_CONTROL&FPU_CONTROL_IM) )                      \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
        }                                                           \
      }

#define FPU_FCOM_CHECK_INVALID_OPERAND_U64(U64,ACTION)              \
  if ( FPU_CHECKU64_SPECIAL(U64) )                                  \
    {                                                               \
      if ( FPU_CHECKU64_DENORMAL(U64) )                             \
        {                                                           \
          FPU_STATUS|= FPU_STATUS_DE;                               \
          if ( !(FPU_CONTROL&FPU_CONTROL_DM) )                      \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
        }                                                           \
      else if ( FPU_CHECKU64_NAN(U64) )                             \
        {                                                           \
          FPU_STATUS|= FPU_STATUS_IE;                               \
          if ( !(FPU_CONTROL&FPU_CONTROL_IM) )                      \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }                 \
        }                                                           \
      }


static void
fcom_common (
             PROTO_INTERP,
             const long double st0,
             const long double src
             )
{
  
  if ( st0 > src )
    {
      FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
    }
  else if ( st0 < src )
    {
      FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
      FPU_STATUS|= FPU_STATUS_C0;
    }
  else // st0 == src
    {
      FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
      FPU_STATUS|= FPU_STATUS_C3;
    }
  
} // end fcom_common


static void
fcom_float (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg,ftype;
  float_u32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fcom_float - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  // En cas d'error IA tots els bits a 1
  FPU_STATUS|= (FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return,ftype);
  FPU_FCOM_CHECK_INVALID_OPERAND_U32(val.u32,return);
  
  // Operació
  fcom_common ( INTERP, FPU_REG(fpu_reg).v, (long double) val.f );
  if ( pop )
    {
      FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fcom_float


static void
fcom_double (
             PROTO_INTERP,
             const uint8_t modrmbyte,
             const bool    pop
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg,ftype;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fcom_double - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  // En cas d'error IA tots els bits a 1
  FPU_STATUS|= (FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return,ftype);
  FPU_FCOM_CHECK_INVALID_OPERAND_U64(val.u64,return);
  
  // Operació
  fcom_common ( INTERP, FPU_REG(fpu_reg).v, (long double) val.d );
  if ( pop )
    {
      FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fcom_double


static void
fcom80 (
        PROTO_INTERP,
        const uint8_t modrmbyte,
        const bool    pop
        )
{
  
  int fpu_reg0,fpu_reg_src,ftype;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg_src= (FPU_TOP+(modrmbyte&0x7))&0x7; // ST source
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg_src,return);
  // En cas d'error IA tots els bits a 1
  FPU_STATUS|= (FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg0,return,ftype);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg_src,return,ftype);

  // Operació.
  fcom_common ( INTERP, FPU_REG(fpu_reg0).v, FPU_REG(fpu_reg_src).v );
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fcom80


static void
fcom (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{
  fcom80 ( INTERP, modrmbyte, false );
} // end fcom


static void
fcomp (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{
  fcom80 ( INTERP, modrmbyte, true );
} // end fcomp


static void
fcompp (
        PROTO_INTERP
        )
{

  int fpu_reg0,fpu_reg1,ftype;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg1= (FPU_TOP+1)&0x7; // ST(1)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg1,return);
  // En cas d'error IA tots els bits a 1
  FPU_STATUS|= (FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg0,return,ftype);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg1,return,ftype);

  // Operació.
  fcom_common ( INTERP, FPU_REG(fpu_reg0).v, FPU_REG(fpu_reg1).v );
  FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_REG(fpu_reg1).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_TOP= (FPU_TOP+2)&0x7;
  
} // end fcompp




/*****************/
/* FCOS - Cosine */
/*****************/

// NOTA!! No comprove sNaN ni unsupported format
#define FPU_COS_CHECK_INVALID_OPERAND(IND,ACTION,INT_VAR)       \
  if ( FPU_REG(IND).tag == IA32_CPU_FPU_TAG_SPECIAL )           \
    {                                                           \
      (INT_VAR)= fpclassify ( FPU_REG(IND).v );                 \
      if ( (INT_VAR) == FP_SUBNORMAL )                          \
        {                                                       \
          FPU_STATUS|= FPU_STATUS_DE;                           \
          if ( !(FPU_CONTROL&FPU_CONTROL_DM) )                  \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }             \
        }                                                       \
      else if ( (INT_VAR) == FP_INFINITE )                      \
        {                                                       \
          FPU_STATUS|= FPU_STATUS_IE;                           \
          if ( !(FPU_CONTROL&FPU_CONTROL_IM) )                  \
            { FPU_STATUS|= FPU_STATUS_ES; ACTION; }             \
        }                                                       \
    }

static void
fcos (
      PROTO_INTERP
      )
{

  static const long double MAX= 9223372036854775807.0;
  static const long double MIN= -9223372036854775807.0;

  int fpu_reg,ftype;
  bool i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;

  // Intenta realitzar operació
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg,return,ftype);
  // --> Operació
  if ( FPU_REG(fpu_reg).v >= MIN && FPU_REG(fpu_reg).v <= MAX )
    {
      FPU_STATUS&= ~FPU_STATUS_C2;
#pragma STDC FENV_ACCESS on
      feclearexcept ( FE_INEXACT );
      fesetround ( fpu_get_round_mode ( INTERP ) );
      FPU_REG(fpu_reg).v= cosl ( FPU_REG(fpu_reg).v );
      i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
      // --> Actualitza etiqueta.
      fpu_update_tag ( INTERP, fpu_reg );
      // --> Inexact result
      FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
    }
  else FPU_STATUS|= FPU_STATUS_C2;
  
} // end fcos




/*****************************/
/* FDIV/FDIVP/FIDIV - Divide */
/*****************************/

static int
fdiv_common (
             PROTO_INTERP,
             const int         fpu_reg,
             const long double src
             )
{
  
  bool o_exc,u_exc,i_exc,ia_exc;


  // --> Divide-by-zero (prèvia a operació)
  if ( src == 0.0 || src == -0.0 )
    {
      FPU_STATUS|= FPU_STATUS_ZE;
      if ( !(FPU_CONTROL&FPU_CONTROL_ZM) )
        { FPU_STATUS|= FPU_STATUS_ES; return -1; }
    }
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v/= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return -1);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return -1);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return -1);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return -1);
  
  return 0;
  
} // end fdiv_common


static void
fdiv_float (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return ); }
  else val.u32= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  
  // Operació
  if ( fdiv_common ( INTERP, fpu_reg, (long double) val.f ) != 0 )
    return;
  
} // end fdiv_float


static void
fdiv_double (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fdiv_double - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  
  // Operació
  if ( fdiv_common ( INTERP, fpu_reg, (long double) val.d ) != 0 )
    return;
  
} // end fdiv_double


static void
fdiv80_i_0 (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{

  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fdiv_common ( INTERP, fpu_regi, FPU_REG(fpu_reg0).v ) != 0 )
    return;
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fdiv80_i_0


static void
fdiv_0_i (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fdiv_common ( INTERP, fpu_reg0, FPU_REG(fpu_regi).v ) != 0 )
    return;
  
} // end fdiv_0_i


static void
fdivp_i_0 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  fdiv80_i_0 ( INTERP, modrmbyte, true );
} // end fdivp_i_0




/****************************************/
/* FDIVR/FDIVRP/FIDIVR - Reverse Divide */
/****************************************/

static int
fdivr_common (
              PROTO_INTERP,
              const int         fpu_reg,
              const long double src
              )
{
  
  bool o_exc,u_exc,i_exc,ia_exc;


  // --> Divide-by-zero (prèvia a operació)
  if ( FPU_REG(fpu_reg).tag == IA32_CPU_FPU_TAG_ZERO )
    {
      FPU_STATUS|= FPU_STATUS_ZE;
      if ( !(FPU_CONTROL&FPU_CONTROL_ZM) )
        { FPU_STATUS|= FPU_STATUS_ES; return -1; }
    }
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v= src / FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return -1);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return -1);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return -1);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return -1);
 
  return 0;
  
} // end fdivr_common


static void
fdivr_float (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return ); }
  else val.u32= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  FPU_CHECK_SNAN_VALUE_U32(val.u32,return);
  // NOTA!! Faltaria el unsupported format
  
  // Operació
  if ( fdivr_common ( INTERP, fpu_reg, (long double) val.f ) != 0 )
    return;
  
} // end fdivr_float


static void
fdivr_double (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fdivr_double - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  FPU_CHECK_SNAN_VALUE_U64(val.u64,return);
  // NOTA!! Faltaria el unsupported format
  
  // Operació
  if ( fdivr_common ( INTERP, fpu_reg, (long double) val.d ) != 0 )
    return;
  
} // end fdivr_double


static void
fdivr80_i_0 (
             PROTO_INTERP,
             const uint8_t modrmbyte,
             const bool    pop
             )
{

  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fdivr_common ( INTERP, fpu_regi, FPU_REG(fpu_reg0).v ) != 0 )
    return;
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fdivr80_i_0


static void
fdivr_0_i (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fdivr_common ( INTERP, fpu_reg0, FPU_REG(fpu_regi).v ) != 0 )
    return;
  
} // end fdivr_0_i


static void
fdivr_i_0 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  fdivr80_i_0 ( INTERP, modrmbyte, false );
} // end fdivr_i_0


static void
fdivrp_i_0 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  fdivr80_i_0 ( INTERP, modrmbyte, true );
} // end fdivrp_i_0




/****************************************/
/* FFREE - Free Floating-Point Register */
/****************************************/

static void
ffree (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{

  int fpu_reg;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);

  // Operació. NOTA!! No es comprova si estava o no actiu.
  fpu_reg= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
  
} // end ffree




/***********************/
/* FILD - Load Integer */
/***********************/

static void
fild_int16 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  eaddr_r16_t eaddr;
  int fpu_reg;
  uint16_t val;


  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return ); }
  else val= *(eaddr.v.reg);

  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg).v= (long double) ((int16_t) val);
  if ( val == 0 ) FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  else            FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fild_int16


static void
fild_int32 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  eaddr_r32_t eaddr;
  int fpu_reg;
  uint32_t val;


  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return ); }
  else val= *(eaddr.v.reg);

  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg).v= (long double) ((int32_t) val);
  if ( val == 0 ) FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  else            FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fild_int32


static void
fild_int64 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;


  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] - fild_int64 !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg).v= (long double) ((int64_t) val.u64);
  if ( val.u64 == 0 ) FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  else                FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fild_int64




/*************************************************/
/* FINIT/FNINIT - Initialize Floating-Point Unit */
/*************************************************/

static void
finit (
       PROTO_INTERP
       )
{

  int i;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  
  FPU_CONTROL= 0x37F; // PC=0x3 IM|DM|ZM|OM|UM
  FPU_STATUS= 0;
  FPU_TOP= 0;
  for ( i= 0; i < 8; ++i )
    FPU_REG(i).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_DPTR.offset= 0; FPU_DPTR.selector= 0;
  FPU_IPTR.offset= 0; FPU_IPTR.selector= 0;
  FPU_OPCODE= 0;
  
} // end finit




/******************************/
/* FIST/FISTP - Store Integer */
/******************************/

static void
fist_int16 (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{
  
  eaddr_r16_t eaddr;
  int fpu_reg;
  uint16_t val;
  bool i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  
  // Intenta realitzar operació.
  fpu_reg= FPU_TOP; // ST(0)
  FPU_STATUS&= ~FPU_STATUS_C1; // Si tot va bé
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  val= (uint16_t) ((int16_t) rintl ( FPU_REG(fpu_reg).v ));
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  
  // Escriu valor en memòria.
  if ( eaddr.is_addr )
    { WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return ); }
  else *(eaddr.v.reg)= val;
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  
  // Pop
  if ( pop )
    {
      FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fist_int16


static void
fist_int32 (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  uint32_t val;
  bool i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;

  // Intenta realitzar operació.
  fpu_reg= FPU_TOP; // ST(0)
  FPU_STATUS&= ~FPU_STATUS_C1; // Si tot va bé
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  val= (uint32_t) ((int32_t) rintl ( FPU_REG(fpu_reg).v ));
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  
  // Escriu valor en memòria.
  if ( eaddr.is_addr )
    { WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return ); }
  else *(eaddr.v.reg)= val;
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  
  // Pop
  if ( pop )
    {
      FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fist_int32


static void
fist_int64 (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  bool i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;

  // Intenta realitzar operació.
  fpu_reg= FPU_TOP; // ST(0)
  FPU_STATUS&= ~FPU_STATUS_C1; // Si tot va bé
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  val.u64= (uint64_t) ((int64_t) rintl ( FPU_REG(fpu_reg).v ));
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  
  // Escriu valor en memòria.
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fistp_int64 - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val.u32.l, return );
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+4, val.u32.h, return );
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  
  // Pop
  if ( pop )
    {
      FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fist_int64




/***********************************/
/* FLD - Load Floating Point Value */
/***********************************/

static void
fld_float (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return ); }
  else val.u32= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  // --> Invalid arithmetic operand (source SNaN)
  if ( FPU_CHECKU32_SNAN(val.u32) )
    {
      FPU_STATUS|= FPU_STATUS_IE;
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )
        { FPU_STATUS|= FPU_STATUS_ES; return; }
      else val.u32= FPU_U32_SNAN2QNAN ( val.u32 ); // SNaN -> QNan
    }
  // --> Denormal operand
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg).v= (long double) val.f;
  if ( FPU_CHECKU32_ZERO(val.u32) )
    FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  // El valor denormal es normalitza
  else if ( FPU_CHECKU32_INF(val.u32) || FPU_CHECKU32_NAN(val.u32) )
    FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
  else FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fld_float


static void
fld_double (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr,
                "[CAL_IMPLEMENTAR] fld_double  - load from register\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  // --> Invalid arithmetic operand (source SNaN)
  if ( FPU_CHECKU64_SNAN(val.u64) )
    {
      FPU_STATUS|= FPU_STATUS_IE;
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )
        { FPU_STATUS|= FPU_STATUS_ES; return; }
      else val.u64= FPU_U64_SNAN2QNAN ( val.u64 ); // SNaN -> QNan
    }
  // --> Denormal operand
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg).v= (long double) val.d;
  if ( FPU_CHECKU64_ZERO(val.u64) )
    FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  // El valor denormal es normalitza
  else if ( FPU_CHECKU64_INF(val.u64) || FPU_CHECKU64_NAN(val.u64) )
    FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
  else FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fld_double


static void
fld_ldouble (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  ldouble_u_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr,
                "[CAL_IMPLEMENTAR] fld_ldouble  - load from register\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.v0, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.v1, return );
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+8, &val.u16.v4, return );
  val.u16.v5= 0;
  val.u32.v3= 0;
  
  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // ST(0)
  // --> Stack overflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  // --> Invalid arithmetic operand (source SNaN)
  if ( FPU_CHECKU80_SNAN(val.u64.v0,val.u16.v4) )
    {
      FPU_STATUS|= FPU_STATUS_IE;
      if ( !(FPU_CONTROL&FPU_CONTROL_IM) )
        { FPU_STATUS|= FPU_STATUS_ES; return; }
      else val.u64.v0= FPU_U80_SNAN2QNAN ( val.u64.v0 ); // SNaN -> QNan
    }
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg).v= val.ld;
  if ( FPU_CHECKU80_ZERO(val.u64.v0,val.u16.v4) )
    FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_ZERO;
  // El valor denormal es normalitza
  else if ( FPU_CHECKU80_INF(val.u64.v0,val.u16.v4) ||
            FPU_CHECKU80_NAN(val.u64.v0,val.u16.v4) )
    FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_SPECIAL;
  else FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_VALID;
  
} // end fld_ldouble


static void
fld_reg (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{

  int fpu_reg,fpu_reg_src;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // nou ST(0)
  fpu_reg_src= (FPU_TOP+(modrmbyte&0x7))&0x7;
  // --> Stack overflow/underflow
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg_src,return);
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg)= FPU_REG(fpu_reg_src);
  
} // end fld_reg




/***************************************************************/
/* FLD1/FLDL2T/FLDL2E/FLDPI/FLDLG2/FLDLN2/FLDZ - Load Constant */
/***************************************************************/

static void
fld_val (
         PROTO_INTERP,
         const long double val,
         const int         tag
         )
{

  int fpu_reg;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions
  fpu_reg= (FPU_TOP-1)&0x7; // nou ST(0)
  FPU_CHECK_STACK_OVERFLOW(fpu_reg,return);
  
  // Assigna valor
  FPU_TOP= (FPU_TOP-1)&0x7;
  FPU_REG(fpu_reg).v= val;
  FPU_REG(fpu_reg).tag= tag;
  
} // end fld_val


static void
fld1 (
      PROTO_INTERP
      )
{
  fld_val ( INTERP, +1.0L, IA32_CPU_FPU_TAG_VALID );
} // end fld1


static void
fldl2e (
        PROTO_INTERP
        )
{
  fld_val ( INTERP, 1.4426950408889634074L, IA32_CPU_FPU_TAG_VALID );
} // end fldl2e


static void
fldln2 (
      PROTO_INTERP
      )
{
  fld_val ( INTERP, logl ( +2.0L ), IA32_CPU_FPU_TAG_VALID );
} // end fldln2


static void
fldz (
      PROTO_INTERP
      )
{
  fld_val ( INTERP, +0.0L, IA32_CPU_FPU_TAG_ZERO );
} // end fldz




/*************************************/
/* FLDCW - Load x87 FPU Control Word */
/*************************************/

static void
fldcw (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{

  eaddr_r16_t eaddr;
  uint16_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return ); }
  else val= *(eaddr.v.reg);
  FPU_CONTROL= val;
  fpu_update_exceptions ( INTERP );
  
} // end fldcw




/*******************************/
/* FMUL/FMULP/FIMUL - Multiply */
/*******************************/

static int
fmul_common (
             PROTO_INTERP,
             const int         fpu_reg,
             const long double src
             )
{
  
  bool o_exc,u_exc,i_exc,ia_exc;

  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v*= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return -1);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return -1);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return -1);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return -1);
  
  return 0;
  
} // end fmul_common


static void
fmul_float (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return ); }
  else val.u32= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  
  // Operació
  if ( fmul_common ( INTERP, fpu_reg, (long double) val.f ) != 0 )
    return;
  
} // end fmul_float


static void
fmul_double (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fmult_double - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  
  // Operació
  if ( fmul_common ( INTERP, fpu_reg, (long double) val.d ) != 0 )
    return;
  
} // end fmul_double


static void
fmul80_i_0 (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{

  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Preparació
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fmul_common ( INTERP, fpu_regi, FPU_REG(fpu_reg0).v ) != 0 )
    return;
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fmul80_i_0


static void
fmul_0_i (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Preparació
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fmul_common ( INTERP, fpu_reg0, FPU_REG(fpu_regi).v ) != 0 )
    return;
  
} // end fmul_0_i


static void
fmul_i_0 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  fmul80_i_0 ( INTERP, modrmbyte, false );
} // end fmul_i_0


static void
fmulp_i_0 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  fmul80_i_0 ( INTERP, modrmbyte, true );
} // end fmulp_i_0


static void
fmul_int32 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  eaddr_r32_t eaddr;
  int fpu_reg;
  uint32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return ); }
  else val= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  
  // Operació
  if ( fmul_common ( INTERP, fpu_reg, (long double) ((int32_t) val) ) != 0 )
    return;
  
} // end fmul_int32




/*******************************/
/* FPATAN - Partial Arctangent */
/*******************************/

static void
fpatan (
       PROTO_INTERP
       )
{

  int fpu_reg0,fpu_reg1,ftype;
  bool i_exc,u_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg1= (FPU_TOP+1)&0x7;
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg1,return);
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg0,return,ftype); // Me val !
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg1,return,ftype); // Me val !
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT|FE_UNDERFLOW );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg1).v= atan2l ( FPU_REG(fpu_reg1).v, FPU_REG(fpu_reg0).v  );
  i_exc= fetestexcept ( FE_INEXACT );
  u_exc= fetestexcept ( FE_UNDERFLOW );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg1 );
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
  
  // Pop.
  FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_TOP= (FPU_TOP+1)&0x7;
  
} // end fpatan




/*****************************/
/* FPREM - Partial Remainder */
/*****************************/

static void
fprem (
       PROTO_INTERP
       )
{
  
  int fpu_reg0,fpu_reg1,ftype;
  bool ia_exc,u_exc;
  ldouble_u_t aux;
  uint16_t exp0,exp1;
  uint64_t q;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg1= (FPU_TOP+1)&0x7;
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg1,return);
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg0,return,ftype); // Me val !
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg1,return,ftype); // Me val !
  // --> Operació
  // NOTA!!! Farem l'operació gastant la funció matemàtica, però per
  // al tema dels flags farem alguns càlculs a partir dels exponents.
  // --> Exponents (C0,C3,C1,C2)
  aux.ld= FPU_REG(fpu_reg0).v;
  exp0= GET_EXPONENT_LDOUBLE_U(aux);
  aux.ld= FPU_REG(fpu_reg1).v;
  exp1= GET_EXPONENT_LDOUBLE_U(aux);
  if ( (((int) exp0) - ((int) exp1)) < 64 )
    {
      FPU_STATUS&= ~(FPU_STATUS_C0|FPU_STATUS_C1|FPU_STATUS_C2|FPU_STATUS_C3);
      fesetround ( FE_TOWARDZERO );
      q= (uint64_t) rintl ( FPU_REG(fpu_reg0).v / FPU_REG(fpu_reg1).v );
      if ( (q&0x1)!=0  ) FPU_STATUS|= FPU_STATUS_C1;
      if ( (q&0x2)!=0  ) FPU_STATUS|= FPU_STATUS_C3;
      if ( (q&0x4)!=0  ) FPU_STATUS|= FPU_STATUS_C0;
    }
  else { FPU_STATUS|= FPU_STATUS_C2; }
  // --> Operació real.
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INVALID|FE_UNDERFLOW );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  // NOTA!! FPREM1 seria remainder
  FPU_REG(fpu_reg0).v= fmodl ( FPU_REG(fpu_reg0).v, FPU_REG(fpu_reg1).v );
  ia_exc= fetestexcept ( FE_INEXACT );
  u_exc= fetestexcept ( FE_UNDERFLOW );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg0 );
  // --> Inexact result
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
  
} // end fprem




/***************************/
/* FPTAN - Partial Tangent */
/***************************/

static void
fptan (
       PROTO_INTERP
       )
{

  static const long double MAX= 9223372036854775807.0;
  static const long double MIN= -9223372036854775807.0;
  
  int fpu_reg,ftype,fpu_reg_new;
  bool i_exc,u_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg,return,ftype); // Me val !
  // --> Operació
  if ( FPU_REG(fpu_reg).v >= MIN && FPU_REG(fpu_reg).v <= MAX )
    {
      
      // Comprova si es pot ficar el 1.0
      fpu_reg_new= (FPU_TOP-1)&0x7; // Nou ST(0)
      FPU_CHECK_STACK_OVERFLOW(fpu_reg_new,return);
      
      // Calcula tangent
      FPU_STATUS&= ~FPU_STATUS_C2;
#pragma STDC FENV_ACCESS on
      feclearexcept ( FE_INEXACT|FE_UNDERFLOW );
      fesetround ( fpu_get_round_mode ( INTERP ) );
      FPU_REG(fpu_reg).v= tanl ( FPU_REG(fpu_reg).v );
      i_exc= fetestexcept ( FE_INEXACT );
      u_exc= fetestexcept ( FE_UNDERFLOW );
#pragma STDC FENV_ACCESS off
      // --> Actualitza etiqueta.
      fpu_update_tag ( INTERP, fpu_reg );
      // --> Inexact result
      FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
      // --> Numeric underflow
      FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
      
      // Assigna valor 1.0
      FPU_TOP= fpu_reg_new;
      FPU_REG(fpu_reg_new).v= 1.0L;
      FPU_REG(fpu_reg_new).tag= IA32_CPU_FPU_TAG_VALID;
      
    }
  else FPU_STATUS|= FPU_STATUS_C2;
  
} // end fptan




/******************************/
/* FRNDINT - Round to Integer */
/******************************/

static void
frndint (
         PROTO_INTERP
         )
{

  int fpu_reg;
  bool i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;

  // Intenta realitzar operació
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_REG(fpu_reg,return);
  // NOTA!! Faltaria SNAN i unsupported que no sé com fer-ho
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v= rintl ( FPU_REG(fpu_reg).v );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  
} // end frndint




/**********************************/
/* FRSTOR - Restore x87 FPU State */
/**********************************/

// Llig els registres.
static void
frstor_common (
               PROTO_INTERP,
               IA32_SegmentRegister *seg,
               uint32_t              off
               )
{

  int i;
  ldouble_u_t val;
  
  
  val.u16.v5= 0;
  val.u32.v3= 0;
  for ( i= 0; i < 8; i++ )
    {
      READD_DATA ( seg, off+i*10, &val.u32.v0, return );
      READD_DATA ( seg, off+i*10+4, &val.u32.v1, return );
      READW_DATA ( seg, off+i*10+8, &val.u16.v4, return );
      FPU_REG(i).v= val.ld;
    }
  
} // end frstor_common


static void
frstor32 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  eaddr_r32_t eaddr;
  uint32_t tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  
  // Obté adreça.
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr ) { EXCEPTION0 ( EXCP_GP ); return; }
  
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( EFLAGS&VM_FLAG )
        {
          // EN REALITAT NO EM QUEDA CLAR SI AÇÒ HA DE SER COM EL
          // PROTECTED O COM EL REAL.
          printf("[CAL IMPLEMENTAR] frstor32 - protected mode VM!!!\n");
          exit(EXIT_FAILURE);
        }
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &tmp, return );
      FPU_CONTROL= (uint16_t) tmp;
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &tmp, return );
      FPU_STATUS= (uint16_t) tmp;
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+8, &tmp, return );
      fpu_set_tag_word ( INTERP, (uint16_t) tmp );
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+12,
                   &(FPU_IPTR.offset), return );
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+16, &tmp, return );
      FPU_IPTR.selector= (uint16_t) tmp;
      FPU_OPCODE= (uint16_t) ((tmp>>16)&0x07FF);
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+20,
                   &(FPU_DPTR.offset), return );
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+24, &tmp, return );
      FPU_DPTR.selector= (uint16_t) tmp;
      frstor_common ( INTERP, eaddr.v.addr.seg, eaddr.v.addr.off+28 );
    }
  else
    {
      printf("[CAL IMPLEMENTAR] frstor32 - real mode!!!\n");
      exit(EXIT_FAILURE);
    }
  
} // end frstor32


static void
frstor16 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  eaddr_r16_t eaddr;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  
  // Obté adreça.
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr ) { EXCEPTION0 ( EXCP_GP ); return; }
  
  // Executa.
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( EFLAGS&VM_FLAG )
        {
          // EN REALITAT NO EM QUEDA CLAR SI AÇÒ HA DE SER COM EL
          // PROTECTED O COM EL REAL.
          printf("[CAL IMPLEMENTAR] frstor16 - protected mode VM!!!\n");
          exit(EXIT_FAILURE);
        }
      printf("[CAL IMPLEMENTAR] frstor16 - protected mode!!!\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      printf("[CAL IMPLEMENTAR] frstor16 - real mode!!!\n");
      exit(EXIT_FAILURE);
    }
  
} // end frstor16


static void
frstor (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{
  OPERAND_SIZE_IS_32 ?
    frstor32 ( INTERP, modrmbyte ) : frstor16 ( INTERP, modrmbyte );
} // end frstor




/**************************************/
/* FSAVE/FNSAVE - Store x87 FPU State */
/**************************************/

// Desa els registres i reinicialitza.
static void
fsave_common (
              PROTO_INTERP,
              IA32_SegmentRegister *seg,
              uint32_t              off
              )
{

  int i;
  ldouble_u_t val;
  

  // Desa registres
  for ( i= 0; i < 8; i++ )
    {
      val.ld= FPU_REG(i).v;
      WRITED ( seg, off+i*10, val.u32.v0, return );
      WRITED ( seg, off+i*10+4, val.u32.v1, return );
      WRITEW ( seg, off+i*10+8, val.u16.v4, return );
    }
  
  // Reinicialitza.
  FPU_CONTROL= 0x37F; // PC=0x3 IM|DM|ZM|OM|UM
  FPU_STATUS= 0;
  FPU_TOP= 0;
  for ( i= 0; i < 8; ++i )
    FPU_REG(i).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_DPTR.offset= 0; FPU_DPTR.selector= 0;
  FPU_IPTR.offset= 0; FPU_IPTR.selector= 0;
  //FPU_OPCODE= 0; ????? Pareix que l'opcode no es modifica.
  
} // end fsave_common


static void
fsave32 (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{

  eaddr_r32_t eaddr;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }

  // Obté adreça.
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr ) { EXCEPTION0 ( EXCP_GP ); return; }
  
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( EFLAGS&VM_FLAG )
        {
          // EN REALITAT NO EM QUEDA CLAR SI AÇÒ HA DE SER COM EL
          // PROTECTED O COM EL REAL.
          printf("[CAL IMPLEMENTAR] fsave32 - protected mode VM!!!\n");
          exit(EXIT_FAILURE);
        }
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off,
               (uint32_t) FPU_CONTROL, return );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+4,
               (uint32_t) FPU_STATUS, return );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+8,
               (uint32_t) fpu_get_tag_word ( INTERP ), return );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+12,
               FPU_IPTR.offset, return );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+16,
               ((uint32_t) FPU_IPTR.selector) |
               (((uint32_t) (FPU_OPCODE&0x07FF))<<16),
               return );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+20,
               FPU_DPTR.offset, return );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+24,
               (uint32_t) FPU_DPTR.selector, return );
      fsave_common ( INTERP, eaddr.v.addr.seg, eaddr.v.addr.off+28 );
    }
  else
    {
      printf("[CAL IMPLEMENTAR] fsave32 - real mode!!!\n");
      exit(EXIT_FAILURE);
    }
  
} // end fsave32


static void
fsave16 (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  
  eaddr_r16_t eaddr;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }

  // Obté adreça.
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr ) { EXCEPTION0 ( EXCP_GP ); return; }
  
  // Executa.
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( EFLAGS&VM_FLAG )
        {
          // EN REALITAT NO EM QUEDA CLAR SI AÇÒ HA DE SER COM EL
          // PROTECTED O COM EL REAL.
          printf("[CAL IMPLEMENTAR] fsave16 - protected mode VM!!!\n");
          exit(EXIT_FAILURE);
        }
      printf("[CAL IMPLEMENTAR] fsave16 - protected mode!!!\n");
      exit(EXIT_FAILURE);
    }
  else
    {
      printf("[CAL IMPLEMENTAR] fsave16 - real mode!!!\n");
      exit(EXIT_FAILURE);
    }
  
} // end fsave16


static void
fsave (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{
  OPERAND_SIZE_IS_32 ?
    fsave32 ( INTERP, modrmbyte ) : fsave16 ( INTERP, modrmbyte );
} // end fsave




/******************/
/* FSCALE - Scale */
/******************/

static void
fscale (
        PROTO_INTERP
        )
{
  
  int fpu_reg0,fpu_reg1,ftype;
  bool i_exc,u_exc,o_exc;
  long double tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg1= (FPU_TOP+1)&0x7; // ST(1)
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg1,return);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg0,return,ftype); // Me val !
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg1,return,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT );
  fesetround ( FE_TOWARDZERO );
  tmp= rintl ( FPU_REG(fpu_reg1).v );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg0).v*= powl ( 2.0L, tmp );
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg0 );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  
} // end fscale




/*****************/
/* FSIN - Sine   */
/*****************/

static void
fsin (
      PROTO_INTERP
      )
{

  static const long double MAX= 9223372036854775807.0;
  static const long double MIN= -9223372036854775807.0;
  
  int fpu_reg,ftype;
  bool i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_COS_CHECK_INVALID_OPERAND(fpu_reg,return,ftype);
  // --> Operació
  if ( FPU_REG(fpu_reg).v >= MIN && FPU_REG(fpu_reg).v <= MAX )
    {
      FPU_STATUS&= ~FPU_STATUS_C2;
#pragma STDC FENV_ACCESS on
      feclearexcept ( FE_INEXACT );
      fesetround ( fpu_get_round_mode ( INTERP ) );
      FPU_REG(fpu_reg).v= sinl ( FPU_REG(fpu_reg).v );
      i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
      // --> Actualitza etiqueta.
      fpu_update_tag ( INTERP, fpu_reg );
      // --> Inexact result
      FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
    }
  else FPU_STATUS|= FPU_STATUS_C2;
  
} // end fsin




/***********************/
/* FSQRT - Square Root */
/***********************/

static void
fsqrt (
       PROTO_INTERP
       )
{
  
  int fpu_reg,ftype;
  bool i_exc,ia_exc;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v= sqrtl ( FPU_REG(fpu_reg).v );
  ia_exc= fetestexcept ( FE_INVALID );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return);
  
} // end fsqrt




/*****************************************/
/* FST/FSTP - Store Floating Point Value */
/*****************************************/

static void
fst_float (
           PROTO_INTERP,
           const uint8_t modrmbyte,
           const bool    pop
           )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;
  bool o_exc,u_exc,i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  
  // Intenta realitzar operació.
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  val.f= (float) FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
  
  // Escriu valor en memòria.
  if ( eaddr.is_addr )
    { WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val.u32, return ); }
  else *(eaddr.v.reg)= val.u32;

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
      if ( o_exc || u_exc ) FPU_STATUS&= ~FPU_STATUS_C1;
      else
        {
          FPU_STATUS|= FPU_STATUS_PE;
          if ( !(FPU_CONTROL&FPU_CONTROL_PM) )
            { FPU_STATUS|= FPU_STATUS_ES; return; }
        }
    }
  
  // Pop
  if ( pop )
    {
      FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fst_float


static void
fst_double (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  bool o_exc,u_exc,i_exc;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  
  // Intenta realitzar operació.
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  // --> Operació
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  val.d= (double) FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
#pragma STDC FENV_ACCESS off
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
  
  // Escriu valor en memòria.
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr,
                "[CAL_IMPLEMENTAR] fst_double - eaddr.is_addr"
                " not implemented" );
      exit ( EXIT_FAILURE );
    }
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val.u32.l, return );
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+4, val.u32.h, return );
  
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
      if ( o_exc || u_exc ) FPU_STATUS&= ~FPU_STATUS_C1;
      else
        {
          FPU_STATUS|= FPU_STATUS_PE;
          if ( !(FPU_CONTROL&FPU_CONTROL_PM) )
            { FPU_STATUS|= FPU_STATUS_ES; return; }
        }
    }
  
  // Pop
  if ( pop )
    {
      FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fst_double


static void
fstp_ldouble (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  ldouble_u_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  
  // Intenta realitzar operació.
  fpu_reg= FPU_TOP; // ST(0)
  // --> Stack underflow
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  // --> Operació
  val.ld= FPU_REG(fpu_reg).v;
  
  // Escriu valor en memòria.
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr,
                "[CAL_IMPLEMENTAR] fst_ldouble - eaddr.is_addr"
                " not implemented" );
      exit ( EXIT_FAILURE );
    }
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val.u32.v0, return );
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+4, val.u32.v1, return );
  WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off+8, val.u16.v4, return );

  // Pop
  FPU_REG(fpu_reg).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_TOP= (FPU_TOP+1)&0x7;
  
} // end fstp_ldouble


static void
fst (
     PROTO_INTERP,
     const uint8_t modrmbyte,
     const bool    pop
     )
{

  // NOTA!!! FST ST(i) no comprova OVERFLOW i no apila. Simplement
  // copia en ST8i) el valor de ST(0) sense importar si estava buit o
  // no.
  
  int fpu_reg0,fpu_reg_dst;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  
  // Intenta realitzar operació.
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg_dst= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  // --> Operació
  FPU_REG(fpu_reg_dst)= FPU_REG(fpu_reg0);
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fst



/*********************************************/
/* FSTCW/FNSTCW - Store x87 FPU Control Word */
/*********************************************/

static void
fstcw (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{

  eaddr_r16_t eaddr;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, FPU_CONTROL, return ); }
  else *(eaddr.v.reg)= FPU_CONTROL;
  
} // end fstcw




/********************************************/
/* FSTSW/FNSTSW - Store x87 FPU Status Word */
/********************************************/

static void
fstsw (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{

  eaddr_r16_t eaddr;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, FPU_STATUS_TOP, return ); }
  else *(eaddr.v.reg)= FPU_STATUS_TOP;
  
} // end fstsw


static void
fnstsw_AX (
           PROTO_INTERP
           )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  
  AX= FPU_STATUS_TOP;
  
} // end fnstsw_AX



/*******************************/
/* FSUB/FSUBP/FISUB - Subtract */
/*******************************/

static int
fsub_common (
             PROTO_INTERP,
             const int         fpu_reg,
             const long double src
             )
{
  
  bool o_exc,u_exc,i_exc,ia_exc;

  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v-= src;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return -1);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return -1);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return -1);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return -1);
  
  return 0;
  
} // end fsub_common


static void
fsub_float (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return ); }
  else val.u32= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  
  // Operació
  if ( fsub_common ( INTERP, fpu_reg, (long double) val.f ) != 0 )
    return;
  
} // end fsub_float


static void
fsub_double (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fsub_double - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  
  // Operació
  if ( fsub_common ( INTERP, fpu_reg, (long double) val.d ) != 0 )
    return;
  
} // end fsub_double


static void
fsub80_i_0 (
            PROTO_INTERP,
            const uint8_t modrmbyte,
            const bool    pop
            )
{
  
  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fsub_common ( INTERP, fpu_regi, FPU_REG(fpu_reg0).v ) != 0 )
    return;
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fsub80_i_0


static void
fsub_0_i (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fsub_common ( INTERP, fpu_reg0, FPU_REG(fpu_regi).v ) != 0 )
    return;
  
} // end fsub_0_i


static void
fsub_i_0 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  fsub80_i_0 ( INTERP, modrmbyte, false );
} // end fsub_i_0


static void
fsubp_i_0 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  fsub80_i_0 ( INTERP, modrmbyte, true );
} // end fsubp_i_0




/******************************************/
/* FSUBR/FSUBRP/FISUBR - Reverse Subtract */
/******************************************/

static int
fsubr_common (
              PROTO_INTERP,
              const int         fpu_reg,
              const long double src
              )
{
  
  bool o_exc,u_exc,i_exc,ia_exc;

  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg).v= src - FPU_REG(fpu_reg).v;
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return -1);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return -1);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return -1);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return -1);
  
  return 0;
  
} // end fsubr_common


static void
fsubr_float (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  float_u32_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    { READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32, return ); }
  else val.u32= *(eaddr.v.reg);
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U32(val.u32,return);
  
  // Operació
  if ( fsubr_common ( INTERP, fpu_reg, (long double) val.f ) != 0 )
    return;
  
} // end fsubr_float


static void
fsubr_double (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  eaddr_r32_t eaddr;
  int fpu_reg;
  double_u64_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Llig valor
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( !eaddr.is_addr )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] fsubr_double - !eaddr.is_addr\n" );
      exit ( EXIT_FAILURE );
    }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val.u32.l, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &val.u32.h, return );
  
  // Prepara i comprova excepcions prèvies
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_CHECK_DENORMAL_VALUE_U64(val.u64,return);
  
  // Operació
  if ( fsubr_common ( INTERP, fpu_reg, (long double) val.d ) != 0 )
    return;
  
} // end fsubr_double


static void
fsubr_0_i (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fsubr_common ( INTERP, fpu_reg0, FPU_REG(fpu_regi).v ) != 0 )
    return;
  
} // end fsubr_0_i


static void
fsubr80_i_0 (
             PROTO_INTERP,
             const uint8_t modrmbyte,
             const bool    pop
             )
{
  
  int fpu_reg0,fpu_regi;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Prepara i comprova excepcions prèvies
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_regi= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_regi,return);
  
  // Operació
  if ( fsubr_common ( INTERP, fpu_regi, FPU_REG(fpu_reg0).v ) != 0 )
    return;
  if ( pop )
    {
      FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
      FPU_TOP= (FPU_TOP+1)&0x7;
    }
  
} // end fsubr80_i_0



static void
fsubrp_i_0 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  fsubr80_i_0 ( INTERP, modrmbyte, true );
} // end fsubrp_i_0




/***************/
/* FTST - TEST */
/***************/

static void
ftst (
      PROTO_INTERP
      )
{

  int fpu_reg,ftype;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;

  // Prepara
  fpu_reg= FPU_TOP; // ST(0)
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg,return);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg,return,ftype);
  
  // Compara
  if ( FPU_REG(fpu_reg).tag == IA32_CPU_FPU_TAG_SPECIAL &&
       !isinf ( FPU_REG(fpu_reg).v ) )
    { FPU_STATUS|= (FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0); }
  else if ( FPU_REG(fpu_reg).v > 0.0L )
    { FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2|FPU_STATUS_C0); }
  else if ( FPU_REG(fpu_reg).v < 0.0L )
    {
      FPU_STATUS&= ~(FPU_STATUS_C3|FPU_STATUS_C2);
      FPU_STATUS|= FPU_STATUS_C0;
    }
  else if ( FPU_REG(fpu_reg).v == 0.0L )
    {
      FPU_STATUS&= ~(FPU_STATUS_C2|FPU_STATUS_C0);
      FPU_STATUS|= FPU_STATUS_C3;
    }
  else
    {
      printf ( "[EE] ftst WTF !!\n" );
      exit ( EXIT_FAILURE );
    }
  
} // end ftst




/*************************/
/* FXAM - Examine ModR/M */
/*************************/

static void
fxam (
      PROTO_INTERP
      )
{

  int fpu_reg;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  
  fpu_reg= FPU_TOP; // ST(0)
  switch ( FPU_REG(fpu_reg).tag )
    {
    case IA32_CPU_FPU_TAG_VALID:
      FPU_STATUS&= ~(FPU_STATUS_C0|FPU_STATUS_C3);
      FPU_STATUS|= FPU_STATUS_C2;
      break;
    case IA32_CPU_FPU_TAG_ZERO:
      FPU_STATUS&= ~(FPU_STATUS_C0|FPU_STATUS_C2);
      FPU_STATUS|= FPU_STATUS_C3;
      break;
    case IA32_CPU_FPU_TAG_SPECIAL:
      switch ( fpclassify ( FPU_REG(fpu_reg).v ) )
        {
        case FP_NAN:
          FPU_STATUS&= ~(FPU_STATUS_C2|FPU_STATUS_C3);
          FPU_STATUS|= FPU_STATUS_C0;
          break;
        case FP_INFINITE:
          FPU_STATUS&= ~FPU_STATUS_C3;
          FPU_STATUS|= (FPU_STATUS_C0|FPU_STATUS_C2);
          break;
        case FP_SUBNORMAL:
          FPU_STATUS&= ~FPU_STATUS_C0;
          FPU_STATUS|= (FPU_STATUS_C2|FPU_STATUS_C3);
          break;
        default:
          printf ( "[EE] fxam WTF (B)!!\n" );
          exit ( EXIT_FAILURE );
        }
      break;
    case IA32_CPU_FPU_TAG_EMPTY:
      FPU_STATUS&= ~FPU_STATUS_C2;
      FPU_STATUS|= (FPU_STATUS_C0|FPU_STATUS_C3);
      break;
    default:
      printf ( "[EE] fxam WTF!!\n" );
      exit ( EXIT_FAILURE );
    }
  
} // end fxam




/*************************************/
/* FXCH - Exchange Register Contents */
/*************************************/

static void
fxch (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{

  int fpu_reg0,fpu_reg_src;
  IA32_FPURegister tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);

  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg_src= (FPU_TOP+(modrmbyte&0x7))&0x7;
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg_src,return);

  tmp= FPU_REG(fpu_reg0);
  FPU_REG(fpu_reg0)= FPU_REG(fpu_reg_src);
  FPU_REG(fpu_reg_src)= tmp;
  
  FPU_STATUS&= ~FPU_STATUS_C1;
  
} // end fxch




/*****************************/
/* FYL2X - Compute y ∗ log2x */
/*****************************/

static void
fyl2x (
       PROTO_INTERP
       )
{
  
  int fpu_reg0,fpu_reg1,ftype;
  bool i_exc,u_exc,o_exc,ia_exc,dz_exc;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( CR0&(CR0_EM|CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  FPU_STATUS&= ~FPU_STATUS_C1;
  
  // Intenta realitzar operació
  fpu_reg0= FPU_TOP; // ST(0)
  fpu_reg1= (FPU_TOP+1)&0x7; // ST(1)
  // --> Stack underflow i altres
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg0,return);
  FPU_CHECK_STACK_UNDERFLOW(fpu_reg1,return);
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg0,return,ftype); // Me val !
  FPU_FCOM_CHECK_INVALID_OPERAND(fpu_reg1,return,ftype); // Me val !
  
#pragma STDC FENV_ACCESS on
  feclearexcept ( FE_DIVBYZERO|FE_OVERFLOW|FE_UNDERFLOW|FE_INEXACT|FE_INVALID );
  fesetround ( fpu_get_round_mode ( INTERP ) );
  FPU_REG(fpu_reg1).v*= log2l ( FPU_REG(fpu_reg0).v );
  o_exc= fetestexcept ( FE_OVERFLOW );
  u_exc= fetestexcept ( FE_UNDERFLOW );
  i_exc= fetestexcept ( FE_INEXACT );
  ia_exc= fetestexcept ( FE_INVALID );
  dz_exc= fetestexcept ( FE_DIVBYZERO );
#pragma STDC FENV_ACCESS off
  // --> Actualitza etiqueta.
  fpu_update_tag ( INTERP, fpu_reg1 );
  // --> Numeric overflow
  FPU_CHECK_NUMERIC_OVERFLOW_BOOL(o_exc,return);
  // --> Numeric underflow
  FPU_CHECK_NUMERIC_UNDERFLOW_BOOL(u_exc,return);
  // --> Invalid arithmetic operand
  FPU_CHECK_INVALID_OPERATION_BOOL(ia_exc,return);
  // --> Inexact result
  FPU_CHECK_INEXACT_RESULT_BOOL(i_exc,return);
  // --> Divide by zero
  FPU_CHECK_DIVBYZERO_RESULT_BOOL(dz_exc,return);

  // Pop.
  FPU_REG(fpu_reg0).tag= IA32_CPU_FPU_TAG_EMPTY;
  FPU_TOP= (FPU_TOP+1)&0x7;
  
} // end fyl2x




/*********************/
/* WAIT/FWAIT - Wait */
/*********************/

static void
fwait (
       PROTO_INTERP
       )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( (CR0&CR0_MP) && (CR0&CR0_TS) ) { EXCEPTION ( EXCP_NM ); return; }
  FPU_CHECK_EXCEPTIONS(return);
  
} // end fwait
