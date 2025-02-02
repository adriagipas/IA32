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
 *  interpreter_lop.h - Operacions llògiques.
 *
 */



#define SET_LOP_FLAGS(RES,MBIT)                                         \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|ZF_FLAG|CF_FLAG|PF_FLAG);                  \
  EFLAGS|=                                                              \
    ((((RES)&(MBIT)) ? SF_FLAG : 0) |                                   \
     (((RES)==0) ? ZF_FLAG : 0) |                                       \
     PFLAG[(uint8_t) ((RES)&0xFF)])

#define SETU8_LOP_FLAGS(RES)                    \
  SET_LOP_FLAGS(RES,0x80)

#define SETU16_LOP_FLAGS(RES)                   \
  SET_LOP_FLAGS(RES,0x8000)

#define SETU32_LOP_FLAGS(RES)                   \
  SET_LOP_FLAGS(RES,0x80000000)

#define LOP8(OP,DST,SRC)                                                \
  (DST)= (DST) OP (SRC);                                                \
  SETU8_LOP_FLAGS ( (DST) )

#define LOP16(OP,DST,SRC)                                            \
  (DST)= (DST) OP (SRC);                                             \
  SETU16_LOP_FLAGS ( (DST) )
  
#define LOP32(OP,DST,SRC)                                        \
  (DST)= (DST) OP (SRC);                                         \
  SETU32_LOP_FLAGS ( (DST) )

#define LOP_RM8_VAL8(OP,DST,VAL,EADDR)                                  \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      LOP8 ( OP, (DST), (VAL) );                                        \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      LOP8 ( OP, *((EADDR).v.reg), (VAL) );                             \
    }

#define LOP_RM16_VAL16(OP,DST,VAL,EADDR)                                \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      LOP16 ( OP, (DST), (VAL) );                                       \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      LOP16 ( OP, *((EADDR).v.reg), (VAL) );                            \
    }

#define LOP_RM32_VAL32(OP,DST,VAL,EADDR)                                \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      LOP32 ( OP, (DST), (VAL) );                                       \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      LOP32 ( OP, *((EADDR).v.reg), (VAL) );                    \
    }




/*********************/
/* AND - Logical AND */
/*********************/

static void
and_AL_imm8 (
             PROTO_INTERP
             )
{

  uint8_t val;

  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP8 ( &, AL, val );
  
} // end and_AL_imm8


static void
and_AX_imm16 (
              PROTO_INTERP
              )
{

  uint16_t val;

  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP16 ( &, AX, val );
  
} // end and_AX_imm16


static void
and_EAX_imm32 (
               PROTO_INTERP
               )
{

  uint32_t val;

  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP32 ( &, EAX, val );
  
} // end and_EAX_imm32


static void
and_A_imm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? and_EAX_imm32 ( INTERP ) : and_AX_imm16 ( INTERP );
} // end and_A_imm


static void
and_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  uint8_t val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &val, return );
  LOP_RM8_VAL8 ( &, dst, val, eaddr );
  
} // end and_rm8_imm8


static void
and_rm16_imm16 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{

  uint16_t val, dst;
  eaddr_r16_t eaddr;


  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  LOP_RM16_VAL16 ( &, dst, val, eaddr );
  
} // end and_rm16_imm16


static void
and_rm32_imm32 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{

  uint32_t val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  LOP_RM32_VAL32 ( &, dst, val, eaddr );
  
} // end and_rm32_imm32 


static void
and_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    and_rm32_imm32 ( INTERP, modrmbyte ) :
    and_rm16_imm16 ( INTERP, modrmbyte );
} // end and_rm_imm


static void
and_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint16_t val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  LOP_RM16_VAL16 ( &, dst, val, eaddr );
  
} // end and_rm16_imm8


static void
and_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint32_t val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  LOP_RM32_VAL32 ( &, dst, val, eaddr );
  
} // end and_rm32_imm8


static void
and_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    and_rm32_imm8 ( INTERP, modrmbyte ) :
    and_rm16_imm8 ( INTERP, modrmbyte );
} // end and_rm_imm8


static void
and_rm8_r8 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte;
  uint8_t dst, *reg;
  eaddr_r8_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM8_VAL8 ( &, dst, *reg, eaddr );
  
} // end and_rm8_r8


static void
and_rm16_r16 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint16_t dst, *reg;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM16_VAL16 ( &, dst, *reg, eaddr );
  
} // end and_rm16_r16


static void
and_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t dst, *reg;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM32_VAL32 ( &, dst, *reg, eaddr );
  
} // end and_rm32_r32


static void
and_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? and_rm32_r32 ( INTERP ) : and_rm16_r16 ( INTERP );
} // end and_rm_r


static void
and_r8_rm8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  uint8_t val, *reg;
  eaddr_r8_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP8 ( &, *reg, val );
  
} // end and_r8_rm8


static void
and_r16_rm16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t val, *reg;
  eaddr_r16_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP16 ( &, *reg, val );
  
} // end and_r16_rm16


static void
and_r32_rm32 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint32_t val, *reg;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP32 ( &, *reg, val );
  
} // end and_r32_rm32


static void
and_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? and_r32_rm32 ( INTERP ) : and_r16_rm16 ( INTERP );
} // end and_r_rm




/***********************************/
/* NOT - One's Complement Negation */
/***********************************/

static void
not_rm8 (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  
  uint8_t val;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
      val= ~val;
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else
    {
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
      *eaddr.v.reg= ~(*eaddr.v.reg);
    }
  
} // end not_rm8


static void
not_rm16 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint16_t val;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
      val= ~val;
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else
    {
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
      *eaddr.v.reg= ~(*eaddr.v.reg);
    }
  
} // end not_rm16


static void
not_rm32 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  uint32_t val;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
      val= ~val;
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else
    {
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
      *eaddr.v.reg= ~(*eaddr.v.reg);
    }
  
} // end not_rm32


static void
not_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    not_rm32 ( INTERP, modrmbyte ) :
    not_rm16 ( INTERP, modrmbyte );
} // end not_rm




/*****************************/
/* OR - Logical Inclusive OR */
/*****************************/

static void
or_AL_imm8 (
            PROTO_INTERP
            )
{

  uint8_t val;

  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP8 ( |, AL, val );
  
} // end or_AL_imm8


static void
or_AX_imm16 (
             PROTO_INTERP
             )
{

  uint16_t val;

  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP16 ( |, AX, val );
  
} // end or_AX_imm16


static void
or_EAX_imm32 (
              PROTO_INTERP
              )
{

  uint32_t val;

  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP32 ( |, EAX, val );
  
} // end or_EAX_imm32


static void
or_A_imm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? or_EAX_imm32 ( INTERP ) : or_AX_imm16 ( INTERP );
} // end or_A_imm


static void
or_rm8_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  uint8_t val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &val, return );
  LOP_RM8_VAL8 ( |, dst, val, eaddr );
  
} // end or_rm8_imm8


static void
or_rm16_imm16 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{
  
  uint16_t val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  LOP_RM16_VAL16 ( |, dst, val, eaddr );
  
} // end or_rm16_imm16


static void
or_rm32_imm32 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{
  
  uint32_t val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  LOP_RM32_VAL32 ( |, dst, val, eaddr );
  
} // end or_rm32_imm32 


static void
or_rm_imm (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    or_rm32_imm32 ( INTERP, modrmbyte ) :
    or_rm16_imm16 ( INTERP, modrmbyte );
} // end or_rm_imm


static void
or_rm16_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  int8_t data;
  uint16_t val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  LOP_RM16_VAL16 ( |, dst, val, eaddr );
  
} // end or_rm16_imm8


static void
or_rm32_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  int8_t data;
  uint32_t val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  LOP_RM32_VAL32 ( |, dst, val, eaddr );
  
} // end or_rm32_imm8


static void
or_rm_imm8 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    or_rm32_imm8 ( INTERP, modrmbyte ) :
    or_rm16_imm8 ( INTERP, modrmbyte );
} // end or_rm_imm8


static void
or_rm8_r8 (
           PROTO_INTERP
           )
{
  
  uint8_t modrmbyte;
  uint8_t dst, *reg;
  eaddr_r8_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM8_VAL8 ( |, dst, *reg, eaddr );
  
} // end or_rm8_r8


static void
or_rm16_r16 (
             PROTO_INTERP
             )
{
  
  uint8_t modrmbyte;
  uint16_t dst, *reg;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM16_VAL16 ( |, dst, *reg, eaddr );
  
} // end or_rm16_r16


static void
or_rm32_r32 (
             PROTO_INTERP
             )
{

  uint8_t modrmbyte;
  uint32_t dst, *reg;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM32_VAL32 ( |, dst, *reg, eaddr );
  
} // end or_rm32_r32


static void
or_rm_r (
         PROTO_INTERP
         )
{
  OPERAND_SIZE_IS_32 ? or_rm32_r32 ( INTERP ) : or_rm16_r16 ( INTERP );
} // end or_rm_r


static void
or_r8_rm8 (
           PROTO_INTERP
           )
{

  uint8_t modrmbyte;
  uint8_t val, *reg;
  eaddr_r8_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP8 ( |, *reg, val );
  
} // end or_r8_rm8


static void
or_r16_rm16 (
             PROTO_INTERP
             )
{

  uint8_t modrmbyte;
  uint16_t val, *reg;
  eaddr_r16_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP16 ( |, *reg, val );
  
} // end or_r16_rm16


static void
or_r32_rm32 (
             PROTO_INTERP
             )
{
  
  uint8_t modrmbyte;
  uint32_t val, *reg;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP32 ( |, *reg, val );
  
} // end or_r32_rm32


static void
or_r_rm (
         PROTO_INTERP
         )
{
  OPERAND_SIZE_IS_32 ? or_r32_rm32 ( INTERP ) : or_r16_rm16 ( INTERP );
} // end or_r_rm




/**************************/
/* TEST - Logical Compare */
/**************************/

#define TEST8(TMP,SRC1,SRC2)                                         \
  (TMP)= (SRC1) & (SRC2);                                            \
  SETU8_LOP_FLAGS ( (TMP) )

#define TEST16(TMP,SRC1,SRC2)                                        \
  (TMP)= (SRC1) & (SRC2);                                            \
  SETU16_LOP_FLAGS ( (TMP) )

#define TEST32(TMP,SRC1,SRC2)                                        \
  (TMP)= (SRC1) & (SRC2);                                            \
  SETU32_LOP_FLAGS ( (TMP) )

#define TEST_RM8_VAL8(TMP,SRC1,SRC2,EADDR)                              \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(SRC1), return ); \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      TEST8 ( (TMP), (SRC1), (SRC2) );                                  \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      TEST8 ( (TMP), *((EADDR).v.reg), (SRC2) );                        \
    }

#define TEST_RM16_VAL16(TMP,SRC1,SRC2,EADDR)                              \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(SRC1), return ); \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      TEST16 ( (TMP), (SRC1), (SRC2) );                                 \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      TEST16 ( (TMP), *((EADDR).v.reg), (SRC2) );                       \
    }

#define TEST_RM32_VAL32(TMP,SRC1,SRC2,EADDR)                            \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(SRC1), return ); \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      TEST32 ( (TMP), (SRC1), (SRC2) );                                 \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      TEST32 ( (TMP), *((EADDR).v.reg), (SRC2) );                       \
    }


static void
test_AL_imm8 (
              PROTO_INTERP
              )
{
  
  uint8_t tmp, val;
  
  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  TEST8 ( tmp, AL, val );
  
} // end test_AL_imm8


static void
test_AX_imm16 (
               PROTO_INTERP
               )
{
  
  uint16_t tmp, val;
  
  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  TEST16 ( tmp, AX, val );
  
} // end test_AX_imm16


static void
test_EAX_imm32 (
                PROTO_INTERP
                )
{
  
  uint32_t tmp, val;
  
  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  TEST32 ( tmp, EAX, val );
  
} // end test_EAX_imm32


static void
test_A_imm (
            PROTO_INTERP
            )
{
  OPERAND_SIZE_IS_32 ? test_EAX_imm32 ( INTERP ) : test_AX_imm16 ( INTERP );
} // end test_A_imm


static void
test_rm8_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{
  
  uint8_t tmp, val, src1;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  TEST_RM8_VAL8 ( tmp, src1, val, eaddr );
  
} // end test_rm8_imm8


static void
test_rm16_imm16 (
                 PROTO_INTERP,
                 const uint8_t modrmbyte
                 )
{

  uint16_t val, tmp, src1;
  eaddr_r16_t eaddr;


  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  TEST_RM16_VAL16 ( tmp, src1, val, eaddr );
  
} // end test_rm16_imm16


static void
test_rm32_imm32 (
                 PROTO_INTERP,
                 const uint8_t modrmbyte
                 )
{
  
  uint32_t tmp, val, src1;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  TEST_RM32_VAL32 ( tmp, src1, val, eaddr );
  
} // end test_rm32_imm32


static void
test_rm_imm (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    test_rm32_imm32 ( INTERP, modrmbyte ) :
    test_rm16_imm16 ( INTERP, modrmbyte );
} // end test_rm_imm


static void
test_rm8_r8 (
             PROTO_INTERP
             )
{

  uint8_t modrmbyte;
  uint8_t tmp, src1, *reg;
  eaddr_r8_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  TEST_RM8_VAL8 ( tmp, src1, *reg, eaddr );
  
} // end test_rm8_r8


static void
test_rm16_r16 (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte;
  uint16_t tmp, src1, *reg;
  eaddr_r16_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  TEST_RM16_VAL16 ( tmp, src1, *reg, eaddr );
  
} // end test_rm16_r16


static void
test_rm32_r32 (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte;
  uint32_t tmp, src1, *reg;
  eaddr_r32_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  TEST_RM32_VAL32 ( tmp, src1, *reg, eaddr );
  
} // end test_rm32_r32


static void
test_rm_r (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? test_rm32_r32 ( INTERP ) : test_rm16_r16 ( INTERP );
} // end test_rm_r



/******************************/
/* XOR - Logical Exclusive OR */
/******************************/

static void
xor_AL_imm8 (
             PROTO_INTERP
             )
{
  
  uint8_t val;
  
  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP8 ( ^, AL, val );
  
} // end xor_AL_imm8


static void
xor_AX_imm16 (
              PROTO_INTERP
              )
{

  uint16_t val;

  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP16 ( ^, AX, val );
  
} // end xor_AX_imm16


static void
xor_EAX_imm32 (
               PROTO_INTERP
               )
{

  uint32_t val;

  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  LOP32 ( ^, EAX, val );
  
} // end xor_EAX_imm32


static void
xor_A_imm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? xor_EAX_imm32 ( INTERP ) : xor_AX_imm16 ( INTERP );
} // end xor_A_imm


static void
xor_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t val, dst;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &val, return );
  LOP_RM8_VAL8 ( ^, dst, val, eaddr );
  
} // end xor_rm8_imm8


static void
xor_rm16_imm16 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{

  uint16_t val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  LOP_RM16_VAL16 ( ^, dst, val, eaddr );
  
} // end xor_rm16_imm16


static void
xor_rm32_imm32 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{

  uint32_t val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  LOP_RM32_VAL32 ( ^, dst, val, eaddr );
  
} // end xor_rm32_imm32 


static void
xor_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    xor_rm32_imm32 ( INTERP, modrmbyte ) :
    xor_rm16_imm16 ( INTERP, modrmbyte );
} // end xor_rm_imm


static void
xor_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint16_t val, dst;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  LOP_RM16_VAL16 ( ^, dst, val, eaddr );
  
} // end xor_rm16_imm8


static void
xor_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint32_t val, dst;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  LOP_RM32_VAL32 ( ^, dst, val, eaddr );
  
} // end xor_rm32_imm8


static void
xor_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    xor_rm32_imm8 ( INTERP, modrmbyte ) :
    xor_rm16_imm8 ( INTERP, modrmbyte );
} // end xor_rm_imm8


static void
xor_rm8_r8 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte;
  uint8_t dst, *reg;
  eaddr_r8_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM8_VAL8 ( ^, dst, *reg, eaddr );
  
} // end xor_rm8_r8


static void
xor_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t dst, *reg;
  eaddr_r16_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM16_VAL16 ( ^, dst, *reg, eaddr );
  
} // end xor_rm16_r16


static void
xor_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t dst, *reg;
  eaddr_r32_t eaddr;
  

  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  LOP_RM32_VAL32 ( ^, dst, *reg, eaddr );
  
} // end xor_rm32_r32


static void
xor_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? xor_rm32_r32 ( INTERP ) : xor_rm16_r16 ( INTERP );
} // end xor_rm_r


static void
xor_r8_rm8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  uint8_t val, *reg;
  eaddr_r8_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP8 ( ^, *reg, val );
  
} // end xor_r8_rm8


static void
xor_r16_rm16 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint16_t val, *reg;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP16 ( ^, *reg, val );
  
} // end xor_r16_rm16


static void
xor_r32_rm32 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint32_t val, *reg;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  LOP32 ( ^, *reg, val );
  
} // end xor_r32_rm32


static void
xor_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? xor_r32_rm32 ( INTERP ) : xor_r16_rm16 ( INTERP );
} // end xor_r_rm
