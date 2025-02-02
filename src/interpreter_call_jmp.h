/*
 * Copyright 2016-2025 Adrià Giménez Pastor.
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
 *  interpreter_call_jmp.h - Conjunt d'instruccions JMP/CALL.
 *
 */


static int
call_jmp_far_protected_code (
                             PROTO_INTERP,
                             const uint16_t  selector,
                             const uint32_t  offset,
                             const uint32_t  stype,
                             seg_desc_t     *desc,
                             const bool      is_call
                             )
{

  int nbytes;
  uint32_t dpl,cpl,off_stack;
  bool conforming;
  IA32_SegmentRegister tmp_seg;
  
  
  // Privilegis.
  conforming= (stype&0x04)!=0;
  dpl= SEG_DESC_GET_DPL ( *desc );
  cpl= CPL;
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
      return -1;
    }

  // Comprova espai pila.
  if ( is_call )
    {
      nbytes= OPERAND_SIZE_IS_32 ? 8 : 4;
      off_stack= P_SS->h.is32 ?
        (ESP-nbytes) : ((uint32_t) ((uint16_t) (SP-nbytes)));
      if ( off_stack < P_SS->h.lim.firstb ||
           off_stack > (P_SS->h.lim.lastb-(nbytes-1)) )
        { EXCEPTION_ERROR_CODE ( EXCP_SS, 0 ); return -1; }
    }
  
  // Comprova que el offset és vàlid en el segment nou.
  load_segment_descriptor ( &tmp_seg, (selector&0xFFFC) | cpl, desc );
  if ( offset < tmp_seg.h.lim.firstb || offset > tmp_seg.h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_GP );
      return -1;
    }

  // Apila adreça retorn
  if ( is_call )
    {
      if ( OPERAND_SIZE_IS_32 )
        {
          PUSHD ( (uint32_t) P_CS->v, return -1 );
          PUSHD ( EIP, return -1 );
        }
      else
        {
          PUSHW ( P_CS->v, return -1 );
          PUSHW ( (uint16_t) (EIP&0xFFFF), return -1 );
        }
    }
  
  // Carrega el descriptor.
  set_segment_descriptor_access_bit ( INTERP, desc );
  *P_CS= tmp_seg;
  EIP= offset;
  
  return 0;
  
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return -1;
  
} // end call_jmp_far_protected_code


static int
call_far_protected_gate_more_pl (
                                 PROTO_INTERP,
                                 const uint16_t  selector,
                                 const uint32_t  offset,
                                 seg_desc_t     *desc,
                                 const bool      is32,
                                 const uint8_t   param_count
                                 )
{

  int nbytes;
  uint32_t dpl,dpl_new_SS,rpl_new_SS,off,new_ESP,new_SS_stype,tmp_ESP;
  uint16_t new_SS_selector,new_SP,tmp_SS_selector;
  seg_desc_t new_SS_desc;
  IA32_SegmentRegister new_SS,tmp_seg;
  
  
  // Identify stack-segment selector for new privilege level in current TSS.
  dpl= SEG_DESC_GET_DPL(*desc);
  assert ( dpl < 3 );
  if ( TR.h.tss_is32 )
    {
      off= (dpl<<3) + 4;
      if ( (off+5) > TR.h.lim.lastb ) goto excp_ts_error;
      READLW ( TR.h.lim.addr+off+4, &new_SS_selector, true, true, return -1);
      READLD ( TR.h.lim.addr+off, &new_ESP, true, true, return -1 );
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
    { EXCEPTION_SEL ( EXCP_TS, new_SS_selector ); return -1; };
  if ( read_segment_descriptor ( INTERP, new_SS_selector,
                                 &new_SS_desc, true ) != 0 )
    return -1;
  rpl_new_SS= new_SS_selector&0x3;
  dpl_new_SS= SEG_DESC_GET_DPL(new_SS_desc);
  new_SS_stype= SEG_DESC_GET_STYPE ( new_SS_desc );
  if ( rpl_new_SS != dpl ||
       dpl_new_SS != dpl ||
       (new_SS_stype&0x1A)!=0x12 // No writable data
       )
    { EXCEPTION_SEL ( EXCP_TS, new_SS_selector ); return -1; }
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
    { EXCEPTION0 ( EXCP_GP ); return -1; }

  // Fixa pila, apila i nou valors
  set_segment_descriptor_access_bit ( INTERP, desc );
  set_segment_descriptor_access_bit ( INTERP, &new_SS_desc );
  tmp_SS_selector= P_SS->v;
  tmp_ESP= ESP;
  if ( is32 )
    {
      *P_SS= new_SS;
      ESP= new_ESP;
      PUSHD ( (uint32_t) tmp_SS_selector, return -1 );
      PUSHD ( tmp_ESP, return -1 );
      if ( param_count != 0 )
        {
          printf("[CAL IMPLEMENTAR] "
                 "call_far_protected_gate_more_pl - PARAM_COUNT:%u\n",
                 param_count);
          exit(EXIT_FAILURE);
        }
      PUSHD ( (uint32_t) P_CS->v, return -1 );
      PUSHD ( EIP, return -1 );
      *P_CS= tmp_seg;
      EIP= offset;
    }
  else
    {
      printf("[CAL IMPLEMENTAR] "
             "call_far_protected_gate_more_pl - GATE16\n");
      exit(EXIT_FAILURE);
    }
  
  return 0;

 excp_ts_error:
  EXCEPTION_SEL ( EXCP_TS, TR.v );
  return -1;
 excp_ss:
  EXCEPTION_SEL ( EXCP_SS, new_SS_selector );
  return -1;
  
} // end call_far_protected_gate_more_pl


static int
call_far_protected_gate (
                         PROTO_INTERP,
                         const uint16_t  selector_gate,
                         seg_desc_t     *desc_gate,
                         const bool      is32
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
    { EXCEPTION_SEL ( EXCP_NP, selector_gate ); return -1; }
  
  // Comprovacions selector gate.
  selector= CG_DESC_GET_SELECTOR ( *desc_gate );
  if ( (selector&0xFFFC) == 0 ) // Null segment selector..
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return -1; };
  if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
    return -1;
  stype= SEG_DESC_GET_STYPE ( desc );
  dpl= SEG_DESC_GET_DPL ( desc );
  if ( (stype&0x18)!=0x18 || // No és codi
       dpl > CPL )
    goto excp_gp_sel;
  if ( !SEG_DESC_IS_PRESENT(desc) )
    { EXCEPTION_SEL ( EXCP_NP, selector ); return -1; }
  
  // Comprova privilegi destí.
  conforming= (stype&0x04)!=0;
  param_count= (uint8_t) CG_DESC_GET_PARAMCOUNT ( *desc_gate );
  offset= CG_DESC_GET_OFFSET ( *desc_gate );
  if ( !conforming && dpl < CPL )
    {
      if ( call_far_protected_gate_more_pl ( INTERP, selector, offset,
                                             &desc, is32, param_count ) != 0 )
        return -1;
    }
  else
    {
      printf("[CAL IMPLEMENTAR] "
             "call_far_protected_gate - SAME PRIVILEGE\n");
      exit(EXIT_FAILURE);
    }
  
  return 0;
  
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return -1;
 excp_gp_gate:
  EXCEPTION_SEL ( EXCP_GP, selector_gate );
  return -1;
  
} // end call_far_protected_gate


static int
call_jmp_far_protected (
                        PROTO_INTERP,
                        const uint16_t selector,
                        const uint32_t offset,
                        const bool     is_call
                        )
{

  seg_desc_t desc;
  uint32_t stype;

  
  // Null Segment Selector checking.
  if ( (selector&0xFFFC) == 0 )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return -1; };

  // Llig.
  if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
    return -1;
  
  // Type Checking.
  // NOTA!!! Assumisc EFER.LMA==0, aquest bi sols té sentit en 64bits.
  stype= SEG_DESC_GET_STYPE ( desc );
  if ( (stype&0x18) == 0x18 ) // Segments no especials de codi.
    {
      if ( call_jmp_far_protected_code ( INTERP, selector, offset,
                                         stype, &desc, is_call ) != 0 )
        return -1;
    }
  else if ( (stype&0x17) == 0x04 ) // Call gates (16 i 32 bits).
    {
      if ( is_call )
        {
          if ( call_far_protected_gate ( INTERP, selector,
                                         &desc, stype==0x0c) != 0 )
            return -1;
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
      /*
      if ( change_CS_EIP_protected_task_gate ( INTERP, selector,
                                               op, stype, &desc ) != 0 )
        return -1;
      */
    }
  else if ( (stype&0x15) == 0x01 ) // TSS (16 i 32 bits).
    {
      printf ( "[EE] jmp_far_protected not implemented for TSS" );
      exit ( EXIT_FAILURE );
      /*
      if ( change_CS_EIP_protected_tss ( INTERP, selector,
                                         op, stype, &desc ) != 0 )
        return -1;
      */
    }
  else goto excp_sel_gp;

  return 0;

 excp_sel_gp:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return -1;
  
} // end call_jmp_far_protected




/*************************/
/* CALL - Call Procedure */
/*************************/

#define CALL_NEAR16(OFF)        					\
  if ( (OFF) < P_CS->h.lim.firstb || (OFF) > P_CS->h.lim.lastb )        \
    {                                                                   \
      EXCEPTION0 ( EXCP_GP );                                            \
      return;                                                           \
    }                                                                   \
  PUSHW ( (uint16_t) (EIP&0xFFFF), return );        			\
  EIP= (OFF)

#define CALL_NEAR32(OFF)        					\
  if ( (OFF) < P_CS->h.lim.firstb || (OFF) > P_CS->h.lim.lastb )        \
    {                                                                   \
      EXCEPTION0 ( EXCP_GP );                                            \
      return;                                                           \
    }                                                                   \
  PUSHD ( EIP, return );        					\
  EIP= (OFF)


static int
call_far_nonprotected (
                       PROTO_INTERP,
                       const uint16_t selector,
                       const uint32_t offset
                      )
{

  uint32_t tmp;

  
  // NOTA!!! quan operand_size és 16, els bits superiors de offset
  // estan a 0!!
  if ( OPERAND_SIZE_IS_32 )
    {
      // Comprove abans de fer cap push (els push ho tornen a
      // comprovar)
      tmp= P_SS->h.is32 ? (ESP-6) : ((uint32_t) ((uint16_t) (SP-6)));
      if ( tmp > 0xFFFA ) { EXCEPTION ( EXCP_SS ); return -1; }
      if ( offset > 0xFFFF ) { EXCEPTION ( EXCP_GP ); return -1; }
      PUSHW ( P_CS->v, return -1 );
      PUSHD ( EIP, return -1 );
    }
  else
    {
      // Comprove abans de fer cap push (els push ho tornen a
      // comprovar)
      tmp= P_SS->h.is32 ? (ESP-4) : ((uint32_t) ((uint16_t) (SP-4)));
      if ( tmp > 0xFFFC ) { EXCEPTION ( EXCP_SS ); return -1; }
      PUSHW ( P_CS->v, return -1 );
      PUSHW ( ((uint16_t) (EIP&0xFFFF)), return -1 );
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
  
  return 0;
  
} // end call_far_nonprotected


#define CALL_FAR(SEL,OFF)        					\
  if ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG))        		\
    {        								\
      if ( call_jmp_far_protected ( INTERP, (SEL), (OFF), true ) != 0 ) \
        return;                                                         \
    }        								\
  else        								\
    {        								\
      if ( call_far_nonprotected ( INTERP, (SEL), (OFF) ) != 0 )        \
        return;                                                         \
    }

static void
call_rel16 (
            PROTO_INTERP
            )
{

  uint16_t offset;
  uint32_t tmp;
  
  
  IW ( &offset, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  tmp= (EIP + (uint32_t) ((int32_t) ((int16_t) offset)))&0xFFFF;
  CALL_NEAR16 ( tmp );
  
} // end call_rel16


static void
call_rel32 (
            PROTO_INTERP
            )
{

  uint32_t offset, tmp;
  

  ID ( &offset, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  tmp= EIP + offset;
  CALL_NEAR32 ( tmp );
  
} // end call_rel32


static void
call_rel (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? call_rel32 ( INTERP ) : call_rel16 ( INTERP );
} // end call_rel


static void
call_rm16 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint16_t dst;
  uint32_t tmp;
  eaddr_r16_t eaddr;
  
  
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &dst, return );
    }
  else dst= *(eaddr.v.reg);
  tmp= (uint32_t) dst;
  CALL_NEAR16 ( tmp );
  
} // end call_rm16


static void
call_rm32 (
           PROTO_INTERP,
           const uint8_t modrmbyte
           )
{
  
  uint32_t tmp;
  eaddr_r32_t eaddr;
  
  
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &tmp, return );
    }
  else tmp= *(eaddr.v.reg);
  CALL_NEAR32 ( tmp );
  
} // end call_rm32


static void
call_rm (
         PROTO_INTERP,
         const uint8_t modrmbyte
         )
{
  OPERAND_SIZE_IS_32 ?
    call_rm32 ( INTERP, modrmbyte ) :
    call_rm16 ( INTERP, modrmbyte );
} // end call_rm


static void
call_ptr16_16 (
               PROTO_INTERP
               )
{

  uint16_t selector,tmp;
  uint32_t offset;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IW ( &tmp, return );
  IW ( &selector, return );
  offset= (uint32_t) tmp;
  CALL_FAR ( selector, offset );
  
} // end call_ptr16_16


static void
call_ptr16_32 (
               PROTO_INTERP
               )
{

  uint16_t selector;
  uint32_t offset;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ID ( &offset, return );
  IW ( &selector, return );
  CALL_FAR ( selector, offset );
  
} // end call_ptr16_32


static void
call_ptr16_16_32 (
        	PROTO_INTERP
        	)
{
  OPERAND_SIZE_IS_32 ? call_ptr16_32 ( INTERP ) : call_ptr16_16 ( INTERP );
} // end call_ptr16_16_32


static void
call_m16_16 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint16_t selector,tmp;
  uint32_t offset;
  eaddr_r16_t eaddr;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &tmp, return );
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &selector, return );
    }
  else { tmp= selector= 0; EXCEPTION ( EXCP_UD ); return; }
  offset= (uint32_t) tmp;
  CALL_FAR ( selector, offset );
  
} // end call_m16_16


static void
call_m16_32 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  
  uint16_t selector;
  uint32_t offset;
  eaddr_r32_t eaddr;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &offset, return );
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &selector, return );
    }
  else { offset= selector= 0; EXCEPTION ( EXCP_UD ); return; }
  CALL_FAR ( selector, offset );
  
} // end call_m16_32


static void
call_m16_off (
              PROTO_INTERP,
              const uint8_t modrmbyte
              )
{
  OPERAND_SIZE_IS_32 ?
    call_m16_32 ( INTERP, modrmbyte ) :
    call_m16_16 ( INTERP, modrmbyte );
} // end call_m16_off




/**************/
/* JMP - Jump */
/**************/

static int
jmp_far_nonprotected (
                      PROTO_INTERP,
                      const uint16_t selector,
                      const uint32_t offset
                      )
{
  
  // NOTA!!! quan operand_size és 16, els bits superiors de offset
  // estan a 0!!
  if ( OPERAND_SIZE_IS_32 && offset > 0xFFFF )
    {
      WW ( UDATA, "jmp_far_nonprotected -> no és cumpleix la condició"
           " (offset > 0xFFFF). Aquesta condició és una interpretació"
           " de (IF tempEIP is beyond code segment limit THEN #GP(0); FI;)."
           " Cal revisar" );
      EXCEPTION ( EXCP_GP );
      return -1;
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
  
  return 0;
  
} // end jmp_far_nonprotected


#define JMP_FAR(SEL,OFF)                                                \
  if ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG))                   \
    {                                                                   \
      if ( call_jmp_far_protected ( INTERP, (SEL), (OFF), false ) != 0 ) \
        return;                                                         \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      if ( jmp_far_nonprotected ( INTERP, (SEL), (OFF) ) != 0 )         \
        return;                                                         \
    }

#define JMP_NEAR(OFF)                                                   \
  if ( (OFF) < P_CS->h.lim.firstb || (OFF) > P_CS->h.lim.lastb )        \
    {                                                                   \
      EXCEPTION0 ( EXCP_GP );                                            \
      return;                                                           \
    }                                                                   \
  EIP= (OFF)


// En realitat és l'anomenat short jmp.
static void
jmp_rel8 (
          PROTO_INTERP
          )
{

  uint8_t offset;
  uint32_t tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IB ( &offset, return );

  // ATENCIÓ!! Aquest pot estar en mode OPERAND_SIZE32 o OPERAND_SIZE16.
  tmp= EIP + (uint32_t) ((int32_t) ((int8_t) offset));
  if ( !OPERAND_SIZE_IS_32 ) tmp&= 0xFFFF;
  JMP_NEAR ( tmp );
  
} // end jpm_rel8


static void
jmp_rel16 (
           PROTO_INTERP
           )
{

  uint16_t offset;
  uint32_t tmp;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IW ( &offset, return );
  tmp= (EIP + (uint32_t) ((int32_t) ((int16_t) offset)))&0xFFFF;
  JMP_NEAR ( tmp );
  
} // end jpm_rel16


static void
jmp_rel32 (
           PROTO_INTERP
           )
{

  uint32_t offset, tmp;
  

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ID ( &offset, return );
  tmp= EIP + offset;
  JMP_NEAR ( tmp );
  
} // end jmp_rel32


static void
jmp_rel (
         PROTO_INTERP
         )
{
  OPERAND_SIZE_IS_32 ? jmp_rel32 ( INTERP ) : jmp_rel16 ( INTERP );
} // end jmp_rel


static void
jmp_rm16 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  uint16_t offset;
  eaddr_r16_t eaddr;


  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &offset, return );
    }
  else offset= *eaddr.v.reg;
  JMP_NEAR ( ((uint32_t) offset) );
  
} // end jmp_rm16


static void
jmp_rm32 (
          PROTO_INTERP,
          const uint8_t modrmbyte
          )
{

  uint32_t offset;
  eaddr_r32_t eaddr;


  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &offset, return );
    }
  else offset= *eaddr.v.reg;
  JMP_NEAR ( offset );
  
} // end jmp_rm32


static void
jmp_rm (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    jmp_rm32 ( INTERP, modrmbyte ) :
    jmp_rm16 ( INTERP, modrmbyte );
} // end jmp_rm


static void
jmp_ptr16_16 (
              PROTO_INTERP
              )
{

  uint16_t selector,tmp;
  uint32_t offset;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IW ( &tmp, return );
  IW ( &selector, return );
  offset= (uint32_t) tmp;
  JMP_FAR ( selector, offset );
  
} // end jmp_ptr16_16


static void
jmp_ptr16_32 (
              PROTO_INTERP
              )
{

  uint16_t selector;
  uint32_t offset;

  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ID ( &offset, return );
  IW ( &selector, return );
  JMP_FAR ( selector, offset );
  
} // end jmp_ptr16_32


static void
jmp_ptr16_16_32 (
                 PROTO_INTERP
                 )
{
  OPERAND_SIZE_IS_32 ? jmp_ptr16_32 ( INTERP ) : jmp_ptr16_16 ( INTERP );
} // end jmp_ptr16_16_32


static void
jmp_m16_16 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint16_t selector,tmp;
  uint32_t offset;
  eaddr_r16_t eaddr;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &tmp, return );
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+2, &selector, return );
    }
  else { tmp= selector= 0; EXCEPTION ( EXCP_UD ); return; }
  offset= (uint32_t) tmp;
  JMP_FAR ( selector, offset );
  
} // end jmp_m16_16


static void
jmp_m16_32 (
            PROTO_INTERP,
            const uint8_t modrmbyte
            )
{
  
  uint16_t selector;
  uint32_t offset;
  eaddr_r32_t eaddr;
  
  
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( eaddr.is_addr )
    {
      READD_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off, &offset, return );
      READW_DATA ( eaddr.v.addr.seg, eaddr.v.addr.off+4, &selector, return );
    }
  else { offset= selector= 0; EXCEPTION ( EXCP_UD ); return; }
  JMP_FAR ( selector, offset );
  
} // end jmp_m16_32


static void
jmp_m16_off (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{
  OPERAND_SIZE_IS_32 ?
    jmp_m16_32 ( INTERP, modrmbyte ) :
    jmp_m16_16 ( INTERP, modrmbyte );
} // end jmp_m16_off




/**********************************/
/* Jcc - Jump if Condition Is Met */
/**********************************/

static void
jcc_rel8 (
          PROTO_INTERP,
          const bool cond
          )
{

  uint8_t offset;
  uint32_t tmp;
  
  
  IB ( &offset, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( cond )
    {
      tmp= EIP + (uint32_t) ((int32_t) ((int8_t) offset));
      if ( !OPERAND_SIZE_IS_32 ) tmp&= 0xFFFF;
      JMP_NEAR ( tmp );
    }
  
} // end jcc_rel8


static void
jcc_rel16 (
           PROTO_INTERP,
           const bool cond
           )
{

  uint16_t offset;
  uint32_t tmp;
  

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  IW ( &offset, return );
  if ( cond )
    {
      tmp= (EIP + (uint32_t) ((int32_t) ((int16_t) offset)))&0xFFFF;
      JMP_NEAR ( tmp );
    }
  
} // end jcc_rel16


static void
jcc_rel32 (
           PROTO_INTERP,
           const bool cond
           )
{

  uint32_t offset, tmp;
  

  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  ID ( &offset, return );
  if ( cond )
    {
      tmp= EIP + offset;
      JMP_NEAR ( tmp );
    }
  
} // end jcc_rel32


static void
ja_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&(ZF_FLAG|CF_FLAG))==0);
  jcc_rel8 ( INTERP, cond );
} // end ja_rel8


static void
ja_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&(ZF_FLAG|CF_FLAG))==0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end ja_rel


static void
jae_rel8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&(CF_FLAG))==0);
  jcc_rel8 ( INTERP, cond );
} // end jae_rel8


static void
jae_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&(CF_FLAG))==0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jae_rel


static void
jb_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&CF_FLAG)!=0);
  jcc_rel8 ( INTERP, cond );
} // end jb_rel8


static void
jb_rel (
        PROTO_INTERP
        )
{
  bool cond;
  cond= ((EFLAGS&CF_FLAG)!=0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jb_rel


static void
jcxz_rel8 (
           PROTO_INTERP
           )
{
  bool cond;
  cond= ADDRESS_SIZE_IS_32 ? (ECX==0) : (CX==0);
  jcc_rel8 ( INTERP, cond );
} // end jcxz_rel8


static void
jg_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond=
    ((EFLAGS&ZF_FLAG)==0) &&
    !(((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  jcc_rel8 ( INTERP, cond );
} // end jg_rel8


static void
jg_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond=
    ((EFLAGS&ZF_FLAG)==0) &&
    !(((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jg_rel


static void
je_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&ZF_FLAG)!=0);
  jcc_rel8 ( INTERP, cond );
} // end je_rel8


static void
je_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&ZF_FLAG)!=0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end je_rel


static void
jge_rel8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= !(((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  jcc_rel8 ( INTERP, cond );
} // end jge_rel8


static void
jge_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond= !(((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jge_rel


static void
jl_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= (((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  jcc_rel8 ( INTERP, cond );
} // end jl_rel8


static void
jl_rel (
        PROTO_INTERP
        )
{
  bool cond;
  cond= (((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jl_rel


static void
jna_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&(ZF_FLAG|CF_FLAG))!=0);
  jcc_rel8 ( INTERP, cond );
} // end jna_rel8


static void
jna_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&(ZF_FLAG|CF_FLAG))!=0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jna_rel


static void
jne_rel8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&ZF_FLAG)==0);
  jcc_rel8 ( INTERP, cond );
} // end jne_rel8


static void
jne_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&ZF_FLAG)==0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jne_rel


static void
jng_rel8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond=
    ((EFLAGS&ZF_FLAG)!=0) ||
    (((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  jcc_rel8 ( INTERP, cond );
} // end jng_rel8


static void
jng_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond=
    ((EFLAGS&ZF_FLAG)!=0) ||
    (((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jng_rel


static void
jno_rel8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&OF_FLAG)==0);
  jcc_rel8 ( INTERP, cond );
} // end jno_rel8


static void
jns_rel8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&SF_FLAG)==0);
  jcc_rel8 ( INTERP, cond );
} // end jns_rel8


static void
jns_rel (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&SF_FLAG)==0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jns_rel


static void
jo_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&OF_FLAG)!=0);
  jcc_rel8 ( INTERP, cond );
} // end jo_rel8


static void
jo_rel (
        PROTO_INTERP
        )
{
  bool cond;
  cond= ((EFLAGS&OF_FLAG)!=0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end jo_rel


static void
jp_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&PF_FLAG)!=0);
  jcc_rel8 ( INTERP, cond );
} // end jp_rel8


static void
jpo_rel8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&PF_FLAG)==0);
  jcc_rel8 ( INTERP, cond );
} // end jpo_rel8


static void
js_rel8 (
         PROTO_INTERP
         )
{
  bool cond;
  cond= ((EFLAGS&SF_FLAG)!=0);
  jcc_rel8 ( INTERP, cond );
} // end js_rel8


static void
js_rel (
        PROTO_INTERP
        )
{
  bool cond;
  cond= ((EFLAGS&SF_FLAG)!=0);
  OPERAND_SIZE_IS_32 ? jcc_rel32 ( INTERP, cond ) : jcc_rel16 ( INTERP, cond );
} // end js_rel




/***********************************************/
/* LOOP/LOOPcc - Loop According to ECX Counter */
/***********************************************/

static void
loopcc_rel8 (
             PROTO_INTERP,
             const bool cond
             )
{

  uint8_t offset;
  uint32_t tmp;
  bool jump;
  
  
  IB ( &offset, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( ADDRESS_SIZE_IS_32 ) { --ECX; jump= (ECX!=0); }
  else                      { --CX; jump= (CX!=0); }
  if ( jump && cond )
    {
      tmp= EIP + (uint32_t) ((int32_t) ((int8_t) offset));
      if ( !OPERAND_SIZE_IS_32 ) tmp&= 0xFFFF;
      JMP_NEAR ( tmp );
    }
  
} // end loopcc_rel8


static void
loop_rel8 (
           PROTO_INTERP
           )
{
  loopcc_rel8 ( INTERP, true );
} // end loop_rel8


static void
loope_rel8 (
            PROTO_INTERP
            )
{
  loopcc_rel8 ( INTERP, (EFLAGS&ZF_FLAG)!=0 );
} // end loope_rel8


static void
loopne_rel8 (
             PROTO_INTERP
             )
{
  loopcc_rel8 ( INTERP, (EFLAGS&ZF_FLAG)==0 );
} // end loopne_rel8




/*******************************/
/* RET - Return from Procedure */
/*******************************/

static int
ret_near16 (
            PROTO_INTERP
            )
{

  uint16_t tmp;
  uint32_t tmpEIP;


  POPW ( &tmp, return -1 );
  tmpEIP= (uint32_t) tmp;
  if ( tmpEIP < P_CS->h.lim.firstb || tmpEIP > P_CS->h.lim.lastb )
    {
      EXCEPTION0 ( EXCP_GP );
      return -1;
    }
  EIP= tmpEIP;

  return 0;
  
} // end ret_near16


static int
ret_near32 (
            PROTO_INTERP
            )
{

  uint32_t tmp;


  POPD ( &tmp, return -1 );
  EIP= tmp;

  return 0;
  
} // end ret_near32


static void
ret_near (
          PROTO_INTERP
          )
{
  OPERAND_SIZE_IS_32 ? ret_near32 ( INTERP ) : ret_near16 ( INTERP );
} // end ret_near


static int
ret_far_protected_same_pl (
                           PROTO_INTERP,
                           const uint16_t selector,
                           seg_desc_t     desc
                           )
{

  uint32_t offset,tmp_eip;
  uint16_t tmp16;
  IA32_SegmentRegister sreg;
  
  
  // Carrega segment, llig EIP sense fer POP i comprova que està en
  // els límits.
  offset= P_SS->h.is32 ? ESP : ((uint32_t) ((uint16_t) SP));
  if ( OPERAND_SIZE_IS_32 )
    {
      READLD ( P_SS->h.lim.addr + offset, &tmp_eip, true, false, return -1 );
    }
  else
    {
      READLW ( P_SS->h.lim.addr + offset, &tmp16, true, false, return -1 );
      tmp_eip= (uint32_t) tmp16;
    }
  load_segment_descriptor ( &sreg, selector, &desc );
  if ( tmp_eip < sreg.h.lim.firstb || tmp_eip > sreg.h.lim.lastb )
    goto excp_gp;

  // Actualitza pila i valors.
  if ( OPERAND_SIZE_IS_32 )
    {
      if ( P_SS->h.is32 ) ESP+= 8;
      else                SP+= 8;
    }
  else
    {
      if ( P_SS->h.is32 ) ESP+= 4;
      else                SP+= 4;
    }
  EIP= tmp_eip;
  set_segment_descriptor_access_bit ( INTERP, &desc );
  *(P_CS)= sreg;
  
  return 0;

 excp_gp:
  EXCEPTION_ERROR_CODE ( EXCP_GP, 0 );
  return -1;
      
} // end ret_far_protected_same_pl


static int
ret_far_protected_outer_pl (
                            PROTO_INTERP,
                            const uint16_t selector,
                            seg_desc_t     desc,
                            const uint16_t imm
                            )
{

  uint32_t last_byte,offset,tmp,stype,rpl,rpl_SS,dpl_SS,tmp_eip,tmp_esp;
  uint16_t selector_SS,tmp16;
  seg_desc_t desc_SS;
  IA32_SegmentRegister tmp_CS;
  
  
  // Comprovacions adicionals pila i llig altres valors.
  offset= P_SS->h.is32 ? ESP : ((uint32_t) ((uint16_t) SP));
  if ( OPERAND_SIZE_IS_32 )
    {
      last_byte=
        P_SS->h.is32 ? (ESP+16+imm-1) : ((uint32_t) ((uint16_t) (SP+16+imm-1)));
      if ( last_byte < P_SS->h.lim.firstb || last_byte > P_SS->h.lim.lastb )
        goto excp_ss;
      READLD ( P_SS->h.lim.addr + offset, &tmp_eip, true, false, return -1 );
      READLD ( P_SS->h.lim.addr + offset + imm + 8, &tmp_esp,
               true, false, return -1 );
      READLD ( P_SS->h.lim.addr + offset + imm + 12, &tmp,
               true, false, return -1 );
      selector_SS= (uint16_t) (tmp&0xFFFF);
    }
  else
    {
      last_byte=
        P_SS->h.is32 ? (ESP+8+imm-1) : ((uint32_t) ((uint16_t) (SP+8+imm-1)));
      if ( last_byte < P_SS->h.lim.firstb || last_byte > P_SS->h.lim.lastb )
        goto excp_ss;
      READLW ( P_SS->h.lim.addr + offset, &tmp16, true, false, return -1 );
      tmp_eip= (uint32_t) tmp16;
      READLW ( P_SS->h.lim.addr + offset + imm + 4, &tmp16,
               true, false, return -1 );
      tmp_esp= (uint32_t) tmp16;
      READLW ( P_SS->h.lim.addr + offset + imm + 6, &selector_SS,
               true, false, return -1 );
    }
  
  // Comprovacions del descriptor de la pila.
  // --> NULL segment
  if ( (selector_SS&0xFFFC) == 0 ) goto excp_gp;
  // --> Límits
  if ( read_segment_descriptor ( INTERP, selector_SS, &desc_SS, true ) != 0 )
    return -1;
  // --> Privilegis
  stype= SEG_DESC_GET_STYPE ( desc_SS );
  rpl= selector&0x3;
  rpl_SS= selector_SS&0x3;
  dpl_SS= SEG_DESC_GET_DPL ( desc_SS );
  if ( rpl != rpl_SS ||
       rpl != dpl_SS ||
       (stype&0x1A)!=0x12 // no és data writable
       )
    goto excp_gp_sel;
  if ( !SEG_DESC_IS_PRESENT ( desc_SS ) ) goto excp_ss_sel;
  
  // Comprova EIP.
  load_segment_descriptor ( &tmp_CS, selector, &desc );
  if ( tmp_eip < tmp_CS.h.lim.firstb || tmp_eip > tmp_CS.h.lim.lastb )
    goto excp_gp;
  
  // Actualitza pila (¿¿¿???) i valors.
  if ( OPERAND_SIZE_IS_32 )
    {
      if ( P_SS->h.is32 ) ESP+= 16 + imm;
      else                SP+= 16 + imm;
    }
  else
    {
      if ( P_SS->h.is32 ) ESP+= 8 + imm;
      else                SP+= 8 + imm;
    }
  EIP= tmp_eip;
  set_segment_descriptor_access_bit ( INTERP, &desc );
  *(P_CS)= tmp_CS;
  ESP= tmp_esp;
  set_segment_descriptor_access_bit ( INTERP, &desc_SS );
  load_segment_descriptor ( P_SS, selector_SS, &desc_SS );
  
  // Comprova DPL segments.
  // --> ES
  if ( P_ES->h.dpl < tmp_CS.h.pl && P_ES->h.data_or_nonconforming )
    { P_ES->h.isnull= true; P_ES->v&= 0x0003; }
  // --> FS
  if ( P_FS->h.dpl < tmp_CS.h.pl && P_FS->h.data_or_nonconforming )
    { P_FS->h.isnull= true; P_FS->v&= 0x0003; }
  // --> GS
  if ( P_GS->h.dpl < tmp_CS.h.pl && P_GS->h.data_or_nonconforming )
    { P_GS->h.isnull= true; P_GS->v&= 0x0003; }
  // --> DS
  if ( P_DS->h.dpl < tmp_CS.h.pl && P_DS->h.data_or_nonconforming )
    { P_DS->h.isnull= true; P_DS->v&= 0x0003; }
  
  return 0;
  
 excp_ss:
  EXCEPTION_ERROR_CODE ( EXCP_SS, 0 );
  return -1;
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return -1;
 excp_gp:
  EXCEPTION_ERROR_CODE ( EXCP_GP, 0 );
  return -1;
 excp_ss_sel:
  EXCEPTION_SEL ( EXCP_SS, selector_SS );
  return -1;
  
} // end ret_far_protected_outer_pl


static int
ret_far_protected (
                   PROTO_INTERP,
                   const uint16_t imm
                   )
{
  
  uint32_t last_byte,tmp,offset,stype,dpl,rpl;
  uint16_t selector;
  seg_desc_t desc;
  
  
  // Comprovacions pila i llig selector sense fer POP.
  offset= P_SS->h.is32 ? ESP : ((uint32_t) ((uint16_t) SP));
  if ( OPERAND_SIZE_IS_32 )
    {
      last_byte= P_SS->h.is32 ? (ESP+8-1) : ((uint32_t) ((uint16_t) (SP+8-1)));
      if ( last_byte < P_SS->h.lim.firstb || last_byte > P_SS->h.lim.lastb )
        goto excp_ss;
      READLD ( P_SS->h.lim.addr + offset + 4, &tmp, true, false, return -1 );
      selector= (uint16_t) (tmp&0xFFFF);
    }
  else
    {
      last_byte= P_SS->h.is32 ? (ESP+4-1) : ((uint32_t) ((uint16_t) (SP+4-1)));
      if ( last_byte < P_SS->h.lim.firstb || last_byte > P_SS->h.lim.lastb )
        goto excp_ss;
      READLW ( P_SS->h.lim.addr + offset + 2, &selector,
               true, false, return -1 );
    }
  
  // Comprovacions del descriptor.
  // --> NULL segment
  if ( (selector&0xFFFC) == 0 )
    { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); return -1;}
  // --> Límits
  if ( read_segment_descriptor ( INTERP, selector, &desc, true ) != 0 )
    return -1;
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
    { EXCEPTION_SEL ( EXCP_NP, selector ); return -1; }
  
  // Torna en funció privilegi
  if ( rpl > CPL )
    {
      if ( ret_far_protected_outer_pl ( INTERP, selector, desc, imm ) != 0 )
        return -1;
    }
  else
    {
      if ( ret_far_protected_same_pl ( INTERP, selector, desc ) != 0 )
        return -1;
    }
  
  return 0;

 excp_ss:
  EXCEPTION_ERROR_CODE ( EXCP_SS, 0 );
  return -1;
 excp_gp_sel:
  EXCEPTION_SEL ( EXCP_GP, selector );
  return -1;
  
} // end ret_far_protected


static int
ret_far_nonprotected (
                      PROTO_INTERP
                      )
{

  uint32_t tmp;
  uint16_t selector,tmp16;
  
  
  // NOTA!! FAig comprovacions abans de fer el POP. Encara que són
  // redundants.
  tmp= P_SS->h.is32 ? ESP : ((uint32_t) ((uint16_t) SP));
  if ( OPERAND_SIZE_IS_32 )
    {
      if ( tmp > 0xFFF8 ) { EXCEPTION ( EXCP_SS ); return -1; }
      POPD ( &EIP, return -1 );
      POPD ( &tmp, return -1 );
      selector= (uint16_t) tmp;
    }
  else
    {
      if ( tmp > 0xFFFC ) { EXCEPTION ( EXCP_SS ); return -1; }
      // NOTA!!! Ací la documentació és un poc contradictòria,
      // assumisc que es desa 1 paraula.
      POPW ( &tmp16, return -1 );
      POPW ( &selector, return -1 );
      // NOTA!! Fa una serie de comprovacions que no tenen molt de
      // sentit. Si carrega 1 paraula no té sentit fer comprovacions.
      EIP= (uint32_t) tmp16;
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

  return 0;
  
} // end ret_far_nonprotected


static void
ret_far (
         PROTO_INTERP
         )
{
  
  if ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG))
    ret_far_protected ( INTERP, 0 );
  else
    ret_far_nonprotected ( INTERP );
  
} // end ret_far


static void
ret_near_imm16 (
                PROTO_INTERP
                )
{

  int ret;
  uint16_t val;
  

  IW ( &val, return );
  ret= OPERAND_SIZE_IS_32 ? ret_near32 ( INTERP ) : ret_near16 ( INTERP );
  if ( ret == -1 ) return;
  if ( ADDRESS_SIZE_IS_32 ) ESP+= (uint32_t) val;
  else                      SP+= val;
  
} // end ret_near_imm16


static void
ret_far_imm16 (
               PROTO_INTERP
               )
{

  int ret;
  uint16_t val;
  

  IW ( &val, return );
  if ( PROTECTED_MODE_ACTIVATED && !(EFLAGS&VM_FLAG))
    ret= ret_far_protected ( INTERP, val );
  else
    ret= ret_far_nonprotected ( INTERP );
  if ( ret == -1 ) return;
  if ( ADDRESS_SIZE_IS_32 ) ESP+= (uint32_t) val;
  else                      SP+= val;
  
} // end ret_far_imm16
