%define MB1 esi
%define MB2 edi
%define MIN_ERR ebx
%define ROW ecx
%define SAD edx
%define MB1_ROW_INC ebp+16

BITS 32

GLOBAL SAD_Macroblock_mmx

SECTION .text
ALIGN 8

SAD_Macroblock_mmx:
	push ebp;
	mov ebp, esp;

	push ebx;
	push ecx;
	push edx;
	push esi;
	push edi;

	mov MB1, [ebp+8];
	mov MB2, [ebp+12];
	mov eax, DWORD [MB1_ROW_INC];
	sub eax, 16;
	imul eax, 2;
	mov DWORD [MB1_ROW_INC], eax;
	mov MIN_ERR, [ebp+20];
	mov SAD, 0;
	mov ROW, 16;

sad_mb_row_loop:
	movq	mm1, [MB1];
	psadbw	mm1, [MB2];
	movd eax, mm1;
	add SAD, eax;
	add MB1, 8;
	add MB2, 8;
	movq	mm1, [MB1];
	psadbw	mm1, [MB2];
	movd eax, mm1;
	add SAD, eax;
	add MB1, 8;
	add MB2, 8;
	movq	mm1, [MB1];
	psadbw	mm1, [MB2];
	movd eax, mm1;
	add SAD, eax;
	add MB1, 8;
	add MB2, 8;
	movq	mm1, [MB1];
	psadbw	mm1, [MB2];
	movd eax, mm1;
	add SAD, eax;
	add MB1, 8;
	add MB2, 8;

	cmp SAD, MIN_ERR;
	jg near sad_mb_done;

	add MB1, [MB1_ROW_INC];
	sub ROW, 1;
	jnz near sad_mb_row_loop;
	
sad_mb_done:
	mov eax, SAD;

	pop edi;
	pop esi;
	pop edx;
	pop ecx;
	pop ebx;

	emms;
	pop ebp;
	ret;
