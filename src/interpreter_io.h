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
 *  interpreter_io.h - Operacions d'entrada/eixida.
 *
 */




/************************/
/* IN - Input from Port */
/************************/

static void
in_AL (
       PROTO_INTERP,
       const uint16_t port
       )
{

  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( CPL > IOPL || (EFLAGS&VM_FLAG) )
        {
          if ( check_io_permission_bit_map ( INTERP, port, 1 ) )
            AL= PORT_READ8 ( port );
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); }
        }
      else if ( CPL <= IOPL ) AL= PORT_READ8 ( port );
    }
  else AL= PORT_READ8 ( port );
  
} // end in_AL


static void
in_AX (
       PROTO_INTERP,
       const uint16_t port
       )
{

  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( CPL > IOPL || (EFLAGS&VM_FLAG) )
        {
          if ( check_io_permission_bit_map ( INTERP, port, 2 ) )
            AX= PORT_READ16 ( port );
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); }
        }
      else if ( CPL <= IOPL ) AX= PORT_READ16 ( port );
    }
  else AX= PORT_READ16 ( port );
  
} // end in_AX


static void
in_EAX (
        PROTO_INTERP,
        const uint16_t port
        )
{

  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( CPL > IOPL || (EFLAGS&VM_FLAG) )
        {
          if ( check_io_permission_bit_map ( INTERP, port, 4 ) )
            EAX= PORT_READ32 ( port );
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); }
        }
      else if ( CPL <= IOPL ) EAX= PORT_READ32 ( port );
    }
  else EAX= PORT_READ32 ( port );
  
} // end in_EAX


static void
in_AL_imm8 (
            PROTO_INTERP
            )
{

  uint8_t port;
  

  IB ( &port, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  in_AL ( INTERP, (uint16_t) port );
  
} // end in_AL_imm8


static void
in_AL_DX (
         PROTO_INTERP
         )
{
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  in_AL ( INTERP, DX );
} // end in_AL_DX


static void
in_A_DX (
         PROTO_INTERP
         )
{
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  OPERAND_SIZE_IS_32 ? in_EAX ( INTERP, DX ) : in_AX ( INTERP, DX );
} // end in_A_DX




/**************************************************/
/* INS/INSB/INSW/INSD - Input from Port to String */
/**************************************************/

static bool
ins_outs_check_permission (
                           PROTO_INTERP,
                           const uint16_t port,
                           const int      nbits
                           )
{

  bool ret;

  
  if ( PROTECTED_MODE_ACTIVATED &&
       (CPL > IOPL || (EFLAGS&VM_FLAG)) )
    {
      if ( check_io_permission_bit_map ( INTERP, port, nbits ) )
        ret= true;
      else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); ret= false; }
    }
  else ret= true;
  
  return ret;
    
} // end ins_outs_check_permission


static void
insb (
      PROTO_INTERP
      )
{

  uint8_t data;
  
  
  // Comprovacions
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !ins_outs_check_permission ( INTERP, DX, 1 ) ) return;
  
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      // Mou
      data= PORT_READ8 ( DX );
      if ( ADDRESS_SIZE_IS_32 )
        {
          WRITEB ( P_ES, EDI, data, return );
          if ( EFLAGS&DF_FLAG ) --EDI;
          else                  ++EDI;
        }
      else
        {
          WRITEB ( P_ES, (uint32_t) DI, data, return );
          if ( EFLAGS&DF_FLAG ) --DI;
          else                  ++DI;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;
      
    }
  
} // end insb


static void
insw (
      PROTO_INTERP
      )
{

  uint16_t data;
  
  
  // Comprovacions
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !ins_outs_check_permission ( INTERP, DX, 2 ) ) return;
  
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      // Mou
      data= PORT_READ16 ( DX );
      if ( ADDRESS_SIZE_IS_32 )
        {
          WRITEW ( P_ES, EDI, data, return );
          if ( EFLAGS&DF_FLAG ) EDI-= 2;
          else                  EDI+= 2;
        }
      else
        {
          WRITEW ( P_ES, (uint32_t) DI, data, return );
          if ( EFLAGS&DF_FLAG ) DI-= 2;
          else                  DI+= 2;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;

    }
  
} // end insw


static void
insd (
      PROTO_INTERP
      )
{
  
  uint32_t data;
  
  
  // Comprovacions
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !ins_outs_check_permission ( INTERP, DX, 4 ) ) return;

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      // Mou
      data= PORT_READ32 ( DX );
      if ( ADDRESS_SIZE_IS_32 )
        {
          WRITED ( P_ES, EDI, data, return );
          if ( EFLAGS&DF_FLAG ) EDI-= 4;
          else                  EDI+= 4;
        }
      else
        {
          WRITED ( P_ES, (uint32_t) DI, data, return );
          if ( EFLAGS&DF_FLAG ) DI-= 4;
          else                  DI+= 4;
        }
      
      // Repeteix si pertoca.
      TRY_REP_MS;

    }
  
} // end insd


static void
ins (
     PROTO_INTERP
     )
{
  OPERAND_SIZE_IS_32 ? insd ( INTERP ) : insw ( INTERP );
} // end ins




/************************/
/* OUT - Output to Port */
/************************/

static void
out_AL (
        PROTO_INTERP,
        const uint16_t port
        )
{

  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( CPL > IOPL || (EFLAGS&VM_FLAG) )
        {
          if ( check_io_permission_bit_map ( INTERP, port, 1 ) )
            PORT_WRITE8 ( port, AL );
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); }
        }
      else if ( CPL <= IOPL ) PORT_WRITE8 ( port, AL );
    }
  else PORT_WRITE8 ( port, AL );

} // end out_AL


static void
out_AX (
        PROTO_INTERP,
        const uint16_t port
        )
{

  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( CPL > IOPL || (EFLAGS&VM_FLAG) )
        {
          if ( check_io_permission_bit_map ( INTERP, port, 2 ) )
            PORT_WRITE16 ( port, AX );
          else { EXCEPTION_ERROR_CODE ( EXCP_GP, 0 ); }
        }
      else if ( CPL <= IOPL ) PORT_WRITE16 ( port, AX );
    }
  else PORT_WRITE16 ( port, AX );

} // end out_AX


static void
out_EAX (
         PROTO_INTERP,
         const uint16_t port
         )
{

  if ( PROTECTED_MODE_ACTIVATED )
    {
      if ( CPL > IOPL || (EFLAGS&VM_FLAG) )
        {
          printf ( "[EE] out_EAX: Cal implementar la protecció de I/O" );
          exit ( EXIT_FAILURE );
          /*
            IF (Any I/O Permission Bit for I/O port being accessed = 1)
            THEN (* I/O operation is not allowed *)
            #GP(0);
            ELSE ( * I/O operation is allowed *)
            DEST ← SRC; (* Writes to selected I/O port *)
            FI;
          */
        }
      else if ( CPL <= IOPL ) PORT_WRITE32 ( port, EAX );
    }
  else PORT_WRITE32 ( port, EAX );

} // end out_EAX


static void
out_imm8_AL (
             PROTO_INTERP
             )
{

  uint8_t port;
  
  
  IB ( &port, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  out_AL ( INTERP, (uint16_t) port );
  
} // end out_imm8_AL


static void
out_imm8_A (
            PROTO_INTERP
            )
{
  
  uint8_t port;
  
  
  IB ( &port, return );
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  OPERAND_SIZE_IS_32 ?
    out_EAX ( INTERP, (uint16_t) port ) :
    out_AX ( INTERP, (uint16_t) port );
  
} // end out_imm8_A


static void
out_DX_AL (
           PROTO_INTERP
           )
{
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  out_AL ( INTERP, DX );
} // end out_DX_AL


static void
out_DX_A (
          PROTO_INTERP
          )
{
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  OPERAND_SIZE_IS_32 ? out_EAX ( INTERP, DX ) : out_AX ( INTERP, DX );
} // end out_DX_A




/**************************************************/
/* OUTS/OUTSB/OUTSW/OUTSD - Output String to Port */
/**************************************************/

static void
outsb (
       PROTO_INTERP
       )
{

  uint8_t data;
  addr_t addr;
  
  
  // Comprovacions
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !ins_outs_check_permission ( INTERP, DX, 1 ) ) return;
  
  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      // Mou
      get_addr_dsesi ( INTERP, &addr );
      READB_DATA ( addr.seg, addr.off, &data, return );
      PORT_WRITE8 ( DX, data );
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
  
} // end outsb


static void
outsw (
       PROTO_INTERP
       )
{

  uint16_t data;
  addr_t addr;
  
  
  // Comprovacions
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !ins_outs_check_permission ( INTERP, DX, 2 ) ) return;

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      // Mou
      get_addr_dsesi ( INTERP, &addr );
      READW_DATA ( addr.seg, addr.off, &data, return );
      PORT_WRITE16 ( DX, data );
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
  
} // end outsw


static void
outsd (
       PROTO_INTERP
       )
{
  
  uint32_t data;
  addr_t addr;
  
  
  // Comprovacions
  if ( QLOCKED ) { EXCEPTION ( EXCP_UD ); return; }
  if ( !ins_outs_check_permission ( INTERP, DX, 4 ) ) return;

  if ( !REPE_REPNE_ENABLED || !REP_COUNTER_IS_0 )
    {
      
      // Mou
      get_addr_dsesi ( INTERP, &addr );
      READD_DATA ( addr.seg, addr.off, &data, return );
      PORT_WRITE32 ( DX, data );
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
  
} // end outsd


static void
outs (
      PROTO_INTERP
      )
{
  OPERAND_SIZE_IS_32 ? outsd ( INTERP ) : outsw ( INTERP );
} // end outs
