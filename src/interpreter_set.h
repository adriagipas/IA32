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
 *  interpreter_set.h - Implementa SETcc i semblants.
 *
 *  Adrià Giménez Pastor, 2020,2021.
 *
 */


/*********************************/
/* SETcc - Set Byte on Condition */
/*********************************/

static void
setcc_rm8 (
           PROTO_INTERP,
           const bool cond
           )
{

  uint8_t val,modrmbyte;
  eaddr_r8_t eaddr;


  IB ( &modrmbyte, return );
  if ( get_addr_mode_r8 ( INTERP, modrmbyte, &eaddr, NULL ) == -1 )
    return;
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  val= cond ? 0x01 : 0x00;
  if ( eaddr.is_addr )
    {
      WRITEB ( eaddr.v.addr.seg, eaddr.v.addr.off, val, return );
    }
  else *(eaddr.v.reg)= val;
  
} // end setcc_rm8


static void
seta_rm8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&(ZF_FLAG|CF_FLAG))==0);
  setcc_rm8 ( INTERP, cond );
} // end seta_rm8


static void
setae_rm8 (
           PROTO_INTERP
           )
{
  bool cond;
  cond= ((EFLAGS&(CF_FLAG))==0);
  setcc_rm8 ( INTERP, cond );
} // end setae_rm8


static void
setb_rm8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&CF_FLAG)!=0);
  setcc_rm8 ( INTERP, cond );
} // end setb_rm8


static void
sete_rm8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= ((EFLAGS&ZF_FLAG)!=0);
  setcc_rm8 ( INTERP, cond );
} // end sete_rm8


static void
setg_rm8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond=
    ((EFLAGS&ZF_FLAG)==0) &&
    !(((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  setcc_rm8 ( INTERP, cond );
} // end setg_rm8


static void
setge_rm8 (
           PROTO_INTERP
           )
{
  bool cond;
  cond= !(((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  setcc_rm8 ( INTERP, cond );
} // end setge_rm8


static void
setl_rm8 (
          PROTO_INTERP
          )
{
  bool cond;
  cond= (((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  setcc_rm8 ( INTERP, cond );
} // end setl_rm8


static void
setna_rm8 (
           PROTO_INTERP
           )
{
  bool cond;
  cond= ((EFLAGS&(ZF_FLAG|CF_FLAG))!=0);
  setcc_rm8 ( INTERP, cond );
} // end setna_rm8


static void
setne_rm8 (
           PROTO_INTERP
           )
{
  bool cond;
  cond= ((EFLAGS&ZF_FLAG)==0);
  setcc_rm8 ( INTERP, cond );
} // end setne_rm8


static void
setng_rm8 (
           PROTO_INTERP
           )
{
  bool cond;
  cond=
    ((EFLAGS&ZF_FLAG)!=0) ||
    (((EFLAGS&SF_FLAG)==0)^((EFLAGS&OF_FLAG)==0));
  setcc_rm8 ( INTERP, cond );
} // end setng_rm8


static void
sets_rm8 (
           PROTO_INTERP
           )
{
  bool cond;
  cond= ((EFLAGS&SF_FLAG)!=0);
  setcc_rm8 ( INTERP, cond );
} // end sets_rm8
