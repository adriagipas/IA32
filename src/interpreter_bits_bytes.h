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
 *  interpreter_bits_bytes.h - Operacions bits/bytes.
 *
 */


/**************************/
/* BSF - Bit Scan Forward */
/**************************/

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


static void
bsf_r16_rm16 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint16_t tmp, val, *reg;
  eaddr_r16_t eaddr;

  
  if ( REP_ENABLED ) WW ( UDATA, "TZCNT not supported" );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  
  if ( val == 0 ) { EFLAGS|= ZF_FLAG; }
  else
    {
      tmp= BSF_TABLE[val&0xFF];
      if ( tmp == 8 ) tmp+= BSF_TABLE[val>>8];
      *reg= tmp;
      EFLAGS&= ~ZF_FLAG;
    }
  
} // end bsf_r6_rm16


static void
bsf_r32_rm32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp, val, *reg;
  eaddr_r32_t eaddr;
  

  if ( REP_ENABLED ) WW ( UDATA, "TZCNT not supported" );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);

  if ( val == 0 ) { EFLAGS|= ZF_FLAG; }
  else
    {
      tmp= BSF_TABLE[val&0xFF];
      if ( tmp == 8 )
        {
          tmp+= BSF_TABLE[(val>>8)&0xFF];
          if ( tmp == 16 )
            {
              tmp+= BSF_TABLE[(val>>16)&0xFF];
              if ( tmp == 24 ) tmp+= BSF_TABLE[val>>24];
            }
        }
      *reg= tmp;
      EFLAGS&= ~ZF_FLAG;
    }
  
} // end bsf_r32_rm32 */


static void
bsf_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? bsf_r32_rm32 ( INTERP ) : bsf_r16_rm16 ( INTERP );
} // end bsf_r_rm




/**************************/
/* BSR - Bit Scan Reverse */
/**************************/

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


static void
bsr_r16_rm16 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  uint16_t tmp, val, *reg;
  eaddr_r16_t eaddr;
  

  if ( REP_ENABLED ) WW ( UDATA, "LZCNT not supported" );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  
  if ( val == 0 ) { EFLAGS|= ZF_FLAG; }
  else
    {
      tmp= 15 - BSR_TABLE[val>>8];
      if ( tmp == 7 ) tmp-= BSR_TABLE[val&0xFF];
      *reg= tmp;
      EFLAGS&= ~ZF_FLAG;
    }
  
} // end bsr_r6_rm16


static void
bsr_r32_rm32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp, val, *reg;
  eaddr_r32_t eaddr;
  

  if ( REP_ENABLED ) WW ( UDATA, "LZCNT not supported" );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);

  if ( val == 0 ) { EFLAGS|= ZF_FLAG; }
  else
    {
      tmp= 31 - BSR_TABLE[val>>24];
      if ( tmp == 23 )
        {
          tmp-= BSR_TABLE[(val>>16)&0xFF];
          if ( tmp == 15 )
            {
              tmp-= BSR_TABLE[(val>>8)&0xFF];
              if ( tmp == 7 ) tmp-= BSR_TABLE[val&0xFF];
            }
        }
      *reg= tmp;
      EFLAGS&= ~ZF_FLAG;
    }
  
} // end bsr_r32_rm32


static void
bsr_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? bsr_r32_rm32 ( INTERP ) : bsr_r16_rm16 ( INTERP );
} // end bsr_r_rm




/*********************/
/* BSWAP - Byte Swap */
/*********************/

static void
bswap_r (
         PROTO_INTERP,
         const uint8_t opcode
         )
{

  uint32_t *reg;
  uint32_t tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  switch ( opcode&0x7 )
    {
    case 0: reg= &EAX; break;
    case 1: reg= &ECX; break;
    case 2: reg= &EDX; break;
    case 3: reg= &EBX; break;
    case 4: reg= &ESP; break;
    case 5: reg= &EBP; break;
    case 6: reg= &ESI; break;
    case 7: reg= &EDI; break;
    }

  tmp= *reg;
  *reg=
    (tmp<<24) |
    ((tmp<<8)&0x00FF0000) |
    ((tmp>>8)&0x0000FF00) |
    (tmp>>24);
  
} // end bswap_r




/*****************/
/* BT - Bit Test */
/*****************/

#define BT_RM16_VAL16(VAL,EADDR,TMPI,TMPA,TMPD)        			\
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%16;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off +        				\
        (uint32_t) ((((int32_t) ((int16_t) (VAL)))>>4)<<1);		\
      READW_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return ); \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
    }        								\
  else        								\
    {        								\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }       \
    }

#define BT_RM32_VAL32(VAL,EADDR,TMPI,TMPA,TMPD)        			\
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%32;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off + (uint32_t) ((((int32_t) (VAL))>>5)<<2); \
      READD_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return );       \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
    }        								\
  else        								\
    {        								\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }       \
    }

static void
bt_rm16_r16 (
             PROTO_INTERP
             )
{

  uint8_t modrmbyte;
  uint16_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BT_RM16_VAL16 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bt_rm16_r16


static void
bt_rm32_r32 (
             PROTO_INTERP
             )
{

  uint8_t modrmbyte;
  uint32_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BT_RM32_VAL32 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bt_rm32_r32


static void
bt_rm_r (
         PROTO_INTERP
         )
{
  OPERAND_SIZE_IS_32 ? bt_rm32_r32 ( INTERP ) : bt_rm16_r16 ( INTERP );
} // end bt_rm_r


static void
bt_rm16_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  int8_t data;
  uint16_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  BT_RM16_VAL16 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bt_rm16_imm8


static void
bt_rm32_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  int8_t data;
  uint32_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  BT_RM32_VAL32 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bt_rm32_imm8


static void
bt_rm_imm8 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    bt_rm32_imm8 ( INTERP, modrmbyte ) :
    bt_rm16_imm8 ( INTERP, modrmbyte );
} // end bt_rm_imm8




/*********************************/
/* BTC - Bit Test and Complement */
/*********************************/

#define BTC_RM16_VAL16(VAL,EADDR,TMPI,TMPA,TMPD)        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%16;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off +        				\
        (uint32_t) ((((int32_t) ((int16_t) (VAL)))>>4)<<1);		\
      READW_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return );       \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
      (TMPD)^= (1<<(TMPI));        					\
      WRITEW ( (EADDR).v.addr.seg, (TMPA), (TMPD), return );        	\
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        \
      (*((EADDR).v.reg))^= (1<<(TMPI));        				\
    }

#define BTC_RM32_VAL32(VAL,EADDR,TMPI,TMPA,TMPD)        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%32;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off + (uint32_t) ((((int32_t) (VAL))>>5)<<2); \
      READD_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return );        \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
      (TMPD)^= (1<<(TMPI));        					\
      WRITED ( (EADDR).v.addr.seg, (TMPA), (TMPD), return );        	\
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        \
      (*((EADDR).v.reg))^= (1<<(TMPI));        				\
    }

#if 0
static void
btc_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BTC_RM16_VAL16 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} /* end btc_rm16_r16 */


static void
btc_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BTC_RM32_VAL32 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} /* end btc_rm32_r32 */


static void
btc_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? btc_rm32_r32 ( INTERP ) : btc_rm16_r16 ( INTERP );
} /* end btc_rm_r */
#endif

static void
btc_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint16_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  BTC_RM16_VAL16 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end btc_rm16_imm8


static void
btc_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint32_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  BTC_RM32_VAL32 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end btc_rm32_imm8


static void
btc_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    btc_rm32_imm8 ( INTERP, modrmbyte ) :
    btc_rm16_imm8 ( INTERP, modrmbyte );
} // end btc_rm_imm8


/****************************/
/* BTR - Bit Test and Reset */
/****************************/

#define BTR_RM16_VAL16(VAL,EADDR,TMPI,TMPA,TMPD)        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%16;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off +        				\
        (uint32_t) ((((int32_t) ((int16_t) (VAL)))>>4)<<1);		\
      READW_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return );       \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
      (TMPD)&= ~(1<<(TMPI));        					\
      WRITEW ( (EADDR).v.addr.seg, (TMPA), (TMPD), return );        	\
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        \
      (*((EADDR).v.reg))&= ~(1<<(TMPI));        			\
    }

#define BTR_RM32_VAL32(VAL,EADDR,TMPI,TMPA,TMPD)        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%32;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off + (uint32_t) ((((int32_t) (VAL))>>5)<<2); \
      READD_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return );        \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
      (TMPD)&= ~(1<<(TMPI));        					\
      WRITED ( (EADDR).v.addr.seg, (TMPA), (TMPD), return );        	\
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        \
      (*((EADDR).v.reg))&= ~(1<<(TMPI));        			\
    }

static void
btr_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BTR_RM16_VAL16 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end btr_rm16_r16


static void
btr_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BTR_RM32_VAL32 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end btr_rm32_r32


static void
btr_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? btr_rm32_r32 ( INTERP ) : btr_rm16_r16 ( INTERP );
} // end btr_rm_r


static void
btr_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{
  
  int8_t data;
  uint16_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  BTR_RM16_VAL16 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end btr_rm16_imm8


static void
btr_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint32_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  BTR_RM32_VAL32 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end btr_rm32_imm8


static void
btr_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    btr_rm32_imm8 ( INTERP, modrmbyte ) :
    btr_rm16_imm8 ( INTERP, modrmbyte );
} // end btr_rm_imm8




/**************************/
/* BTS - Bit Test and Set */
/**************************/

#define BTS_RM16_VAL16(VAL,EADDR,TMPI,TMPA,TMPD)        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%16;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off +        				\
        (uint32_t) ((((int32_t) ((int16_t) (VAL)))>>4)<<1);		\
      READW_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return );       \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
      (TMPD)|= (1<<(TMPI));        					\
      WRITEW ( (EADDR).v.addr.seg, (TMPA), (TMPD), return );        	\
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        \
      (*((EADDR).v.reg))|= (1<<(TMPI));         			\
    }

#define BTS_RM32_VAL32(VAL,EADDR,TMPI,TMPA,TMPD)        		\
  EFLAGS&= ~CF_FLAG;        						\
  (TMPI)= (VAL)%32;        						\
  if ( (EADDR).is_addr )        					\
    {        								\
      (TMPA)= (EADDR).v.addr.off + (uint32_t) ((((int32_t) (VAL))>>5)<<2); \
      READD_DATA ( (EADDR).v.addr.seg, (TMPA), &(TMPD), return );        \
      if ( (TMPD)&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        		\
      (TMPD)|= (1<<(TMPI));        					\
      WRITED ( (EADDR).v.addr.seg, (TMPA), (TMPD), return );        	\
    }        								\
  else        								\
    {        								\
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }        		\
      if ( (*((EADDR).v.reg))&(1<<(TMPI)) ) { EFLAGS|= CF_FLAG; }        \
      (*((EADDR).v.reg))|= (1<<(TMPI));         			\
    }

static void
bts_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint16_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BTS_RM16_VAL16 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bts_rm16_r16


static void
bts_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  uint32_t tmp_ind, tmp_dst, *reg;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  BTS_RM32_VAL32 ( *reg, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bts_rm32_r32


static void
bts_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? bts_rm32_r32 ( INTERP ) : bts_rm16_r16 ( INTERP );
} // end bts_rm_r


static void
bts_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint16_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint16_t) ((int16_t) data);
  BTS_RM16_VAL16 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bts_rm16_imm8


static void
bts_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  int8_t data;
  uint32_t tmp_ind, tmp_dst, val;
  uint32_t tmp_addr;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &data, return );
  val= (uint32_t) ((int32_t) data);
  BTS_RM32_VAL32 ( val, eaddr, tmp_ind, tmp_addr, tmp_dst );
  
} // end bts_rm32_imm8


static void
bts_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    bts_rm32_imm8 ( INTERP, modrmbyte ) :
    bts_rm16_imm8 ( INTERP, modrmbyte );
} // end bts_rm_imm8

