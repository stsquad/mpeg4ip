
//////////////////////////////////////////////////////////////////////////////
//
//  fdctmm32.c - AP922 MMX fDCT
//  ----------
//  Intel Application Note AP-922 - fast, precise implementation of DCT
//        http://developer.intel.com/vtune/cbts/appnotes.htm
//  ----------
//  
//       This code will run on any MMX CPU.  The dct_row operation can be
//  further optimized using PentiumIII/Athlon instructions (pshufw.)  
//  If the code will be run on a 3D-Now CPU (AMD K6-2/Athlon), a slight
//  accruacy-boost can be obtained.  Please see fdctam32.c for details.
//  
//    For a fast, precise MMX implementation of inverse-DCT 
//              visit http://www.elecard.com/peter
//              or check out Avery Lee's Virtualdub source-code
//                 http://www.concentric.net/~psilon  
//
//  Revision history
//  ----------------
//
//  v1.01 08/26/2000 (clipper bugfix)
//         In my haste to get this code out the door, I neglected to consider
//       the numerical range of output.  I *believe* an IEEE-1180/1990 fdct 
//       is range-limited to {-2048, +2047}.
//       fdctmm32.c now saturates the output DCT coefficients to this range.
//         A few comment typos were corrected. Equivalent-C code for
//       the forward_dct column stage has also been added.  The pseudo-C
//       code in Intel's AP-922 omits several important operations that 
//       would cause dct8_frw_col() to fail, if it were used directly without 
//       modification.
//         There is still room for additional optimization in the 
//       frw_dct_row_mmx() function.  The data pack/unpack operation could be
//       shortened with pshufw.
//
//  v1.0 07/20/2000 (initial release)
//       Initial release of AP922 MMX forward_DCT.
//       
//     
//  liaor@iname.com  http://members.tripod.com/~liaor  
//////////////////////////////////////////////////////////////////////////////

#define INP eax		// pointer to (short *blk)
#define OUT ecx		// pointer to output (temporary store space qwTemp[])
#define TABLE ebx	// pointer to tab_frw_01234567[]
#define TABLEF ebx  // pointer to tg_all_16
#define round_frw_row edx
//#define round_frw_col edx

#define x0 INP + 0*16
#define x1 INP + 1*16
#define x2 INP + 2*16
#define x3 INP + 3*16
#define x4 INP + 4*16
#define x5 INP + 5*16
#define x6 INP + 6*16
#define x7 INP + 7*16
#define y0 OUT + 0*16
#define y1 OUT + 1*16
#define y2 OUT + 2*16
#define y3 OUT + 3*16
#define y4 OUT + 4*16
#define y5 OUT + 5*16
#define y6 OUT + 6*16
#define y7 OUT + 7*16


//////////////////////////////////////////////////////////////////////
//
// constants for the forward DCT
// -----------------------------
//
// Be sure to check that your compiler is aligning all constants to QWORD
// (8-byte) memory boundaries!  Otherwise the unaligned memory access will
// severely stall MMX execution.
//
//////////////////////////////////////////////////////////////////////

#define BITS_FRW_ACC	3 //; 2 or 3 for accuracy
#define SHIFT_FRW_COL	BITS_FRW_ACC
#define SHIFT_FRW_ROW	(BITS_FRW_ACC + 17)

// v1.01 The original SHIFT_FRW_ROW constant has been replaced by a
//      "two stage" shift operation.  The 1st-shift (CLIP1) aligns the
//      intermediate 32-bit integer data to a {-32768, +32768} (16-bit word) 
//      range.  The MMX instruction "packssdw" simultaneous clips and packs
//      the intermediate-data into 16-bit format.
//      The 2nd-shift (CLIP2) restores the proper final range {-2048,+2047}

#define SHIFT_FRW_ROW_CLIP2	(4)  // 4-bit shift -> { 32768 <> 2048 }
#define SHIFT_FRW_ROW_CLIP1	( SHIFT_FRW_ROW - SHIFT_FRW_ROW_CLIP2 )

//#define RND_FRW_ROW		(262144 * (BITS_FRW_ACC - 1)) //; 1 << (SHIFT_FRW_ROW-1)
#define RND_FRW_ROW		(1 << (SHIFT_FRW_ROW-1))
//#define RND_FRW_COL		(2 * (BITS_FRW_ACC - 1)) //; 1 << (SHIFT_FRW_COL-1)
#define RND_FRW_COL		(1 << (SHIFT_FRW_COL-1))

const static __int64 one_corr = 0x0001000100010001;
const static long r_frw_row[2] = {RND_FRW_ROW, RND_FRW_ROW };

//const static short tg_1_16[4] = {13036, 13036, 13036, 13036 }; //tg * (2<<16) + 0.5
//const static short tg_2_16[4] = {27146, 27146, 27146, 27146 }; //tg * (2<<16) + 0.5
//const static short tg_3_16[4] = {-21746, -21746, -21746, -21746 }; //tg * (2<<16) + 0.5
//const static short cos_4_16[4] = {-19195, -19195, -19195, -19195 }; //cos * (2<<16) + 0.5
//const static short ocos_4_16[4] = {23170, 23170, 23170, 23170 }; //cos * (2<<15) + 0.5

//concatenated table, for forward DCT-column transformation
const static short tg_all_16[] = {
	13036, 13036, 13036, 13036,		// tg * (2<<16) + 0.5
	27146, 27146, 27146, 27146,		// tg * (2<<16) + 0.5
	-21746, -21746, -21746, -21746,	// tg * (2<<16) + 0.5
	-19195, -19195, -19195, -19195,	//cos * (2<<16) + 0.5
	23170, 23170, 23170, 23170 };	//cos * (2<<15) + 0.5

#define tg_1_16 (TABLEF + 0)
#define tg_2_16 (TABLEF + 8)
#define tg_3_16 (TABLEF + 16)
#define cos_4_16 (TABLEF + 24)
#define ocos_4_16 (TABLEF + 32)


// CONCATENATED IDCT COEFF TABLE, rows 0,1,2,3,4,5,6,7 (in order )
//
    /*
static const short tab_inv_01234567[] = { // inverse_dct coeff table
	//row0, this row is required
	16384, 16384, 16384, -16384,	// ; movq-> w06 w04 w02 w00
	21407, 8867, 8867, -21407,		// w07 w05 w03 w01
	16384, -16384, 16384, 16384,	//; w14 w12 w10 w08
	-8867, 21407, -21407, -8867,	//; w15 w13 w11 w09
	22725, 12873, 19266, -22725,	//; w22 w20 w18 w16
	19266, 4520, -4520, -12873,		//; w23 w21 w19 w17
	12873, 4520, 4520, 19266,		//; w30 w28 w26 w24
	-22725, 19266, -12873, -22725,  //w31 w29 w27 w25

	//row1
	22725, 22725, 22725, -22725,	// ; movq-> w06 w04 w02 w00
	29692, 12299, 12299, -29692,	//	; w07 w05 w03 w01
	22725, -22725, 22725, 22725,	//; w14 w12 w10 w08
	-12299, 29692, -29692, -12299,	//; w15 w13 w11 w09
	31521, 17855, 26722, -31521,	//; w22 w20 w18 w16
	26722, 6270, -6270, -17855,		//; w23 w21 w19 w17
	17855, 6270, 6270, 26722,		//; w30 w28 w26 w24
	-31521, 26722, -17855, -31521,	// w31 w29 w27 w25

	//row2
	21407, 21407, 21407, -21407,	// ; movq-> w06 w04 w02 w00
	27969, 11585, 11585, -27969,	// ; w07 w05 w03 w01
	21407, -21407, 21407, 21407,	// ; w14 w12 w10 w08
	-11585, 27969, -27969, -11585,	//  ;w15 w13 w11 w09
	29692, 16819, 25172, -29692, 	// ;w22 w20 w18 w16
	25172, 5906, -5906, -16819, 	// ;w23 w21 w19 w17
	16819, 5906, 5906, 25172, 		// ;w30 w28 w26 w24
	-29692, 25172, -16819, -29692,	//  ;w31 w29 w27 w25

	//row3
	19266, 19266, 19266, -19266,	//; movq-> w06 w04 w02 w00
	25172, 10426, 10426, -25172,	//; w07 w05 w03 w01
	19266, -19266, 19266, 19266,	//; w14 w12 w10 w08
	-10426, 25172, -25172, -10426,	//; w15 w13 w11 w09
	26722, 15137, 22654, -26722,	//; w22 w20 w18 w16
	22654, 5315, -5315, -15137,		//; w23 w21 w19 w17
	15137, 5315, 5315, 22654,		//; w30 w28 w26 w24
	-26722, 22654, -15137, -26722,	//; w31 w29 w27 w25

	//row4
	16384, 16384, 16384, -16384,	// ; movq-> w06 w04 w02 w00
	21407, 8867, 8867, -21407,		// w07 w05 w03 w01
	16384, -16384, 16384, 16384,	//; w14 w12 w10 w08
	-8867, 21407, -21407, -8867,	//; w15 w13 w11 w09
	22725, 12873, 19266, -22725,	//; w22 w20 w18 w16
	19266, 4520, -4520, -12873,		//; w23 w21 w19 w17
	12873, 4520, 4520, 19266,		//; w30 w28 w26 w24
	-22725, 19266, -12873, -22725,  //w31 w29 w27 w25

	//row5
	19266, 19266, 19266, -19266,	//; movq-> w06 w04 w02 w00
	25172, 10426, 10426, -25172,	//; w07 w05 w03 w01
	19266, -19266, 19266, 19266,	//; w14 w12 w10 w08
	-10426, 25172, -25172, -10426,	//; w15 w13 w11 w09
	26722, 15137, 22654, -26722,	//; w22 w20 w18 w16
	22654, 5315, -5315, -15137,		//; w23 w21 w19 w17
	15137, 5315, 5315, 22654,		//; w30 w28 w26 w24
	-26722, 22654, -15137, -26722,	//; w31 w29 w27 w25

	//row6
	21407, 21407, 21407, -21407,	// ; movq-> w06 w04 w02 w00
	27969, 11585, 11585, -27969,	// ; w07 w05 w03 w01
	21407, -21407, 21407, 21407,	// ; w14 w12 w10 w08
	-11585, 27969, -27969, -11585,	//  ;w15 w13 w11 w09
	29692, 16819, 25172, -29692, 	// ;w22 w20 w18 w16
	25172, 5906, -5906, -16819, 	// ;w23 w21 w19 w17
	16819, 5906, 5906, 25172, 		// ;w30 w28 w26 w24
	-29692, 25172, -16819, -29692,	//  ;w31 w29 w27 w25

	//row7
	22725, 22725, 22725, -22725,	// ; movq-> w06 w04 w02 w00
	29692, 12299, 12299, -29692,	//	; w07 w05 w03 w01
	22725, -22725, 22725, 22725,	//; w14 w12 w10 w08
	-12299, 29692, -29692, -12299,	//; w15 w13 w11 w09
	31521, 17855, 26722, -31521,	//; w22 w20 w18 w16
	26722, 6270, -6270, -17855,		//; w23 w21 w19 w17
	17855, 6270, 6270, 26722,		//; w30 w28 w26 w24
	-31521, 26722, -17855, -31521};	// w31 w29 w27 w25
*/

static const short tab_frw_01234567[] = {  // forward_dct coeff table
    //row0
    16384, 16384, 21407, -8867,     //    w09 w01 w08 w00
    16384, 16384, 8867, -21407,     //    w13 w05 w12 w04
    16384, -16384, 8867, 21407,     //    w11 w03 w10 w02
    -16384, 16384, -21407, -8867,   //    w15 w07 w14 w06
    22725, 12873, 19266, -22725,    //    w22 w20 w18 w16
    19266, 4520, -4520, -12873,     //    w23 w21 w19 w17
    12873, 4520, 4520, 19266,       //    w30 w28 w26 w24
    -22725, 19266, -12873, -22725,  //    w31 w29 w27 w25

    //row1
    22725, 22725, 29692, -12299,    //    w09 w01 w08 w00
    22725, 22725, 12299, -29692,    //    w13 w05 w12 w04
    22725, -22725, 12299, 29692,    //    w11 w03 w10 w02
    -22725, 22725, -29692, -12299,  //    w15 w07 w14 w06
    31521, 17855, 26722, -31521,    //    w22 w20 w18 w16
    26722, 6270, -6270, -17855,     //    w23 w21 w19 w17
    17855, 6270, 6270, 26722,       //    w30 w28 w26 w24
    -31521, 26722, -17855, -31521,  //    w31 w29 w27 w25

    //row2
    21407, 21407, 27969, -11585,    //    w09 w01 w08 w00
    21407, 21407, 11585, -27969,    //    w13 w05 w12 w04
    21407, -21407, 11585, 27969,    //    w11 w03 w10 w02
    -21407, 21407, -27969, -11585,  //    w15 w07 w14 w06
    29692, 16819, 25172, -29692,    //    w22 w20 w18 w16
    25172, 5906, -5906, -16819,     //    w23 w21 w19 w17
    16819, 5906, 5906, 25172,       //    w30 w28 w26 w24
    -29692, 25172, -16819, -29692,  //    w31 w29 w27 w25

    //row3
    19266, 19266, 25172, -10426,    //    w09 w01 w08 w00
    19266, 19266, 10426, -25172,    //    w13 w05 w12 w04
    19266, -19266, 10426, 25172,    //    w11 w03 w10 w02
    -19266, 19266, -25172, -10426,  //    w15 w07 w14 w06, 
    26722, 15137, 22654, -26722,    //    w22 w20 w18 w16
    22654, 5315, -5315, -15137,     //    w23 w21 w19 w17
    15137, 5315, 5315, 22654,       //    w30 w28 w26 w24
    -26722, 22654, -15137, -26722,  //    w31 w29 w27 w25, 

    //row4
    16384, 16384, 21407, -8867,     //    w09 w01 w08 w00
    16384, 16384, 8867, -21407,     //    w13 w05 w12 w04
    16384, -16384, 8867, 21407,     //    w11 w03 w10 w02
    -16384, 16384, -21407, -8867,   //    w15 w07 w14 w06
    22725, 12873, 19266, -22725,    //    w22 w20 w18 w16
    19266, 4520, -4520, -12873,     //    w23 w21 w19 w17
    12873, 4520, 4520, 19266,       //    w30 w28 w26 w24
    -22725, 19266, -12873, -22725,  //    w31 w29 w27 w25 

    //row5
    19266, 19266, 25172, -10426,    //    w09 w01 w08 w00
    19266, 19266, 10426, -25172,    //    w13 w05 w12 w04
    19266, -19266, 10426, 25172,    //    w11 w03 w10 w02
    -19266, 19266, -25172, -10426,  //    w15 w07 w14 w06
    26722, 15137, 22654, -26722,    //    w22 w20 w18 w16
    22654, 5315, -5315, -15137,     //    w23 w21 w19 w17
    15137, 5315, 5315, 22654,       //    w30 w28 w26 w24
    -26722, 22654, -15137, -26722,  //    w31 w29 w27 w25

    //row6
    21407, 21407, 27969, -11585,    //    w09 w01 w08 w00
    21407, 21407, 11585, -27969,    //    w13 w05 w12 w04
    21407, -21407, 11585, 27969,    //    w11 w03 w10 w02
    -21407, 21407, -27969, -11585,  //    w15 w07 w14 w06, 
    29692, 16819, 25172, -29692,    //    w22 w20 w18 w16
    25172, 5906, -5906, -16819,     //    w23 w21 w19 w17
    16819, 5906, 5906, 25172,       //    w30 w28 w26 w24
    -29692, 25172, -16819, -29692,  //    w31 w29 w27 w25, 

    //row7
    22725, 22725, 29692, -12299,    //    w09 w01 w08 w00
    22725, 22725, 12299, -29692,    //    w13 w05 w12 w04
    22725, -22725, 12299, 29692,    //    w11 w03 w10 w02
    -22725, 22725, -29692, -12299,  //    w15 w07 w14 w06, 
    31521, 17855, 26722, -31521,    //    w22 w20 w18 w16
    26722, 6270, -6270, -17855,     //    w23 w21 w19 w17
    17855, 6270, 6270, 26722,       //    w30 w28 w26 w24
    -31521, 26722, -17855, -31521   //    w31 w29 w27 w25
};



void
fdct_mm32( short *blk )
{
    static __int64 xt70[2]; // xt7xt6xt5xt4, xt3xt2xt1xt0
    static int a0, a1, a2, a3, b0, b1, b2, b3;
    static short *sptr, *optr, *tf; // tf = table_ptr
    static short *xt = (short *) &xt70[0];
    static int j;
    
    const static short _tg_1_16   = 13036;  //tg * (2<<16) + 0.5
    const static short _tg_2_16   = 27146;  //tg * (2<<16) + 0.5
    const static short _tg_3_16   =-21746;  //tg * (2<<16) + 0.5
    const static short _cos_4_16  =-19195;  //cos * (2<<16) + 0.5
    const static short _ocos_4_16 = 23170;  //cos * (2<<15) + 0.5
    const static short _one_corr  =     1;  //rounding compensation

    static short t0, t1, t2, t3, t4, t5, t6, t7;
    static short tp03, tm03, tp12, tm12, tp65, tm65;
    static short tp465, tm465, tp765, tm765;

    __asm {

    ////////////////////////////////////////////////////////////////////////
    //
    // The high-level pseudocode for the fdct_mm32() routine :
    //
    // fdct_mm32()
    // {
    //    forward_dct_col03(); // dct_column transform on cols 0-3
    //    forward_dct_col47(); // dct_column transform on cols 4-7
    //    for ( j = 0; j < 8; j=j+1 )
    //      forward_dct_row1(j); // dct_row transform on row #j
    // }

	mov INP, dword ptr [blk];		;// input data is row 0 of blk[]
    ;// transform the left half of the matrix (4 columns)

    lea TABLEF, dword ptr [tg_all_16];
    mov OUT, INP;

//	lea round_frw_col, dword ptr [r_frw_col]
    // for ( i = 0; i < 2; i = i + 1)
    // the for-loop is executed twice.  We are better off unrolling the 
    // loop to avoid branch misprediction.
	mmx32_fdct_col03: // begin processing columns 0-3
    movq mm0, [x1] ; 0 ; x1
     ;//

    movq mm1, [x6] ; 1 ; x6
     movq mm2, mm0 ; 2 ; x1

    movq mm3, [x2] ; 3 ; x2
     paddsw mm0, mm1 ; t1 = x[1] + x[6]

    movq mm4, [x5] ; 4 ; x5
     psllw mm0, SHIFT_FRW_COL ; t1

    movq mm5, [x0] ; 5 ; x0
     paddsw mm4, mm3 ; t2 = x[2] + x[5]

    paddsw mm5, [x7] ; t0 = x[0] + x[7]
     psllw mm4, SHIFT_FRW_COL ; t2

    movq mm6, mm0 ; 6 ; t1
     psubsw mm2, mm1 ; 1 ; t6 = x[1] - x[6]

    movq mm1, qword ptr [tg_2_16] ; 1 ; tg_2_16
     psubsw mm0, mm4 ; tm12 = t1 - t2

    movq mm7, [x3] ; 7 ; x3
     pmulhw mm1, mm0 ; tm12*tg_2_16

    paddsw mm7, [x4] ; t3 = x[3] + x[4]
     psllw mm5, SHIFT_FRW_COL ; t0

    paddsw mm6, mm4 ; 4 ; tp12 = t1 + t2
     psllw mm7, SHIFT_FRW_COL ; t3

    movq mm4, mm5 ; 4 ; t0
     psubsw mm5, mm7 ; tm03 = t0 - t3

    paddsw mm1, mm5 ; y2 = tm03 + tm12*tg_2_16
     paddsw mm4, mm7 ; 7 ; tp03 = t0 + t3

    por mm1, qword ptr one_corr ; correction y2 +0.5
     psllw mm2, SHIFT_FRW_COL+1 ; t6

    pmulhw mm5, qword ptr [tg_2_16] ; tm03*tg_2_16
     movq mm7, mm4 ; 7 ; tp03

    psubsw mm3, [x5] ; t5 = x[2] - x[5]
     psubsw mm4, mm6 ; y4 = tp03 - tp12

    movq [y2], mm1 ; 1 ; save y2
     paddsw mm7, mm6 ; 6 ; y0 = tp03 + tp12
     
    movq mm1, [x3] ; 1 ; x3
     psllw mm3, SHIFT_FRW_COL+1 ; t5

    psubsw mm1, [x4] ; t4 = x[3] - x[4]
     movq mm6, mm2 ; 6 ; t6
    
    movq [y4], mm4 ; 4 ; save y4
     paddsw mm2, mm3 ; t6 + t5

    pmulhw mm2, qword ptr [ocos_4_16] ; tp65 = (t6 + t5)*cos_4_16
     psubsw mm6, mm3 ; 3 ; t6 - t5

    pmulhw mm6, qword ptr [ocos_4_16] ; tm65 = (t6 - t5)*cos_4_16
     psubsw mm5, mm0 ; 0 ; y6 = tm03*tg_2_16 - tm12

    por mm5, qword ptr one_corr ; correction y6 +0.5
     psllw mm1, SHIFT_FRW_COL ; t4

    por mm2, qword ptr one_corr ; correction tp65 +0.5
     movq mm4, mm1 ; 4 ; t4

    movq mm3, [x0] ; 3 ; x0
     paddsw mm1, mm6 ; tp465 = t4 + tm65

    psubsw mm3, [x7] ; t7 = x[0] - x[7]
     psubsw mm4, mm6 ; 6 ; tm465 = t4 - tm65

    movq mm0, qword ptr [tg_1_16] ; 0 ; tg_1_16
     psllw mm3, SHIFT_FRW_COL ; t7

    movq mm6, qword ptr [tg_3_16] ; 6 ; tg_3_16
     pmulhw mm0, mm1 ; tp465*tg_1_16

    movq [y0], mm7 ; 7 ; save y0
     pmulhw mm6, mm4 ; tm465*tg_3_16

    movq [y6], mm5 ; 5 ; save y6
     movq mm7, mm3 ; 7 ; t7

    movq mm5, qword ptr [tg_3_16] ; 5 ; tg_3_16
     psubsw mm7, mm2 ; tm765 = t7 - tp65

    paddsw mm3, mm2 ; 2 ; tp765 = t7 + tp65
     pmulhw mm5, mm7 ; tm765*tg_3_16

    paddsw mm0, mm3 ; y1 = tp765 + tp465*tg_1_16
     paddsw mm6, mm4 ; tm465*tg_3_16

    pmulhw mm3, qword ptr [tg_1_16] ; tp765*tg_1_16
     ;//

    por mm0, qword ptr one_corr ; correction y1 +0.5
     paddsw mm5, mm7 ; tm765*tg_3_16

    psubsw mm7, mm6 ; 6 ; y3 = tm765 - tm465*tg_3_16
     add INP, 0x08   ; // increment pointer

    movq [y1], mm0 ; 0 ; save y1
     paddsw mm5, mm4 ; 4 ; y5 = tm765*tg_3_16 + tm465

    movq [y3], mm7 ; 7 ; save y3
     psubsw mm3, mm1 ; 1 ; y7 = tp765*tg_1_16 - tp465

    movq [y5], mm5 ; 5 ; save y5


  mmx32_fdct_col47: // begin processing columns 4-7
    movq mm0, [x1] ; 0 ; x1
     ;//
    movq [y7], mm3 ; 3 ; save y7 (columns 0-4)
     ;//

    movq mm1, [x6] ; 1 ; x6
     movq mm2, mm0 ; 2 ; x1

    movq mm3, [x2] ; 3 ; x2
     paddsw mm0, mm1 ; t1 = x[1] + x[6]

    movq mm4, [x5] ; 4 ; x5
     psllw mm0, SHIFT_FRW_COL ; t1

    movq mm5, [x0] ; 5 ; x0
     paddsw mm4, mm3 ; t2 = x[2] + x[5]

    paddsw mm5, [x7] ; t0 = x[0] + x[7]
     psllw mm4, SHIFT_FRW_COL ; t2

    movq mm6, mm0 ; 6 ; t1
     psubsw mm2, mm1 ; 1 ; t6 = x[1] - x[6]

    movq mm1, qword ptr [tg_2_16] ; 1 ; tg_2_16
     psubsw mm0, mm4 ; tm12 = t1 - t2

    movq mm7, [x3] ; 7 ; x3
     pmulhw mm1, mm0 ; tm12*tg_2_16

    paddsw mm7, [x4] ; t3 = x[3] + x[4]
     psllw mm5, SHIFT_FRW_COL ; t0

    paddsw mm6, mm4 ; 4 ; tp12 = t1 + t2
     psllw mm7, SHIFT_FRW_COL ; t3

    movq mm4, mm5 ; 4 ; t0
     psubsw mm5, mm7 ; tm03 = t0 - t3

    paddsw mm1, mm5 ; y2 = tm03 + tm12*tg_2_16
     paddsw mm4, mm7 ; 7 ; tp03 = t0 + t3

    por mm1, qword ptr one_corr ; correction y2 +0.5
     psllw mm2, SHIFT_FRW_COL+1 ; t6

    pmulhw mm5, qword ptr [tg_2_16] ; tm03*tg_2_16
     movq mm7, mm4 ; 7 ; tp03

    psubsw mm3, [x5] ; t5 = x[2] - x[5]
     psubsw mm4, mm6 ; y4 = tp03 - tp12

    movq [y2+8], mm1 ; 1 ; save y2
     paddsw mm7, mm6 ; 6 ; y0 = tp03 + tp12
     
    movq mm1, [x3] ; 1 ; x3
     psllw mm3, SHIFT_FRW_COL+1 ; t5

    psubsw mm1, [x4] ; t4 = x[3] - x[4]
     movq mm6, mm2 ; 6 ; t6
    
    movq [y4+8], mm4 ; 4 ; save y4
     paddsw mm2, mm3 ; t6 + t5

    pmulhw mm2, qword ptr [ocos_4_16] ; tp65 = (t6 + t5)*cos_4_16
     psubsw mm6, mm3 ; 3 ; t6 - t5

    pmulhw mm6, qword ptr [ocos_4_16] ; tm65 = (t6 - t5)*cos_4_16
     psubsw mm5, mm0 ; 0 ; y6 = tm03*tg_2_16 - tm12

    por mm5, qword ptr one_corr ; correction y6 +0.5
     psllw mm1, SHIFT_FRW_COL ; t4

    por mm2, qword ptr one_corr ; correction tp65 +0.5
     movq mm4, mm1 ; 4 ; t4

    movq mm3, [x0] ; 3 ; x0
     paddsw mm1, mm6 ; tp465 = t4 + tm65

    psubsw mm3, [x7] ; t7 = x[0] - x[7]
     psubsw mm4, mm6 ; 6 ; tm465 = t4 - tm65

    movq mm0, qword ptr [tg_1_16] ; 0 ; tg_1_16
     psllw mm3, SHIFT_FRW_COL ; t7

    movq mm6, qword ptr [tg_3_16] ; 6 ; tg_3_16
     pmulhw mm0, mm1 ; tp465*tg_1_16

    movq [y0+8], mm7 ; 7 ; save y0
     pmulhw mm6, mm4 ; tm465*tg_3_16

    movq [y6+8], mm5 ; 5 ; save y6
     movq mm7, mm3 ; 7 ; t7

    movq mm5, qword ptr [tg_3_16] ; 5 ; tg_3_16
     psubsw mm7, mm2 ; tm765 = t7 - tp65

    paddsw mm3, mm2 ; 2 ; tp765 = t7 + tp65
     pmulhw mm5, mm7 ; tm765*tg_3_16

    paddsw mm0, mm3 ; y1 = tp765 + tp465*tg_1_16
     paddsw mm6, mm4 ; tm465*tg_3_16

    pmulhw mm3, qword ptr [tg_1_16] ; tp765*tg_1_16
     ;//

    por mm0, qword ptr one_corr ; correction y1 +0.5
     paddsw mm5, mm7 ; tm765*tg_3_16

    psubsw mm7, mm6 ; 6 ; y3 = tm765 - tm465*tg_3_16
     ;//

    movq [y1+8], mm0 ; 0 ; save y1
     paddsw mm5, mm4 ; 4 ; y5 = tm765*tg_3_16 + tm465

    movq [y3+8], mm7 ; 7 ; save y3
     psubsw mm3, mm1 ; 1 ; y7 = tp765*tg_1_16 - tp465

    movq [y5+8], mm5 ; 5 ; save y5

    movq [y7+8], mm3 ; 3 ; save y7

  //   emms;
  //  }   // end of forward_dct_col07() 
    //  done with dct_col transform


  ////////////////////////////////////////////////////////////////////////
  //
  // fdct_mmx32_rows() --
  // the following subroutine performs the row-transform operation,
  //
  //  The output is stored into blk[], destroying the original
  //  source data.

  //  v1.01 - output is range-clipped to {-2048, +2047}

	mov INP, dword ptr [blk];		;// row 0
	 mov edi, 0x08;	//x = 8

	lea TABLE, dword ptr [tab_frw_01234567]; // row 0
	 mov OUT, INP;

	lea round_frw_row, dword ptr [r_frw_row];
	// for ( x = 8; x > 0; --x )  // transform 1 row per iteration

// ---------- loop begin
  lp_mmx_fdct_row1:
    movd mm5, dword ptr [INP+12]; // mm5 = 7 6

    punpcklwd mm5, dword ptr [INP+8] // mm5 =  5 7 4 6

    movq mm2, mm5;     // mm2 = 5 7 4 6
    psrlq mm5, 32;     // mm5 = _ _ 5 7

    movq mm0, qword ptr [INP]; // mm0 = 3 2 1 0
    punpcklwd mm5, mm2;// mm5 = 4 5 6 7

    movq mm1, mm0;     // mm1 = 3 2 1 0
    paddsw mm0, mm5;   // mm0 = [3+4, 2+5, 1+6, 0+7] (xt3, xt2, xt1, xt0)

    psubsw mm1, mm5;   // mm1 = [3-4, 2-5, 1-6, 0-7] (xt7, xt6, xt5, xt4)
    movq mm2, mm0;     // mm2 = [ xt3 xt2 xt1 xt0 ]

    //movq [ xt3xt2xt1xt0 ], mm0; // debugging
    //movq [ xt7xt6xt5xt4 ], mm1; // debugging

    punpcklwd mm0, mm1;// mm0 = [ xt5 xt1 xt4 xt0 ]

    punpckhwd mm2, mm1;// mm2 = [ xt7 xt3 xt6 xt2 ]
    movq mm1, mm2;     // mm1

    ;// shuffle bytes around

//  movq mm0, qword ptr [INP] ; 0 ; x3 x2 x1 x0

//  movq mm1, qword ptr [INP+8] ; 1 ; x7 x6 x5 x4
    movq mm2, mm0 ; 2 ; x3 x2 x1 x0

    movq mm3, qword ptr [TABLE] ; 3 ; w06 w04 w02 w00
    punpcklwd mm0, mm1 ; x5 x1 x4 x0

    movq mm5, mm0 ; 5 ; x5 x1 x4 x0
    punpckldq mm0, mm0 ; x4 x0 x4 x0  [ xt2 xt0 xt2 xt0 ]

    movq mm4, qword ptr [TABLE+8] ; 4 ; w07 w05 w03 w01
    punpckhwd mm2, mm1 ; 1 ; x7 x3 x6 x2

    pmaddwd mm3, mm0 ; x4*w06+x0*w04 x4*w02+x0*w00
    movq mm6, mm2 ; 6 ; x7 x3 x6 x2

    movq mm1, qword ptr [TABLE+32] ; 1 ; w22 w20 w18 w16
    punpckldq mm2, mm2 ; x6 x2 x6 x2  [ xt3 xt1 xt3 xt1 ]

    pmaddwd mm4, mm2 ; x6*w07+x2*w05 x6*w03+x2*w01
    punpckhdq mm5, mm5 ; x5 x1 x5 x1  [ xt6 xt4 xt6 xt4 ]

    pmaddwd mm0, qword ptr [TABLE+16] ; x4*w14+x0*w12 x4*w10+x0*w08
    punpckhdq mm6, mm6 ; x7 x3 x7 x3  [ xt7 xt5 xt7 xt5 ]

    movq mm7, qword ptr [TABLE+40] ; 7 ; w23 w21 w19 w17
    pmaddwd mm1, mm5 ; x5*w22+x1*w20 x5*w18+x1*w16
//mm3 = a1, a0 (y2,y0)
//mm1 = b1, b0 (y3,y1)
//mm0 = a3,a2  (y6,y4)
//mm5 = b3,b2  (y7,y5)

    paddd mm3, qword ptr [round_frw_row] ; +rounder (y2,y0)
    pmaddwd mm7, mm6 ; x7*w23+x3*w21 x7*w19+x3*w17

    pmaddwd mm2, qword ptr [TABLE+24] ; x6*w15+x2*w13 x6*w11+x2*w09
    paddd mm3, mm4 ; 4 ; a1=sum(even1) a0=sum(even0) // now ( y2, y0)

    pmaddwd mm5, qword ptr [TABLE+48] ; x5*w30+x1*w28 x5*w26+x1*w24
    ;//

    pmaddwd mm6, qword ptr [TABLE+56] ; x7*w31+x3*w29 x7*w27+x3*w25
    paddd mm1, mm7 ; 7 ; b1=sum(odd1) b0=sum(odd0) // now ( y3, y1)

    paddd mm0, qword ptr [round_frw_row] ; +rounder (y6,y4)
    psrad mm3, SHIFT_FRW_ROW_CLIP1 ;// (y2, y0) 

    paddd mm1, qword ptr [round_frw_row] ; +rounder (y3,y1)
    paddd mm0, mm2 ; 2 ; a3=sum(even3) a2=sum(even2) // now (y6, y4)

    paddd mm5, qword ptr [round_frw_row] ; +rounder (y7,y5)
    psrad mm1, SHIFT_FRW_ROW_CLIP1 ;// y1=a1+b1 y0=a0+b0

    paddd mm5, mm6 ; 6 ; b3=sum(odd3) b2=sum(odd2) // now ( y7, y5)
    psrad mm0, SHIFT_FRW_ROW_CLIP1 ;//y3=a3+b3 y2=a2+b2

    add OUT, 16;  // increment row-output address by 1 row
    psrad mm5, SHIFT_FRW_ROW_CLIP1;// y4=a3-b3 y5=a2-b2

    add INP, 16;  // increment row-address by 1 row
    packssdw mm3, mm0 ;// 0 ; y6 y4 y2 y0, saturate {-32768,+32767}

    packssdw mm1, mm5 ;// 3 ; y7 y5 y3 y1, saturate {-32768,+32767}
    movq mm6, mm3;    // mm0 = y6 y4 y2 y0

    punpcklwd mm3, mm1; // y3 y2 y1 y0
    sub edi, 0x01;   // i = i - 1
    
    punpckhwd mm6, mm1; // y7 y6 y5 y4
    add TABLE,64;  // increment to next table

    psraw mm3, SHIFT_FRW_ROW_CLIP2;  // descale [y3 y2 y1 y0] to {-2048,+2047}

    psraw mm6, SHIFT_FRW_ROW_CLIP2;  // descale [y7 y6 y5 y4] to {-2048,+2047}

    movq qword ptr [OUT-16], mm3 ; 1 ; save y3 y2 y1 y0

    movq qword ptr [OUT-8], mm6 ; 7 ; save y7 y6 y5 y4

    cmp edi, 0x00;
    jg lp_mmx_fdct_row1;  // begin fdct processing on next row
    emms;
    }
    
/*
    ////////////////////////////////////////////////////////////////////////
    //
    // DCT_8_FRW_COL(), equivalent c_code
    //
    // This C-code can be substituted for the same __asm block
    //
    // I found several *DISCREPANCIES* between the AP-922 C-listing 
    // and actual corrected code (shown below).
    //
    ////////////////////////////////////////////////////////////////////////

    sptr = (short *) blk;
    optr = (short *) blk; // output will overwrite source data!

    for ( j = 0; j < 8; j=j+1 ) // dct_frw_col1 loop
    {
      // read source-data column #j into xt[0..7]
      xt[7] = sptr[7*8];
      xt[6] = sptr[6*8];
      xt[5] = sptr[5*8];
      xt[4] = sptr[4*8];

      xt[3] = sptr[3*8];
      xt[2] = sptr[2*8];
      xt[1] = sptr[1*8];
      xt[0] = sptr[0*8];
 
#define  LEFT_SHIFT( x ) ((x) << (SHIFT_FRW_COL) )   // left shift
#define LEFT_SHIFT1( x ) ((x) << (SHIFT_FRW_COL+1) ) // left shift+1

      t0 = LEFT_SHIFT ( xt[0] + xt[7] );
      t1 = LEFT_SHIFT ( xt[1] + xt[6] );
      t2 = LEFT_SHIFT ( xt[2] + xt[5] );
      t3 = LEFT_SHIFT ( xt[3] + xt[4] );
      t4 = LEFT_SHIFT ( xt[3] - xt[4] );
      t5 = LEFT_SHIFT1( xt[2] - xt[5] ); // *** DISCREPANCY
      t6 = LEFT_SHIFT1( xt[1] - xt[6] ); // *** DISCREPANCY
      t7 = LEFT_SHIFT ( xt[0] - xt[7] );
 
      tp03 = t0 + t3;
      tm03 = t0 - t3;
      tp12 = t1 + t2;
      tm12 = t1 - t2;

// pmulhw/pmulhrw emulation macros 
#define X86_PMULHW( X ) ((short) ( ((int)X)>>16 ))   //Intel MMX
//#define X86_PMULHRW( X ) ((short) ( ( (((int)X)>>15)+1) >>1) ) //3DNow-MMX

      optr[0*8] = tp03 + tp12;
      optr[4*8] = tp03 - tp12;
      optr[2*8] = tm03 + X86_PMULHW( tm12 * _tg_2_16 );
      optr[2*8] = optr[2*8] | _one_corr; // one_correction
      optr[6*8] = X86_PMULHW( tm03 * _tg_2_16 ) - tm12;
      optr[6*8] = optr[6*8] | _one_corr; // one_correction
 
      tp65 = X86_PMULHW( (t6 +t5 )*_ocos_4_16 ); // *** DISCREPANCY
      tp65 = tp65 | _one_corr; // one_correction
      tm65 = X86_PMULHW( (t6 -t5 )*_ocos_4_16 ); // *** DISCREPANCY
  
      tp765 = t7 + tp65;
      tm765 = t7 - tp65;
      tp465 = t4 + tm65;
      tm465 = t4 - tm65;
 
      optr[1*8]  = tp765 + X86_PMULHW( tp465 * _tg_1_16 );
      optr[1*8]  = optr[1*8] | _one_corr; // one_correction
      optr[7*8] = X86_PMULHW( tp765 * _tg_1_16 ) - tp465;

//    optr[5*8] = X86_PMULHW( tm765 * _tg_3_16 ) + tm465; // *** DISCREPANCY
      // from pg8 of AP-922,  ICONST = [ const*(2^16) + 0.5 ]
      //                      const * x = PMULHW( ICONST,x ) + x
      // The constant "tg_3_16" > 0.5, thus _tg_3_16 is encoded as tg_3_16-1.0
      // optr[5*8] = X86_PMULHW( tm765 * ( tg_3_16 - 1.0 ) ) + tm465
      //           = [tm765*tg_3_16 - tm765] + tm465
      //
      // optr[5*8] + tm765 = [ tm765*tg_3_16 ] + tm465 + tm765
      //                   = [ tm765*tg_3_16 ] + tm465 <-- what we want

      optr[5*8] = X86_PMULHW( tm765 * _tg_3_16 ) + tm465 + tm765;

//    optr[3*8] = tm765 - X86_PMULHW( tm465 * _tg_3_16 ); // *** DISCREPANCY
      // The following operations must be performed in the shown order!
      // same trick (as shown for optr[5*8]) applies to optr[3*8]

      optr[3*8] = X86_PMULHW( tm465 * _tg_3_16 ) + tm465;
      optr[3*8] = tm765 - optr[3*8];

    ++sptr;   // increment source pointer +1 column
    ++optr;   // increment output pointer +1 column
  } // end for ( j = 0 ..., end of C_equivalent code for forward_dct_col_1
 
    ////////////////////////////////////////////////////////////////////////
    //
    // DCT8_FRW_ROW1(), equivalent c_code
    //
    // This C-code can be substituted for the same __asm block
    // For a derivation of this code, please read fdctmm32.doc
    ////////////////////////////////////////////////////////////////////////

    sptr = (short *) blk;
    optr = (short *) blk; // output will overwrite source data!
    tf = &tab_frw_01234567[ 0 ]; // fdct_row load table_forward_w

    for ( j = 0; j < 8; j=j+1 ) // dct_frw_row1 loop
    {
        // forward_dct_row input arithmetic + shuffle
        xt[3] = sptr[3] + sptr[4];
        xt[2] = sptr[2] + sptr[5];
        xt[1] = sptr[1] + sptr[6];
        xt[0] = sptr[0] + sptr[7];

        xt[7] = sptr[3] - sptr[4];
        xt[6] = sptr[2] - sptr[5];
        xt[5] = sptr[1] - sptr[6];
        xt[4] = sptr[0] - sptr[7];
  

        a3 = ( xt[0]*tf[10]+ xt[2]*tf[11]) + ( xt[1]*tf[14]+ xt[3]*tf[15]);
        a2 = ( xt[0]*tf[8] + xt[2]*tf[9] ) + ( xt[1]*tf[12]+ xt[3]*tf[13]);
        a1 = ( xt[0]*tf[2] + xt[2]*tf[3] ) + ( xt[1]*tf[6] + xt[3]*tf[7] );
        a0 = ( xt[0]*tf[0] + xt[2]*tf[1] ) + ( xt[1]*tf[4] + xt[3]*tf[5] );
        tf += 16;  // increment table pointer

        b3 = ( xt[4]*tf[10]+ xt[6]*tf[11]) + ( xt[5]*tf[14]+ xt[7]*tf[15]);
        b2 = ( xt[4]*tf[8] + xt[6]*tf[9] ) + ( xt[5]*tf[12]+ xt[7]*tf[13]);
        b1 = ( xt[4]*tf[2] + xt[6]*tf[3] ) + ( xt[5]*tf[6] + xt[7]*tf[7] );
        b0 = ( xt[4]*tf[0] + xt[6]*tf[1] ) + ( xt[5]*tf[4] + xt[7]*tf[5] );
        tf += 16;  // increment table pointer

        // apply rounding constants to scaled elements
        // note, in the MMX implementation, the shift&round is done *last.*
        // Here, the C-code applies the shifts 1st, then the clipping.
#define SHIFT_AND_ROUND_FRW_ROW( x )  ( ((x)+RND_FRW_ROW) >> SHIFT_FRW_ROW )

        a3 = SHIFT_AND_ROUND_FRW_ROW( a3 );
        a2 = SHIFT_AND_ROUND_FRW_ROW( a2 );
        a1 = SHIFT_AND_ROUND_FRW_ROW( a1 );
        a0 = SHIFT_AND_ROUND_FRW_ROW( a0 );

        b3 = SHIFT_AND_ROUND_FRW_ROW( b3 );
        b2 = SHIFT_AND_ROUND_FRW_ROW( b2 );
        b1 = SHIFT_AND_ROUND_FRW_ROW( b1 );
        b0 = SHIFT_AND_ROUND_FRW_ROW( b0 );

        // v1.01, clip output results to range {-2048, +2047}

        // In the MMX implementation, the "clipper" is integrated into
        // the shift&round operation (thanks to packssdw)
        a3 = (a3 > 2047) ?  2047 : a3; // ceiling @ +2047
        a2 = (a2 > 2047) ?  2047 : a2; // ceiling @ +2047
        a1 = (a1 > 2047) ?  2047 : a1; // ceiling @ +2047
        a0 = (a0 > 2047) ?  2047 : a0; // ceiling @ +2047
        b3 = (b3 > 2047) ?  2047 : b3; // ceiling @ +2047
        b2 = (b2 > 2047) ?  2047 : b2; // ceiling @ +2047
        b1 = (b1 > 2047) ?  2047 : b1; // ceiling @ +2047
        b0 = (b0 > 2047) ?  2047 : b0; // ceiling @ +2047

        a3 = (a3 <-2048) ? -2048 : a3; // floor   @ -2048
        a2 = (a2 <-2048) ? -2048 : a2; // floor   @ -2048
        a1 = (a1 <-2048) ? -2048 : a1; // floor   @ -2048
        a0 = (a0 <-2048) ? -2048 : a0; // floor   @ -2048
        b3 = (b3 <-2048) ? -2048 : b3; // floor   @ -2048
        b2 = (b2 <-2048) ? -2048 : b2; // floor   @ -2048
        b1 = (b1 <-2048) ? -2048 : b1; // floor   @ -2048
        b0 = (b0 <-2048) ? -2048 : b0; // floor   @ -2048


        // forward_dct_row, assign outputs
        optr[ 3 ] = b1;
        optr[ 2 ] = a1;
        optr[ 1 ] = b0;
        optr[ 0 ] = a0;

        optr[ 7 ] = b3;
        optr[ 6 ] = a3;
        optr[ 5 ] = b2;
        optr[ 4 ] = a2;

        sptr += 8;   // increment source pointer +1 row
        optr += 8;   // increment output pointer +1 row
    } // end for ( j = 0 ..., end of C_equivalent code for forward_dct_row_1
  */  
} // fdct_mm32( short *blk )

