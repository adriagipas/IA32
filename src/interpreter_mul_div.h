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
 *  interpreter_mul_div.h - Implementa multiplicacions i divisions.
 *
 */




/*************************/
/* DIV - Unsigned Divide */
/*************************/

static void
div_rm8 (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  
  uint8_t src;
  uint16_t tmp,val;
  eaddr_r8_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Divideix
  if ( src == 0 ) { EXCEPTION ( EXCP_DE ); return; }
  val= (uint16_t) AX;
  tmp= val/((uint16_t) src);
  if ( tmp > 0xFF ) { EXCEPTION ( EXCP_DE ); return; }
  AL= (uint8_t) tmp;
  AH= (uint8_t) (val%((uint16_t) src));
  
} // end div_rm8


static void
div_rm16 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint16_t src;
  uint32_t val,tmp;
  eaddr_r16_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Divideix
  if ( src == 0 ) { EXCEPTION ( EXCP_DE ); return; }
  val= (((uint32_t) DX)<<16) | ((uint32_t) AX);
  tmp= val/((uint32_t) src);
  if ( tmp > 0xFFFF ) { EXCEPTION ( EXCP_DE ); return; }
  AX= (uint16_t) tmp;
  DX= (uint16_t) (val%((uint32_t) src));
  
} // end div_rm16


static void
div_rm32 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint32_t src;
  uint64_t val,tmp;
  eaddr_r32_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Divideix
  if ( src == 0 ) { EXCEPTION ( EXCP_DE ); return; }
  val= (((uint64_t) EDX)<<32) | ((uint64_t) EAX);
  tmp= val/((uint64_t) src);
  if ( tmp > 0xFFFFFFFF ) { EXCEPTION ( EXCP_DE ); return; }
  EAX= (uint32_t) tmp;
  EDX= (uint32_t) (val%((uint64_t) src));
  
} // end div_rm32


static void
div_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    div_rm32 ( INTERP, modrmbyte ) :
    div_rm16 ( INTERP, modrmbyte );
} // end div_rm




/************************/
/* IDIV - Signed Divide */
/************************/

static void
idiv_rm8 (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  
  uint8_t src;
  uint16_t tmp,val;
  eaddr_r8_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Divideix
  if ( src == 0 ) { EXCEPTION ( EXCP_DE ); return; }
  val= (uint16_t) ((int16_t) ((int8_t) AX));
  tmp= ((int16_t) val)/((int16_t) ((int8_t) src));
  if ( ((int16_t) tmp) > ((int16_t) ((int8_t) 0x7f)) ||
       ((int16_t) tmp) < ((int16_t) ((int8_t) 0x80)) )
    { EXCEPTION ( EXCP_DE ); return; }
  AL= (uint8_t) tmp;
  AH= (uint8_t) (((int16_t) val)%((int16_t) ((int8_t) src)));
  
} // end idiv_rm8


static void
idiv_rm16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint16_t src;
  uint32_t val,tmp;
  eaddr_r16_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;
  
  // Divideix
  if ( src == 0 ) { EXCEPTION ( EXCP_DE ); return; }
  val= (((uint32_t) DX)<<16) | ((uint32_t) AX);
  tmp= ((int32_t) val)/((int32_t) ((int16_t) src));
  if ( ((int32_t) tmp) > ((int32_t) ((int16_t) 0x7fff)) ||
       ((int32_t) tmp) < ((int32_t) ((int16_t) 0x8000)) )
    { EXCEPTION ( EXCP_DE ); return; }
  AX= (uint16_t) tmp;
  DX= (uint16_t) (((int32_t) val)%((int32_t) ((int16_t) src)));
  
} // end idiv_rm16


static void
idiv_rm32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint32_t src;
  uint64_t val,tmp;
  eaddr_r32_t eaddr;
  
  
  // Obté src.
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;
  
  // Divideix
  if ( src == 0 ) { EXCEPTION ( EXCP_DE ); return; }
  val= (((uint64_t) EDX)<<32) | ((uint64_t) EAX);
  tmp= ((int64_t) val)/((int64_t) ((int32_t) src));
  if ( ((int64_t) tmp) > ((int64_t) ((int32_t) 0x7fffffff)) ||
       ((int64_t) tmp) < ((int64_t) ((int32_t) 0x80000000)) )
    { EXCEPTION ( EXCP_DE ); return; }
  EAX= (uint32_t) tmp;
  EDX= (uint32_t) (((int64_t) val)%((int64_t) ((int32_t) src)));
  
} // end idiv_rm32


static void
idiv_rm (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  OPERAND_SIZE_IS_32 ?
    idiv_rm32 ( INTERP, modrmbyte ) :
    idiv_rm16 ( INTERP, modrmbyte );
} // end idiv_rm




/**************************/
/* IMUL - Signed Multiply */
/**************************/

#define IMUL16(DST,SRC1,SRC2,TMP32)                                     \
  (TMP32)= (uint32_t) (((int32_t) ((int16_t) (SRC1))) *                 \
                       ((int32_t) ((int16_t) (SRC2))));                 \
  (DST)= (uint16_t) ((TMP32)&0xFFFF);                                   \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|CF_FLAG);                                  \
  EFLAGS|=                                                              \
    (((DST)&0x8000) ? SF_FLAG : 0) |                                    \
    (((((DST)&0x8000)!=0)^(((TMP32)&0x80000000)!=0)) ? (CF_FLAG|OF_FLAG) : 0)

#define IMUL32(DST,SRC1,SRC2,TMP64)                                     \
  (TMP64)= (uint64_t) (((int64_t) ((int32_t) (SRC1))) *                 \
                       ((int64_t) ((int32_t) (SRC2))));                 \
  (DST)= (uint32_t) ((TMP64)&0xFFFFFFFF);                               \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|CF_FLAG);                                  \
  EFLAGS|=                                                              \
    (((DST)&0x80000000) ? SF_FLAG : 0) |                                \
    (((((DST)&0x80000000)!=0)^(((TMP64)&0x8000000000000000)!=0)) ?      \
     (CF_FLAG|OF_FLAG) : 0)

#define IMUL_DST_RM16_VAL16(DST,VOP1,VAL,TMP32,EADDR)                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(VOP1), return ); \
      IMUL16 ( (DST), (VOP1), (VAL), (TMP32) );                         \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      IMUL16 ( (DST), *((EADDR).v.reg), (VAL), (TMP32) );               \
    }

#define IMUL_DST_RM32_VAL32(DST,VOP1,VAL,TMP64,EADDR)                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(VOP1), return ); \
      IMUL32 ( (DST), (VOP1), (VAL), (TMP64) );                         \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      IMUL32 ( (DST), *((EADDR).v.reg), (VAL), (TMP64) );               \
    }

#define IMUL32_OP2(DST,SRC,TMP64)                                       \
  (TMP64)= (uint64_t) ((int64_t) ((int32_t) (DST))) *                   \
    ((int64_t) ((int32_t) (SRC)));                                      \
  (DST)= (uint32_t) ((TMP64)&0xFFFFFFFF);                               \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|CF_FLAG);                                  \
  EFLAGS|=                                                              \
    (((DST)&0x80000000) ? SF_FLAG : 0) |                                \
    (((((DST)&0x80000000)!=0)^(((TMP64)&0x8000000000000000)!=0)) ?      \
     (CF_FLAG|OF_FLAG) : 0)

#define IMUL16_OP2(DST,SRC,TMP32)                                       \
  (TMP32)= (uint32_t) ((int32_t) ((int16_t) (DST))) *                   \
    ((int32_t) ((int16_t) (SRC)));                                      \
  (DST)= (uint16_t) ((TMP32)&0xFFFF);                                   \
  EFLAGS&= ~(OF_FLAG|SF_FLAG|CF_FLAG);                                  \
  EFLAGS|=                                                              \
    (((DST)&0x8000) ? SF_FLAG : 0) |                                    \
    (((((DST)&0x8000)!=0)^(((TMP32)&0x80000000)!=0)) ? (CF_FLAG|OF_FLAG) : 0)

static void
imul_rm8 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint8_t src;
  eaddr_r8_t eaddr;
  
  
  // Obté src.
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Multiplica
  AX= (uint16_t) (((int16_t) ((int8_t) AL)) * ((int16_t) ((int8_t) src)));
  EFLAGS&= ~(OF_FLAG|SF_FLAG|CF_FLAG);
  EFLAGS|=
    ((AX&0x0080) ? SF_FLAG : 0 ) |
    (((uint16_t) ((int16_t) ((int8_t) (AX&0xFF))))!=AX ? (CF_FLAG|OF_FLAG) : 0);
  
} // end imul_rm8


static void
imul_rm16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint16_t src;
  eaddr_r16_t eaddr;
  uint32_t tmp;
  
  
  // Obté src.
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;
  
  // Multiplica
  tmp= (uint32_t) (((int32_t) ((int16_t) AX)) * ((int32_t) ((int16_t) src)));
  DX= (uint16_t) (tmp>>16);
  AX= (uint16_t) (tmp&0xFFFF);
  EFLAGS&= ~(OF_FLAG|SF_FLAG|CF_FLAG);
  EFLAGS|=
    ((tmp&0x8000) ? SF_FLAG : 0 ) |
    (((uint32_t) ((int32_t) ((int16_t) (tmp&0xFFFF))))!=tmp ?
     (CF_FLAG|OF_FLAG) : 0);
  
} // end imul_rm16


static void
imul_rm32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint32_t src;
  eaddr_r32_t eaddr;
  uint64_t tmp;
  
  
  // Obté src.
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;
  
  // Multiplica
  tmp= (uint64_t) (((int64_t) ((int32_t) EAX)) * ((int64_t) ((int32_t) src)));
  EDX= (uint32_t) (tmp>>32);
  EAX= (uint32_t) (tmp&0xFFFFFFFF);
  // NOTA!!! En el manual fica:
  //   SF ← TMP_XP[32];
  // però crec que és una errata. Per coherència deuria ser:
  //   SF ← TMP_XP[31];
  EFLAGS&= ~(OF_FLAG|SF_FLAG|CF_FLAG);
  EFLAGS|=
    ((tmp&0x80000000) ? SF_FLAG : 0 ) |
    (((uint64_t) ((int64_t) ((int32_t) (tmp&0xFFFFFFFF))))!=tmp ?
     (CF_FLAG|OF_FLAG) : 0);
  
} // end imul_rm32


static void
imul_rm (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  OPERAND_SIZE_IS_32 ?
    imul_rm32 ( INTERP, modrmbyte ) :
    imul_rm16 ( INTERP, modrmbyte );
} // end imul_rm


static void
imul_r16_rm16 (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte;
  uint16_t val, *reg;
  eaddr_r16_t eaddr;
  uint32_t tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  IMUL16_OP2 ( *reg, val, tmp );
  
} // end imul_r16_rm16


static void
imul_r32_rm32 (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte;
  uint32_t val, *reg;
  eaddr_r32_t eaddr;
  uint64_t tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  IMUL32_OP2 ( *reg, val, tmp );
  
} // end imul_r32_rm32


static void
imul_r_rm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? imul_r32_rm32 ( INTERP ) : imul_r16_rm16 ( INTERP );
} // end imul_r_rm


static void
imul_r16_rm16_imm8 (
                    PROTO_INTERP
                    )
{
  
  uint8_t modrmbyte,imm;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  uint16_t op3,op1;
  uint32_t tmp;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  IB ( &imm, return );
  op3= (uint16_t) ((int16_t) ((int8_t) imm));
  IMUL_DST_RM16_VAL16 ( *reg, op1, op3, tmp, eaddr );
  
} // end imul_r16_rm16_imm8


static void
imul_r32_rm32_imm8 (
                    PROTO_INTERP
                    )
{
  
  uint8_t modrmbyte,imm;
  eaddr_r32_t eaddr;
  uint32_t *reg;
  uint32_t op3,op1;
  uint64_t tmp;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  IB ( &imm, return );
  op3= (uint32_t) ((int32_t) ((int8_t) imm));
  IMUL_DST_RM32_VAL32 ( *reg, op1, op3, tmp, eaddr );
  
} // end imul_r32_rm32_imm8


static void
imul_r_rm_imm8 (
                PROTO_INTERP
                )
{
  OPERAND_SIZE_IS_32 ?
    imul_r32_rm32_imm8 ( INTERP ) :
    imul_r16_rm16_imm8 ( INTERP );
} // end imul_r_rm_imm8


static void
imul_r16_rm16_imm16 (
                     PROTO_INTERP
                     )
{
  
  uint8_t modrmbyte;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  uint16_t op3,op1;
  uint32_t tmp;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  IW ( &op3, return );
  IMUL_DST_RM16_VAL16 ( *reg, op1, op3, tmp, eaddr );
  
} // end imul_r16_rm16_imm16


static void
imul_r32_rm32_imm32 (
                     PROTO_INTERP
                     )
{
  
  uint8_t modrmbyte;
  eaddr_r32_t eaddr;
  uint32_t *reg;
  uint32_t op3,op1;
  uint64_t tmp;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  ID ( &op3, return );
  IMUL_DST_RM32_VAL32 ( *reg, op1, op3, tmp, eaddr );
  
} // end imul_r32_rm32_imm32


static void
imul_r_rm_imm (
                PROTO_INTERP
                )
{
  OPERAND_SIZE_IS_32 ?
    imul_r32_rm32_imm32 ( INTERP ) :
    imul_r16_rm16_imm16 ( INTERP );
} // end imul_r_rm_imm




/***************************/
/* MUL - Unsigned Multiply */
/***************************/

static void
mul_rm8 (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  
  uint8_t src;
  eaddr_r8_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Multiplica
  AX= ((uint16_t) AL) * ((uint16_t) src);
  if ( (AX&0xFF00) == 0 ) EFLAGS&= ~(OF_FLAG|CF_FLAG);
  else                    EFLAGS|= (OF_FLAG|CF_FLAG);
  
} // end mul_rm8


static void
mul_rm16 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint16_t src;
  uint32_t tmp;
  eaddr_r16_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Multiplica
  tmp= ((uint32_t) AX) * ((uint32_t) src);
  DX= (uint16_t) (tmp>>16);
  AX= (uint16_t) (tmp&0xFFFF);
  if ( DX == 0 ) EFLAGS&= ~(OF_FLAG|CF_FLAG);
  else           EFLAGS|= (OF_FLAG|CF_FLAG);
  
} // end mul_rm16


static void
mul_rm32 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint32_t src;
  uint64_t tmp;
  eaddr_r32_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &src, return );
    }
  else src= *eaddr.v.reg;

  // Multiplica
  tmp= ((uint64_t) EAX) * ((uint64_t) src);
  EDX= (uint32_t) (tmp>>32);
  EAX= (uint32_t) (tmp&0xFFFFFFFF);
  if ( EDX == 0 ) EFLAGS&= ~(OF_FLAG|CF_FLAG);
  else            EFLAGS|= (OF_FLAG|CF_FLAG);
  
} // end mul_rm32


static void
mul_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    mul_rm32 ( INTERP, modrmbyte ) :
    mul_rm16 ( INTERP, modrmbyte );
} // end mul_rm
