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
 *  dis.c - Implementació del 'disassembler'.
 *
 */


#include <stddef.h>
#include <stdlib.h>

#include "IA32.h"




/**********/
/* MACROS */
/**********/

#define PROTO_DIS        			\
  IA32_Disassembler *dis,        		\
  uint64_t          *offset,        		\
  IA32_Inst         *inst

#define DIS dis, offset, inst

#define MEM(OFFSET,TO_PTR,ACTION_ON_ERROR)                  \
  {                                                         \
    if ( !dis->mem_read ( dis->udata, OFFSET, TO_PTR ) )    \
      {                                                     \
        ACTION_ON_ERROR;                                    \
      }                                                     \
  }

#define ADDRESS_SIZE_IS_32        					\
  ((dis->address_size_is_32 ( dis->udata ))^dis->_switch_addr_size)

#define OPERAND_SIZE_IS_32        					\
  ((dis->operand_size_is_32 ( dis->udata ))^dis->_switch_operand_size)

#define OVERRIDE_SEG(TO)        			\
  if ( dis->_data_seg != IA32_UNK_SEG ) (TO)= dis->_data_seg

#define IS_REG16(OP)        					\
  ((OP)==IA32_AX||(OP)==IA32_CX||(OP)==IA32_DX||(OP)==IA32_BX|| \
   (OP)==IA32_SP||(OP)==IA32_BP||(OP)==IA32_SI||(OP)==IA32_DI)

#define IS_REG32(OP)        					    \
  ((OP)==IA32_EAX||(OP)==IA32_ECX||(OP)==IA32_EDX||(OP)==IA32_EBX|| \
   (OP)==IA32_ESP||(OP)==IA32_EBP||(OP)==IA32_ESI||(OP)==IA32_EDI)




/*********************/
/* FUNCIONS PRIVADES */
/*********************/

static bool
exec_inst (
           PROTO_DIS,
           const uint8_t opcode
           );

static bool
read_op_u8 (
            PROTO_DIS,
            const int  op
            )
{
  
  uint8_t val;
  
  MEM ( (*offset)++, &val, return false );
  inst->bytes[inst->nbytes++]= val;
  ++(inst->real_nbytes);
  inst->ops[op].u8= val;

  return true;
  
} // end read_op_u8


static bool
read_op_ptr16 (
               PROTO_DIS,
               const int op
               )
{
  
  uint16_t val;
  uint8_t aux;
  

  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  val= (uint16_t) aux;
  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  val|= ((uint16_t) aux)<<8;
  inst->ops[op].ptr16= val;

  return true;
  
} // end read_op_ptr16


static bool
read_op_u16 (
             PROTO_DIS,
             const int op
             )
{
  
  uint16_t val;
  uint8_t aux;
  

  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  val= (uint16_t) aux;
  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  val|= ((uint16_t) aux)<<8;
  inst->ops[op].u16= val;

  return true;
  
} // end read_op_u16


static bool
read_op_u32_val (
        	 PROTO_DIS,
                 uint32_t *val
        	 )
{
  
  uint8_t aux;
  

  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  (*val)= (uint32_t) aux;
  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  (*val)|= ((uint32_t) aux)<<8;
  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  (*val)|= ((uint32_t) aux)<<16;
  MEM ( (*offset)++, &aux, return false );
  inst->bytes[inst->nbytes++]= aux;
  ++(inst->real_nbytes);
  (*val)|= ((uint32_t) aux)<<24;

  return true;
  
} // end read_op_u32_val


static bool
read_op_u32 (
             PROTO_DIS,
             const int op
             )
{
  return read_op_u32_val ( DIS, &(inst->ops[op].u32) );
} // end read_op_u32


static bool
read_op_sib_u32 (
        	 PROTO_DIS,
        	 const int op
        	 )
{
  return read_op_u32_val ( DIS, &(inst->ops[op].sib_u32) );
} // end read_op_sib_u32


static bool
get_addr_16bit (
        	PROTO_DIS,
        	const uint8_t modrm,
        	const int     op_addr
        	)
{
  
  switch ( modrm )
    {
      
      // Mod 00
    case 0x00: // [BX+SI]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_SI;
      break;
    case 0x01: // [BX+DI]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_DI;
      break;
    case 0x02: // [BP+SI]
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_SI;
      break;
    case 0x03: // [BP+DI]
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_DI;
      break;
    case 0x04: // [SI]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_SI;
      break;
    case 0x05: // [DI]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_DI;
      break;
    case 0x06: // disp16.
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x07: // [BX]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX;
      break;
      
      // Mod 01
    case 0x08: // [BX+SI]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_SI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x09: // [BX+DI]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_DI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0A: // [BP+SI]+disp8
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_SI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0B: // [BP+DI]+disp8
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_DI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0C: // [SI]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_SI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0D: // [DI]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_DI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0E: // [BP]+disp8.
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0F: // [BX]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
      
      // Mod 10
    case 0x10: // [BX+SI]+disp16
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_SI_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x11: // [BX+DI]+disp16
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_DI_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x12: // [BP+SI]+disp16
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_SI_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x13: // [BP+DI]+disp16
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_DI_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x14: // [SI]+disp16
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_SI_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x15: // [DI]+disp16
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_DI_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x16: // [BP]+disp16.
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR16_BP_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
    case 0x17: // [BX]+disp16
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR16_BX_DISP16;
      if ( !read_op_u16 ( DIS, op_addr ) )
        return false;
      break;
      
      // Mod 11
    default:
      inst->ops[op_addr].type= IA32_NONE;
      break;
      
    }
  
  // Sobreescriu el segment.
  OVERRIDE_SEG ( inst->ops[op_addr].data_seg );

  return true;
  
} // end get_addr_16bit


static bool
get_addr_sib (
              PROTO_DIS,
              const uint8_t modrm,
              const int     op
              )
{

  uint8_t sib;
  

  MEM ( (*offset)++, &sib, return false );
  inst->bytes[inst->nbytes++]= sib;
  ++(inst->real_nbytes);
  
  /* Base */
  switch ( sib&0x7 )
    {
    case 0:
      inst->ops[op].data_seg= IA32_DS;
      inst->ops[op].sib_val= IA32_SIB_VAL_EAX;
      break;
    case 1:
      inst->ops[op].data_seg= IA32_DS;
      inst->ops[op].sib_val= IA32_SIB_VAL_ECX;
      break;
    case 2:
      inst->ops[op].data_seg= IA32_DS;
      inst->ops[op].sib_val= IA32_SIB_VAL_EDX;
      break;
    case 3:
      inst->ops[op].data_seg= IA32_DS;
      inst->ops[op].sib_val= IA32_SIB_VAL_EBX;
      break;
    case 4:
      inst->ops[op].data_seg= IA32_SS;
      inst->ops[op].sib_val= IA32_SIB_VAL_ESP;
      break;
    case 5:
      switch ( modrm>>3 )
        {
        case 0: // MOD 00
          inst->ops[op].data_seg= IA32_DS;
          inst->ops[op].sib_val= IA32_SIB_VAL_DISP32;
          if ( !read_op_sib_u32 ( DIS, op ) )
            return false;
          break;
        case 1: // MOD 01, MOD 10
        case 2:
          inst->ops[op].data_seg= IA32_SS; // P_DS o P_SS ?????
          inst->ops[op].sib_val= IA32_SIB_VAL_EBP;
          break;
        default: break;
        }
      break;
    case 6:
      inst->ops[op].data_seg= IA32_DS;
      inst->ops[op].sib_val= IA32_SIB_VAL_ESI;
      break;
    case 7:
      inst->ops[op].data_seg= IA32_DS;
      inst->ops[op].sib_val= IA32_SIB_VAL_EDI;
      break;
    }

  // Índex.
  switch ( sib>>3 )
    {
      
      // SS 00
    case 0x00: inst->ops[op].sib_scale= IA32_SIB_SCALE_EAX; break;
    case 0x01: inst->ops[op].sib_scale= IA32_SIB_SCALE_ECX; break;
    case 0x02: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDX; break;
    case 0x03: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBX; break;
    case 0x04: inst->ops[op].sib_scale= IA32_SIB_SCALE_NONE; break;
    case 0x05: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBP; /*inst->ops[op].data_seg= IA32_SS;*/break;
    case 0x06: inst->ops[op].sib_scale= IA32_SIB_SCALE_ESI; break;
    case 0x07: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDI; break;
      
      // SS 01
    case 0x08: inst->ops[op].sib_scale= IA32_SIB_SCALE_EAX_2; break;
    case 0x09: inst->ops[op].sib_scale= IA32_SIB_SCALE_ECX_2; break;
    case 0x0A: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDX_2; break;
    case 0x0B: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBX_2; break;
    case 0x0C: inst->ops[op].sib_scale= IA32_SIB_SCALE_NONE; break;
    case 0x0D: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBP_2; /*inst->ops[op].data_seg= IA32_SS;*/break;
    case 0x0E: inst->ops[op].sib_scale= IA32_SIB_SCALE_ESI_2; break;
    case 0x0F: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDI_2; break;
      
      // SS 10
    case 0x10: inst->ops[op].sib_scale= IA32_SIB_SCALE_EAX_4; break;
    case 0x11: inst->ops[op].sib_scale= IA32_SIB_SCALE_ECX_4; break;
    case 0x12: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDX_4; break;
    case 0x13: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBX_4; break;
    case 0x14: inst->ops[op].sib_scale= IA32_SIB_SCALE_NONE; break;
    case 0x15: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBP_4; /*inst->ops[op].data_seg= IA32_SS;*/break;
    case 0x16: inst->ops[op].sib_scale= IA32_SIB_SCALE_ESI_4; break;
    case 0x17: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDI_4; break;
      
      // SS 11
    case 0x18: inst->ops[op].sib_scale= IA32_SIB_SCALE_EAX_8; break;
    case 0x19: inst->ops[op].sib_scale= IA32_SIB_SCALE_ECX_8; break;
    case 0x1A: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDX_8; break;
    case 0x1B: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBX_8; break;
    case 0x1C: inst->ops[op].sib_scale= IA32_SIB_SCALE_NONE; break;
    case 0x1D: inst->ops[op].sib_scale= IA32_SIB_SCALE_EBP_8; /*inst->ops[op].data_seg= IA32_SS;*/break;
    case 0x1E: inst->ops[op].sib_scale= IA32_SIB_SCALE_ESI_8; break;
    case 0x1F: inst->ops[op].sib_scale= IA32_SIB_SCALE_EDI_8; break;
      
    }

  return true;
  
} // end get_addr_sib


static bool
get_addr_32bit (
                PROTO_DIS,
                const uint8_t modrm,
        	const int     op_addr
                )
{
  
  switch ( modrm )
    {
      
      // Mod 00
    case 0x00: // [EAX]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EAX;
      break;
    case 0x01: // [ECX]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_ECX;
      break;
    case 0x02: // [EDX]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EDX;
      break;
    case 0x03: // [EBX]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EBX;
      break;
    case 0x04: // [--][--]
      inst->ops[op_addr].type= IA32_ADDR32_SIB;
      if ( !get_addr_sib ( DIS, modrm, op_addr ) )
        return false;
      break;
    case 0x05: // disp32
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x06: // [ESI]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_ESI;
      break;
    case 0x07: // [EDI]
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EDI;
      break;

      // Mod 01
    case 0x08: // [EAX]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EAX_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x09: // [ECX]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_ECX_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0A: // [EDX]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EDX_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0B: // [EBX]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EBX_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0C: // [--][--]+disp8
      inst->ops[op_addr].type= IA32_ADDR32_SIB_DISP8;
      if ( !get_addr_sib ( DIS, modrm, op_addr ) )
        return false;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0D: // [EBP]+disp8
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR32_EBP_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0E: // [ESI]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_ESI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;
    case 0x0F: // [EDI]+disp8
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EDI_DISP8;
      if ( !read_op_u8 ( DIS, op_addr ) )
        return false;
      break;

      // Mod 10
    case 0x10: // [EAX]+disp32
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EAX_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x11: // [ECX]+disp32
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_ECX_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x12: // [EDX]+disp32
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EDX_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x13: // [EBX]+disp32
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EBX_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x14: // [--][--]+disp32
      inst->ops[op_addr].type= IA32_ADDR32_SIB_DISP32;
      if ( !get_addr_sib ( DIS, modrm, op_addr ) )
        return false;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x15: // [EBP]+disp32
      inst->ops[op_addr].data_seg= IA32_SS;
      inst->ops[op_addr].type= IA32_ADDR32_EBP_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x16: // [ESI]+disp32
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_ESI_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;
    case 0x17: // [EDI]+disp32
      inst->ops[op_addr].data_seg= IA32_DS;
      inst->ops[op_addr].type= IA32_ADDR32_EDI_DISP32;
      if ( !read_op_u32 ( DIS, op_addr ) )
        return false;
      break;

      // Mod 11
    default:
      inst->ops[op_addr].type= IA32_NONE;
      break;
      
    }
  
  // Sobreescriu el segment.
  OVERRIDE_SEG ( inst->ops[op_addr].data_seg );

  return true;
  
} // end get_addr_32bit


static bool
get_addr_mode_r8 (
                  PROTO_DIS,
                  const uint8_t modrmbyte,
        	  const int     op_eaddr,
        	  const int     op_reg // -1  si no s'ha d'assignar.
                  )
{

  uint8_t modrm;
  bool ret;
  

  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_AL; break;
      case 1: inst->ops[op_reg].type= IA32_CL; break;
      case 2: inst->ops[op_reg].type= IA32_DL; break;
      case 3: inst->ops[op_reg].type= IA32_BL; break;
      case 4: inst->ops[op_reg].type= IA32_AH; break;
      case 5: inst->ops[op_reg].type= IA32_CH; break;
      case 6: inst->ops[op_reg].type= IA32_DH; break;
      case 7: inst->ops[op_reg].type= IA32_BH; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_AL; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_CL; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_DL; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_BL; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_AH; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_CH; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_DH; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_BH; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_r8


static bool
get_addr_mode_rm8_r16 (
                       PROTO_DIS,
                       const uint8_t modrmbyte,
                       const int     op_eaddr,
                       const int     op_reg // -1  si no s'ha d'assignar.
                       )
{

  uint8_t modrm;
  bool ret;


  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_AX; break;
      case 1: inst->ops[op_reg].type= IA32_CX; break;
      case 2: inst->ops[op_reg].type= IA32_DX; break;
      case 3: inst->ops[op_reg].type= IA32_BX; break;
      case 4: inst->ops[op_reg].type= IA32_SP; break;
      case 5: inst->ops[op_reg].type= IA32_BP; break;
      case 6: inst->ops[op_reg].type= IA32_SI; break;
      case 7: inst->ops[op_reg].type= IA32_DI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_AL; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_CL; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_DL; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_BL; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_AH; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_CH; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_DH; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_BH; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_rm8_r16


static bool
get_addr_mode_r16 (
        	   PROTO_DIS,
        	   const uint8_t modrmbyte,
        	   const int     op_eaddr,
        	   const int     op_reg // -1  si no s'ha d'assignar.
        	   )
{

  uint8_t modrm;
  bool ret;
  

  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_AX; break;
      case 1: inst->ops[op_reg].type= IA32_CX; break;
      case 2: inst->ops[op_reg].type= IA32_DX; break;
      case 3: inst->ops[op_reg].type= IA32_BX; break;
      case 4: inst->ops[op_reg].type= IA32_SP; break;
      case 5: inst->ops[op_reg].type= IA32_BP; break;
      case 6: inst->ops[op_reg].type= IA32_SI; break;
      case 7: inst->ops[op_reg].type= IA32_DI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_AX; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_CX; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_DX; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_BX; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_SP; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_BP; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_SI; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_DI; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }
  
  return ret;
  
} // end get_addr_mode_r16


static bool
get_addr_mode_sreg16 (
                      PROTO_DIS,
                      const uint8_t modrmbyte,
                      const int     op_eaddr,
                      const int     op_reg // -1  si no s'ha d'assignar.
                      )
{

  uint8_t modrm;
  bool ret;
  

  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_SEG_ES; break;
      case 1: inst->ops[op_reg].type= IA32_SEG_CS; break;
      case 2: inst->ops[op_reg].type= IA32_SEG_SS; break;
      case 3: inst->ops[op_reg].type= IA32_SEG_DS; break;
      case 4: inst->ops[op_reg].type= IA32_SEG_FS; break;
      case 5: inst->ops[op_reg].type= IA32_SEG_GS; break;
      default: inst->ops[op_reg].type= IA32_NONE; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_AX; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_CX; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_DX; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_BX; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_SP; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_BP; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_SI; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_DI; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }
  
  return ret;
  
} // end get_addr_mode_sreg16


static bool
get_addr_mode_sreg32 (
                      PROTO_DIS,
                      const uint8_t  modrmbyte,
                      const int      op_eaddr,
                      const int      op_reg, // -1  si no s'ha d'assignar.
                      bool          *is_reg
                      )
{

  uint8_t modrm;
  bool ret;
  

  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_SEG_ES; break;
      case 1: inst->ops[op_reg].type= IA32_SEG_CS; break;
      case 2: inst->ops[op_reg].type= IA32_SEG_SS; break;
      case 3: inst->ops[op_reg].type= IA32_SEG_DS; break;
      case 4: inst->ops[op_reg].type= IA32_SEG_FS; break;
      case 5: inst->ops[op_reg].type= IA32_SEG_GS; break;
      default: inst->ops[op_reg].type= IA32_NONE; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  *is_reg= true;
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_EAX; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_ECX; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_EDX; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_EBX; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_ESP; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_EBP; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_ESI; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_EDI; break;
      
    default:
      *is_reg= false;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }
  
  return ret;
  
} // end get_addr_mode_sreg32


static bool
get_addr_mode_rm8_r32 (
                       PROTO_DIS,
                       const uint8_t modrmbyte,
                       const int     op_eaddr,
                       const int     op_reg // -1  si no s'ha d'assignar.
                       )
{

  uint8_t modrm;
  bool ret;
  

  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_EAX; break;
      case 1: inst->ops[op_reg].type= IA32_ECX; break;
      case 2: inst->ops[op_reg].type= IA32_EDX; break;
      case 3: inst->ops[op_reg].type= IA32_EBX; break;
      case 4: inst->ops[op_reg].type= IA32_ESP; break;
      case 5: inst->ops[op_reg].type= IA32_EBP; break;
      case 6: inst->ops[op_reg].type= IA32_ESI; break;
      case 7: inst->ops[op_reg].type= IA32_EDI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_AL; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_CL; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_DL; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_BL; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_AH; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_CH; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_DH; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_BH; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_rm8_r32


static bool
get_addr_mode_rm16_r32 (
                        PROTO_DIS,
                        const uint8_t modrmbyte,
                        const int     op_eaddr,
                        const int     op_reg // -1  si no s'ha d'assignar.
                       )
{

  uint8_t modrm;
  bool ret;
  

  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_EAX; break;
      case 1: inst->ops[op_reg].type= IA32_ECX; break;
      case 2: inst->ops[op_reg].type= IA32_EDX; break;
      case 3: inst->ops[op_reg].type= IA32_EBX; break;
      case 4: inst->ops[op_reg].type= IA32_ESP; break;
      case 5: inst->ops[op_reg].type= IA32_EBP; break;
      case 6: inst->ops[op_reg].type= IA32_ESI; break;
      case 7: inst->ops[op_reg].type= IA32_EDI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_AX; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_CX; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_DX; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_BX; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_SP; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_BP; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_SI; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_DI; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }

  return ret;
  
} // end get_addr_mode_rm16_r32


static bool
get_addr_mode_r32 (
        	   PROTO_DIS,
        	   const uint8_t modrmbyte,
        	   const int     op_eaddr,
        	   const int     op_reg // -1  si no s'ha d'assignar.
        	   )
{

  uint8_t modrm;
  bool ret;


  ret= true;
  
  // Reg.
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_EAX; break;
      case 1: inst->ops[op_reg].type= IA32_ECX; break;
      case 2: inst->ops[op_reg].type= IA32_EDX; break;
      case 3: inst->ops[op_reg].type= IA32_EBX; break;
      case 4: inst->ops[op_reg].type= IA32_ESP; break;
      case 5: inst->ops[op_reg].type= IA32_EBP; break;
      case 6: inst->ops[op_reg].type= IA32_ESI; break;
      case 7: inst->ops[op_reg].type= IA32_EDI; break;
      }
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_EAX; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_ECX; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_EDX; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_EBX; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_ESP; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_EBP; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_ESI; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_EDI; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }
  
  return ret;
  
} // end get_addr_mode_r32


static bool
get_addr_mode_r32_m16 (
                       PROTO_DIS,
                       const uint8_t       modrmbyte,
                       const int           op_eaddr,
                       const IA32_Mnemonic name16,
                       const IA32_Mnemonic name32
                       )
{

  uint8_t modrm;
  bool ret;
  
  
  ret= true;
  inst->name= name32;
  
  // Effective address.
  modrm= (modrmbyte&0x7)|((modrmbyte&0xC0)>>3);
  switch ( modrm )
    {
      
      // Mod 11
    case 0x18: inst->ops[op_eaddr].type= IA32_EAX; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_ECX; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_EDX; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_EBX; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_ESP; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_EBP; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_ESI; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_EDI; break;
      
    default:
      inst->name= name16;
      if ( ADDRESS_SIZE_IS_32 )
        ret= get_addr_32bit ( DIS, modrm, op_eaddr );
      else
        ret= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }
  
  return ret;
  
} // end get_addr_mode_r32_m16


static bool
get_addr_moffs (
                PROTO_DIS,
                const int op
                )
{

  if ( ADDRESS_SIZE_IS_32 )
    {
      if ( !read_op_u32 ( DIS, op ) )
        return false;
      inst->ops[op].type= IA32_MOFFS_OFF32;
    }
  else
    {
      if ( !read_op_u16 ( DIS, op ) )
        return false;
      inst->ops[op].type= IA32_MOFFS_OFF16;
    }
  inst->ops[op].data_seg= IA32_DS;
  OVERRIDE_SEG ( inst->ops[op].data_seg );
  
  return true;
  
} // end get_addr_moffs


static void
get_addr_mode_cr (
                  PROTO_DIS,
                  const uint8_t modrmbyte,
                  const int     op_reg,
                  const int     op_cr
                  )
{

  uint8_t modrm;
  
  
  // Reg.
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: inst->ops[op_cr].type= IA32_CR0; break;
    case 1: inst->ops[op_cr].type= IA32_CR1; break;
    case 2: inst->ops[op_cr].type= IA32_CR2; break;
    case 3: inst->ops[op_cr].type= IA32_CR3; break;
    case 4: inst->ops[op_cr].type= IA32_CR4; break;
    default: inst->ops[op_cr].type= IA32_NONE; break;
    }
  
  // Effective address.
  modrm= (modrmbyte&0x7);
  switch ( modrm )
    {
      
    case 0: inst->ops[op_reg].type= IA32_EAX; break;
    case 1: inst->ops[op_reg].type= IA32_ECX; break;
    case 2: inst->ops[op_reg].type= IA32_EDX; break;
    case 3: inst->ops[op_reg].type= IA32_EBX; break;
    case 4: inst->ops[op_reg].type= IA32_ESP; break;
    case 5: inst->ops[op_reg].type= IA32_EBP; break;
    case 6: inst->ops[op_reg].type= IA32_ESI; break;
    case 7: inst->ops[op_reg].type= IA32_EDI; break;
      
    }
  
} // end get_addr_mode_cr


static void
get_addr_mode_dr (
                  PROTO_DIS,
                  const uint8_t modrmbyte,
                  const int     op_reg,
                  const int     op_dr
                  )
{

  uint8_t modrm;
  
  
  // Reg.
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: inst->ops[op_dr].type= IA32_DR0; break;
    case 1: inst->ops[op_dr].type= IA32_DR1; break;
    case 2: inst->ops[op_dr].type= IA32_DR2; break;
    case 3: inst->ops[op_dr].type= IA32_DR3; break;
    case 4: inst->ops[op_dr].type= IA32_DR4; break;
    case 5: inst->ops[op_dr].type= IA32_DR5; break;
    case 6: inst->ops[op_dr].type= IA32_DR6; break;
    case 7: inst->ops[op_dr].type= IA32_DR7; break;
    default: inst->ops[op_dr].type= IA32_NONE; break;
    }
  
  // Effective address.
  modrm= (modrmbyte&0x7);
  switch ( modrm )
    {
      
    case 0: inst->ops[op_reg].type= IA32_EAX; break;
    case 1: inst->ops[op_reg].type= IA32_ECX; break;
    case 2: inst->ops[op_reg].type= IA32_EDX; break;
    case 3: inst->ops[op_reg].type= IA32_EBX; break;
    case 4: inst->ops[op_reg].type= IA32_ESP; break;
    case 5: inst->ops[op_reg].type= IA32_EBP; break;
    case 6: inst->ops[op_reg].type= IA32_ESI; break;
    case 7: inst->ops[op_reg].type= IA32_EDI; break;
      
    }
  
} // end get_addr_mode_dr

#if 0
static void
get_addr_mode_rxmm (
        	    PROTO_DIS,
        	    const uint8_t modrmbyte,
        	    const int     op_eaddr,
        	    const int     op_reg /* -1  si no s'ha d'assignar. */
        	    )
{

  uint8_t modrm;
  

  /* Reg. */
  if ( op_reg != -1 )
    switch ( (modrmbyte>>3)&0x7 )
      {
      case 0: inst->ops[op_reg].type= IA32_XMM0; break;
      case 1: inst->ops[op_reg].type= IA32_XMM1; break;
      case 2: inst->ops[op_reg].type= IA32_XMM2; break;
      case 3: inst->ops[op_reg].type= IA32_XMM3; break;
      case 4: inst->ops[op_reg].type= IA32_XMM4; break;
      case 5: inst->ops[op_reg].type= IA32_XMM5; break;
      case 6: inst->ops[op_reg].type= IA32_XMM6; break;
      case 7: inst->ops[op_reg].type= IA32_XMM7; break;
      }
  
  /* Effective address. */
  modrm= (modrmbyte&0x7)|(modrmbyte>>6);
  switch ( modrm )
    {
      
      /* Mod 11 */
    case 0x18: inst->ops[op_eaddr].type= IA32_XMM0; break;
    case 0x19: inst->ops[op_eaddr].type= IA32_XMM1; break;
    case 0x1A: inst->ops[op_eaddr].type= IA32_XMM2; break;
    case 0x1B: inst->ops[op_eaddr].type= IA32_XMM3; break;
    case 0x1C: inst->ops[op_eaddr].type= IA32_XMM4; break;
    case 0x1D: inst->ops[op_eaddr].type= IA32_XMM5; break;
    case 0x1E: inst->ops[op_eaddr].type= IA32_XMM6; break;
    case 0x1F: inst->ops[op_eaddr].type= IA32_XMM7; break;
      
    default:
      if ( ADDRESS_SIZE_IS_32 ) offset= get_addr_32bit ( DIS, modrm, op_eaddr );
      else                      offset= get_addr_16bit ( DIS, modrm, op_eaddr );
      break;
      
    }
  
} /* end get_addr_mode_rxmm */
#endif

static bool
unk_inst (
          IA32_Inst *inst	  
          )
{
  
  inst->name= IA32_UNK;
  inst->ops[0].type= IA32_NONE;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} /* end unk_inst */


static bool
inst_rm8_r8 (
             PROTO_DIS,
             const IA32_Mnemonic name
             )
{
  
  uint8_t modrmbyte;


  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r8 ( DIS, modrmbyte, 0, 1 ) )
    return false;

  return true;
  
} // end inst_rm8_r8


static bool
inst_rm16_r16 (
               PROTO_DIS,
               const IA32_Mnemonic name
               )
{

  uint8_t modrmbyte;


  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, 1 ) )
    return false;

  return true;
  
} // end inst_rm16_r16


static bool
inst_rm32_r32 (
               PROTO_DIS,
               const IA32_Mnemonic name
               )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, 1 ) )
    return false;

  return true;
  
} // end inst_rm32_r32


static bool
inst_rm_r (
           PROTO_DIS,
           const IA32_Mnemonic name16,
           const IA32_Mnemonic name32
           )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_rm32_r32 ( DIS, name32 );
  else                      ret= inst_rm16_r16 ( DIS, name16 );

  return ret;
  
} // end inst_rm_r


static bool
inst_r8_rm8 (
             PROTO_DIS,
             const IA32_Mnemonic name
             )
{

  uint8_t modrmbyte;


  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r8 ( DIS, modrmbyte, 1, 0 ) )
    return false;

  return true;
  
} // end inst_r8_rm8


static bool
inst_r16_rm16 (
               PROTO_DIS,
               const IA32_Mnemonic name
               )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 1, 0 ) )
    return false;

  return true;
  
} // end inst_r16_rm16


static bool
inst_rm16_sreg (
                PROTO_DIS,
                const IA32_Mnemonic name
                )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_sreg16 ( DIS, modrmbyte, 0, 1 ) )
    return false;

  return true;
  
} // end inst_rm16_sreg


static bool
inst_rm32_sreg (
                PROTO_DIS,
                const IA32_Mnemonic name16,
                const IA32_Mnemonic name32
                )
{
  
  uint8_t modrmbyte;
  bool is_reg;
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( !get_addr_mode_sreg32 ( DIS, modrmbyte, 0, 1, &is_reg ) )
    return false;
  inst->name= is_reg ? name32 : name16;

  return true;
  
} // end inst_rm32_sreg


static bool
inst_rm_sreg (
              PROTO_DIS,
              const IA32_Mnemonic name16,
              const IA32_Mnemonic name32
              )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_rm32_sreg ( DIS, name16, name32 );
  else                      ret= inst_rm16_sreg ( DIS, name16 );

  return ret;
  
} // end inst_rm_sreg


static bool
inst_sreg_rm16 (
                PROTO_DIS,
                const IA32_Mnemonic name
                )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_sreg16 ( DIS, modrmbyte, 1, 0 ) )
    return false;

  return true;
  
} // end inst_sreg_rm16


static bool
inst_r32_rm32 (
               PROTO_DIS,
               const IA32_Mnemonic name
               )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 1, 0 ) )
    return false;

  return true;
  
} // end inst_r32_rm32


static bool
inst_r_rm (
           PROTO_DIS,
           const IA32_Mnemonic name16,
           const IA32_Mnemonic name32
           )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_r32_rm32 ( DIS, name32 );
  else                      ret= inst_r16_rm16 ( DIS, name16 );

  return ret;
  
} // end inst_r_rm


static bool
inst_r16_rm16_imm8 (
                    PROTO_DIS,
                    const IA32_Mnemonic name
                    )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 1, 0 ) )
    return false;
  if ( !read_op_u8 ( DIS, 2 ) )
    return false;
  inst->ops[2].type= IA32_IMM8;
  
  return true;
  
} // end inst_r16_rm16_imm8


static bool
inst_r32_rm32_imm8 (
                    PROTO_DIS,
                    const IA32_Mnemonic name
                    )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 1, 0 ) )
    return false;
  if ( !read_op_u8 ( DIS, 2 ) )
    return false;
  inst->ops[2].type= IA32_IMM8;
  
  return true;
  
} // end inst_r32_rm32_imm8


static bool
inst_r_rm_imm8 (
                PROTO_DIS,
                const IA32_Mnemonic name16,
                const IA32_Mnemonic name32
                )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_r32_rm32_imm8 ( DIS, name32 );
  else                      ret= inst_r16_rm16_imm8 ( DIS, name16 );

  return ret;
  
} // end inst_r_rm_imm8


static bool
inst_r16_rm16_imm16 (
                     PROTO_DIS,
                     const IA32_Mnemonic name
                     )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 1, 0 ) )
    return false;
  if ( !read_op_u16 ( DIS, 2 ) )
    return false;
  inst->ops[2].type= IA32_IMM16;
  
  return true;
  
} // end inst_r16_rm16_imm16


static bool
inst_r32_rm32_imm32 (
                     PROTO_DIS,
                     const IA32_Mnemonic name
                     )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 1, 0 ) )
    return false;
  if ( !read_op_u32 ( DIS, 2 ) )
    return false;
  inst->ops[2].type= IA32_IMM32;
  
  return true;
  
} // end inst_r32_rm32_imm32


static bool
inst_r_rm_imm (
               PROTO_DIS,
               const IA32_Mnemonic name16,
               const IA32_Mnemonic name32
               )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_r32_rm32_imm32 ( DIS, name32 );
  else                      ret= inst_r16_rm16_imm16 ( DIS, name16 );

  return ret;
  
} // end inst_r_rm_imm


static bool
inst_rm16_r16_imm8 (
                    PROTO_DIS,
                    const IA32_Mnemonic name
                    )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, 1 ) )
    return false;
  if ( !read_op_u8 ( DIS, 2 ) )
    return false;
  inst->ops[2].type= IA32_IMM8;
  
  return true;
  
} // end inst_rm16_r16_imm8


static bool
inst_rm32_r32_imm8 (
                    PROTO_DIS,
                    const IA32_Mnemonic name
                    )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, 1 ) )
    return false;
  if ( !read_op_u8 ( DIS, 2 ) )
    return false;
  inst->ops[2].type= IA32_IMM8;
  
  return true;
  
} // end inst_rm32_r32_imm8


static bool
inst_rm_r_imm8 (
                PROTO_DIS,
                const IA32_Mnemonic name16,
                const IA32_Mnemonic name32
                )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_rm32_r32_imm8 ( DIS, name32 );
  else                      ret= inst_rm16_r16_imm8 ( DIS, name16 );

  return ret;
  
} // end inst_rm_r_imm8


static bool
inst_rm16_r16_CL (
                  PROTO_DIS,
                  const IA32_Mnemonic name
                  )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, 1 ) )
    return false;
  inst->ops[2].type= IA32_CL;
  
  return true;
  
} // end inst_rm16_r16_CL


static bool
inst_rm32_r32_CL (
                  PROTO_DIS,
                  const IA32_Mnemonic name
                  )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, 1 ) )
    return false;
  inst->ops[2].type= IA32_CL;
  
  return true;
  
} // end inst_rm32_r32_CL


static bool
inst_rm_r_CL (
              PROTO_DIS,
              const IA32_Mnemonic name16,
              const IA32_Mnemonic name32
              )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_rm32_r32_CL ( DIS, name32 );
  else                      ret= inst_rm16_r16_CL ( DIS, name16 );

  return ret;
  
} // end inst_rm_r_CL


static bool
inst_r16_rm8 (
              PROTO_DIS,
              const IA32_Mnemonic name
              )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_rm8_r16 ( DIS, modrmbyte, 1, 0 ) )
    return false;
  
  return true;
  
} // end inst_r16_rm8


static bool
inst_r32_rm8 (
              PROTO_DIS,
              const IA32_Mnemonic name
              )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_rm8_r32 ( DIS, modrmbyte, 1, 0 ) )
    return false;

  return true;
  
} // end inst_r32_rm8


static bool
inst_r_rm8 (
            PROTO_DIS,
            const IA32_Mnemonic name16,
            const IA32_Mnemonic name32
            )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_r32_rm8 ( DIS, name32 );
  else                      ret= inst_r16_rm8 ( DIS, name16 );

  return ret;
  
} // end inst_r_rm8


static bool
inst_r32_rm16 (
              PROTO_DIS,
              const IA32_Mnemonic name
              )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_rm16_r32 ( DIS, modrmbyte, 1, 0 ) )
    return false;

  return true;
  
} // end inst_r32_rm16


static bool
inst_r32_cr (
             PROTO_DIS,
             const IA32_Mnemonic name
             )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  get_addr_mode_cr ( DIS, modrmbyte, 0, 1 );
  
  return true;
  
} // end inst_r32_cr


static bool
inst_r32_dr (
             PROTO_DIS,
             const IA32_Mnemonic name
             )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  get_addr_mode_dr ( DIS, modrmbyte, 0, 1 );
  
  return true;
  
} // end inst_r32_dr


static bool
inst_cr_r32 (
             PROTO_DIS,
             const IA32_Mnemonic name
             )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  get_addr_mode_cr ( DIS, modrmbyte, 1, 0 );
  
  return true;
  
} // end inst_cr_r32


static bool
inst_dr_r32 (
             PROTO_DIS,
             const IA32_Mnemonic name
             )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  get_addr_mode_dr ( DIS, modrmbyte, 1, 0 );
  
  return true;
  
} // end inst_dr_r32


static bool
inst_AX (
         PROTO_DIS,
         const IA32_Mnemonic name
         )
{
  
  inst->name= name;
  inst->ops[0].type= IA32_AX;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_AX


static bool
inst_AL_imm8 (
              PROTO_DIS,
              const IA32_Mnemonic name
              )
{
  
  if ( !read_op_u8 ( DIS, 1 ) )
    return false;
  inst->name= name;
  inst->ops[0].type= IA32_AL;
  inst->ops[1].type= IA32_IMM8;

  return true;
  
} // end inst_AL_imm8


static bool
inst_A_imm (
            PROTO_DIS,
            const IA32_Mnemonic name16,
            const IA32_Mnemonic name32
            )
{
  
  if ( OPERAND_SIZE_IS_32 )
    {
      if ( !read_op_u32 ( DIS, 1 ) )
        return false;
      inst->ops[0].type= IA32_EAX;
      inst->ops[1].type= IA32_IMM32;
      inst->name= name32;
    }
  else
    {
      if ( !read_op_u16 ( DIS, 1 ) )
        return false;
      inst->ops[0].type= IA32_AX;
      inst->ops[1].type= IA32_IMM16;
      inst->name= name16;
    }

  return true;
  
} // end inst_A_imm


static bool
inst_imm8_AL (
              PROTO_DIS,
              const IA32_Mnemonic name
              )
{
  
  if ( !read_op_u8 ( DIS, 0 ) )
    return false;
  inst->name= name;
  inst->ops[0].type= IA32_IMM8;
  inst->ops[1].type= IA32_AL;

  return true;
  
} // end inst_imm8_AL


static bool
inst_imm8_A (
             PROTO_DIS,
             const IA32_Mnemonic name16,
             const IA32_Mnemonic name32
             )
{

  if ( !read_op_u8 ( DIS, 0 ) )
    return false;
  inst->ops[0].type= IA32_IMM8;
  if ( OPERAND_SIZE_IS_32 )
    {
      inst->name= name32;
      inst->ops[1].type= IA32_EAX;
    }
  else
    {
      inst->name= name16;
      inst->ops[1].type= IA32_AX;
    }

  return true;
  
} // end inst_imm8_A


static bool
inst_DX_AL (
            PROTO_DIS,
            const IA32_Mnemonic name
            )
{
  
  inst->name= name;
  inst->ops[0].type= IA32_DX;
  inst->ops[1].type= IA32_AL;

  return true;
  
} // end inst_DX_AL


static bool
inst_DX_A (
           PROTO_DIS,
           const IA32_Mnemonic name16,
           const IA32_Mnemonic name32
           )
{
  
  inst->ops[0].type= IA32_DX;
  if ( OPERAND_SIZE_IS_32 )
    {
      inst->name= name32;
      inst->ops[1].type= IA32_EAX;
    }
  else
    {
      inst->name= name16;
      inst->ops[1].type= IA32_AX;
    }

  return true;
  
} // end inst_DX_A


static bool
inst_AL_DX (
            PROTO_DIS,
            const IA32_Mnemonic name
            )
{
  
  inst->name= name;
  inst->ops[0].type= IA32_AL;
  inst->ops[1].type= IA32_DX;

  return true;
  
} // end inst_AL_DX


static bool
inst_A_DX (
           PROTO_DIS,
           const IA32_Mnemonic name16,
           const IA32_Mnemonic name32
           )
{
  
  inst->ops[1].type= IA32_DX;
  if ( OPERAND_SIZE_IS_32 )
    {
      inst->name= name32;
      inst->ops[0].type= IA32_EAX;
    }
  else
    {
      inst->name= name16;
      inst->ops[0].type= IA32_AX;
    }

  return true;
  
} // end inst_A_DX


static bool
inst_sreg (
           PROTO_DIS,
           const IA32_Mnemonic   name16,
           const IA32_Mnemonic   name32,
           const IA32_InstOpType seg
           )
{
  
  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].type= seg;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_sreg


static bool
inst_CS (
         PROTO_DIS,
         const IA32_Mnemonic name16,
         const IA32_Mnemonic name32
         )
{
  return inst_sreg ( DIS, name16, name32, IA32_SEG_CS );
} // end inst_CS


static bool
inst_SS (
         PROTO_DIS,
         const IA32_Mnemonic name16,
         const IA32_Mnemonic name32
         )
{
  return inst_sreg ( DIS, name16, name32, IA32_SEG_SS );
} // end inst_SS


static bool
inst_DS (
         PROTO_DIS,
         const IA32_Mnemonic name16,
         const IA32_Mnemonic name32
         )
{
  return inst_sreg ( DIS, name16, name32, IA32_SEG_DS );
} // end inst_DS


static bool
inst_ES (
         PROTO_DIS,
         const IA32_Mnemonic name16,
         const IA32_Mnemonic name32
         )
{
  return inst_sreg ( DIS, name16, name32, IA32_SEG_ES );
} // end inst_ES


static bool
inst_FS (
         PROTO_DIS,
         const IA32_Mnemonic name16,
         const IA32_Mnemonic name32
         )
{
  return inst_sreg ( DIS, name16, name32, IA32_SEG_FS );
} // end inst_FS


static bool
inst_GS (
         PROTO_DIS,
         const IA32_Mnemonic name16,
         const IA32_Mnemonic name32
         )
{
  return inst_sreg ( DIS, name16, name32, IA32_SEG_GS );
} // end inst_GS


/*
static void
inst_rxmm_rmxmm (
        	 PROTO_DIS,
        	 const IA32_Mnemonic name
        	 )
{

  uint8_t modrmbyte;


  modrmbyte= MEM ( offset++ );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  get_addr_mode_rxmm ( DIS, modrmbyte, 1, 0 );
  
} // end inst_rxmm_rmxmm
*/

static bool
inst_xlatb (
            PROTO_DIS,
            const IA32_Mnemonic  name16,
            const IA32_Mnemonic  name32
            )
{
  
  inst->name= ADDRESS_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].data_seg= IA32_DS;
  OVERRIDE_SEG(inst->ops[0].data_seg);
  inst->ops[0].type= IA32_NONE;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_xlatb


static bool
inst_no_op (
            PROTO_DIS,
            const IA32_Mnemonic  name16,
            const IA32_Mnemonic  name32
            )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].type= IA32_NONE;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_no_op


static bool
inst_no_op_nosize (
                   PROTO_DIS,
                   const IA32_Mnemonic name
                   )
{

  inst->name= name;
  inst->ops[0].type= IA32_NONE;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_no_op_nosize


static bool
inst_rm8_imm8 (
               PROTO_DIS,
               const uint8_t       modrmbyte,
               const IA32_Mnemonic name
               )
{
  
  inst->name= name;
  if ( !get_addr_mode_r8 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  inst->ops[1].type= IA32_IMM8;
  if ( !read_op_u8 ( DIS, 1 ) )
    return false;

  return true;
  
} // end inst_rm8_imm8


static bool
inst_rm_imm (
             PROTO_DIS,
             const uint8_t       modrmbyte,
             const IA32_Mnemonic name16,
             const IA32_Mnemonic name32
             )
{
  

  if ( OPERAND_SIZE_IS_32 )
    {
      if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->ops[1].type= IA32_IMM32;
      if ( !read_op_u32 ( DIS, 1 ) )
        return false;
      inst->name= name32;
    }
  else
    {
      if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->ops[1].type= IA32_IMM16;
      if ( !read_op_u16 ( DIS, 1 ) )
        return false;
      inst->name= name16;
    }

  return true;
  
} // end inst_rm_imm


static bool
inst_rm8_1 (
            PROTO_DIS,
            const uint8_t       modrmbyte,
            const IA32_Mnemonic name
            )
{

  if ( !get_addr_mode_r8 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  inst->name= name;
  inst->ops[1].type= IA32_CONSTANT_1;

  return true;
  
} // end inst_rm8_1


static bool
inst_rm_1 (
           PROTO_DIS,
           const uint8_t       modrmbyte,
           const IA32_Mnemonic name16,
           const IA32_Mnemonic name32
           )
{

  if ( OPERAND_SIZE_IS_32 )
    {
      if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->name= name32;
    }
  else
    {
      if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->name= name16;
    }
  inst->ops[1].type= IA32_CONSTANT_1;

  return true;
  
} // end inst_rm_1


static bool
inst_3 (
        PROTO_DIS,
        const IA32_Mnemonic name16,
        const IA32_Mnemonic name32
        )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].type= IA32_CONSTANT_3;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_3


static bool
inst_imm8 (
           PROTO_DIS,
           const IA32_Mnemonic name16,
           const IA32_Mnemonic name32
           )
{
  
  inst->ops[0].type= IA32_IMM8;
  if ( !read_op_u8 ( DIS, 0 ) )
    return false;
  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[1].type= IA32_NONE;
  
  return true;
  
} // end inst_imm8


static bool
inst_imm16_imm8 (
                 PROTO_DIS,
                 const IA32_Mnemonic name16,
                 const IA32_Mnemonic name32
                 )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].type= IA32_IMM16;
  if ( !read_op_u16 ( DIS, 0 ) )
    return false;
  inst->ops[1].type= IA32_IMM8;
  if ( !read_op_u8 ( DIS, 1 ) )
    return false;
  
  return true;
  
} // end inst_imm16_imm8


static bool
inst_imm16 (
           PROTO_DIS,
           const IA32_Mnemonic name16,
           const IA32_Mnemonic name32
            )
{
  
  inst->ops[0].type= IA32_IMM16;
  if ( !read_op_u16 ( DIS, 0 ) )
    return false;
  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[1].type= ADDRESS_SIZE_IS_32 ? IA32_USE_ADDR32 : IA32_USE_ADDR16;
  
  return true;
  
} // end inst_imm16


static bool
inst_imm (
          PROTO_DIS,
          const IA32_Mnemonic name16,
          const IA32_Mnemonic name32
          )
{
  

  if ( OPERAND_SIZE_IS_32 )
    {
      inst->ops[0].type= IA32_IMM32;
      if ( !read_op_u32 ( DIS, 0 ) )
        return false;
      inst->name= name32;
    }
  else
    {
      inst->ops[0].type= IA32_IMM16;
      if ( !read_op_u16 ( DIS, 0 ) )
        return false;
      inst->name= name16;
    }
  inst->ops[1].type= IA32_NONE;
 
  return true;
  
} // end inst_imm


static bool
inst_rm_imm8 (
              PROTO_DIS,
              const uint8_t       modrmbyte,
              const IA32_Mnemonic name16,
              const IA32_Mnemonic name32
              )
{
  
  if ( OPERAND_SIZE_IS_32 )
    {
      if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->name= name32;
    }
  else
    {
      if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->name= name16;
    }
  inst->ops[1].type= IA32_IMM8;
  if ( !read_op_u8 ( DIS, 1 ) )
    return false;

  return true;
  
} // end inst_rm_imm8


static bool
inst_rm_CL (
            PROTO_DIS,
            const uint8_t       modrmbyte,
            const IA32_Mnemonic name16,
            const IA32_Mnemonic name32
            )
{
  
  if ( OPERAND_SIZE_IS_32 )
    {
      if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->name= name32;
    }
  else
    {
      if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, -1 ) )
        return false;
      inst->name= name16;
    }
  inst->ops[1].type= IA32_CL;
  
  return true;
  
} // end inst_rm_CL


static bool
inst_rm8_CL (
             PROTO_DIS,
             const uint8_t       modrmbyte,
             const IA32_Mnemonic name
             )
{
  
  if ( !get_addr_mode_r8 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  inst->name= name;
  inst->ops[1].type= IA32_CL;
  
  return true;
  
} // end inst_rm8_CL


static bool
inst_r16_m16 (
              PROTO_DIS,
              const IA32_Mnemonic name
              )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 1, 0 ) )
    return false;
  if ( IS_REG16 ( inst->ops[1].type ) )
    return unk_inst ( inst );

  return true;
  
} // end inst_r16_m16


static bool
inst_r32_m32 (
              PROTO_DIS,
              const IA32_Mnemonic name
              )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 1, 0 ) )
    return false;
  if ( IS_REG32 ( inst->ops[1].type ) )
    return unk_inst ( inst );

  return true;
  
} // end inst_r32_m32


static bool
inst_r_m (
          PROTO_DIS,
          const IA32_Mnemonic name16,
          const IA32_Mnemonic name32
          )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_r32_m32 ( DIS, name32 );
  else                      ret= inst_r16_m16 ( DIS, name16 );

  return ret;
  
} // end inst_r_m


static bool
inst_r32 (
          PROTO_DIS,
          const IA32_Mnemonic name,
          const uint8_t       opcode
          )
{

  inst->name= name;
  switch ( opcode&0x7 )
    {
    case 0: inst->ops[0].type= IA32_EAX; break;
    case 1: inst->ops[0].type= IA32_ECX; break;
    case 2: inst->ops[0].type= IA32_EDX; break;
    case 3: inst->ops[0].type= IA32_EBX; break;
    case 4: inst->ops[0].type= IA32_ESP; break;
    case 5: inst->ops[0].type= IA32_EBP; break;
    case 6: inst->ops[0].type= IA32_ESI; break;
    case 7: inst->ops[0].type= IA32_EDI; break;
    }
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_r32


static bool
inst_r16 (
          PROTO_DIS,
          const IA32_Mnemonic name,
          const uint8_t       opcode
          )
{

  inst->name= name;
  switch ( opcode&0x7 )
    {
    case 0: inst->ops[0].type= IA32_AX; break;
    case 1: inst->ops[0].type= IA32_CX; break;
    case 2: inst->ops[0].type= IA32_DX; break;
    case 3: inst->ops[0].type= IA32_BX; break;
    case 4: inst->ops[0].type= IA32_SP; break;
    case 5: inst->ops[0].type= IA32_BP; break;
    case 6: inst->ops[0].type= IA32_SI; break;
    case 7: inst->ops[0].type= IA32_DI; break;
    }
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_r16


static bool
inst_r (
        PROTO_DIS,
        const IA32_Mnemonic name16,
        const IA32_Mnemonic name32,
        const uint8_t       opcode
        )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_r32 ( DIS, name32, opcode );
  else                      ret= inst_r16 ( DIS, name16, opcode );

  return ret;
  
} // end inst_r


static bool
inst_r_A (
          PROTO_DIS,
          const IA32_Mnemonic name16,
          const IA32_Mnemonic name32,
          const uint8_t       opcode
          )
{
  
  if ( OPERAND_SIZE_IS_32 )
    {
      if ( !inst_r32 ( DIS, name32, opcode ) )
        return false;
      inst->ops[1].type= IA32_EAX;
    }
  else
    {
      if ( !inst_r16 ( DIS, name16, opcode ) )
        return false;
      inst->ops[1].type= IA32_AX;
    }

  return true;
  
} // end inst_r_A


static bool
inst_rel8_opsize (
                  PROTO_DIS,
                  const IA32_Mnemonic name16,
                  const IA32_Mnemonic name32
                  )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].type= IA32_REL8;
  if ( !read_op_u8 ( DIS, 0 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_rel8_opsize


static bool
inst_rel16 (
            PROTO_DIS,
            const IA32_Mnemonic name
            )
{

  inst->name= name;
  inst->ops[0].type= IA32_REL16;
  if ( !read_op_u16 ( DIS, 0 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_rel16


static bool
inst_rel32 (
            PROTO_DIS,
            const IA32_Mnemonic name
            )
{
  
  inst->name= name;
  inst->ops[0].type= IA32_REL32;
  if ( !read_op_u32 ( DIS, 0 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_rel32


static bool
inst_rel (
          PROTO_DIS,
          const IA32_Mnemonic name16,
          const IA32_Mnemonic name32
          )
{
  if ( OPERAND_SIZE_IS_32 ) return inst_rel32 ( DIS, name32 );
  else                      return inst_rel16 ( DIS, name16 );
} // end inst_rel


static bool
inst_loop_like (
                PROTO_DIS,
                const IA32_Mnemonic name16,
                const IA32_Mnemonic name32
                )
{
  
  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].type= IA32_REL8;
  if ( !read_op_u8 ( DIS, 0 ) )
    return false;
  inst->ops[1].type= ADDRESS_SIZE_IS_32 ? IA32_ECX : IA32_CX;

  return true;
  
} // end inst_loop


static bool
inst_rm8_nomodrmbyte (
                      PROTO_DIS,
                      const IA32_Mnemonic name
                      )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  inst->name= name;
  if ( !get_addr_mode_r8 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_rm8_nomodrmbyte


static bool
inst_rm8 (
          PROTO_DIS,
          const uint8_t       modrmbyte,
          const IA32_Mnemonic name
          )
{
  
  inst->name= name;
  if ( !get_addr_mode_r8 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_rm8


static bool
inst_rm16 (
           PROTO_DIS,
           const uint8_t       modrmbyte,
           const IA32_Mnemonic name
           )
{
  
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_rm16


static bool
inst_rm32 (
           PROTO_DIS,
           const uint8_t       modrmbyte,
           const IA32_Mnemonic name
           )
{
  
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_rm32


static bool
inst_r32m16 (
             PROTO_DIS,
             const uint8_t       modrmbyte,
             const IA32_Mnemonic name16,
             const IA32_Mnemonic name32
             )
{
  
  if ( !get_addr_mode_r32_m16 ( DIS, modrmbyte, 0, name16, name32 ) )
    return false;
  inst->ops[1].type= IA32_NONE;
  
  return true;
  
} // end inst_r32m16


static bool
inst_rm (
         PROTO_DIS,
         const uint8_t       modrmbyte,
         const IA32_Mnemonic name16,
         const IA32_Mnemonic name32
         )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_rm32 ( DIS, modrmbyte, name32 );
  else                      ret= inst_rm16 ( DIS, modrmbyte, name16 );

  return ret;
  
} // end inst_rm


static bool
inst_rm__m16 (
              PROTO_DIS,
              const uint8_t       modrmbyte,
              const IA32_Mnemonic name16,
              const IA32_Mnemonic name32
              )
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_r32m16 ( DIS, modrmbyte, name16, name32 );
  else                      ret= inst_rm16 ( DIS, modrmbyte, name16 );
  
  return ret;
  
} // end inst_rm__m16


static bool
inst_ptr16_16 (
               PROTO_DIS,
               const IA32_Mnemonic name
               )
{

  inst->name= name;
  inst->ops[0].type= IA32_PTR16_16;
  if ( !read_op_u16 ( DIS, 0 ) )
    return false;
  if ( !read_op_ptr16 ( DIS, 0 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_ptr16_16


static bool
inst_ptr16_32 (
               PROTO_DIS,
               const IA32_Mnemonic name
               )
{

  inst->name= name;
  inst->ops[0].type= IA32_PTR16_32;
  if ( !read_op_u32 ( DIS, 0 ) )
    return false;
  if ( !read_op_ptr16 ( DIS, 0 ) )
    return false;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_ptr16_32


static bool
inst_ptr16_off (
        	PROTO_DIS,
        	const IA32_Mnemonic name16,
                const IA32_Mnemonic name32
        	)
{

  bool ret;

  
  if ( OPERAND_SIZE_IS_32 ) ret= inst_ptr16_32 ( DIS, name32 );
  else                      ret= inst_ptr16_16 ( DIS, name16 );

  return ret;
  
} // end inst_ptr16_off


static bool
inst_m16 (
          PROTO_DIS,
          const uint8_t       modrmbyte,
          const IA32_Mnemonic name
          )
{
  
  inst->name= name;
  if ( !get_addr_mode_r16 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  if ( inst->ops[0].type >= IA32_AX && inst->ops[0].type <= IA32_DI )
    return unk_inst ( inst );
  else
    inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_m16


static bool
inst_m32 (
          PROTO_DIS,
          const uint8_t       modrmbyte,
          const IA32_Mnemonic name
          )
{
  
  inst->name= name;
  if ( !get_addr_mode_r32 ( DIS, modrmbyte, 0, -1 ) )
    return false;
  if ( inst->ops[0].type >= IA32_EAX && inst->ops[0].type <= IA32_EDI )
    return unk_inst ( inst );
  else
    inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_m32


static bool
inst_m (
        PROTO_DIS,
        const uint8_t       modrmbyte,
        const IA32_Mnemonic name16,
        const IA32_Mnemonic name32
        )
{
  if ( OPERAND_SIZE_IS_32 ) return inst_m32 ( DIS, modrmbyte, name32 );
  else                      return inst_m16 ( DIS, modrmbyte, name16 );
} // end inst_m


static bool
inst_none (
           PROTO_DIS,
           const IA32_Mnemonic name
           )
{
  
  inst->name= name;
  inst->ops[0].type= IA32_NONE;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_none


static bool
inst_AL_moffs8 (
                PROTO_DIS,
                const IA32_Mnemonic name
                )
{

  inst->name= name;
  inst->ops[0].type= IA32_AL;
  if ( !get_addr_moffs ( DIS, 1 ) )
    return false;

  return true;
  
} // end inst_AL_moffs8


static bool
inst_A_moffs (
              PROTO_DIS,
              const IA32_Mnemonic name16,
              const IA32_Mnemonic name32
              )
{

  if ( OPERAND_SIZE_IS_32 )
    {
      inst->name= name32;
      inst->ops[0].type= IA32_EAX;
    }
  else
    {
      inst->name= name16;
      inst->ops[0].type= IA32_AX;
    }
  if ( !get_addr_moffs ( DIS, 1 ) )
    return false;

  return true;
  
} // end inst_A_moffs


static bool
inst_moffs8_AL (
                PROTO_DIS,
                const IA32_Mnemonic name
                )
{

  inst->name= name;
  inst->ops[1].type= IA32_AL;
  if ( !get_addr_moffs ( DIS, 0 ) )
    return false;

  return true;
  
} // end inst_moffs8_AL


static bool
inst_moffs_A (
              PROTO_DIS,
              const IA32_Mnemonic name16,
              const IA32_Mnemonic name32
              )
{

  if ( OPERAND_SIZE_IS_32 )
    {
      inst->name= name32;
      inst->ops[1].type= IA32_EAX;
    }
  else
    {
      inst->name= name16;
      inst->ops[1].type= IA32_AX;
    }
  if ( !get_addr_moffs ( DIS, 0 ) )
    return false;

  return true;
  
} // end inst_moffs_A


static bool
inst_fpu_stack_pos (
                    PROTO_DIS,
                    const IA32_Mnemonic name,
                    const uint8_t       modrmbyte
                    )
{

  inst->name= name;
  inst->ops[0].type= IA32_FPU_STACK_POS;
  inst->ops[0].fpu_stack_pos= modrmbyte&0x7;
  inst->ops[1].type= IA32_NONE;

  return true;
  
} // end inst_fpu_stack_pos


static bool
inst_fpu_st0_stack_pos (
                        PROTO_DIS,
                        const IA32_Mnemonic name,
                        const uint8_t       modrmbyte
                        )
{

  inst->name= name;
  inst->ops[0].type= IA32_FPU_STACK_POS;
  inst->ops[0].fpu_stack_pos= 0;
  inst->ops[1].type= IA32_FPU_STACK_POS;
  inst->ops[1].fpu_stack_pos= modrmbyte&0x7;

  return true;
  
} // end inst_fpu_st0_stack_pos


static bool
inst_fpu_stack_pos_st0 (
                        PROTO_DIS,
                        const IA32_Mnemonic name,
                        const uint8_t       modrmbyte
                        )
{

  inst->name= name;
  inst->ops[0].type= IA32_FPU_STACK_POS;
  inst->ops[0].fpu_stack_pos= modrmbyte&0x7;
  inst->ops[1].type= IA32_FPU_STACK_POS;
  inst->ops[1].fpu_stack_pos= 0;

  return true;
  
} // end inst_fpu_stack_pos_st0


static bool
mov_r8_imm8 (
             PROTO_DIS,
             const uint8_t opcode
             )
{

  inst->name= IA32_MOV8;
  inst->ops[1].type= IA32_IMM8;
  if ( !read_op_u8 ( DIS, 1 ) )
    return false;
  switch ( opcode&0x7 )
    {
    case 0: inst->ops[0].type= IA32_AL; break;
    case 1: inst->ops[0].type= IA32_CL; break;
    case 2: inst->ops[0].type= IA32_DL; break;
    case 3: inst->ops[0].type= IA32_BL; break;
    case 4: inst->ops[0].type= IA32_AH; break;
    case 5: inst->ops[0].type= IA32_CH; break;
    case 6: inst->ops[0].type= IA32_DH; break;
    case 7: inst->ops[0].type= IA32_BH; break;
    }

  return true;
  
} // end mov_r8_imm8


static bool
mov_r_imm (
           PROTO_DIS,
           const uint8_t opcode
           )
{

  if ( OPERAND_SIZE_IS_32 )
    {
      inst->name= IA32_MOV32;
      inst->ops[1].type= IA32_IMM32;
      if ( !read_op_u32 ( DIS, 1 ) )
        return false;
      switch ( opcode&0x7 )
        {
        case 0: inst->ops[0].type= IA32_EAX; break;
        case 1: inst->ops[0].type= IA32_ECX; break;
        case 2: inst->ops[0].type= IA32_EDX; break;
        case 3: inst->ops[0].type= IA32_EBX; break;
        case 4: inst->ops[0].type= IA32_ESP; break;
        case 5: inst->ops[0].type= IA32_EBP; break;
        case 6: inst->ops[0].type= IA32_ESI; break;
        case 7: inst->ops[0].type= IA32_EDI; break;
        }
    }
  else
    {
      inst->name= IA32_MOV16;
      inst->ops[1].type= IA32_IMM16;
      if ( !read_op_u16 ( DIS, 1 ) )
        return false;
      switch ( opcode&0x7 )
        {
        case 0: inst->ops[0].type= IA32_AX; break;
        case 1: inst->ops[0].type= IA32_CX; break;
        case 2: inst->ops[0].type= IA32_DX; break;
        case 3: inst->ops[0].type= IA32_BX; break;
        case 4: inst->ops[0].type= IA32_SP; break;
        case 5: inst->ops[0].type= IA32_BP; break;
        case 6: inst->ops[0].type= IA32_SI; break;
        case 7: inst->ops[0].type= IA32_DI; break;
        }
    }

  return true;
  
} // end mov_r_imm


// Es pot gastar per a totes les grandàries
static bool
movs (
      PROTO_DIS,
      IA32_Mnemonic name
      )
{

  inst->name= name;
  inst->ops[1].data_seg= IA32_DS;
  OVERRIDE_SEG(inst->ops[1].data_seg);
  inst->ops[0].data_seg= IA32_ES;
  if ( ADDRESS_SIZE_IS_32 )
    {
      inst->ops[0].type= IA32_ADDR32_EDI;
      inst->ops[1].type= IA32_ADDR32_ESI;
    }
  else
    {
      inst->ops[0].type= IA32_ADDR16_DI;
      inst->ops[1].type= IA32_ADDR16_SI;
    }
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end movs


// Es pot gastar per a totes les grandàries
static bool
cmps (
      PROTO_DIS,
      IA32_Mnemonic name16,
      IA32_Mnemonic name32
      )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].data_seg= IA32_DS;
  OVERRIDE_SEG(inst->ops[0].data_seg);
  inst->ops[1].data_seg= IA32_ES;
  if ( ADDRESS_SIZE_IS_32 )
    {
      inst->ops[1].type= IA32_ADDR32_EDI;
      inst->ops[0].type= IA32_ADDR32_ESI;
    }
  else
    {
      inst->ops[1].type= IA32_ADDR16_DI;
      inst->ops[0].type= IA32_ADDR16_SI;
    }
  if ( dis->_repe_repz_enabled )
    inst->prefix= IA32_PREFIX_REPE;
  else if ( dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REPNE;

  return true;
  
} // end cmps


// Es pot gastar per a totes les grandàries
static bool
scas (
       PROTO_DIS,
       IA32_Mnemonic name16,
       IA32_Mnemonic name32
       )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[1].type= IA32_NONE;
  inst->ops[0].data_seg= IA32_ES;
  if ( ADDRESS_SIZE_IS_32 )
    inst->ops[0].type= IA32_ADDR32_EDI;
  else
    inst->ops[0].type= IA32_ADDR16_DI;
  if ( dis->_repe_repz_enabled )
    inst->prefix= IA32_PREFIX_REPE;
  else if ( dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REPNE;

  return true;
  
} // end scas


static bool
lodsb (
       PROTO_DIS,
       IA32_Mnemonic name
       )
{

  inst->name= name;
  inst->ops[1].data_seg= IA32_DS;
  OVERRIDE_SEG(inst->ops[1].data_seg);
  inst->ops[0].type= IA32_AL;
  if ( ADDRESS_SIZE_IS_32 )
    inst->ops[1].type= IA32_ADDR32_ESI;
  else
    inst->ops[1].type= IA32_ADDR16_SI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end lodsb


static bool
lods (
      PROTO_DIS,
      IA32_Mnemonic name16,
      IA32_Mnemonic name32
      )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[1].data_seg= IA32_DS;
  OVERRIDE_SEG(inst->ops[1].data_seg);
  inst->ops[0].type= OPERAND_SIZE_IS_32 ? IA32_EAX : IA32_AX;
  if ( ADDRESS_SIZE_IS_32 )
    inst->ops[1].type= IA32_ADDR32_ESI;
  else
    inst->ops[1].type= IA32_ADDR16_SI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end lods


static bool
outsb (
       PROTO_DIS,
       IA32_Mnemonic name
       )
{
  
  inst->name= name;
  inst->ops[1].data_seg= IA32_DS;
  OVERRIDE_SEG(inst->ops[1].data_seg);
  inst->ops[0].type= IA32_DX;
  if ( ADDRESS_SIZE_IS_32 )
    inst->ops[1].type= IA32_ADDR32_ESI;
  else
    inst->ops[1].type= IA32_ADDR16_SI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end outsb


static bool
outs (
      PROTO_DIS,
      IA32_Mnemonic name16,
      IA32_Mnemonic name32
      )
{

  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[1].data_seg= IA32_DS;
  OVERRIDE_SEG(inst->ops[1].data_seg);
  inst->ops[0].type= IA32_DX;
  if ( ADDRESS_SIZE_IS_32 )
    inst->ops[1].type= IA32_ADDR32_ESI;
  else
    inst->ops[1].type= IA32_ADDR16_SI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end outs


static bool
movs_opsize (
             PROTO_DIS,
             IA32_Mnemonic name16,
             IA32_Mnemonic name32
             )
{
  return OPERAND_SIZE_IS_32 ? movs ( DIS, name32 ) : movs ( DIS, name16 );
} // end movs_opsize


static bool
stosb (
       PROTO_DIS,
       IA32_Mnemonic name
       )
{

  inst->name= name;
  inst->ops[1].type= IA32_AL;
  inst->ops[0].data_seg= IA32_ES;
  inst->ops[0].type= ADDRESS_SIZE_IS_32 ? IA32_ADDR32_EDI : IA32_ADDR16_DI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end stosb


static bool
stos (
      PROTO_DIS,
      IA32_Mnemonic name16,
      IA32_Mnemonic name32
      )
{

  if ( OPERAND_SIZE_IS_32 )
    {
      inst->name= name32;
      inst->ops[1].type= IA32_EAX;
    }
  else
    {
      inst->name= name16;
      inst->ops[1].type= IA32_AX;
    }
  inst->ops[0].data_seg= IA32_ES;
  inst->ops[0].type= ADDRESS_SIZE_IS_32 ? IA32_ADDR32_EDI : IA32_ADDR16_DI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end stos


static bool
insb (
      PROTO_DIS,
      IA32_Mnemonic name
      )
{

  inst->ops[1].type= IA32_DX;
  inst->name= name;
  inst->ops[0].data_seg= IA32_ES;
  inst->ops[0].type= ADDRESS_SIZE_IS_32 ? IA32_ADDR32_EDI : IA32_ADDR16_DI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end insb


static bool
ins (
     PROTO_DIS,
     IA32_Mnemonic name16,
     IA32_Mnemonic name32
     )
{

  inst->ops[1].type= IA32_DX;
  inst->name= OPERAND_SIZE_IS_32 ? name32 : name16;
  inst->ops[0].data_seg= IA32_ES;
  inst->ops[0].type= ADDRESS_SIZE_IS_32 ? IA32_ADDR32_EDI : IA32_ADDR16_DI;
  if ( dis->_repe_repz_enabled || dis->_repne_repnz_enabled )
    inst->prefix= IA32_PREFIX_REP;

  return true;
  
} // end ins


static bool
inst_0f_ba (
            PROTO_DIS
            )
{

  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 4: return inst_rm_imm8 ( DIS, modrmbyte, IA32_BT16, IA32_BT32 );
    case 5: return inst_rm_imm8 ( DIS, modrmbyte, IA32_BTS16, IA32_BTS32 );
    case 6: return inst_rm_imm8 ( DIS, modrmbyte, IA32_BTR16, IA32_BTR32 );
    case 7: return inst_rm_imm8 ( DIS, modrmbyte, IA32_BTC16, IA32_BTC32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_0f_ba


static bool
inst_0f_00 (
            PROTO_DIS
            )
{

  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm16 ( DIS, modrmbyte, IA32_SLDT );
    case 1: return inst_rm16 ( DIS, modrmbyte, IA32_STR );
    case 2: return inst_rm16 ( DIS, modrmbyte, IA32_LLDT );
    case 3: return inst_rm16 ( DIS, modrmbyte, IA32_LTR );
    case 4: return inst_rm16 ( DIS, modrmbyte, IA32_VERR );
    case 5: return inst_rm16 ( DIS, modrmbyte, IA32_VERW );
      
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_0f_00


static bool
inst_0f_01 (
            PROTO_DIS
            )
{

  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0:
      if ( (modrmbyte>>6) == 0x3 )
        return unk_inst ( inst );
      else return inst_m ( DIS, modrmbyte, IA32_SGDT16, IA32_SGDT32 );
      break;
    case 1:
      if ( (modrmbyte>>6) == 0x3 )
        return unk_inst ( inst );
      else return inst_m ( DIS, modrmbyte, IA32_SIDT16, IA32_SIDT32 );
      break;
    case 2:
      if ( (modrmbyte>>6) == 0x3 )
        return unk_inst ( inst );
      else return inst_m ( DIS, modrmbyte, IA32_LGDT16, IA32_LGDT32 );
      break;
    case 3:
      if ( (modrmbyte>>6) == 0x3 )
        return unk_inst ( inst );
      else return inst_m ( DIS, modrmbyte, IA32_LIDT16, IA32_LIDT32 );
      break;
    case 4: return inst_rm__m16 ( DIS, modrmbyte, IA32_SMSW16, IA32_SMSW32 );
      
    case 6: return inst_rm16 ( DIS, modrmbyte, IA32_LMSW );
    case 7:
      if ( (modrmbyte>>6) == 0x3 )
        return unk_inst ( inst );
      else return inst_m ( DIS, modrmbyte, IA32_INVLPG16, IA32_INVLPG32 );
      break;
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_0f_01


static bool
inst_0f (
         PROTO_DIS
         )
{

  uint8_t opcode;


  MEM ( (*offset)++, &opcode, return false );
  inst->bytes[inst->nbytes++]= opcode;
  ++(inst->real_nbytes);
  switch ( opcode )
    {
    case 0x00: return inst_0f_00 ( DIS );
    case 0x01: return inst_0f_01 ( DIS );
    case 0x02: return inst_r_rm ( DIS, IA32_LAR16, IA32_LAR32 );
    case 0x03: return inst_r_rm ( DIS, IA32_LSL16, IA32_LSL32 );

    case 0x06: return inst_no_op_nosize ( DIS, IA32_CLTS );
        
    case 0x09: return inst_no_op_nosize ( DIS, IA32_WBINVD );
        
    case 0x20: return inst_r32_cr ( DIS, IA32_MOV32 );
    case 0x21: return inst_r32_dr ( DIS, IA32_MOV32 );
    case 0x22: return inst_cr_r32 ( DIS, IA32_MOV32 );
    case 0x23: return inst_dr_r32 ( DIS, IA32_MOV32 );
      
      /*
    case 0x54: inst_rxmm_rmxmm ( DIS, IA32_ANDPS );
    case 0x55: inst_rxmm_rmxmm ( DIS, IA32_ANDNPS );

    case 0x58: inst_rxmm_rmxmm ( DIS, IA32_ADDPS );
      */

    case 0x80: return inst_rel ( DIS, IA32_JO16, IA32_JO32 );
        
    case 0x82: return inst_rel ( DIS, IA32_JB16, IA32_JB32 );
    case 0x83: return inst_rel ( DIS, IA32_JAE16, IA32_JAE32 );
    case 0x84: return inst_rel ( DIS, IA32_JE16, IA32_JE32 );
    case 0x85: return inst_rel ( DIS, IA32_JNE16, IA32_JNE32 );
    case 0x86: return inst_rel ( DIS, IA32_JNA16, IA32_JNA32 );
    case 0x87: return inst_rel ( DIS, IA32_JA16, IA32_JA32 );
    case 0x88: return inst_rel ( DIS, IA32_JS16, IA32_JS32 );
    case 0x89: return inst_rel ( DIS, IA32_JNS16, IA32_JNS32 );

    case 0x8c: return inst_rel ( DIS, IA32_JL16, IA32_JL32 );
    case 0x8d: return inst_rel ( DIS, IA32_JGE16, IA32_JGE32 );
    case 0x8e: return inst_rel ( DIS, IA32_JNG16, IA32_JNG32 );
    case 0x8f: return inst_rel ( DIS, IA32_JG16, IA32_JG32 );

    case 0x92: return inst_rm8_nomodrmbyte ( DIS, IA32_SETB );
    case 0x93: return inst_rm8_nomodrmbyte ( DIS, IA32_SETAE );
    case 0x94: return inst_rm8_nomodrmbyte ( DIS, IA32_SETE );
    case 0x95: return inst_rm8_nomodrmbyte ( DIS, IA32_SETNE );
    case 0x96: return inst_rm8_nomodrmbyte ( DIS, IA32_SETNA );
    case 0x97: return inst_rm8_nomodrmbyte ( DIS, IA32_SETA );
    case 0x98: return inst_rm8_nomodrmbyte ( DIS, IA32_SETS );

    case 0x9c: return inst_rm8_nomodrmbyte ( DIS, IA32_SETL );
    case 0x9d: return inst_rm8_nomodrmbyte ( DIS, IA32_SETGE );
    case 0x9e: return inst_rm8_nomodrmbyte ( DIS, IA32_SETNG );
    case 0x9f: return inst_rm8_nomodrmbyte ( DIS, IA32_SETG );
    case 0xa0: return inst_FS ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0xa1: return inst_FS ( DIS, IA32_POP16, IA32_POP32 );
    case 0xa2: return inst_no_op_nosize ( DIS, IA32_CPUID );
    case 0xa3: return inst_rm_r ( DIS, IA32_BT16, IA32_BT32 );
    case 0xa4: return inst_rm_r_imm8 ( DIS, IA32_SHLD16, IA32_SHLD32 );
    case 0xa5: return inst_rm_r_CL ( DIS, IA32_SHLD16, IA32_SHLD32 );

    case 0xa8: return inst_GS ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0xa9: return inst_GS ( DIS, IA32_POP16, IA32_POP32 );
        
    case 0xab: return inst_rm_r ( DIS, IA32_BTS16, IA32_BTS32 );
    case 0xac: return inst_rm_r_imm8 ( DIS, IA32_SHRD16, IA32_SHRD32 );
    case 0xad: return inst_rm_r_CL ( DIS, IA32_SHRD16, IA32_SHRD32 );

    case 0xaf: return inst_r_rm ( DIS, IA32_IMUL16, IA32_IMUL32 );

    case 0xb2: return inst_r_m ( DIS, IA32_LSS16, IA32_LSS32 );
    case 0xb3: return inst_rm_r ( DIS, IA32_BTR16, IA32_BTR32 );
    case 0xb4: return inst_r_m ( DIS, IA32_LFS16, IA32_LFS32 );
    case 0xb5: return inst_r_m ( DIS, IA32_LGS16, IA32_LGS32 );
    case 0xb6: return inst_r_rm8 ( DIS, IA32_MOVZX16, IA32_MOVZX32B );
    case 0xb7: return inst_r32_rm16 ( DIS, IA32_MOVZX32W );
      
    case 0xba: return inst_0f_ba ( DIS );
      /*
    case 0xbb: return inst_rm_r ( DIS, IA32_BTC );
      */
    case 0xbc: return inst_r_rm ( DIS, IA32_BSF16, IA32_BSF32 );
    case 0xbd: return inst_r_rm ( DIS, IA32_BSR16, IA32_BSR32 );
    case 0xbe: return inst_r_rm8 ( DIS, IA32_MOVSX16, IA32_MOVSX32B );
    case 0xbf: return inst_r32_rm16 ( DIS, IA32_MOVSX32W );
      
    case 0xc8 ... 0xcf: return inst_r32 ( DIS, IA32_BSWAP, opcode );
      
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_0f


static bool
override_seg (
              PROTO_DIS,
              const IA32_SegmentName seg
              )
{

  uint8_t opcode;

  
  MEM ( (*offset)++, &opcode, return false );
  ++(inst->real_nbytes);
  if ( dis->_group2_inst ) // Maxaca
    {
      inst->bytes[inst->nbytes-1]= opcode;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }
  else
    {
      inst->bytes[inst->nbytes++]= opcode;
      dis->_group2_inst= true;
      dis->_data_seg= seg;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }

  return true;
  
} // end override_seg


static bool
switch_op_size_and_other (
        		  PROTO_DIS
        		  )
{

  uint8_t opcode;


  MEM ( (*offset)++, &opcode, return false );
  ++(inst->real_nbytes);
  if ( dis->_group3_inst ) // Maxaca
    {
      inst->bytes[inst->nbytes-1]= opcode;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }
  else
    {
      inst->bytes[inst->nbytes++]= opcode;
      dis->_group3_inst= true;
      dis->_switch_operand_size= true;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }

  return true;
  
} // end switch_op_size_and_other


static bool
switch_addr_size (
        	  PROTO_DIS
        	  )
{

  uint8_t opcode;


  MEM ( (*offset)++, &opcode, return false );
  ++(inst->real_nbytes);
  if ( dis->_group4_inst ) // Maxaca
    {
      inst->bytes[inst->nbytes-1]= opcode;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }
  else
    {
      inst->bytes[inst->nbytes++]= opcode;
      dis->_group4_inst= true;
      dis->_switch_addr_size= true;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }

  return true;
  
} // end switch_addr_size


static bool
inst_80 (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;

  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_ADD8 );
    case 1: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_OR8 );
    case 2: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_ADC8 );
    case 3: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_SBB8 );
    case 4: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_AND8 );
    case 5: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_SUB8 );
    case 6: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_XOR8 );
    case 7: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_CMP8 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_80


static bool
inst_81 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm_imm ( DIS, modrmbyte, IA32_ADD16, IA32_ADD32 );
    case 1: return inst_rm_imm ( DIS, modrmbyte, IA32_OR16, IA32_OR32 );
    case 2: return inst_rm_imm ( DIS, modrmbyte, IA32_ADC16, IA32_ADC32 );
    case 3: return inst_rm_imm ( DIS, modrmbyte, IA32_SBB16, IA32_SBB32 );
    case 4: return inst_rm_imm ( DIS, modrmbyte, IA32_AND16, IA32_AND32 );
    case 5: return inst_rm_imm ( DIS, modrmbyte, IA32_SUB16, IA32_SUB32 );
    case 6: return inst_rm_imm ( DIS, modrmbyte, IA32_XOR16, IA32_XOR32 );
    case 7: return inst_rm_imm ( DIS, modrmbyte, IA32_CMP16, IA32_CMP32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_81 


static bool
inst_83 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm_imm8 ( DIS, modrmbyte, IA32_ADD16, IA32_ADD32 );
    case 1: return inst_rm_imm8 ( DIS, modrmbyte, IA32_OR16, IA32_OR32 );
    case 2: return inst_rm_imm8 ( DIS, modrmbyte, IA32_ADC16, IA32_ADC32 );
    case 3: return inst_rm_imm8 ( DIS, modrmbyte, IA32_SBB16, IA32_SBB32 );
    case 4: return inst_rm_imm8 ( DIS, modrmbyte, IA32_AND16, IA32_AND32 );
    case 5: return inst_rm_imm8 ( DIS, modrmbyte, IA32_SUB16, IA32_SUB32 );
    case 6: return inst_rm_imm8 ( DIS, modrmbyte, IA32_XOR16, IA32_XOR32 );
    case 7: return inst_rm_imm8 ( DIS, modrmbyte, IA32_CMP16, IA32_CMP32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_83


static bool
inst_8f (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm ( DIS, modrmbyte, IA32_POP16, IA32_POP32 );
      
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_8f


static bool
inst_c0 (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;

  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_ROL8 );
    case 1: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_ROR8 );
        
    case 4: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_SHL8 );
    case 5: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_SHR8 );

    case 7: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_SAR8 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_c0


static bool
inst_c1 (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;

  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm_imm8 ( DIS, modrmbyte, IA32_ROL16, IA32_ROL32 );
    case 1: return inst_rm_imm8 ( DIS, modrmbyte, IA32_ROR16, IA32_ROR32 );
    case 2: return inst_rm_imm8 ( DIS, modrmbyte, IA32_RCL16, IA32_RCL32 );
    case 3: return inst_rm_imm8 ( DIS, modrmbyte, IA32_RCR16, IA32_RCR32 );
    case 4: return inst_rm_imm8 ( DIS, modrmbyte, IA32_SHL16, IA32_SHL32 );
    case 5: return inst_rm_imm8 ( DIS, modrmbyte, IA32_SHR16, IA32_SHR32 );
      
    case 7: return inst_rm_imm8 ( DIS, modrmbyte, IA32_SAR16, IA32_SAR32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_c1


static bool
inst_c6 (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;

  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_MOV8 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_c6


static bool
inst_c7 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm_imm ( DIS, modrmbyte, IA32_MOV16, IA32_MOV32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_c7


static bool
inst_d0 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm8_1 ( DIS, modrmbyte, IA32_ROL8 );
    case 1: return inst_rm8_1 ( DIS, modrmbyte, IA32_ROR8 );
    case 2: return inst_rm8_1 ( DIS, modrmbyte, IA32_RCL8 );
    case 3: return inst_rm8_1 ( DIS, modrmbyte, IA32_RCR8 );
    case 4: return inst_rm8_1 ( DIS, modrmbyte, IA32_SHL8 );
    case 5: return inst_rm8_1 ( DIS, modrmbyte, IA32_SHR8 );
      
    case 7: return inst_rm8_1 ( DIS, modrmbyte, IA32_SAR8 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_d0


static bool
inst_d1 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm_1 ( DIS, modrmbyte, IA32_ROL16, IA32_ROL32 );
    case 1: return inst_rm_1 ( DIS, modrmbyte, IA32_ROR16, IA32_ROR32 );
    case 2: return inst_rm_1 ( DIS, modrmbyte, IA32_RCL16, IA32_RCL32 );
    case 3: return inst_rm_1 ( DIS, modrmbyte, IA32_RCR16, IA32_RCR32 );
    case 4: return inst_rm_1 ( DIS, modrmbyte, IA32_SHL16, IA32_SHL32 );
    case 5: return inst_rm_1 ( DIS, modrmbyte, IA32_SHR16, IA32_SHR32 );

    case 7: return inst_rm_1 ( DIS, modrmbyte, IA32_SAR16, IA32_SAR32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_d1


static bool
inst_d2 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm8_CL ( DIS, modrmbyte, IA32_ROL8 );
    case 1: return inst_rm8_CL ( DIS, modrmbyte, IA32_ROR8 );
    case 2: return inst_rm8_CL ( DIS, modrmbyte, IA32_RCL8 );
    case 3: return inst_rm8_CL ( DIS, modrmbyte, IA32_RCR8 );
    case 4: return inst_rm8_CL ( DIS, modrmbyte, IA32_SHL8 );
    case 5: return inst_rm8_CL ( DIS, modrmbyte, IA32_SHR8 );
      
    case 7: return inst_rm8_CL ( DIS, modrmbyte, IA32_SAR8 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_d2


static bool
inst_d3 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);

  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm_CL ( DIS, modrmbyte, IA32_ROL16, IA32_ROL32 );
    case 1: return inst_rm_CL ( DIS, modrmbyte, IA32_ROR16, IA32_ROR32 );
    case 2: return inst_rm_CL ( DIS, modrmbyte, IA32_RCL16, IA32_RCL32 );
    case 3: return inst_rm_CL ( DIS, modrmbyte, IA32_RCR16, IA32_RCR32 );
    case 4: return inst_rm_CL ( DIS, modrmbyte, IA32_SHL16, IA32_SHL32 );
    case 5: return inst_rm_CL ( DIS, modrmbyte, IA32_SHR16, IA32_SHR32 );

    case 7: return inst_rm_CL ( DIS, modrmbyte, IA32_SAR16, IA32_SAR32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_d3


static bool
inst_d8 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: return inst_rm32 ( DIS, modrmbyte, IA32_FADD32 );
        case 1: return inst_rm32 ( DIS, modrmbyte, IA32_FMUL32 );
        case 2: return inst_rm32 ( DIS, modrmbyte, IA32_FCOM32 );
        case 3: return inst_rm32 ( DIS, modrmbyte, IA32_FCOMP32 );
        case 4: return inst_rm32 ( DIS, modrmbyte, IA32_FSUB32 );
        case 5: return inst_rm32 ( DIS, modrmbyte, IA32_FSUBR32 );
        case 6: return inst_rm32 ( DIS, modrmbyte, IA32_FDIV32 ); 
        case 7: return inst_rm32 ( DIS, modrmbyte, IA32_FDIVR32 );
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7:
          return inst_fpu_st0_stack_pos ( DIS, IA32_FADD80, modrmbyte );
        case 0xc8 ... 0xcf:
          return inst_fpu_st0_stack_pos ( DIS, IA32_FMUL80, modrmbyte );
        case 0xd0 ... 0xd7:
          return inst_fpu_stack_pos ( DIS, IA32_FCOM80, modrmbyte );
        case 0xd8 ... 0xdf:
          return inst_fpu_stack_pos ( DIS, IA32_FCOMP80, modrmbyte );
        case 0xe0 ... 0xe7:
          return inst_fpu_st0_stack_pos ( DIS, IA32_FSUB80, modrmbyte );
        case 0xe8 ... 0xef:
          return inst_fpu_st0_stack_pos ( DIS, IA32_FSUBR80, modrmbyte );
        case 0xf0 ... 0xf7:
          return inst_fpu_st0_stack_pos ( DIS, IA32_FDIV80, modrmbyte );
        case 0xf8 ... 0xff:
          return inst_fpu_st0_stack_pos ( DIS, IA32_FDIVR80, modrmbyte );
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_d8


static bool
inst_d9 (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: return inst_rm32 ( DIS, modrmbyte, IA32_FLD32 );

        case 2: return inst_rm32 ( DIS, modrmbyte, IA32_FST32 );
        case 3: return inst_rm32 ( DIS, modrmbyte, IA32_FSTP32 );
            
        case 5: return inst_rm16 ( DIS, modrmbyte, IA32_FLDCW );
            
        case 7: return inst_rm16 ( DIS, modrmbyte, IA32_FSTCW );
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7:
          return inst_fpu_stack_pos ( DIS, IA32_FLD80, modrmbyte );
        case 0xc8 ... 0xcf:
          return inst_fpu_stack_pos ( DIS, IA32_FXCH, modrmbyte );
          
        case 0xe0: return inst_none ( DIS, IA32_FCHS );
        case 0xe1: return inst_none ( DIS, IA32_FABS );

        case 0xe4: return inst_none ( DIS, IA32_FTST );
        case 0xe5: return inst_none ( DIS, IA32_FXAM );

        case 0xe8: return inst_none ( DIS, IA32_FLD1 );

        case 0xea: return inst_none ( DIS, IA32_FLDL2E );
          
        case 0xed: return inst_none ( DIS, IA32_FLDLN2 );
        case 0xee: return inst_none ( DIS, IA32_FLDZ );

        case 0xf0: return inst_none ( DIS, IA32_F2XM1 );
        case 0xf1: return inst_none ( DIS, IA32_FYL2X );
        case 0xf2: return inst_none ( DIS, IA32_FPTAN );
        case 0xf3: return inst_none ( DIS, IA32_FPATAN );

        case 0xf8: return inst_none ( DIS, IA32_FPREM );

        case 0xfa: return inst_none ( DIS, IA32_FSQRT );
            
        case 0xfc: return inst_none ( DIS, IA32_FRNDINT );
        case 0xfd: return inst_none ( DIS, IA32_FSCALE );
        case 0xfe: return inst_none ( DIS, IA32_FSIN );
        case 0xff: return inst_none ( DIS, IA32_FCOS );
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_d9


static bool
inst_da (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {

        case 1: return inst_rm32 ( DIS, modrmbyte, IA32_FIMUL32 );
          
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_da


static bool
inst_db (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: return inst_rm32 ( DIS, modrmbyte, IA32_FILD32 );
            
        case 2: return inst_rm32 ( DIS, modrmbyte, IA32_FIST32 );
        case 3: return inst_rm32 ( DIS, modrmbyte, IA32_FISTP32 );
          
        case 5: return inst_rm32 ( DIS, modrmbyte, IA32_FLD80 );

        case 7: return inst_rm32 ( DIS, modrmbyte, IA32_FSTP80 );
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
          
        case 0xe2: return inst_no_op ( DIS, IA32_FCLEX, IA32_FCLEX );
        case 0xe3: return inst_no_op ( DIS, IA32_FINIT, IA32_FINIT );
        case 0xe4: return inst_no_op ( DIS, IA32_FSETPM, IA32_FSETPM );
          
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_db


static bool
inst_dc (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: return inst_rm32 ( DIS, modrmbyte, IA32_FADD64 );
        case 1: return inst_rm32 ( DIS, modrmbyte, IA32_FMUL64 );
        case 2: return inst_rm32 ( DIS, modrmbyte, IA32_FCOM64 );
        case 3: return inst_rm32 ( DIS, modrmbyte, IA32_FCOMP64 );
        case 4: return inst_rm32 ( DIS, modrmbyte, IA32_FSUB64 );
        case 5: return inst_rm32 ( DIS, modrmbyte, IA32_FSUBR64 );
        case 6: return inst_rm32 ( DIS, modrmbyte, IA32_FDIV64 );
        case 7: return inst_rm32 ( DIS, modrmbyte, IA32_FDIVR64 );
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {

        case 0xc8 ... 0xcf:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FMUL80, modrmbyte );

        case 0xe8 ... 0xef:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FSUB80, modrmbyte );
        case 0xf0 ... 0xf7:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FDIVR80, modrmbyte );
          
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_dc


static bool
inst_dd (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: return inst_rm32 ( DIS, modrmbyte, IA32_FLD64 );

        case 2: return inst_rm32 ( DIS, modrmbyte, IA32_FST64 );
        case 3: return inst_rm32 ( DIS, modrmbyte, IA32_FSTP64 );
        case 4: return inst_rm ( DIS, modrmbyte, IA32_FRSTOR16, IA32_FRSTOR32 );

        case 6: return inst_rm ( DIS, modrmbyte, IA32_FSAVE16, IA32_FSAVE32 );
        case 7: return inst_rm16 ( DIS, modrmbyte, IA32_FSTSW );
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7:
          return inst_fpu_stack_pos ( DIS, IA32_FFREE, modrmbyte );
          
        case 0xd0 ... 0xd7:
          return inst_fpu_stack_pos ( DIS, IA32_FST80, modrmbyte );
        case 0xd8 ... 0xdf:
          return inst_fpu_stack_pos ( DIS, IA32_FSTP80, modrmbyte );
          
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_dd


static bool
inst_de (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {
        case 0xc0 ... 0xc7:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FADDP80, modrmbyte );
        case 0xc8 ... 0xcf:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FMULP80, modrmbyte );
          
        case 0xd9: return inst_none ( DIS, IA32_FCOMPP );

        case 0xe0 ... 0xe7:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FSUBRP80, modrmbyte );
        case 0xe8 ... 0xef:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FSUBP80, modrmbyte );
        case 0xf0 ... 0xf7:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FDIVRP80, modrmbyte );
        case 0xf8 ... 0xff:
          return inst_fpu_stack_pos_st0 ( DIS, IA32_FDIVP80, modrmbyte );
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_de


static bool
inst_df (
         PROTO_DIS
         )
{
  
  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  if ( modrmbyte <= 0xbf )
    {
      switch ( (modrmbyte>>3)&0x7 )
        {
        case 0: return inst_rm16 ( DIS, modrmbyte, IA32_FILD16 );
          
        case 3: return inst_rm32 ( DIS, modrmbyte, IA32_FISTP16 );
            
        case 5: return inst_rm32 ( DIS, modrmbyte, IA32_FILD64 );
        case 6: return inst_rm32 ( DIS, modrmbyte, IA32_FBSTP );
        case 7: return inst_rm32 ( DIS, modrmbyte, IA32_FISTP64 );
        default: return unk_inst ( inst );
        }
    }
  else
    {
      switch ( modrmbyte )
        {

        case 0xe0: return inst_AX ( DIS, IA32_FNSTSW );
          
        default: return unk_inst ( inst );
        }
    }

  return true;
  
} // end inst_df

/*
static bool
lock (
      PROTO_DIS
      )
{
  
  uint8_t opcode;
  
  
  if ( dis->_group1_inst ) { unk_inst ( inst ); return false; }

  MEM ( (*offset)++, &opcode, return false );
  inst->bytes[inst->nbytes++]= opcode;
  ++(inst->real_nbytes);

  dis->_group1_inst= true;
  //inst->locked= true;
  if ( exec_inst ( DIS, opcode ) )
    return false;

  return true;
  
} // end lock
*/

static bool
repne_repnz_and_other(
        	      PROTO_DIS
        	      )
{
  
  uint8_t opcode;
  
  
  MEM ( (*offset)++, &opcode, return false );
  ++(inst->real_nbytes);
  if ( dis->_group1_inst ) // Maxaca
    {
      inst->bytes[inst->nbytes-1]= opcode;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }
  else
    {
      inst->bytes[inst->nbytes++]= opcode;
      dis->_group1_inst= true;
      dis->_repne_repnz_enabled= true;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }

  return true;
  
} // end repne_repnz_and_other


static bool
inst_f6 (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {

    case 0: return inst_rm8_imm8 ( DIS, modrmbyte, IA32_TEST8 );

    case 2: return inst_rm8 ( DIS, modrmbyte, IA32_NOT8 );
    case 3: return inst_rm8 ( DIS, modrmbyte, IA32_NEG8 );
    case 4: return inst_rm8 ( DIS, modrmbyte, IA32_MUL8 );
    case 5: return inst_rm8 ( DIS, modrmbyte, IA32_IMUL8 );
    case 6: return inst_rm8 ( DIS, modrmbyte, IA32_DIV8 );
    case 7: return inst_rm8 ( DIS, modrmbyte, IA32_IDIV8 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_f6


static bool
inst_f7 (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;
  
  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm_imm ( DIS, modrmbyte, IA32_TEST16, IA32_TEST32 );
      
    case 2: return inst_rm ( DIS, modrmbyte, IA32_NOT16, IA32_NOT32 );
    case 3: return inst_rm ( DIS, modrmbyte, IA32_NEG16, IA32_NEG32 );
    case 4: return inst_rm ( DIS, modrmbyte, IA32_MUL16, IA32_MUL32 );
    case 5: return inst_rm ( DIS, modrmbyte, IA32_IMUL16, IA32_IMUL32 );
    case 6: return inst_rm ( DIS, modrmbyte, IA32_DIV16, IA32_DIV32 );
    case 7: return inst_rm ( DIS, modrmbyte, IA32_IDIV16, IA32_IDIV32 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_f7


static bool
repe_repz_and_other(
        	    PROTO_DIS
        	    )
{
  
  uint8_t opcode;

  
  MEM ( (*offset)++, &opcode, return false );
  ++(inst->real_nbytes);
  if ( dis->_group1_inst )
    {
      inst->bytes[inst->nbytes-1]= opcode;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }
  else
    {
      inst->bytes[inst->nbytes++]= opcode;
      dis->_group1_inst= true;
      dis->_repe_repz_enabled= true;
      if ( !exec_inst ( DIS, opcode ) )
        return false;
    }

  return true;
  
} // end repe_repz_and_other


static bool
inst_fe (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;

  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm8 ( DIS, modrmbyte, IA32_INC8 );
    case 1: return inst_rm8 ( DIS, modrmbyte, IA32_DEC8 );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_fe


static bool
inst_ff (
         PROTO_DIS
         )
{

  uint8_t modrmbyte;

  
  MEM ( (*offset)++, &modrmbyte, return false );
  inst->bytes[inst->nbytes++]= modrmbyte;
  ++(inst->real_nbytes);
  
  switch ( (modrmbyte>>3)&0x7 )
    {
    case 0: return inst_rm ( DIS, modrmbyte, IA32_INC16, IA32_INC32 );
    case 1: return inst_rm ( DIS, modrmbyte, IA32_DEC16, IA32_DEC32 );
    case 2:
      return inst_rm ( DIS, modrmbyte,IA32_CALL16_NEAR, IA32_CALL32_NEAR );
    case 3: return inst_m ( DIS, modrmbyte, IA32_CALL16_FAR, IA32_CALL32_FAR );
    case 4: return inst_rm ( DIS, modrmbyte, IA32_JMP16_NEAR, IA32_JMP32_NEAR );
    case 5: return inst_m ( DIS, modrmbyte, IA32_JMP16_FAR, IA32_JMP32_FAR );
    case 6: return inst_rm ( DIS, modrmbyte, IA32_PUSH16, IA32_PUSH32 );
      
    default: return unk_inst ( inst );
    }

  return true;
  
} // end inst_ff


static bool
exec_inst (
           PROTO_DIS,
           const uint8_t opcode
           )
{

  switch ( opcode )
    {
    case 0x00: return inst_rm8_r8 ( DIS, IA32_ADD8 );
    case 0x01: return inst_rm_r ( DIS, IA32_ADD16, IA32_ADD32 );
    case 0x02: return inst_r8_rm8 ( DIS, IA32_ADD8 );
    case 0x03: return inst_r_rm ( DIS, IA32_ADD16, IA32_ADD32 );
    case 0x04: return inst_AL_imm8 ( DIS, IA32_ADD8 );
    case 0x05: return inst_A_imm ( DIS, IA32_ADD16, IA32_ADD32 );
    case 0x06: return inst_ES ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0x07: return inst_ES ( DIS, IA32_POP16, IA32_POP32 );
    case 0x08: return inst_rm8_r8 ( DIS, IA32_OR8 );
    case 0x09: return inst_rm_r ( DIS, IA32_OR16, IA32_OR32 );
    case 0x0a: return inst_r8_rm8 ( DIS, IA32_OR8 );
    case 0x0b: return inst_r_rm ( DIS, IA32_OR16, IA32_OR32 );
    case 0x0c: return inst_AL_imm8 ( DIS, IA32_OR8 );
    case 0x0d: return inst_A_imm ( DIS, IA32_OR16, IA32_OR32 );
    case 0x0e: return inst_CS ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0x0f: return inst_0f ( DIS );
    case 0x10: return inst_rm8_r8 ( DIS, IA32_ADC8 );
    case 0x11: return inst_rm_r ( DIS, IA32_ADC16, IA32_ADC32 );
    case 0x12: return inst_r8_rm8 ( DIS, IA32_ADC8 );
    case 0x13: return inst_r_rm ( DIS, IA32_ADC16, IA32_ADC32 );
    case 0x14: return inst_AL_imm8 ( DIS, IA32_ADC8 );
    case 0x15: return inst_A_imm ( DIS, IA32_ADC16, IA32_ADC32 );
    case 0x16: return inst_SS ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0x17: return inst_SS ( DIS, IA32_POP16, IA32_POP32 );
    case 0x18: return inst_rm8_r8 ( DIS, IA32_SBB8 );
    case 0x19: return inst_rm_r ( DIS, IA32_SBB16, IA32_SBB32 );
    case 0x1a: return inst_r8_rm8 ( DIS, IA32_SBB8 );
    case 0x1b: return inst_r_rm ( DIS, IA32_SBB16, IA32_SBB32 );
    case 0x1c: return inst_AL_imm8 ( DIS, IA32_SBB8 );
    case 0x1d: return inst_A_imm ( DIS, IA32_SBB16, IA32_SBB32 );
    case 0x1e: return inst_DS ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0x1f: return inst_DS ( DIS, IA32_POP16, IA32_POP32 );
    case 0x20: return inst_rm8_r8 ( DIS, IA32_AND8 );
    case 0x21: return inst_rm_r ( DIS, IA32_AND16, IA32_AND32 );
    case 0x22: return inst_r8_rm8 ( DIS, IA32_AND8 );
    case 0x23: return inst_r_rm ( DIS, IA32_AND16, IA32_AND32 );
    case 0x24: return inst_AL_imm8 ( DIS, IA32_AND8 );
    case 0x25: return inst_A_imm ( DIS, IA32_AND16, IA32_AND32 );
    case 0x26: return override_seg ( DIS, IA32_ES );
    case 0x27: return inst_no_op ( DIS, IA32_DAA, IA32_DAA );
    case 0x28: return inst_rm8_r8 ( DIS, IA32_SUB8 );
    case 0x29: return inst_rm_r ( DIS, IA32_SUB16, IA32_SUB32 );
    case 0x2a: return inst_r8_rm8 ( DIS, IA32_SUB8 );
    case 0x2b: return inst_r_rm ( DIS, IA32_SUB16, IA32_SUB32 );
    case 0x2c: return inst_AL_imm8 ( DIS, IA32_SUB8 );
    case 0x2d: return inst_A_imm ( DIS, IA32_SUB16, IA32_SUB32 );
    case 0x2e: return override_seg ( DIS, IA32_CS );
    case 0x2f: return inst_no_op ( DIS, IA32_DAS, IA32_DAS );
    case 0x30: return inst_rm8_r8 ( DIS, IA32_XOR8 );
    case 0x31: return inst_rm_r ( DIS, IA32_XOR16, IA32_XOR32 );
    case 0x32: return inst_r8_rm8 ( DIS, IA32_XOR8 );
    case 0x33: return inst_r_rm ( DIS, IA32_XOR16, IA32_XOR32 );
    case 0x34: return inst_AL_imm8 ( DIS, IA32_XOR8 );
    case 0x35: return inst_A_imm ( DIS, IA32_XOR16, IA32_XOR32 );
    case 0x36: return override_seg ( DIS, IA32_SS );
      /*
    case 0x37: return inst_no_op ( inst, IA32_AAA );
      */
    case 0x38: return inst_rm8_r8 ( DIS, IA32_CMP8 );
    case 0x39: return inst_rm_r ( DIS, IA32_CMP16, IA32_CMP32 );
    case 0x3a: return inst_r8_rm8 ( DIS, IA32_CMP8 );
    case 0x3b: return inst_r_rm ( DIS, IA32_CMP16, IA32_CMP32 );
    case 0x3c: return inst_AL_imm8 ( DIS, IA32_CMP8 );
    case 0x3d: return inst_A_imm ( DIS, IA32_CMP16, IA32_CMP32 );
    case 0x3e: return override_seg ( DIS, IA32_DS );
    case 0x3f: return inst_no_op_nosize ( DIS, IA32_AAS );
    case 0x40 ... 0x47: return inst_r ( DIS, IA32_INC16, IA32_INC32, opcode );
    case 0x48 ... 0x4f: return inst_r ( DIS, IA32_DEC16, IA32_DEC32, opcode );
    case 0x50 ... 0x57: return inst_r ( DIS, IA32_PUSH16, IA32_PUSH32, opcode );
    case 0x58 ... 0x5f: return inst_r ( DIS, IA32_POP16, IA32_POP32, opcode );
    case 0x60: return inst_no_op ( DIS, IA32_PUSHA16, IA32_PUSHA32 );
    case 0x61: return inst_no_op ( DIS, IA32_POPA16, IA32_POPA32 );
    case 0x62: return inst_r_m ( DIS, IA32_BOUND16, IA32_BOUND32 );
      /*
    case 0x63: return inst_rm16_r16 ( DIS, IA32_ARPL );
      */
    case 0x64: return override_seg ( DIS, IA32_FS );
    case 0x65: return override_seg ( DIS, IA32_GS );
    case 0x66: return switch_op_size_and_other ( DIS );
    case 0x67: return switch_addr_size ( DIS );
    case 0x68: return inst_imm ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0x69: return inst_r_rm_imm ( DIS, IA32_IMUL16, IA32_IMUL32 );
    case 0x6a: return inst_imm8 ( DIS, IA32_PUSH16, IA32_PUSH32 );
    case 0x6b: return inst_r_rm_imm8 ( DIS, IA32_IMUL16, IA32_IMUL32 );
    case 0x6c: return insb ( DIS, IA32_INS8 );
    case 0x6d: return ins ( DIS, IA32_INS16, IA32_INS32 );
    case 0x6e: return outsb ( DIS, IA32_OUTS8 );
    case 0x6f: return outs ( DIS, IA32_OUTS16, IA32_OUTS32 );
    case 0x70: return inst_rel8_opsize ( DIS, IA32_JO16, IA32_JO32 );
    case 0x71: return inst_rel8_opsize ( DIS, IA32_JNO16, IA32_JNO32 );
    case 0x72: return inst_rel8_opsize ( DIS, IA32_JB16, IA32_JB32 );
    case 0x73: return inst_rel8_opsize ( DIS, IA32_JAE16, IA32_JAE32 );
    case 0x74: return inst_rel8_opsize ( DIS, IA32_JE16, IA32_JE32 );
    case 0x75: return inst_rel8_opsize ( DIS, IA32_JNE16, IA32_JNE32 );
    case 0x76: return inst_rel8_opsize ( DIS, IA32_JNA16, IA32_JNA32 );
    case 0x77: return inst_rel8_opsize ( DIS, IA32_JA16, IA32_JA32 );
    case 0x78: return inst_rel8_opsize ( DIS, IA32_JS16, IA32_JS32 );
    case 0x79: return inst_rel8_opsize ( DIS, IA32_JNS16, IA32_JNS32 );
    case 0x7a: return inst_rel8_opsize ( DIS, IA32_JP16, IA32_JP32 );
    case 0x7b: return inst_rel8_opsize ( DIS, IA32_JPO16, IA32_JPO32 );
    case 0x7c: return inst_rel8_opsize ( DIS, IA32_JL16, IA32_JL32 );
    case 0x7d: return inst_rel8_opsize ( DIS, IA32_JGE16, IA32_JGE32 );
    case 0x7e: return inst_rel8_opsize ( DIS, IA32_JNG16, IA32_JNG32 );
    case 0x7f: return inst_rel8_opsize ( DIS, IA32_JG16, IA32_JG32 );
    case 0x80: return inst_80 ( DIS );
    case 0x81: return inst_81 ( DIS );
      
    case 0x83: return inst_83 ( DIS );
    case 0x84: return inst_rm8_r8 ( DIS, IA32_TEST8 );
    case 0x85: return inst_rm_r ( DIS, IA32_TEST16, IA32_TEST32 );
    case 0x86: return inst_r8_rm8 ( DIS, IA32_XCHG8 );
    case 0x87: return inst_r_rm ( DIS, IA32_XCHG16, IA32_XCHG32 );
    case 0x88: return inst_rm8_r8 ( DIS, IA32_MOV8 );
    case 0x89: return inst_rm_r ( DIS, IA32_MOV16, IA32_MOV32 );
    case 0x8a: return inst_r8_rm8 ( DIS, IA32_MOV8 );
    case 0x8b: return inst_r_rm ( DIS, IA32_MOV16, IA32_MOV32 );
    case 0x8c: return inst_rm_sreg ( DIS, IA32_MOV16, IA32_MOV32 );
    case 0x8d: return inst_r_m ( DIS, IA32_LEA16, IA32_LEA32 );
    case 0x8e: return inst_sreg_rm16 ( DIS, IA32_MOV16 );
    case 0x8f: return inst_8f ( DIS );
    case 0x90: return inst_none ( DIS, IA32_NOP );
    case 0x91 ... 0x97:
      return inst_r_A ( DIS, IA32_XCHG16, IA32_XCHG32, opcode );
     
    case 0x98: return inst_no_op ( DIS, IA32_CBW, IA32_CWDE );
    case 0x99: return inst_no_op ( DIS, IA32_CWD, IA32_CDQ );
    case 0x9a: return inst_ptr16_off ( DIS, IA32_CALL16_FAR, IA32_CALL32_FAR );
    case 0x9b: return inst_no_op ( DIS, IA32_FWAIT, IA32_FWAIT );
    case 0x9c: return inst_no_op ( DIS, IA32_PUSHF16, IA32_PUSHF32 );
    case 0x9d: return inst_no_op ( DIS, IA32_POPF16, IA32_POPF32 );
    case 0x9e: return inst_none ( DIS, IA32_SAHF );
    case 0x9f: return inst_none ( DIS, IA32_LAHF );
    case 0xa0: return inst_AL_moffs8 ( DIS, IA32_MOV8 );
    case 0xa1: return inst_A_moffs ( DIS, IA32_MOV16, IA32_MOV32 );
    case 0xa2: return inst_moffs8_AL ( DIS, IA32_MOV8 );
    case 0xa3: return inst_moffs_A ( DIS, IA32_MOV16, IA32_MOV32 );
    case 0xa4: return movs ( DIS, IA32_MOVS8 );
    case 0xa5: return movs_opsize ( DIS, IA32_MOVS16, IA32_MOVS32 );
    case 0xa6: return cmps ( DIS, IA32_CMPS8, IA32_CMPS8 );
    case 0xa7: return cmps ( DIS, IA32_CMPS16, IA32_CMPS32 );
    case 0xa8: return inst_AL_imm8 ( DIS, IA32_TEST8 );
    case 0xa9: return inst_A_imm ( DIS, IA32_TEST16, IA32_TEST32 );
    case 0xaa: return stosb ( DIS, IA32_STOS8 );
    case 0xab: return stos ( DIS, IA32_STOS16, IA32_STOS32 );
    case 0xac: return lodsb ( DIS, IA32_LODS8 );
    case 0xad: return lods ( DIS, IA32_LODS16, IA32_LODS32 );
    case 0xae: return scas ( DIS, IA32_SCAS8, IA32_SCAS8 );
    case 0xaf: return scas ( DIS, IA32_SCAS16, IA32_SCAS32 );
    case 0xb0 ... 0xb7: return mov_r8_imm8 ( DIS, opcode );
    case 0xb8 ... 0xbf: return mov_r_imm ( DIS, opcode );
    case 0xc0: return inst_c0 ( DIS );
    case 0xc1: return inst_c1 ( DIS );
    case 0xc2: return inst_imm16 ( DIS, IA32_RET16_NEAR, IA32_RET32_NEAR );
    case 0xc3: return inst_no_op ( DIS, IA32_RET16_NEAR, IA32_RET32_NEAR );
    case 0xc4: return inst_r_m ( DIS, IA32_LES16, IA32_LES32 );
    case 0xc5: return inst_r_m ( DIS, IA32_LDS16, IA32_LDS32 );
    case 0xc6: return inst_c6 ( DIS );
    case 0xc7: return inst_c7 ( DIS );
    case 0xc8: return inst_imm16_imm8 ( DIS, IA32_ENTER16, IA32_ENTER32 );
    case 0xc9: return inst_no_op ( DIS, IA32_LEAVE16, IA32_LEAVE32 );
    case 0xca: return inst_imm16 ( DIS, IA32_RET16_FAR, IA32_RET32_FAR );
    case 0xcb: return inst_no_op ( DIS, IA32_RET16_FAR, IA32_RET32_FAR );
    case 0xcc: return inst_3 ( DIS, IA32_INT16, IA32_INT32 );
    case 0xcd: return inst_imm8 ( DIS, IA32_INT16, IA32_INT32 );
    case 0xce: return inst_no_op ( DIS, IA32_INTO16, IA32_INTO32 );
    case 0xcf: return inst_no_op ( DIS, IA32_IRET16, IA32_IRET32 );
    case 0xd0: return inst_d0 ( DIS );
    case 0xd1: return inst_d1 ( DIS );
    case 0xd2: return inst_d2 ( DIS );
    case 0xd3: return inst_d3 ( DIS );
    case 0xd4: return inst_imm8 ( DIS, IA32_AAM, IA32_AAM );
    case 0xd5: return inst_imm8 ( DIS, IA32_AAD, IA32_AAD );
      
    case 0xd7: return inst_xlatb ( DIS, IA32_XLATB16, IA32_XLATB32 );
    case 0xd8: return inst_d8 ( DIS );
    case 0xd9: return inst_d9 ( DIS );
    case 0xda: return inst_da ( DIS );
    case 0xdb: return inst_db ( DIS );
    case 0xdc: return inst_dc ( DIS );
    case 0xdd: return inst_dd ( DIS );
    case 0xde: return inst_de ( DIS );
    case 0xdf: return inst_df ( DIS );
    case 0xe0: return inst_loop_like ( DIS, IA32_LOOPNE16, IA32_LOOPNE32 );
    case 0xe1: return inst_loop_like ( DIS, IA32_LOOPE16, IA32_LOOPE32 );
    case 0xe2: return inst_loop_like ( DIS, IA32_LOOP16, IA32_LOOP32 );
    case 0xe3:
      return inst_rel8_opsize ( DIS,
                                ADDRESS_SIZE_IS_32 ? IA32_JECXZ16 : IA32_JCXZ16,
                                ADDRESS_SIZE_IS_32 ? IA32_JECXZ32 : IA32_JCXZ32
                                );
    case 0xe4: return inst_AL_imm8 ( DIS, IA32_IN );
      
    case 0xe6: return inst_imm8_AL ( DIS, IA32_OUT );
    case 0xe7: return inst_imm8_A ( DIS, IA32_OUT, IA32_OUT );
    case 0xe8: return inst_rel ( DIS, IA32_CALL16_NEAR, IA32_CALL32_NEAR );
    case 0xe9: return inst_rel ( DIS, IA32_JMP16_NEAR, IA32_JMP32_NEAR );
    case 0xea: return inst_ptr16_off ( DIS, IA32_JMP16_FAR, IA32_JMP32_FAR );
    case 0xeb:
      return inst_rel8_opsize ( DIS, IA32_JMP16_NEAR, IA32_JMP32_NEAR );
    case 0xec: return inst_AL_DX ( DIS, IA32_IN );
    case 0xed: return inst_A_DX ( DIS, IA32_IN, IA32_IN );
    case 0xee: return inst_DX_AL ( DIS, IA32_OUT );
    case 0xef: return inst_DX_A ( DIS, IA32_OUT, IA32_OUT );
      //case 0xf0: return lock ( DIS );
      
    case 0xf2: return repne_repnz_and_other ( DIS );
    case 0xf3: return repe_repz_and_other ( DIS );
    case 0xf4: return inst_none ( DIS, IA32_HLT );
    case 0xf5: return inst_none ( DIS, IA32_CMC );
    case 0xf6: return inst_f6 ( DIS );
    case 0xf7: return inst_f7 ( DIS );
    case 0xf8: return inst_none ( DIS, IA32_CLC );
    case 0xf9: return inst_none ( DIS, IA32_STC );
    case 0xfa: return inst_none ( DIS, IA32_CLI );
    case 0xfb: return inst_none ( DIS, IA32_STI );
    case 0xfc: return inst_none ( DIS, IA32_CLD );
    case 0xfd: return inst_none ( DIS, IA32_STD );
    case 0xfe: return inst_fe ( DIS );
    case 0xff: return inst_ff ( DIS );
    default: return unk_inst ( inst );
    }

  return true;
  
} // end exec_inst




/**********************/
/* FUNCIONS PÚBLIQUES */
/**********************/

bool
IA32_dis (
          IA32_Disassembler *dis,
          uint64_t           offset,
          IA32_Inst         *inst
          )
{
  
  uint8_t opcode;
  
  
  // Inicialització.
  dis->_data_seg= IA32_UNK_SEG;
  dis->_group1_inst= false;
  dis->_group2_inst= false;
  dis->_group3_inst= false;
  dis->_group4_inst= false;
  dis->_switch_operand_size= false;
  dis->_switch_addr_size= false;
  dis->_repe_repz_enabled= false;
  dis->_repne_repnz_enabled= false;
  
  // Descodifica.
  inst->prefix= IA32_PREFIX_NONE;
  inst->nbytes= 0;
  MEM ( offset++, &opcode, return false );
  inst->bytes[inst->nbytes++]= opcode;
  inst->real_nbytes= 1;
  inst->ops[2].type= IA32_NONE; // El tercer operador és poc freqüent
  if ( !exec_inst ( dis, &offset, inst, opcode ) )
    return false;
  
  return true;
  
} // end IA32_dis
