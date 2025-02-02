/*
 * Copyright 2024-2025 Adrià Giménez Pastor.
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
 *  interpreter_cache.h - Conjunt d'instruccions relacionades amb la cache.
 *
 *  NOTA!!! De moment he decidit no emular la cache, per tant les
 *  instruccions ara mateixa no fan res.
 *
 */


/***********************************/
/* INVLPG - Invalidate TLB Entries */
/***********************************/

static void
invlpg_op16 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r16_t eaddr;

  
  // Obté dades
  if ( get_addr_mode_r16 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  if ( (EFLAGS&VM_FLAG) || (CPL != 0 && PROTECTED_MODE_ACTIVATED) )
    { EXCEPTION0 ( EXCP_GP ); return; }
  
  // ARA NO FA RES !!!!
  
} // end invlpg_op16


static void
invlpg_op32 (
             PROTO_INTERP,
             const uint8_t modrmbyte
             )
{

  eaddr_r32_t eaddr;


  // Obté dades
  if ( get_addr_mode_r32 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED || !eaddr.is_addr ) { EXCEPTION ( EXCP_UD ); return; }
  if ( (EFLAGS&VM_FLAG) || (CPL != 0 && PROTECTED_MODE_ACTIVATED) )
    { EXCEPTION0 ( EXCP_GP ); return; }
  
  // ARA NO FA RES !!!!
  
} // end invlpg_op32


static void
invlpg (
        PROTO_INTERP,
        const uint8_t modrmbyte
        )
{
  OPERAND_SIZE_IS_32 ?
    invlpg_op32 ( INTERP, modrmbyte ) :
    invlpg_op16 ( INTERP, modrmbyte );
} // end invlpg
