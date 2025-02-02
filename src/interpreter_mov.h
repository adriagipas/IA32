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
 *  interpreter_mov.h - Instruccions MOV.
 *
 */




/**************/
/* MOV - Move */
/**************/

#define READ_RM16(DST,EADDR)                                            \
  if ( (EADDR).is_addr )                                                \
    {                                                                   \
      READW_DATA ( (EADDR).v.addr.seg, (EADDR).v.addr.off, &(DST), return ); \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      (DST)= *((EADDR).v.reg);                                          \
    }

static void
mov_rm8_r8 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  eaddr_r8_t eaddr;
  uint8_t *reg;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, *reg, return );
    }
  else *(eaddr.v.reg)= *reg;
  
} // end mov_rm8_r8


static void
mov_rm16_r16 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  eaddr_r16_t eaddr;
  uint16_t *reg;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, *reg, return );
    }
  else *(eaddr.v.reg)= *reg;
  
} // end mov_rm16_r16


static void
mov_rm32_r32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  eaddr_r32_t eaddr;
  uint32_t *reg;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, *reg, return );
    }
  else *(eaddr.v.reg)= *reg;
  
} // end mov_rm32_r32


static void
mov_rm_r (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? mov_rm32_r32 ( INTERP ) : mov_rm16_r16 ( INTERP );
} // end mov_rm_r


static void
mov_r8_rm8 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte;
  eaddr_r8_t eaddr;
  uint8_t *reg;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, reg, return );
    }
  else *reg= *(eaddr.v.reg);
  
} // end mov_r8_rm8


static void
mov_r16_rm16 (
              PROTO_INTERP
              )
{
  
  uint8_t modrmbyte;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, reg, return );
    }
  else *reg= *(eaddr.v.reg);
  
} // end mov_r16_rm16


static void
mov_r32_rm32 (
              PROTO_INTERP
              )
{

  uint8_t modrmbyte;
  eaddr_r32_t eaddr;
  uint32_t *reg;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, reg, return );
    }
  else *reg= *(eaddr.v.reg);
  
} // end mov_r32_rm32


static void
mov_r_rm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? mov_r32_rm32 ( INTERP ) : mov_r16_rm16 ( INTERP );
} // end mov_r_rm


static void
mov_rm16_sreg (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte;
  IA32_SegmentRegister *reg;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_sreg16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, reg->v, return );
    }
  else *(eaddr.v.reg)= reg->v;
  
} // end mov_rm16_sreg


static void
mov_rm32_sreg (
               PROTO_INTERP
               )
{
  
  uint8_t modrmbyte;
  IA32_SegmentRegister *reg;
  eaddr_r32_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_sreg32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    { // IMPORTANT!! Si és memòria 16bit.
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, reg->v, return );
    }
  else *(eaddr.v.reg)= (uint32_t) reg->v;
  
} // end mov_rm32_sreg


// NOTA!!!!! Segons el manual no acaba de tindre sentit que operand
// size siga 32, però si ho és i el destí és un registre funciona
// igualment perquè en la part baixa es copia el segment. El que
// ocorre en la part alta depen del model. En els models moderns fica
// 0000. En cas de que siga memòria no està gens clar. HE DECIDIT
// fer-lo com si es poguera emprar 32 bits sense problema, i ficant la
// part alta a 0000.
// IMPORTANT!!! si el destí no és registre aleshores sí que és de 16bit.
static void
mov_rm_sreg (
             PROTO_INTERP
             )
{
  OPERAND_SIZE_IS_32 ? mov_rm32_sreg ( INTERP ) : mov_rm16_sreg ( INTERP );
} // end mov_rm_sreg


// NOTA!!! Al revés no cal fer res. Perquè si el que cal fer és
// quedar-se amb la part baixa, això ja s'està fent si llegim 16bits.
static void
mov_sreg_rm16 (
               PROTO_INTERP
               )
{
  
  uint8_t modrmbyte;
  uint16_t selector;
  IA32_SegmentRegister *reg;
  eaddr_r16_t eaddr;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_sreg16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) goto ud_exc;
  
  // Llig selector
  READ_RM16 ( selector, eaddr );

  // Carrega
  if ( set_sreg ( INTERP, selector, reg ) != 0 )
    return;
  
  return;

 ud_exc:
  EXCEPTION ( EXCP_UD );
  return;
  
} // end mov_sreg_rm16


static void
mov_AL_moffs8 (
               PROTO_INTERP
               )
{
  
  addr_t addr;
  
  
  if ( get_addr_moffs ( INTERP, &addr ) != 0  )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  
  READB_DATA ( addr.seg, addr.off, &AL, return );
  
} // end mov_AL_moffs8


static void
mov_AX_moffs16 (
                 PROTO_INTERP
                 )
{
  
  addr_t addr;
  
  
  if ( get_addr_moffs ( INTERP, &addr ) != 0  )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  
  READW_DATA ( addr.seg, addr.off, &AX, return );
  
} // end mov_AX_moffs16


static void
mov_EAX_moffs32 (
                 PROTO_INTERP
                 )
{

  addr_t addr;
  
  
  if ( get_addr_moffs ( INTERP, &addr ) != 0  )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  
  READD_DATA ( addr.seg, addr.off, &EAX, return );
  
} // end mov_EAX_moffs32


static void
mov_A_moffs (
             PROTO_INTERP
             )
{
  OPERAND_SIZE_IS_32 ? mov_EAX_moffs32 ( INTERP ) : mov_AX_moffs16 ( INTERP );
} // end mov_A_moffs


static void
mov_moffs8_AL (
               PROTO_INTERP
               )
{
  
  addr_t addr;
  
  
  if ( get_addr_moffs ( INTERP, &addr ) != 0  )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  
  WRITEB ( addr.seg, addr.off, AL, return );
  
} // end mov_moffs8_AL


static void
mov_moffs16_AX (
                PROTO_INTERP
                )
{
  
  addr_t addr;
  
  
  if ( get_addr_moffs ( INTERP, &addr ) != 0  )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  WRITEW ( addr.seg, addr.off, AX, return );
  
} // end mov_moffs16_AX


static void
mov_moffs32_EAX (
                 PROTO_INTERP
                 )
{

  addr_t addr;
  
  
  if ( get_addr_moffs ( INTERP, &addr ) != 0  )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  WRITED ( addr.seg, addr.off, EAX, return );
  
} // end mov_moffs32_EAX


static void
mov_moffs_A (
             PROTO_INTERP
             )
{
  OPERAND_SIZE_IS_32 ? mov_moffs32_EAX ( INTERP ) : mov_moffs16_AX ( INTERP );
} // end mov_moffs_A


static void
mov_r8_imm8 (
             PROTO_INTERP,
             const uint8_t opcode
             )
{

  uint8_t val;
  
  
  IB ( &val, return );
  switch ( opcode&0x7 )
    {
    case 0: AL= val; break;
    case 1: CL= val; break;
    case 2: DL= val; break;
    case 3: BL= val; break;
    case 4: AH= val; break;
    case 5: CH= val; break;
    case 6: DH= val; break;
    case 7: BH= val; break;
    }
  
} // end mov_r8_imm8


static void
mov_r_imm (
           PROTO_INTERP,
           const uint8_t opcode
           )
{

  uint32_t val32;
  uint16_t val16;

  
  if ( OPERAND_SIZE_IS_32 )
    {
      ID ( &val32, return );
      switch ( opcode&0x7 )
        {
        case 0: EAX= val32; break;
        case 1: ECX= val32; break;
        case 2: EDX= val32; break;
        case 3: EBX= val32; break;
        case 4: ESP= val32; break;
        case 5: EBP= val32; break;
        case 6: ESI= val32; break;
        case 7: EDI= val32; break;
        }
    }
  else
    {
      IW ( &val16, return );
      switch ( opcode&0x7 )
        {
        case 0: AX= val16; break;
        case 1: CX= val16; break;
        case 2: DX= val16; break;
        case 3: BX= val16; break;
        case 4: SP= val16; break;
        case 5: BP= val16; break;
        case 6: SI= val16; break;
        case 7: DI= val16; break;
        }
    }
  
} // end mov_r_imm


static void
mov_rm8_imm8 (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  
  uint8_t val;
  eaddr_r8_t eaddr;
  
  
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IB ( (uint8_t *) &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else *(eaddr.v.reg)= val;
  
} // end mov_rm8_imm8


static void
mov_rm16_imm16 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{
  
  uint16_t val;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else *(eaddr.v.reg)= val;
  
} // end mov_rm16_imm16


static void
mov_rm32_imm32 (
                PROTO_INTERP,
                const uint8_t modrmbyte
                )
{
  
  uint32_t val;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else *(eaddr.v.reg)= val;
  
} // end mov_rm32_imm32


static void
mov_rm_imm (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  OPERAND_SIZE_IS_32 ?
    mov_rm32_imm32 ( INTERP, modrmbyte ) :
    mov_rm16_imm16 ( INTERP, modrmbyte );
} // end mov_rm_imm




/****************************************/
/* MOV - Move to/from Control Registers */
/****************************************/

static void
mov_r32_cr (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  int cr;
  uint32_t *reg;

  
  IB ( &modrmbyte, return );
  get_addr_mode_cr ( INTERP, modrmbyte, &reg, &cr );
  if ( QLOCKED ||
       (CPL != 0 && // <-- Nivell seguretat 0!!!
        (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG))) )
    { EXCEPTION0 ( EXCP_GP ); return; }

  switch ( cr )
    {
    case 0: *reg= CR0; break;

    case 2: *reg= CR2; break;
    case 3: *reg= CR3; break;
    default:
      printf ( "[EE] mov_r32_cr: Lectura de CR%d no implementada!!!\n", cr );
      exit ( EXIT_FAILURE );
    }
  
} // end mov_r32_cr


static void
mov_cr_r32 (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  int cr;
  uint32_t *reg,val;

  
  IB ( &modrmbyte, return );
  get_addr_mode_cr ( INTERP, modrmbyte, &reg, &cr );
  if ( QLOCKED ||
       ((CPL != 0) && // <-- Nivell seguretat 0!!!
        (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG))) )
    goto exc_gp;
  
  switch ( cr )
    {
      
    case 0:
      val= ((*reg)&CR0_MASK)&(~CR0_ET);
      // Comprovacions situaciones absurdes. Faltaran segurament!
      if ( ((CR0_PG&val) && !(CR0_PE&val)) ) goto exc_gp;
      CR0= val;
      update_mem_callbacks ( INTERP );
      if ( val&(CR0_CD|CR0_NW|CR0_AM|
                CR0_WP|CR0_NE|CR0_TS|CR0_EM|CR0_MP) )
        {
          WW ( UDATA, "mov_cr_r32 Falten flags en CR0:"
               " CD,NW,AM,WP,NE,TS,EM,MP -> %X", CR0 );
          exit ( EXIT_FAILURE );
        }
      break;
      
    case 2: CR2= *reg; break;
    case 3:
      val= (*reg)&CR3_MASK;
      CR3= val;
      if ( val&(CR3_PCD|CR3_PWT) )
        WW ( UDATA, "mov_cr_r32 Falten flags en CR3:"
             " PCD,PWT" );
      break;
      
    default:
      printf ( "[EE] mov_cr_r32: Escritura en CR%d no implementada!!!\n", cr );
      exit ( EXIT_FAILURE );
    }

  return;

 exc_gp:
  EXCEPTION0 ( EXCP_GP );
  
} // end mov_cr_r32




/**************************************/
/* MOV - Move to/from Debug Registers */
/**************************************/

static void
mov_r32_dr (
            PROTO_INTERP
            )
{

  uint8_t modrmbyte;
  int dr;
  uint32_t *reg;

  
  IB ( &modrmbyte, return );
  get_addr_mode_cr ( INTERP, modrmbyte, &reg, &dr );
  if ( QLOCKED ||
       (CPL != 0 && // <-- Nivell seguretat 0!!!
        (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG))) )
    { EXCEPTION0 ( EXCP_GP ); return; }

  switch ( dr )
    {
    case 4:
    case 5:
      if ( (CR4&CR4_DE) != 0 ) { EXCEPTION ( EXCP_UD ); return; }
      printf ( "[EE] mov_r32_dr: Lectura de DR%d no implementada!!!\n", dr );
      exit ( EXIT_FAILURE );
      break;
    case 7: *reg= DR7; break;
    default:
      printf ( "[EE] mov_r32_dr: Lectura de DR%d no implementada!!!\n", dr );
      exit ( EXIT_FAILURE );
    }
  
} // end mov_r32_dr


static void
mov_dr_r32 (
            PROTO_INTERP
            )
{
  
  uint8_t modrmbyte;
  int dr;
  uint32_t *reg;

  
  IB ( &modrmbyte, return );
  get_addr_mode_cr ( INTERP, modrmbyte, &reg, &dr );
  if ( QLOCKED ||
       ((CPL != 0) && // <-- Nivell seguretat 0!!!
        (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG))) )
    goto exc_gp;
  
  switch ( dr )
    {
    case 4:
    case 5:
      if ( (CR4&CR4_DE) != 0 ) { EXCEPTION ( EXCP_UD ); return; }
      printf ( "[EE] mov_dr_r32: Escritura en DR%d no implementada!!!\n", dr );
      exit ( EXIT_FAILURE );
      break;
    case 7:
      if ( ((*reg)&DR7_MASK) != 0 )
        {
          printf ( "[CAL IMPLEMENTAR] DR7 - VAL: %X!!!\n", *reg );
          exit ( EXIT_FAILURE );
        }
      DR7= ((*reg)&DR7_MASK) | DR7_RESERVED_SET1;
      break;
    default:
      printf ( "[EE] mov_dr_r32: Escritura en DR%d no implementada!!!\n", dr );
      exit ( EXIT_FAILURE );
    }

  return;

 exc_gp:
  EXCEPTION0 ( EXCP_GP );
  
} // end mov_dr_r32




/******************************************************************/
/* MOVS/MOVSB/MOVSW/MOVSD/MOVSQ - Move Data from String to String */
/******************************************************************/

// NOTA!! Aparentment Microsoft utilitza el prefixe F2 en compte del
// F3 en les operacions MOVS. Açò és un comportament no documentat en
// la documentació oficial de Intel. Aparentment el funcionament és el
// mateix que amb F3.

static void
movsb (
       PROTO_INTERP
       )
{

  addr_t src,dst;
  uint8_t val;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addrs_dsesi_esedi ( INTERP, &src, &dst );
      READB_DATA ( src.seg, src.off, &val, return );
      WRITEB ( dst.seg, dst.off, val, return );
      
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
      TRY_REP_MS;

    }
  
} // end movsb


static void
movsw (
       PROTO_INTERP
       )
{

  addr_t src,dst;
  uint16_t val;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addrs_dsesi_esedi ( INTERP, &src, &dst );
      READW_DATA ( src.seg, src.off, &val, return );
      WRITEW ( dst.seg, dst.off, val, return );
      
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
      TRY_REP_MS;

    }
  
} // end movsw


static void
movsd (
       PROTO_INTERP
       )
{

  addr_t src,dst;
  uint32_t val;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addrs_dsesi_esedi ( INTERP, &src, &dst );
      READD_DATA ( src.seg, src.off, &val, return );
      WRITED ( dst.seg, dst.off, val, return );
      
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
      TRY_REP_MS;
      
    }
  
} // end movsd


static void
movs (
      PROTO_INTERP
      )
{
  OPERAND_SIZE_IS_32 ? movsd ( INTERP ) : movsw ( INTERP );
} // end movs




/*******************************************/
/* MOVSX/MOVSXD - Move with Sign-Extension */
/*******************************************/

static void
movsx_r16_rm8 (
               PROTO_INTERP
               )
{
  
  uint8_t modrmbyte,val;
  eaddr_r8_t eaddr;
  uint16_t *reg;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rm8_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  *reg= (uint16_t) ((int16_t) ((int8_t) val));
  
} // end movsx_r16_rm8


static void
movsx_r32_rm8 (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte,val;
  eaddr_r8_t eaddr;
  uint32_t *reg;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_rm8_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  *reg= (uint32_t) ((int32_t) ((int8_t) val));
  
} // end movsx_r32_rm8


static void
movsx_r_rm8 (
             PROTO_INTERP
             )
{
  OPERAND_SIZE_IS_32 ? movsx_r32_rm8 ( INTERP ) : movsx_r16_rm8 ( INTERP );
} // end movsx_r_rm8


static void
movsx_r32_rm16 (
                PROTO_INTERP
                )
{

  uint8_t modrmbyte;
  uint16_t val;
  eaddr_r16_t eaddr;
  uint32_t *reg;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_rm16_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  *reg= (uint32_t) ((int32_t) ((int16_t) val));
  
} // end movsx_r32_rm16




/*********************************/
/* MOVZX - Move with Zero-Extend */
/*********************************/

static void
movzx_r16_rm8 (
               PROTO_INTERP
               )
{
  
  uint8_t modrmbyte,val;
  eaddr_r8_t eaddr;
  uint16_t *reg;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_rm8_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  *reg= (uint16_t) val;
  
} // end movzx_r16_rm8


static void
movzx_r32_rm8 (
               PROTO_INTERP
               )
{

  uint8_t modrmbyte,val;
  eaddr_r8_t eaddr;
  uint32_t *reg;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_rm8_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  *reg= (uint32_t) val;
  
} // end movzx_r32_rm8


static void
movzx_r_rm8 (
             PROTO_INTERP
             )
{
  OPERAND_SIZE_IS_32 ? movzx_r32_rm8 ( INTERP ) : movzx_r16_rm8 ( INTERP );
} // end movzx_r_rm8


static void
movzx_r32_rm16 (
                PROTO_INTERP
                )
{

  uint8_t modrmbyte;
  uint16_t val;
  eaddr_r16_t eaddr;
  uint32_t *reg;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_rm16_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *(eaddr.v.reg);
  *reg= (uint32_t) val;
  
} // end movzx_r32_rm16
