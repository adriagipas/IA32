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
 *  interpreter_shift_rols.h - Shift, rol i operacions similars.
 *
 */




/***************************/
/* SAL/SAR/SHL/SHR - Shift */
/***************************/

#define SET_SAHLR_FLAGS(RES,MBIT)                                       \
  EFLAGS&= ~(SF_FLAG|ZF_FLAG|PF_FLAG);                                  \
  EFLAGS|=                                                              \
    ((((RES)&(MBIT)) ? SF_FLAG : 0) |                                   \
     (((RES)==0) ? ZF_FLAG : 0) |                                       \
     PFLAG[(uint8_t) ((RES)&0xFF)])

#define SETU8_SAHLR_FLAGS(RES)                  \
  SET_SAHLR_FLAGS(RES,0x80)

#define SETU16_SAHLR_FLAGS(RES)                 \
  SET_SAHLR_FLAGS(RES,0x8000)

#define SETU32_SAHLR_FLAGS(RES)                 \
  SET_SAHLR_FLAGS(RES,0x80000000)

#define SHR8(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (DST)&0x80 ) EFLAGS|= OF_FLAG;                               \
      else              EFLAGS&= ~OF_FLAG;                              \
    }                                                                   \
  if ( (COUNT_VAR) > 8  )                                               \
    {                                                                   \
      EFLAGS&= ~CF_FLAG;                                                \
      (DST)= 0;                                                         \
    }                                                                   \
  else if ( (COUNT_VAR) > 0 )                                           \
    {                                                                   \
      if ( (DST)&(1<<((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;               \
      else                              EFLAGS&= ~CF_FLAG;              \
      (DST)= (COUNT_VAR)==8 ? 0 : ((DST)>>(COUNT_VAR));                 \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU8_SAHLR_FLAGS ( (DST) ); }

#define SHR16(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (DST)&0x8000 ) EFLAGS|= OF_FLAG;                             \
      else                EFLAGS&= ~OF_FLAG;                            \
    }                                                                   \
  if ( (COUNT_VAR) > 16  )                                              \
    {                                                                   \
      EFLAGS&= ~CF_FLAG;                                                \
      (DST)= 0;                                                         \
    }                                                                   \
  else if ( (COUNT_VAR) > 0 )                                           \
    {                                                                   \
      if ( (DST)&(1<<((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;               \
      else                              EFLAGS&= ~CF_FLAG;              \
      (DST)= (COUNT_VAR)==16 ? 0 : ((DST)>>(COUNT_VAR));                \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU16_SAHLR_FLAGS ( (DST) ); }

#define SHR32(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (DST)&0x80000000 ) EFLAGS|= OF_FLAG;                         \
      else                    EFLAGS&= ~OF_FLAG;                        \
    }                                                                   \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (DST)&(1<<((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;               \
      else                              EFLAGS&= ~CF_FLAG;              \
      (DST)= (COUNT_VAR)==32 ? 0 : ((DST)>>(COUNT_VAR));                \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU32_SAHLR_FLAGS ( (DST) ); }

#define SHR_RM8(DST,COUNT_VAR,EADDR)                                    \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SHR8 ( (DST), (COUNT_VAR) );                                      \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHR8 ( *((EADDR).v.reg), (COUNT_VAR) );                           \
    }

#define SHR_RM16(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SHR16 ( (DST), (COUNT_VAR) );                                     \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHR16 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }

#define SHR_RM32(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SHR32 ( (DST), (COUNT_VAR) );                                     \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      SHR32 ( *((EADDR).v.reg), (COUNT_VAR) );                  \
    }

#define SAR8(DST,COUNT_VAR)                                             \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      EFLAGS&= ~OF_FLAG;                                                \
    }                                                                   \
  if ( (COUNT_VAR) >= 8  )                                              \
    {                                                                   \
      if ( (DST)&0x80 ) { EFLAGS|= CF_FLAG; (DST)= 0xFF; }              \
      else              { EFLAGS&= ~CF_FLAG; (DST)= 0; }                \
    }                                                                   \
  else if ( (COUNT_VAR) > 0 )                                           \
    {                                                                   \
      if ( (DST)&(1<<((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;               \
      else                              EFLAGS&= ~CF_FLAG;              \
      (DST)= (uint8_t) (((int8_t) (DST))>>(COUNT_VAR));                 \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU8_SAHLR_FLAGS ( (DST) ); }

#define SAR16(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      EFLAGS&= ~OF_FLAG;                                                \
    }                                                                   \
  if ( (COUNT_VAR) >= 16  )                                             \
    {                                                                   \
      if ( (DST)&0x8000 ) { EFLAGS|= CF_FLAG; (DST)= 0xFFFF; }          \
      else                { EFLAGS&= ~CF_FLAG; (DST)= 0; }              \
    }                                                                   \
  else if ( (COUNT_VAR) > 0 )                                           \
    {                                                                   \
      if ( (DST)&(1<<((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;               \
      else                              EFLAGS&= ~CF_FLAG;              \
      (DST)= (uint16_t) (((int16_t) (DST))>>(COUNT_VAR));               \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU16_SAHLR_FLAGS ( (DST) ); }

#define SAR32(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      EFLAGS&= ~OF_FLAG;                                                \
    }                                                                   \
  if ( (COUNT_VAR) == 32  )                                             \
    {                                                                   \
      if ( (DST)&0x80000000 ) { EFLAGS|= CF_FLAG; (DST)= 0xFFFFFFFF; }  \
      else                    { EFLAGS&= ~CF_FLAG; (DST)= 0; }          \
    }                                                                   \
  else if ( (COUNT_VAR) > 0 )                                           \
    {                                                                   \
      if ( (DST)&(1<<((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;               \
      else                              EFLAGS&= ~CF_FLAG;              \
      (DST)= (uint32_t) (((int32_t) (DST))>>(COUNT_VAR));               \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU32_SAHLR_FLAGS ( (DST) ); }

#define SAR_RM8(DST,COUNT_VAR,EADDR)                                    \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SAR8 ( (DST), (COUNT_VAR) );                                      \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SAR8 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }

#define SAR_RM16(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SAR16 ( (DST), (COUNT_VAR) );                                     \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SAR16 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }

#define SAR_RM32(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SAR32 ( (DST), (COUNT_VAR) );                                     \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      SAR32 ( *((EADDR).v.reg), (COUNT_VAR) );                  \
    }

#define SHL8(DST,COUNT_VAR)                                             \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80)^(((DST)&0x40)<<1))!=0 ) EFLAGS|= OF_FLAG;      \
      else                                       EFLAGS&= ~OF_FLAG;     \
    }                                                                   \
  if ( (COUNT_VAR) > 8  )                                               \
    {                                                                   \
      EFLAGS&= ~CF_FLAG;                                                \
      (DST)= 0;                                                         \
    }                                                                   \
  else if ( (COUNT_VAR) > 0 )                                           \
    {                                                                   \
      if ( (DST)&(0x80>>((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;            \
      else                                 EFLAGS&= ~CF_FLAG;           \
      (DST)= (COUNT_VAR)==8 ? 0 : ((DST)<<(COUNT_VAR));                 \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU8_SAHLR_FLAGS ( (DST) ); }

#define SHL16(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x8000)^(((DST)&0x4000)<<1))!=0 ) EFLAGS|= OF_FLAG;    \
      else                                           EFLAGS&= ~OF_FLAG;   \
    }                                                                   \
  if ( (COUNT_VAR) > 16  )                                              \
    {                                                                   \
      EFLAGS&= ~CF_FLAG;                                                \
      (DST)= 0;                                                         \
    }                                                                   \
  else if ( (COUNT_VAR) > 0 )                                           \
    {                                                                   \
      if ( (DST)&(0x8000>>((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;          \
      else                                   EFLAGS&= ~CF_FLAG;         \
      (DST)= (COUNT_VAR)==16 ? 0 : ((DST)<<(COUNT_VAR));                \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU16_SAHLR_FLAGS ( (DST) ); }

#define SHL32(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80000000)^(((DST)&0x40000000)<<1))!=0 ) EFLAGS|= OF_FLAG; \
      else                                                   EFLAGS&= ~OF_FLAG;\
    }                                                                   \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (DST)&(0x80000000>>((COUNT_VAR)-1)) ) EFLAGS|= CF_FLAG;      \
      else                                       EFLAGS&= ~CF_FLAG;     \
      (DST)= (COUNT_VAR)==32 ? 0 : ((DST)<<(COUNT_VAR));                \
    }                                                                   \
  if ( (COUNT_VAR) != 0 ) { SETU32_SAHLR_FLAGS ( (DST) ); }

#define SHL_RM8(DST,COUNT_VAR,EADDR)                                    \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SHL8 ( (DST), (COUNT_VAR) );                                      \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHL8 ( *((EADDR).v.reg), (COUNT_VAR) );                           \
    }

#define SHL_RM16(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SHL16 ( (DST), (COUNT_VAR) );                                     \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHL16 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }

#define SHL_RM32(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      SHL32 ( (DST), (COUNT_VAR) );                                     \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      SHL32 ( *((EADDR).v.reg), (COUNT_VAR) );                  \
    }


static void
shl_rm8_1 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SHL_RM8 ( dst, count, eaddr );
  
} // end shl_rm8_1


static void
shl_rm8_CL (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SHL_RM8 ( dst, count, eaddr );
  
} // end shl_rm8_CL


static void
shl_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{

  uint8_t data;
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;


  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SHL_RM8 ( dst, count, eaddr );
  
} // end shl_rm8_imm8


static void
shl_rm16_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SHL_RM16 ( dst, count, eaddr );
  
} // end shl_rm16_1


static void
shl_rm32_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SHL_RM32 ( dst, count, eaddr );
  
} // end shl_rm32_1


static void
shl_rm_1 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  OPERAND_SIZE_IS_32 ?
    shl_rm32_1 ( INTERP, modrmbyte ) :
    shl_rm16_1 ( INTERP, modrmbyte );
} // end shl_rm_1


static void
shl_rm16_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SHL_RM16 ( dst, count, eaddr );
  
} // end shl_rm16_CL


static void
shl_rm32_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SHL_RM32 ( dst, count, eaddr );
  
} // end shl_rm32_CL


static void
shl_rm_CL (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    shl_rm32_CL ( INTERP, modrmbyte ) :
    shl_rm16_CL ( INTERP, modrmbyte );
} // end shl_rm_CL


static void
shl_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SHL_RM16 ( dst, count, eaddr );
  
} // end shl_rm16_imm8


static void
shl_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SHL_RM32 ( dst, count, eaddr );
  
} // end shl_rm32_imm8


static void
shl_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    shl_rm32_imm8 ( INTERP, modrmbyte ) :
    shl_rm16_imm8 ( INTERP, modrmbyte );
} // end shl_rm_imm8


static void
shr_rm8_1 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SHR_RM8 ( dst, count, eaddr );
  
} // end shr_rm8_1


static void
shr_rm8_CL (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SHR_RM8 ( dst, count, eaddr );
  
} // end shr_rm8_CL


static void
shr_rm16_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SHR_RM16 ( dst, count, eaddr );
  
} // end shr_rm16_1


static void
shr_rm32_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SHR_RM32 ( dst, count, eaddr );
  
} // end shr_rm32_1


static void
shr_rm_1 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  OPERAND_SIZE_IS_32 ?
    shr_rm32_1 ( INTERP, modrmbyte ) :
    shr_rm16_1 ( INTERP, modrmbyte );
} // end shr_rm_1


static void
shr_rm16_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SHR_RM16 ( dst, count, eaddr );
  
} // end shr_rm16_CL


static void
shr_rm32_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SHR_RM32 ( dst, count, eaddr );
  
} // end shr_rm32_CL


static void
shr_rm_CL (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    shr_rm32_CL ( INTERP, modrmbyte ) :
    shr_rm16_CL ( INTERP, modrmbyte );
} // end shr_rm_CL


static void
shr_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t data;
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SHR_RM8 ( dst, count, eaddr );
  
} // end shr_rm8_imm8


static void
shr_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SHR_RM16 ( dst, count, eaddr );
  
} // end shr_rm16_imm8


static void
shr_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SHR_RM32 ( dst, count, eaddr );
  
} // end shr_rm32_imm8


static void
shr_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    shr_rm32_imm8 ( INTERP, modrmbyte ) :
    shr_rm16_imm8 ( INTERP, modrmbyte );
} // end shr_rm_imm8


static void
sar_rm8_1 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SAR_RM8 ( dst, count, eaddr );
  
} // end sar_rm8_1


static void
sar_rm8_CL (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SAR_RM8 ( dst, count, eaddr );
  
} // end sar_rm8_CL


static void
sar_rm16_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SAR_RM16 ( dst, count, eaddr );
  
} // end sar_rm16_CL


static void
sar_rm32_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  SAR_RM32 ( dst, count, eaddr );
  
} // end sar_rm32_CL


static void
sar_rm_CL (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    sar_rm32_CL ( INTERP, modrmbyte ) :
    sar_rm16_CL ( INTERP, modrmbyte );
} // end sar_rm_CL


static void
sar_rm16_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SAR_RM16 ( dst, count, eaddr );
  
} // end sar_rm16_1


static void
sar_rm32_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  SAR_RM32 ( dst, count, eaddr );
  
} // end sar_rm32_1


static void
sar_rm_1 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  OPERAND_SIZE_IS_32 ?
    sar_rm32_1 ( INTERP, modrmbyte ) :
    sar_rm16_1 ( INTERP, modrmbyte );
} // end sar_rm_1


static void
sar_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t data;
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SAR_RM8 ( dst, count, eaddr );
  
} // end sar_rm8_imm8


static void
sar_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{
  
  uint8_t data;
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SAR_RM16 ( dst, count, eaddr );
  
} // end sar_rm16_imm8


static void
sar_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{
  
  uint8_t data;
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  SAR_RM32 ( dst, count, eaddr );
  
} // end sar_rm32_imm8


static void
sar_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    sar_rm32_imm8 ( INTERP, modrmbyte ) :
    sar_rm16_imm8 ( INTERP, modrmbyte );
} // end sar_rm_imm8




/**************************************/
/* SHLD - Double Precision Shift Left */
/**************************************/

#define SHLD16(DST,OP1,OP2,COUNT)                                       \
  if ( (COUNT) > 0 && (COUNT) <= 16 )                                   \
    {                                                                   \
      if ( (OP1)&(1<<(16-(COUNT))) ) EFLAGS|= CF_FLAG;                  \
      else                           EFLAGS&= ~CF_FLAG;                 \
      (DST)= (COUNT)==16 ? (OP2) : (((OP1)<<(COUNT)) | ((OP2)>>(16-(COUNT)))); \
      if ( (COUNT) == 1 )                                               \
        {                                                               \
          if ( ((DST)^(OP1))&0x8000 ) EFLAGS|= OF_FLAG;                 \
          else                        EFLAGS&= ~OF_FLAG;                \
        }                                                               \
      SETU16_SAHLR_FLAGS ( (DST) );                                     \
    }                                                                   \
  else (DST)= (OP1);
  
#define SHLD32(DST,OP1,OP2,COUNT)                                       \
  if ( (COUNT) > 0 && (COUNT) <= 32 )                                   \
    {                                                                   \
      if ( (OP1)&(1<<(32-(COUNT))) ) EFLAGS|= CF_FLAG;                  \
      else                           EFLAGS&= ~CF_FLAG;                 \
      (DST)= (COUNT)==32 ? (OP2) : (((OP1)<<(COUNT)) | ((OP2)>>(32-(COUNT)))); \
      if ( (COUNT) == 1 )                                               \
        {                                                               \
          if ( ((DST)^(OP1))&0x80000000 ) EFLAGS|= OF_FLAG;             \
          else                            EFLAGS&= ~OF_FLAG;            \
        }                                                               \
      SETU32_SAHLR_FLAGS ( (DST) );                                     \
    }                                                                   \
  else (DST)= (OP1);

#define SHLD_RM16(DST,OP1,OP2,COUNT,EADDR)                              \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(OP1), return ); \
      SHLD16 ( (DST), (OP1), (OP2), (COUNT) );                          \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHLD16 ( (DST), *((EADDR).v.reg), (OP2), (COUNT) );               \
      *((EADDR).v.reg)= (DST);                                          \
    }

#define SHLD_RM32(DST,OP1,OP2,COUNT,EADDR)                              \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(OP1), return ); \
      SHLD32 ( (DST), (OP1), (OP2), (COUNT) );                          \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHLD32 ( (DST), *((EADDR).v.reg), (OP2), (COUNT) );               \
      *((EADDR).v.reg)= (DST);                                          \
    }


static void
shld_rm16_r16_imm8 (
                    PROTO_INTERP
                    )
{
  
  uint8_t modrmbyte,data;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  uint16_t dst,op1,op2;
  int count;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  IB ( &data, return );
  op2= *reg;
  count= data;
  SHLD_RM16 ( dst, op1, op2, count, eaddr );
  
} // end shld_rm16_r16_imm8


static void
shld_rm32_r32_imm8 (
                    PROTO_INTERP
                    )
{
  
  uint8_t modrmbyte,data;
  eaddr_r32_t eaddr;
  uint32_t *reg;
  uint32_t dst,op1,op2;
  int count;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  IB ( &data, return );
  op2= *reg;
  count= data;
  SHLD_RM32 ( dst, op1, op2, count, eaddr );
  
} // end shld_rm32_r32_imm8


static void
shld_rm_r_imm8 (
                PROTO_INTERP
                )
{
  OPERAND_SIZE_IS_32 ?
    shld_rm32_r32_imm8 ( INTERP ) :
    shld_rm16_r16_imm8 ( INTERP );
} // end shld_rm_r_imm8


static void
shld_rm16_r16_CL (
                  PROTO_INTERP
                  )
{
  
  uint8_t modrmbyte;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  uint16_t dst,op1,op2;
  int count;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  op2= *reg;
  count= CL&0x1F;
  SHLD_RM16 ( dst, op1, op2, count, eaddr );
  
} // end shld_rm16_r16_CL


static void
shld_rm32_r32_CL (
                  PROTO_INTERP
                  )
{
  
  uint8_t modrmbyte;
  eaddr_r32_t eaddr;
  uint32_t *reg;
  uint32_t dst,op1,op2;
  int count;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  op2= *reg;
  count= CL&0x1F;
  SHLD_RM32 ( dst, op1, op2, count, eaddr );
  
} // end shld_rm32_r32_CL


static void
shld_rm_r_CL (
              PROTO_INTERP
              )
{
  OPERAND_SIZE_IS_32 ?
    shld_rm32_r32_CL ( INTERP ) :
    shld_rm16_r16_CL ( INTERP );
} // end shld_rm_r_CL




/***************************************/
/* SHRD - Double Precision Shift Right */
/***************************************/

#define SHRD16(DST,OP1,OP2,COUNT)                                       \
  if ( (COUNT) > 0 && (COUNT) <= 16 )                                   \
    {                                                                   \
      if ( (OP1)&(1<<((COUNT)-1)) ) EFLAGS|= CF_FLAG;                   \
      else                          EFLAGS&= ~CF_FLAG;                  \
      (DST)= (COUNT)==16 ? (OP2) : (((OP1)>>(COUNT)) | ((OP2)<<(16-(COUNT)))); \
      if ( (COUNT) == 1 )                                               \
        {                                                               \
          if ( ((DST)^(OP1))&0x8000 ) EFLAGS|= OF_FLAG;                 \
          else                        EFLAGS&= ~OF_FLAG;                \
        }                                                               \
      SETU16_SAHLR_FLAGS ( (DST) );                                     \
    }                                                                   \
  else (DST)= (OP1);
  
#define SHRD32(DST,OP1,OP2,COUNT)                                       \
  if ( (COUNT) > 0 && (COUNT) <= 32 )                                   \
    {                                                                   \
      if ( (OP1)&(1<<((COUNT)-1)) ) EFLAGS|= CF_FLAG;                   \
      else                          EFLAGS&= ~CF_FLAG;                  \
      (DST)= (COUNT)==32 ? (OP2) : (((OP1)>>(COUNT)) | ((OP2)<<(32-(COUNT)))); \
      if ( (COUNT) == 1 )                                               \
        {                                                               \
          if ( ((DST)^(OP1))&0x80000000 ) EFLAGS|= OF_FLAG;             \
          else                            EFLAGS&= ~OF_FLAG;            \
        }                                                               \
      SETU32_SAHLR_FLAGS ( (DST) );                                     \
    }                                                                   \
  else (DST)= (OP1);

#define SHRD_RM16(DST,OP1,OP2,COUNT,EADDR)                              \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(OP1), return ); \
      SHRD16 ( (DST), (OP1), (OP2), (COUNT) );                          \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHRD16 ( (DST), *((EADDR).v.reg), (OP2), (COUNT) );               \
      *((EADDR).v.reg)= (DST);                                          \
    }

#define SHRD_RM32(DST,OP1,OP2,COUNT,EADDR)                              \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(OP1), return ); \
      SHRD32 ( (DST), (OP1), (OP2), (COUNT) );                          \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      SHRD32 ( (DST), *((EADDR).v.reg), (OP2), (COUNT) );               \
      *((EADDR).v.reg)= (DST);                                          \
    }


static void
shrd_rm16_r16_imm8 (
                    PROTO_INTERP
                    )
{
  
  uint8_t modrmbyte,data;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  uint16_t dst,op1,op2;
  int count;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  IB ( &data, return );
  op2= *reg;
  count= data;
  SHRD_RM16 ( dst, op1, op2, count, eaddr );
  
} // end shrd_rm16_r16_imm8


static void
shrd_rm32_r32_imm8 (
                    PROTO_INTERP
                    )
{
  
  uint8_t modrmbyte,data;
  eaddr_r32_t eaddr;
  uint32_t *reg;
  uint32_t dst,op1,op2;
  int count;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  IB ( &data, return );
  op2= *reg;
  count= data;
  SHRD_RM32 ( dst, op1, op2, count, eaddr );
  
} // end shrd_rm32_r32_imm8


static void
shrd_rm_r_imm8 (
                PROTO_INTERP
                )
{
  OPERAND_SIZE_IS_32 ?
    shrd_rm32_r32_imm8 ( INTERP ) :
    shrd_rm16_r16_imm8 ( INTERP );
} // end shrd_rm_r_imm8


static void
shrd_rm16_r16_CL (
                  PROTO_INTERP
                  )
{
  
  uint8_t modrmbyte;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  uint16_t dst,op1,op2;
  int count;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  op2= *reg;
  count= CL&0x1F;
  SHRD_RM16 ( dst, op1, op2, count, eaddr );
  
} // end shrd_rm16_r16_CL


static void
shrd_rm32_r32_CL (
                  PROTO_INTERP
                  )
{
  
  uint8_t modrmbyte;
  eaddr_r32_t eaddr;
  uint32_t *reg;
  uint32_t dst,op1,op2;
  int count;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  op2= *reg;
  count= CL&0x1F;
  SHRD_RM32 ( dst, op1, op2, count, eaddr );
  
} // end shrd_rm32_r32_CL


static void
shrd_rm_r_CL (
              PROTO_INTERP
              )
{
  OPERAND_SIZE_IS_32 ?
    shrd_rm32_r32_CL ( INTERP ) :
    shrd_rm16_r16_CL ( INTERP );
} // end shrd_rm_r_CL




/****************************/
/* RCL/RCR/ROL/ROR - Rotate */
/****************************/

#define RCL8(DST,COUNT_VAR,TMP)                                         \
  (COUNT_VAR)&= 0x1F;                                                   \
  (TMP)= ((EFLAGS&CF_FLAG)!=0);                                         \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80)!=0)^(((DST)&0x40)!=0)) EFLAGS|= OF_FLAG;       \
      else                                      EFLAGS&= ~OF_FLAG;      \
    }                                                                   \
  (COUNT_VAR)%= 9;                                                      \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (((DST)>>(8-(COUNT_VAR)))&0x1) != 0 ) EFLAGS|= CF_FLAG;     \
      else                                       EFLAGS&= ~CF_FLAG;    \
      if ( (COUNT_VAR) == 8 )                                           \
        (DST)= ((TMP)<<7) | ((DST)>>1);                                 \
      else if ( (COUNT_VAR) == 1 )                                      \
        (DST)= ((DST)<<1) | (TMP);                                      \
      else                                                              \
        (DST)=                                                          \
          ((DST)<<(COUNT_VAR)) |                                        \
          ((TMP)<<((COUNT_VAR)-1)) |                                    \
          ((DST)>>(9-(COUNT_VAR)));                                     \
    }

#define RCL16(DST,COUNT_VAR,TMP)                                        \
  (COUNT_VAR)&= 0x1F;                                                   \
  (TMP)= ((EFLAGS&CF_FLAG)!=0);                                         \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x8000)!=0)^(((DST)&0x4000)!=0)) EFLAGS|= OF_FLAG;   \
      else                                          EFLAGS&= ~OF_FLAG;  \
    }                                                                   \
  (COUNT_VAR)%= 17;                                                     \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (((DST)>>(16-(COUNT_VAR)))&0x1) != 0 ) EFLAGS|= CF_FLAG;     \
      else                                        EFLAGS&= ~CF_FLAG;    \
      if ( (COUNT_VAR) == 16 )                                          \
        (DST)= ((TMP)<<15) | ((DST)>>1);                                \
      else if ( (COUNT_VAR) == 1 )                                      \
        (DST)= ((DST)<<1) | (TMP);                                      \
      else                                                              \
        (DST)=                                                          \
          ((DST)<<(COUNT_VAR)) |                                        \
          ((TMP)<<((COUNT_VAR)-1)) |                                    \
          ((DST)>>(17-(COUNT_VAR)));                                    \
    }

#define RCL32(DST,COUNT_VAR,TMP)                                        \
  (COUNT_VAR)&= 0x1F;                                                   \
  (TMP)= ((EFLAGS&CF_FLAG)!=0);                                         \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80000000)!=0)^(((DST)&0x40000000)!=0)) EFLAGS|= OF_FLAG; \
      else                                                  EFLAGS&= ~OF_FLAG; \
    }                                                                   \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (((DST)>>(32-(COUNT_VAR)))&0x1) != 0 ) EFLAGS|= CF_FLAG;     \
      else                                        EFLAGS&= ~CF_FLAG;    \
      if ( (COUNT_VAR) == 1 )                                           \
        (DST)= ((DST)<<1) | (TMP);                                      \
      else                                                              \
        (DST)=                                                          \
          ((DST)<<(COUNT_VAR)) |                                        \
          ((TMP)<<((COUNT_VAR)-1)) |                                    \
          ((DST)>>(33-(COUNT_VAR)));                                    \
    }

#define RCL_RM8(DST,COUNT_VAR,TMP,EADDR)                                \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      RCL8 ( (DST), (COUNT_VAR), (TMP) );                               \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      RCL8 ( *((EADDR).v.reg), (COUNT_VAR), (TMP) );                    \
    }

#define RCL_RM16(DST,COUNT_VAR,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      RCL16 ( (DST), (COUNT_VAR), (TMP) );                              \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      RCL16 ( *((EADDR).v.reg), (COUNT_VAR), (TMP) );                   \
    }

#define RCL_RM32(DST,COUNT_VAR,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      RCL32 ( (DST), (COUNT_VAR), (TMP) );                              \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      RCL32 ( *((EADDR).v.reg), (COUNT_VAR), (TMP) );           \
    }

#define RCR8(DST,COUNT_VAR,TMP)                                         \
  (COUNT_VAR)&= 0x1F;                                                   \
  (TMP)= ((EFLAGS&CF_FLAG)!=0);                                         \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80)!=0)^(TMP)) EFLAGS|= OF_FLAG;                   \
      else                          EFLAGS&= ~OF_FLAG;                  \
    }                                                                   \
  (COUNT_VAR)%= 9;                                                      \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (((DST)>>((COUNT_VAR)-1))&0x1) != 0 ) EFLAGS|= CF_FLAG;      \
      else                                       EFLAGS&= ~CF_FLAG;     \
      if ( (COUNT_VAR) == 8 )                                           \
        (DST)= (TMP) | ((DST)<<1);                                      \
      else if ( (COUNT_VAR) == 1 )                                      \
        (DST)= ((DST)>>1) | ((TMP)<<7);                                 \
      else                                                              \
        (DST)=                                                          \
          ((DST)>>(COUNT_VAR)) |                                        \
          ((TMP)<<(8-(COUNT_VAR))) |                                    \
          ((DST)<<(9-(COUNT_VAR)));                                     \
    }

#define RCR16(DST,COUNT_VAR,TMP)                                        \
  (COUNT_VAR)&= 0x1F;                                                   \
  (TMP)= ((EFLAGS&CF_FLAG)!=0);                                         \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x8000)!=0)^(TMP)) EFLAGS|= OF_FLAG;                 \
      else                            EFLAGS&= ~OF_FLAG;                \
    }                                                                   \
  (COUNT_VAR)%= 17;                                                     \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (((DST)>>((COUNT_VAR)-1))&0x1) != 0 ) EFLAGS|= CF_FLAG;      \
      else                                       EFLAGS&= ~CF_FLAG;     \
      if ( (COUNT_VAR) == 16 )                                          \
        (DST)= (TMP) | ((DST)<<1);                                      \
      else if ( (COUNT_VAR) == 1 )                                      \
        (DST)= ((DST)>>1) | ((TMP)<<15);                                \
      else                                                              \
        (DST)=                                                          \
          ((DST)>>(COUNT_VAR)) |                                        \
          ((TMP)<<(16-(COUNT_VAR))) |                                   \
          ((DST)<<(17-(COUNT_VAR)));                                    \
    }

#define RCR32(DST,COUNT_VAR,TMP)                                        \
  (COUNT_VAR)&= 0x1F;                                                   \
  (TMP)= ((EFLAGS&CF_FLAG)!=0);                                         \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80000000)!=0)^(TMP)) EFLAGS|= OF_FLAG;             \
      else                                EFLAGS&= ~OF_FLAG;            \
    }                                                                   \
  if ( (COUNT_VAR) > 0 )                                                \
    {                                                                   \
      if ( (((DST)>>((COUNT_VAR)-1))&0x1) != 0 ) EFLAGS|= CF_FLAG;      \
      else                                       EFLAGS&= ~CF_FLAG;     \
      if ( (COUNT_VAR) == 1 )                                           \
        (DST)= ((DST)>>1) | ((TMP)<<31);                                \
      else                                                              \
        (DST)=                                                          \
          ((DST)>>(COUNT_VAR)) |                                        \
          ((TMP)<<(32-(COUNT_VAR))) |                                   \
          ((DST)<<(33-(COUNT_VAR)));                                    \
    }

#define RCR_RM8(DST,COUNT_VAR,TMP,EADDR)                                \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      RCR8 ( (DST), (COUNT_VAR), (TMP) );                               \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      RCR8 ( *((EADDR).v.reg), (COUNT_VAR), (TMP) );                    \
    }

#define RCR_RM16(DST,COUNT_VAR,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      RCR16 ( (DST), (COUNT_VAR), (TMP) );                              \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      RCR16 ( *((EADDR).v.reg), (COUNT_VAR), (TMP) );                   \
    }

#define RCR_RM32(DST,COUNT_VAR,TMP,EADDR)                               \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      RCR32 ( (DST), (COUNT_VAR), (TMP) );                              \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return );        \
    }                                                           \
  else                                                          \
    {                                                           \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }         \
      RCR32 ( *((EADDR).v.reg), (COUNT_VAR), (TMP) );           \
    }

#define ROL8(DST,COUNT_VAR)                                             \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80)^(((DST)&0x40)<<1))!=0 ) EFLAGS|= OF_FLAG;      \
      else                                       EFLAGS&= ~OF_FLAG;     \
    }                                                                   \
  (COUNT_VAR)%= 8;                                                      \
  if ( (COUNT_VAR) > 0 )                                                \
    (DST)= ((DST)<<(COUNT_VAR)) | ((DST)>>(8-(COUNT_VAR)));             \
  if ( ((DST)&0x01) != 0 ) EFLAGS|= CF_FLAG;                            \
  else                     EFLAGS&= ~CF_FLAG;

#define ROL16(DST,COUNT_VAR)                                             \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x8000)^(((DST)&0x4000)<<1))!=0 ) EFLAGS|= OF_FLAG;  \
      else                                           EFLAGS&= ~OF_FLAG; \
    }                                                                   \
  (COUNT_VAR)%= 16;                                                     \
  if ( (COUNT_VAR) > 0 )                                                \
    (DST)= ((DST)<<(COUNT_VAR)) | ((DST)>>(16-(COUNT_VAR)));            \
  if ( ((DST)&0x01) != 0 ) EFLAGS|= CF_FLAG;                            \
  else                     EFLAGS&= ~CF_FLAG;

#define ROL32(DST,COUNT_VAR)                                             \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80000000)^(((DST)&0x40000000)<<1))!=0 )            \
        EFLAGS|= OF_FLAG;                                               \
      else EFLAGS&= ~OF_FLAG;                                           \
    }                                                                   \
  (COUNT_VAR)%= 32;                                                     \
  if ( (COUNT_VAR) > 0 )                                                \
    (DST)= ((DST)<<(COUNT_VAR)) | ((DST)>>(32-(COUNT_VAR)));            \
  if ( ((DST)&0x01) != 0 ) EFLAGS|= CF_FLAG;                            \
  else                     EFLAGS&= ~CF_FLAG;

#define ROL_RM8(DST,COUNT_VAR,EADDR)                                    \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ROL8 ( (DST), (COUNT_VAR) );                                      \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      ROL8 ( *((EADDR).v.reg), (COUNT_VAR) );                           \
    }

#define ROL_RM16(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ROL16 ( (DST), (COUNT_VAR) );                                     \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      ROL16 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }

#define ROL_RM32(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ROL32 ( (DST), (COUNT_VAR) );                                     \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      ROL32 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }

#define ROR8(DST,COUNT_VAR)                                             \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80)^(((DST)&0x01)<<7))!=0 ) EFLAGS|= OF_FLAG;      \
      else                                       EFLAGS&= ~OF_FLAG;     \
    }                                                                   \
  (COUNT_VAR)%= 8;                                                      \
  if ( (COUNT_VAR) > 0 )                                                \
    (DST)= ((DST)<<(8-(COUNT_VAR))) | ((DST)>>(COUNT_VAR));             \
  if ( ((DST)&0x80) != 0 ) EFLAGS|= CF_FLAG;                            \
  else                     EFLAGS&= ~CF_FLAG;

#define ROR16(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x8000)^(((DST)&0x01)<<15))!=0 ) EFLAGS|= OF_FLAG;   \
      else                                          EFLAGS&= ~OF_FLAG;  \
    }                                                                   \
  (COUNT_VAR)%= 16;                                                     \
  if ( (COUNT_VAR) > 0 )                                                \
    (DST)= ((DST)<<(16-(COUNT_VAR))) | ((DST)>>(COUNT_VAR));            \
  if ( ((DST)&0x8000) != 0 ) EFLAGS|= CF_FLAG;                          \
  else                       EFLAGS&= ~CF_FLAG;

#define ROR32(DST,COUNT_VAR)                                            \
  (COUNT_VAR)&= 0x1F;                                                   \
  if ( (COUNT_VAR) == 1 )                                               \
    {                                                                   \
      if ( (((DST)&0x80000000)^(((DST)&0x01)<<31))!=0 ) EFLAGS|= OF_FLAG; \
      else                                              EFLAGS&= ~OF_FLAG; \
    }                                                                   \
  if ( (COUNT_VAR) > 0 )                                                \
    (DST)= ((DST)<<(32-(COUNT_VAR))) | ((DST)>>(COUNT_VAR));            \
  if ( ((DST)&0x80000000) != 0 ) EFLAGS|= CF_FLAG;                      \
  else                           EFLAGS&= ~CF_FLAG;

#define ROR_RM8(DST,COUNT_VAR,EADDR)                                    \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READB_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ROR8 ( (DST), (COUNT_VAR) );                                      \
      WRITEB ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      ROR8 ( *((EADDR).v.reg), (COUNT_VAR) );                           \
    }

#define ROR_RM16(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ROR16 ( (DST), (COUNT_VAR) );                                     \
      WRITEW ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      ROR16 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }

#define ROR_RM32(DST,COUNT_VAR,EADDR)                                   \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READD_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
      ROR32 ( (DST), (COUNT_VAR) );                                     \
      WRITED ( (EADDR).v.addr.seg, (EADDR).v.addr.off, (DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }                 \
      ROR32 ( *((EADDR).v.reg), (COUNT_VAR) );                          \
    }


static void
rcl_rm8_1 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  uint8_t dst,tmp;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  RCL_RM8 ( dst, count, tmp, eaddr );
  
} // end rcl_rm8_1


static void
rcl_rm8_CL (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint8_t dst,tmp;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  RCL_RM8 ( dst, count, tmp, eaddr );
  
} // end rcl_rm8_CL


static void
rcl_rm16_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint16_t dst,tmp;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  RCL_RM16 ( dst, count, tmp, eaddr );
  
} // end rcl_rm16_CL


static void
rcl_rm32_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint32_t dst,tmp;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  RCL_RM32 ( dst, count, tmp, eaddr );
  
} // end rcl_rm32_CL


static void
rcl_rm_CL (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    rcl_rm32_CL ( INTERP, modrmbyte ) :
    rcl_rm16_CL ( INTERP, modrmbyte );
} // end rcl_rm_CL


static void
rcl_rm16_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint16_t dst,tmp;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  RCL_RM16 ( dst, count, tmp, eaddr );
  
} // end rcl_rm16_1


static void
rcl_rm32_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint32_t dst,tmp;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  RCL_RM32 ( dst, count, tmp, eaddr );
  
} // end rcl_rm32_1


static void
rcl_rm_1 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  OPERAND_SIZE_IS_32 ?
    rcl_rm32_1 ( INTERP, modrmbyte ) :
    rcl_rm16_1 ( INTERP, modrmbyte );
} // end rcl_rm_1


static void
rcl_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint16_t dst,tmp;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  RCL_RM16 ( dst, count, tmp, eaddr );
  
} // end rcl_rm16_imm8


static void
rcl_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint32_t dst,tmp;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  RCL_RM32 ( dst, count, tmp, eaddr );
  
} // end rcl_rm32_imm8


static void
rcl_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    rcl_rm32_imm8 ( INTERP, modrmbyte ) :
    rcl_rm16_imm8 ( INTERP, modrmbyte );
} // end rcl_rm_imm8


static void
rcr_rm8_1 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  uint8_t dst,tmp;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  RCR_RM8 ( dst, count, tmp, eaddr );
  
} // end rcr_rm8_1


static void
rcr_rm8_CL (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint8_t dst,tmp;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  RCR_RM8 ( dst, count, tmp, eaddr );
  
} // end rcr_rm8_CL


static void
rcr_rm16_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  uint16_t dst,tmp;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  RCR_RM16 ( dst, count, tmp, eaddr );
  
} // end rcr_rm16_CL


static void
rcr_rm32_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint32_t dst,tmp;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  RCR_RM32 ( dst, count, tmp, eaddr );
  
} // end rcr_rm32_CL


static void
rcr_rm_CL (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    rcr_rm32_CL ( INTERP, modrmbyte ) :
    rcr_rm16_CL ( INTERP, modrmbyte );
} // end rcr_rm_CL


static void
rcr_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint16_t dst,tmp;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  RCR_RM16 ( dst, count, tmp, eaddr );
  
} // end rcr_rm16_imm8


static void
rcr_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint32_t dst,tmp;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  RCR_RM32 ( dst, count, tmp, eaddr );
  
} // end rcr_rm32_imm8


static void
rcr_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    rcr_rm32_imm8 ( INTERP, modrmbyte ) :
    rcr_rm16_imm8 ( INTERP, modrmbyte );
} // end rcr_rm_imm8


static void
rcr_rm16_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint16_t dst,tmp;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  RCR_RM16 ( dst, count, tmp, eaddr );
  
} // end rcr_rm16_1


static void
rcr_rm32_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint32_t dst,tmp;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  RCR_RM32 ( dst, count, tmp, eaddr );
  
} // end rcr_rm32_1


static void
rcr_rm_1 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  OPERAND_SIZE_IS_32 ?
    rcr_rm32_1 ( INTERP, modrmbyte ) :
    rcr_rm16_1 ( INTERP, modrmbyte );
} // end rcr_rm_1


static void
rol_rm8_1 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  ROL_RM8 ( dst, count, eaddr );
  
} // end rol_rm8_1


static void
rol_rm8_CL (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  ROL_RM8 ( dst, count, eaddr );
  
} // end rol_rm8_CL


static void
rol_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t data;
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  

  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  ROL_RM8 ( dst, count, eaddr );
  
} // end rol_rm8_imm8


static void
rol_rm16_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  ROL_RM16 ( dst, count, eaddr );
  
} // end rol_rm16_CL


static void
rol_rm32_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  ROL_RM32 ( dst, count, eaddr );
  
} // end rol_rm32_CL


static void
rol_rm_CL (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    rol_rm32_CL ( INTERP, modrmbyte ) :
    rol_rm16_CL ( INTERP, modrmbyte );
} // end rol_rm_CL


static void
rol_rm16_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  ROL_RM16 ( dst, count, eaddr );
  
} // end rol_rm16_1


static void
rol_rm32_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  ROL_RM32 ( dst, count, eaddr );
  
} // end rol_rm32_1


static void
rol_rm_1 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  OPERAND_SIZE_IS_32 ?
    rol_rm32_1 ( INTERP, modrmbyte ) :
    rol_rm16_1 ( INTERP, modrmbyte );
} // end rol_rm_1


static void
rol_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{
  
  uint8_t data;
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  ROL_RM16 ( dst, count, eaddr );
  
} // end rol_rm16_imm8


static void
rol_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  ROL_RM32 ( dst, count, eaddr );
  
} // end rol_rm32_imm8


static void
rol_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    rol_rm32_imm8 ( INTERP, modrmbyte ) :
    rol_rm16_imm8 ( INTERP, modrmbyte );
} // end rol_rm_imm8


static void
ror_rm8_1 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  ROR_RM8 ( dst, count, eaddr );
  
} // end ror_rm8_1


static void
ror_rm8_CL (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  ROR_RM8 ( dst, count, eaddr );
  
} // end ror_rm8_CL


static void
ror_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t data;
  uint8_t dst;
  eaddr_r8_t eaddr;
  int count;
  

  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  ROR_RM8 ( dst, count, eaddr );
  
} // end ror_rm8_imm8


static void
ror_rm16_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  ROR_RM16 ( dst, count, eaddr );
  
} // end ror_rm16_CL


static void
ror_rm32_CL (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= CL;
  ROR_RM32 ( dst, count, eaddr );
  
} // end ror_rm32_CL


static void
ror_rm_CL (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  OPERAND_SIZE_IS_32 ?
    ror_rm32_CL ( INTERP, modrmbyte ) :
    ror_rm16_CL ( INTERP, modrmbyte );
} // end ror_rm_CL


static void
ror_rm16_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  ROR_RM16 ( dst, count, eaddr );
  
} // end ror_rm16_imm8


static void
ror_rm32_imm8 (
               PROTO_INTERP,
               const uint8_t modrmbyte
               )
{

  uint8_t data;
  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( &data, return );
  count= data;
  ROR_RM32 ( dst, count, eaddr );
  
} // end ror_rm32_imm8


static void
ror_rm_imm8 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    ror_rm32_imm8 ( INTERP, modrmbyte ) :
    ror_rm16_imm8 ( INTERP, modrmbyte );
} // end ror_rm_imm8


static void
ror_rm16_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint16_t dst;
  eaddr_r16_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  ROR_RM16 ( dst, count, eaddr );
  
} // end ror_rm16_1


static void
ror_rm32_1 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{

  uint32_t dst;
  eaddr_r32_t eaddr;
  int count;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  count= 1;
  ROR_RM32 ( dst, count, eaddr );
  
} // end ror_rm32_1


static void
ror_rm_1 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  OPERAND_SIZE_IS_32 ?
    ror_rm32_1 ( INTERP, modrmbyte ) :
    ror_rm16_1 ( INTERP, modrmbyte );
} // end ror_rm_1


