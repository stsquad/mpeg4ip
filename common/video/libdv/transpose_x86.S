.text
.global transpose_mmx
	
transpose_mmx:
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl   %edi

	movl    8(%ebp), %esi          # source

        pushl %ebx
        pushl %ecx
        pushl %edx

        movl            $8, %ebx   # ebx is x_size
        movl            %ebx, %ecx
        movl            %esi, %edi      # pointer to the matrix

        sall            $2, %ecx
        movl            %ebx, %eax
        addl            %ebx, %ecx
        subl            $4, %eax        # eax is the inner loop variable

        addl            %ebx, %ecx      # ecx is 6*row size
        movl            %eax, %edx      # edx is the outer loop variable

do_4x4_block_where_x_equals_y: 

        movq    (%esi), %mm0            # m03:m02|m01:m00 - first line

        movq    (%esi,%ebx,4), %mm2     # m23:m22|m21:m20 - third line
        movq    %mm0, %mm6              # copy first line

        punpcklwd (%esi,%ebx,2), %mm0
        # m11:m01|m10:m00 - interleave first and second lines
        movq    %mm2, %mm7              # copy third line

        punpcklwd (%esi,%ecx,), %mm2
        # m31:m21|m30:m20 - interleave third and fourth lines
        movq    %mm0, %mm4              # copy first intermediate result

        movq    (%esi,%ebx,2), %mm1     # m13:m12|m11:m10 - second line
        punpckldq %mm2, %mm0
        # m30:m20|m10:m00 - interleave to produce result 1

        movq    (%esi,%ecx,), %mm3      # m33:m32|m31:m30 - fourth line
        punpckhdq %mm2, %mm4
        # m31:m21|m11:m01 - interleave to produce result 2

        movq    %mm0, (%esi)            # write result 1
        punpckhwd %mm1, %mm6
        # m13:m03|m12:m02 - interleave first and second lines

        movq    %mm4, (%esi,%ebx,2)     # write result 2
        punpckhwd %mm3, %mm7
        # m33:m23|m32:m22 - interleave third and fourth lines

        movq    %mm6, %mm5              # copy first intermediate result
        punpckldq %mm7, %mm6
        # m32:m22|m12:m02 - interleave to produce result 3

        leal    (%edi,%ebx,8), %edi
        # reload edi to point to a 4x4 set 4 rows down
        punpckhdq %mm7, %mm5
        # m33:m23|m13:m03 - interleave to produce result 4

        movq    %mm6, (%esi,%ebx,4)     # write result 3

        movq    %mm5, (%esi,%ecx,)      # write result 4

        cmpl    $0, %edx
        # check to see if the number of rows left is zero
        je      all_done_ready_to_exit
        #last time through you are done and ready to exit

do_4x4_blocks_x_and_y_not_equal: 

# transpose the two mirror image 4x4 sets so that the writes 
# can be done without overwriting unused data 

        movq    8(%esi), %mm0           # m03:m02|m01:m00 - first line

        movq    8(%esi,%ebx,4), %mm2     # m23:m22|m21:m20 - third line
        movq    %mm0, %mm6              # copy first line

        punpcklwd 8(%esi,%ebx,2), %mm0
        # m11:m01|m10:m00 - interleave first and second lines
        movq    %mm2, %mm7              # copy third line

        punpcklwd 8(%esi,%ecx,), %mm2
        # m31:m21|m30:m20 - interleave third and fourth lines
        movq    %mm0, %mm4              # copy first intermediate result
# all references for second 4 x 4 block are referred by "n" instead of "m"
        movq    (%edi), %mm1            # n03:n02|n01:n00 - first line 
        punpckldq %mm2, %mm0
        # m30:m20|m10:m00 - interleave to produce first result

        movq    (%edi,%ebx,4), %mm3     # n23:n22|n21:n20 - third line
        punpckhdq %mm2, %mm4
        # m31:m21|m11:m01 - interleave to produce second result

        punpckhwd 8(%esi,%ebx,2), %mm6
        # m13:m03|m12:m02 - interleave first and second lines
        movq    %mm1, %mm2              # copy first line

        punpckhwd 8(%esi,%ecx,), %mm7
        # m33:m23|m32:m22 - interleave third and fourth lines
        movq    %mm6, %mm5              # copy first intermediate result

        movq    %mm0, (%edi)            # write result 1
        punpckhdq %mm7, %mm5
        # m33:m23|m13:m03 - produce third result

        punpcklwd (%edi,%ebx,2), %mm1
        # n11:n01|n10:n00 - interleave first and second lines
        movq    %mm3, %mm0              # copy third line

        punpckhwd (%edi,%ebx,2), %mm2
        # n13:n03|n12:n02 - interleave first and second lines

        movq    %mm4, (%edi,%ebx,2)     # write result 2 out
        punpckldq %mm7, %mm6
        # m32:m22|m12:m02 - produce fourth result

        punpcklwd (%edi,%ecx,), %mm3
        # n31:n21|n30:n20 - interleave third and fourth lines
        movq    %mm1, %mm4              # copy first intermediate result

        movq    %mm6, (%edi,%ebx,4)     # write result 3 out
        punpckldq %mm3, %mm1
        # n30:n20|n10:n00 - produce first result

        punpckhwd (%edi,%ecx,), %mm0
        # n33:n23|n32:n22 - interleave third and fourth lines
        movq    %mm2, %mm6              # copy second intermediate result

        movq    %mm5, (%edi,%ecx,)      # write result 4 out
        punpckhdq %mm3, %mm4
        # n31:n21|n11:n01- produce second result

        movq    %mm1, 8(%esi)
        # write result 5 out - (first result for other 4 x 4 block)
        punpckldq %mm0, %mm2
        # n32:n22|n12:n02- produce third result

        movq    %mm4, 8(%esi,%ebx,2)     # write result 6 out
        punpckhdq %mm0, %mm6
        # n33:n23|n13:n03 - produce fourth result

        movq    %mm2, 8(%esi,%ebx,4)     # write result 7 out

        movq    %mm6, 8(%esi,%ecx,)     # write result 8 out

        addl    $8, %esi
        # increment esi to point to next 4 x 4 block in same row
        leal    (%edi,%ebx,8), %edi
        # increment edi to point to next 4 x 4 block below current one

        subl    $4, %eax                # decrement inner loop variable
        jnz     do_4x4_blocks_x_and_y_not_equal
        # edi points to start of the second row in block we just finished

        sall    %edx
        leal    8(%esi,%ebx,8), %esi     # reload edi to point four rows down

        subl    %edx, %esi
        # subtract the number of bytes in last row 
        # now we point to spot where row = col 
        subl    $8, %edx                # sub 4 from row number

        sarl    %edx
        movl    %esi, %edi

        movl    %edx, %eax
        # reset x_size to outer loop variable to start new row
        jmp     do_4x4_block_where_x_equals_y

all_done_ready_to_exit: 
                popl %edx
                popl %ecx
                popl %ebx

popl %edi
popl    %esi
popl %ebp

ret     $0
