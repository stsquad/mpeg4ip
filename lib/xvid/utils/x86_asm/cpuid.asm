;/******************************************************************************
; *                                                                            *
; *  This file is part of XviD, a free MPEG-4 video encoder/decoder            *
; *                                                                            *
; *  XviD is an implementation of a part of one or more MPEG-4 Video tools     *
; *  as specified in ISO/IEC 14496-2 standard.  Those intending to use this    *
; *  software module in hardware or software products are advised that its     *
; *  use may infringe existing patents or copyrights, and any such use         *
; *  would be at such party's own risk.  The original developer of this        *
; *  software module and his/her company, and subsequent editors and their     *
; *  companies, will have no liability for use of this software or             *
; *  modifications or derivatives thereof.                                     *
; *                                                                            *
; *  XviD is free software; you can redistribute it and/or modify it           *
; *  under the terms of the GNU General Public License as published by         *
; *  the Free Software Foundation; either version 2 of the License, or         *
; *  (at your option) any later version.                                       *
; *                                                                            *
; *  XviD is distributed in the hope that it will be useful, but               *
; *  WITHOUT ANY WARRANTY; without even the implied warranty of                *
; *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
; *  GNU General Public License for more details.                              *
; *                                                                            *
; *  You should have received a copy of the GNU General Public License         *
; *  along with this program; if not, write to the Free Software               *
; *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA  *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  cpuid.asm, check cpu features                                             *
; *                                                                            *
; *  Copyright (C) 2001 - Michael Militzer <isibaar@xvid.org>,                 *
; *                                                                            *
; *  For more information visit the XviD homepage: http://www.xvid.org         *
; *                                                                            *
; ******************************************************************************/
;
;/******************************************************************************
; *                                                                            *
; *  Revision history:                                                         *
; *                                                                            *
; *  17.12.2001 initial version  (Isibaar)                                     *
; *                                                                            *
; ******************************************************************************/

bits 32

%define CPUID_TSC				0x00000010
%define CPUID_MMX				0x00800000
%define CPUID_SSE				0x02000000
%define CPUID_SSE2				0x04000000

%define EXT_CPUID_3DNOW			0x80000000
%define EXT_CPUID_AMD_3DNOWEXT	0x40000000
%define EXT_CPUID_AMD_MMXEXT	0x00400000

%define XVID_CPU_MMX			0x00000001
%define XVID_CPU_MMXEXT			0x00000002
%define XVID_CPU_SSE	        0x00000004
%define XVID_CPU_SSE2			0x00000008
%define XVID_CPU_3DNOW          0x00000010
%define XVID_CPU_3DNOWEXT		0x00000020
%define XVID_CPU_TSC            0x00000040


%macro cglobal 1 
	%ifdef PREFIX
		global _%1 
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

ALIGN 32

section .data

features	dd 0

vendor		dd 0,0,0
vendorAMD	db "AuthenticAMD"

%macro  CHECK_FEATURE         3

    mov     ecx, %1
    and     ecx, edx
    neg     ecx
    sbb     ecx, ecx
    and     ecx, %2
    or      [%3], ecx

%endmacro

section .text

; int check_cpu_feature(void)

cglobal check_cpu_features
check_cpu_features:
	
	pushad
	pushfd	                        

	; CPUID command ?
	pop		eax
	mov		ecx, eax
	xor		eax, 0x200000
	push	eax
	popfd
	pushfd
	pop		eax
	cmp		eax, ecx

	jz		near .cpu_quit		; no CPUID command -> exit


	; get vendor string, used later
    xor     eax, eax
    cpuid           
    mov     [vendor], ebx       ; vendor string 
    mov     [vendor+4], edx     
    mov     [vendor+8], ecx     
    test    eax, eax

    jz      near .cpu_quit

    mov     eax, 1 
    cpuid


    ; RDTSC command ?
	CHECK_FEATURE CPUID_TSC, XVID_CPU_TSC, features

    ; MMX support ?
	CHECK_FEATURE CPUID_MMX, XVID_CPU_MMX, features

    ; SSE support ?
	CHECK_FEATURE CPUID_SSE, (XVID_CPU_MMXEXT+XVID_CPU_SSE), features

	; SSE2 support?
	CHECK_FEATURE CPUID_SSE2, XVID_CPU_SSE2, features


	; extended functions?
    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000000
    jbe     near .cpu_quit

    mov     eax, 0x80000001
    cpuid
         
    ; 3DNow! support ?
	CHECK_FEATURE EXT_CPUID_3DNOW, XVID_CPU_3DNOW, features

	; AMD cpu ?
    lea     esi, [vendorAMD]
    lea     edi, [vendor]
    mov     ecx, 12
    cld
    repe    cmpsb
    jnz     .cpu_quit

	; 3DNOW extended ?
	CHECK_FEATURE EXT_CPUID_AMD_3DNOWEXT, XVID_CPU_3DNOWEXT, features

	; extended MMX ?
	CHECK_FEATURE EXT_CPUID_AMD_MMXEXT, XVID_CPU_MMXEXT, features
        
.cpu_quit:  

	popad
	
	mov eax, [features]
	
	ret
