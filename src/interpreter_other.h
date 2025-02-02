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
 *  interpreter_other.h - Altres instruccions aïllades.
 *
 */



/*****************************************/
/* AAD - ASCII Adjust AX Before Division */
/*****************************************/

static void
aad (
     PROTO_INTERP
     )
{

  uint8_t imm8;
  
  
  IB ( &imm8, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  AL= (uint8_t) ((((uint16_t) AH)*((uint16_t) imm8) + ((uint16_t) AL))&0xFF);
  AH= 0;
  SETU8_SF_ZF_PF_FLAGS ( AL );
  
} // end aad


/****************************************/
/* AAM - ASCII Adjust AX After Multiply */
/****************************************/

static void
aam (
     PROTO_INTERP
     )
{

  uint8_t imm8,tmp;
  
  
  IB ( &imm8, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( imm8 != 0 )
    {
      tmp= AL;
      AH= tmp/imm8;
      AL= tmp%imm8;
      SETU8_SF_ZF_PF_FLAGS ( AL );
    }
  else { EXCEPTION ( EXCP_DE ); }
  
} // end aam




/*******************************************/
/* AAS - ASCII Adjust AL After Subtraction */
/*******************************************/

static void
aas (
     PROTO_INTERP
     )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( (AL&0xF) > 9 || (EFLAGS&AF_FLAG)!=0 )
    {
      AX-= 6;
      AH-= 1;
      EFLAGS|= (AF_FLAG|CF_FLAG);
    }
  else EFLAGS&= ~(AF_FLAG|CF_FLAG);
  AL&= 0x0F;
  
} // end aas




/********************************************/
/* BOUND - Check Array Index Against Bounds */
/********************************************/

static void
bound16 (
         PROTO_INTERP
         )
{
  
  uint8_t modrmbyte;
  eaddr_r16_t eaddr;
  uint16_t *reg, lower,upper;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &lower, return );
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &upper, return );
  printf("[CAL IMPLEMENTAR] BOUND16 - IND:%d LOWER:%d UPPER+2:%d\n",
         (int16_t) *reg, (int16_t) lower, ((int16_t) upper)+2 );
  exit(EXIT_FAILURE);
  if ( ((int16_t) *reg) < ((int16_t) lower) ||
       ((int16_t) *reg) > (((int16_t) upper) + 2) )
    { EXCEPTION ( EXCP_BR ); return; }
  
} // end bound16


static void
bound32 (
         PROTO_INTERP
         )
{

  uint8_t modrmbyte;
  eaddr_r32_t eaddr;
  uint32_t *reg, lower, upper;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &lower, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &upper, return );
  printf("[CAL IMPLEMENTAR] BOUND32 - IND:%d LOWER:%d UPPER+4:%d\n",
         (int32_t) *reg, (int32_t) lower, ((int32_t) upper)+4 );
  exit(EXIT_FAILURE);
  if ( ((int32_t) *reg) < ((int32_t) lower) ||
       ((int32_t) *reg) > (((int32_t) upper) + 4) )
    { EXCEPTION ( EXCP_BR ); return; }
  
} // end bound32


static void
bound (
       PROTO_INTERP
       )
{
  if ( OPERAND_SIZE_IS_32 ) bound32 ( INTERP );
  else                      bound16 ( INTERP );
} // end bound




/*************************************************************/
/* CBW/CWDE/CDQE - Convert Byte to Word/Convert Word to      */
/*                 Doubleword/Convert Doubleword to Quadword */
/*************************************************************/

static void
cbw_cwde (
          PROTO_INTERP
          )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      EAX= (uint32_t) ((int32_t) ((int16_t) AX));
    }
  else
    {
      AX= (uint16_t) ((int16_t) ((int8_t) AL));
    }
    
} // end cbw_cwde




/**************************/
/* CLC - Clear Carry Flag */
/**************************/

static void
clc (
     PROTO_INTERP
     )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  EFLAGS&= ~CF_FLAG;
  
} // end clc




/******************************/
/* CLD - Clear Direction Flag */
/******************************/

static void
cld (
     PROTO_INTERP
     )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  EFLAGS&= ~DF_FLAG;
  
} // end cld




/******************************/
/* CLI - Clear Interrupt Flag */
/******************************/

static void
cli (
     PROTO_INTERP
     )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( (EFLAGS&VM_FLAG) ) // Virtual mode
        {
          if ( IOPL == 3 ) { EFLAGS&= ~IF_FLAG; }
          else if ( IOPL < 3 && (CR4&CR4_VME)!=0 ) { EFLAGS&= ~VIF_FLAG; }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
        }
      else
        {
          if ( IOPL >= CPL ) { EFLAGS&= ~IF_FLAG; }
          else if ( IOPL < CPL && CPL == 3 && (CR4&CR4_PVI)!=0 )
            { EFLAGS&= ~VIF_FLAG; }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
        }
    }

  // Sense protecció
  else EFLAGS&= ~IF_FLAG;
  
} // end cli




/******************************************/
/* CLTS - Clear Task-Switched Flag in CR0 */
/******************************************/

static void
clts (
      PROTO_INTERP
      )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( CPL != 0 || (EFLAGS&VM_FLAG)!=0 )
        { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
    }
  CR0&= ~CR0_TS;
  
} // end clts




/*******************************/
/* CMC - Complement Carry Flag */
/*******************************/

static void
cmc (
     PROTO_INTERP
     )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( (EFLAGS&CF_FLAG) != 0 ) EFLAGS&= ~CF_FLAG;
  else                         EFLAGS|= CF_FLAG;
  
} // end cmc




/******************************/
/* CPUID - CPU Identification */
/******************************/

static void
cpuid (
       PROTO_INTERP
       )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IA32_cpu_cpuid ( CPU );
  
} // end cpuid




/***************************************************************************/
/* CWD/CDQ/CQO - Convert Word to Doubleword/Convert Doubleword to Quadword */
/***************************************************************************/

static void
cwd_cdq (
         PROTO_INTERP
         )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      EDX= (EAX&0x80000000)!=0 ? 0xFFFFFFFF : 0x00000000;
    }
  else
    {
      DX= (AX&0x8000)!=0 ? 0xFFFF : 0x0000;
    }
  
} // end cwd_cdq




/******************************************/
/* DAA - Decimal Adjust AL after Addition */
/******************************************/

static void
daa (
     PROTO_INTERP
     )
{

  uint8_t old_AL,tmp;
  bool old_CF;
  

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  old_AL= AL;
  old_CF= ((EFLAGS&CF_FLAG)!=0);
  if ( (AL&0xF) > 9 || (EFLAGS&AF_FLAG)!=0 )
    {
      tmp= AL+6;
      if ( tmp < AL ) EFLAGS|= CF_FLAG;
      AL= tmp;
      EFLAGS|= AF_FLAG;
    }
  else EFLAGS&= ~AF_FLAG;
  if ( old_AL > 0x99 || old_CF )
    {
      AL+= 0x60;
      EFLAGS|= CF_FLAG;
    }
  else EFLAGS&= ~CF_FLAG;
  SETU8_SF_ZF_PF_FLAGS ( AL );
  
} // end daa




/*********************************************/
/* DAS - Decimal Adjust AL after Subtraction */
/*********************************************/

static void
das (
     PROTO_INTERP
     )
{
  
  uint8_t old_AL,tmp;
  bool old_CF;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  old_AL= AL;
  old_CF= ((EFLAGS&CF_FLAG)!=0);
  if ( (AL&0xF) > 9 || (EFLAGS&AF_FLAG)!=0 )
    {
      tmp= AL-6;
      if ( tmp > AL ) EFLAGS|= CF_FLAG;
      AL= tmp;
      EFLAGS|= AF_FLAG;
    }
  else EFLAGS&= ~AF_FLAG;
  if ( old_AL > 0x99 || old_CF )
    {
      AL-= 0x60;
      EFLAGS|= CF_FLAG;
    }
  else EFLAGS&= ~CF_FLAG;
  SETU8_SF_ZF_PF_FLAGS ( AL );
  
} // end das




/*****************************************************/
/* ENTER - Make Stack Frame for Procedure Parameters */
/*****************************************************/

static void
enter_op16 (
            PROTO_INTERP
            )
{

  uint16_t alloc_size;
  uint8_t nesting_level;
  uint16_t frame_tmp,tmp;
  int i;
  
  
  // Obté dades
  IW ( &alloc_size, return );
  IB ( &nesting_level, return );
  nesting_level%= 32;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  // Apila EBP
  PUSHW ( BP, return );
  frame_tmp= SP;

  // Display per a nestinglevel>0
  for ( i= 1; i < nesting_level; ++i )
    {
      if ( P_SS->h.is32 )
        {
          EBP-= 2;
          READW_DATA ( P_SS, EBP, &tmp, return );
        }
      else
        {
          BP-= 2;
          READW_DATA ( P_SS, (uint32_t) BP, &tmp, return );
        }
      PUSHW ( tmp, return );
    }
  if ( nesting_level > 0 ) { PUSHW ( frame_tmp, return ); }

  // Reserva memòria.
  BP= frame_tmp;
  SP-= alloc_size;
  
  // NOTA!! No vaig a comprovar la #SS perquè en realitat quan es
  // gaste en la següent instrucció es generarà la exepció.
  
} // end enter_op16


static void
enter_op32 (
            PROTO_INTERP
            )
{

  uint16_t alloc_size;
  uint8_t nesting_level;
  uint32_t frame_tmp,tmp;
  int i;
  
  
  // Obté dades
  IW ( &alloc_size, return );
  IB ( &nesting_level, return );
  nesting_level%= 32;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  // Apila EBP
  PUSHD ( EBP, return );
  frame_tmp= ESP;

  // Display per a nestinglevel>0
  for ( i= 1; i < nesting_level; ++i )
    {
      if ( P_SS->h.is32 )
        {
          EBP-= 4;
          READD_DATA ( P_SS, EBP, &tmp, return );
        }
      else
        {
          BP-= 4;
          READD_DATA ( P_SS, (uint32_t) BP, &tmp, return );
        }
      PUSHD ( tmp, return );
    }
  if ( nesting_level > 0 ) { PUSHD ( frame_tmp, return ); }

  // Reserva memòria.
  EBP= frame_tmp;
  ESP-= (uint32_t) alloc_size;

  // NOTA!! No vaig a comprovar la #SS perquè en realitat quan es
  // gaste en la següent instrucció es generarà la exepció.
  
} // end enter_op32


static void
enter (
       PROTO_INTERP
       )
{
  OPERAND_SIZE_IS_32 ? enter_op32 ( INTERP ) : enter_op16 ( INTERP );
} // end enter




/**************/
/* HLT - Halt */
/**************/

static void
hlt (
     PROTO_INTERP
     )
{
  
  if ( (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG)) && CPL != 0 )
    { EXCEPTION0 ( EXCP_GP ); return; }
  INTERP->_halted= true;
  
} // end hlt




/**************************************************/
/* INT n/INTO/INT 3 - Call to Interrupt Procedure */
/**************************************************/

static void
int3 (
      PROTO_INTERP
      )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( INTERP->trace_soft_int != NULL )
    INTERP->trace_soft_int ( INTERP->udata, 3, INTERP->cpu );
  INTERRUPTION ( 3, INTERRUPTION_TYPE_INT3, return );
  
} // end int3


static void
int_imm8 (
          PROTO_INTERP
          )
{
  
  uint8_t val;
  
  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( INTERP->trace_soft_int != NULL )
    INTERP->trace_soft_int ( INTERP->udata, val, INTERP->cpu );
  INTERRUPTION ( val, INTERRUPTION_TYPE_IMM, return );
  
} // end int_imm8


static void
into (
      PROTO_INTERP
      )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( (EFLAGS&OF_FLAG) != 0 )
    {
      if ( INTERP->trace_soft_int != NULL )
        INTERP->trace_soft_int ( INTERP->udata, 4, INTERP->cpu );
      INTERRUPTION ( 4, INTERRUPTION_TYPE_INTO, return );
    }
  
} // end into




/*********************************/
/* IRET/IRETD - Interrupt Return */
/*********************************/

static void
iret_real (
           PROTO_INTERP
           )
{

  uint32_t tmp_eip,tmp32;
  uint16_t tmp16;
  

  // Llig valors
  if ( OPERAND_SIZE_IS_32 )
    {
      POPD ( &tmp_eip, return );
      POPD ( &tmp32, return );
      P_CS->v= (uint16_t) (tmp32&0xFFFF);
      P_CS->h.lim.addr= ((uint32_t) P_CS->v)<<4;
      POPD ( &tmp32, return );
      EFLAGS= (tmp32&0x257FD5) | (EFLAGS&0x1A0000) | EFLAGS_1S;
    }
  else
    {
      POPW ( &tmp16, return );
      tmp_eip= (uint32_t) tmp16;
      POPW ( &(P_CS->v), return );
      P_CS->h.lim.addr= ((uint32_t) P_CS->v)<<4;
      POPW ( &tmp16, return );
      EFLAGS= (EFLAGS&0xFFFF0000) | ((uint32_t) tmp16) | EFLAGS_1S;
    }
  P_CS->h.lim.firstb= 0;
  P_CS->h.lim.lastb= 0xFFFF;
  P_CS->h.is32= false;
  P_CS->h.readable= true;
  P_CS->h.writable= true;
  P_CS->h.executable= true;
  P_CS->h.isnull= false;
  P_CS->h.data_or_nonconforming= false;
  
  // Comprovacions finals.
  if (  tmp_eip > 0xFFFF )
    {
      EXCEPTION ( EXCP_GP );
      return;
    }
  EIP= tmp_eip;
  
} // end iret_real


static void
iret_protected_same_pl (
                        PROTO_INTERP,
                        const uint16_t selector,
                        const uint32_t tmp_eflags
                        )
{

  seg_desc_t desc;
  IA32_SegmentRegister sreg;
  uint32_t eflags_mask;
  
  
  // RETURN-TO-SAME-PRIVILEGE-LEVEL: (* PE = 1, RPL = CPL *)

  // Llig descriptor i comprova limits.
  if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
    return;
  load_segment_descriptor ( &sreg, selector, &desc );
  if ( EIP < sreg.h.lim.firstb || EIP > sreg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }

  // Fixa EFLAGS
  eflags_mask= (CF_FLAG|PF_FLAG|AF_FLAG|ZF_FLAG|SF_FLAG|
                TF_FLAG|DF_FLAG|OF_FLAG|NT_FLAG);
  if ( OPERAND_SIZE_IS_32 ) eflags_mask|= (RF_FLAG|AC_FLAG|ID_FLAG);
  if ( CPL <= IOPL ) eflags_mask|= IF_FLAG;
  if ( CPL == 0 )
    {
      eflags_mask|= IOPL_FLAG;
      if ( OPERAND_SIZE_IS_32 ) eflags_mask|= (VIF_FLAG|VIP_FLAG);
    }
  EFLAGS= (EFLAGS&(~eflags_mask))|(tmp_eflags&eflags_mask)|EFLAGS_1S;

  // Copia sreg
  *(P_CS)= sreg;
  
} // end iret_protected_same_pl


static void
iret_protected_outer_pl (
                         PROTO_INTERP,
                         const uint16_t selector,
                         const uint32_t tmp_eflags
                         )
{

  seg_desc_t desc;
  IA32_SegmentRegister sreg;
  uint32_t eflags_mask,tmp32_SS,tmp_ESP,new_SS_stype,rpl,rpl_SS,dpl_SS;
  uint16_t tmp16,new_SS_selector;
  seg_desc_t new_SS_desc;
  IA32_SegmentRegister new_SS;
  
  
  // IMPORTANT!!! EN LA DOCUMENTACIÓ FALTA UNA PART. LA DE TORNAR A
  // LLEGIR LA PILA.
  if ( OPERAND_SIZE_IS_32 )
    {
      POPD ( &tmp_ESP, return );
      POPD ( &tmp32_SS, return );
    }
  else
    {
      POPW ( &tmp16, return );
      tmp_ESP= (uint16_t) tmp16;
      POPW ( &tmp16, return );
      tmp32_SS= (uint16_t) tmp16;
    }
  
  // Llig descriptor i comprova limits.
  if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
    return;
  load_segment_descriptor ( &sreg, selector, &desc );
  if ( EIP < sreg.h.lim.firstb || EIP > sreg.h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
  
  // Fixa EFLAGS
  eflags_mask= (CF_FLAG|PF_FLAG|AF_FLAG|ZF_FLAG|SF_FLAG|
                TF_FLAG|DF_FLAG|OF_FLAG|NT_FLAG);
  if ( OPERAND_SIZE_IS_32 ) eflags_mask|= (RF_FLAG|AC_FLAG|ID_FLAG);
  if ( CPL <= IOPL ) eflags_mask|= IF_FLAG;
  if ( CPL == 0 )
    {
      eflags_mask|= IOPL_FLAG;
      if ( OPERAND_SIZE_IS_32 ) eflags_mask|= (VM_FLAG|VIF_FLAG|VIP_FLAG);
    }
  EFLAGS= (EFLAGS&(~eflags_mask))|(tmp_eflags&eflags_mask)|EFLAGS_1S;
  
  // Actualitza CPL
  sreg.h.pl= (uint8_t) (selector&0x3); // CPL= rpl

  // Carrega SS (sense comprovacions?????? No té sentit!!!!)
  new_SS_selector= (uint16_t) (tmp32_SS&0xFFFF);
  if ( read_segment_descriptor ( INTERP, new_SS_selector,
                                 &new_SS_desc, true ) != 0 )
    return;
  // Fique algunes comprovacions que he tret de ret_outer_pl
  new_SS_stype= SEG_DESC_GET_STYPE ( new_SS_desc );
  rpl= selector&0x3;
  rpl_SS= new_SS_selector&0x3;
  dpl_SS= SEG_DESC_GET_DPL ( new_SS_desc );
  if ( rpl != rpl_SS ||
       rpl != dpl_SS ||
       (new_SS_stype&0x1A)!=0x12 // no és data writable
       )
    { EXCEPTION_SEL ( EXCP_GP, selector ); return; }
  if ( !SEG_DESC_IS_PRESENT ( new_SS_desc ) )
    { EXCEPTION_SEL ( EXCP_SS, new_SS_selector ); return; }
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
  *(P_SS)= new_SS;
  ESP= tmp_ESP;
  
  // Comprova DPL segments.
  // --> ES
  if ( P_ES->h.dpl < sreg.h.pl && P_ES->h.data_or_nonconforming )
    { P_ES->h.isnull= true; P_ES->v&= 0x0003; }
  // --> FS
  if ( P_FS->h.dpl < sreg.h.pl && P_FS->h.data_or_nonconforming )
    { P_FS->h.isnull= true; P_FS->v&= 0x0003; }
  // --> GS
  if ( P_GS->h.dpl < sreg.h.pl && P_GS->h.data_or_nonconforming )
    { P_GS->h.isnull= true; P_GS->v&= 0x0003; }
  // --> DS
  if ( P_DS->h.dpl < sreg.h.pl && P_DS->h.data_or_nonconforming )
    { P_DS->h.isnull= true; P_DS->v&= 0x0003; }
  
  // Copia sreg
  *(P_CS)= sreg;
  
} // end iret_protected_outer_pl


static void
iret_to_virtual8086_mode (
                          PROTO_INTERP,
                          const uint16_t selector,
                          const uint32_t tmp_eflags
                          )
{

  uint32_t tmp32,tmp32_SS,tmp_ESP;

  
  // RETURN-TO-VIRTUAL-8086-MODE:
  // (* Interrupted procedure was in virtual-8086 mode:
  //    PE = 1, CPL=0, VM = 1 in flag image *)

  // NOTA!!!! No queda gens clar si al carregar els selectors es
  // consulta el GDT o directament es carrega com si fora mode
  // real. Vaig a assumir el segon perquè amb el primer tinc errors
  // raros.

  // Fica CS
  if ( EIP < P_CS->h.lim.firstb || EIP > P_CS->h.lim.lastb )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
  P_CS->v= (uint16_t) (selector&0xFFFF);
  P_CS->h.lim.addr= ((uint32_t) P_CS->v)<<4;
  P_CS->h.lim.firstb= 0;
  P_CS->h.lim.lastb= 0xFFFF;
  P_CS->h.pl= 3; // Es fixa
  P_CS->h.is32= false;
  P_CS->h.readable= true;
  P_CS->h.writable= true;
  P_CS->h.executable= true;
  P_CS->h.isnull= false;
  P_CS->h.data_or_nonconforming= false;
  
  // NOTA!!! SI HO HE ENTÈS BÉ ACÍ ENS CARREGUEM EL DESCRIPTOR
  // CACHE. NO VAL EL UNREAL MODE.
  // Fixa EFLAGS i carrega registres
  EFLAGS= tmp_eflags | EFLAGS_1S;
  POPD ( &tmp_ESP, return );
  // --> SS
  POPD ( &tmp32_SS, return );
  // --> ES
  POPD ( &tmp32, return );
  P_ES->v= (uint16_t) (tmp32&0xFFFF);
  P_ES->h.lim.addr= ((uint32_t) P_ES->v)<<4;
  P_ES->h.lim.firstb= 0;
  P_ES->h.lim.lastb= 0xFFFF;
  P_ES->h.is32= false;
  P_ES->h.readable= true;
  P_ES->h.writable= true;
  P_ES->h.executable= true;
  P_ES->h.isnull= false;
  P_ES->h.data_or_nonconforming= true;
  // --> DS
  POPD ( &tmp32, return );
  P_DS->v= (uint16_t) (tmp32&0xFFFF);
  P_DS->h.lim.addr= ((uint32_t) P_DS->v)<<4;
  P_DS->h.lim.firstb= 0;
  P_DS->h.lim.lastb= 0xFFFF;
  P_DS->h.is32= false;
  P_DS->h.readable= true;
  P_DS->h.writable= true;
  P_DS->h.executable= true;
  P_DS->h.isnull= false;
  P_DS->h.data_or_nonconforming= true;
  // --> FS
  POPD ( &tmp32, return );
  P_FS->v= (uint16_t) (tmp32&0xFFFF);
  P_FS->h.lim.addr= ((uint32_t) P_FS->v)<<4;
  P_FS->h.lim.firstb= 0;
  P_FS->h.lim.lastb= 0xFFFF;
  P_FS->h.is32= false;
  P_FS->h.readable= true;
  P_FS->h.writable= true;
  P_FS->h.executable= true;
  P_FS->h.isnull= false;
  P_FS->h.data_or_nonconforming= true;
  // --> GS
  POPD ( &tmp32, return );
  P_GS->v= (uint16_t) (tmp32&0xFFFF);
  P_GS->h.lim.addr= ((uint32_t) P_GS->v)<<4;
  P_GS->h.lim.firstb= 0;
  P_GS->h.lim.lastb= 0xFFFF;
  P_GS->h.is32= false;
  P_GS->h.readable= true;
  P_GS->h.writable= true;
  P_GS->h.executable= true;
  P_GS->h.isnull= false;
  P_GS->h.data_or_nonconforming= true;
  // Acaba SS i ESP
  P_SS->v= (uint16_t) (tmp32_SS&0xFFFF);
  P_SS->h.lim.addr= ((uint32_t) P_SS->v)<<4;
  P_SS->h.lim.firstb= 0;
  P_SS->h.lim.lastb= 0xFFFF;
  P_SS->h.is32= false;
  P_SS->h.readable= true;
  P_SS->h.writable= true;
  P_SS->h.executable= true;
  P_SS->h.isnull= false;
  P_SS->h.data_or_nonconforming= true;
  ESP= tmp_ESP;
  
} // end iret_to_virtual8086_mode


static void
iret_protected (
                PROTO_INTERP
                )
{

  uint16_t selector,tmp16;
  uint32_t tmp32,tmp_eflags;
  uint8_t rpl;

  
  // GOTO TASK-RETURN; (* PE = 1, VM = 0, NT = 1 *)
  if ( EFLAGS&NT_FLAG )
    {
      READLW ( TR.h.lim.addr, &selector, true, true, return );
      if ( switch_task ( INTERP, SWITCH_TASK_IRET, selector, 0 ) != 0 )
        return;
      if ( EIP < P_CS->h.lim.firstb || EIP > P_CS->h.lim.lastb )
        { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
    }
  else
    {
      // Recupera EIP i CS
      if ( OPERAND_SIZE_IS_32 )
        {
          POPD ( &EIP, return );
          POPD ( &tmp32, return );
          selector= (uint16_t) (tmp32&0xFFFF);
          POPD ( &tmp_eflags, return );
        }
      else
        {
          POPW ( &tmp16, return );
          EIP= (uint32_t) tmp16;
          POPW ( &selector, return );
          POPW ( &tmp16, return );
          tmp_eflags= (uint32_t) tmp16;
        }

      // NOTA!!!! No carregue el selector encara!!! Ho deixe per al final
      if ( tmp_eflags&VM_FLAG && CPL == 0 )
        iret_to_virtual8086_mode ( INTERP, selector, tmp_eflags );
      else
        {
          rpl= selector&0x3;
          if ( rpl > CPL )
            iret_protected_outer_pl ( INTERP, selector, tmp_eflags );
          else iret_protected_same_pl ( INTERP, selector, tmp_eflags );
        }
    }
  
} // end iret_protected


static void
iret_v86 (
          PROTO_INTERP
          )
{
  
  uint32_t tmp_eip,tmp32,emask;
  uint16_t tmp16;
  

  // Comprovació IOPL
  if ( IOPL != 3 ) { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
  
  // Llig valors
  if ( OPERAND_SIZE_IS_32 )
    {
      POPD ( &tmp_eip, return );
      POPD ( &tmp32, return );
      P_CS->v= (uint16_t) (tmp32&0xFFFF);
      P_CS->h.lim.addr= ((uint32_t) P_CS->v)<<4;
      POPD ( &tmp32, return );
      emask= VM_FLAG|IOPL_FLAG|VIP_FLAG|VIF_FLAG;
      EFLAGS= (tmp32&(~emask)) | (EFLAGS&emask) | EFLAGS_1S;
    }
  else
    {
      POPW ( &tmp16, return );
      tmp_eip= (uint32_t) tmp16;
      POPW ( &(P_CS->v), return );
      P_CS->h.lim.addr= ((uint32_t) P_CS->v)<<4;
      POPW ( &tmp16, return );
      EFLAGS=
        (EFLAGS&(0xFFFF0000|IOPL_FLAG)) |
        ((uint32_t) (tmp16&(~IOPL_FLAG))) |
        EFLAGS_1S;
    }
  P_CS->h.lim.firstb= 0;
  P_CS->h.lim.lastb= 0xFFFF;
  P_CS->h.is32= false;
  P_CS->h.readable= true;
  P_CS->h.writable= true;
  P_CS->h.executable= true;
  P_CS->h.isnull= false;
  P_CS->h.data_or_nonconforming= false;

  // Comprovacions finals.
  if (  tmp_eip > 0xFFFF )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
  EIP= tmp_eip;
  
} // end iret_v86


static void
iret (
      PROTO_INTERP
      )
{

  if ( !PROTECTED_MODE_ACTIVATED ) iret_real ( INTERP );
  else
    {
      if ( EFLAGS&VM_FLAG ) iret_v86 ( INTERP );
      else                  iret_protected ( INTERP );
    }
  
} // end iret




/*********************************************/
/* LAHF - Load Status Flags into AH Register */
/*********************************************/

static void
lahf (
      PROTO_INTERP
      )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  AH=
    (((EFLAGS&SF_FLAG)!=0) ? 0x80 : 0x00) |
    (((EFLAGS&ZF_FLAG)!=0) ? 0x40 : 0x00) |
    (((EFLAGS&AF_FLAG)!=0) ? 0x10 : 0x00) |
    (((EFLAGS&PF_FLAG)!=0) ? 0x04 : 0x00) |
    0x02 |
    (((EFLAGS&CF_FLAG)!=0) ? 0x01 : 0x00)
    ;
  
} // end lahf




/*********************************/
/* LAR - Load Access Rights Byte */
/*********************************/

static bool
lar_check_segment_descriptor (
                              PROTO_INTERP,
                              const uint16_t   selector,
                              const seg_desc_t desc
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


static void
lar_op16 (
          PROTO_INTERP
          )
{

  eaddr_r16_t eaddr;
  uint16_t selector,*reg;
  seg_desc_t desc;
  uint8_t modrmbyte;

  
  // Obté dades
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) )
    { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    { READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &selector, return ); }
  else selector= *(eaddr.v.reg);

  // Operació. ATENCIÓ! no llenera execpció si està fora dels límits.
  if ( read_segment_descriptor ( INTERP, selector, &desc, false ) == 0 &&
       lar_check_segment_descriptor ( INTERP, selector, desc ) )
    {
      *reg= (uint16_t) (desc.w0&0xFF00);
      EFLAGS|= ZF_FLAG;
    }
  else { EFLAGS&= ~ZF_FLAG; }
  
} // end lar_op16


static void
lar_op32 (
          PROTO_INTERP
          )
{

  eaddr_r32_t eaddr;
  uint16_t selector;
  uint32_t *reg;
  seg_desc_t desc;
  uint8_t modrmbyte;

  
  // Obté dades
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) )
    { EXCEPTION ( EXCP_UD ); return; }
  // NOTA!! Com és un selector llig 16bits.
  if ( eaddr.is_addr )
    { READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &selector, return ); }
  else selector= (uint16_t) *(eaddr.v.reg);

  // Operació. ATENCIÓ! no llenera execpció si està fora dels límits.
  if ( read_segment_descriptor ( INTERP, selector, &desc, false ) == 0 &&
       lar_check_segment_descriptor ( INTERP, selector, desc ) )
    {
      *reg= desc.w0&0x00F0FF00;
      EFLAGS|= ZF_FLAG;
    }
  else { EFLAGS&= ~ZF_FLAG; }
  
} // end lar_op32


static void
lar (
     PROTO_INTERP
     )
{
  OPERAND_SIZE_IS_32 ? lar_op32 ( INTERP ) : lar_op16 ( INTERP );
} // end lar




/******************************************/
/* LDS/LES/LFS/LGS/LSS - Load Far Pointer */
/******************************************/

static void
lsreg_r16_m16_16 (
                  PROTO_INTERP,
                  IA32_SegmentRegister *sreg
                  )
{

  uint16_t *reg;
  uint16_t selector,offset;
  eaddr_r16_t eaddr;
  uint8_t modrmbyte;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &offset, return );
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &selector, return );
  
  // Carrega
  if ( set_sreg ( INTERP, selector, sreg ) )
    return;
  *reg= offset;
  
} // end lsreg_r16_m16_16


static void
lsreg_r32_m16_32 (
                  PROTO_INTERP,
                  IA32_SegmentRegister *sreg
                  )
{
  
  uint32_t *reg;
  uint16_t selector;
  uint32_t offset;
  eaddr_r32_t eaddr;
  uint8_t modrmbyte;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &offset, return );
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &selector, return );

  // Carrega
  if ( set_sreg ( INTERP, selector, sreg ) )
    return;
  *reg= offset;
  
} // end lsreg_r32_m16_32


static void
lsreg_r_m16_off (
                 PROTO_INTERP,
                 IA32_SegmentRegister *sreg
                 )
{
  OPERAND_SIZE_IS_32 ?
    lsreg_r32_m16_32 ( INTERP, sreg ) :
    lsreg_r16_m16_16 ( INTERP, sreg );
} // end lsreg_r_m16_off


static void
lds_r_m16_off (
               PROTO_INTERP
               )
{
  lsreg_r_m16_off ( INTERP, P_DS );
} // end lds_r_m16_off


static void
lss_r_m16_off (
               PROTO_INTERP
               )
{
  lsreg_r_m16_off ( INTERP, P_SS );
} // end lss_r_m16_off


static void
lfs_r_m16_off (
               PROTO_INTERP
               )
{
  lsreg_r_m16_off ( INTERP, P_FS );
} // end lfs_r_m16_off


static void
lgs_r_m16_off (
               PROTO_INTERP
               )
{
  lsreg_r_m16_off ( INTERP, P_GS );
} // end lgs_r_m16_off


static void
les_r_m16_off (
               PROTO_INTERP
               )
{
  lsreg_r_m16_off ( INTERP, P_ES );
} // end les_r_m16_off




/********************************/
/* LEA - Load Effective Address */
/********************************/

// NOTA!!! get_addr_mode_r? ja tracta el tema de ADDRESS_SIZE_IS_32
static void
lea_r16_m (
           PROTO_INTERP
           )
{
  
  uint16_t *reg;
  eaddr_r16_t eaddr;
  uint8_t modrmbyte;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  *reg= (uint16_t) eaddr.v.addr.off;
  
} // end lea_r16_m


static void
lea_r32_m (
           PROTO_INTERP
           )
{
  
  uint32_t *reg;
  eaddr_r32_t eaddr;
  uint8_t modrmbyte;

  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  *reg= eaddr.v.addr.off;
  
} // end lea_r32_m


static void
lea_r_m (
         PROTO_INTERP
         )
{
  OPERAND_SIZE_IS_32 ?
    lea_r32_m ( INTERP ) :
    lea_r16_m ( INTERP );
} // end lea_r_m




/*************************************/
/* LEAVE - High Level Procedure Exit */
/*************************************/

static void
leave (
       PROTO_INTERP
       )
{

  if ( P_SS->h.is32 ) ESP= EBP;
  else                SP= BP;
  if ( OPERAND_SIZE_IS_32 ) { POPD ( &EBP, return ); }
  else                      { POPW ( &BP, return ); }
  
} // end leave




/***************************************************************/
/* LGDT/LIDT - Load Global/Interrupt Descriptor Table Register */
/***************************************************************/

static void
lidt_op16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r16_t eaddr;
  uint16_t limit;
  uint32_t base;

  
  // Obté dades
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &limit, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &base, return );
  if ( CPL != 0 && // <-- Nivel seguritat 0!!!
       (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG)) )
    { EXCEPTION0 ( EXCP_GP ); return; }
  
  // Fixa valors
  IDTR.addr= base&0xFFFFFF;
  IDTR.firstb= 0;
  IDTR.lastb= limit;
  
} // end lidt_op16


static void
lidt_op32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r32_t eaddr;
  uint16_t limit;
  uint32_t base;

  
  // Obté dades
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &limit, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &base, return );
  if ( CPL != 0 && // <-- Nivel seguritat 0!!!
       (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG)) )
    { EXCEPTION0 ( EXCP_GP ); return; }

  // Fixa valors
  IDTR.addr= base;
  IDTR.firstb= 0;
  IDTR.lastb= limit;
  
} // end lidt_op32


static void
lidt (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{
  OPERAND_SIZE_IS_32 ?
    lidt_op32 ( INTERP, modrmbyte ) :
    lidt_op16 ( INTERP, modrmbyte );
} // end lidt


static void
lgdt_op16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r16_t eaddr;
  uint16_t limit;
  uint32_t base;

  
  // Obté dades
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &limit, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &base, return );
  if ( CPL != 0 && // <-- Nivel seguritat 0!!!
       (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG)) )
    { EXCEPTION0 ( EXCP_GP ); return; }

  // Fixa valors
  GDTR.addr= base&0xFFFFFF;
  GDTR.firstb= 0;
  GDTR.lastb= limit;
  
} // end lgdt_op16


static void
lgdt_op32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r32_t eaddr;
  uint16_t limit;
  uint32_t base;

  
  // Obté dades
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &limit, return );
  READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &base, return );
  if ( CPL != 0 && // <-- Nivel seguritat 0!!!
       (PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG)) )
    { EXCEPTION0 ( EXCP_GP ); return; }

  // Fixa valors
  GDTR.addr= base;
  GDTR.firstb= 0;
  GDTR.lastb= limit;
  
} // end lgdt_op32


static void
lgdt (
       PROTO_INTERP,
       const uint8_t modrmbyte
       )
{
  OPERAND_SIZE_IS_32 ?
    lgdt_op32 ( INTERP, modrmbyte ) :
    lgdt_op16 ( INTERP, modrmbyte );
} // end lgdt




/***********************************************/
/* LLDT - Load Local Descriptor Table Register */
/***********************************************/

static void
lldt (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{

  eaddr_r16_t eaddr;
  uint16_t *reg,selector;
  seg_desc_t desc;
  uint32_t cpl,stype,elimit;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) goto ud_exc;
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) ) goto ud_exc;

  // Obté selector
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &selector, return );
    }
  else selector= *(eaddr.v.reg);

  // ATENCIÓ!!!! Es supossa que l'ordre de comprovacions seria:
  // 1.- Limits
  // 2.- Si és vàlid (no NULL), si no ho és parem d'executar
  // 3.- Lectura (Pot generar més execpcions)
  //
  // No tinc clar què fer ací, llançaré un GP.
  if ( selector&0x04 ) goto gp_exc;
  // Però per comoditat vaig a fer 1-3-2.
  if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
    return;
  if ( (selector&0xFFFC) == 0 ) LDTR.h.isnull= true;
  else
    {
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
    }
  
  return;

 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return;
 gp_exc:
  EXCEPTION0 ( EXCP_GP );
  return;
 ud_exc:
  EXCEPTION ( EXCP_UD );
  return;
  
} // end lldt




/***********************************/
/* LMSW - Load Machine Status Word */
/***********************************/

static void
lmsw (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{

  eaddr_r16_t eaddr;
  uint16_t tmp;
  
  
  // Obté dades
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  // OJO!!! Que pareix que en Virtual-8086 Mode Exceptions no s'accepta.
  if ( PROTECTED_MODE_ACTIVATED && ( CPL != 0 || (EFLAGS&VM_FLAG) ) )
    { EXCEPTION0 ( EXCP_GP ); return; }

  // Carrega CR0
  if ( eaddr.is_addr )
    { READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &tmp, return ); }
  else tmp= *(eaddr.v.reg);
  if ( CR0&CR0_PE ) tmp|= (uint16_t) CR0_PE;
  CR0= (CR0&(~0xF)) | ((uint32_t) (tmp&0xF));
  update_mem_callbacks ( INTERP );
  if ( tmp&(CR0_TS|CR0_EM|CR0_MP) )
    WW ( UDATA, "lmsw Falten flags en CR0: TS,EM,MP" );
  
} // end lmsw




/**********************************************/
/* LODS/LODSB/LODSW/LODSD/LODSQ - Load String */
/**********************************************/

static void
lodsb (
       PROTO_INTERP
       )
{
  
  addr_t addr;
  uint8_t val;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addr_dsesi ( INTERP, &addr );
      READB_DATA ( addr.seg, addr.off, &val, return );
      
      // Assigna
      AL= val;
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) --ESI;
          else                  ++ESI;
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) --SI;
          else                  ++SI;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;
      
    }
  
} // end lodsb


static void
lodsw (
       PROTO_INTERP
       )
{
  
  addr_t addr;
  uint16_t val;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addr_dsesi ( INTERP, &addr );
      READW_DATA ( addr.seg, addr.off, &val, return );
      
      // Assigna
      AX= val;
  
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) ESI-= 2;
          else                  ESI+= 2;
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) SI-= 2;
          else                  SI+= 2;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;

    }
  
} // end lodsw


static void
lodsd (
       PROTO_INTERP
       )
{
  
  addr_t addr;
  uint32_t val;


  // Mou.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      get_addr_dsesi ( INTERP, &addr );
      READD_DATA ( addr.seg, addr.off, &val, return );
      
      // Assigna
      EAX= val;
      
      // Actualitza punters.
      if ( ADDRESS_SIZE_IS_32 )
        {
          if ( EFLAGS&DF_FLAG ) ESI-= 4;
          else                  ESI+= 4;
        }
      else
        {
          if ( EFLAGS&DF_FLAG ) SI-= 4;
          else                  SI+= 4;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;

    }
  
} // end lodsd


static void
lods (
      PROTO_INTERP
      )
{
  OPERAND_SIZE_IS_32 ? lodsd ( INTERP ) : lodsw ( INTERP );
} // end lods




/****************************/
/* LSL - Load Segment Limit */
/****************************/

static bool
lsl_check_segment_descriptor (
                              PROTO_INTERP,
                              const uint16_t   selector,
                              const seg_desc_t desc
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


static void
lsl_op16 (
          PROTO_INTERP
          )
{
  
  eaddr_r16_t eaddr;
  uint16_t selector,*reg;
  seg_desc_t desc;
  uint8_t modrmbyte;
  uint32_t tmp;
  
  
  // Obté dades
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) )
    { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    { READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &selector, return ); }
  else selector= *(eaddr.v.reg);

  // Operació. ATENCIÓ! no llenera execpció si està fora dels límits.
  if ( read_segment_descriptor ( INTERP, selector, &desc, false ) == 0 &&
       lsl_check_segment_descriptor ( INTERP, selector, desc ) )
    {
      tmp= (desc.w0&0x000F0000)|(desc.w1&0xFFFF);
      if ( SEG_DESC_G_IS_SET ( desc ) )
        tmp= (tmp<<12) | 0xFFF;
      *reg= (uint16_t) (tmp&0xFFFF);
      EFLAGS|= ZF_FLAG;
    }
  else { EFLAGS&= ~ZF_FLAG; }
  
} // end lsl_op16


static void
lsl_op32 (
          PROTO_INTERP
          )
{

  eaddr_r32_t eaddr;
  uint16_t selector;
  uint32_t *reg;
  seg_desc_t desc;
  uint8_t modrmbyte;
  uint32_t tmp;

  
  // Obté dades
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) )
    { EXCEPTION ( EXCP_UD ); return; }
  // NOTA!! Com és un selector llig 16bits.
  if ( eaddr.is_addr )
    { READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &selector, return ); }
  else selector= (uint16_t) *(eaddr.v.reg);

  // Operació. ATENCIÓ! no llenera execpció si està fora dels límits.
  if ( read_segment_descriptor ( INTERP, selector, &desc, false ) == 0 &&
       lsl_check_segment_descriptor ( INTERP, selector, desc ) )
    {
      tmp= (desc.w0&0x000F0000)|(desc.w1&0xFFFF);
      if ( SEG_DESC_G_IS_SET ( desc ) )
        tmp= (tmp<<12) | 0xFFF;
      *reg= tmp;
      EFLAGS|= ZF_FLAG;
    }
  else { EFLAGS&= ~ZF_FLAG; }
  
} // end lsl_op32


static void
lsl (
     PROTO_INTERP
     )
{
  OPERAND_SIZE_IS_32 ? lsl_op32 ( INTERP ) : lsl_op16 ( INTERP );
} // end lsl




/****************************/
/* LTR - Load Task Register */
/****************************/

static void
ltr (
     PROTO_INTERP,
     const uint8_t modrmbyte
     )
{

  eaddr_r16_t eaddr;
  uint16_t *reg,selector;
  seg_desc_t desc;
  uint32_t cpl,stype,elimit;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) goto ud_exc;
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) ) goto ud_exc;

  // Obté selector
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &selector, return );
    }
  else selector= *(eaddr.v.reg);

  // Null selector
  if ( (selector&0xFFFC) == 0 ) goto gp_exc;

  // Tipus no global
  if ( selector&0x04 ) goto excp_gp_sel;

  // Llig
  if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
    return;
  
  // Ompli valors
  TR.h.isnull= false;
  cpl= CPL;
  if ( cpl != 0 ) goto gp_exc;
  stype= SEG_DESC_GET_STYPE ( desc );
  if ( (stype&0x17) != 0x01 ) goto excp_gp_sel; // Available TSS
  if ( !SEG_DESC_IS_PRESENT ( desc ) )
    {
      EXCEPTION_SEL ( EXCP_NP, selector );
      return;
    }
  // Carrega (No faig el que es fa normalment perquè es TR, sols
  // el rellevant per a TR).
  TR.h.is32= SEG_DESC_B_IS_SET ( desc );
  TR.h.tss_is32= ((stype&0x1D)==0x09);
  TR.h.pl= (uint8_t) (selector&0x3);
  TR.h.lim.addr=
    (desc.w1>>16) | ((desc.w0<<16)&0x00FF0000) | (desc.w0&0xFF000000);
  elimit= (desc.w0&0x000F0000) | (desc.w1&0x0000FFFF);
  if ( SEG_DESC_G_IS_SET ( desc ) )
    {
      elimit<<= 12;
      elimit|= 0xFFF;
    }
  TR.v= selector;
  TR.h.lim.firstb= 0;
  TR.h.lim.lastb= elimit;

  // Fixa busy.
  set_tss_segment_descriptor_busy_bit ( INTERP, &desc );
  
  return;
  
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return;
 gp_exc:
  EXCEPTION0 ( EXCP_GP );
  return;
 ud_exc:
  EXCEPTION ( EXCP_UD );
  return;         
  
} // end ltr




/**********************/
/* NOP - No Operation */
/**********************/

static void
nop (
     PROTO_INTERP
     )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( INTERP->_repe_repz_enabled && 0 /*INTERP->cpu->model >= PENTIUM4*/ )
    {
      printf("[EE] PAUSE NOT IMPLEMENTED\n");
      exit ( EXIT_FAILURE );
    }
  
} // end nop




/************************************/
/* POP - Pop a Value from the Stack */
/************************************/

static void
pop_rm16 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint16_t tmp;
  eaddr_r16_t eaddr;

  
  // Obté src.
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  POPW ( &tmp, return );
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, tmp, return );
    }
  else *eaddr.v.reg= tmp;
  
} // end pop_rm16


static void
pop_rm32 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{
  
  uint32_t tmp;
  eaddr_r32_t eaddr;
  
  
  // Obté src.
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  POPD ( &tmp, return );
  if ( eaddr.is_addr )
    {
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, tmp, return );
    }
  else *eaddr.v.reg= tmp;
  
} // end pop_rm32


static void
pop_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    pop_rm32 ( INTERP, modrmbyte ) :
    pop_rm16 ( INTERP, modrmbyte );
} // end pop_rm


static void
pop_r (
       PROTO_INTERP,
       const uint8_t opcode
       )
{

  uint32_t tmp32;
  uint16_t tmp16;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      switch ( opcode&0x7 )
        {
        case 0: POPD ( &EAX, return ); break;
        case 1: POPD ( &ECX, return ); break;
        case 2: POPD ( &EDX, return ); break;
        case 3: POPD ( &EBX, return ); break;
        case 4: POPD ( &tmp32, return ); ESP= tmp32; break;
        case 5: POPD ( &EBP, return ); break;
        case 6: POPD ( &ESI, return ); break;
        case 7: POPD ( &EDI, return ); break;
        }
    }
  else
    {
      switch ( opcode&0x7 )
        {
        case 0: POPW ( &AX, return ); break;
        case 1: POPW ( &CX, return ); break;
        case 2: POPW ( &DX, return ); break;
        case 3: POPW ( &BX, return ); break;
        case 4: POPW ( &tmp16, return ); SP= tmp16; break; // També???
        case 5: POPW ( &BP, return ); break;
        case 6: POPW ( &SI, return ); break;
        case 7: POPW ( &DI, return ); break;
        }
    }
  
} // end pop_r


static void
pop_sreg (
          PROTO_INTERP,
          IA32_SegmentRegister *sreg
          )
{

  uint16_t selector;
  uint32_t tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  // NOTA!! No està clar què fa amb opsize32. Vaig a assumir que la
  // part alta s'ignora.
  if ( OPERAND_SIZE_IS_32 )
    {
      POPD ( &tmp, return );
      selector= (uint16_t) tmp;
    }
  else { POPW ( &selector, return ); }
  
  // Carrega
  if ( set_sreg ( INTERP, selector, sreg ) != 0 )
    return;
  
} // end pop_sreg


static void
pop_DS (
        PROTO_INTERP
        )
{
  pop_sreg ( INTERP, P_DS );
} // end pop_DS


static void
pop_ES (
        PROTO_INTERP
        )
{
  pop_sreg ( INTERP, P_ES );
} // end pop_ES


static void
pop_SS (
        PROTO_INTERP
        )
{
  pop_sreg ( INTERP, P_SS );
} // end pop_SS


static void
pop_FS (
        PROTO_INTERP
        )
{
  pop_sreg ( INTERP, P_FS );
} // end pop_FS


static void
pop_GS (
        PROTO_INTERP
        )
{
  pop_sreg ( INTERP, P_GS );
} // end pop_GS




/**************************************************/
/* POPA/POPAD - Pop All General-Purpose Registers */
/**************************************************/

static void
popa (
      PROTO_INTERP
      )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      POPD ( &EDI, return );
      POPD ( &ESI, return );
      POPD ( &EBP, return );
      ESP+= 4;
      POPD ( &EBX, return );
      POPD ( &EDX, return );
      POPD ( &ECX, return );
      POPD ( &EAX, return );
    }
  else
    {
      POPW ( &DI, return );
      POPW ( &SI, return );
      POPW ( &BP, return );
      SP+= 2;
      POPW ( &BX, return );
      POPW ( &DX, return );
      POPW ( &CX, return );
      POPW ( &AX, return );
    }
  
} // end popa




/*****************************************************/
/* POPF/POPFD/POPFQ - Pop Stack into EFLAGS Register */
/*****************************************************/

static void
popf (
      PROTO_INTERP
      )
{

  uint32_t val,cpl,mask;
  uint16_t tmp16;
  
  
  // NOTA!!! El pseudocodi i la taula no coincideixen del tot. La
  // taula pareix més sofisticada. Així que vaig a gastar el
  // pseudocodi.
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      POPD ( &val, return );
    }
  else
    {
      POPW ( &tmp16, return );
      val= (uint32_t) tmp16;
    }
  
  // Comprovacions sobre que bits es poden o no actualitzar.
  if ( !(EFLAGS&VM_FLAG) ) // Not in Virtual-8086 Mode
    {
      cpl= CPL;
      if ( cpl == 0 )
        {
          if ( OPERAND_SIZE_IS_32 )
            {
              // All non-reserved flags except RF, VIP, VIF, and VM
              // can be modified
              // VIP, VIF, VM, and all reserved bits are
              // unaffected. RF is cleared.
              mask= (~(RF_FLAG|VIP_FLAG|VIF_FLAG|VM_FLAG))&0x003F7FD5;
              EFLAGS= (EFLAGS&(~RF_FLAG)&(~mask)) | (val&mask) | EFLAGS_1S;
            }
          else // CPL=0 OPS16
            {
              // All non-reserved flags can be modified.
              mask= 0x00007FD5;
              EFLAGS= (EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
            }
        }
      else // CPL>0
        {
          if ( OPERAND_SIZE_IS_32 )
            {
              if ( CPL > IOPL ) // CPL>0, CPL>IOPL OPS32
                {
                  // All non-reserved bits except IF, IOPL, VIP, VIF,
                  // VM and RF can be modified;
                  // IF, IOPL, VIP, VIF, VM and all reserved bits are
                  // unaffected; RF is cleared.
                  mask= (~(IF_FLAG|IOPL_FLAG|VIP_FLAG|
                           VIF_FLAG|VM_FLAG))&0x003F7FD5;
                  EFLAGS= (EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
                  EFLAGS&= ~(RF_FLAG);
                }
              else // CPL>0, CPL<=IOPL OPS32
                {
                  // All non-reserved bits except IOPL, VIP, VIF, VM
                  // and RF can be modified;
                  // IOPL, VIP, VIF, VM and all reserved bits are
                  // unaffected; RF is cleared.
                  mask= (~(IOPL_FLAG|VIP_FLAG|VIF_FLAG|VM_FLAG))&0x003F7FD5;
                  EFLAGS= (EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
                  EFLAGS&= ~(RF_FLAG);
                }
            }
          else // CPL>0 OPS16
            {
              // All non-reserved bits except IOPL can be modified;
              // IOPL and all reserved bits are unaffected.
              mask= (~IOPL_FLAG)&0x00007FD5;
              EFLAGS= (EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
            }
        }
    }
  else if ( (CR4&CR4_VME) != 0 ) // Virtual mode amb VME Enabled
    {
      printf ( "[EE] POPF no implementat en mode virtual VME!!!!\n" );
      exit ( EXIT_FAILURE );
    }
  else // Virtual mode
    {
      if ( IOPL == 3 )
        {
          if ( OPERAND_SIZE_IS_32 )
            {
              // All non-reserved bits except IOPL, VIP, VIF, VM, and
              // RF can be modified
              // VIP, VIF, VM, IOPL and all reserved bits are
              // unaffected. RF is cleared.
              mask= (~(RF_FLAG|VIP_FLAG|VIF_FLAG|VM_FLAG|IOPL_FLAG))&0x003F7FD5;
              EFLAGS= (EFLAGS&(~RF_FLAG)&(~mask)) | (val&mask) | EFLAGS_1S;
            }
          else
            {
              // All non-reserved bits except IOPL can be modified;
              // IOPL and all reserved bits are unaffected.
              mask= (~IOPL_FLAG)&0x00007FD5;
              EFLAGS= (EFLAGS&(~mask)) | (val&mask) | EFLAGS_1S;
            }
        }
      else // IOPL < 3
        { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
      
    }
  
} // end popf




/***********************************************************/
/* PUSH - Push Word, Doubleword or Quadword Onto the Stack */
/***********************************************************/

static void
push_rm16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint16_t val;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *eaddr.v.reg;
  PUSHW ( val, return );
  
} // end push_rm16


static void
push_rm32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  uint32_t val;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &val, return );
    }
  else val= *eaddr.v.reg;
  PUSHD ( val, return );
  
} // end push_rm32


static void
push_rm (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  OPERAND_SIZE_IS_32 ?
    push_rm32 ( INTERP, modrmbyte ) :
    push_rm16 ( INTERP, modrmbyte );
} // end push_rm


static void
push_r (
        PROTO_INTERP,
        const uint8_t opcode
        )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      switch ( opcode&0x7 )
        {
        case 0: PUSHD ( EAX, return ); break;
        case 1: PUSHD ( ECX, return ); break;
        case 2: PUSHD ( EDX, return ); break;
        case 3: PUSHD ( EBX, return ); break;
        case 4: PUSHD ( ESP, return ); break;
        case 5: PUSHD ( EBP, return ); break;
        case 6: PUSHD ( ESI, return ); break;
        case 7: PUSHD ( EDI, return ); break;
        }
    }
  else
    {
      switch ( opcode&0x7 )
        {
        case 0: PUSHW ( AX, return ); break;
        case 1: PUSHW ( CX, return ); break;
        case 2: PUSHW ( DX, return ); break;
        case 3: PUSHW ( BX, return ); break;
        case 4: PUSHW ( SP, return ); break;
        case 5: PUSHW ( BP, return ); break;
        case 6: PUSHW ( SI, return ); break;
        case 7: PUSHW ( DI, return ); break;
        }
    }
  
} // end push_r


static void
push_imm8 (
           PROTO_INTERP
           )
{
  
  uint8_t val;
  uint32_t val32;
  uint16_t val16;

  
  IB ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      val32= (uint32_t) ((int32_t) ((int8_t) val));
      PUSHD ( val32, return );
    }
  else
    {
      val16= (uint16_t) ((int16_t) ((int8_t) val));
      PUSHW ( val16, return );
    }
  
} // end push_imm8


static void
push_imm16 (
            PROTO_INTERP
            )
{
  
  uint16_t val;

  
  IW ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  PUSHW ( val, return );
  
} // end push_imm16


static void
push_imm32 (
            PROTO_INTERP
            )
{

  uint32_t val;

  
  ID ( &val, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  PUSHD ( val, return );
  
} // end push_imm32


static void
push_imm (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? push_imm32 ( INTERP ) : push_imm16 ( INTERP );
} // end push_imm


static void
push_sreg (
           PROTO_INTERP,
           IA32_SegmentRegister *sreg
           )
{
  
  uint32_t val32;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      // NOTA !!!!! En els processador més nous, la part alta no es
      // modifica en la pila.
      val32= (uint32_t) sreg->v;
      PUSHD ( val32, return );
    }
  else { PUSHW ( sreg->v, return ); }
  
} // end push_sreg


static void
push_CS (
         PROTO_INTERP
         )
{
  push_sreg ( INTERP, P_CS );
} // end push_CS


static void
push_SS (
         PROTO_INTERP
         )
{
  push_sreg ( INTERP, P_SS );
} // end push_SS


static void
push_DS (
         PROTO_INTERP
         )
{
  push_sreg ( INTERP, P_DS );
} // end push_DS


static void
push_ES (
         PROTO_INTERP
         )
{
  push_sreg ( INTERP, P_ES );
} // end push_ES


static void
push_FS (
         PROTO_INTERP
         )
{
  push_sreg ( INTERP, P_FS );
} // end push_FS


static void
push_GS (
         PROTO_INTERP
         )
{
  push_sreg ( INTERP, P_GS );
} // end push_GS




/*****************************************************/
/* PUSHA/PUSHAD - Push All General-Purpose Registers */
/*****************************************************/

static void
pusha (
       PROTO_INTERP
       )
{

  uint32_t tmp32;
  uint16_t tmp16;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !PROTECTED_MODE_ACTIVATED )
    {
      if ( (OPERAND_SIZE_IS_32 &&
            (ESP==7 || ESP==9 || ESP==11 || ESP==13 || ESP==15)) ||
           (!OPERAND_SIZE_IS_32 &&
            (SP==7 || SP==9 || SP==11 || SP==13 || SP==15)))
        {
          // In the real-address mode, if the ESP or SP register is 1,
          // 3, or 5 when PUSHA/PUSHAD executes: an #SS exception is
          // generated but not delivered (the stack error reported
          // prevents #SS delivery). Next, the processor generates a
          // #DF exception and enters a shutdown state as described in
          // the #DF discussion in Chapter 6 of the Intel® 64 and
          // IA-32 Architectures Software Developer’s Manual, Volume
          // 3A.
          fprintf ( stderr,
                    "[EE] pusha - Cal implementar excepció rara pila\n" );
          exit ( EXIT_FAILURE );
        }
    }
  
  if ( OPERAND_SIZE_IS_32 )
    {
      tmp32= ESP;
      PUSHD ( EAX, return );
      PUSHD ( ECX, return );
      PUSHD ( EDX, return );
      PUSHD ( EBX, return );
      PUSHD ( tmp32, return );
      PUSHD ( EBP, return );
      PUSHD ( ESI, return );
      PUSHD ( EDI, return );
    }
  else
    {
      tmp16= SP;
      PUSHW ( AX, return );
      PUSHW ( CX, return );
      PUSHW ( DX, return );
      PUSHW ( BX, return );
      PUSHW ( tmp16, return );
      PUSHW ( BP, return );
      PUSHW ( SI, return );
      PUSHW ( DI, return );
    }
  
} // end pusha




/******************************************************/
/* PUSHF/PUSHFD - Push EFLAGS Register onto the Stack */
/******************************************************/

static void
pushf (
       PROTO_INTERP
       )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( OPERAND_SIZE_IS_32 )
    {
      PUSHD ( EFLAGS&0x00FCFFFF, return );
    }
  else
    {
      PUSHW ( (uint16_t) (EFLAGS&0xFFFF), return );
    }
  
} // end pushf




/******************************/
/* SAHF - Store AH into Flags */
/******************************/

static void
sahf (
      PROTO_INTERP
      )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }

  if ( AH&0x80 ) EFLAGS|= SF_FLAG;
  else           EFLAGS&= ~SF_FLAG;
  if ( AH&0x40 ) EFLAGS|= ZF_FLAG;
  else           EFLAGS&= ~ZF_FLAG;
  if ( AH&0x10 ) EFLAGS|= AF_FLAG;
  else           EFLAGS&= ~AF_FLAG;
  if ( AH&0x04 ) EFLAGS|= PF_FLAG;
  else           EFLAGS&= ~PF_FLAG;
  if ( AH&0x01 ) EFLAGS|= CF_FLAG;
  else           EFLAGS&= ~CF_FLAG;
  
} // end sahf




/*************************************************/
/* SGDT - Store Global Descriptor Table Register */
/*************************************************/

static void
sgdt_op16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r16_t eaddr;


  // Obté dades
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  if ( 0 /*INTERP->cpu->model == 286 */ )
    {
      printf("[EE] PAUSE NOT IMPLEMENTED\n");
      exit ( EXIT_FAILURE );
    }
  WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, GDTR.lastb, return );
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+2, GDTR.addr&0x00FFFFFF, return );
  
} // end sgdt_op16


static void
sgdt_op32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r32_t eaddr;


  // Obté dades
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  if ( 0 /*INTERP->cpu->model == 286 */ )
    {
      printf("[EE] PAUSE NOT IMPLEMENTED\n");
      exit ( EXIT_FAILURE );
    }
  WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, GDTR.lastb, return );
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+2, GDTR.addr, return );
  
} // end sgdt_op32


static void
sgdt (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{
  OPERAND_SIZE_IS_32 ?
    sgdt_op32 ( INTERP, modrmbyte ) :
    sgdt_op16 ( INTERP, modrmbyte );
} // end sgdt




/****************************************************/
/* SIDT - Store Interrupt Descriptor Table Register */
/****************************************************/

static void
sidt_op16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r16_t eaddr;


  // Obté dades
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr )
    { EXCEPTION_ERROR_CODE ( EXCP_UD, 0 ); return; }
  if ( 0 /*INTERP->cpu->model == 286 */ )
    {
      printf("[EE] PAUSE NOT IMPLEMENTED\n");
      exit ( EXIT_FAILURE );
    }
  WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, IDTR.lastb, return );
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+2, IDTR.addr&0x00FFFFFF, return );
  
} // end sidt_op16


static void
sidt_op32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r32_t eaddr;


  // Obté dades
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr )
    { EXCEPTION_ERROR_CODE ( EXCP_UD, 0 ); return; }
  if ( 0 /*INTERP->cpu->model == 286 */ )
    {
      printf("[EE] PAUSE NOT IMPLEMENTED\n");
      exit ( EXIT_FAILURE );
    }
  WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, IDTR.lastb, return );
  WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off+2, IDTR.addr, return );
  
} // end sidt_op32


static void
sidt (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{
  OPERAND_SIZE_IS_32 ?
    sidt_op32 ( INTERP, modrmbyte ) :
    sidt_op16 ( INTERP, modrmbyte );
} // end sidt




/************************************************/
/* SLDT - Store Local Descriptor Table Register */
/************************************************/

static void
sldt (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{
  
  eaddr_r16_t eaddr;
  uint16_t *reg;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, LDTR.v, return );
    }
  else *(eaddr.v.reg)= LDTR.v;
  
} // end sldt




/************************************/
/* SMSW - Store Machine Status Word */
/************************************/

static void
smsw_op16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r16_t eaddr;
  uint16_t tmp;
  
  
  // Obté dades
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  tmp= (uint16_t) (CR0&0xFFFF);
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, tmp, return );
    }
  else *eaddr.v.reg= tmp;
  
} // end smsw_op16


static void
smsw_op32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{

  eaddr_r32_t eaddr;
  uint16_t tmp;
  
  
  // Obté dades
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  tmp= (uint16_t) (CR0&0xFFFF);
  // OJO!!! Dona igual que siga 32bits, en memòria es desen 16bits.
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, tmp, return );
    }
  else *eaddr.v.reg= (uint32_t) tmp;
  
} // end smsw_op32


static void
smsw (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{
  OPERAND_SIZE_IS_32 ?
    smsw_op32 ( INTERP, modrmbyte ) :
    smsw_op16 ( INTERP, modrmbyte );
} // end smsw




/************************/
/* STC - Set Carry Flag */
/************************/

static void
stc (
     PROTO_INTERP
     )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  EFLAGS|= CF_FLAG;
  
} // end stc




/****************************/
/* STD - Set Direction Flag */
/****************************/

static void
std (
     PROTO_INTERP
     )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  EFLAGS|= DF_FLAG;
  
} // end std




/****************************/
/* STI - Set Interrupt Flag */
/****************************/

static void
sti (
     PROTO_INTERP
     )
{
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !PROTECTED_MODE_ACTIVATED )
    {
      if ( (EFLAGS&IF_FLAG) == 0 )
        INTERP->_inhibit_interrupt= true;
      EFLAGS|= IF_FLAG;
    }
  else
    {
      if ( EFLAGS&VM_FLAG )
        {
          if ( IOPL == 3 )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                INTERP->_inhibit_interrupt= true;
              EFLAGS|= IF_FLAG;
            }
          else if ( IOPL < 3 && (EFLAGS&VIP_FLAG)==0 && (CR4&CR4_VME)!=0 )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                INTERP->_inhibit_interrupt= true;
              EFLAGS|= VIF_FLAG;
            }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
        }
      else
        {
          if ( IOPL >= CPL )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                INTERP->_inhibit_interrupt= true;
              EFLAGS|= IF_FLAG;
            }
          else if ( IOPL < CPL && CPL==3 && (CR4&CR4_PVI)!=0 )
            {
              if ( (EFLAGS&IF_FLAG) == 0 )
                INTERP->_inhibit_interrupt= true;
              EFLAGS|= VIF_FLAG;
            }
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return; }
        }
    }
  
} // end sti




/***********************************************/
/* STOS/STOSB/STOSW/STOSD/STOSQ - Store String */
/***********************************************/

// NOTA!! Aparentment Microsoft utilitza el prefixe F2 en compte del
// F3 en les operacions MOVS. Açò és un comportament no documentat en
// la documentació oficial de Intel. Aparentment el funcionament és el
// mateix que amb F3.

static void
stosb (
       PROTO_INTERP
       )
{
  
  // Copia.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      if ( ADDRESS_SIZE_IS_32 )
        {
          WRITEB ( P_ES, EDI, AL, return );
          if ( EFLAGS&DF_FLAG ) --EDI;
          else                  ++EDI;
        }
      else
        {
          WRITEB ( P_ES, (uint32_t) DI, AL, return );
          if ( EFLAGS&DF_FLAG ) --DI;
          else                  ++DI;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;

    }
  
} // end stosb


static void
stosw (
       PROTO_INTERP
       )
{
  
  // Copia.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      if ( ADDRESS_SIZE_IS_32 )
        {
          WRITEW ( P_ES, EDI, AX, return );
          if ( EFLAGS&DF_FLAG ) EDI-= 2;
          else                  EDI+= 2;
        }
      else
        {
          WRITEW ( P_ES, (uint32_t) DI, AX, return );
          if ( EFLAGS&DF_FLAG ) DI-= 2;
          else                  DI+= 2;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;

    }
  
} // end stosw


static void
stosd (
       PROTO_INTERP
       )
{
  
  // Copia.
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      if ( ADDRESS_SIZE_IS_32 )
        {
          WRITED ( P_ES, EDI, EAX, return );
          if ( EFLAGS&DF_FLAG ) EDI-= 4;
          else                  EDI+= 4;
        }
      else
        {
          WRITED ( P_ES, (uint32_t) DI, EAX, return );
          if ( EFLAGS&DF_FLAG ) DI-= 4;
          else                  DI+= 4;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;

    }
  
} // end stosd


static void
stos (
      PROTO_INTERP
      )
{
  OPERAND_SIZE_IS_32 ? stosd ( INTERP ) : stosw ( INTERP );
} // end stos




/*****************************/
/* STR - Store Task Register */
/*****************************/

static void
str (
     PROTO_INTERP,
     const uint8_t modrmbyte
     )
{
  
  eaddr_r16_t eaddr;
  uint16_t *reg;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) ) goto ud_exc;
  if ( eaddr.is_addr )
    {
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, TR.v, return );
    }
  else *(eaddr.v.reg)= TR.v;

  return;
  
 ud_exc:
  EXCEPTION ( EXCP_UD );
  return;
  
} // end str




/*******************************************************/
/* VERR/VERW - Verify a Segment for Reading or Writing */
/*******************************************************/

static void
verr_verw (
           PROTO_INTERP,
           const uint8_t modrmbyte,
           const bool    check_read
           )
{
  
  eaddr_r16_t eaddr;
  uint16_t *reg,selector;
  uint32_t off,dpl,cpl,rpl,stype;
  seg_desc_t desc;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( QLOCKED ) goto ud_exc;
  if ( !PROTECTED_MODE_ACTIVATED || (EFLAGS&VM_FLAG) ) goto ud_exc;
  
  // Obté selector
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &selector, return );
    }
  else selector= *(eaddr.v.reg);
  
  // Comprovació
  EFLAGS&= ~ZF_FLAG;
  if ( (selector&0xFFFC) == 0 ) return; // NULL selector
  // --> Comprova offset
  off= (uint32_t) (selector&0xFFF8);
  if ( selector&0x4 ) // LDT
    {
      if ( off < LDTR.h.lim.firstb || (off+7) > LDTR.h.lim.lastb )
        return;
      desc.addr= LDTR.h.lim.addr + off;
    }
  else // GDT
    {
      if ( off < GDTR.firstb || (off+7) > GDTR.lastb )
        return;
      desc.addr= GDTR.addr + off;
    }
  // --> Llig descriptor
  READLD ( desc.addr, &(desc.w1), true, true, return );
  READLD ( desc.addr+4, &(desc.w0), true, true, return );
  // --> Obté valors protecció
  dpl= SEG_DESC_GET_DPL ( desc );
  cpl= CPL;
  rpl= selector&0x3;
  stype= SEG_DESC_GET_STYPE ( desc );
  // --> System segment
  if ( (stype&0x10) == 0x00 ) return; // System segment
  // --> Check readable/writable
  if ( (check_read && ((stype&0x1A) == 0x18)) || // Not readable
       (!check_read && ((stype&0x1A) != 0x12)) ) // Not writable
    return;
  // --> Non conforming code segment
  if ( (stype&0x1C) == 0x18 && (cpl > dpl || rpl > dpl) )
    return;
  
  // Fixa valor.
  EFLAGS|= ZF_FLAG;
  
  return;
  
 ud_exc:
  EXCEPTION ( EXCP_UD );
  return;
  
} // end verr_verw


static void
verr (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{
  verr_verw ( INTERP, modrmbyte, true );
} // end verr


static void
verw (
      PROTO_INTERP,
      const uint8_t modrmbyte
      )
{
  verr_verw ( INTERP, modrmbyte, false );
} // end verw




/********************************************/
/* WBINVD - Write Back and Invalidate Cache */
/********************************************/

static void
wbinvd (
        PROTO_INTERP
        )
{

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( (PROTECTED_MODE_ACTIVATED && CPL != 0) || (EFLAGS&VM_FLAG) )
    { EXCEPTION0 ( EXCP_GP ); return; }

  printf("[CAL_IMPLEMENTAR] WBINVD\n");
  /*
   * Mirar PC/src/NOTES_MEMORIA_CAU.txt
   *
   *  WriteBack(InternalCaches);
   *  Flush(InternalCaches);
   *  SignalWriteBack(ExternalCaches);
   *  SignalFlush(ExternalCaches);
   *  Continue; (* Continue execution *)
   */
  
} // end wbinvd




/*************************************************/
/* XCHG - Exchange Register/Memory with Register */
/*************************************************/

static void
xchg_A_r (
          PROTO_INTERP,
          const uint8_t opcode
          )
{
  
  uint32_t *reg32,tmp32;
  uint16_t *reg16,tmp16;
  
  
  if ( OPERAND_SIZE_IS_32 )
    {
      switch ( opcode&0x7 )
        {
        case 0: reg32= &EAX; break;
        case 1: reg32= &ECX; break;
        case 2: reg32= &EDX; break;
        case 3: reg32= &EBX; break;
        case 4: reg32= &ESP; break;
        case 5: reg32= &EBP; break;
        case 6: reg32= &ESI; break;
        case 7: reg32= &EDI; break;
        }
      tmp32= *reg32;
      *reg32= EAX;
      EAX= tmp32;
    }
  else
    {
      switch ( opcode&0x7 )
        {
        case 0: reg16= &AX; break;
        case 1: reg16= &CX; break;
        case 2: reg16= &DX; break;
        case 3: reg16= &BX; break;
        case 4: reg16= &SP; break;
        case 5: reg16= &BP; break;
        case 6: reg16= &SI; break;
        case 7: reg16= &DI; break;
       }
      tmp16= *reg16;
      *reg16= AX;
      AX= tmp16;
    }
  
} // end xchg_A_r


static void
xchg_r8_rm8 (
             PROTO_INTERP
             )
{
  
  uint8_t modrmbyte,data,tmp;
  eaddr_r8_t eaddr;
  uint8_t *reg;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      // NOTA!!!! If a memory operand is referenced, the processor’s
      // locking protocol is automatically implemented
      READB_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &data, return );
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, *reg, return );
      *reg= data;
    }
  else
    {
      tmp= *reg;
      *reg= *(eaddr.v.reg);
      *(eaddr.v.reg)= tmp;
    }
  
} // end xchg_r8_rm8


static void
xchg_r16_rm16 (
               PROTO_INTERP
               )
{
  
  uint8_t modrmbyte;
  uint16_t data,tmp;
  eaddr_r16_t eaddr;
  uint16_t *reg;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      // NOTA!!!! If a memory operand is referenced, the processor’s
      // locking protocol is automatically implemented
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &data, return );
      WRITEW ( eaddr.v.addr.seg, eaddr.v.addr.off, *reg, return );
      *reg= data;
    }
  else
    {
      tmp= *reg;
      *reg= *(eaddr.v.reg);
      *(eaddr.v.reg)= tmp;
    }
  
} // end xchg_r16_rm16


static void
xchg_r32_rm32 (
               PROTO_INTERP
               )
{
  
  uint8_t modrmbyte;
  uint32_t data,tmp;
  eaddr_r32_t eaddr;
  uint32_t *reg;
  
  
  IB ( &modrmbyte, return );
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, &reg ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      // NOTA!!!! If a memory operand is referenced, the processor’s
      // locking protocol is automatically implemented
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &data, return );
      WRITED ( eaddr.v.addr.seg, eaddr.v.addr.off, *reg, return );
      *reg= data;
    }
  else
    {
      tmp= *reg;
      *reg= *(eaddr.v.reg);
      *(eaddr.v.reg)= tmp;
    }
  
} // end xchg_r32_rm32


static void
xchg_r_rm (
           PROTO_INTERP
           )
{
  OPERAND_SIZE_IS_32 ? xchg_r32_rm32 ( INTERP ) : xchg_r16_rm16 ( INTERP );
} // end xchg_r_rm




/******************************************/
/* XLAT/XLATB - Table Look-up Translation */
/******************************************/

static void
xlatb (
       PROTO_INTERP
       )
{

  addr_t addr;
  uint8_t val;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  addr.seg= P_DS;
  OVERRIDE_SEG ( addr.seg );
  addr.off= ADDRESS_SIZE_IS_32 ?
    (EBX + (uint32_t) AL) :
    ((uint32_t) (BX + (uint16_t) AL));
  READB_DATA ( addr.seg, addr.off, &val, return );
  AL= val;
  
} // end xlatb
