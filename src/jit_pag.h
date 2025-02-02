/*
 * Copyright 2023-2025 Adrià Giménez Pastor.
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
 *  jit_pag.h - Conté la part de 'jit.c' on s'implementa el codi dels
 *              paginadors.
 *
 */


// MACROS

#define PDE_P       0x00000001
#define PDE_RW      0x00000002
#define PDE_US      0x00000004
#define PDE_PS      0x00000080
#define PDE_PTEADDR 0xFFFFF000

#define PTE_P       0x00000001
#define PTE_RW      0x00000002
#define PTE_US      0x00000004

#define ECODE_P    0x0001
#define ECODE_WR   0x0002
#define ECODE_US   0x0004
#define ECODE_RSVD 0x0008
#define ECODE_ID   0x0010
#define ECODE_PK   0x0020

#define IA32_JIT_P32_L2_4KB_SIZE 1024




// FUNCIONS

static void
paging_32b_free (
                 IA32_JIT_Paging32b *p32
                 )
{

  int n,ind;


  for ( n= 0; n < p32->N; ++n )
    {
      ind= p32->a[n];
      if ( p32->v[ind].v4kB != NULL )
        free ( p32->v[ind].v4kB );
    }
  free ( p32 );
  
} // end paging_32b_free


// Sols reserva memòria
static IA32_JIT_Paging32b *
paging_32b_new (void)
{

  IA32_JIT_Paging32b *ret;
  int n;
  

  ret= (IA32_JIT_Paging32b *) malloc__ ( sizeof(IA32_JIT_Paging32b) );
  ret->N= 0;
  for ( n= 0; n < IA32_JIT_P32_L1_SIZE; ++n )
    {
      ret->v[n].active= false;
      ret->v[n].v4kB= NULL;
    }
  ret->addr_min= 0;
  ret->addr_max= 0;
  ret->base_addr= 0;
  
  return ret;
  
} // end paging_32b_new


static void
paging_32b_clear (
                  IA32_JIT_Paging32b *p32
                  )
{

  int n,i;


  for ( n= 0; n < p32->N; ++n )
    {
      i= p32->a[n];
      p32->v[i].active= false;
      if ( p32->v[i].v4kB != NULL )
        {
          free ( p32->v[i].v4kB );
          p32->v[i].v4kB= NULL;
        }
    }
  p32->N= 0;
  p32->addr_min= 0;
  p32->addr_max= 0;
  p32->base_addr= 0;
  
} // end paging_32b_clear


static void
paging_32b_CR3_changed (
                        IA32_JIT *jit
                        )
{

  IA32_JIT_Paging32b *p32;
  
  
  p32= jit->_pag32;
  paging_32b_clear ( p32 );
  p32->base_addr= (uint64_t) (CR3&CR3_PDB);
  p32->addr_min= p32->base_addr;
  p32->addr_max= p32->base_addr + (1<<12); // 1024 entrades de 4 bytes
  
} // end paging_32b_CR3_changed


static void
paging_32b_alloc_page (
                       IA32_JIT       *jit,
                       const uint32_t  addr,
                       const bool      add_actives
                       )
{

  IA32_JIT_Paging32b *p32;
  uint64_t pde_addr,end_addr;
  uint32_t pde;
  int ind,n;
  

  p32= jit->_pag32;

  // Llig pde
  pde_addr= (uint64_t) (p32->base_addr | ((addr>>22)<<2));
  pde= READU32 ( pde_addr );
  ind= (int) (addr>>22);

  // Inicialitza.
  p32->v[ind].active= true;
  p32->v[ind].error= ((pde&PDE_P) == 0);
  if ( !p32->v[ind].error )
    {
      p32->v[ind].pse_enabled= ((CR4&CR4_PSE)!=0);
      p32->v[ind].svm_addr= ((pde&PDE_US)==0);
      p32->v[ind].writing_allowed= ((pde&PDE_RW)!=0);
      p32->v[ind].is4KB= !p32->v[ind].pse_enabled || ((pde&PDE_PS) == 0);
      if ( !p32->v[ind].is4KB )
        {
          fprintf ( stderr, "[CAL_IMPLEMENTAR] paging_32b_alloc_page - 4MB\n" );
          exit ( EXIT_FAILURE );
        }
      else
        {
          if ( p32->v[ind].pse_enabled )
            {
              // NOTA!! Quan s'implemente igual és alguna cosa que es
              // comprova en execució i no ací.
              fprintf ( stderr, "[CAL_IMPLEMENTAR] paging_32b_alloc_page"
                        " - 4KB comprovació reserved bits\n" );
              exit ( EXIT_FAILURE );
            }
          p32->v[ind].v4kB= (IA32_JIT_P32_L2_4KB *)
            malloc__ ( sizeof(IA32_JIT_P32_L2_4KB)*IA32_JIT_P32_L2_4KB_SIZE );
          for ( n= 0; n < IA32_JIT_P32_L2_4KB_SIZE; ++n )
            p32->v[ind].v4kB[n].active= false;
          p32->v[ind].base_addr= (uint64_t) (pde&PDE_PTEADDR);
          end_addr= p32->v[ind].base_addr + 4*1024;
          if ( p32->v[ind].base_addr < p32->addr_min )
            p32->addr_min= p32->v[ind].base_addr;
          else if ( p32->v[ind].base_addr > p32->addr_max )
            p32->addr_max= p32->v[ind].base_addr;
          if ( end_addr < p32->addr_min ) p32->addr_min= end_addr;
          else if ( end_addr > p32->addr_max ) p32->addr_max= end_addr;
        }
    }
  
  // Desa en actius
  if ( add_actives ) p32->a[p32->N++]= ind;
  
} // end paging_32b_alloc_page


static void
paging_32b_active_4kB (
                       IA32_JIT        *jit,
                       IA32_JIT_P32_L1 *pde,
                       const uint32_t   addr
                       )
{

  uint64_t pte_addr;
  uint32_t pte;
  int ind;


  // Prepara
  ind= (int) ((addr>>12)&0x3FF);
  pte_addr= pde->base_addr | ((uint64_t) ((addr>>10)&0x00000FFC));

  // Activa
  pde->v4kB[ind].active= true;
  pte= READU32 ( pte_addr );
  if ( (pte&PTE_P) == 0 ) pde->v4kB[ind].error= true;
  else
    {
      pde->v4kB[ind].error= false;
      pde->v4kB[ind].svm_addr=
        (pte&PTE_US) == 0 ? true : pde->svm_addr;
      pde->v4kB[ind].writing_allowed=
        (pte&PTE_RW) == 0 ? false : pde->writing_allowed;
      pde->v4kB[ind].base_addr= (uint64_t) (pte&0xFFFFF000);
    }

} // end paging_32b_active_4kB


static bool
paging_32b_check_access (
                         IA32_JIT   *jit,
                         const bool  explicit_svm,
                         const bool  implicit_svm,
                         const bool  svm_addr,
                         const bool  writing_allowed,
                         const bool  ifetch,
                         const bool  writing
                         )
{
  
  bool ret;

  
  // Supervidor-mode access
  if ( explicit_svm || implicit_svm )
    {
      
      if ( ifetch ) // Instruction fetch
        {
          if ( svm_addr ) ret= true;
          else ret= (CR4&CR4_SMEP) ? false : true;
        }
      
      else if ( writing ) // Escriptura dades
        {
          if ( svm_addr ) // supervisor-mode address
            {
              if ( CR0&CR0_WP ) ret= writing_allowed ? true : false;
              else ret= true;
            }
          else // user-mode address
            {
              if ( CR0&CR0_WP )
                {
                  if ( !writing_allowed ) ret= false;
                  else
                    {
                      if ( (CR4&CR4_SMAP) != 0 )
                        {
                          if ( (EFLAGS&AC_FLAG) && explicit_svm ) ret= true;
                          else                                    ret= false;
                        }
                      else ret= true;
                    }
                }
              else
                {
                  if ( (CR4&CR4_SMAP) != 0 )
                    {
                      if ( (EFLAGS&AC_FLAG) && explicit_svm ) ret= true;
                      else                                    ret= false;
                    }
                  else ret= true;
                }
            }
        }
      else // Lectura dades
        {
          // Data may be read (implicitly or explicitly) from any
          // supervisor-mode address.
          if ( svm_addr ) ret= true;
          else // user-mode address
            {
              if ( (CR4&CR4_SMAP) != 0 )
                {
                  if ( (EFLAGS&AC_FLAG) && explicit_svm ) ret= true;
                  else ret= false;
                }
              // If CR4.SMAP = 0, data may be read from any user-mode
              // address with a protection key for which read access
              // is permitted.
              else ret= true;
            }
        }
    }
  
  // User-mode access
  else
    {
      if ( ifetch ) ret= true;
      else if ( writing )
        {
          if ( svm_addr ) ret= false;
          else if ( !writing_allowed ) ret= false;
          else ret= true;
        }
      else // Lectura
        {
          if ( svm_addr ) ret= false;
          else ret= true;
        }
    }

  return ret;
  
} // end paging_32b_check_access


// Torna cert si ha anat tot bé. Si no_check_erros és cert mai falla i
// torna alguna cosa sempre. Si no es pot traduir torna l'adreça sense
// traduir.
static bool
paging_32b_translate (
                      IA32_JIT       *jit,
                      const uint32_t  addr,
                      uint64_t       *laddr,
                      const bool      writing,
                      const bool      ifetch,
                      const bool      implicit_svm
                      )
{

  int i,j;
  IA32_JIT_Paging32b *p32;
  IA32_JIT_P32_L1 *pde;
  uint16_t ecode;
  bool explicit_svm,pse_enabled;
  
  
  // Preparacions.
  p32= jit->_pag32;
  ecode= 0;
  explicit_svm= CPL < 3;
  pse_enabled= (CR4&CR4_PSE)!=0;
  *laddr= (uint64_t) addr;
  
  // Selecciona PDE
  i= (int) (addr>>22);
  if ( !p32->v[i].active ) paging_32b_alloc_page ( jit, addr, true );
  pde= &(p32->v[i]);
  if ( pde->error ) { ecode= 0; goto error; }
  if ( pde->pse_enabled != pse_enabled )
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] paging_32b_translate -"
                " pde->pse_enabled != pse_enabled\n" );
      exit ( EXIT_FAILURE );
    }

  // Segon nivell
  if ( pde->is4KB )
    {
      j= (int) ((addr>>12)&0x3FF);
      if ( !pde->v4kB[j].active ) paging_32b_active_4kB ( jit, pde, addr );
      if ( pde->v4kB[j].error ) { ecode= 0; goto error; }
      // Ho faig abans per si falla i estem en mode no_check_errors
      *laddr= pde->v4kB[j].base_addr | ((uint64_t) (addr&0x00000FFF));
      if ( !paging_32b_check_access ( jit, explicit_svm, implicit_svm,
                                      pde->v4kB[j].svm_addr,
                                      pde->v4kB[j].writing_allowed,
                                      ifetch, writing ) )
        {
          ecode= ECODE_P; // page-level protection violation
          goto error;
        }
    }
  else
    {
      fprintf ( stderr, "[CAL_IMPLEMENTAR] paging_32b_translate - 4MB\n" );
      exit ( EXIT_FAILURE );
    }
  
  return true;

 error:
  CR2= addr;
  if ( writing ) ecode|= ECODE_WR;
  if ( ifetch ) ecode|= ECODE_ID;
  if ( !implicit_svm && !explicit_svm ) ecode|= ECODE_US;
  EXCEPTION_ERROR_CODE ( EXCP_PF, ecode );
  return false;
  
} // end paging_32b_translate


// Es crida cada vegada que s'escriu en una adreça. IMPORTANT!!! S'ha
// de cridar després d'haver-se actualitzat en memòria.
static void
paging_32b_addr_changed (
                         IA32_JIT       *jit,
                         const uint64_t  addr
                         )
{

  IA32_JIT_Paging32b *p32;
  IA32_JIT_P32_L1 *pde;
  uint64_t addr_tmp;
  int ind,n;
  uint32_t fake_addr;
  bool stop;
  
  
  p32= jit->_pag32;
  if ( addr >= p32->addr_min && addr < p32->addr_max )
    {

      // Entrades de la taula.
      addr_tmp= p32->base_addr + (1<<12);
      if ( addr >= p32->base_addr && addr < addr_tmp )
        {
          ind= (int) ((addr-p32->base_addr)>>2);
          // Reactiva
          if ( p32->v[ind].active )
            {
              p32->v[ind].active= false;
              if ( p32->v[ind].v4kB != NULL )
                {
                  free ( p32->v[ind].v4kB );
                  p32->v[ind].v4kB= NULL;
                }
              fake_addr= ((uint32_t) ind)<<22;
              paging_32b_alloc_page ( jit, fake_addr, false );
            }
        }
      
      // Entrades nivell 2
      else
        {
          stop= false;
          for ( n= 0; n < p32->N && !stop; ++n )
            {
              pde= &p32->v[p32->a[n]];
              if ( pde->error ) continue;
              if ( pde->is4KB )
                {
                  addr_tmp= pde->base_addr + (1<<12);
                  if ( addr >= pde->base_addr && addr < addr_tmp )
                    {
                      stop= true;
                      ind= (int) ((addr-pde->base_addr)>>2);
                      // Recarrega
                      if ( pde->v4kB[ind].active )
                        {
                          fake_addr= ((uint32_t) ind)<<12;
                          paging_32b_active_4kB ( jit, pde, fake_addr );
                        }
                    }
                }
              else
                {
                  fprintf ( stderr,"[CAL_IMPLEMENTAR] paging_32b_addr_changed -"
                            " 4MB!!\n" );
                  exit ( EXIT_FAILURE );
                }
            }
        }
      
    }
  
} // end paging_32b_addr_changed
