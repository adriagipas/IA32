/*
 * Copyright 2020-2025 Adrià Giménez Pastor.
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
 *  interpreter_add.h - Implementa ADD i semblants
 *
 */


/************************/
/* ADC - Add with Carry */
/************************/

#define SET_AD_FLAGS(RES,OP1,OP2,MBIT)        				\
  EFLAGS&= ~(OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|CF_FLAG|PF_FLAG);        	\
  EFLAGS|=        							\
    ((((~((OP1)^(OP2)))&((OP1)^(RES))&(MBIT)) ? OF_FLAG : 0) |        	\
    (((RES)&(MBIT)) ? SF_FLAG : 0) |        				\
    (((RES)==0) ? ZF_FLAG : 0) |        				\
    (((((OP1)^(OP2))^(RES))&0x10) ? AF_FLAG : 0) |        		\
    (((((OP1)&(OP2)) | (((OP1)|(OP2))&(~(RES))))&(MBIT)) ? CF_FLAG : 0) | \
    PFLAG[(uint8_t) ((RES)&0xFF)])

#define SETU8_AD_FLAGS(RES,OP1,OP2)        	\
  SET_AD_FLAGS(RES,OP1,OP2,0x80)

#define SETU16_AD_FLAGS(RES,OP1,OP2)        	\
  SET_AD_FLAGS(RES,OP1,OP2,0x8000)

#define SETU32_AD_FLAGS(RES,OP1,OP2)        	\
  SET_AD_FLAGS(RES,OP1,OP2,0x80000000)

#define ADC8(DST,SRC,VARU8)        					\
  VARU8= (DST);        							\
  (DST)= VARU8 + (SRC) + ((EFLAGS&CF_FLAG)!=0);        			\
  SETU8_AD_FLAGS ( (DST), VARU8, (SRC) )

#define ADC16(DST,SRC,VARU16)        					\
  VARU16= (DST);        						\
  (DST)= VARU16 + (SRC) + ((EFLAGS&CF_FLAG)!=0);        		\
  SETU16_AD_FLAGS ( (DST), VARU16, (SRC) )

#define ADC32(DST,SRC,VARU32)        					\
  VARU32= (DST);        						\
  (DST)= VARU32 + (SRC) + ((EFLAGS&CF_FLAG)!=0);        		\
  SETU32_AD_FLAGS ( (DST), VARU32, (SRC) )

#define ADC_RM8_VAL8(DST,VAL,TMP,EADDR)        				\
  if ( (EADDR).is_addr )        					\
    {        								\
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ADC8 ( (DST), (VAL), (TMP) );        				\
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      ADC8 ( *((EADDR).v.reg), (VAL), (TMP) );        			\
    }

#define ADC_RM16_VAL16(DST,VAL,TMP,EADDR)        			\
  if ( (EADDR).is_addr )        					\
    {        								\
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ADC16 ( (DST), (VAL), (TMP) );        				\
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      ADC16 ( *((EADDR).v.reg), (VAL), (TMP) );        			\
    }

#define ADC_RM32_VAL32(DST,VAL,TMP,EADDR)        			\
  if ( (EADDR).is_addr )        					\
    {        								\
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ADC32 ( (DST), (VAL), (TMP) );        				\
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      ADC32 ( *((EADDR).v.reg), (VAL), (TMP) );        			\
    }


static void
adc_AL_imm8 (
             PROTO_INTERP
             )
{

  uint8_t tmp, val;

  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ADC8 ( AL, val, tmp );
  
} // end adc_AL_imm8


static void
adc_AX_imm16 (
              PROTO_INTERP
              )
{

  uint16_t tmp, val;

  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ADC16 ( AX, val, tmp );
  
} // end adc_AX_imm16


static void
adc_EAX_imm32 (
               PROTO_INTERP
               )
{

  uint32_t tmp, val;
  
  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ADC32 ( EAX, val, tmp );
  
} // end adc_EAX_imm32


static void
adc_A_imm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? adc_EAX_imm32 ( INTERP ) : adc_AX_imm16 ( INTERP );
} // end adc_A_imm


static void
adc_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t tmp, val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &val, return );
  ADC_RM8_VAL8 ( dst, val, tmp, eaddr );
  
} // end adc_rm8_imm8


static void
adc_rm16_imm16 (
        	PROTO_INTERP,
        	const uint8_t modrmbyte
        	)
{

  uint16_t tmp, val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  ADC_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end adc_rm16_imm16


static void
adc_rm32_imm32 (
        	PROTO_INTERP,
        	const uint8_t modrmbyte
        	)
{

  uint32_t tmp, val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  ADC_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end adc_rm32_imm32


static void
adc_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    adc_rm32_imm32 ( INTERP, modrmbyte ) :
    adc_rm16_imm16 ( INTERP, modrmbyte );
} // end adc_rm_imm


static void
adc_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint16_t tmp, val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  ADC_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end adc_rm16_imm8


static void
adc_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint32_t tmp, val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  ADC_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end adc_rm32_imm8


static void
adc_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    adc_rm32_imm8 ( INTERP, modrmbyte ) :
    adc_rm16_imm8 ( INTERP, modrmbyte );
} // end adc_rm_imm8


static void
adc_rm8_r8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte, tmp, dst, *reg;
  eaddr_r8_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  ADC_RM8_VAL8 ( dst, *reg, tmp, eaddr );
  
} // end adc_rm8_r8


static void
adc_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp, dst, *reg;
  eaddr_r16_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  ADC_RM16_VAL16 ( dst, *reg, tmp, eaddr );
  
} // end adc_rm16_r16


static void
adc_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp, dst, *reg;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  ADC_RM32_VAL32 ( dst, *reg, tmp, eaddr );
  
} // end adc_rm32_r32


static void
adc_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? adc_rm32_r32 ( INTERP ) : adc_rm16_r16 ( INTERP );
} // end adc_rm_r


static void
adc_r8_rm8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte, tmp, val, *reg;
  eaddr_r8_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  ADC8 ( *reg, val, tmp );
  
} // end adc_r8_rm8


static void
adc_r16_rm16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp, val, *reg;
  eaddr_r16_t eaddr;
  

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  ADC16 ( *reg, val, tmp );
  
} // end adc_r16_rm16


static void
adc_r32_rm32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp, val, *reg;
  eaddr_r32_t eaddr;
  

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  ADC32 ( *reg, val, tmp );
  
} // end adc_r32_rm32


static void
adc_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? adc_r32_rm32 ( INTERP ) : adc_r16_rm16 ( INTERP );
} // end adc_r_rm


/********************************************************************/
/* ADCX - Unsigned Integer Addition of Two Operands with Carry Flag */
/********************************************************************/
#if 0
static void
adcx_r32_rm32 (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte;
  uint32_t tmp, val, *reg;
  eaddr_r32_t eaddr;
  

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);

  tmp= *reg;
  *reg= tmp + val + ((EFLAGS&CF_FLAG)!=0);
  EFLAGS&= ~CF_FLAG;
  EFLAGS|=
    (((tmp&val) | ((tmp|val)&(~(*reg)))) & 0x80000000) ? CF_FLAG : 0;
  
} /* end adcx_r32_rm32 */
#endif


/*************************************************************/
/* ADDPD - Add Packed Double-Precision Floating-Point Values */
/*************************************************************/
#if 0
#define CHECK_FP_DENORMALISED(OP1,OP2)           \
  ((fpclassify ( OP1 ) == FP_SUBNORMAL) || \
   (fpclassify ( OP2 ) == FP_SUBNORMAL))

#define SET_SIMD_RETURN  0x2
#define SET_SIMD_SET_RES 0x1

#define SET_SIMD_FP_EXCEPTION__CHECK(MASK,RET_BASE)        		\
  if ( MXCSR&MASK ) return RET_BASE;        				\
  else        								\
    {        								\
      EXCEPTION ( (CR4&CR4_OSXMMEXCPT) ? EXCP_XM : EXCP_UD );        	\
      return SET_SIMD_RETURN|RET_BASE;        				\
    }

#define PRECHECK_SIMD_EXCP_TYPE2        				\
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
  if ( (CR0&CR0_EM) || !(CR4&CR4_OSFXSR) ) { EXCEPTION ( EXCP_UD ); return; } \
  if ( CR0&CR0_TS ) { EXCEPTION ( EXCP_NM ); return; }

#define PRECHECK_SIMD_EXCP_TYPE3 PRECHECK_SIMD_EXCP_TYPE2 


/* Torna 1 si hi ha que continuar, 0 si hi ha que fer una excepció. */
static int
set_simd_fp_exception (
        	       PROTO_INTERP,
        	       const int  flags,
        	       const bool denormal
        	       )
{

  if ( flags&FE_INVALID )
    {
      MXCSR|= MXCSR_IE;
      SET_SIMD_FP_EXCEPTION__CHECK ( MXCSR_IM, 0 );
    }
  if ( flags&FE_DIVBYZERO )
    {
      MXCSR|= MXCSR_ZE;
      SET_SIMD_FP_EXCEPTION__CHECK ( MXCSR_ZM, 0 );
    }
  if ( denormal )
    {
      MXCSR|= MXCSR_DE;
      SET_SIMD_FP_EXCEPTION__CHECK ( MXCSR_DM, 0 );
    }
  if ( flags&FE_OVERFLOW )
    {
      MXCSR|= MXCSR_OE;
      SET_SIMD_FP_EXCEPTION__CHECK ( MXCSR_OM, SET_SIMD_SET_RES );
    }
  if ( flags&FE_UNDERFLOW )
    {
      MXCSR|= MXCSR_UE;
      SET_SIMD_FP_EXCEPTION__CHECK ( MXCSR_UM, SET_SIMD_SET_RES );
    }
  if ( flags&FE_INEXACT )
    {
      MXCSR|= MXCSR_PE;
      SET_SIMD_FP_EXCEPTION__CHECK ( MXCSR_PM, SET_SIMD_SET_RES );
    }

  return 0;
  
} /* end set_simd_fp_exception */


static void
addpd_rxmm_rmxmm128 (
        	     PROTO_INTERP
        	     )
{

  uint8_t modrmbyte;
  IA32_XMMRegister *reg, val, *op2;
  eaddr_rxmm_t eaddr;
  int i, flags, tmp;
  double res;
  bool denormal;
  

  /* Exceptions inicials. */
  PRECHECK_SIMD_EXCP_TYPE2;
  
  /* Opté operadors. */
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rxmm ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READDQ_SSE ( eaddr.v.addr.seg, eaddr.v.addr.off, val.i64, return );
      op2= &val;
    }
  else op2= eaddr.v.reg;

  /* Operació. */
  /* ATENCIÓ!!!! AMB YMM l'endianisme podria ser important. */
  for ( i= 0; i < 2; ++i )
    {

      /* Operació segura. */
      LOCK_FLOAT;
      feclearexcept ( FE_ALL_EXCEPT );
      res= reg->f64[i] + op2->f64[i];
      flags= fetestexcept ( FE_ALL_EXCEPT );
      UNLOCK_FLOAT;

      /* Denormal i fixa excepcions. */
      denormal= CHECK_FP_DENORMALISED ( reg->f64[i], op2->f64[i] );
      if ( !flags && !denormal ) reg->f64[i]= res;
      else
        {
          tmp= set_simd_fp_exception ( INTERP, flags, denormal );
          if ( tmp&SET_SIMD_SET_RES ) reg->f64[i]= res;
          if ( tmp&SET_SIMD_RETURN ) return;
        }
      
    }
  
} /* end addpd_rxmm_rmxmm128 */
#endif

/*************************************************************/
/* ADDPS - Add Packed Single-Precision Floating-Point Values */
/*************************************************************/
#if 0
static void
addps_rxmm_rmxmm128 (
        	     PROTO_INTERP
        	     )
{

  uint8_t modrmbyte;
  IA32_XMMRegister *reg, val, *op2;
  eaddr_rxmm_t eaddr;
  int i, flags, tmp;
  float res;
  bool denormal;
  

  /* Exceptions inicials. */
  PRECHECK_SIMD_EXCP_TYPE2;
  
  /* Opté operadors. */
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rxmm ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READDQ_SSE ( eaddr.v.addr.seg, eaddr.v.addr.off, val.i64, return );
      op2= &val;
    }
  else op2= eaddr.v.reg;

  /* Operació. */
  /* ATENCIÓ!!!! AMB YMM l'endianisme podria ser important. */
  for ( i= 0; i < 4; ++i )
    {

      /* Operació segura. */
      LOCK_FLOAT;
      feclearexcept ( FE_ALL_EXCEPT );
      res= reg->f32[i] + op2->f32[i];
      flags= fetestexcept ( FE_ALL_EXCEPT );
      UNLOCK_FLOAT;

      /* Denormal i fixa excepcions. */
      denormal= CHECK_FP_DENORMALISED ( reg->f32[i], op2->f32[i] );
      if ( !flags && !denormal ) reg->f32[i]= res;
      else
        {
          tmp= set_simd_fp_exception ( INTERP, flags, denormal );
          if ( tmp&SET_SIMD_SET_RES ) reg->f32[i]= res;
          if ( tmp&SET_SIMD_RETURN ) return;
        }
      
    }
  
} /* end addps_rxmm_rmxmm128 */
#endif

/*************************************************************/
/* ADDSD - Add Scalar Double-Precision Floating-Point Values */
/*************************************************************/
#if 0
static void
addsd_rxmm_rmxmm64 (
        	    PROTO_INTERP
        	    )
{

#ifdef IA32_LE
  static const int IND= 0;
#else /* __BIG_ENDIAN__ */
  /* ATENCIÓ!!!! AMB YMM IND podria canviar. */
  static const int IND= 1; 
#endif
  
  uint8_t modrmbyte;
  IA32_XMMRegister *reg;
  eaddr_rxmm_t eaddr;
  int flags, tmp;
  double res, op2;
  bool denormal;
  

  /* Exceptions inicials. */
  PRECHECK_SIMD_EXCP_TYPE3;
  
  /* Opté operadors. */
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rxmm ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READQ ( eaddr.v.addr.seg, eaddr.v.addr.off, (uint64_t *) &op2, return );
    }
  else op2= eaddr.v.reg->f64[IND];

  /* Operació segura. */
  LOCK_FLOAT;
  feclearexcept ( FE_ALL_EXCEPT );
  res= reg->f64[IND] + op2;
  flags= fetestexcept ( FE_ALL_EXCEPT );
  UNLOCK_FLOAT;
  
  /* Denormal i fixa excepcions. */
  denormal= CHECK_FP_DENORMALISED ( reg->f64[IND], op2 );
  if ( !flags && !denormal ) reg->f64[IND]= res;
  else
    {
      tmp= set_simd_fp_exception ( INTERP, flags, denormal );
      if ( tmp&SET_SIMD_SET_RES ) reg->f64[IND]= res;
    }
  
} /* end addsd_rxmm_rmxmm64 */
#endif

/*************************************************************/
/* ADDSS - Add Scalar Single-Precision Floating-Point Values */
/*************************************************************/
#if 0
static void
addss_rxmm_rmxmm32 (
        	    PROTO_INTERP
        	    )
{

#ifdef IA32_LE
  static const int IND= 0;
#else /* __BIG_ENDIAN__ */
  /* ATENCIÓ!!!! AMB YMM IND podria canviar. */
  static const int IND= 3; 
#endif
  
  uint8_t modrmbyte;
  IA32_XMMRegister *reg;
  eaddr_rxmm_t eaddr;
  int flags, tmp;
  float res, op2;
  bool denormal;


  /* Exceptions inicials. */
  PRECHECK_SIMD_EXCP_TYPE3;
  
  /* Opté operadors. */
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rxmm ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off,
        	   (uint32_t *) &op2, return );
    }
  else op2= eaddr.v.reg->f32[IND];

  /* Operació segura. */
  LOCK_FLOAT;
  feclearexcept ( FE_ALL_EXCEPT );
  res= reg->f32[IND] + op2;
  flags= fetestexcept ( FE_ALL_EXCEPT );
  UNLOCK_FLOAT;
  
  /* Denormal i fixa excepcions. */
  denormal= CHECK_FP_DENORMALISED ( reg->f32[IND], op2 );
  if ( !flags && !denormal ) reg->f32[IND]= res;
  else
    {
      tmp= set_simd_fp_exception ( INTERP, flags, denormal );
      if ( tmp&SET_SIMD_SET_RES ) reg->f32[IND]= res;
    }
  
} /* end addss_rxmm_rmxmm32 */
#endif

/********************************************/
/* ADDSUBPD - Packed Double-FP Add/Subtract */
/********************************************/
#if 0
static void
addsubpd_rxmm_rmxmm128 (
        		PROTO_INTERP
        		)
{

  uint8_t modrmbyte;
  IA32_XMMRegister *reg, val, *op2;
  eaddr_rxmm_t eaddr;
  int i, flags, tmp;
  double res;
  bool denormal;
  int switc;
  

  /* Exceptions inicials. */
  PRECHECK_SIMD_EXCP_TYPE2;
  
  /* Opté operadors. */
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rxmm ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READDQ_SSE ( eaddr.v.addr.seg, eaddr.v.addr.off, val.i64, return );
      op2= &val;
    }
  else op2= eaddr.v.reg;

  /* Operació. */
  /* ATENCIÓ!!!! AMB YMM podrien canviar els índex. */
  switc= 0;
#ifdef IA32_LE  
  for ( i= 0; i < 2; ++i )
#else
  for ( i= 1; i >= 0; --i )
#endif
    {

      /* Operació segura. */
      LOCK_FLOAT;
      feclearexcept ( FE_ALL_EXCEPT );
      if ( switc ) res= reg->f64[i] + op2->f64[i];
      else         res= reg->f64[i] - op2->f64[i];
      switc^= 1;
      flags= fetestexcept ( FE_ALL_EXCEPT );
      UNLOCK_FLOAT;
      
      /* Denormal i fixa excepcions. */
      denormal= CHECK_FP_DENORMALISED ( reg->f64[i], op2->f64[i] );
      if ( !flags && !denormal ) reg->f64[i]= res;
      else
        {
          tmp= set_simd_fp_exception ( INTERP, flags, denormal );
          if ( tmp&SET_SIMD_SET_RES ) reg->f64[i]= res;
          if ( tmp&SET_SIMD_RETURN ) return;
        }
      
    }
  
} /* end addsubpd_rxmm_rmxmm128 */
#endif

/********************************************/
/* ADDSUBPS - Packed Single-FP Add/Subtract */
/********************************************/
#if 0
static void
addsubps_rxmm_rmxmm128 (
        		PROTO_INTERP
        		)
{

  uint8_t modrmbyte;
  IA32_XMMRegister *reg, val, *op2;
  eaddr_rxmm_t eaddr;
  int i, flags, tmp;
  float res;
  bool denormal;
  int switc;
  

  /* Exceptions inicials. */
  PRECHECK_SIMD_EXCP_TYPE2;
  
  /* Opté operadors. */
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rxmm ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READDQ_SSE ( eaddr.v.addr.seg, eaddr.v.addr.off, val.i64, return );
      op2= &val;
    }
  else op2= eaddr.v.reg;

  /* Operació. */
  /* ATENCIÓ!!!! AMB YMM podrien canviar els índex. */
  switc= 0;
#ifdef IA32_LE  
  for ( i= 0; i < 4; ++i )
#else
  for ( i= 3; i >= 0; --i )
#endif
    {

      /* Operació segura. */
      LOCK_FLOAT;
      feclearexcept ( FE_ALL_EXCEPT );
      if ( switc ) res= reg->f32[i] + op2->f32[i];
      else         res= reg->f32[i] - op2->f32[i];
      switc^= 1;
      flags= fetestexcept ( FE_ALL_EXCEPT );
      UNLOCK_FLOAT;
      
      /* Denormal i fixa excepcions. */
      denormal= CHECK_FP_DENORMALISED ( reg->f32[i], op2->f32[i] );
      if ( !flags && !denormal ) reg->f32[i]= res;
      else
        {
          tmp= set_simd_fp_exception ( INTERP, flags, denormal );
          if ( tmp&SET_SIMD_SET_RES ) reg->f32[i]= res;
          if ( tmp&SET_SIMD_RETURN ) return;
        }
      
    }
  
} /* end addsubps_rxmm_rmxmm128 */
#endif

/***********************************************************************/
/* ADOX - Unsigned Integer Addition of Two Operands with Overflow Flag */
/***********************************************************************/
#if 0
static void
adox_r32_rm32 (
               PROTO_INTERP
               )
{
  
  uint8_t modrmbyte;
  uint32_t tmp, val, *reg;
  eaddr_r32_t eaddr;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);

  tmp= *reg;
  *reg= tmp + val + ((EFLAGS&OF_FLAG)!=0);
  EFLAGS&= ~OF_FLAG;
  EFLAGS|=
    (((tmp&val) | ((tmp|val)&(~(*reg)))) & 0x80000000) ? OF_FLAG : 0;
  
} // end adox_r32_rm32
#endif

/*************/
/* ADD - Add */
/*************/

#define ADD8(DST,SRC,VARU8)                                             \
  VARU8= (DST);                                                         \
  (DST)= VARU8 + (SRC);        						\
  SETU8_AD_FLAGS ( (DST), VARU8, (SRC) )

#define ADD16(DST,SRC,VARU16)                                           \
  VARU16= (DST);                                                        \
  (DST)= VARU16 + (SRC);        					\
  SETU16_AD_FLAGS ( (DST), VARU16, (SRC) )

#define ADD32(DST,SRC,VARU32)                                           \
  VARU32= (DST);                                                        \
  (DST)= VARU32 + (SRC);        					\
  SETU32_AD_FLAGS ( (DST), VARU32, (SRC) )

#define ADD_RM8_VAL8(DST,VAL,TMP,EADDR)                         \
  if ( (EADDR).is_addr )                                        \
    {        								\
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ADD8 ( (DST), (VAL), (TMP) );        				\
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      ADD8 ( *((EADDR).v.reg), (VAL), (TMP) );                  \
    }

#define ADD_RM16_VAL16(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ADD16 ( (DST), (VAL), (TMP) );                                    \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      ADD16 ( *((EADDR).v.reg), (VAL), (TMP) );                         \
    }

#define ADD_RM32_VAL32(DST,VAL,TMP,EADDR)        			\
  if ( (EADDR).is_addr )        					\
    {        								\
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ADD32 ( (DST), (VAL), (TMP) );                            \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      ADD32 ( *((EADDR).v.reg), (VAL), (TMP) );                 \
    }

static void
add_AL_imm8 (
             PROTO_INTERP
             )
{
  
  uint8_t tmp, val;
  
  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ADD8 ( AL, val, tmp );
  
} // end add_AL_imm8


static void
add_AX_imm16 (
              PROTO_INTERP
              )
{

  uint16_t tmp, val;


  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ADD16 ( AX, val, tmp );
  
} // end add_AX_imm16


static void
add_EAX_imm32 (
               PROTO_INTERP
               )
{

  uint32_t tmp, val;
  

  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ADD32 ( EAX, val, tmp );
  
} // end add_EAX_imm32


static void
add_A_imm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? add_EAX_imm32 ( INTERP ) : add_AX_imm16 ( INTERP );
} // end add_A_imm


static void
add_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  uint8_t tmp, val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &val, return );
  ADD_RM8_VAL8 ( dst, val, tmp, eaddr );
  
} // end add_rm8_imm8


static void
add_rm16_imm16 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{

  uint16_t tmp, val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  ADD_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end add_rm16_imm16


static void
add_rm32_imm32 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{

  uint32_t tmp, val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  ADD_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end add_rm32_imm32


static void
add_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    add_rm32_imm32 ( INTERP, modrmbyte ) :
    add_rm16_imm16 ( INTERP, modrmbyte );
} // end add_rm_imm


static void
add_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint16_t tmp, val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  ADD_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end add_rm16_imm8


static void
add_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint32_t tmp, val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  ADD_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end add_rm32_imm8


static void
add_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    add_rm32_imm8 ( INTERP, modrmbyte ) :
    add_rm16_imm8 ( INTERP, modrmbyte );
} // end add_rm_imm8


static void
add_rm8_r8 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte, tmp, dst, *reg;
  eaddr_r8_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  ADD_RM8_VAL8 ( dst, *reg, tmp, eaddr );
  
} // end add_rm8_r8


static void
add_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp, dst, *reg;
  eaddr_r16_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  ADD_RM16_VAL16 ( dst, *reg, tmp, eaddr );
  
} // end add_rm16_r16


static void
add_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp, dst, *reg;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  ADD_RM32_VAL32 ( dst, *reg, tmp, eaddr );
  
} // end add_rm32_r32


static void
add_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? add_rm32_r32 ( INTERP ) : add_rm16_r16 ( INTERP );
} // end add_rm_r


static void
add_r8_rm8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte, tmp, val, *reg;
  eaddr_r8_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  ADD8 ( *reg, val, tmp );
  
} // end add_r8_rm8


static void
add_r16_rm16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp, val, *reg;
  eaddr_r16_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  ADD16 ( *reg, val, tmp );
  
} // end add_r16_rm16


static void
add_r32_rm32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp, val, *reg;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  ADD32 ( *reg, val, tmp );
  
} // end add_r32_rm32


static void
add_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? add_r32_rm32 ( INTERP ) : add_r16_rm16 ( INTERP );
} // end add_r_rm


/************************/
/* INC - Increment by 1 */
/************************/

#define SET_INC_FLAGS(RES,OP1,MBIT)        				\
  EFLAGS&= ~(OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG);                  \
  EFLAGS|=        							\
    ((((~((OP1)^(0x1)))&((OP1)^(RES))&(MBIT)) ? OF_FLAG : 0) |        	\
    (((RES)&(MBIT)) ? SF_FLAG : 0) |        				\
    (((RES)==0) ? ZF_FLAG : 0) |        				\
    (((((OP1)^(0x1))^(RES))&0x10) ? AF_FLAG : 0) |        		\
    PFLAG[(uint8_t) ((RES)&0xFF)])

#define SETU8_INC_FLAGS(RES,OP1)        	\
  SET_INC_FLAGS(RES,OP1,0x80)

#define SETU16_INC_FLAGS(RES,OP1)        	\
  SET_INC_FLAGS(RES,OP1,0x8000)

#define SETU32_INC_FLAGS(RES,OP1)        	\
  SET_INC_FLAGS(RES,OP1,0x80000000)

#define INC8(DST,VARU8)                                                 \
  VARU8= (DST);        							\
  (DST)= VARU8 + 1;                                                     \
  SETU8_INC_FLAGS ( (DST), VARU8 )

#define INC16(DST,VARU16)        					\
  VARU16= (DST);        						\
  (DST)= VARU16 + 1;                                                    \
  SETU16_INC_FLAGS ( (DST), VARU16 )

#define INC32(DST,VARU32)        					\
  VARU32= (DST);        						\
  (DST)= VARU32 + 1;                                                    \
  SETU32_INC_FLAGS ( (DST), VARU32 )

static void
inc_rm8 (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  
  uint8_t val,tmp;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
      INC8 ( val, tmp );
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else { INC8 ( *eaddr.v.reg, tmp ); }
  
} // end inc_rm8


static void
inc_r (
       PROTO_INTERP,
       const uint8_t opcode
       )
{

  uint16_t tmp16;
  uint32_t tmp32;

  
  if ( OPERAND_SIZE_IS_32 )
    {
      switch ( opcode&0x7 )
        {
        case 0: INC32 ( EAX, tmp32 ); break;
        case 1: INC32 ( ECX, tmp32 ); break;
        case 2: INC32 ( EDX, tmp32 ); break;
        case 3: INC32 ( EBX, tmp32 ); break;
        case 4: INC32 ( ESP, tmp32 ); break;
        case 5: INC32 ( EBP, tmp32 ); break;
        case 6: INC32 ( ESI, tmp32 ); break;
        case 7: INC32 ( EDI, tmp32 ); break;
        }
    }
  else
    {
      switch ( opcode&0x7 )
        {
        case 0: INC16 ( AX, tmp16 ); break;
        case 1: INC16 ( CX, tmp16 ); break;
        case 2: INC16 ( DX, tmp16 ); break;
        case 3: INC16 ( BX, tmp16 ); break;
        case 4: INC16 ( SP, tmp16 ); break;
        case 5: INC16 ( BP, tmp16 ); break;
        case 6: INC16 ( SI, tmp16 ); break;
        case 7: INC16 ( DI, tmp16 ); break;
        }
    }
  
} // end inc_r


static void
inc_rm16 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  uint16_t val,tmp;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
      INC16 ( val, tmp );
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else { INC16 ( *eaddr.v.reg, tmp ); }
  
} // end inc_rm16


static void
inc_rm32 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  uint32_t val,tmp;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
      INC32 ( val, tmp );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else { INC32 ( *eaddr.v.reg, tmp ); }
  
} // end inc_rm32


static void
inc_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    inc_rm32 ( INTERP, modrmbyte ) :
    inc_rm16 ( INTERP, modrmbyte );
} // end inc_rm
