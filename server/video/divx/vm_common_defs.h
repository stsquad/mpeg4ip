
#ifndef _VM_COMMON_DEFS_H_
#define _VM_COMMON_DEFS_H_

   #   ifdef __cplusplus
       extern "C" {
   #   endif /* __cplusplus */

#define VERSION		1		/* image structure version */


   #   ifdef __cplusplus
       }
   #   endif /* __cplusplus  */ 



/* maximum allowed number of VOs and VOLs */

#define MAX_NUM_VOS 32
#define MAX_NUM_VOLS 16

/* end of bitstream code */

#define EOB_CODE                        1
#define EOB_CODE_LENGTH                32


/*** 10/28 TEST */
/* #define MB_trace_thres	22 */
#define MB_trace_thres	8193 /* changed */
/* 10/28 TEST ***/

#define EXTENDED_PAR 0xF

/* session layer and vop layer start codes */

#define SESSION_START_CODE 	0x01B0	
#define SESSION_END_CODE 	0x01B1

#define VO_START_CODE 		0x8      
#define VO_START_CODE_LENGTH	27
                                         
#define VO_HEADER_LENGTH        32        /* lengtho of VO header: VO_START_CODE +  VO_ID */

#define SOL_START_CODE          0x01be   
#define SOL_START_CODE_LENGTH   32

#define VOL_START_CODE 0x12             /* 25-MAR-97 JDL : according to WD2 */
#define VOL_START_CODE_LENGTH 28

#define VOP_START_CODE 0x1B6		 	/* 25-MAR-97 JDL : according to WD2 */
#define VOP_START_CODE_LENGTH	32	

#define GROUP_START_CODE	0x01B3		/* 05-05-1997 Minhua Zhou */
#define GROUP_START_CODE_LENGTH  32        /* 10.12.97 Luis Ducla-Soares */

#define VOP_ID_CODE_LENGTH		5
#define VOP_TEMP_REF_CODE_LENGTH	16

#define USER_DATA_START_CODE	    0x01B2	/* Due to N2171 Cl. 2.1.9, MW 23-MAR-1998 */
#define USER_DATA_START_CODE_LENGTH 32		/* Due to N2171 Cl. 2.1.9, MW 23-MAR-1998 */

#define START_CODE_PREFIX	    0x01	/* Due to N2171 Cl. 2.1.9, MW 23-MAR-1998 */
#define START_CODE_PREFIX_LENGTH    24		/* Due to N2171 Cl. 2.1.9, MW 23-MAR-1998 */

#define SHORT_VIDEO_START_MARKER         0x20 
#define SHORT_VIDEO_START_MARKER_LENGTH  22   
#define SHORT_VIDEO_END_MARKER            0x3F    

#define GOB_RESYNC_MARKER         0x01 
#define GOB_RESYNC_MARKER_LENGTH  17   

/* motion and resync markers used in error resilient mode  */

#define DC_MARKER                      438273    /* 09.10.97 LDS: according to WD4.0 */
#define DC_MARKER_LENGTH                19

#define MOTION_MARKER_COMB             126977    /* 26.04.97 LDS: according to VM7.0 */
#define MOTION_MARKER_COMB_LENGTH       17

#define MOTION_MARKER_SEP              81921     /* 26.04.97 LDS: according to VM6.0 */
#define MOTION_MARKER_SEP_LENGTH        17

#define RESYNC_MARKER           1           /* 26.04.97 LDS: according to VM6.0 */
#define RESYNC_MARKER_LENGTH    17

#define SPRITE_NOT_USED		0
#define STATIC_SPRITE		1
#define GMC_SPRITE		2		/* NTT for GMC coding */

/* macroblock size */
#define MB_SIZE 16

/* VOL types */

#define RECTANGULAR 0
#define BINARY 1
#define BINARY_SHAPE_ONLY 2 /* HYUNDAI (Grayscale) */
#define GREY_SCALE 3 	/* HYUNDAI (Grayscale) */

/* macroblock modes */

#define MODE_INTRA                      0
#define MODE_INTER                      1
#define MODE_INTRA_Q			2	/* not used currently */
#define MODE_INTER_Q			3 	/* not used currently */	
#define MODE_INTER4V                    4
#define MODE_GMC                        5	/* NTT for GMC coding */
#define MODE_GMC_Q                      6

#define MBM_INTRA 			0
#define MBM_INTER16 			1
#define MBM_SPRITE 			3
#define MBM_INTER8 			4
#define MBM_TRANSPARENT 		2
#define MBM_OUT 			5
#define MBM_SKIPPED			6


/* (from mot_est.h) */
#define MBM_OPAQUE         7  /* opaque block value (all pixels 1 or 255)          */
#define MBM_BOUNDARY       8  /* block in the boundary of the shape => transparent */

#define MBM_FIELD00         9   /* ref(Top)=Top, ref(Bot)=Top */
#define MBM_FIELD01         10  /* ref(Top)=Top, ref(Bot)=Bot */
#define MBM_FIELD10         11  /* ref(Top)=Bot, ref(Bot)=Top */
#define MBM_FIELD11         12  /* ref(Top)=Bot, ref(Bot)=Bot */

#define MBM_B_MODE      0x07    /* Mode mask */
#define MBM_B_FWDFRM    0       /* Forward frame prediction */
#define MBM_B_BAKFRM    1       /* Backward frame prediction */
#define MBM_B_AVEFRM    2       /* Average (bidirectional) frame prediction */
#define MBM_B_DIRECT    3       /* Direct mode */
/* nothing defined */           /* Transparent */
#define MBM_B_FWDFLD    5       /* Forward field prediction */
#define MBM_B_BAKFLD    6       /* Backward field prediction */
#define MBM_B_AVEFLD    7       /* Average (bidirectional) field prediction */
#define MBM_B_REFFLDS   0xF0    /* Mask of reference file selectors */
#define MBM_B_FWDTOP    0x10    /* Fwd Top fld reference is bot if set */
#define MBM_B_FWDBOT    0x20    /* Fwd Bot fld reference is bot if set */
#define MBM_B_BAKTOP    0x40    /* Bak Top fld reference is bot if set */
#define MBM_B_BAKBOT    0x80    /* Bak Bot fld reference is bot if set */

 
/* typedef enum
   {
   MBMODE_INTRA=0,
   MBMODE_INTER16=1,
   MBMODE_INTER8=4,
   MBMODE_TRANSPARENT=2,
   MBMODE_OUT=5,
   MBMODE_SPRITE=3
   } MBMODE; */
/* (from mot_est.h) */


#define BINARY_ALPHA 		255
#define BINARY_SHAPE 		1
#define ARB_SHAPE 			1

#define NO_SHAPE_EFFECTS 0

#define REVERSE_VLC          1      /* 26.04.97 LDS */

#endif
