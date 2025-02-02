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
 *  cpu.c - Funcions relacionades amb IA32_CPU.
 *
 */


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IA32.h"




/**********/
/* MACROS */
/**********/

#define CPUID_FI1_SSE3       0x00000001
#define CPUID_FI1_PCLMULQDQ  0x00000002
#define CPUID_FI1_DTES64     0x00000004
#define CPUID_FI1_MONITOR    0x00000008
#define CPUID_FI1_DS_CPL     0x00000010
#define CPUID_FI1_VMX        0x00000020
#define CPUID_FI1_SMX        0x00000040
#define CPUID_FI1_EIST       0x00000080
#define CPUID_FI1_TM2        0x00000100
#define CPUID_FI1_SSSE3      0x00000200
#define CPUID_FI1_CNXT_ID    0x00000400
#define CPUID_FI1_SDBG       0x00000800
#define CPUID_FI1_FMA        0x00001000
#define CPUID_FI1_CMPXCHG16B 0x00002000
#define CPUID_FI1_XTPR       0x00004000
#define CPUID_FI1_PDCM       0x00008000

#define CPUID_FI1_PCID       0x00020000
#define CPUID_FI1_DCA        0x00040000
#define CPUID_FI1_SSE4_1     0x00080000
#define CPUID_FI1_SSE4_2     0x00100000
#define CPUID_FI1_X2APIC     0x00200000
#define CPUID_FI1_MOVBE      0x00400000
#define CPUID_FI1_POPCNT     0x00800000
#define CPUID_FI1_TSC_DEAD   0x01000000
#define CPUID_FI1_AES        0x02000000
#define CPUID_FI1_XSAVE      0x04000000
#define CPUID_FI1_OSXSAVE    0x08000000
#define CPUID_FI1_AVX        0x10000000
#define CPUID_FI1_F16C       0x20000000
#define CPUID_FI1_RDRAND     0x40000000

#define CPUID_FI2_FPU        0x00000001
#define CPUID_FI2_VME        0x00000002
#define CPUID_FI2_DE         0x00000004
#define CPUID_FI2_PSE        0x00000008
#define CPUID_FI2_TSC        0x00000010
#define CPUID_FI2_MSR        0x00000020
#define CPUID_FI2_PAE        0x00000040
#define CPUID_FI2_MCE        0x00000080
#define CPUID_FI2_CX8        0x00000100
#define CPUID_FI2_APIC       0x00000200

#define CPUID_FI2_SEP        0x00000800
#define CPUID_FI2_MTRR       0x00001000
#define CPUID_FI2_PGE        0x00002000
#define CPUID_FI2_MCA        0x00004000
#define CPUID_FI2_CMOV       0x00008000
#define CPUID_FI2_PAT        0x00010000
#define CPUID_FI2_PSE_36     0x00020000
#define CPUID_FI2_PSN        0x00040000
#define CPUID_FI2_CLFSH      0x00080000

#define CPUID_FI2_DS         0x00200000
#define CPUID_FI2_ACPI       0x00400000
#define CPUID_FI2_MMX        0x00800000
#define CPUID_FI2_FXSR       0x01000000
#define CPUID_FI2_SSE        0x02000000
#define CPUID_FI2_SSE2       0x04000000
#define CPUID_FI2_SS         0x08000000
#define CPUID_FI2_HTT        0x10000000
#define CPUID_FI2_TM         0x20000000

#define CPUID_FI2_PBE        0x80000000




/*********************/
/* FUNCIONS PRIVADES */
/*********************/

static void
cpuid_model_family_stepping (
                             IA32_CPU *cpu
                             )
{
  switch ( cpu->model )
    {
    case IA32_CPU_P5_60MHZ:
    case IA32_CPU_P5_66MHZ:
      // Estes versions de Pentium no tenen APIC
      // Version Information: Type, Family, Model, and Stepping ID
      cpu->eax.v= (0x5<<8) | (0x1<<4);
      // Brand Index, CLFLUSH line size, Maximum number of..., Initial APIC ID
      cpu->ebx.v= 0x00000000;
      // Feature Information 1
      cpu->ecx.v= 0x00000000;
      // Feature Information 2
      cpu->edx.v=
        CPUID_FI2_FPU |
        CPUID_FI2_VME |
        CPUID_FI2_DE | // <--- ¿¿??
        CPUID_FI2_PSE |
        CPUID_FI2_TSC |
        CPUID_FI2_MSR |
        CPUID_FI2_CX8 |
        //CPUID_FI2_PGE | // <--- ¿¿???
        //CPUID_FI2_DTS | // <--- ¿¿???
        //CPUID_FI2_ACPI | // <--- ¿¿??
        //CPUID_FI2_SNOOP | // <--- ¿¿??
        //CPUID_FI2_SS | // <--- ¿¿??
        //CPUID_FI2_HTT | // <--- ¿¿??
        //CPUID_FI2_TM | // <--- ¿¿??
        //CPUID_FI2_PBE | // <--- ¿¿??
        0
        ;
      break;
      /*
    case IA32_CPU_PENTIUM_WITH_MMX:
      // Version Information: Type, Family, Model, and Stepping ID
      EAX= (0x5<<8) | (0x4<<4);
      // Brand Index, CLFLUSH line size, Maximum number of..., Initial APIC ID
      EBX= 0x00000000;
      // Feature Information 1
      ECX= 0x00000000;
      // Feature Information 2
      EDX=
        CPUID_FI2_FPU |
        CPUID_FI2_VME |
        CPUID_FI2_DE | // <--- ¿¿??
        CPUID_FI2_PSE |
        CPUID_FI2_TSC |
        CPUID_FI2_MSR |
        CPUID_FI2_CX8 |
        CPUID_FI2_APIC |
        //CPUID_FI2_PGE | // <--- ¿¿???
        //CPUID_FI2_DTS | // <--- ¿¿???
        //CPUID_FI2_ACPI | // <--- ¿¿??
        CPUID_FI2_MMX |
        //CPUID_FI2_SNOOP | // <--- ¿¿??
        //CPUID_FI2_SS | // <--- ¿¿??
        //CPUID_FI2_HTT | // <--- ¿¿??
        //CPUID_FI2_TM | // <--- ¿¿??
        //CPUID_FI2_PBE | // <--- ¿¿??
        0
        ;
      break;
      */
    default:
      printf ( "[EE] CPUID EAX=01 no implementat en model %d\n", cpu->model );
      exit ( EXIT_FAILURE );
    }
} // end cpuid_model_family_stepping




/**********************/
/* FUNCIONS PÚBLIQUES */
/**********************/

void
IA32_cpu_init (
               IA32_CPU                 *cpu,
               const IA32_CPU_INIT_MODE  mode,
               const IA32_CPU_MODEL      model
               )
{

  int i;

  
  // Model
  cpu->model= model;
  
  // Principals
  cpu->eflags= 0x00000002;
  cpu->eip= 0x0000FFF0;

  // Control
  cpu->cr0=
    (mode==IA32_CPU_POWER_UP) ?
    0x60000010 : ((cpu->cr0&0x60000000) | 0x10);
  cpu->cr2= 0x00000000;
  cpu->cr3= 0x00000000;
  cpu->cr4= 0x00000000;

  // Depuració.
  cpu->dr0= 0x00000000; // ¿¿??
  cpu->dr1= 0x00000000; // ¿¿??
  cpu->dr2= 0x00000000; // ¿¿??
  cpu->dr3= 0x00000000; // ¿¿??
  cpu->dr4= 0x00000000; // ¿¿??
  cpu->dr5= 0x00000000; // ¿¿??
  cpu->dr6= 0xFFFE0FF0; // ¿¿??
  cpu->dr7= 0x00000400; // ¿¿??
  
  // Segment CS
  cpu->cs.v= 0xF000;
  cpu->cs.h.lim.addr= 0xFFFF0000;
  cpu->cs.h.lim.firstb= 0;
  cpu->cs.h.lim.lastb= 0xFFFF;
  cpu->cs.h.is32= false;
  cpu->cs.h.readable= true;
  cpu->cs.h.writable= true;
  cpu->cs.h.executable= true;
  cpu->cs.h.isnull= false;
  cpu->cs.h.tss_is32= false;
  cpu->cs.h.pl= 0;
  cpu->cs.h.dpl= 0;
  cpu->cs.h.data_or_nonconforming= false;

  // Altres segments
  cpu->ss.v= 0x0000;
  cpu->ss.h.lim.addr= 0x00000000;
  cpu->ss.h.lim.firstb= 0;
  cpu->ss.h.lim.lastb= 0xFFFF;
  cpu->ss.h.is32= false;
  cpu->ss.h.readable= true;
  cpu->ss.h.writable= true;
  cpu->ss.h.executable= false;
  cpu->ss.h.isnull= false;
  cpu->ss.h.tss_is32= false;
  cpu->ss.h.pl= 0;
  cpu->ss.h.dpl= 0;
  cpu->ss.h.data_or_nonconforming= true;
  cpu->ds= cpu->es= cpu->fs= cpu->gs= cpu->ss;

  // Registres de propòsit general.
  switch ( model )
    {
    case IA32_CPU_P5_60MHZ:
    case IA32_CPU_P5_66MHZ:
    case IA32_CPU_P54C_75MHZ:
    case IA32_CPU_P54C_90MHZ:
    case IA32_CPU_P54C_100MHZ:
      cpu->edx.v= 0x00000500;
      break; // 000005¿xx?H
      /*
    case IA32_CPU_P6:       cpu->edx.v= 0x00000600; break; // 000¿n?06¿xx?H
    case IA32_CPU_PENTIUM4: cpu->edx.v= 0x00000F00; break; // 00000F¿xx?H
      */
    default: cpu->edx.v= 0;
    }
  cpu->eax.v= cpu->ebx.v= cpu->ecx.v= 0;
  cpu->esi.v= cpu->edi.v= 0;
  cpu->ebp.v= cpu->esp.v= 0;

  // FPU
  // De moment tot a 0.
  memset ( &(cpu->fpu), 0, sizeof(cpu->fpu) );
  for ( i= 0; i < 8; ++i )
    cpu->fpu.regs[i].tag= IA32_CPU_FPU_TAG_EMPTY;
  
  // MM- ???

  // XMM- i MXCSR
  if ( mode == IA32_CPU_POWER_UP || mode == IA32_CPU_RESET )
    {
      memset ( &(cpu->xmm0), 0, sizeof(cpu->xmm0) );
      memset ( &(cpu->xmm1), 0, sizeof(cpu->xmm1) );
      memset ( &(cpu->xmm2), 0, sizeof(cpu->xmm2) );
      memset ( &(cpu->xmm3), 0, sizeof(cpu->xmm3) );
      memset ( &(cpu->xmm4), 0, sizeof(cpu->xmm4) );
      memset ( &(cpu->xmm5), 0, sizeof(cpu->xmm5) );
      memset ( &(cpu->xmm6), 0, sizeof(cpu->xmm6) );
      memset ( &(cpu->xmm7), 0, sizeof(cpu->xmm7) );
      cpu->mxcsr= 0x1F80;
    }

  // GDTR, ¿¿IDTR???
  cpu->gdtr.addr= 0x00000000;
  cpu->gdtr.firstb= 0;
  cpu->gdtr.lastb= 0xFFFF;
  cpu->idtr.addr= 0x00000000;
  cpu->idtr.firstb= 0;
  cpu->idtr.lastb= 0xFFFF;

  // LDTR, Task Register
  cpu->ldtr.v= 0x0000;
  cpu->ldtr.h.lim.addr= 0x00000000;
  cpu->ldtr.h.lim.firstb= 0;
  cpu->ldtr.h.lim.lastb= 0xFFFF;
  cpu->ldtr.h.is32= false;
  cpu->ldtr.h.readable= true;
  cpu->ldtr.h.writable= true;
  cpu->ldtr.h.executable= false;
  cpu->ldtr.h.isnull= false;
  cpu->ldtr.h.tss_is32= false;
  cpu->ldtr.h.pl= 0;
  cpu->tr= cpu->ldtr;

  // FALTEN MÉS COSES QUE NO TINC IMPLEMENTADES ...
  
} // end IA32_cpu_init


void
IA32_cpu_cpuid (
                IA32_CPU *cpu
                )
{

  switch ( cpu->eax.v )
    {

    case 0x00000000:
      cpu->eax.v= 0x00000000; // OEM processor
      cpu->ebx.v= 0x756E6547; // Genu
      cpu->ecx.v= 0x6C65746E; // ntel
      cpu->edx.v= 0x49656E69; // ineI
      break;
    case 0x00000001: cpuid_model_family_stepping ( cpu ); break;
      // NOTA!!! Intel and AMD have also reserved CPUID leaves
      // 0x40000000 - 0x400000FF for software use. Hypervisors can use
      // these leaves to provide an interface to pass information from
      // the hypervisor to the guest operating system running inside a
      // virtual machine.
      // Es gasta per a saber si estem en KVM (KVMKVMKVM) o per coses
      // de XEN
      // Jo he decidit ficar MEMUPCMEMUPC
    case 0x40000000:
      cpu->ebx.v= 0x554D454D; // MEMU
      cpu->ecx.v= 0x454D4350; // PCME
      cpu->edx.v= 0x4350554D; // MUPC
      break;
    case 0x40000001 ... 0x4FFFFFFF:
      cpu->ebx.v= cpu->ecx.v= cpu->edx.v= 0;
      break;
    default:
      printf ( "[EE] CPUID EAX=%X no implementat\n", cpu->eax.v );
      exit ( EXIT_FAILURE );
    }
    
} // end IA32_cpu_cpuid
