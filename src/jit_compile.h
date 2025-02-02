/*
 * Copyright 2021-2025 Adrià Giménez Pastor.
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
 *  jit_compile.h - Conté la part de 'jit.c' dedicada a compilar a
 *                  bytecode.
 *
 */

// DEFINCIONS //////////////////////////////////////////////////////////////////
static bool
compile_get_rm16_op (
                     const IA32_JIT_DisEntry *e,
                     IA32_JIT_Page           *p,
                     const int                op,
                     const bool               use_res
                     );

static bool
compile_get_rm8_op (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const int                op,
                    const bool               use_res
                    );

// FUNCIONS ////////////////////////////////////////////////////////////////////
static void
add_word (
          IA32_JIT_Page  *p,
          const uint16_t  word
          )
{

  uint32_t tmp;

  
  if ( p->N == p->capacity )
    {
      tmp= p->capacity*2;
      if ( tmp < p->capacity )
        {
          fprintf ( stderr, "cannot allocate memory\n" );
          exit ( EXIT_FAILURE );
        }
      p->v= (uint16_t *) realloc__ ( p->v, tmp*sizeof(uint16_t) );
      p->capacity= tmp;
    }
  p->v[p->N++]= word;
  
} // end add_word


// Torna cert si ho ha processat
static void
update_eip (
            const IA32_JIT_DisEntry *e,
            IA32_JIT_Page           *p,
            const bool               stop
            )
{

  switch ( e->inst.real_nbytes )
    {
    case 1: add_word ( p, BC_INC1_EIP ); break;
    case 2: add_word ( p, BC_INC2_EIP ); break;
    case 3: add_word ( p, BC_INC3_EIP ); break;
    case 4: add_word ( p, BC_INC4_EIP ); break;
    case 5: add_word ( p, BC_INC5_EIP ); break;
    case 6: add_word ( p, BC_INC6_EIP ); break;
    case 7: add_word ( p, BC_INC7_EIP ); break;
    case 8: add_word ( p, BC_INC8_EIP ); break;
    case 9: add_word ( p, BC_INC9_EIP ); break;
    case 10: add_word ( p, BC_INC10_EIP ); break;
    case 11: add_word ( p, BC_INC11_EIP ); break;
    case 12: add_word ( p, BC_INC12_EIP ); break;
      
    case 14: add_word ( p, BC_INC14_EIP ); break;
    default:
      fprintf ( FERROR, "[EE] jit.c - update_eip: real_nbytes no suportats %d",
                e->inst.real_nbytes );
      exit ( EXIT_FAILURE );
    }
  if ( !stop ) p->v[p->N-1]+= BC_INC1_EIP_NOSTOP-BC_INC1_EIP;
  
} // end update_eip


static void
compile_set_count (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const int                op,
                   const bool               apply_mod
                   )
{

  uint16_t count;

  
  if ( e->inst.ops[op].type == IA32_CONSTANT_1 )
    add_word ( p, BC_SET_1_COUNT );
  else if ( e->inst.ops[op].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET_IMM_COUNT );
      count= (uint16_t) (e->inst.ops[op].u8);
      if ( apply_mod ) count&= 0x1F;
      add_word ( p, count );
    }
  else if ( e->inst.ops[op].type == IA32_CL )
    add_word ( p, apply_mod ? BC_SET_CL_COUNT : BC_SET_CL_COUNT_NOMOD );
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_set_count: Operador %d desconegut: %d\n",
                op, e->inst.ops[op].type );
      exit ( EXIT_FAILURE );
    }

} // end compile_set_count


static void
compile_get_sib (
                 const IA32_JIT_DisEntry *e,
                 IA32_JIT_Page           *p,
                 const int                op
                 )
{

  // Comencem pel val
  switch ( e->inst.ops[op].sib_val )
    {
    case IA32_SIB_VAL_EAX: add_word ( p, BC_SET32_EAX_OFFSET ); break;
    case IA32_SIB_VAL_ECX: add_word ( p, BC_SET32_ECX_OFFSET ); break;
    case IA32_SIB_VAL_EDX: add_word ( p, BC_SET32_EDX_OFFSET ); break;
    case IA32_SIB_VAL_EBX: add_word ( p, BC_SET32_EBX_OFFSET ); break;
    case IA32_SIB_VAL_ESP: add_word ( p, BC_SET32_ESP_OFFSET ); break;
    case IA32_SIB_VAL_DISP32:
      add_word ( p, BC_SET32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].sib_u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].sib_u32>>16) );
      break;
    case IA32_SIB_VAL_EBP: add_word ( p, BC_SET32_EBP_OFFSET ); break;
    case IA32_SIB_VAL_ESI: add_word ( p, BC_SET32_ESI_OFFSET ); break;
    case IA32_SIB_VAL_EDI: add_word ( p, BC_SET32_EDI_OFFSET ); break;
    default:
      fprintf ( FERROR, "[EE] jit.c - compile_get_sib: sib_val desconegut %d\n",
                e->inst.ops[op].sib_val );
      exit ( EXIT_FAILURE );
    }

  // Escala
  switch ( e->inst.ops[op].sib_scale )
    {
    case IA32_SIB_SCALE_NONE: break; // No fa res
    case IA32_SIB_SCALE_EAX: add_word( p, BC_ADD32_EAX_OFFSET ); break;
    case IA32_SIB_SCALE_ECX: add_word( p, BC_ADD32_ECX_OFFSET ); break;
    case IA32_SIB_SCALE_EDX: add_word( p, BC_ADD32_EDX_OFFSET ); break;
    case IA32_SIB_SCALE_EBX: add_word( p, BC_ADD32_EBX_OFFSET ); break;
    case IA32_SIB_SCALE_EBP: add_word( p, BC_ADD32_EBP_OFFSET ); break;
    case IA32_SIB_SCALE_ESI: add_word( p, BC_ADD32_ESI_OFFSET ); break;
    case IA32_SIB_SCALE_EDI: add_word( p, BC_ADD32_EDI_OFFSET ); break;
      
    case IA32_SIB_SCALE_EAX_2: add_word( p, BC_ADD32_EAX_2_OFFSET ); break;
    case IA32_SIB_SCALE_ECX_2: add_word( p, BC_ADD32_ECX_2_OFFSET ); break;
    case IA32_SIB_SCALE_EDX_2: add_word( p, BC_ADD32_EDX_2_OFFSET ); break;
    case IA32_SIB_SCALE_EBX_2: add_word( p, BC_ADD32_EBX_2_OFFSET ); break;
    case IA32_SIB_SCALE_EBP_2: add_word( p, BC_ADD32_EBP_2_OFFSET ); break;
    case IA32_SIB_SCALE_ESI_2: add_word( p, BC_ADD32_ESI_2_OFFSET ); break;
    case IA32_SIB_SCALE_EDI_2: add_word( p, BC_ADD32_EDI_2_OFFSET ); break;
    case IA32_SIB_SCALE_EAX_4: add_word( p, BC_ADD32_EAX_4_OFFSET ); break;
    case IA32_SIB_SCALE_ECX_4: add_word( p, BC_ADD32_ECX_4_OFFSET ); break;
    case IA32_SIB_SCALE_EDX_4: add_word( p, BC_ADD32_EDX_4_OFFSET ); break;
    case IA32_SIB_SCALE_EBX_4: add_word( p, BC_ADD32_EBX_4_OFFSET ); break;
    case IA32_SIB_SCALE_EBP_4: add_word( p, BC_ADD32_EBP_4_OFFSET ); break;
    case IA32_SIB_SCALE_ESI_4: add_word( p, BC_ADD32_ESI_4_OFFSET ); break;
    case IA32_SIB_SCALE_EDI_4: add_word( p, BC_ADD32_EDI_4_OFFSET ); break;
    case IA32_SIB_SCALE_EAX_8: add_word( p, BC_ADD32_EAX_8_OFFSET ); break;
    case IA32_SIB_SCALE_ECX_8: add_word( p, BC_ADD32_ECX_8_OFFSET ); break;
    case IA32_SIB_SCALE_EDX_8: add_word( p, BC_ADD32_EDX_8_OFFSET ); break;
    case IA32_SIB_SCALE_EBX_8: add_word( p, BC_ADD32_EBX_8_OFFSET ); break;
    case IA32_SIB_SCALE_EBP_8: add_word( p, BC_ADD32_EBP_8_OFFSET ); break;
    case IA32_SIB_SCALE_ESI_8: add_word( p, BC_ADD32_ESI_8_OFFSET ); break;
    case IA32_SIB_SCALE_EDI_8: add_word( p, BC_ADD32_EDI_8_OFFSET ); break;
    default:
      fprintf ( FERROR, "[EE] jit.c - compile_get_sib:"
                " sib_scale desconegut %d\n",
                e->inst.ops[op].sib_scale );
      exit ( EXIT_FAILURE );
    }
  
} // compile_get_sib


// Torna cert si ho ha processat
static bool
compile_get_addr (
                  const IA32_JIT_DisEntry *e,
                  IA32_JIT_Page           *p,
                  const int                op
                  )
{

  bool ret;

  
  ret= true;
  switch ( e->inst.ops[op].type )
    {
    case IA32_ADDR16_BX_SI:
      add_word ( p, BC_SET16_BX_SI_OFFSET );
      break;
    case IA32_ADDR16_BX_DI:
      add_word ( p, BC_SET16_BX_DI_OFFSET );
      break;
    case IA32_ADDR16_BP_SI:
      add_word ( p, BC_SET16_BP_SI_OFFSET );
      break;
    case IA32_ADDR16_BP_DI:
      add_word ( p, BC_SET16_BP_DI_OFFSET );
      break;
    case IA32_ADDR16_SI:
      add_word ( p, BC_SET16_SI_OFFSET );
      break;
    case IA32_ADDR16_DI:
      add_word ( p, BC_SET16_DI_OFFSET );
      break;
    case IA32_ADDR16_DISP16:
      add_word ( p, BC_SET16_IMM_OFFSET );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_BX:
      add_word ( p, BC_SET16_BX_OFFSET );
      break;
    case IA32_ADDR16_BX_SI_DISP8:
      add_word ( p, BC_SET16_BX_SI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR16_BX_DI_DISP8:
      add_word ( p, BC_SET16_BX_DI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR16_BP_SI_DISP8:
      add_word ( p, BC_SET16_BP_SI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR16_BP_DI_DISP8:
      add_word ( p, BC_SET16_BP_DI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;

    case IA32_ADDR16_SI_DISP8:
      add_word ( p, BC_SET16_SI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR16_DI_DISP8:
      add_word ( p, BC_SET16_DI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR16_BP_DISP8:
      add_word ( p, BC_SET16_BP_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR16_BX_DISP8:
      add_word ( p, BC_SET16_BX_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR16_BX_SI_DISP16:
      add_word ( p, BC_SET16_BX_SI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_BX_DI_DISP16:
      add_word ( p, BC_SET16_BX_DI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_BP_SI_DISP16:
      add_word ( p, BC_SET16_BP_SI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_BP_DI_DISP16:
      add_word ( p, BC_SET16_BP_DI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_SI_DISP16:
      add_word ( p, BC_SET16_SI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_DI_DISP16:
      add_word ( p, BC_SET16_DI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_BP_DISP16:
      add_word ( p, BC_SET16_BP_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR16_BX_DISP16:
      add_word ( p, BC_SET16_BX_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET16 );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    case IA32_ADDR32_EAX: add_word ( p, BC_SET32_EAX_OFFSET ); break;
    case IA32_ADDR32_ECX: add_word ( p, BC_SET32_ECX_OFFSET ); break;
    case IA32_ADDR32_EDX: add_word ( p, BC_SET32_EDX_OFFSET ); break;
    case IA32_ADDR32_EBX: add_word ( p, BC_SET32_EBX_OFFSET ); break;
      
    case IA32_ADDR32_ESI: add_word ( p, BC_SET32_ESI_OFFSET ); break;
    case IA32_ADDR32_EDI: add_word ( p, BC_SET32_EDI_OFFSET ); break;

    case IA32_ADDR32_SIB: compile_get_sib ( e, p, op ); break;
      
    case IA32_ADDR32_DISP32:
      add_word ( p, BC_SET32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;

    case IA32_ADDR32_EAX_DISP8:
      add_word ( p, BC_SET32_EAX_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR32_ECX_DISP8:
      add_word ( p, BC_SET32_ECX_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;      
    case IA32_ADDR32_EDX_DISP8:
      add_word ( p, BC_SET32_EDX_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR32_EBX_DISP8:
      add_word ( p, BC_SET32_EBX_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR32_SIB_DISP8:
      compile_get_sib ( e, p, op );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR32_EBP_DISP8:
      add_word ( p, BC_SET32_EBP_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR32_ESI_DISP8:
      add_word ( p, BC_SET32_ESI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR32_EDI_DISP8:
      add_word ( p, BC_SET32_EDI_OFFSET );
      add_word ( p, BC_ADD16_IMM_OFFSET );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[op].u8)) );
      break;
    case IA32_ADDR32_EAX_DISP32:
      add_word ( p, BC_SET32_EAX_OFFSET );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_ADDR32_ECX_DISP32:
      add_word ( p, BC_SET32_ECX_OFFSET );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_ADDR32_EDX_DISP32:
      add_word ( p, BC_SET32_EDX_OFFSET );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_ADDR32_EBX_DISP32:
      add_word ( p, BC_SET32_EBX_OFFSET );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_ADDR32_SIB_DISP32:
      compile_get_sib ( e, p, op );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_ADDR32_EBP_DISP32:
      add_word ( p, BC_SET32_EBP_OFFSET );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_ADDR32_ESI_DISP32:
      add_word ( p, BC_SET32_ESI_OFFSET );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_ADDR32_EDI_DISP32:
      add_word ( p, BC_SET32_EDI_OFFSET );
      add_word ( p, BC_ADD32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;

    case IA32_MOFFS_OFF32:
      add_word ( p, BC_SET32_IMM_OFFSET );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[op].u32>>16) );
      break;
    case IA32_MOFFS_OFF16:
      add_word ( p, BC_SET16_IMM_OFFSET );
      add_word ( p, e->inst.ops[op].u16 );
      break;
    default: ret= false;
    }

  return ret;
  
} // compile_get_addr


// Torna cert si ho ha processat
static void
compile_get_rm32_read_mem (
                           const IA32_JIT_DisEntry *e,
                           IA32_JIT_Page           *p,
                           const int                op,
                           const bool               use_res
                           )
{

  switch ( e->inst.ops[op].data_seg )
    {
    case IA32_DS:
      add_word ( p, BC_DS_READ32_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SS:
      add_word ( p, BC_SS_READ32_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_ES:
      add_word ( p, BC_ES_READ32_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_CS:
      add_word ( p, BC_CS_READ32_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_FS:
      add_word ( p, BC_FS_READ32_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_GS:
      add_word ( p, BC_GS_READ32_OP0 + (use_res ? -1 : op) );
      break;
    default:
      fprintf ( FERROR, "jit - compile_get_rm32_read_mem: segment"
                " desconegut %d en operador %d",
                e->inst.ops[op].data_seg, op );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_get_rm32_read_mem


// Torna cert si ho ha processat
static bool
compile_get_rm32_op (
                     const IA32_JIT_DisEntry *e,
                     IA32_JIT_Page           *p,
                     const int                op,
                     const bool               use_res,
                     const bool               apply_bitoffset_op1
                     )
{

  bool ret;


  ret= true;
  switch ( e->inst.ops[op].type )
    {
      
    case IA32_DR7:
      add_word ( p, BC_SET32_DR7_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_CR0:
      add_word ( p, BC_SET32_CR0_OP0 + (use_res ? -1 : op) );
      break;

    case IA32_CR2:
      add_word ( p, BC_SET32_CR2_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_CR3:
      add_word ( p, BC_SET32_CR3_OP0 + (use_res ? -1 : op) );
      break;
      
    case IA32_EAX:
      add_word ( p, BC_SET32_EAX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_ECX:
      add_word ( p, BC_SET32_ECX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_EDX:
      add_word ( p, BC_SET32_EDX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_EBX:
      add_word ( p, BC_SET32_EBX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_ESP:
      add_word ( p, BC_SET32_ESP_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_EBP:
      add_word ( p, BC_SET32_EBP_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_ESI:
      add_word ( p, BC_SET32_ESI_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_EDI:
      add_word ( p, BC_SET32_EDI_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_ES:
      add_word ( p, BC_SET32_ES_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_CS:
      add_word ( p, BC_SET32_CS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_SS:
      add_word ( p, BC_SET32_SS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_DS:
      add_word ( p, BC_SET32_DS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_FS:
      add_word ( p, BC_SET32_FS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_GS:
      add_word ( p, BC_SET32_GS_OP0 + (use_res ? -1 : op) );
      break;
    default: // Intenta operacions de memòria.
      ret= compile_get_addr ( e, p, op );
      if ( ret )
        {
          if ( apply_bitoffset_op1 )
            add_word ( p, BC_ADD32_BITOFFSETOP1_OFFSET );
          compile_get_rm32_read_mem ( e, p, op, use_res );
        }
    }
  
  return ret;
  
} // end compile_get_rm32_op


// Esta funció es gasta quan l'addreça d'escriptura s'ha de carregar en offset.
static bool
write_rm32_op (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const int                op
               )
{

  bool ret;
  
  
  ret= true;
  switch ( e->inst.ops[op].type )
    {
      
    case IA32_DR7: add_word ( p, BC_SET32_RES_DR7 ); break;
    case IA32_CR0: add_word ( p, BC_SET32_RES_CR0 ); break;
      
    case IA32_CR2: add_word ( p, BC_SET32_RES_CR2 ); break;
    case IA32_CR3: add_word ( p, BC_SET32_RES_CR3 ); break;
        
    case IA32_EAX: add_word ( p, BC_SET32_RES_EAX ); break;
    case IA32_ECX: add_word ( p, BC_SET32_RES_ECX ); break;
    case IA32_EDX: add_word ( p, BC_SET32_RES_EDX ); break;
    case IA32_EBX: add_word ( p, BC_SET32_RES_EBX ); break;
    case IA32_ESP: add_word ( p, BC_SET32_RES_ESP ); break;
    case IA32_EBP: add_word ( p, BC_SET32_RES_EBP ); break;
    case IA32_ESI: add_word ( p, BC_SET32_RES_ESI ); break;
    case IA32_EDI: add_word ( p, BC_SET32_RES_EDI ); break;
    case IA32_SEG_ES: add_word ( p, BC_SET32_RES_ES ); break;

    case IA32_SEG_SS: add_word ( p, BC_SET32_RES_SS ); break;
    case IA32_SEG_DS: add_word ( p, BC_SET32_RES_DS ); break;
    case IA32_SEG_FS: add_word ( p, BC_SET32_RES_FS ); break;
    case IA32_SEG_GS: add_word ( p, BC_SET32_RES_GS ); break;
    default: // Intenta operacions de memòria.
      ret= compile_get_addr ( e, p, op );
      if ( ret )
        {
          switch ( e->inst.ops[op].data_seg )
            {
            case IA32_DS: add_word ( p, BC_DS_RES_WRITE32 ); break;
            case IA32_SS: add_word ( p, BC_SS_RES_WRITE32 ); break;
            case IA32_ES: add_word ( p, BC_ES_RES_WRITE32 ); break;
            case IA32_CS: add_word ( p, BC_CS_RES_WRITE32 ); break;
            case IA32_FS: add_word ( p, BC_FS_RES_WRITE32 ); break;
            case IA32_GS: add_word ( p, BC_GS_RES_WRITE32 ); break;
            default:
              fprintf ( FERROR, "jit - write_rm32_op: segment"
                        " desconegut %d en operador %d",
                        e->inst.ops[op].data_seg, op );
              exit ( EXIT_FAILURE );
            }
        }
    }

  return ret;
  
} // end write_rm32_op


// Esta funció es gasta quan l'addreça d'escriptura ja està carregada
// en offset
static void
write_res32 (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p,
             const int                op
             )
{
  
  // Registres
  if ( e->inst.ops[op].type >= IA32_EAX && e->inst.ops[op].type <= IA32_EDI )
    {
      switch ( e->inst.ops[op].type )
        {
        case IA32_EAX: add_word ( p, BC_SET32_RES_EAX ); break;
        case IA32_ECX: add_word ( p, BC_SET32_RES_ECX ); break;
        case IA32_EDX: add_word ( p, BC_SET32_RES_EDX ); break;
        case IA32_EBX: add_word ( p, BC_SET32_RES_EBX ); break;
        case IA32_ESP: add_word ( p, BC_SET32_RES_ESP ); break;
        case IA32_EBP: add_word ( p, BC_SET32_RES_EBP ); break;
        case IA32_ESI: add_word ( p, BC_SET32_RES_ESI ); break;
        case IA32_EDI: add_word ( p, BC_SET32_RES_EDI ); break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - write_res32: Operador %d registre: %d\n",
                    op, e->inst.ops[op].type );
          exit ( EXIT_FAILURE );
        }
    }
  // Memòria.
  else
    {
      switch ( e->inst.ops[op].data_seg )
        {
        case IA32_DS: add_word ( p, BC_DS_RES_WRITE32 ); break;
        case IA32_SS: add_word ( p, BC_SS_RES_WRITE32 ); break;
        case IA32_ES: add_word ( p, BC_ES_RES_WRITE32 ); break;
        case IA32_CS: add_word ( p, BC_CS_RES_WRITE32 ); break;
        case IA32_FS: add_word ( p, BC_FS_RES_WRITE32 ); break;
        case IA32_GS: add_word ( p, BC_GS_RES_WRITE32 ); break;
        default:
          fprintf ( FERROR, "[EE] jit - write_res32: segment"
                    " desconegut %d en operador %d",
                    e->inst.ops[op].data_seg, op );
          exit ( EXIT_FAILURE );
        }
    }
  
} // end write_res32


static void
compile_inst_rm32_imm32_noflags_read (
                                      const IA32_JIT_DisEntry *e,
                                      IA32_JIT_Page           *p,
                                      const uint16_t           op_word
                                      )
{

  // Operador 0 (típicament rm o imm)
  if ( e->inst.ops[0].type == IA32_IMM32 )
    {
      add_word ( p, BC_SET32_IMM32_RES );
      add_word ( p, (uint16_t) (e->inst.ops[0].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[0].u32>>16) );
    }
  else if ( e->inst.ops[0].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET32_SIMM_RES );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) (e->inst.ops[0].u8))) );
    }
  else if ( !compile_get_rm32_op ( e, p, 0, true, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm32_imm32_noflags_read: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_inst_rm32_imm32_noflags_read


static void
compile_inst_rm32_noflags_write (
                                 const IA32_JIT_DisEntry *e,
                                 IA32_JIT_Page           *p,
                                 const uint16_t           op_word
                                 )
{

  add_word ( p, op_word );
    
  // Operador 0 (típicament rm)
  if ( !write_rm32_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm32_noflags_write: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  update_eip ( e, p, true );
  
} // end compile_inst_rm32_noflags_write


static void
compile_inst_rm32_noflags_read (
                                const IA32_JIT_DisEntry *e,
                                IA32_JIT_Page           *p,
                                const uint16_t           op_word
                                )
{
    
  // Operador 0 (típicament rm)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm32_noflags_read: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_inst_rm32_noflags_read


static void
compile_inst_rm32_noflags_read_write (
                                      const IA32_JIT_DisEntry *e,
                                      IA32_JIT_Page           *p,
                                      const uint16_t           op_word,
                                      const int                read_op
                                      )
{
    
  // Operador 0 (típicament rm)
  if ( !compile_get_rm32_op ( e, p, read_op, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm32_noflags_read: Operador"
                " %d desconegut: %d\n",
                read_op, e->inst.ops[read_op].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, op_word );
  write_res32 ( e, p, 0 );
  update_eip ( e, p, true );
  
} // end compile_inst_rm32_noflags_read_write


static void
compile_inst_imm16_imm8 (
                         const IA32_JIT_DisEntry *e,
                         IA32_JIT_Page           *p,
                         const uint16_t           op_word
                         )
{

  // Operador 0 ha de ser imm16
  if ( e->inst.ops[0].type != IA32_IMM16 )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_imm16_imm8: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  // Operador 1 ha de ser imm16
  if ( e->inst.ops[1].type != IA32_IMM8 )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_imm16_imm8: Operador"
                " 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, op_word );
  add_word ( p, e->inst.ops[0].u16 );
  add_word ( p, e->inst.ops[1].u8 );
  update_eip ( e, p, true );
  
} // end compile_inst_imm16_imm8


static void
compile_lop32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const uint16_t           op_word,
               const bool               write
               )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lop32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1 (típicament rm o imm8 o imm)
  if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      add_word ( p, BC_SET32_SIMM_OP1 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
    }
  else if ( e->inst.ops[1].type == IA32_IMM32 ) // Especial pel signe
    {
      add_word ( p, BC_SET32_IMM32_OP1 );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32>>16) );
    }
  else if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lop32: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, op_word );
  if ( write ) write_res32 ( e, p, 0 );
  
  // Flags
  if ( (e->flags&OF_FLAG)!=0 || (e->flags&CF_FLAG)!=0 )
    add_word ( p, BC_CLEAR_OF_CF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_lop32


static void
compile_mov32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
      
  // Operador 1 (origen) (típicament rm o IMM)
  if ( e->inst.ops[1].type == IA32_IMM32 ) // Especial pel signe
    {
      add_word ( p, BC_SET32_IMM32_RES );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32>>16) );
    }
  else if ( !compile_get_rm32_op ( e, p, 1, true, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mov32: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  // Operador 0 (destí) (típicament rm o EAX)
  if ( !write_rm32_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mov32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // EIP
  update_eip ( e, p, true );
  
} // end compile_mov32


static void
compile_movszx32 (
                  const IA32_JIT_DisEntry *e,
                  IA32_JIT_Page           *p,
                  const bool               is8,
                  const bool               sign
                  )
{

  if ( is8 )
    {
      // Operador 1 (origen) (típicament rm o IMM)
      /*
      if ( e->inst.ops[1].type == IA32_IMM32 ) // Especial pel signe
        {
          add_word ( p, BC_SET32_IMM32_RES );
          add_word ( p, (uint16_t) (e->inst.ops[1].u32&0xFFFF) );
          add_word ( p, (uint16_t) (e->inst.ops[1].u32>>16) );
        }
        else*/ if ( !compile_get_rm8_op ( e, p, 1, true ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_movszx32 8bit: Operador 1"
                    " desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, sign ? BC_SET32_RES8_RES_SIGNED : BC_SET32_RES8_RES );
    }
  else // 16 bit
    {
      if ( !compile_get_rm16_op ( e, p, 1, true ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_movsx32 16bit: Operador 1"
                    " desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, sign ? BC_SET32_RES16_RES_SIGNED : BC_SET32_RES16_RES );
    }
  
  // Operador 0 (destí) (típicament rm o EAX)
  if ( !write_rm32_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_movszx32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // EIP
  update_eip ( e, p, true );
  
} // end compile_movszx32


static void
compile_add32_like (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               op1_is_1,
                    const bool               update_cflag,
                    const bool               use_carry
                    )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_add32_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  if ( op1_is_1 ) add_word ( p, BC_SET32_1_OP1 );
  
  // Operador 1 (típicament rm o imm8 o imm)
  else
    {
      if ( e->inst.ops[1].type == IA32_IMM32 ) // Amb 32 el signe no importa
        {
          add_word ( p, BC_SET32_IMM32_OP1 );
          add_word ( p, (uint16_t) (e->inst.ops[1].u32&0xFFFF) );
          add_word ( p, (uint16_t) (e->inst.ops[1].u32>>16) );
        }
      else if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
        {
          add_word ( p, BC_SET32_SIMM_OP1 );
          add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
        }
      else if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_add32_like: Operador"
                    " 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  
  // Operació
  add_word ( p, use_carry ? BC_ADC32 : BC_ADD32 );
  write_res32 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_AD_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_ZF_EFLAGS );
  if ( e->flags&AF_FLAG ) add_word ( p, BC_SET32_AD_AF_EFLAGS );
  if ( (e->flags&CF_FLAG) && update_cflag )
    add_word ( p, BC_SET32_AD_CF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_add32_like


static void
compile_sub32_like (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               write_result,
                    const bool               op1_is_1,
                    const bool               update_cflag,
                    const bool               use_carry,
                    const bool               neg_op
                    )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sub32_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament rm o imm8 o imm)
  if ( neg_op ) add_word ( p, BC_SET32_OP0_OP1_0_OP0 );
  
  else if ( op1_is_1 ) add_word ( p, BC_SET32_1_OP1 );

  else if ( e->inst.ops[1].type == IA32_IMM32 ) // Amb 32 el signe no importa
    {
      add_word ( p, BC_SET32_IMM32_OP1 );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32>>16) );
    }
  else if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      add_word ( p, BC_SET32_SIMM_OP1 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
    }
  else if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sub32_like: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, use_carry ? BC_SBB32 : BC_SUB32 );
  if ( write_result ) write_res32 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SB_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_ZF_EFLAGS );
  if ( e->flags&AF_FLAG ) add_word ( p, BC_SET32_SB_AF_EFLAGS );
  if ( (e->flags&CF_FLAG) && update_cflag )
    add_word ( p, neg_op ? BC_SET32_NEG_CF_EFLAGS : BC_SET32_SB_CF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_sub32_like


static void
compile_mul32_like (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               is_imul
                    )
{

  int numops;


  numops= 3;

  // Operador 3
  if ( e->inst.ops[2].type == IA32_NONE ) --numops;
  else if ( e->inst.ops[2].type == IA32_IMM8 )
    {
      // NOTA!! Fique este en el OP0 i així puc ficar el 1 en el OP1
      // sense fer res estrany.
      numops= 3;
      add_word ( p, BC_SET32_SIMM_OP0 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[2].u8)) );
    }
  else if ( e->inst.ops[2].type == IA32_IMM32 )
    {
      // NOTA!! Fique este en el OP0 i així puc ficar el 1 en el OP1
      // sense fer res estrany.
      numops= 3;
      add_word ( p, BC_SET32_IMM32_OP0 );
      add_word ( p, (uint16_t) (e->inst.ops[2].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[2].u32>>16) );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mul32_like: Operador 2 desconegut: %d\n",
                e->inst.ops[2].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 2 i 1
  if ( e->inst.ops[1].type == IA32_NONE )
    {
      --numops;
      if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul32_like (1ops):"
                    " Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET32_EAX_OP1 );
    }
  else if ( numops == 3 )
    {
      if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul32_like (3ops):"
                    " Operador 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul32_like (2ops):"
                    " Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul32_like (2ops):"
                    " Operador 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  
  // Operació
  add_word ( p, is_imul ? BC_IMUL32 : BC_MUL32 );

  // Desa resultat i flags
  if ( numops == 3 )
    {
      
      // Desa
      add_word ( p, BC_SET32_RES64_RES );
      if ( !write_rm32_op ( e, p, 0 ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul32_like: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }

      // Flags
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, BC_SET32_MUL_CFOF_EFLAGS );
      
    }
  else if ( numops == 1 )
    {
      add_word ( p, BC_SET32_RES64_EAX_EDX );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, is_imul ?
                   BC_SET32_IMUL1_CFOF_EFLAGS :
                   BC_SET32_MUL1_CFOF_EFLAGS );
    }
  else
    {
      // Desa
      add_word ( p, BC_SET32_RES64_RES );
      write_res32 ( e, p, 0 );
      
      // Flags
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, BC_SET32_MUL_CFOF_EFLAGS );
    }
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_mul32_like


static void
compile_bt32_like (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const uint16_t           op,
                   const bool               write_res
                   )
{
  
  // Operador 1 (típicament rm o imm8)
  if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      add_word ( p, BC_SET32_SIMM_OP1 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
    }
  else if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_bt32_like: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 0 (típicament rm)
  if ( !compile_get_rm32_op ( e, p, 0, false, true ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_bt32_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, op );
  if ( write_res ) write_res32 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_bt32_like


static void
compile_sar32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sar32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SAR_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SAR_CF_EFLAGS );
  add_word ( p, BC_SAR32 );
  write_res32 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_sar32


static void
compile_shl32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shl32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SHL_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SHL_CF_EFLAGS );
  add_word ( p, BC_SHL32 );
  write_res32 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shl32


static void
compile_shld32 (
                const IA32_JIT_DisEntry *e,
                IA32_JIT_Page           *p
                )
{

  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shld32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament r)
  if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shld32: Operador 1 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 2 (count)
  compile_set_count ( e, p, 2, e->inst.ops[2].type == IA32_CL );

  // Operació
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SHLD_CF_EFLAGS );
  add_word ( p, BC_SHLD32 );
  write_res32 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SHLD_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shld32


static void
compile_shr32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shr32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SHR_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SHR_CF_EFLAGS );
  add_word ( p, BC_SHR32 );
  write_res32 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shr32


static void
compile_shrd32 (
                const IA32_JIT_DisEntry *e,
                IA32_JIT_Page           *p
                )
{
  
  // Operador 0 (típicament rm)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shrd32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament r)
  if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shrd32: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 2
  compile_set_count ( e, p, 2, e->inst.ops[2].type == IA32_CL );

  // Operació
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SHRD_CF_EFLAGS );
  add_word ( p, BC_SHRD32 );
  write_res32 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SHRD_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET32_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shrd32


static void
compile_rcl32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rcl32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_RCL_OF_EFLAGS );
  add_word ( p, BC_RCL32 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_RCL_CF_EFLAGS );
  write_res32 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rcl32


static void
compile_rcr32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rcr32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_RCR_OF_EFLAGS );
  add_word ( p, BC_RCR32 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_RCR_CF_EFLAGS );
  write_res32 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rcr32


static void
compile_rol32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o EAX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rol32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  // --> ROL=SHL
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SHL_OF_EFLAGS );
  add_word ( p, BC_ROL32 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_ROL_CF_EFLAGS );
  write_res32 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rol32


static void
compile_ror32 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_ror32: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_ROR_OF_EFLAGS );
  add_word ( p, BC_ROR32 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_ROR_CF_EFLAGS );
  write_res32 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_ror32


static void
compile_xchg32 (
                const IA32_JIT_DisEntry *e,
                IA32_JIT_Page           *p
                )
{
  
  // Operador 0 (típicament rm)
  if ( !compile_get_rm32_op ( e, p, 0, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_xchg32: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1 (típicament rm)
  if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_xchg32: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }

  // Desa valors
  add_word ( p, BC_SET32_OP1_RES );
  write_res32 ( e, p, 0 );
  add_word ( p, BC_SET32_OP0_RES );
  write_res32 ( e, p, 1 );
  
  update_eip ( e, p, true );
  
} // end compile_xchg32


// Torna cert si ho ha processat
static void
compile_get_rm16_read_mem (
                           const IA32_JIT_DisEntry *e,
                           IA32_JIT_Page           *p,
                           const int                op,
                           const bool               use_res
                           )
{
  
  switch ( e->inst.ops[op].data_seg )
    {
    case IA32_DS:
      add_word ( p, BC_DS_READ16_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SS:
      add_word ( p, BC_SS_READ16_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_ES:
      add_word ( p, BC_ES_READ16_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_CS:
      add_word ( p, BC_CS_READ16_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_FS:
      add_word ( p, BC_FS_READ16_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_GS:
      add_word ( p, BC_GS_READ16_OP0 + (use_res ? -1 : op) );
      break;
    default:
      fprintf ( FERROR, "jit - compile_get_rm16_read_mem: segment"
                " desconegut %d en operador %d",
                e->inst.ops[op].data_seg, op );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_get_rm16_read_mem


// Torna cert si ho ha processat
static bool
compile_get_rm16_op (
                     const IA32_JIT_DisEntry *e,
                     IA32_JIT_Page           *p,
                     const int                op,
                     const bool               use_res
                     )
{

  bool ret;
  
  
  ret= true;
  switch ( e->inst.ops[op].type )
    {
    case IA32_AX:
      add_word ( p, BC_SET16_AX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_CX:
      add_word ( p, BC_SET16_CX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_DX:
      add_word ( p, BC_SET16_DX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_BX:
      add_word ( p, BC_SET16_BX_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SP:
      add_word ( p, BC_SET16_SP_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_BP:
      add_word ( p, BC_SET16_BP_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SI:
      add_word ( p, BC_SET16_SI_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_DI:
      add_word ( p, BC_SET16_DI_OP0 + (use_res ? -1 : op) );
      break;

    case IA32_SEG_ES:
      add_word ( p, BC_SET16_ES_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_CS:
      add_word ( p, BC_SET16_CS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_SS:
      add_word ( p, BC_SET16_SS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_DS:
      add_word ( p, BC_SET16_DS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_FS:
      add_word ( p, BC_SET16_FS_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_SEG_GS:
      add_word ( p, BC_SET16_GS_OP0 + (use_res ? -1 : op) );
      break;
      
    default: // Intenta operacions de memòria.
      ret= compile_get_addr ( e, p, op );
      if ( ret ) compile_get_rm16_read_mem ( e, p, op, use_res );
    }

  return ret;
  
} // end compile_get_rm16_op


// Esta funció es gasta quan l'addreça d'escriptura ja està carregada
// en offset
static void
write_res16 (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p,
             const int                op
             )
{

  // Registres
  if ( e->inst.ops[op].type >= IA32_AX && e->inst.ops[op].type <= IA32_DI )
    {
      switch ( e->inst.ops[op].type )
        {
        case IA32_AX: add_word ( p, BC_SET16_RES_AX ); break;
        case IA32_CX: add_word ( p, BC_SET16_RES_CX ); break;
        case IA32_DX: add_word ( p, BC_SET16_RES_DX ); break;
        case IA32_BX: add_word ( p, BC_SET16_RES_BX ); break;
        case IA32_SP: add_word ( p, BC_SET16_RES_SP ); break;
        case IA32_BP: add_word ( p, BC_SET16_RES_BP ); break;
        case IA32_SI: add_word ( p, BC_SET16_RES_SI ); break;
        case IA32_DI: add_word ( p, BC_SET16_RES_DI ); break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - write_res16: Operador %d registre: %d\n",
                    op, e->inst.ops[op].type );
          exit ( EXIT_FAILURE );
        }
    }
  // Memòria.
  else
    {
      switch ( e->inst.ops[op].data_seg )
        {
        case IA32_DS: add_word ( p, BC_DS_RES_WRITE16 ); break;
        case IA32_SS: add_word ( p, BC_SS_RES_WRITE16 ); break;
        case IA32_ES: add_word ( p, BC_ES_RES_WRITE16 ); break;
        case IA32_CS: add_word ( p, BC_CS_RES_WRITE16 ); break;
        case IA32_FS: add_word ( p, BC_FS_RES_WRITE16 ); break;
        case IA32_GS: add_word ( p, BC_GS_RES_WRITE16 ); break;
        default:
          fprintf ( FERROR, "[EE] jit - write_res16: segment"
                    " desconegut %d en operador %d",
                    e->inst.ops[op].data_seg, op );
          exit ( EXIT_FAILURE );
        }
    }
  
} // end write_res16


static void
compile_inst_rm16_noflags_read_write (
                                      const IA32_JIT_DisEntry *e,
                                      IA32_JIT_Page           *p,
                                      const uint16_t           op_word,
                                      const int                read_op
                                      )
{
  
  // Operador read_op (típicament rm)
  if ( !compile_get_rm16_op ( e, p, read_op, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm16_noflags_read: Operador"
                " %d desconegut: %d\n",
                read_op, e->inst.ops[read_op].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, op_word );
  write_res16 ( e, p, 0 );
  update_eip ( e, p, true );
  
} // end compile_inst_rm16_noflags_read_write


static void
compile_inst_rm16_imm16_noflags_read (
                                      const IA32_JIT_DisEntry *e,
                                      IA32_JIT_Page           *p,
                                      const uint16_t           op_word
                                      )
{

  // Operador 0 (típicament rm o imm)
  if ( e->inst.ops[0].type == IA32_IMM16 )
    {
      add_word ( p, BC_SET16_IMM_RES );
      add_word ( p, e->inst.ops[0].u16 );
    }
  else if ( e->inst.ops[0].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET16_IMM_RES );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) (e->inst.ops[0].u8))) );
    }
  else if ( !compile_get_rm16_op ( e, p, 0, true ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm16_imm16_noflags_read: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_inst_rm16_imm16_noflags_read


// Esta funció es gasta quan l'addreça d'escriptura s'ha de carregar en offset.
static bool
write_rm16_op (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const int                op
               )
{

  bool ret;
  
  
  ret= true;
  switch ( e->inst.ops[op].type )
    {
    case IA32_AX: add_word ( p, BC_SET16_RES_AX ); break;
    case IA32_CX: add_word ( p, BC_SET16_RES_CX ); break;
    case IA32_DX: add_word ( p, BC_SET16_RES_DX ); break;
    case IA32_BX: add_word ( p, BC_SET16_RES_BX ); break;
    case IA32_SP: add_word ( p, BC_SET16_RES_SP ); break;
    case IA32_BP: add_word ( p, BC_SET16_RES_BP ); break;
    case IA32_SI: add_word ( p, BC_SET16_RES_SI ); break;
    case IA32_DI: add_word ( p, BC_SET16_RES_DI ); break;
    case IA32_SEG_ES: add_word ( p, BC_SET16_RES_ES ); break;
    case IA32_SEG_SS: add_word ( p, BC_SET16_RES_SS ); break;
    case IA32_SEG_DS: add_word ( p, BC_SET16_RES_DS ); break;
    case IA32_SEG_FS: add_word ( p, BC_SET16_RES_FS ); break;
    case IA32_SEG_GS: add_word ( p, BC_SET16_RES_GS ); break;
      
    default: // Intenta operacions de memòria.
      ret= compile_get_addr ( e, p, op );
      if ( ret )
        {
          switch ( e->inst.ops[op].data_seg )
            {
            case IA32_DS: add_word ( p, BC_DS_RES_WRITE16 ); break;
            case IA32_SS: add_word ( p, BC_SS_RES_WRITE16 ); break;
            case IA32_ES: add_word ( p, BC_ES_RES_WRITE16 ); break;
            case IA32_CS: add_word ( p, BC_CS_RES_WRITE16 ); break;
            case IA32_FS: add_word ( p, BC_FS_RES_WRITE16 ); break;
            case IA32_GS: add_word ( p, BC_GS_RES_WRITE16 ); break;
            default:
              fprintf ( FERROR, "jit - write_rm16_op: segment"
                        " desconegut %d en operador %d",
                        e->inst.ops[op].data_seg, op );
              exit ( EXIT_FAILURE );
            }
        }
    }

  return ret;
  
} // end write_rm16


static void
compile_inst_rm16_noflags_read (
                                const IA32_JIT_DisEntry *e,
                                IA32_JIT_Page           *p,
                                const uint16_t           op_word
                                )
{
    
  // Operador 0 (típicament rm)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm16_noflags_read: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_inst_rm16_noflags_read


static void
compile_inst_rm16_noflags_write (
                                 const IA32_JIT_DisEntry *e,
                                 IA32_JIT_Page           *p,
                                 const uint16_t           op_word
                                 )
{

  add_word ( p, op_word );
  
  // Operador 0 (típicament rm)
  if ( !write_rm16_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm16_noflags_write: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  update_eip ( e, p, true );
  
} // end compile_inst_rm16_noflags_write

static void
compile_lop16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const uint16_t           op_word,
               const bool               write
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lop16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1 (típicament rm o imm8 o imm)
  if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      add_word ( p, BC_SET16_IMM_OP1 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
    }
  else if ( e->inst.ops[1].type == IA32_IMM16 )
    {
      add_word ( p, BC_SET16_IMM_OP1 );
      add_word ( p, e->inst.ops[1].u16 );
    }
  else if ( !compile_get_rm16_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lop16: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, op_word );
  if ( write ) write_res16 ( e, p, 0 );
  
  // Flags
  if ( (e->flags&OF_FLAG)!=0 || (e->flags&CF_FLAG)!=0 )
    add_word ( p, BC_CLEAR_OF_CF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_lop16


static void
compile_xchg16 (
                const IA32_JIT_DisEntry *e,
                IA32_JIT_Page           *p
                )
{
    
  // Operador 0 (típicament rm)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_xchg16: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament rm o imm8 o imm)
  /*
  if ( e->inst.ops[1].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET8_IMM_OP1 );
      add_word ( p, (uint16_t) (e->inst.ops[1].u8) );
    }
    else*/ if ( !compile_get_rm16_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_xchg16: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }

  // Desa valors
  add_word ( p, BC_SET16_OP1_RES );
  write_res16 ( e, p, 0 );
  add_word ( p, BC_SET16_OP0_RES );
  write_res16 ( e, p, 1 );
  
  update_eip ( e, p, true );
  
} // end compile_xchg16


static void
compile_mov16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 1 (origen) (típicament rm o IMM)
  if ( e->inst.ops[1].type == IA32_IMM16 )
    {
      add_word ( p, BC_SET16_IMM_RES );
      add_word ( p, e->inst.ops[1].u16 );
    }
  else if ( !compile_get_rm16_op ( e, p, 1, true ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mov16: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  // Operador 0 (destí) (típicament rm o AX)
  if ( !write_rm16_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mov16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // EIP
  update_eip ( e, p, true );
  
} // end compile_mov16


static void
compile_movszx (
                const IA32_JIT_DisEntry *e,
                IA32_JIT_Page           *p,
                const bool               sign
                )
{
  
  if ( !compile_get_rm8_op ( e, p, 1, true ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_movszx 8bit: Operador 1"
                " desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, sign ? BC_SET16_RES8_RES_SIGNED : BC_SET16_RES8_RES );
  
  // Operador 0 (destí) (típicament rm o EAX)
  if ( !write_rm16_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_movszx: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_movszx


static void
compile_lsreg (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const IA32_SegmentName   seg,
               const bool               op32
               )
{
  
  // Operador 1
  if ( !compile_get_addr ( e, p, 1 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lsreg: Operador"
                " memòria 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  switch ( e->inst.ops[1].data_seg )
    {
    case IA32_DS:
      add_word ( p, op32 ?
                 BC_DS_READ_OFFSET_SELECTOR :
                 BC_DS_READ_OFFSET16_SELECTOR);
      break;
    case IA32_SS:
      add_word ( p, op32 ?
                 BC_SS_READ_OFFSET_SELECTOR :
                 BC_SS_READ_OFFSET16_SELECTOR);
      break;
    case IA32_ES:
      add_word ( p, op32 ?
                 BC_ES_READ_OFFSET_SELECTOR :
                 BC_ES_READ_OFFSET16_SELECTOR);
      break;
    case IA32_CS:
      add_word ( p, op32 ?
                 BC_CS_READ_OFFSET_SELECTOR :
                 BC_CS_READ_OFFSET16_SELECTOR);
      break;
    case IA32_FS:
      add_word ( p, op32 ?
                 BC_FS_READ_OFFSET_SELECTOR :
                 BC_FS_READ_OFFSET16_SELECTOR );
      break;
      
    default:
      fprintf ( FERROR, "jit - compile_lsreg: segment"
                " desconegut %d en operador 1",
                e->inst.ops[1].data_seg );
      exit ( EXIT_FAILURE );
    }

  // Assignació
  add_word ( p, BC_SET16_SELECTOR_RES );
  switch ( seg )
    {
    case IA32_DS: add_word ( p, BC_SET16_RES_DS ); break;
    case IA32_SS: add_word ( p, BC_SET16_RES_SS ); break;
    case IA32_ES: add_word ( p, BC_SET16_RES_ES ); break;

    case IA32_FS: add_word ( p, BC_SET16_RES_FS ); break;
    case IA32_GS: add_word ( p, BC_SET16_RES_GS ); break;
    default:
      fprintf ( FERROR, "jit - compile_lsreg: segment"
                " destí desconegut %d",
                seg );
      exit ( EXIT_FAILURE );
    }
  if ( op32 )
    {
      add_word ( p, BC_SET32_OFFSET_RES );
      if ( !write_rm32_op ( e, p, 0 ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_lsreg: Operador"
                    " 0 desconegut (B): %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      add_word ( p, BC_SET16_OFFSET_RES );
      if ( !write_rm16_op ( e, p, 0 ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_lsreg: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
    }
  update_eip ( e, p, true );
  
} // end compile_lsreg


static void
compile_add16_like (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               op1_is_1,
                    const bool               update_cflag,
                    const bool               use_carry
                    )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_add16_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  if ( op1_is_1 ) add_word ( p, BC_SET16_1_OP1 );
  
  // Operador 1 (típicament rm o imm8 o imm)
  else
    {
      if ( e->inst.ops[1].type == IA32_IMM16 ) // Amb 16 el signe no importa
        {
          add_word ( p, BC_SET16_IMM_OP1 );
          add_word ( p, (uint16_t) e->inst.ops[1].u16 );
        }
      else if ( e->inst.ops[1].type == IA32_IMM8 )
        {
          add_word ( p, BC_SET16_IMM_OP1 );
          add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
        }
      else if ( !compile_get_rm16_op ( e, p, 1, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_add16_like: Operador"
                    " 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  
  // Operació
  add_word ( p, use_carry ? BC_ADC16 : BC_ADD16 );
  write_res16 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_AD_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_ZF_EFLAGS );
  if ( e->flags&AF_FLAG ) add_word ( p, BC_SET16_AD_AF_EFLAGS );
  if ( (e->flags&CF_FLAG) && update_cflag )
    add_word ( p, BC_SET16_AD_CF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_add16_like


static void
compile_sub16_like (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               write_result,
                    const bool               op1_is_1,
                    const bool               update_cflag,
                    const bool               use_carry,
                    const bool               neg_op
                    )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sub16_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1 (típicament rm o imm8 o imm)
  if ( neg_op ) add_word ( p, BC_SET16_OP0_OP1_0_OP0 );
  
  // Operador 1 (típicament rm o imm8 o imm)
  else if ( op1_is_1 ) add_word ( p, BC_SET16_1_OP1 );
  
  else if ( e->inst.ops[1].type == IA32_IMM16 ) // Amb 16 el signe no importa
    {
      add_word ( p, BC_SET16_IMM_OP1 );
      add_word ( p, e->inst.ops[1].u16 );
    }
  else if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      add_word ( p, BC_SET16_IMM_OP1 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
    }
  else if ( !compile_get_rm16_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sub16_like: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, use_carry ? BC_SBB16 : BC_SUB16 );
  if ( write_result ) write_res16 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SB_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_ZF_EFLAGS );
  if ( e->flags&AF_FLAG ) add_word ( p, BC_SET16_SB_AF_EFLAGS );
  if ( (e->flags&CF_FLAG) && update_cflag )
    add_word ( p, neg_op ? BC_SET16_NEG_CF_EFLAGS : BC_SET16_SB_CF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_sub16_like


static void
compile_mul16_like (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               is_imul
                    )
{

  int numops;


  numops= 3;

  // Operador 3
  if ( e->inst.ops[2].type == IA32_NONE ) --numops;
  else if ( e->inst.ops[2].type == IA32_IMM8 )
    {
      // NOTA!! Fique este en el OP0 i així puc ficar el 1 en el OP1
      // sense fer res estrany.
      numops= 3;
      add_word ( p, BC_SET16_IMM_OP0 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[2].u8)) );
    }
  else if ( e->inst.ops[2].type == IA32_IMM16 )
    {
      // NOTA!! Fique este en el OP0 i així puc ficar el 1 en el OP1
      // sense fer res estrany.
      numops= 3;
      add_word ( p, BC_SET16_IMM_OP0 );
      add_word ( p, e->inst.ops[2].u16 );
    }
  /*
  else if ( e->inst.ops[2].type == IA32_IMM32 )
    {
      // NOTA!! Fique este en el OP0 i així puc ficar el 1 en el OP1
      // sense fer res estrany.
      numops= 3;
      add_word ( p, BC_SET32_IMM32_OP0 );
      add_word ( p, (uint16_t) (e->inst.ops[2].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[2].u32>>16) );
      }*/
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mul16_like: Operador 2 desconegut: %d\n",
                e->inst.ops[2].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 2 i 1
  if ( e->inst.ops[1].type == IA32_NONE )
    {
      --numops;
      if ( !compile_get_rm16_op ( e, p, 0, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul16_like (1ops):"
                    " Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET16_AX_OP1 );
    }
  else if ( numops == 3 )
    {
      if ( !compile_get_rm16_op ( e, p, 1, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul16_like (3ops):"
                    " Operador 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      if ( !compile_get_rm16_op ( e, p, 0, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul16_like (2ops):"
                    " Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      if ( !compile_get_rm16_op ( e, p, 1, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul16_like (2ops):"
                    " Operador 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  
  // Operació
  add_word ( p, is_imul ? BC_IMUL16 : BC_MUL16 );

  // Desa resultat i flags
  if ( numops == 3 )
    {
      
      // Desa
      add_word ( p, BC_SET16_RES32_RES );
      if ( !write_rm16_op ( e, p, 0 ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul16_like: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }

      // Flags
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, BC_SET16_MUL_CFOF_EFLAGS );
      
    }
  else if ( numops == 1 )
    {
      add_word ( p, BC_SET16_RES32_AX_DX );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, is_imul ?
                   BC_SET16_IMUL1_CFOF_EFLAGS :
                   BC_SET16_MUL1_CFOF_EFLAGS );
    }
  else
    {
      // Desa
      add_word ( p, BC_SET16_RES32_RES );
      write_res16 ( e, p, 0 );
      
      // Flags
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, BC_SET16_MUL_CFOF_EFLAGS );
    }
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_mul16_like


static void
compile_bt16_like (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const uint16_t           op,
                   const bool               write_res
                   )
{
  
  // Operador 1 (típicament rm o imm8)
  if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      add_word ( p, BC_SET16_IMM_OP1 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[1].u8)) );
    }
  else if ( !compile_get_rm16_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_bt16_like: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 0 (típicament rm)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_bt16_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, op );
  if ( write_res ) write_res16 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_bt16_like


static void
compile_rcl16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rcl16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_RCL_OF_EFLAGS );
  add_word ( p, BC_RCL16 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_RCL_CF_EFLAGS );
  write_res16 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rcl16


static void
compile_rcr16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rcr16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_RCR_OF_EFLAGS );
  add_word ( p, BC_RCR16 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_RCR_CF_EFLAGS );
  write_res16 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rcr16


static void
compile_rol16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rol16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  // --> ROL=SHL
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SHL_OF_EFLAGS );
  add_word ( p, BC_ROL16 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_ROL_CF_EFLAGS );
  write_res16 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rol16


static void
compile_ror16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_ror16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_ROR_OF_EFLAGS );
  add_word ( p, BC_ROR16 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_ROR_CF_EFLAGS );
  write_res16 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_ror16


static void
compile_sar16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sar16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  // SAR_OF no depen de res ni op.
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SAR_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SAR_CF_EFLAGS );
  add_word ( p, BC_SAR16 );
  write_res16 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_sar16


static void
compile_shl16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shl16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SHL_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SHL_CF_EFLAGS );
  add_word ( p, BC_SHL16 );
  write_res16 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shl16


static void
compile_shld16 (
                const IA32_JIT_DisEntry *e,
                IA32_JIT_Page           *p
                )
{

  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shld16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament r)
  if ( !compile_get_rm16_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shld16: Operador 1 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 2 (count)
  compile_set_count ( e, p, 2, e->inst.ops[2].type == IA32_CL );
  
  // Operació
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SHLD_CF_EFLAGS );
  add_word ( p, BC_SHLD16 );
  write_res16 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SHLD_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shld16


static void
compile_shr16 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shr16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SHR_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SHR_CF_EFLAGS );
  add_word ( p, BC_SHR16 );
  write_res16 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shr16


static void
compile_shrd16 (
                const IA32_JIT_DisEntry *e,
                IA32_JIT_Page           *p
                )
{
  
  // Operador 0 (típicament rm)
  if ( !compile_get_rm16_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shrd16: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament r)
  if ( !compile_get_rm16_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shrd16: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 2
  compile_set_count ( e, p, 2, e->inst.ops[2].type == IA32_CL );
  
  // Operació
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SHRD_CF_EFLAGS );
  add_word ( p, BC_SHRD16 );
  write_res16 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SHRD_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET16_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shrd16


// Torna cert si ho ha processat
static bool
compile_get_rm8_op (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const int                op,
                    const bool               use_res
                    )
{

  bool ret;
  
  
  ret= true;
  switch ( e->inst.ops[op].type )
    {

    case IA32_AL:
      add_word ( p, BC_SET8_AL_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_CL:
      add_word ( p, BC_SET8_CL_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_DL:
      add_word ( p, BC_SET8_DL_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_BL:
      add_word ( p, BC_SET8_BL_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_AH:
      add_word ( p, BC_SET8_AH_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_CH:
      add_word ( p, BC_SET8_CH_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_DH:
      add_word ( p, BC_SET8_DH_OP0 + (use_res ? -1 : op) );
      break;
    case IA32_BH:
      add_word ( p, BC_SET8_BH_OP0 + (use_res ? -1 : op) );
      break;
      
    default: // Intenta operacions de memòria.
      ret= compile_get_addr ( e, p, op );
      if ( ret )
        {
          switch ( e->inst.ops[op].data_seg )
            {
            case IA32_DS:
              add_word ( p, BC_DS_READ8_OP0 + (use_res ? -1 : op) );
              break;
            case IA32_SS:
              add_word ( p, BC_SS_READ8_OP0 + (use_res ? -1 : op) );
              break;
            case IA32_ES:
              add_word ( p, BC_ES_READ8_OP0 + (use_res ? -1 : op) );
              break;
            case IA32_CS:
              add_word ( p, BC_CS_READ8_OP0 + (use_res ? -1 : op) );
              break;
            case IA32_FS:
              add_word ( p, BC_FS_READ8_OP0 + (use_res ? -1 : op) );
              break;
            case IA32_GS:
              add_word ( p, BC_GS_READ8_OP0 + (use_res ? -1 : op) );
              break;
            default:
              fprintf ( FERROR, "jit - compile_get_rm8_op: segment"
                        " desconegut %d en operador %d",
                        e->inst.ops[op].data_seg, op );
              exit ( EXIT_FAILURE );
            }
        }
    }

  return ret;
  
} // end compile_get_rm8_op


// Esta funció es gasta quan l'addreça d'escriptura ja està carregada
// en offset
static void
write_res8 (
            const IA32_JIT_DisEntry *e,
            IA32_JIT_Page           *p,
            const int                op
            )
{

  // Registres
  if ( e->inst.ops[op].type >= IA32_AL && e->inst.ops[op].type <= IA32_BH )
    {
      switch ( e->inst.ops[op].type )
        {
        case IA32_AL: add_word ( p, BC_SET8_RES_AL ); break;
        case IA32_CL: add_word ( p, BC_SET8_RES_CL ); break;
        case IA32_DL: add_word ( p, BC_SET8_RES_DL ); break;
        case IA32_BL: add_word ( p, BC_SET8_RES_BL ); break;
        case IA32_AH: add_word ( p, BC_SET8_RES_AH ); break;
        case IA32_CH: add_word ( p, BC_SET8_RES_CH ); break;
        case IA32_DH: add_word ( p, BC_SET8_RES_DH ); break;
        case IA32_BH: add_word ( p, BC_SET8_RES_BH ); break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - write_res8: Operador %d registre: %d\n",
                    op, e->inst.ops[op].type );
          exit ( EXIT_FAILURE );
        }
    }
  // Memòria.
  else
    {
      switch ( e->inst.ops[op].data_seg )
        {
        case IA32_DS: add_word ( p, BC_DS_RES_WRITE8 ); break;
        case IA32_SS: add_word ( p, BC_SS_RES_WRITE8 ); break;
        case IA32_ES: add_word ( p, BC_ES_RES_WRITE8 ); break;
        case IA32_CS: add_word ( p, BC_CS_RES_WRITE8 ); break;
        case IA32_FS: add_word ( p, BC_FS_RES_WRITE8 ); break;
        case IA32_GS: add_word ( p, BC_GS_RES_WRITE8 ); break;
        default:
          fprintf ( FERROR, "[EE] jit - write_res8: segment"
                    " desconegut %d en operador %d",
                    e->inst.ops[op].data_seg, op );
          exit ( EXIT_FAILURE );
        }
    }
  
} // end write_res8


// Esta funció es gasta quan l'addreça d'escriptura s'ha de carregar en offset.
static bool
write_rm8_op (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const int                op
               )
{

  bool ret;
  
  
  ret= true;
  switch ( e->inst.ops[op].type )
    {
    case IA32_AL: add_word ( p, BC_SET8_RES_AL ); break;
    case IA32_CL: add_word ( p, BC_SET8_RES_CL ); break;
    case IA32_DL: add_word ( p, BC_SET8_RES_DL ); break;
    case IA32_BL: add_word ( p, BC_SET8_RES_BL ); break;
    case IA32_AH: add_word ( p, BC_SET8_RES_AH ); break;
    case IA32_CH: add_word ( p, BC_SET8_RES_CH ); break;
    case IA32_DH: add_word ( p, BC_SET8_RES_DH ); break;
    case IA32_BH: add_word ( p, BC_SET8_RES_BH ); break;
    default: // Intenta operacions de memòria.
      ret= compile_get_addr ( e, p, op );
      if ( ret )
        {
          switch ( e->inst.ops[op].data_seg )
            {
            case IA32_DS: add_word ( p, BC_DS_RES_WRITE8 ); break;
            case IA32_SS: add_word ( p, BC_SS_RES_WRITE8 ); break;
            case IA32_ES: add_word ( p, BC_ES_RES_WRITE8 ); break;
            case IA32_CS: add_word ( p, BC_CS_RES_WRITE8 ); break;
            case IA32_FS: add_word ( p, BC_FS_RES_WRITE8 ); break;
            case IA32_GS: add_word ( p, BC_GS_RES_WRITE8 ); break;
            default:
              fprintf ( FERROR, "jit - write_rm8_op: segment"
                        " desconegut %d en operador %d",
                        e->inst.ops[op].data_seg, op );
              exit ( EXIT_FAILURE );
            }
        }
    }

  return ret;
  
} // end write_rm8


static void
compile_inst_rm8_noflags_read (
                                const IA32_JIT_DisEntry *e,
                                IA32_JIT_Page           *p,
                                const uint16_t           op_word
                                )
{
    
  // Operador 0 (típicament rm)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm8_noflags_read: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_inst_rm8_noflags_read


static void
compile_inst_rm8_noflags_read_write (
                                     const IA32_JIT_DisEntry *e,
                                     IA32_JIT_Page           *p,
                                     const uint16_t           op_word
                                     )
{
    
  // Operador 0 (típicament rm)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_rm8_noflags_read_write: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, op_word );
  write_res8 ( e, p, 0 );
  update_eip ( e, p, true );
  
} // end compile_inst_rm8_noflags_read_write


static void
compile_lop8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p,
              const uint16_t           op_word,
              const bool               write
              )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lop8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1 (típicament rm o imm8 o imm)
  if ( e->inst.ops[1].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET8_IMM_OP1 );
      add_word ( p, (uint16_t) (e->inst.ops[1].u8) );
    }
  else if ( !compile_get_rm8_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lop8: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, op_word );
  if ( write ) write_res8 ( e, p, 0 );
  
  // Flags
  if ( (e->flags&OF_FLAG)!=0 || (e->flags&CF_FLAG)!=0 )
    add_word ( p, BC_CLEAR_OF_CF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_lop8


static void
compile_xchg8 (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{
    
  // Operador 0 (típicament rm)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_xchg8: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament rm o imm8 o imm)
  /*
  if ( e->inst.ops[1].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET8_IMM_OP1 );
      add_word ( p, (uint16_t) (e->inst.ops[1].u8) );
    }
    else*/ if ( !compile_get_rm8_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_xchg8: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }

  // Desa valors
  add_word ( p, BC_SET8_OP1_RES );
  write_res8 ( e, p, 0 );
  add_word ( p, BC_SET8_OP0_RES );
  write_res8 ( e, p, 1 );
  
  update_eip ( e, p, true );
  
} // end compile_xchg8


static void
compile_mov8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
      
  // Operador 1 (origen) (típicament rm o IMM)
  /*
  if ( e->inst.ops[1].type == IA32_IMM32 ) // Especial pel signe
    {
      add_word ( p, BC_SET32_IMM32_RES );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[1].u32>>16) );
    }
    else*/
  if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      add_word ( p, BC_SET8_IMM_RES );
      add_word ( p, (uint16_t) e->inst.ops[1].u8 );
    }
  else if ( !compile_get_rm8_op ( e, p, 1, true ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mov8: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 0 (destí) (típicament rm o EAX)
  if ( !write_rm8_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mov8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // EIP
  update_eip ( e, p, true );
  
} // end compile_mov8


static void
compile_add8_like (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const bool               op1_is_1,
                   const bool               update_cflag,
                   const bool               use_carry
                   )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false/*, false*/ ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_add8_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  if ( op1_is_1 ) add_word ( p, BC_SET8_1_OP1 );
  
  // Operador 1 (típicament rm o imm8 o imm)
  else
    {
      if ( e->inst.ops[1].type == IA32_IMM8 ) // El signe no importa
        {
          add_word ( p, BC_SET8_IMM_OP1 );
          add_word ( p, e->inst.ops[1].u8 );
        }
      else if ( !compile_get_rm8_op ( e, p, 1, false/*, false*/ ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_add8_like: Operador"
                    " 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  
  // Operació
  add_word ( p, use_carry ? BC_ADC8 : BC_ADD8 );
  write_res8 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_AD_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_ZF_EFLAGS );
  if ( e->flags&AF_FLAG ) add_word ( p, BC_SET8_AD_AF_EFLAGS );
  if ( (e->flags&CF_FLAG) && update_cflag )
    add_word ( p, BC_SET8_AD_CF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_add8_like


static void
compile_sub8_like (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               write_result,
                    const bool               op1_is_1,
                    const bool               update_cflag,
                    const bool               use_carry,
                    const bool               neg_op
                    )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sub8_like: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 1 (típicament rm o imm8 o imm)
  if ( neg_op ) add_word ( p, BC_SET8_OP0_OP1_0_OP0 );
  
  // Operador 1 (típicament rm o imm8)
  else if ( op1_is_1 ) add_word ( p, BC_SET8_1_OP1 );
  
  else if ( e->inst.ops[1].type == IA32_IMM8 ) // Especial pel signe
    {
      // No cal extendre el byte perquè ja és de 8 bits.
      add_word ( p, BC_SET8_IMM_OP1 );
      add_word ( p, (uint16_t) e->inst.ops[1].u8 );
    }
  else if ( !compile_get_rm8_op ( e, p, 1, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sub8_like: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operació
  add_word ( p, use_carry ? BC_SBB8 : BC_SUB8 );
  if ( write_result ) write_res8 ( e, p, 0 );
  
  // Flags
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SB_OF_EFLAGS );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_ZF_EFLAGS );
  if ( e->flags&AF_FLAG ) add_word ( p, BC_SET8_SB_AF_EFLAGS );
  if ( (e->flags&CF_FLAG) && update_cflag )
    add_word ( p, neg_op ? BC_SET8_NEG_CF_EFLAGS : BC_SET8_SB_CF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_sub8_like


static void
compile_mul8_like (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const bool               is_imul
                   )
{
  
  int numops;

  
  numops= 3;
  
  // Operador 3
  if ( e->inst.ops[2].type == IA32_NONE ) --numops;
  /*else if ( e->inst.ops[2].type == IA32_IMM8 )
    {
      // NOTA!! Fique este en el OP0 i així puc ficar el 1 en el OP1
      // sense fer res estrany.
      numops= 3;
      add_word ( p, BC_SET32_SIMM_OP0 );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[2].u8)) );
    }
  else if ( e->inst.ops[2].type == IA32_IMM32 )
    {
      // NOTA!! Fique este en el OP0 i així puc ficar el 1 en el OP1
      // sense fer res estrany.
      numops= 3;
      add_word ( p, BC_SET32_IMM32_OP0 );
      add_word ( p, (uint16_t) (e->inst.ops[2].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[2].u32>>16) );
      }*/
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mul8_like: Operador 2 desconegut: %d\n",
                e->inst.ops[2].type );
      exit ( EXIT_FAILURE );
    }

  // Operador 2 i 1
  if ( e->inst.ops[1].type == IA32_NONE )
    {
      --numops;
      if ( !compile_get_rm8_op ( e, p, 0, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul8_like (1ops):"
                    " Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET8_AL_OP1 );
    }
  else if ( numops == 3 )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mul8_like:"
                " CAL IMPLEMENTAR numops==3\n" );
      exit ( EXIT_FAILURE );
      /*
      if ( !compile_get_rm32_op ( e, p, 1, false, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul32_like (3ops):"
                    " Operador 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      */
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mul8_like:"
                " CAL IMPLEMENTAR numops==???\n" );
      exit ( EXIT_FAILURE );
      /*
      if ( !compile_get_rm16_op ( e, p, 0, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul16_like (2ops):"
                    " Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      if ( !compile_get_rm16_op ( e, p, 1, false ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul16_like (2ops):"
                    " Operador 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      */
    }
  
  // Operació
  add_word ( p, is_imul ? BC_IMUL8 : BC_MUL8 );

  // Desa resultat i flags
  if ( numops == 3 )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mul8_like:"
                " CAL IMPLEMENTAR numops==3 B\n" );
      exit ( EXIT_FAILURE );
      /*
      // Desa
      add_word ( p, BC_SET32_RES64_RES );
      if ( !write_rm32_op ( e, p, 0 ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_mul32_like: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }

      // Flags
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, BC_SET32_MUL_CFOF_EFLAGS );
      */
    }
  else if ( numops == 1 )
    {
      add_word ( p, BC_SET16_RES_AX );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, is_imul ?
                   BC_SET8_IMUL1_CFOF_EFLAGS :
                   BC_SET8_MUL1_CFOF_EFLAGS );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_mul8_like:"
                " CAL IMPLEMENTAR numops==??? B\n" );
      exit ( EXIT_FAILURE );
      /*
      // Desa
      add_word ( p, BC_SET16_RES32_RES );
      write_res16 ( e, p, 0 );
      
      // Flags
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
      if ( e->flags&(CF_FLAG|OF_FLAG) )
        add_word ( p, BC_SET16_MUL_CFOF_EFLAGS );
      */
    }
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_mul8_like


static void
compile_rcl8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rcl8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_RCL_OF_EFLAGS );
  add_word ( p, BC_RCL8 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_RCL_CF_EFLAGS );
  write_res8 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rcl8


static void
compile_rcr8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rcr8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_RCR_OF_EFLAGS );
  add_word ( p, BC_RCR8 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_RCR_CF_EFLAGS );
  write_res8 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rcr8


static void
compile_rol8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_rol8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  // --> ROL == SHL 
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SHL_OF_EFLAGS );
  add_word ( p, BC_ROL8 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_ROL_CF_EFLAGS );
  write_res8 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_rol8


static void
compile_ror8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_ror8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );
  
  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_ROR_OF_EFLAGS );
  add_word ( p, BC_ROR8 );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_ROR_CF_EFLAGS );
  write_res8 ( e, p, 0 );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_ror8


static void
compile_sar8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  // Operador 0 (típicament rm o AX)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sar8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  // SAR_OF no depen de res ni op.
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SAR_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_SAR_CF_EFLAGS );
  add_word ( p, BC_SAR8 );
  write_res8 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_sar8


static void
compile_shl8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shl8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SHL_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_SHL_CF_EFLAGS );
  add_word ( p, BC_SHL8 );
  write_res8 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shl8


static void
compile_shr8 (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  // Operador 0 (típicament rm o AL)
  if ( !compile_get_rm8_op ( e, p, 0, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_shr8: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1
  compile_set_count ( e, p, 1, true );

  // Operació
  if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SHR_OF_EFLAGS );
  if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_SHR_CF_EFLAGS );
  add_word ( p, BC_SHR8 );
  write_res8 ( e, p, 0 );
  
  // Flags
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SHIFT_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_SHIFT_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_SHIFT_PF_EFLAGS );
  
  // EIP
  update_eip ( e, p, true );
  
} // end compile_shr8


// Gaste el mateix per a 16 i 32 bits
static void
compile_lea (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p
             )
{

  // L'operador 1 ha de ser una adreça
  if ( !compile_get_addr ( e, p, 1 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lea: Operador 1 desconegut"
                " %d\n", e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }

  // L'operador 0 ha de ser un registre de 16 o 32 bits.
  switch ( e->inst.ops[0].type )
    {
    case IA32_AX: add_word ( p, BC_SET16_OFFSET_AX ); break;
    case IA32_CX: add_word ( p, BC_SET16_OFFSET_CX ); break;
    case IA32_DX: add_word ( p, BC_SET16_OFFSET_DX ); break;
    case IA32_BX: add_word ( p, BC_SET16_OFFSET_BX ); break;
    case IA32_SP: add_word ( p, BC_SET16_OFFSET_SP ); break;
    case IA32_BP: add_word ( p, BC_SET16_OFFSET_BP ); break;
    case IA32_SI: add_word ( p, BC_SET16_OFFSET_SI ); break;
    case IA32_DI: add_word ( p, BC_SET16_OFFSET_DI ); break;
        
    case IA32_EAX: add_word ( p, BC_SET32_OFFSET_EAX ); break;
    case IA32_ECX: add_word ( p, BC_SET32_OFFSET_ECX ); break;
    case IA32_EDX: add_word ( p, BC_SET32_OFFSET_EDX ); break;
    case IA32_EBX: add_word ( p, BC_SET32_OFFSET_EBX ); break;
    case IA32_ESP: add_word ( p, BC_SET32_OFFSET_ESP ); break;
    case IA32_EBP: add_word ( p, BC_SET32_OFFSET_EBP ); break;
    case IA32_ESI: add_word ( p, BC_SET32_OFFSET_ESI ); break;
    case IA32_EDI: add_word ( p, BC_SET32_OFFSET_EDI ); break;
    default:
      fprintf ( FERROR,
                "[EE] jit.c - compile_lea: Operador 0 desconegut"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  update_eip ( e, p, true );
  
} // end compile_lea


static void
compile_setcc (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const uint16_t           bc_cond
               )
{
  
  add_word ( p, bc_cond );
  add_word ( p, BC_SET8_COND_RES );
  if ( !write_rm8_op ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - setcc: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  update_eip ( e, p, true );
  
} // end compile_setcc


static void
compile_jcc (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p,
             const uint16_t           bc_cond,
             const bool               is32
             )
{

  // Fixa cond
  add_word ( p, bc_cond );
  
  // Sols accepta REL16, REL32 o REL8
  if ( e->inst.ops[0].type == IA32_REL8 )
    {
      add_word ( p, is32 ? BC_BRANCH32 : BC_BRANCH16 );
      /*
      add_word ( p, is32 ?
                 BC_JMP32_NEAR_REL_IF_COND :
                 BC_JMP16_NEAR_REL_IF_COND );
      */
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[0].u8)) );
      add_word ( p, e->inst.real_nbytes );
      /*
      add_word ( p, 0 );
      add_word ( p, 0 ); // Padding
      add_word ( p, BC_INC_IMM_EIP_AND_GOTO_EIP );
      add_word ( p, e->inst.real_nbytes );
      add_word ( p, 0 );
      add_word ( p, 0 );
      add_word ( p, 0 ); // Padding
      */
    }
  else if ( e->inst.ops[0].type == IA32_REL16 )
    {
      assert ( !is32 );
      add_word ( p, BC_BRANCH16 );
      //add_word ( p, BC_JMP16_NEAR_REL_IF_COND );
      add_word ( p, e->inst.ops[0].u16 );
      add_word ( p, e->inst.real_nbytes );
      /*
      add_word ( p, 0 );
      add_word ( p, 0 ); // Padding
      add_word ( p, BC_INC_IMM_EIP_AND_GOTO_EIP );
      add_word ( p, e->inst.real_nbytes );
      add_word ( p, 0 );
      add_word ( p, 0 ); // Padding
      */
    }
  else if ( e->inst.ops[0].type == IA32_REL32 )
    {
      assert ( is32 );
      add_word ( p, BC_BRANCH32_IMM32 );
      //add_word ( p, BC_JMP32_NEAR_REL32_IF_COND );
      add_word ( p, (uint16_t) (e->inst.ops[0].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[0].u32>>16) );
      add_word ( p, e->inst.real_nbytes );
      /*
      add_word ( p, 0 ); // Padding
      add_word ( p, BC_INC_IMM_EIP_AND_GOTO_EIP );
      add_word ( p, e->inst.real_nbytes );
      add_word ( p, 0 );
      add_word ( p, 0 ); // Padding
      */
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_jcc: Operador desconegut"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_jcc


static void
compile_jmp_far_like (
                      const IA32_JIT_DisEntry *e,
                      IA32_JIT_Page           *p,
                      const bool               is_call,
                      const bool               op32
                      )
{

  // Operadors
  if ( e->inst.ops[0].type == IA32_PTR16_16 )
    {
      add_word ( p, BC_SET16_IMM_SELECTOR_OFFSET );
      add_word ( p, e->inst.ops[0].ptr16 ); // Selector
      add_word ( p, e->inst.ops[0].u16 ); // Offset 16
    }
  else if ( e->inst.ops[0].type == IA32_PTR16_32 )
    {
      add_word ( p, BC_SET32_IMM_SELECTOR_OFFSET );
      add_word ( p, e->inst.ops[0].ptr16 ); // Selector
      add_word ( p, (uint16_t) (e->inst.ops[0].u32&0xFFFF) ); // Offset 32
      add_word ( p, (uint16_t) (e->inst.ops[0].u32>>16) );
    }
  else if ( compile_get_addr ( e, p, 0 ) )
    {
      switch ( e->inst.ops[0].data_seg )
        {
        case IA32_DS:
          add_word ( p, op32 ?
                     BC_DS_READ_OFFSET_SELECTOR :
                     BC_DS_READ_OFFSET16_SELECTOR );
          break;
        case IA32_SS:
          add_word ( p, op32 ?
                     BC_SS_READ_OFFSET_SELECTOR :
                     BC_SS_READ_OFFSET16_SELECTOR );
          break;
        case IA32_ES:
          add_word ( p, op32 ?
                     BC_ES_READ_OFFSET_SELECTOR :
                     BC_ES_READ_OFFSET16_SELECTOR );
          break;
        case IA32_CS:
          add_word ( p, op32 ?
                     BC_CS_READ_OFFSET_SELECTOR :
                     BC_CS_READ_OFFSET16_SELECTOR );
          break;

        case IA32_GS:
          add_word ( p, op32 ?
                     BC_GS_READ_OFFSET_SELECTOR :
                     BC_GS_READ_OFFSET16_SELECTOR );
          break;
        default:
          fprintf ( FERROR, "jit - compile_jmp_far_like: segment"
                    " desconegut %d en operador 0",
                    e->inst.ops[0].data_seg );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile: JMP_FAR %d %d %d (%X)\n",
                e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit ( EXIT_FAILURE );
    }

  // Instrucció
  // Ja fa GOTO_EIP
  if ( is_call )
    {
      add_word ( p, op32 ? BC_CALL32_FAR : BC_CALL16_FAR );
      add_word ( p, e->inst.real_nbytes );
    }
  else add_word ( p, op32 ? BC_JMP32_FAR : BC_JMP16_FAR ); 
  
} // end compile_jmp_far


static void
compile_jmp_near_like (
                       const IA32_JIT_DisEntry *e,
                       IA32_JIT_Page           *p,
                       const bool               op32,
                       const bool               is_call // en compte de JMP
                       )
{

  // Operadors
  if ( e->inst.ops[0].type == IA32_REL32 )
    {
      if ( op32 != true )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile: op32??? (1) "
                    "JMP_NEAR RM16 %d %d %d (%X)\n",
                    e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, is_call ? BC_CALL32_NEAR_REL32 : BC_JMP32_NEAR_REL32 );
      add_word ( p, (uint16_t) (e->inst.ops[0].u32&0xFFFF) );
      add_word ( p, (uint16_t) (e->inst.ops[0].u32>>16) );
      add_word ( p, e->inst.real_nbytes );
    }
  else if ( e->inst.ops[0].type == IA32_REL16 )
    {
      if ( is_call )
        add_word ( p, op32 ? BC_CALL32_NEAR_REL : BC_CALL16_NEAR_REL );
      else
        add_word ( p, op32 ? BC_JMP32_NEAR_REL : BC_JMP16_NEAR_REL );
      add_word ( p, e->inst.ops[0].u16 );
      add_word ( p, e->inst.real_nbytes );
    }
  else if ( e->inst.ops[0].type == IA32_REL8 )
    {
      if ( is_call )
        {
          fprintf ( FERROR, "[EE] jit.c - compile_jmp_near_like: CALL_NEAR_REL8\n " );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, op32 ? BC_JMP32_NEAR_REL : BC_JMP16_NEAR_REL );
      add_word ( p, (uint16_t) ((int16_t) ((int8_t) e->inst.ops[0].u8)) );
      add_word ( p, e->inst.real_nbytes );
    }
  else if ( op32 && compile_get_rm32_op ( e, p, 0, true, false ) )
    {
      add_word ( p, is_call ? BC_CALL32_NEAR_RES32 : BC_JMP32_NEAR_RES32 );
      if ( is_call ) add_word ( p, e->inst.real_nbytes );
    }
  else if ( !op32 && compile_get_rm16_op ( e, p, 0, true ) )
    {
      add_word ( p, is_call ? BC_CALL16_NEAR_RES16 : BC_JMP16_NEAR_RES16 );
      if ( is_call ) add_word ( p, e->inst.real_nbytes );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile: JMP_NEAR %d %d %d (%X)\n",
                e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_jmp_near_like


static void
compile_ret_near (
                  const IA32_JIT_DisEntry *e,
                  IA32_JIT_Page           *p,
                  const bool               op32
                  )
{

  if ( op32 )
    {
      add_word ( p, BC_POP32 );
      if ( e->inst.ops[0].type == IA32_NONE )
        add_word ( p, BC_RET32_RES );
      else if ( e->inst.ops[0].type == IA32_IMM16 )
        {
          assert ( e->inst.ops[1].type == IA32_USE_ADDR32 ||
                   e->inst.ops[1].type == IA32_USE_ADDR16 );
          add_word ( p, e->inst.ops[1].type == IA32_USE_ADDR32 ?
                     BC_RET32_RES_INC_ESP : BC_RET32_RES_INC_SP );
          add_word ( p, e->inst.ops[0].u16 );
        }
      else
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ret32_near: %d %d %d (%X)\n",
                    e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      add_word ( p, BC_POP16 );
      if ( e->inst.ops[0].type == IA32_NONE )
        add_word ( p, BC_RET16_RES );
      else if ( e->inst.ops[0].type == IA32_IMM16 )
        {
          assert ( e->inst.ops[1].type == IA32_USE_ADDR32 ||
                   e->inst.ops[1].type == IA32_USE_ADDR16 );
          add_word ( p, e->inst.ops[1].type == IA32_USE_ADDR32 ?
                     BC_RET16_RES_INC_ESP : BC_RET16_RES_INC_SP );
          add_word ( p, e->inst.ops[0].u16 );
        }
      else
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ret16_near: %d %d %d (%X)\n",
                    e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit ( EXIT_FAILURE );
        }
    }
  
} // end compile_ret_near


static void
compile_ret_far (
                 const IA32_JIT_DisEntry *e,
                 IA32_JIT_Page           *p,
                 const bool               op32
                 )
{

  if ( op32 )
    {
      if ( e->inst.ops[0].type == IA32_IMM16 )
        {
          assert ( e->inst.ops[1].type == IA32_USE_ADDR32 ||
                   e->inst.ops[1].type == IA32_USE_ADDR16 );
          add_word ( p, BC_RET32_FAR_NOSTOP );
          add_word ( p, e->inst.ops[1].type == IA32_USE_ADDR32 ?
                     BC_INC_ESP_AND_GOTO_EIP : BC_INC_SP_AND_GOTO_EIP );
          add_word ( p, e->inst.ops[0].u16 );
        }
      else if ( e->inst.ops[0].type != IA32_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ret32_far: %d %d %d (%X)\n",
                    e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit ( EXIT_FAILURE );
        }
      else add_word ( p, BC_RET32_FAR );
    }
  else
    {
      if ( e->inst.ops[0].type == IA32_IMM16 )
        {
          assert ( e->inst.ops[1].type == IA32_USE_ADDR32 ||
                   e->inst.ops[1].type == IA32_USE_ADDR16 );
          add_word ( p, BC_RET16_FAR_NOSTOP );
          add_word ( p, e->inst.ops[1].type == IA32_USE_ADDR32 ?
                     BC_INC_ESP_AND_GOTO_EIP : BC_INC_SP_AND_GOTO_EIP );
          add_word ( p, e->inst.ops[0].u16 );
        }
      else if ( e->inst.ops[0].type != IA32_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ret16_far: %d %d %d (%X)\n",
                    e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit ( EXIT_FAILURE );
        }
      else add_word ( p, BC_RET16_FAR );
    }
  
} // end compile_ret_far


static void
compile_out (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p
             )
{

  // Operador 0 és el port
  if ( e->inst.ops[0].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET16_IMM_PORT );
      add_word ( p, (uint16_t) (e->inst.ops[0].u8) );
    }
  else if ( e->inst.ops[0].type == IA32_DX )
    add_word ( p, BC_SET16_DX_PORT );
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_out: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 1 és el valor
  if ( e->inst.ops[1].type == IA32_AL )
    {
      add_word ( p, BC_SET8_AL_RES );
      add_word ( p, BC_PORT_WRITE8 );
    }
  else if ( e->inst.ops[1].type == IA32_AX )
    {
      add_word ( p, BC_SET16_AX_RES );
      add_word ( p, BC_PORT_WRITE16 );
    }
  else if ( e->inst.ops[1].type == IA32_EAX )
    {
      add_word ( p, BC_SET32_EAX_RES );
      add_word ( p, BC_PORT_WRITE32 );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_out: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }

  update_eip ( e, p, true );
  
} // end compile_out


static void
compile_in (
            const IA32_JIT_DisEntry *e,
            IA32_JIT_Page           *p
            )
{

  // Operador 1 és el port
  if ( e->inst.ops[1].type == IA32_IMM8 )
    {
      add_word ( p, BC_SET16_IMM_PORT );
      add_word ( p, (uint16_t) (e->inst.ops[1].u8) );
    }
  else if ( e->inst.ops[1].type == IA32_DX )
    add_word ( p, BC_SET16_DX_PORT );
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_in: Operador 1 desconegut: %d\n",
                e->inst.ops[1].type );
      exit ( EXIT_FAILURE );
    }
  
  // Operador 0 és el valor
  if ( e->inst.ops[0].type == IA32_AL )
    {
      add_word ( p, BC_PORT_READ8 );
      add_word ( p, BC_SET8_RES_AL );
    }
  else if ( e->inst.ops[0].type == IA32_AX )
    {
      add_word ( p, BC_PORT_READ16 );
      add_word ( p, BC_SET16_RES_AX );
    }
  else if ( e->inst.ops[0].type == IA32_EAX )
    {
      add_word ( p, BC_PORT_READ32 );
      add_word ( p, BC_SET32_RES_EAX );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_in: Operador 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  update_eip ( e, p, true );
      
} // end compile_in


// Com LGDT/LIDT
static void
compile_inst_m1632_read (
                         const IA32_JIT_DisEntry *e,
                         IA32_JIT_Page           *p,
                         const uint16_t           op_word
                         )
{

  if ( !compile_get_addr ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_inst_m1632_read: Operador"
                " memòria 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  switch ( e->inst.ops[0].data_seg )
    {
    case IA32_DS: add_word ( p, BC_DS_READ_SELECTOR_OFFSET ); break;
    case IA32_SS: add_word ( p, BC_SS_READ_SELECTOR_OFFSET ); break;
    case IA32_ES: add_word ( p, BC_ES_READ_SELECTOR_OFFSET ); break;
    case IA32_CS: add_word ( p, BC_CS_READ_SELECTOR_OFFSET ); break;
      
    case IA32_GS: add_word ( p, BC_GS_READ_SELECTOR_OFFSET ); break;
    default:
      fprintf ( FERROR, "jit - compile_inst_m1632_read: segment"
                " desconegut %d en operador 0",
                e->inst.ops[0].data_seg );
      exit ( EXIT_FAILURE );
    }

  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_inst_m1632_read


static void
compile_read32_si (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const int                op,
                   const bool               use_res,
                   const bool               addr32
                   )
{

  add_word ( p, addr32 ? BC_SET32_ESI_OFFSET : BC_SET16_SI_OFFSET );
  switch ( e->inst.ops[op].data_seg )
    {
    case IA32_DS: add_word ( p, BC_DS_READ32_OP0 + (use_res ? -1 : op) ); break;
    case IA32_SS: add_word ( p, BC_SS_READ32_OP0 + (use_res ? -1 : op) ); break;
    case IA32_ES: add_word ( p, BC_ES_READ32_OP0 + (use_res ? -1 : op) ); break;
        
    default:
      fprintf ( FERROR,
                "[EE] jit.c - compile_read32_si: segment"
                " desconegut %d \n",
                e->inst.ops[op].data_seg );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_read32_si


static void
compile_read16_si (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const int                op,
                   const bool               use_res,
                   const bool               addr32
                   )
{

  add_word ( p, addr32 ? BC_SET32_ESI_OFFSET : BC_SET16_SI_OFFSET );
  switch ( e->inst.ops[op].data_seg )
    {
    case IA32_DS: add_word ( p, BC_DS_READ16_OP0 + (use_res ? -1 : op) ); break;
    case IA32_SS: add_word ( p, BC_SS_READ16_OP0 + (use_res ? -1 : op) ); break;
    case IA32_ES: add_word ( p, BC_ES_READ16_OP0 + (use_res ? -1 : op) ); break;
    case IA32_CS: add_word ( p, BC_CS_READ16_OP0 + (use_res ? -1 : op) ); break;
    case IA32_FS: add_word ( p, BC_FS_READ16_OP0 + (use_res ? -1 : op) ); break;
    default:
      fprintf ( FERROR,
                "[EE] jit.c - compile_read16_si: segment"
                " desconegut %d \n",
                e->inst.ops[op].data_seg );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_read16_si


static void
compile_read8_si (
                  const IA32_JIT_DisEntry *e,
                  IA32_JIT_Page           *p,
                  const int                op,
                  const bool               use_res,
                  const bool               addr32
                  )
{

  add_word ( p, addr32 ? BC_SET32_ESI_OFFSET : BC_SET16_SI_OFFSET );
  switch ( e->inst.ops[op].data_seg )
    {
    case IA32_DS: add_word ( p, BC_DS_READ8_OP0 + (use_res ? -1 : op) ); break;
    case IA32_SS: add_word ( p, BC_SS_READ8_OP0 + (use_res ? -1 : op) ); break;
    case IA32_ES: add_word ( p, BC_ES_READ8_OP0 + (use_res ? -1 : op) ); break;
    case IA32_CS: add_word ( p, BC_CS_READ8_OP0 + (use_res ? -1 : op) ); break;
    default:
      fprintf ( FERROR,
                "[EE] jit.c - compile_read8_si: segment"
                " desconegut %d \n",
                e->inst.ops[op].data_seg );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_read8_si


static void
compile_cmps (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{

  uint16_t nwords;


  // Calculoa paraules que té la instrucció
  nwords= 4; // Operació (3) i ZF que fems empre
  if ( e->flags&OF_FLAG ) ++nwords;
  if ( e->flags&SF_FLAG ) ++nwords;
  if ( e->flags&AF_FLAG ) ++nwords;
  if ( e->flags&CF_FLAG ) ++nwords;
  if ( e->flags&PF_FLAG ) ++nwords;
  
  if ( e->inst.ops[0].type == IA32_ADDR32_ESI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPNE ||
           e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_INCIMM_PC_IF_ECX_IS_0 );
          add_word ( p, nwords+2 ); // Bota el de tornar que ocupa 2
        }
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_cmps addr32: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_CMPS32:
          compile_read32_si ( e, p, 0, false, true );
          add_word ( p, BC_CMPS32_ADDR32 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
          add_word ( p, BC_SET32_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET32_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_PF_EFLAGS );
          break;
        case IA32_CMPS8:
          compile_read8_si ( e, p, 0, false, true );
          add_word ( p, BC_CMPS8_ADDR32 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
          add_word ( p, BC_SET8_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET8_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_cmps addr32: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPNE )
        {
          add_word ( p, BC_DECIMM_PC_IF_REPNE32 );
          add_word ( p, nwords );
        }
      else if ( e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_DECIMM_PC_IF_REPE32 );
          add_word ( p, nwords );
        }
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else if ( e->inst.ops[0].type == IA32_ADDR16_SI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_INCIMM_PC_IF_CX_IS_0 );
          add_word ( p, nwords+2 ); // Bota el de tornar que ocupa 2
        }
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_cmps addr16: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_CMPS32:
          compile_read32_si ( e, p, 0, false, false );
          add_word ( p, BC_CMPS32_ADDR16 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
          add_word ( p, BC_SET32_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET32_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_PF_EFLAGS );
          break;
        case IA32_CMPS16:
          compile_read16_si ( e, p, 0, false, false );
          add_word ( p, BC_CMPS16_ADDR16 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
          add_word ( p, BC_SET16_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET16_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_PF_EFLAGS );
          break;
        case IA32_CMPS8:
          compile_read8_si ( e, p, 0, false, false );
          add_word ( p, BC_CMPS8_ADDR16 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
          add_word ( p, BC_SET8_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET8_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_cmps addr16: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_DECIMM_PC_IF_REPE16 );
          add_word ( p, nwords );
        }
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_cmps: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit(EXIT_FAILURE);
    }
    
} // end compile_cmps


static void
compile_lods (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{

  if ( e->inst.ops[1].type == IA32_ADDR32_ESI )
    {
      // Prefixe repetició
      /*if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC2_PC_IF_CX_IS_0 );
        else*/ if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_lods addr32: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_LODS8:
          compile_read8_si ( e, p, 1, true, true );
          add_word ( p, BC_LODS8_ADDR32 );
          break;
        case IA32_LODS16:
          compile_read16_si ( e, p, 1, true, true );
          add_word ( p, BC_LODS16_ADDR32 );
          break;
        case IA32_LODS32:
          compile_read32_si ( e, p, 1, true, true );
          add_word ( p, BC_LODS32_ADDR32 );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_lods addr32: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      /*
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC1_PC_IF_REP16 );
      */
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
    else if ( e->inst.ops[1].type == IA32_ADDR16_SI )
    {
      // Prefixe repetició
      /*if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC2_PC_IF_CX_IS_0 );
        else*/ if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_lods addr16: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_LODS32:
          compile_read32_si ( e, p, 1, true, false );
          add_word ( p, BC_LODS32_ADDR16 );
          break;
        case IA32_LODS16:
          compile_read16_si ( e, p, 1, true, false );
          add_word ( p, BC_LODS16_ADDR16 );
          break;
        case IA32_LODS8:
          compile_read8_si ( e, p, 1, true, false );
          add_word ( p, BC_LODS8_ADDR16 );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_lods addr16: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      /*
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC1_PC_IF_REP16 );
      */
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_lods: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit(EXIT_FAILURE);
    }
  
} // end compile_lods


static void
compile_movs (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{

  if ( e->inst.ops[0].type == IA32_ADDR32_EDI &&
       e->inst.ops[1].type == IA32_ADDR32_ESI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC4_PC_IF_ECX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_movs addr32: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_MOVS32:
          compile_read32_si ( e, p, 1, true, true );
          add_word ( p, BC_MOVS32_ADDR32 );
          break;
        case IA32_MOVS16:
          compile_read16_si ( e, p, 1, true, true );
          add_word ( p, BC_MOVS16_ADDR32 );
          break;
        case IA32_MOVS8:
          compile_read8_si ( e, p, 1, true, true );
          add_word ( p, BC_MOVS8_ADDR32 );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_movs addr32: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC3_PC_IF_REP32 );
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else if ( e->inst.ops[0].type == IA32_ADDR16_DI &&
            e->inst.ops[1].type == IA32_ADDR16_SI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC4_PC_IF_CX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_movs addr16: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
            // Operació
      switch ( e->inst.name )
        {
        case IA32_MOVS32:
          compile_read32_si ( e, p, 1, true, false );
          add_word ( p, BC_MOVS32_ADDR16 );
          break;
        case IA32_MOVS16:
          compile_read16_si ( e, p, 1, true, false );
          add_word ( p, BC_MOVS16_ADDR16 );
          break;
        case IA32_MOVS8:
          compile_read8_si ( e, p, 1, true, false );
          add_word ( p, BC_MOVS8_ADDR16 );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_movs addr16: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC3_PC_IF_REP16 );
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_movs: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit(EXIT_FAILURE);
    }
  
} // end compile_movs


static void
compile_outs (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{

  if ( e->inst.ops[1].type == IA32_ADDR32_ESI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC4_PC_IF_ECX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_outs addr32: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_OUTS8:
          compile_read8_si ( e, p, 1, true, true );
          add_word ( p, BC_OUTS8_ADDR32 );
          break;
        case IA32_OUTS16:
          compile_read16_si ( e, p, 1, true, true );
          add_word ( p, BC_OUTS16_ADDR32 );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_outs addr32: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC3_PC_IF_REP32 );
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else if ( e->inst.ops[1].type == IA32_ADDR16_SI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC4_PC_IF_CX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_outs addr16: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_OUTS8:
          compile_read8_si ( e, p, 1, true, false );
          add_word ( p, BC_OUTS8_ADDR16 );
          break;
        case IA32_OUTS16:
          compile_read16_si ( e, p, 1, true, false );
          add_word ( p, BC_OUTS16_ADDR16 );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_outs addr16: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC3_PC_IF_REP16 );
      // Actualitza EIP.
      update_eip ( e, p, true );
      }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_outs: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit(EXIT_FAILURE);
    }
  
} // end compile_outs


static void
compile_scas (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{
  
  uint16_t nwords;
  
  
  // Calcula paraules que té la instrucció
  nwords= 2; // Operació (1) i ZF que fems empre
  if ( e->flags&OF_FLAG ) ++nwords;
  if ( e->flags&SF_FLAG ) ++nwords;
  if ( e->flags&AF_FLAG ) ++nwords;
  if ( e->flags&CF_FLAG ) ++nwords;
  if ( e->flags&PF_FLAG ) ++nwords;
  
  if ( e->inst.ops[0].type == IA32_ADDR32_EDI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPNE ||
           e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_INCIMM_PC_IF_ECX_IS_0 );
          add_word ( p, nwords+2 ); // Bota el de tornar que ocupa 2
        }
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_scas addr32: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_SCAS8:
          add_word ( p, BC_SCAS8_ADDR32 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
          add_word ( p, BC_SET8_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET8_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
          break;
        case IA32_SCAS16:
          add_word ( p, BC_SCAS16_ADDR32 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
          add_word ( p, BC_SET16_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET16_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_PF_EFLAGS );
          break;
        case IA32_SCAS32:
          add_word ( p, BC_SCAS32_ADDR32 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
          add_word ( p, BC_SET32_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET32_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_PF_EFLAGS );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_scas addr32: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPNE )
        {
          add_word ( p, BC_DECIMM_PC_IF_REPNE32 );
          add_word ( p, nwords );
        }
      else if ( e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_DECIMM_PC_IF_REPE32 );
          add_word ( p, nwords );
        }
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else if ( e->inst.ops[0].type == IA32_ADDR16_DI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPNE ||
           e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_INCIMM_PC_IF_CX_IS_0 );
          add_word ( p, nwords+2 ); // Bota el de tornar que ocupa 2
        }
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_scas addr16: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_SCAS8:
          add_word ( p, BC_SCAS8_ADDR16 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET8_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
          add_word ( p, BC_SET8_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET8_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET8_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
          break;
        case IA32_SCAS16:
          add_word ( p, BC_SCAS16_ADDR16 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET16_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET16_SF_EFLAGS );
          add_word ( p, BC_SET16_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET16_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET16_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET16_PF_EFLAGS );
          break;
        case IA32_SCAS32:
          add_word ( p, BC_SCAS32_ADDR16 );
          if ( e->flags&OF_FLAG ) add_word ( p, BC_SET32_SB_OF_EFLAGS );
          if ( e->flags&SF_FLAG ) add_word ( p, BC_SET32_SF_EFLAGS );
          add_word ( p, BC_SET32_ZF_EFLAGS );
          if ( e->flags&AF_FLAG ) add_word ( p, BC_SET32_SB_AF_EFLAGS );
          if ( e->flags&CF_FLAG ) add_word ( p, BC_SET32_SB_CF_EFLAGS );
          if ( e->flags&PF_FLAG ) add_word ( p, BC_SET32_PF_EFLAGS );
          break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_scas addr16: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REPNE )
        {
          add_word ( p, BC_DECIMM_PC_IF_REPNE16 );
          add_word ( p, nwords );
        }
      else if ( e->inst.prefix == IA32_PREFIX_REPE )
        {
          add_word ( p, BC_DECIMM_PC_IF_REPE16 );
          add_word ( p, nwords );
        }
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else 
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_scas BB: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit(EXIT_FAILURE);
    }
    
} // end compile_scas


static void
compile_stos (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{

  if ( e->inst.ops[0].type == IA32_ADDR32_EDI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC2_PC_IF_ECX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_movs addr32: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_STOS8: add_word ( p, BC_STOS8_ADDR32 ); break;
        case IA32_STOS16: add_word ( p, BC_STOS16_ADDR32 ); break;
        case IA32_STOS32: add_word ( p, BC_STOS32_ADDR32 ); break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_stos addr32: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC1_PC_IF_REP32 );
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else if ( e->inst.ops[0].type == IA32_ADDR16_DI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC2_PC_IF_CX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_stos addr16: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_STOS8: add_word ( p, BC_STOS8_ADDR16 ); break;
        case IA32_STOS16: add_word ( p, BC_STOS16_ADDR16 ); break;
        case IA32_STOS32: add_word ( p, BC_STOS32_ADDR16 ); break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_stos addr16: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC1_PC_IF_REP16 );
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
    else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_stos: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit(EXIT_FAILURE);
    }
  
} // end compile_stos


static void
compile_ins (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p
             )
{

  if ( e->inst.ops[0].type == IA32_ADDR32_EDI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC2_PC_IF_ECX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ins addr32: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_INS8: add_word ( p, BC_INS8_ADDR32 ); break;
        case IA32_INS16: add_word ( p, BC_INS16_ADDR32 ); break;
          
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ins addr32: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC1_PC_IF_REP32 );
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else if ( e->inst.ops[0].type == IA32_ADDR16_DI )
    {
      // Prefixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_INC2_PC_IF_CX_IS_0 );
      else if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ins addr16: prefixe desconegut"
                    " %d \n",
                    e->inst.prefix );
          exit ( EXIT_FAILURE );
        }
      // Operació
      switch ( e->inst.name )
        {
        case IA32_INS32: add_word ( p, BC_INS32_ADDR16 ); break;
        case IA32_INS16: add_word ( p, BC_INS16_ADDR16 ); break;
        default:
          fprintf ( FERROR,
                    "[EE] jit.c - compile_ins addr16: Instrucció desconeguda"
                    " %d %d %d %d (%X)\n",
                    e->inst.name, e->inst.ops[0].type,
                    e->inst.ops[1].type, e->inst.ops[2].type,
                    e->addr );
          exit(EXIT_FAILURE);
        }
      // Sufixe repetició
      if ( e->inst.prefix == IA32_PREFIX_REP )
        add_word ( p, BC_DEC1_PC_IF_REP16 );
      // Actualitza EIP.
      update_eip ( e, p, true );
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_ins: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit(EXIT_FAILURE);
    }
  
} // end compile_ins


static void
compile_sgdt_like (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const uint16_t           op_word
                   )
{

  // L'operador 0 ha de ser una adreça
  if ( !compile_get_addr ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sgdt_like: Operador 0 desconegut"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }

  // Desa valors en res16 i res32
  add_word ( p, op_word );

  // Escriu res16
  write_res16 ( e, p, 0 );

  // Esciure res32
  add_word ( p, BC_ADD16_IMM_OFFSET );
  add_word ( p, (uint16_t) 2 );
  write_res32 ( e, p, 0 );
  
  update_eip ( e, p, true );
  
} // end compile_sgdt_like


static void
compile_invlpg_like (
                     const IA32_JIT_DisEntry *e,
                     IA32_JIT_Page           *p,
                     const uint16_t           op_word
                     )
{

  // L'operador 0 ha de ser una adreça
  if ( !compile_get_addr ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_sgdt_like: Operador 0 desconegut"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_invlpg_like


static void
compile_int (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p,
             const bool               op32
             )
{
  
  add_word ( p, op32 ? BC_INT32 : BC_INT16 );
  add_word ( p, e->inst.real_nbytes );
  if ( e->inst.ops[0].type == IA32_IMM8 )
    add_word ( p, (uint16_t) (e->inst.ops[0].u8) );
  else if ( e->inst.ops[0].type == IA32_CONSTANT_3 )
    add_word ( p, 3 );
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_int: Operador 0 desconegut"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  
} // end compile_int


static void
compile_into (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p,
              const bool               op32
              )
{
  
  add_word ( p, op32 ? BC_INTO32 : BC_INTO16 );
  add_word ( p, e->inst.real_nbytes );
  assert ( e->inst.ops[0].type == IA32_NONE );
  
} // end compile_into


static void
compile_aam_like (
                  const IA32_JIT_DisEntry *e,
                  IA32_JIT_Page           *p,
                  const uint16_t           op_word
                  )
{

  // Operador
  if ( e->inst.ops[0].type != IA32_IMM8 )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_aam_like: Operador 0 no suportat"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_SET8_IMM_OP0 );
  add_word ( p, (uint16_t) (e->inst.ops[0].u8) );

  // Operació i flags
  add_word ( p, op_word );
  if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
  if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_ZF_EFLAGS );
  if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );

  // EIP
  update_eip ( e, p, true );
      
} // end compile_aam_like


static void
compile_lar (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p,
             const bool               op32
             )
{

  bool ret;

  
  ret= compile_get_addr ( e, p, 1 );
  if ( ret )
    {
      compile_get_rm16_read_mem ( e, p, 1, true );
      add_word ( p, BC_SET16_RES_SELECTOR );
    }
  else if ( op32 ) // Assumisc que és un registre de 32 bits.
    {
      if ( !compile_get_rm32_op ( e, p, 1, true, false ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lar B: Operador"
                    " 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET32_RES_SELECTOR );
    }
  else
    {
      if ( !compile_get_rm16_op ( e, p, 1, true ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lar: Operador"
                    " 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET16_RES_SELECTOR );
    }
  if ( op32 )
    {
      // Com LAR a vegades modifica i a vegades no, he decidit llegir el
      // valor en res32 i tornar-lo a escriure. Deurien ser sempre
      // registres.
      if ( !compile_get_rm32_op ( e, p, 0, true, false ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lar: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_LAR32 );
      write_res32 ( e, p, 0 );
    }
  else
    {
      // Com LAR a vegades modifica i a vegades no, he decidit llegir el
      // valor en res16 i tornar-lo a escriure. Deurien ser sempre
      // registres.
      if ( !compile_get_rm16_op ( e, p, 0, true ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lar B: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_LAR16 );
      write_res16 ( e, p, 0 );
    }
  update_eip ( e, p, true );
  
} // end compile_lar


static void
compile_lsl (
             const IA32_JIT_DisEntry *e,
             IA32_JIT_Page           *p,
             const bool               op32
             )
{

  bool ret;

  
  ret= compile_get_addr ( e, p, 1 );
  if ( ret )
    {
      compile_get_rm16_read_mem ( e, p, 1, true );
      add_word ( p, BC_SET16_RES_SELECTOR );
    }
  else if ( op32 ) // Assumisc que és un registre de 32 bits.
    {
      // En aquest punt és sols registre
      if ( !compile_get_rm32_op ( e, p, 1, true, false ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lsl 32: Operador"
                    " 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET32_RES_SELECTOR );
    }
  else // Assumisc que és un registre de 16 bits.
    {
      // En aquest punt és sols registre
      if ( !compile_get_rm16_op ( e, p, 1, true ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lsl 16: Operador"
                    " 1 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET16_RES_SELECTOR );
    }
  if ( op32 )
    {
      // Com LSL a vegades modifica i a vegades no, he decidit llegir el
      // valor en res32 i tornar-lo a escriure. Deurien ser sempre
      // registres.
      if ( !compile_get_rm32_op ( e, p, 0, true, false ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lsl 32: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_LSL32 );
      write_res32 ( e, p, 0 );
    }
  else
    {
      // Com LSL a vegades modifica i a vegades no, he decidit llegir el
      // valor en res16 i tornar-lo a escriure. Deurien ser sempre
      // registres.
      if ( !compile_get_rm16_op ( e, p, 0, true ) ) 
        {
          fprintf ( FERROR,
                    "[EE] jit. compile_lsl: Operador"
                    " 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_LSL16 );
      write_res16 ( e, p, 0 );
    }
  update_eip ( e, p, true );
  
} // end compile_lsl


static void
compile_xlatb (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p,
               const bool               op32
               )
{

  add_word ( p, op32 ? BC_SET32_EBX_AL_OFFSET : BC_SET16_BX_AL_OFFSET );
  switch ( e->inst.ops[0].data_seg )
    {
    case IA32_DS: add_word ( p, BC_DS_READ8_RES ); break;
    case IA32_SS: add_word ( p, BC_SS_READ8_RES ); break;
    case IA32_ES: add_word ( p, BC_ES_READ8_RES ); break;
    case IA32_CS: add_word ( p, BC_CS_READ8_RES ); break;
    case IA32_FS: add_word ( p, BC_FS_READ8_RES ); break;
    case IA32_GS: add_word ( p, BC_GS_READ8_RES ); break;
    default:
      fprintf ( FERROR, "jit - compile_xlatb: segment"
                " desconegut %d en operador 0",
                e->inst.ops[0].data_seg );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_SET8_RES_AL );
  update_eip ( e, p, true );
  
} // end compile_xlatb


// Torna cert si ho ha processat
static void
compile_fpu_read_float (
                        const IA32_JIT_DisEntry *e,
                        IA32_JIT_Page           *p
                        )
{

  
  if ( !compile_get_rm32_op ( e, p, 0, true, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit. compile_fpu_read_float: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_SET32_RES_FLOATU32 );
  
} // end compile_fpu_read_float


// Torna cert si ho ha processat
static void
compile_fpu_read_double (
                         const IA32_JIT_DisEntry *e,
                         IA32_JIT_Page           *p
                         )
{
  
  bool ret;
  

  ret= compile_get_addr ( e, p, 0 );
  if ( !ret )
    {
      fprintf ( FERROR,
                "[EE] jit. compile_fpu_read_double: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  compile_get_rm32_read_mem ( e, p, 0, true );
  add_word ( p, BC_SET32_RES_DOUBLEU64L );
  add_word ( p, BC_ADD16_IMM_OFFSET );
  add_word ( p, 4 );
  compile_get_rm32_read_mem ( e, p, 0, true );
  add_word ( p, BC_SET32_RES_DOUBLEU64H );
  
} // end compile_fpu_read_double


// Torna cert si ho ha processat
static void
compile_fpu_read_ldouble (
                          const IA32_JIT_DisEntry *e,
                          IA32_JIT_Page           *p
                          )
{
  
  bool ret;
  

  ret= compile_get_addr ( e, p, 0 );
  if ( !ret )
    {
      fprintf ( FERROR,
                "[EE] jit. compile_fpu_read_ldouble: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  compile_get_rm32_read_mem ( e, p, 0, true );
  add_word ( p, BC_SET32_RES_LDOUBLEU80L );
  add_word ( p, BC_ADD16_IMM_OFFSET );
  add_word ( p, 4 );
  compile_get_rm32_read_mem ( e, p, 0, true );
  add_word ( p, BC_SET32_RES_LDOUBLEU80M );
  add_word ( p, BC_ADD16_IMM_OFFSET );
  add_word ( p, 4 );
  compile_get_rm16_read_mem ( e, p, 0, true );
  add_word ( p, BC_SET16_RES_LDOUBLEU80H );
  
} // end compile_fpu_read_ldouble


static bool
compile_fpu_write_float (
                         const IA32_JIT_DisEntry *e,
                         IA32_JIT_Page           *p
                         )
{

  bool ret;
  
  
  ret= true;
  switch ( e->inst.ops[0].type )
    {
      /*
    case IA32_CR0: add_word ( p, BC_SET32_RES_CR0 ); break;
    case IA32_EAX: add_word ( p, BC_SET32_RES_EAX ); break;
    case IA32_ECX: add_word ( p, BC_SET32_RES_ECX ); break;
    case IA32_EDX: add_word ( p, BC_SET32_RES_EDX ); break;
    case IA32_EBX: add_word ( p, BC_SET32_RES_EBX ); break;
    case IA32_ESP: add_word ( p, BC_SET32_RES_ESP ); break;
    case IA32_EBP: add_word ( p, BC_SET32_RES_EBP ); break;
    case IA32_ESI: add_word ( p, BC_SET32_RES_ESI ); break;
    case IA32_EDI: add_word ( p, BC_SET32_RES_EDI ); break;
      */
    default: // Intenta operacions de memòria.
      ret= compile_get_addr ( e, p, 0 );
      if ( ret )
        {
          switch ( e->inst.ops[0].data_seg )
            {
            case IA32_DS: add_word ( p, BC_FPU_DS_FLOATU32_WRITE32 ); break;
            case IA32_SS: add_word ( p, BC_FPU_SS_FLOATU32_WRITE32 ); break;
            case IA32_ES: add_word ( p, BC_FPU_ES_FLOATU32_WRITE32 ); break;
              
            default:
              fprintf ( FERROR, "jit - compile_fpu_write_float: segment"
                        " desconegut %d en operador 0",
                        e->inst.ops[0].data_seg );
              exit ( EXIT_FAILURE );
            }
        }
    }

  return ret;
  
} // end compile_fpu_write_float


static bool
compile_fpu_write_double (
                          const IA32_JIT_DisEntry *e,
                          IA32_JIT_Page           *p
                          )
{

  bool ret;
  
  
  ret= compile_get_addr ( e, p, 0 );
  if ( ret )
    {
      switch ( e->inst.ops[0].data_seg )
        {
        case IA32_DS: add_word ( p, BC_FPU_DS_DOUBLEU64_WRITE ); break;
        case IA32_SS: add_word ( p, BC_FPU_SS_DOUBLEU64_WRITE ); break;

        default:
          fprintf ( FERROR, "jit - compile_fpu_write_double: segment"
                    " desconegut %d en operador 0",
                    e->inst.ops[0].data_seg );
          exit ( EXIT_FAILURE );
        }
    }
  
  return ret;
  
} // end compile_fpu_write_double


static bool
compile_fpu_write_ldouble (
                           const IA32_JIT_DisEntry *e,
                           IA32_JIT_Page           *p
                           )
{

  bool ret;
  
  
  ret= compile_get_addr ( e, p, 0 );
  if ( ret )
    {
      switch ( e->inst.ops[0].data_seg )
        {
        case IA32_DS: add_word ( p, BC_FPU_DS_LDOUBLEU80_WRITE ); break;
        case IA32_SS: add_word ( p, BC_FPU_SS_LDOUBLEU80_WRITE ); break;
          
        default:
          fprintf ( FERROR, "jit - compile_fpu_write_ldouble: segment"
                    " desconegut %d en operador 0",
                    e->inst.ops[0].data_seg );
          exit ( EXIT_FAILURE );
        }
    }
  
  return ret;
  
} // end compile_fpu_write_ldouble


static void
compile_fpu_store_reg16 (
                         const IA32_JIT_DisEntry *e,
                         IA32_JIT_Page           *p,
                         const bool               check
                         )
{

  add_word ( p, check ? BC_FPU_BEGIN_OP : BC_FPU_BEGIN_OP_NOCHECK );
  // L'operador 0 ha de ser una adreça
  if ( e->inst.ops[0].type != IA32_AX && // Pot ser AX
       !compile_get_addr ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fpu_store_reg16: Operador 0 desconegut"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  switch ( e->inst.name )
    {
    case IA32_FSTCW: add_word ( p, BC_SET16_FPUCONTROL_RES ); break;
    case IA32_FSTSW:
    case IA32_FNSTSW:
      add_word ( p, BC_SET16_FPUSTATUSTOP_RES );
      break;
    default:
      fprintf ( FERROR,
                "[EE] jit.c - compile_fpu_store_reg16: Instrucció desconeguda"
                " %d\n", e->inst.name );
      exit ( EXIT_FAILURE );
    }
  write_res16 ( e, p, 0 );
  update_eip ( e, p, true );
  
} // end compile_fpu_store_reg16


static void
compile_fpu_addr (
                  const IA32_JIT_DisEntry *e,
                  IA32_JIT_Page           *p,
                  const bool               check
                  )
{

  add_word ( p, check ? BC_FPU_BEGIN_OP : BC_FPU_BEGIN_OP_NOCHECK );
  // L'operador 0 ha de ser una adreça
  if ( !compile_get_addr ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fpu_addr: Operador 0 desconegut"
                " %d\n", e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  switch ( e->inst.name )
    {
    case IA32_FRSTOR32:
      switch ( e->inst.ops[0].data_seg )
        {
        case IA32_DS: add_word ( p, BC_FPU_RSTOR32_DS ); break;
        default:
          fprintf ( FERROR, "[EE] jit - compile_fpu_addr - frstor32: segment"
                    " desconegut %d en operador %d",
                    e->inst.ops[0].data_seg, 0 );
          exit ( EXIT_FAILURE );
        }
      break;
      break;
    case IA32_FSAVE32:
      switch ( e->inst.ops[0].data_seg )
        {
        case IA32_DS: add_word ( p, BC_FPU_SAVE32_DS ); break;
        default:
          fprintf ( FERROR, "[EE] jit - compile_fpu_addr - fsave32: segment"
                    " desconegut %d en operador %d",
                    e->inst.ops[0].data_seg, 0 );
          exit ( EXIT_FAILURE );
        }
      break;
    default:
      fprintf ( FERROR,
                "[EE] jit.c - compile_fpu_addr: Instrucció desconeguda"
                " %d\n", e->inst.name );
      exit ( EXIT_FAILURE );
    }
  update_eip ( e, p, true );
  
} // end compile_fpu_addr


static void
compile_fpu_op_st0 (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const uint16_t           op_word
                    )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  add_word ( p, BC_FPU_SELECT_ST0 );
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_fpu_op_st0


static void
compile_fpu_op_st0_st1 (
                        const IA32_JIT_DisEntry *e,
                        IA32_JIT_Page           *p,
                        const uint16_t           op_word,
                        const bool               pop
                        )
{
  
  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  add_word ( p, BC_FPU_SELECT_ST0 );
  add_word ( p, BC_FPU_SELECT_ST1 );
  add_word ( p, op_word );
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fpu_op_st0_st1


static void
compile_fpu_op_float (
                      const IA32_JIT_DisEntry *e,
                      IA32_JIT_Page           *p,
                      const uint16_t           op_word,
                      const bool               pop
                      )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  compile_fpu_read_float ( e, p );
  switch ( op_word )
    {
    case BC_FPU_COM:
      add_word ( p, BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_FCOM );
      break;
    case BC_FPU_DIVR:
      add_word ( p, BC_FPU_FLOATU32_LDOUBLE_AND_CHECK_DENORMAL_SNAN );
      break;
    default:
      add_word ( p, BC_FPU_FLOATU32_LDOUBLE_AND_CHECK );
    }
  add_word ( p, BC_FPU_SELECT_ST0 );
  add_word ( p, op_word );
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fpu_op_float


static void
compile_fpu_op_double (
                       const IA32_JIT_DisEntry *e,
                       IA32_JIT_Page           *p,
                       const uint16_t           op_word,
                       const bool               pop
                       )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  compile_fpu_read_double ( e, p );
  switch ( op_word )
    {
    case BC_FPU_COM:
      add_word ( p, BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_FCOM );
      break;
    case BC_FPU_DIVR:
      add_word ( p, BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK_DENORMAL_SNAN );
      break;
    default:
      add_word ( p, BC_FPU_DOUBLEU64_LDOUBLE_AND_CHECK );
    }
  add_word ( p, BC_FPU_SELECT_ST0 );
  add_word ( p, op_word );
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fpu_op_double


static void
compile_fpu_op_reg (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const uint16_t           op_word,
                    const int                num_pop,
                    const bool               reverse
                    )
{

  int i;


  // Comú
  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  
  // Arguments
  if ( e->inst.ops[1].type != IA32_NONE )
    {

      if ( e->inst.ops[0].type == IA32_FPU_STACK_POS &&
           e->inst.ops[1].type == IA32_FPU_STACK_POS )
        {
          add_word ( p, BC_FPU_SELECT_ST_REG );
          add_word ( p, e->inst.ops[0].fpu_stack_pos );
          add_word ( p, BC_FPU_SELECT_ST_REG2 );
          add_word ( p, e->inst.ops[1].fpu_stack_pos );
        }
      else
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_fpu_op_reg: Operadors"
                    " desconeguts: %d %d\n",
                    e->inst.ops[0].type, e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
    }
  else // Quan sols té un operador
    {
      if ( !reverse )
        {
          add_word ( p, BC_FPU_SELECT_ST0 );
          add_word ( p, BC_FPU_SELECT_ST_REG2 );
          if ( e->inst.ops[0].type == IA32_FPU_STACK_POS )
            add_word ( p, e->inst.ops[0].fpu_stack_pos );
          else if ( e->inst.ops[0].type == IA32_NONE )
            add_word ( p, 1 );
          else
            {
              fprintf ( FERROR,
                        "[EE] jit.c - compile_fpu_op_reg: Operador 0"
                        " desconegut: %d\n",
                        e->inst.ops[0].type );
              exit ( EXIT_FAILURE );
            }
        }
      else
        {
          if ( e->inst.ops[0].type == IA32_FPU_STACK_POS )
            {
              add_word ( p, BC_FPU_SELECT_ST_REG );
              add_word ( p, e->inst.ops[0].fpu_stack_pos );
            }
          else if ( e->inst.ops[0].type == IA32_NONE )
            {
              add_word ( p, BC_FPU_SELECT_ST_REG );
              add_word ( p, 1 );
            }
          else
            {
              fprintf ( FERROR,
                        "[EE] jit.c - compile_fpu_op_reg (reverse): Operador 0"
                        " desconegut: %d\n",
                        e->inst.ops[0].type );
              exit ( EXIT_FAILURE );
            }
          add_word ( p, BC_FPU_SELECT_ST_REG2 );
          add_word ( p, 0 );
        }
    }
  
  // Operació i pop
  switch ( op_word )
    {
    case BC_FPU_COM:
      add_word ( p, BC_FPU_REG2_LDOUBLE_AND_CHECK_FCOM );
      break;
    default: // La part del check no és 100% igual a l'intèrpret
      add_word ( p, BC_FPU_REG2_LDOUBLE_AND_CHECK );
    }
  add_word ( p, op_word );
  for ( i= 0; i < num_pop; ++i )
    add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fpu_op_reg


static void
compile_fpu_op_int32 (
                      const IA32_JIT_DisEntry *e,
                      IA32_JIT_Page           *p,
                      const uint16_t           op_word
                      )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  if ( !compile_get_rm32_op ( e, p, 0, true, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit. compile_fpu_op_int32: Operador"
                " 0 desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_SET32_SRES_LDOUBLE );
  add_word ( p, BC_FPU_SELECT_ST0 );
  add_word ( p, op_word );
  update_eip ( e, p, true );
  
} // end compile_fpu_op_int32


static void
compile_fld_float (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p
                   )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  compile_fpu_read_float ( e, p );
  add_word ( p, BC_FPU_PUSH_FLOATU32 );
  update_eip ( e, p, true );
  
} // end compile_fld_float


static void
compile_fld_double (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p
                    )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  compile_fpu_read_double ( e, p );
  add_word ( p, BC_FPU_PUSH_DOUBLEU64 );
  update_eip ( e, p, true );
  
} // end compile_fld_double


static void
compile_fld_ldouble (
                     const IA32_JIT_DisEntry *e,
                     IA32_JIT_Page           *p
                     )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  if ( e->inst.ops[0].type == IA32_FPU_STACK_POS )
    {
      add_word ( p, BC_FPU_SELECT_ST_REG2 );
      add_word ( p, e->inst.ops[0].fpu_stack_pos );
      add_word ( p, BC_FPU_PUSH_REG2 );
    }
  else
    {
      compile_fpu_read_ldouble ( e, p );
      add_word ( p, BC_FPU_PUSH_LDOUBLEU80 );
    }
  update_eip ( e, p, true );
  
} // end compile_fld_ldouble


static void
compile_fild_int16 (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p
                    )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  if ( !compile_get_rm16_op ( e, p, 0, true ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_get_rm16_op: Operador 0"
                " desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_FPU_PUSH_RES16 );
  update_eip ( e, p, true );
  
} // end compile_fild_int16


static void
compile_fild_int32 (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p
                    )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  if ( !compile_get_rm32_op ( e, p, 0, true, false ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fild_int32: Operador 0"
                " desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_FPU_PUSH_RES32 );
  update_eip ( e, p, true );
  
} // end compile_fild_int32


static void
compile_fild_int64 (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p
                    )
{
  
  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  compile_fpu_read_double ( e, p );
  add_word ( p, BC_FPU_PUSH_DOUBLEU64_AS_INT64 );
  update_eip ( e, p, true );
  
} // end compile_fild_int64


static void
compile_fldcw (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{

  if ( !compile_get_rm16_op ( e, p, 0, true ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fldcw 16bit: Operador 0"
                " desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_FPU_RES16_CONTROL_WORD );
  update_eip ( e, p, true );
  
} // end compile_fldcw


static void
compile_fist_int16 (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               pop
                    )
{
  
  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  add_word ( p, BC_FPU_SELECT_ST0 );
  if ( e->inst.ops[0].type != IA32_FPU_STACK_POS )
    {
      add_word ( p, BC_FPU_REG_INT16 );
      if ( !write_rm16_op ( e, p, 0 ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_fist_int16: Operador 0"
                    " desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fist_int16: Operador 0"
                " desconegut (B): %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
      /*
      add_word ( p, BC_FPU_REG_ST );
      add_word ( p, e->inst.ops[0].fpu_stack_pos );
      */
    }
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fist_int16


static void
compile_fist_int32 (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               pop
                    )
{
  
  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  add_word ( p, BC_FPU_SELECT_ST0 );
  if ( e->inst.ops[0].type != IA32_FPU_STACK_POS )
    {
      add_word ( p, BC_FPU_REG_INT32 );
      if ( !compile_fpu_write_float ( e, p ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_fist_int32: Operador 0"
                    " desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fist_int32: Operador 0"
                " desconegut (B): %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
      /*
      add_word ( p, BC_FPU_REG_ST );
      add_word ( p, e->inst.ops[0].fpu_stack_pos );
      */
    }
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fist_int32


static void
compile_fist_int64 (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               pop
                    )
{
  
  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  add_word ( p, BC_FPU_SELECT_ST0 );
  if ( e->inst.ops[0].type != IA32_FPU_STACK_POS )
    {
      add_word ( p, BC_FPU_REG_INT64 );
      if ( !compile_fpu_write_double ( e, p ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_fist_int64: Operador 0"
                    " desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fist_int64: Operador 0"
                " desconegut (B): %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
      /*
      add_word ( p, BC_FPU_REG_ST );
      add_word ( p, e->inst.ops[0].fpu_stack_pos );
      */
    }
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fist_int64


static void
compile_fst_float (
                   const IA32_JIT_DisEntry *e,
                   IA32_JIT_Page           *p,
                   const bool               pop
                   )
{
  
  add_word ( p, BC_FPU_BEGIN_OP );
  add_word ( p, BC_FPU_SELECT_ST0 );
  add_word ( p, BC_FPU_REG_FLOATU32 );
  if ( !compile_fpu_write_float ( e, p ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fst_float: Operador 0"
                " desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fst_float


static void
compile_fst_double (
                    const IA32_JIT_DisEntry *e,
                    IA32_JIT_Page           *p,
                    const bool               pop
                   )
{

  add_word ( p, BC_FPU_BEGIN_OP );
  add_word ( p, BC_FPU_SELECT_ST0 );
  add_word ( p, BC_FPU_REG_DOUBLEU64 );
  if ( !compile_fpu_write_double ( e, p ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fst_double: Operador 0"
                " desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fst_double


static void
compile_fst_ldouble (
                     const IA32_JIT_DisEntry *e,
                     IA32_JIT_Page           *p,
                     const bool               pop
                     )
{
  
  add_word ( p, BC_FPU_BEGIN_OP );
  add_word ( p, BC_FPU_SELECT_ST0 );
  if ( e->inst.ops[0].type != IA32_FPU_STACK_POS )
    {
      add_word ( p, BC_FPU_REG_LDOUBLEU80 );
      if ( !compile_fpu_write_ldouble ( e, p ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - compile_fstp_ldouble: Operador 0"
                    " desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
    }
  else
    {
      add_word ( p, BC_FPU_REG_ST );
      add_word ( p, e->inst.ops[0].fpu_stack_pos );
    }
  if ( pop ) add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fst_ldouble


static void
compile_fbstp (
               const IA32_JIT_DisEntry *e,
               IA32_JIT_Page           *p
               )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  add_word ( p, BC_FPU_SELECT_ST0 );
  if ( !compile_get_addr ( e, p, 0 ) )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fbstp: Operador 0"
                " desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  switch ( e->inst.ops[0].data_seg )
    {
    case IA32_SS: add_word ( p, BC_FPU_BSTP_SS ); break;
      /*
    case IA32_ES: add_word ( p, BC_ES_RES_WRITE32 ); break;
    case IA32_CS: add_word ( p, BC_CS_RES_WRITE32 ); break;
    case IA32_FS: add_word ( p, BC_FS_RES_WRITE32 ); break;
    case IA32_GS: add_word ( p, BC_GS_RES_WRITE32 ); break;
      */
    default:
      fprintf ( FERROR, "jit - compile_fbstp: segment"
                " desconegut %d en operador 0",
                e->inst.ops[0].data_seg );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_FPU_POP );
  update_eip ( e, p, true );
  
} // end compile_fbstp


static void
compile_fxch (
              const IA32_JIT_DisEntry *e,
              IA32_JIT_Page           *p
              )
{

  add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
  add_word ( p, BC_FPU_SELECT_ST0 );
  if ( e->inst.ops[0].type != IA32_FPU_STACK_POS )
    {
      fprintf ( FERROR,
                "[EE] jit.c - compile_fxch: Operador 0"
                " desconegut: %d\n",
                e->inst.ops[0].type );
      exit ( EXIT_FAILURE );
    }
  add_word ( p, BC_FPU_SELECT_ST_REG2 );
  add_word ( p, e->inst.ops[0].fpu_stack_pos );
  add_word ( p, BC_FPU_XCH );
  update_eip ( e, p, true );
  
} // end compile_fxch


static void
compile (
         const IA32_JIT          *jit,
         const IA32_JIT_DisEntry *e,
         IA32_JIT_Page           *p
         )
{

  switch ( e->inst.name )
    {
    case IA32_AAM: compile_aam_like ( e, p, BC_AAM ); break;
    case IA32_ADC32: compile_add32_like ( e, p, false, true, true ); break;
    case IA32_ADC16: compile_add16_like ( e, p, false, true, true ); break;
    case IA32_ADC8: compile_add8_like ( e, p, false, true, true ); break;
    case IA32_ADD32: compile_add32_like ( e, p, false, true, false ); break;
    case IA32_ADD16: compile_add16_like ( e, p, false, true, false ); break;
    case IA32_ADD8: compile_add8_like ( e, p, false, true, false ); break;
    case IA32_AND32: compile_lop32 ( e, p, BC_AND32, true ); break;
    case IA32_AND16: compile_lop16 ( e, p, BC_AND16, true ); break;
    case IA32_AND8: compile_lop8 ( e, p, BC_AND8, true ); break;
    case IA32_BSF32:
      if ( e->inst.prefix == IA32_PREFIX_REPE )
        WW ( UDATA, "TZCNT not supported" );
      compile_inst_rm32_noflags_read_write ( e, p, BC_BSF32, 1 );
      break;
    case IA32_BSF16:
      if ( e->inst.prefix == IA32_PREFIX_REPE )
        WW ( UDATA, "TZCNT not supported" );
      compile_inst_rm16_noflags_read_write ( e, p, BC_BSF16, 1 );
      break;
    case IA32_BSR32:
      // NOTA!! Quan el valor és 0 el comportament és diferent entre
      // l'intèrpret i el jit. En l'intèrpret no cànvia el destí en el
      // jit sí, i el valor és random.
      if ( e->inst.prefix == IA32_PREFIX_REPE )
        WW ( UDATA, "LZCNT not supported" );
      compile_inst_rm32_noflags_read_write ( e, p, BC_BSR32, 1 );
      break;
    case IA32_BSR16:
      if ( e->inst.prefix == IA32_PREFIX_REPE )
        WW ( UDATA, "LZCNT not supported" );
      compile_inst_rm16_noflags_read_write ( e, p, BC_BSR16, 1 );
      break;
    case IA32_BSWAP:
      compile_inst_rm32_noflags_read_write ( e, p, BC_BSWAP, 0 );
      break;
    case IA32_BT32: compile_bt32_like ( e, p, BC_BT32, false ); break;
    case IA32_BT16: compile_bt16_like ( e, p, BC_BT16, false ); break;
    case IA32_BTC32: compile_bt32_like ( e, p, BC_BTC32, true ); break;
    case IA32_BTR32: compile_bt32_like ( e, p, BC_BTR32, true ); break;
    case IA32_BTR16: compile_bt16_like ( e, p, BC_BTR16, true ); break;
    case IA32_BTS32: compile_bt32_like ( e, p, BC_BTS32, true ); break;
    case IA32_BTS16: compile_bt16_like ( e, p, BC_BTS16, true ); break;
    case IA32_CALL32_FAR: compile_jmp_far_like ( e, p, true, true ); break;
    case IA32_CALL16_FAR: compile_jmp_far_like ( e, p, true, false ); break;
    case IA32_CALL32_NEAR: compile_jmp_near_like ( e, p, true, true ); break;
    case IA32_CALL16_NEAR: compile_jmp_near_like ( e, p, false, true ); break;
    case IA32_CBW:
      add_word ( p, BC_SET8_AL_RES );
      add_word ( p, BC_SET16_RES8_RES_SIGNED );
      add_word ( p, BC_SET16_RES_AX );
      update_eip ( e, p, true );
      break;
    case IA32_CDQ: add_word ( p, BC_CDQ ); update_eip ( e, p, true ); break;
    case IA32_CLC:
      add_word ( p, BC_CLEAR_CF_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_CLD:
      add_word ( p, BC_CLEAR_DF_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_CLI: add_word ( p, BC_CLI ); update_eip ( e, p, true ); break;
    case IA32_CLTS:
      add_word ( p, BC_CLEAR_TS_CR0 );
      update_eip ( e, p, true );
      break;
    case IA32_CMC: add_word ( p, BC_CMC ); update_eip ( e, p, true ); break;
    case IA32_CMP32:
      compile_sub32_like ( e, p, false, false, true, false, false );
      break;
    case IA32_CMP16:
      compile_sub16_like ( e, p, false, false, true, false, false );
      break;
    case IA32_CMP8:
      compile_sub8_like ( e, p, false, false, true, false, false );
      break;
    case IA32_CMPS32:
    case IA32_CMPS16:
    case IA32_CMPS8: compile_cmps ( e, p ); break;
    case IA32_CPUID: add_word ( p, BC_CPUID ); update_eip ( e, p, true ); break;
    case IA32_CWD: add_word ( p, BC_CWD ); update_eip ( e, p, true ); break;
    case IA32_CWDE: add_word ( p, BC_CWDE ); update_eip ( e, p, true ); break;
    case IA32_DAA:
      add_word ( p, BC_DAA );
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
      if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_ZF_EFLAGS );
      if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_DAS:
      add_word ( p, BC_DAS );
      if ( e->flags&SF_FLAG ) add_word ( p, BC_SET8_SF_EFLAGS );
      if ( e->flags&ZF_FLAG ) add_word ( p, BC_SET8_ZF_EFLAGS );
      if ( e->flags&PF_FLAG ) add_word ( p, BC_SET8_PF_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_ENTER32: compile_inst_imm16_imm8 ( e, p, BC_ENTER32 ); break;
    case IA32_ENTER16: compile_inst_imm16_imm8 ( e, p, BC_ENTER16 ); break;
    case IA32_DEC32:
      compile_sub32_like ( e, p, true, true, false, false, false );
      break;
    case IA32_DEC16:
      compile_sub16_like ( e, p, true, true, false, false, false );
      break;
    case IA32_DEC8:
      compile_sub8_like ( e, p, true, true, false, false, false );
      break;
    case IA32_DIV32:
      compile_inst_rm32_noflags_read ( e, p, BC_DIV32_EDX_EAX );
      break;
    case IA32_DIV16:
      compile_inst_rm16_noflags_read ( e, p, BC_DIV16_DX_AX );
      break;
    case IA32_DIV8:
      compile_inst_rm8_noflags_read ( e, p, BC_DIV8_AH_AL );
      break;
    case IA32_F2XM1: compile_fpu_op_st0 ( e, p, BC_FPU_2XM1 ); break;
    case IA32_FABS:
      add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
      add_word ( p, BC_FPU_SELECT_ST0 );
      add_word ( p, BC_FPU_ABS );
      update_eip ( e, p, true );
      break;
    case IA32_FADD32: compile_fpu_op_float ( e, p, BC_FPU_ADD, false ); break;
    case IA32_FADD64: compile_fpu_op_double ( e, p, BC_FPU_ADD, false ); break;
    case IA32_FADD80: compile_fpu_op_reg ( e, p, BC_FPU_ADD, 0, false ); break;
    case IA32_FADDP80: compile_fpu_op_reg ( e, p, BC_FPU_ADD, 1, true ); break;
    case IA32_FBSTP: compile_fbstp ( e, p ); break;
    case IA32_FCHS:
      add_word ( p, BC_FPU_CHS );
      update_eip ( e, p, true );
      break;
    case IA32_FCLEX:
      add_word ( p, BC_FPU_CLEAR_EXCP );
      update_eip ( e, p, true );
      break;
    case IA32_FCOM32: compile_fpu_op_float ( e, p, BC_FPU_COM, false ); break;
    case IA32_FCOM64: compile_fpu_op_double ( e, p, BC_FPU_COM, false ); break;
    case IA32_FCOMP32: compile_fpu_op_float ( e, p, BC_FPU_COM, true ); break;
    case IA32_FCOMP64: compile_fpu_op_double ( e, p, BC_FPU_COM, true ); break;
    case IA32_FCOMP80: compile_fpu_op_reg ( e, p, BC_FPU_COM, 1, false ); break;
    case IA32_FCOMPP: compile_fpu_op_reg ( e, p, BC_FPU_COM, 2, false ); break;
    case IA32_FCOS: compile_fpu_op_st0 ( e, p, BC_FPU_COS ); break;
    case IA32_FDIV32: compile_fpu_op_float ( e, p, BC_FPU_DIV, false ); break;
    case IA32_FDIV64: compile_fpu_op_double ( e, p, BC_FPU_DIV, false ); break;
    case IA32_FDIV80: compile_fpu_op_reg ( e, p, BC_FPU_DIV, 0, false ); break;
    case IA32_FDIVP80: compile_fpu_op_reg ( e, p, BC_FPU_DIV, 1, true ); break;
    case IA32_FDIVR32: compile_fpu_op_float ( e, p, BC_FPU_DIVR, false ); break;
    case IA32_FDIVR64:
      compile_fpu_op_double ( e, p, BC_FPU_DIVR, false );
      break;
    case IA32_FDIVR80:
      compile_fpu_op_reg ( e, p, BC_FPU_DIVR, 0, false );
      break;
    case IA32_FDIVRP80:
      compile_fpu_op_reg ( e, p, BC_FPU_DIVR, 1, true );
      break;
    case IA32_FFREE:
      add_word ( p, BC_FPU_BEGIN_OP );
      add_word ( p, BC_FPU_FREE );
      add_word ( p, e->inst.ops[0].fpu_stack_pos );
      update_eip ( e, p, true );
      break;
    case IA32_FILD16: compile_fild_int16 ( e, p ); break;
    case IA32_FILD32: compile_fild_int32 ( e, p ); break;
    case IA32_FILD64: compile_fild_int64 ( e, p ); break;
    case IA32_FIMUL32: compile_fpu_op_int32 ( e, p, BC_FPU_MUL ); break;
    case IA32_FINIT:
      add_word ( p, BC_FPU_INIT );
      update_eip ( e, p, true );
      break;
    case IA32_FIST32: compile_fist_int32 ( e, p, false ); break;
    case IA32_FISTP16: compile_fist_int16 ( e, p, true ); break;
    case IA32_FISTP32: compile_fist_int32 ( e, p, true ); break;
    case IA32_FISTP64: compile_fist_int64 ( e, p, true ); break;
    case IA32_FLD1:
      add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
      add_word ( p, BC_FPU_PUSH_ONE );
      update_eip ( e, p, true );
      break;
    case IA32_FLDL2E:
      add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
      add_word ( p, BC_FPU_PUSH_L2E );
      update_eip ( e, p, true );
      break;
    case IA32_FLD32: compile_fld_float ( e, p ); break;
    case IA32_FLD64: compile_fld_double ( e, p ); break;
    case IA32_FLD80: compile_fld_ldouble ( e, p ); break;
    case IA32_FLDCW: compile_fldcw ( e, p ); break;
    case IA32_FLDLN2:
      add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
      add_word ( p, BC_FPU_PUSH_LN2 );
      update_eip ( e, p, true );
      break;
    case IA32_FLDZ:
      add_word ( p, BC_FPU_BEGIN_OP_CLEAR_C1 );
      add_word ( p, BC_FPU_PUSH_ZERO );
      update_eip ( e, p, true );
      break;
    case IA32_FMUL32: compile_fpu_op_float ( e, p, BC_FPU_MUL, false ); break;
    case IA32_FMUL64: compile_fpu_op_double ( e, p, BC_FPU_MUL, false ); break;
    case IA32_FMUL80: compile_fpu_op_reg ( e, p, BC_FPU_MUL, 0, false ); break;
    case IA32_FMULP80: compile_fpu_op_reg ( e, p, BC_FPU_MUL, 1, true ); break;
    case IA32_FNSTSW: compile_fpu_store_reg16 ( e, p, false ); break;
    case IA32_FPATAN:
      compile_fpu_op_st0_st1 ( e, p, BC_FPU_PATAN, true );
      break;
    case IA32_FPREM:
      compile_fpu_op_st0_st1 ( e, p, BC_FPU_PREM, false );
      break;
    case IA32_FPTAN: compile_fpu_op_st0 ( e, p, BC_FPU_PTAN ); break;
    case IA32_FRNDINT: compile_fpu_op_st0 ( e, p, BC_FPU_RNDINT ); break;
    case IA32_FRSTOR32: compile_fpu_addr ( e, p, true ); break;
    case IA32_FSAVE32: compile_fpu_addr ( e, p, false ); break;
    case IA32_FSCALE:
      compile_fpu_op_st0_st1 ( e, p, BC_FPU_SCALE, false );
      break;
    case IA32_FSETPM: update_eip ( e, p, true ); break;
    case IA32_FSIN: compile_fpu_op_st0 ( e, p, BC_FPU_SIN ); break;
    case IA32_FSQRT: compile_fpu_op_st0 ( e, p, BC_FPU_SQRT ); break;
    case IA32_FST32: compile_fst_float ( e, p, false ); break;
    case IA32_FST64: compile_fst_double ( e, p, false ); break;
    case IA32_FST80: compile_fst_ldouble ( e, p, false ); break;
    case IA32_FSTCW: compile_fpu_store_reg16 ( e, p, true ); break;
    case IA32_FSTP32: compile_fst_float ( e, p, true ); break;
    case IA32_FSTP64: compile_fst_double ( e, p, true ); break;
    case IA32_FSTP80: compile_fst_ldouble ( e, p, true ); break;
    case IA32_FSTSW: compile_fpu_store_reg16 ( e, p, true ); break;
    case IA32_FSUB32: compile_fpu_op_float ( e, p, BC_FPU_SUB, false ); break;
    case IA32_FSUB64: compile_fpu_op_double ( e, p, BC_FPU_SUB, false ); break;
    case IA32_FSUB80: compile_fpu_op_reg ( e, p, BC_FPU_SUB, 0, false ); break;
    case IA32_FSUBP80: compile_fpu_op_reg ( e, p, BC_FPU_SUB, 1, true ); break;
    case IA32_FSUBR32: compile_fpu_op_float ( e, p, BC_FPU_SUBR, false ); break;
    case IA32_FSUBR64:
      compile_fpu_op_double ( e, p, BC_FPU_SUBR, false );
      break;
    case IA32_FSUBRP80: compile_fpu_op_reg ( e, p, BC_FPU_SUBR, 1, true );break;
    case IA32_FTST: compile_fpu_op_st0 ( e, p, BC_FPU_TST ); break;
    case IA32_FWAIT:
      add_word ( p, BC_FPU_WAIT );
      update_eip ( e, p, true );
      break;
    case IA32_FXAM:
      add_word ( p, BC_FPU_BEGIN_OP );
      add_word ( p, BC_FPU_XAM );
      update_eip ( e, p, true );
      break;
    case IA32_FXCH: compile_fxch ( e, p ); break;
    case IA32_FYL2X:
      compile_fpu_op_st0_st1 ( e, p, BC_FPU_YL2X, true );
      break;
    case IA32_HLT:
      update_eip ( e, p, false ); // <-- No va a la EIP
      add_word ( p, BC_HALT );
      add_word ( p, (uint16_t) e->inst.real_nbytes );
      break;
    case IA32_IDIV32:
      compile_inst_rm32_noflags_read ( e, p, BC_IDIV32_EDX_EAX );
      break;
    case IA32_IDIV16:
      compile_inst_rm16_noflags_read ( e, p, BC_IDIV16_DX_AX );
      break;
    case IA32_IDIV8:
      compile_inst_rm8_noflags_read ( e, p, BC_IDIV8_AH_AL );
      break;
    case IA32_IMUL32: compile_mul32_like ( e, p, true ); break;
    case IA32_IMUL16: compile_mul16_like ( e, p, true ); break;
    case IA32_IMUL8: compile_mul8_like ( e, p, true ); break;
    case IA32_IN: compile_in ( e, p ); break;
      // NOTA!! No cal distingir entre JMP32_FAR i JMP16_FAR perquè el
      // offset ja és correcte sempre.
    case IA32_INC32: compile_add32_like ( e, p, true, false, false ); break;
    case IA32_INC16: compile_add16_like ( e, p, true, false, false ); break;
    case IA32_INC8: compile_add8_like ( e, p, true, false, false ); break;
    case IA32_INS32: compile_ins ( e, p ); break;
    case IA32_INS16: compile_ins ( e, p ); break;
    case IA32_INS8: compile_ins ( e, p ); break;
    case IA32_INT32: compile_int ( e, p, true ); break;
    case IA32_INT16: compile_int ( e, p, false ); break;
    case IA32_INTO32: compile_into ( e, p, true ); break;
    case IA32_INTO16: compile_into ( e, p, false ); break;
    case IA32_INVLPG32: compile_invlpg_like ( e, p, BC_INVLPG32 ); break;
    case IA32_INVLPG16: compile_invlpg_like ( e, p, BC_INVLPG16 ); break;
    case IA32_IRET32:
      add_word ( p, BC_IRET32 );
      add_word ( p, e->inst.real_nbytes );
      break;
    case IA32_IRET16:
      add_word ( p, BC_IRET16 );
      add_word ( p, e->inst.real_nbytes );
      break;
    case IA32_JA32: compile_jcc ( e, p, BC_SET_A_COND, true ); break;
    case IA32_JA16: compile_jcc ( e, p, BC_SET_A_COND, false ); break;
    case IA32_JAE32: compile_jcc ( e, p, BC_SET_AE_COND, true ); break;
    case IA32_JAE16: compile_jcc ( e, p, BC_SET_AE_COND, false ); break;
    case IA32_JB32: compile_jcc ( e, p, BC_SET_B_COND, true ); break;
    case IA32_JB16: compile_jcc ( e, p, BC_SET_B_COND, false ); break;
    case IA32_JCXZ32: compile_jcc ( e, p, BC_SET_CXZ_COND, true ); break;
    case IA32_JCXZ16: compile_jcc ( e, p, BC_SET_CXZ_COND, false ); break;
    case IA32_JE32: compile_jcc ( e, p, BC_SET_E_COND, true ); break;
    case IA32_JE16: compile_jcc ( e, p, BC_SET_E_COND, false ); break;
    case IA32_JECXZ32: compile_jcc ( e, p, BC_SET_ECXZ_COND, true ); break;
    case IA32_JECXZ16: compile_jcc ( e, p, BC_SET_ECXZ_COND, false ); break;
    case IA32_JG32: compile_jcc ( e, p, BC_SET_G_COND, true ); break;
    case IA32_JG16: compile_jcc ( e, p, BC_SET_G_COND, false ); break;
    case IA32_JGE32: compile_jcc ( e, p, BC_SET_GE_COND, true ); break;
    case IA32_JGE16: compile_jcc ( e, p, BC_SET_GE_COND, false ); break;
    case IA32_JL32: compile_jcc ( e, p, BC_SET_L_COND, true ); break;
    case IA32_JL16: compile_jcc ( e, p, BC_SET_L_COND, false ); break;
    case IA32_JMP32_FAR: compile_jmp_far_like ( e, p, false, true ); break;
    case IA32_JMP16_FAR: compile_jmp_far_like ( e, p, false, false ); break;
    case IA32_JMP32_NEAR: compile_jmp_near_like ( e, p, true, false ); break;
    case IA32_JMP16_NEAR: compile_jmp_near_like ( e, p, false, false ); break;
    case IA32_JNA32: compile_jcc ( e, p, BC_SET_NA_COND, true ); break;
    case IA32_JNA16: compile_jcc ( e, p, BC_SET_NA_COND, false ); break;
    case IA32_JNE32: compile_jcc ( e, p, BC_SET_NE_COND, true ); break;
    case IA32_JNE16: compile_jcc ( e, p, BC_SET_NE_COND, false ); break;
    case IA32_JNG32: compile_jcc ( e, p, BC_SET_NG_COND, true ); break;
    case IA32_JNG16: compile_jcc ( e, p, BC_SET_NG_COND, false ); break;
    case IA32_JNO32: compile_jcc ( e, p, BC_SET_NO_COND, true ); break;
    case IA32_JNO16: compile_jcc ( e, p, BC_SET_NO_COND, false ); break;
    case IA32_JNS32: compile_jcc ( e, p, BC_SET_NS_COND, true ); break;
    case IA32_JNS16: compile_jcc ( e, p, BC_SET_NS_COND, false ); break;
    case IA32_JO32: compile_jcc ( e, p, BC_SET_O_COND, true ); break;
    case IA32_JO16: compile_jcc ( e, p, BC_SET_O_COND, false ); break;
    case IA32_JP32: compile_jcc ( e, p, BC_SET_P_COND, true ); break;
    case IA32_JP16: compile_jcc ( e, p, BC_SET_P_COND, false ); break;
    case IA32_JPO32: compile_jcc ( e, p, BC_SET_PO_COND, true ); break;
    case IA32_JPO16: compile_jcc ( e, p, BC_SET_PO_COND, false ); break;
    case IA32_JS32: compile_jcc ( e, p, BC_SET_S_COND, true ); break;
    case IA32_JS16: compile_jcc ( e, p, BC_SET_S_COND, false ); break;
    case IA32_LAHF: add_word ( p, BC_LAHF ); update_eip ( e, p, true ); break;
    case IA32_LAR32: compile_lar ( e, p, true ); break;
    case IA32_LAR16: compile_lar ( e, p, false ); break;
    case IA32_LDS32: compile_lsreg ( e, p, IA32_DS, true ); break;
    case IA32_LDS16: compile_lsreg ( e, p, IA32_DS, false ); break;
    case IA32_LES32: compile_lsreg ( e, p, IA32_ES, true ); break;
    case IA32_LES16: compile_lsreg ( e, p, IA32_ES, false ); break;
    case IA32_LFS32: compile_lsreg ( e, p, IA32_FS, true ); break;
    case IA32_LFS16: compile_lsreg ( e, p, IA32_FS, false ); break;
    case IA32_LEA32:
    case IA32_LEA16: compile_lea ( e, p ); break;
    case IA32_LEAVE32:
      add_word ( p, BC_LEAVE32 );
      update_eip ( e, p, true );
      break;
    case IA32_LEAVE16:
      add_word ( p, BC_LEAVE16 );
      update_eip ( e, p, true );
      break;
    case IA32_LGDT32: compile_inst_m1632_read ( e, p, BC_LGDT32 ); break;
    case IA32_LGDT16: compile_inst_m1632_read ( e, p, BC_LGDT16 ); break;
    case IA32_LGS32: compile_lsreg ( e, p, IA32_GS, true ); break;
    case IA32_LGS16: compile_lsreg ( e, p, IA32_GS, false ); break;
    case IA32_LIDT32: compile_inst_m1632_read ( e, p, BC_LIDT32 ); break;
    case IA32_LIDT16: compile_inst_m1632_read ( e, p, BC_LIDT16 ); break;
    case IA32_LLDT:
      if ( !compile_get_rm16_op ( e, p, 0, true ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - lldt: Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_LLDT );
      update_eip ( e, p, true );
      break;
    case IA32_LMSW:
      if ( !compile_get_rm16_op ( e, p, 0, true ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - lmsw: Operador 0 desconegut: %d\n",
                    e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_SET16_RES_CR0 );
      update_eip ( e, p, true );
      break;
    case IA32_LODS32: compile_lods ( e, p ); break;
    case IA32_LODS16: compile_lods ( e, p ); break;
    case IA32_LODS8: compile_lods ( e, p ); break;
    case IA32_LOOP32:
    case IA32_LOOP16:
      if ( e->inst.ops[1].type!=IA32_ECX && e->inst.ops[1].type!=IA32_CX )
        {
          fprintf ( FERROR, "LOOP wrong op1: %d\n", e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      compile_jcc ( e, p, e->inst.ops[1].type==IA32_ECX ?
                    BC_SET_DEC_ECX_NOT_ZERO_COND :
                    BC_SET_DEC_CX_NOT_ZERO_COND,
                    e->inst.name==IA32_LOOP32 );
      break;
    case IA32_LOOPE32:
    case IA32_LOOPE16:
      if ( e->inst.ops[1].type!=IA32_ECX && e->inst.ops[1].type!=IA32_CX )
        {
          fprintf ( FERROR, "LOOPE wrong op1: %d\n", e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      compile_jcc ( e, p, e->inst.ops[1].type==IA32_ECX ?
                    BC_SET_DEC_ECX_NOT_ZERO_AND_ZF_COND :
                    BC_SET_DEC_CX_NOT_ZERO_AND_ZF_COND,
                    e->inst.name==IA32_LOOPE32 );
      break;
    case IA32_LOOPNE32:
    case IA32_LOOPNE16:
      if ( e->inst.ops[1].type!=IA32_ECX && e->inst.ops[1].type!=IA32_CX )
        {
          fprintf ( FERROR, "LOOPNE wrong op1: %d\n", e->inst.ops[1].type );
          exit ( EXIT_FAILURE );
        }
      compile_jcc ( e, p, e->inst.ops[1].type==IA32_ECX ?
                    BC_SET_DEC_ECX_NOT_ZERO_AND_NOT_ZF_COND :
                    BC_SET_DEC_CX_NOT_ZERO_AND_NOT_ZF_COND,
                    e->inst.name==IA32_LOOPNE32 );
      break;
    case IA32_LSL32: compile_lsl ( e, p, true ); break;
    case IA32_LSL16: compile_lsl ( e, p, false ); break;
    case IA32_LSS32: compile_lsreg ( e, p, IA32_SS, true ); break;
    case IA32_LSS16: compile_lsreg ( e, p, IA32_SS, false ); break;
    case IA32_LTR:
      if ( !compile_get_rm16_op ( e, p, 0, true ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - ltr: Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, BC_LTR );
      update_eip ( e, p, true );
      break;
    case IA32_MOV32: compile_mov32 ( e, p ); break;
    case IA32_MOV16: compile_mov16 ( e, p ); break;
    case IA32_MOV8: compile_mov8 ( e, p ); break;
    case IA32_MOVS32: compile_movs ( e, p ); break;
    case IA32_MOVS16: compile_movs ( e, p ); break;
    case IA32_MOVS8: compile_movs ( e, p ); break;
    case IA32_MOVSX32W: compile_movszx32 ( e, p, false, true ); break;
    case IA32_MOVSX32B: compile_movszx32 ( e, p, true, true ); break;
    case IA32_MOVSX16: compile_movszx ( e, p, true ); break;
    case IA32_MOVZX32W: compile_movszx32 ( e, p, false, false ); break;
    case IA32_MOVZX32B: compile_movszx32 ( e, p, true, false ); break;
    case IA32_MOVZX16: compile_movszx ( e, p, false ); break;
    case IA32_MUL32: compile_mul32_like ( e, p, false ); break;
    case IA32_MUL16: compile_mul16_like ( e, p, false ); break;
    case IA32_MUL8: compile_mul8_like ( e, p, false ); break;
    case IA32_NEG32:
      compile_sub32_like ( e, p, true, false, true, false, true );
      break;
    case IA32_NEG16:
      compile_sub16_like ( e, p, true, false, true, false, true );
      break;
    case IA32_NEG8:
      compile_sub8_like ( e, p, true, false, true, false, true );
      break;
    case IA32_NOP:
      if ( e->inst.prefix != IA32_PREFIX_NONE )
        {
          fprintf ( FERROR, "[EE] PAUSE NOT IMPLEMENTED\n");
          exit ( EXIT_FAILURE );
        }
      update_eip ( e, p, true );
      break;
    case IA32_NOT32:
      compile_inst_rm32_noflags_read_write ( e, p, BC_NOT32, 0 );
      break;
    case IA32_NOT16:
      compile_inst_rm16_noflags_read_write ( e, p, BC_NOT16, 0 );
      break;
    case IA32_NOT8:
      compile_inst_rm8_noflags_read_write ( e, p, BC_NOT8 );
      break;
    case IA32_OR32: compile_lop32 ( e, p, BC_OR32, true ); break;
    case IA32_OR16: compile_lop16 ( e, p, BC_OR16, true ); break;
    case IA32_OR8: compile_lop8 ( e, p, BC_OR8, true ); break;
    case IA32_OUT: compile_out ( e, p ); break;
    case IA32_OUTS16: compile_outs ( e, p ); break;
    case IA32_OUTS8: compile_outs ( e, p ); break;
    case IA32_POP32: compile_inst_rm32_noflags_write ( e, p, BC_POP32 ); break;
    case IA32_POP16: compile_inst_rm16_noflags_write ( e, p, BC_POP16 ); break;
    case IA32_POPA32:
      add_word ( p, BC_POP32 );
      add_word ( p, BC_SET32_RES_EDI );
      add_word ( p, BC_POP32 );
      add_word ( p, BC_SET32_RES_ESI );
      add_word ( p, BC_POP32 );
      add_word ( p, BC_SET32_RES_EBP );
      add_word ( p, BC_POP32 ); // <-- Discard ESP
      add_word ( p, BC_POP32 );
      add_word ( p, BC_SET32_RES_EBX );
      add_word ( p, BC_POP32 );
      add_word ( p, BC_SET32_RES_EDX );
      add_word ( p, BC_POP32 );
      add_word ( p, BC_SET32_RES_ECX );
      add_word ( p, BC_POP32 );
      add_word ( p, BC_SET32_RES_EAX );
      update_eip ( e, p, true );
      break;
    case IA32_POPA16:
      add_word ( p, BC_POP16 );
      add_word ( p, BC_SET16_RES_DI );
      add_word ( p, BC_POP16 );
      add_word ( p, BC_SET16_RES_SI );
      add_word ( p, BC_POP16 );
      add_word ( p, BC_SET16_RES_BP );
      add_word ( p, BC_POP16 ); // <-- Discard SP
      add_word ( p, BC_POP16 );
      add_word ( p, BC_SET16_RES_BX );
      add_word ( p, BC_POP16 );
      add_word ( p, BC_SET16_RES_DX );
      add_word ( p, BC_POP16 );
      add_word ( p, BC_SET16_RES_CX );
      add_word ( p, BC_POP16 );
      add_word ( p, BC_SET16_RES_AX );
      update_eip ( e, p, true );
      break;
    case IA32_POPF32:
      add_word ( p, BC_POP32_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_POPF16:
      add_word ( p, BC_POP16_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_PUSH32:
      compile_inst_rm32_imm32_noflags_read ( e, p, BC_PUSH32 );
      break;
    case IA32_PUSH16:
      compile_inst_rm16_imm16_noflags_read ( e, p, BC_PUSH16 );
      break;
    case IA32_PUSHA32:
      add_word ( p, BC_PUSHA32_CHECK_EXCEPTION );
      add_word ( p, BC_SET32_ESP_OP0 );
      add_word ( p, BC_SET32_EAX_RES );
      add_word ( p, BC_PUSH32 );
      add_word ( p, BC_SET32_ECX_RES );
      add_word ( p, BC_PUSH32 );
      add_word ( p, BC_SET32_EDX_RES );
      add_word ( p, BC_PUSH32 );
      add_word ( p, BC_SET32_EBX_RES );
      add_word ( p, BC_PUSH32 );
      add_word ( p, BC_SET32_OP0_RES );
      add_word ( p, BC_PUSH32 );
      add_word ( p, BC_SET32_EBP_RES );
      add_word ( p, BC_PUSH32 );
      add_word ( p, BC_SET32_ESI_RES );
      add_word ( p, BC_PUSH32 );
      add_word ( p, BC_SET32_EDI_RES );
      add_word ( p, BC_PUSH32 );
      update_eip ( e, p, true );
      break;
    case IA32_PUSHA16:
      add_word ( p, BC_PUSHA16_CHECK_EXCEPTION );
      add_word ( p, BC_SET16_SP_OP0 );
      add_word ( p, BC_SET16_AX_RES );
      add_word ( p, BC_PUSH16 );
      add_word ( p, BC_SET16_CX_RES );
      add_word ( p, BC_PUSH16 );
      add_word ( p, BC_SET16_DX_RES );
      add_word ( p, BC_PUSH16 );
      add_word ( p, BC_SET16_BX_RES );
      add_word ( p, BC_PUSH16 );
      add_word ( p, BC_SET16_OP0_RES );
      add_word ( p, BC_PUSH16 );
      add_word ( p, BC_SET16_BP_RES );
      add_word ( p, BC_PUSH16 );
      add_word ( p, BC_SET16_SI_RES );
      add_word ( p, BC_PUSH16 );
      add_word ( p, BC_SET16_DI_RES );
      add_word ( p, BC_PUSH16 );
      update_eip ( e, p, true );
      break;
    case IA32_PUSHF32:
      add_word ( p, BC_PUSH32_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_PUSHF16:
      add_word ( p, BC_PUSH16_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_RCL32: compile_rcl32 ( e, p ); break;
    case IA32_RCL16: compile_rcl16 ( e, p ); break;
    case IA32_RCL8: compile_rcl8 ( e, p ); break;
    case IA32_RCR32: compile_rcr32 ( e, p ); break;
    case IA32_RCR16: compile_rcr16 ( e, p ); break;
    case IA32_RCR8: compile_rcr8 ( e, p ); break;
    case IA32_RET32_FAR: compile_ret_far ( e, p, true ); break;
    case IA32_RET16_FAR: compile_ret_far ( e, p, false ); break;
    case IA32_RET32_NEAR: compile_ret_near ( e, p, true ); break;
    case IA32_RET16_NEAR: compile_ret_near ( e, p, false ); break;
    case IA32_ROL32: compile_rol32 ( e, p ); break;
    case IA32_ROL16: compile_rol16 ( e, p ); break;
    case IA32_ROL8: compile_rol8 ( e, p ); break;
    case IA32_ROR32: compile_ror32 ( e, p ); break;
    case IA32_ROR16: compile_ror16 ( e, p ); break;
    case IA32_ROR8: compile_ror8 ( e, p ); break;
    case IA32_SAHF: add_word ( p, BC_SAHF ); update_eip ( e, p, true ); break;
    case IA32_SAR32: compile_sar32 ( e, p ); break;
    case IA32_SAR16: compile_sar16 ( e, p ); break;
    case IA32_SAR8: compile_sar8 ( e, p ); break;
    case IA32_SBB32:
      compile_sub32_like ( e, p, true, false, true, true, false );
      break;
    case IA32_SBB16:
      compile_sub16_like ( e, p, true, false, true, true, false );
      break;
    case IA32_SBB8:
      compile_sub8_like ( e, p, true, false, true, true, false );
      break;
    case IA32_SCAS32:
    case IA32_SCAS16: 
    case IA32_SCAS8: compile_scas ( e, p ); break;
    case IA32_SETA: compile_setcc ( e, p, BC_SET_A_COND ); break;
    case IA32_SETAE: compile_setcc ( e, p, BC_SET_AE_COND ); break;
    case IA32_SETB: compile_setcc ( e, p, BC_SET_B_COND ); break;
    case IA32_SETE: compile_setcc ( e, p, BC_SET_E_COND ); break;
    case IA32_SETG: compile_setcc ( e, p, BC_SET_G_COND ); break;
    case IA32_SETGE: compile_setcc ( e, p, BC_SET_GE_COND ); break;
    case IA32_SETL: compile_setcc ( e, p, BC_SET_L_COND ); break;
    case IA32_SETNA: compile_setcc ( e, p, BC_SET_NA_COND ); break;
    case IA32_SETNE: compile_setcc ( e, p, BC_SET_NE_COND ); break;
    case IA32_SETNG: compile_setcc ( e, p, BC_SET_NG_COND ); break;
    case IA32_SETS: compile_setcc ( e, p, BC_SET_S_COND ); break;
    case IA32_SGDT32: compile_sgdt_like ( e, p, BC_SGDT32 ); break;
    case IA32_SHL32: compile_shl32 ( e, p ); break;
    case IA32_SHL16: compile_shl16 ( e, p ); break;
    case IA32_SHL8: compile_shl8 ( e, p ); break;
    case IA32_SHLD32: compile_shld32 ( e, p ); break;
    case IA32_SHLD16: compile_shld16 ( e, p ); break;
    case IA32_SHR32: compile_shr32 ( e, p ); break;
    case IA32_SHR16: compile_shr16 ( e, p ); break;
    case IA32_SHR8: compile_shr8 ( e, p ); break;
    case IA32_SHRD32: compile_shrd32 ( e, p ); break;
    case IA32_SHRD16: compile_shrd16 ( e, p ); break;
    case IA32_SIDT32: compile_sgdt_like ( e, p, BC_SIDT32 ); break;
    case IA32_SIDT16: compile_sgdt_like ( e, p, BC_SIDT16 ); break;
    case IA32_SLDT:
      compile_inst_rm16_noflags_write ( e, p, BC_SET16_LDTR_RES );
      break;
    case IA32_SMSW16:
      compile_inst_rm16_noflags_write ( e, p, BC_SET16_CR0_RES_NOCHECK );
      break;
      // NOTA! Si tot va bé, el DIS sols traurà SMSW32 quan el destí
      // siga registre.
    case IA32_SMSW32:
      compile_inst_rm32_noflags_write ( e, p, BC_SET32_CR0_RES_NOCHECK );
      break;
    case IA32_STC:
      add_word ( p, BC_SET_CF_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_STD:
      add_word ( p, BC_SET_DF_EFLAGS );
      update_eip ( e, p, true );
      break;
    case IA32_STI: add_word ( p, BC_STI ); update_eip ( e, p, true ); break;
    case IA32_STOS32: compile_stos ( e, p ); break;
    case IA32_STOS16: compile_stos ( e, p ); break;
    case IA32_STOS8: compile_stos ( e, p ); break;
    case IA32_STR:
      compile_inst_rm16_noflags_write ( e, p, BC_SET16_TR_RES );
      break;
    case IA32_SUB32:
      compile_sub32_like ( e, p, true, false, true, false, false );
      break;
    case IA32_SUB16:
      compile_sub16_like ( e, p, true, false, true, false, false );
      break;
    case IA32_SUB8:
      compile_sub8_like ( e, p, true, false, true, false, false );
      break;
    case IA32_TEST32: compile_lop32 ( e, p, BC_AND32, false ); break;
    case IA32_TEST16: compile_lop16 ( e, p, BC_AND16, false ); break;
    case IA32_TEST8: compile_lop8 ( e, p, BC_AND8, false ); break;
    case IA32_UNK: add_word ( p, BC_UNK ); add_word ( p, e->inst.name ); break;
    case IA32_VERR:
    case IA32_VERW:
      if ( !compile_get_rm16_op ( e, p, 0, true ) )
        {
          fprintf ( FERROR,
                    "[EE] jit.c - verr_verw: Operador 0 desconegut: %d\n",
                    e->inst.ops[0].type );
          exit ( EXIT_FAILURE );
        }
      add_word ( p, e->inst.name==IA32_VERR ? BC_VERR : BC_VERW );
      update_eip ( e, p, true );
      break;
    case IA32_WBINVD:
      add_word ( p, BC_WBINVD );
      update_eip ( e, p, true );
      break;
    case IA32_XCHG32: compile_xchg32 ( e, p ); break;
    case IA32_XCHG16: compile_xchg16 ( e, p ); break;
    case IA32_XCHG8: compile_xchg8 ( e, p ); break;
    case IA32_XLATB32: compile_xlatb ( e, p, true ); break;
    case IA32_XLATB16: compile_xlatb ( e, p, false ); break;
    case IA32_XOR32: compile_lop32 ( e, p, BC_XOR32, true ); break;
    case IA32_XOR16: compile_lop16 ( e, p, BC_XOR16, true ); break;
    case IA32_XOR8: compile_lop8 ( e, p, BC_XOR8, true ); break;
    default:
      print_pages ( FERROR, jit );
      fprintf ( FERROR,
                "[EE] jit.c - compile: Instrucció desconeguda"
                " %d %d %d %d (%X)\n",
                e->inst.name, e->inst.ops[0].type,
                e->inst.ops[1].type, e->inst.ops[2].type,
                e->addr );
      exit ( EXIT_FAILURE );
    }
    
} // end compile
