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
 *  interpreter_cmp_sub.h - Implementa CMP, SUB i semblants.
 *
 */


/******************************/
/* CMP - Compare Two Operands */
/******************************/

#define SET_SB_FLAGS(RES,OP1,OP2,MBIT)                                 \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|CF_FLAG|PF_FLAG);          \
  EFLAGS|=                                                              \
    ((((((OP1)^(OP2))&((OP1)^(RES)))&(MBIT)) ? OF_FLAG : 0) |           \
     (((RES)&(MBIT)) ? SF_FLAG : 0) |                                   \
     (((RES)==0) ? ZF_FLAG : 0) |                                       \
     (((((OP1)^(~(OP2)))^(RES))&0x10) ? 0 : AF_FLAG) |                  \
     (((((OP1)&(~(OP2))) | (((OP1)|(~(OP2)))&(~(RES))))&(MBIT))?0:CF_FLAG) | \
     PFLAG[(uint8_t) ((RES)&0xFF)])

#define SETU8_SB_FLAGS(RES,OP1,OP2)             \
  SET_SB_FLAGS(RES,OP1,OP2,0x80)

#define SETU16_SB_FLAGS(RES,OP1,OP2)            \
  SET_SB_FLAGS(RES,OP1,OP2,0x8000)

#define SETU32_SB_FLAGS(RES,OP1,OP2)            \
  SET_SB_FLAGS(RES,OP1,OP2,0x80000000)

#define CMP8(DST,SRC,VARU8)                                            \
  VARU8= (uint8_t) ((int8_t) (DST) -  (int8_t) (SRC));                 \
  SETU8_SB_FLAGS ( (VARU8), (DST), (SRC) )

#define CMP16(DST,SRC,VARU16)                                           \
  VARU16= (uint16_t) ((int16_t) (DST) -  (int16_t) (SRC));              \
  SETU16_SB_FLAGS ( (VARU16), (DST), (SRC) )

#define CMP32(DST,SRC,VARU32)                                           \
  VARU32= (uint32_t) ((int32_t) (DST) - (int32_t) (SRC));               \
  SETU32_SB_FLAGS ( (VARU32), (DST), (SRC) )

#define CMP_RM8_VAL8(DST,VAL,TMP,EADDR)                                 \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      CMP8 ( (DST), (VAL), (TMP) );                                     \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      CMP8 ( *((EADDR).v.reg), (VAL), (TMP) );                         \
    }

#define CMP_RM16_VAL16(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      CMP16 ( (DST), (VAL), (TMP) );                                    \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      CMP16 ( *((EADDR).v.reg), (VAL), (TMP) );                         \
    }

#define CMP_RM32_VAL32(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      CMP32 ( (DST), (VAL), (TMP) );                                    \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      CMP32 ( *((EADDR).v.reg), (VAL), (TMP) );                 \
    }

static void
cmp_AL_imm8 (
             PROTO_INTERP
             )
{

  uint8_t val, tmp;

  
  IB ( (uint8_t *) &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  CMP8 ( AL, val, tmp );
  
} // end cmp_AL_imm8


static void
cmp_AX_imm16 (
              PROTO_INTERP
              )
{

  uint16_t val,tmp;

  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  CMP16 ( AX, val, tmp );
  
} // end cmp_AX_imm16


static void
cmp_EAX_imm32 (
               PROTO_INTERP
               )
{

  uint32_t val,tmp;
  
  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  CMP32 ( EAX, val, tmp );
  
} // end cmp_EAX_imm32


static void
cmp_A_imm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? cmp_EAX_imm32 ( INTERP ) : cmp_AX_imm16 ( INTERP );
} // end cmp_A_imm


static void
cmp_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t tmp, val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &val, return );
  CMP_RM8_VAL8 ( dst, val, tmp, eaddr );
  
} // end cmp_rm8_imm8


static void
cmp_rm16_imm16 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{
  
  uint16_t tmp, val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  CMP_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end cmp_rm16_imm16


static void
cmp_rm32_imm32 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{

  uint32_t tmp, val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  CMP_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end cmp_rm32_imm32


static void
cmp_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    cmp_rm32_imm32 ( INTERP, modrmbyte ) :
    cmp_rm16_imm16 ( INTERP, modrmbyte );
} // end cmp_rm_imm


static void
cmp_rm16_imm8 (
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
  CMP_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end cmp_rm16_imm8


static void
cmp_rm32_imm8 (
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
  CMP_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end cmp_rm32_imm8


static void
cmp_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    cmp_rm32_imm8 ( INTERP, modrmbyte ) :
    cmp_rm16_imm8 ( INTERP, modrmbyte );
} // end cmp_rm_imm8


static void
cmp_rm8_r8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  uint8_t tmp, dst, *reg;
  eaddr_r8_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  CMP_RM8_VAL8 ( dst, *reg, tmp, eaddr );
  
} // end cmp_rm8_r8


static void
cmp_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp, dst, *reg;
  eaddr_r16_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  CMP_RM16_VAL16 ( dst, *reg, tmp, eaddr );
  
} // end cmp_rm16_r16


static void
cmp_rm32_r32 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint32_t tmp, dst, *reg;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  CMP_RM32_VAL32 ( dst, *reg, tmp, eaddr );
  
} // end cmp_rm32_r32


static void
cmp_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? cmp_rm32_r32 ( INTERP ) : cmp_rm16_r16 ( INTERP );
} // end cmp_rm_r


static void
cmp_r8_rm8 (
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
  CMP8 ( *reg, val, tmp );
  
} // end cmp_r8_rm8


static void
cmp_r16_rm16 (
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
  CMP16 ( *reg, val, tmp );
  
} // end cmp_r16_rm16


static void
cmp_r32_rm32 (
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
  CMP32 ( *reg, val, tmp );
  
} // end cmp_r32_rm32


static void
cmp_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? cmp_r32_rm32 ( INTERP ) : cmp_r16_rm16 ( INTERP );
} // end cmp_r_rm


/**********************************************************/
/* CMPS/CMPSB/CMPSW/CMPSD/CMPSQ - Compare String Operands */
/**********************************************************/

static void
cmpsb (
       PROTO_INTERP
       )
{
  
  addr_t src1,src2;
  uint8_t val1,val2,tmp;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addrs_dsesi_esedi ( INTERP, &src1, &src2 );
      READB_DATA ( src1.seg, src1.off, &val1, return );
      READB_DATA ( src2.seg, src2.off, &val2, return );
      
      // Compara
      CMP8 ( val1, val2, tmp );
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) { --ESI; --EDI; }
          else                  { ++ESI; ++EDI; }
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) { --SI; --DI; }
          else                  { ++SI; ++DI; }
        }
      
      // Repeteix si pertoca.
      TRY_REPE_REPNE;
      
    }
  
} // end cmpsb


static void
cmpsw (
       PROTO_INTERP
       )
{
  
  addr_t src1,src2;
  uint16_t val1,val2,tmp;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addrs_dsesi_esedi ( INTERP, &src1, &src2 );
      READW_DATA ( src1.seg, src1.off, &val1, return );
      READW_DATA ( src2.seg, src2.off, &val2, return );
      
      // Compara
      CMP16 ( val1, val2, tmp );
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) { ESI-= 2; EDI-= 2; }
          else                  { ESI+= 2; EDI+= 2; }
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) { SI-= 2; DI-= 2; }
          else                  { SI+= 2; DI+= 2; }
        }
      
      // Repeteix si pertoca.
      TRY_REPE_REPNE;
      
    }
  
} // end cmpsw


static void
cmpsd (
       PROTO_INTERP
       )
{
  
  addr_t src1,src2;
  uint32_t val1,val2,tmp;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addrs_dsesi_esedi ( INTERP, &src1, &src2 );
      READD_DATA ( src1.seg, src1.off, &val1, return );
      READD_DATA ( src2.seg, src2.off, &val2, return );
      
      // Compara
      CMP32 ( val1, val2, tmp );
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) { ESI-= 4; EDI-= 4; }
          else                  { ESI+= 4; EDI+= 4; }
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) { SI-= 4; DI-= 4; }
          else                  { SI+= 4; DI+= 4; }
        }
      
      // Repeteix si pertoca.
      TRY_REPE_REPNE;
      
    }
  
} // end cmpsd


static void
cmps (
      PROTO_INTERP
      )
{
  OPERAND_SIZE_IS_32 ? cmpsd ( INTERP ) : cmpsw ( INTERP );
} // end cmps




/************************/
/* DEC - Decrement by 1 */
/************************/

#define SET_DEC_FLAGS(RES,OP1,MBIT)                                     \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|PF_FLAG);                  \
  EFLAGS|=                                                              \
    ((((((OP1)^(0x1))&((OP1)^(RES)))&(MBIT)) ? OF_FLAG : 0) |           \
     (((RES)&(MBIT)) ? SF_FLAG : 0) |                                   \
     (((RES)==0) ? ZF_FLAG : 0) |                                       \
     (((((OP1)^(~(0x1)))^(RES))&0x10) ? 0 : AF_FLAG) |                  \
     PFLAG[(uint8_t) ((RES)&0xFF)])

#define SETU8_DEC_FLAGS(RES,OP1)                \
  SET_DEC_FLAGS(RES,OP1,0x80)

#define SETU16_DEC_FLAGS(RES,OP1)               \
  SET_DEC_FLAGS(RES,OP1,0x8000)

#define SETU32_DEC_FLAGS(RES,OP1)               \
  SET_DEC_FLAGS(RES,OP1,0x80000000)

#define DEC8(DST,VARU8)                                                 \
  VARU8= (DST);                                                         \
  (DST)= (uint8_t) ((int8_t) (VARU8) - 1);                              \
  SETU8_DEC_FLAGS ( (DST), (VARU8) )

#define DEC16(DST,VARU16)                                               \
  VARU16= (DST);                                                        \
  (DST)= (uint16_t) ((int16_t) (VARU16) - 1);                           \
  SETU16_DEC_FLAGS ( (DST), VARU16 )

#define DEC32(DST,VARU32)                                               \
  VARU32= (DST);                                                        \
  (DST)= (uint32_t) ((int32_t) (VARU32) - 1);                           \
  SETU32_DEC_FLAGS ( (DST), VARU32 )

static void
dec_rm8 (
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
      DEC8 ( val, tmp );
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else { DEC8 ( *eaddr.v.reg, tmp ); }
  
} // end dec_rm8


static void
dec_r (
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
        case 0: DEC32 ( EAX, tmp32 ); break;
        case 1: DEC32 ( ECX, tmp32 ); break;
        case 2: DEC32 ( EDX, tmp32 ); break;
        case 3: DEC32 ( EBX, tmp32 ); break;
        case 4: DEC32 ( ESP, tmp32 ); break;
        case 5: DEC32 ( EBP, tmp32 ); break;
        case 6: DEC32 ( ESI, tmp32 ); break;
        case 7: DEC32 ( EDI, tmp32 ); break;
        }
    }
  else
    {
      switch ( opcode&0x7 )
        {
        case 0: DEC16 ( AX, tmp16 ); break;
        case 1: DEC16 ( CX, tmp16 ); break;
        case 2: DEC16 ( DX, tmp16 ); break;
        case 3: DEC16 ( BX, tmp16 ); break;
        case 4: DEC16 ( SP, tmp16 ); break;
        case 5: DEC16 ( BP, tmp16 ); break;
        case 6: DEC16 ( SI, tmp16 ); break;
        case 7: DEC16 ( DI, tmp16 ); break;
        }
    }
  
} // end dec_r


static void
dec_rm16 (
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
      DEC16 ( val, tmp );
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else { DEC16 ( *eaddr.v.reg, tmp ); }
  
} // end dec_rm16


static void
dec_rm32 (
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
      DEC32 ( val, tmp );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else { DEC32 ( *eaddr.v.reg, tmp ); }
  
} // end dec_rm32


static void
dec_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    dec_rm32 ( INTERP, modrmbyte ) :
    dec_rm16 ( INTERP, modrmbyte );
} // end dec_rm




/***********************************/
/* NEG - Two's Complement Negation */
/***********************************/

#define SET_NEG_FLAGS(RES,OP,MBIT)                                      \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|ZF_FLAG|AF_FLAG|CF_FLAG|PF_FLAG);          \
  EFLAGS|=                                                              \
    ((((((0)^(OP))&((0)^(RES)))&(MBIT)) ? OF_FLAG : 0) |                \
     (((RES)&(MBIT)) ? SF_FLAG : 0) |                                   \
     (((RES)==0) ? ZF_FLAG : 0) |                                       \
     (((((0)^(~(OP)))^(RES))&0x10) ? 0 : AF_FLAG) |                     \
     (((OP)==0) ? 0 : CF_FLAG) |                                        \
     PFLAG[(uint8_t) ((RES)&0xFF)])

#define SETU8_NEG_FLAGS(RES,OP)                 \
  SET_NEG_FLAGS(RES,OP,0x80)

#define SETU16_NEG_FLAGS(RES,OP)                \
  SET_NEG_FLAGS(RES,OP,0x8000)

#define SETU32_NEG_FLAGS(RES,OP)                \
  SET_NEG_FLAGS(RES,OP,0x80000000)

#define NEG8(VAL,VARU8)                                                \
  VARU8= (uint8_t) (- ((int8_t) (VAL)));                               \
  SETU8_NEG_FLAGS ( (VARU8), (VAL) );                                  \
  (VAL)= (VARU8)

#define NEG16(VAL,VARU16)                                              \
  VARU16= (uint16_t) (- ((int16_t) (VAL)));                            \
  SETU16_NEG_FLAGS ( (VARU16), (VAL) );                                \
  (VAL)= (VARU16)

#define NEG32(VAL,VARU32)                                              \
  VARU32= (uint32_t) (- ((int32_t) (VAL)));                            \
  SETU32_NEG_FLAGS ( (VARU32), (VAL) );                                \
  (VAL)= (VARU32)

static void
neg_rm8 (
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
      NEG8 ( val, tmp );
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else
    {
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
      NEG8 ( *eaddr.v.reg, tmp );
    }
  
} // end neg_rm8


static void
neg_rm16 (
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
      NEG16 ( val, tmp );
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else
    {
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
      NEG16 ( *eaddr.v.reg, tmp );
    }
  
} // end neg_rm16


static void
neg_rm32 (
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
      NEG32 ( val, tmp );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else
    {
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
      NEG32 ( *eaddr.v.reg, tmp );
    }
  
} // end neg_rm32


static void
neg_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    neg_rm32 ( INTERP, modrmbyte ) :
    neg_rm16 ( INTERP, modrmbyte );
} // end neg_rm




/*****************************************/
/* SBB - Integer Subtraction with Borrow */
/*****************************************/

// Faig la resta amb unsigned. Restar es negar el segon i sumar
// 1. Quan hi ha carry es suma 0.
#define SBB8(DST,SRC,VARU8)                                            \
  VARU8= (DST);                                                        \
  (DST)= (VARU8) + ((~(SRC))&0xFF) + ((EFLAGS&CF_FLAG)==0);            \
  SETU8_SB_FLAGS ( (DST), (VARU8), (SRC) )

#define SBB16(DST,SRC,VARU16)                                           \
  VARU16= (DST);                                                        \
  (DST)= (VARU16) + ((~(SRC))&0xFFFF) + ((EFLAGS&CF_FLAG)==0);          \
  SETU16_SB_FLAGS ( (DST), (VARU16), (SRC) )

#define SBB32(DST,SRC,VARU32)                                           \
  VARU32= (DST);                                                        \
  (DST)= (VARU32) + ((~(SRC))&0xFFFFFFFF) + ((EFLAGS&CF_FLAG)==0);      \
  SETU32_SB_FLAGS ( (DST), (VARU32), (SRC) )

#define SBB_RM8_VAL8(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SBB8 ( (DST), (VAL), (TMP) );                                     \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SBB8 ( *((EADDR).v.reg), (VAL), (TMP) );                         \
    }

#define SBB_RM16_VAL16(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SBB16 ( (DST), (VAL), (TMP) );                                    \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SBB16 ( *((EADDR).v.reg), (VAL), (TMP) );                         \
    }

#define SBB_RM32_VAL32(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SBB32 ( (DST), (VAL), (TMP) );                                    \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      SBB32 ( *((EADDR).v.reg), (VAL), (TMP) );                 \
    }

static void
sbb_AL_imm8 (
             PROTO_INTERP
             )
{
  
  uint8_t val, tmp;

  
  IB ( (uint8_t *) &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  SBB8 ( AL, val, tmp );
  
} // end sbb_AL_imm8


static void
sbb_AX_imm16 (
              PROTO_INTERP
              )
{

  uint16_t val,tmp;

  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  SBB16 ( AX, val, tmp );
  
} // end sbb_AX_imm16


static void
sbb_EAX_imm32 (
               PROTO_INTERP
               )
{

  uint32_t val,tmp;
  
  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  SBB32 ( EAX, val, tmp );
  
} // end sbb_EAX_imm32


static void
sbb_A_imm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? sbb_EAX_imm32 ( INTERP ) : sbb_AX_imm16 ( INTERP );
} // end sbb_A_imm


static void
sbb_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t tmp, val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &val, return );
  SBB_RM8_VAL8 ( dst, val, tmp, eaddr );
  
} // end sbb_rm8_imm8


static void
sbb_rm16_imm16 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{
  
  uint16_t tmp, val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  SBB_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end sbb_rm16_imm16


static void
sbb_rm32_imm32 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{
  
  uint32_t tmp, val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  SBB_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end sbb_rm32_imm32


static void
sbb_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    sbb_rm32_imm32 ( INTERP, modrmbyte ) :
    sbb_rm16_imm16 ( INTERP, modrmbyte );
} // end sbb_rm_imm


static void
sbb_rm16_imm8 (
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
  SBB_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end sbb_rm16_imm8


static void
sbb_rm32_imm8 (
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
  SBB_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end sbb_rm32_imm8


static void
sbb_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    sbb_rm32_imm8 ( INTERP, modrmbyte ) :
    sbb_rm16_imm8 ( INTERP, modrmbyte );
} // end sbb_rm_imm8


static void
sbb_rm8_r8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte, tmp, dst, *reg;
  eaddr_r8_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  SBB_RM8_VAL8 ( dst, *reg, tmp, eaddr );
  
} // end sbb_rm8_r8


static void
sbb_rm16_r16 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint16_t tmp, dst, *reg;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  SBB_RM16_VAL16 ( dst, *reg, tmp, eaddr );
  
} // end sbb_rm16_r16


static void
sbb_rm32_r32 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint32_t dst, *reg, tmp;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  SBB_RM32_VAL32 ( dst, *reg, tmp, eaddr );
  
} // end sbb_rm32_r32


static void
sbb_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? sbb_rm32_r32 ( INTERP ) : sbb_rm16_r16 ( INTERP );
} // end sbb_rm_r


static void
sbb_r8_rm8 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte;
  uint8_t tmp, val, *reg;
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
  SBB8 ( *reg, val, tmp );
  
} // end sbb_r8_rm8


static void
sbb_r16_rm16 (
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
  SBB16 ( *reg, val, tmp );
  
} // end sbb_r16_rm16


static void
sbb_r32_rm32 (
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
  SBB32 ( *reg, val, tmp );
  
} // end sbb_r32_rm32


static void
sbb_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? sbb_r32_rm32 ( INTERP ) : sbb_r16_rm16 ( INTERP );
} // end sbb_r_rm




/****************************************/
/* SCAS/SCASB/SCASW/SCASD - Scan String */
/****************************************/

static void
scasb (
       PROTO_INTERP
       )
{
  
  addr_t src;
  uint8_t val,tmp;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {

      src.seg= P_ES;
      src.off= ADDRESS_SIZE_IS_32 ? EDI : DI;
      READB_DATA ( src.seg, src.off, &val, return );
      
      // Compara
      CMP8 ( AL, val, tmp );
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) --EDI;
          else                  ++EDI;
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) --DI;
          else                  ++DI;
        }

      // Repeteix si pertoca.
      TRY_REPE_REPNE;
      
    }
  
} // end scasb


static void
scasw (
       PROTO_INTERP
       )
{
  
  addr_t src;
  uint16_t val,tmp;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {

      src.seg= P_ES;
      src.off= ADDRESS_SIZE_IS_32 ? EDI : DI;
      READW_DATA ( src.seg, src.off, &val, return );
      
      // Compara
      CMP16 ( AX, val, tmp );
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) EDI-= 2;
          else                  EDI+= 2;
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) DI-= 2;
          else                  DI+= 2;
        }

      // Repeteix si pertoca.
      TRY_REPE_REPNE;
      
    }
  
} // end scasw


static void
scasd (
       PROTO_INTERP
       )
{
  
  addr_t src;
  uint32_t val,tmp;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {

      src.seg= P_ES;
      src.off= ADDRESS_SIZE_IS_32 ? EDI : DI;
      READD_DATA ( src.seg, src.off, &val, return );
      
      // Compara
      CMP32 ( EAX, val, tmp );
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) EDI-= 4;
          else                  EDI+= 4;
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) DI-= 4;
          else                  DI+= 4;
        }
      
      // Repeteix si pertoca.
      TRY_REPE_REPNE;
      
    }
  
} // end scasd


static void
scas (
      PROTO_INTERP
      )
{
  OPERAND_SIZE_IS_32 ? scasd ( INTERP ) : scasw ( INTERP );
} // end scas




/******************/
/* SUB - Subtract */
/******************/

#define SUB8(DST,SRC,VARU8)                                            \
  VARU8= (DST);                                                        \
  (DST)= (uint8_t) ((int8_t) (VARU8) -  (int8_t) (SRC));               \
  SETU8_SB_FLAGS ( (DST), (VARU8), (SRC) )

#define SUB16(DST,SRC,VARU16)                                           \
  VARU16= (DST);                                                        \
  (DST)= (uint16_t) ((int16_t) (VARU16) -  (int16_t) (SRC));            \
  SETU16_SB_FLAGS ( (DST), (VARU16), (SRC) )

#define SUB32(DST,SRC,VARU32)                                           \
  VARU32= (DST);                                                        \
  (DST)= (uint32_t) ((int32_t) (VARU32) - (int32_t) (SRC));             \
  SETU32_SB_FLAGS ( (DST), (VARU32), (SRC) )

#define SUB_RM8_VAL8(DST,VAL,TMP,EADDR)                                 \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SUB8 ( (DST), (VAL), (TMP) );                                     \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SUB8 ( *((EADDR).v.reg), (VAL), (TMP) );                          \
    }

#define SUB_RM16_VAL16(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SUB16 ( (DST), (VAL), (TMP) );                                    \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SUB16 ( *((EADDR).v.reg), (VAL), (TMP) );                         \
    }

#define SUB_RM32_VAL32(DST,VAL,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SUB32 ( (DST), (VAL), (TMP) );                                    \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      SUB32 ( *((EADDR).v.reg), (VAL), (TMP) );                 \
    }

static void
sub_AL_imm8 (
             PROTO_INTERP
             )
{
  
  uint8_t val,tmp;
  
  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  SUB8 ( AL, val, tmp );
  
} // end sub_AL_imm8


static void
sub_AX_imm16 (
              PROTO_INTERP
              )
{
  
  uint16_t val,tmp;
  
  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  SUB16 ( AX, val, tmp );
  
} // end sub_AX_imm16


static void
sub_EAX_imm32 (
               PROTO_INTERP
               )
{

  uint32_t val,tmp;
  
  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  SUB32 ( EAX, val, tmp );
  
} // end sub_EAX_imm32


static void
sub_A_imm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? sub_EAX_imm32 ( INTERP ) : sub_AX_imm16 ( INTERP );
} // end sub_A_imm


static void
sub_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t tmp, val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &val, return );
  SUB_RM8_VAL8 ( dst, val, tmp, eaddr );
  
} // end sub_rm8_imm8


static void
sub_rm16_imm16 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{
  
  uint16_t tmp, val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  SUB_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end sub_rm16_imm16


static void
sub_rm32_imm32 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{
  
  uint32_t tmp, val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  SUB_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end sub_rm32_imm32


static void
sub_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    sub_rm32_imm32 ( INTERP, modrmbyte ) :
    sub_rm16_imm16 ( INTERP, modrmbyte );
} // end sub_rm_imm


static void
sub_rm16_imm8 (
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
  SUB_RM16_VAL16 ( dst, val, tmp, eaddr );
  
} // end sub_rm16_imm8


static void
sub_rm32_imm8 (
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
  SUB_RM32_VAL32 ( dst, val, tmp, eaddr );
  
} // end sub_rm32_imm8


static void
sub_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    sub_rm32_imm8 ( INTERP, modrmbyte ) :
    sub_rm16_imm8 ( INTERP, modrmbyte );
} // end sub_rm_imm8


static void
sub_rm8_r8 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte;
  uint8_t tmp, dst, *reg;
  eaddr_r8_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  SUB_RM8_VAL8 ( dst, *reg, tmp, eaddr );
  
} // end sub_rm8_r8


static void
sub_rm16_r16 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint16_t tmp, dst, *reg;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  SUB_RM16_VAL16 ( dst, *reg, tmp, eaddr );
  
} // end sub_rm16_r16


static void
sub_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t dst, *reg, tmp;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  SUB_RM32_VAL32 ( dst, *reg, tmp, eaddr );
  
} // end sub_rm32_r32


static void
sub_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? sub_rm32_r32 ( INTERP ) : sub_rm16_r16 ( INTERP );
} // end sub_rm_r


static void
sub_r8_rm8 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte;
  uint8_t tmp, val, *reg;
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
  SUB8 ( *reg, val, tmp );
  
} // end sub_r8_rm8


static void
sub_r16_rm16 (
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
  SUB16 ( *reg, val, tmp );
  
} // end sub_r16_rm16


static void
sub_r32_rm32 (
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
  SUB32 ( *reg, val, tmp );
  
} // end sub_r32_rm32


static void
sub_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? sub_r32_rm32 ( INTERP ) : sub_r16_rm16 ( INTERP );
} // end sub_r_rm
