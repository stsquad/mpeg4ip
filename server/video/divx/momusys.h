
                                             /********************************/
#ifndef _MOMUSYS_H_                          /* TO AVOID REENTERING CODE     */
                                             /********************************/
                                             /********************************/
#	define _MOMUSYS_H_                   /* NOW IMPOSSIBLE TO REENTER    */
                                             /********************************/
#   include "non_unix.h"
                                             /********************************/
#   if defined(C_ANSI) || defined(__STDC__)  /* IF C ANSI MODE IS SUPPORTED  */
#      define  C_IS_ANSI                     /********************************/
#      define  _C_ANSI_ 
#   endif

                                             /********************************/
#   include <stdio.h>                        /* SURE ALL Def. ARE DEFINED !  */
#   include <stdlib.h>                       /* SURE ALL Def. ARE DEFINED !  */
#   include <math.h>                         /********************************/


                                             /********************************/
#   if defined (SYSV) || defined (__STDC__) 
#      include <string.h>                    /********************************/
#      define bzero(s1, length)             memset(s1, '\0', length)
#      define bcopy(s1, s2, length)         memcpy(s1, s2  , length)
#      define bcmp(s1, s2, length)          memcmp(s1, s2, length)
#      define memzero(s1, length)           memset(s1, '\0', length)
#      define index(s1, c)                  strchr(s1, c)
#      define rindex(s1, c)                 strrchr(s1, c)
#   else
#      include <strings.h>
#      define strchr(s1, c)                 index(s1, c)
#      define strrchr(s1, c)                rindex(s1, c)
#      define memcpy(s1, s2 , length)       bcopy(s1, s2, length)
#      define memzero(s1, length)           bzero(s1, length)
#      define memcmp(s1, s2, length)        bcmp(s1, s2, length)
#   endif


                                             /********************************/
#   if !defined(FALSE) || ((FALSE)!= 0)      /* TO AVOID MULTIPLE DEFINE     */
#      define  FALSE           0             /* AND BE SURE FALSE = 0        */
#   endif                                    /********************************/
#   if !defined(TRUE)  || ((TRUE) != 1)      /* TO AVOID MULTIPLE DEFINE     */
#      define  TRUE            1             /* AND BE SURE TRUE = 1         */
#   endif                                    /********************************/

#   ifndef     NULL
#      define  NULL            0
#   endif

                     /**************************************/
                     /**** GENERAL TYPES DEFINITIONS *******/
                     /**************************************/
#define Const const

    typedef void                 Void       ;

    typedef char                 Char       ;
    typedef const    char        C_Char     ;

    typedef unsigned char        Byte       ;
    typedef Const    Byte        C_Byte     ;
    typedef unsigned char        UChar      ;
    typedef Const    UChar       C_UChar    ;
 
    typedef short    int         Short      ;
    typedef short    int         SInt       ;
    typedef unsigned short       UShort     ;
    typedef unsigned short       USInt      ;
    typedef Const    short       C_Short    ;
    typedef Const    short       C_SInt     ;
    typedef Const    UShort      C_UShort   ;
    typedef Const    UShort      C_USInt    ;

    typedef int                  Int        ;
    typedef long     int         LInt       ;
    typedef Const    int         C_Int      ;
    typedef unsigned int         U_Int      ;

    typedef unsigned int         UInt       ;
    typedef unsigned long int    ULInt      ;
    typedef Const    UInt        C_UInt     ;

    typedef float                Float      ;
    typedef Const    float       C_Float    ;

    typedef double               Double     ;
    typedef Const    double      C_Double   ;

    typedef FILE                 File       ;

/* Alternatibe type redefinition */
#if 0
    typedef long            INT32;
    typedef int             INT32;
    typedef short           INT16;
    typedef char            INT8;
    typedef unsigned int    UINT32;
    typedef unsigned short  UINT16;
    typedef unsigned char   UINT8;

 
#   if __STDC__ 
        typedef signed char INT8;
#   else
        typedef char        INT8;
#   endif

    typedef unsigned long   BITS32;
    typedef unsigned short  BITS16;
    typedef unsigned char   BYTE;
    
    typedef unsigned char   BOOL;
#endif


                     /**************************************/
                     /**** GENERAL MACRO DEFINITIONS *******/
                     /**************************************/
#   ifndef MAX
#   define  MAX(a,b)              (((a) > (b)) ? (a) : (b))
#   endif

#   ifndef MIN
#   define  MIN(a,b)              (((a) < (b)) ? (a) : (b))
#   endif

#   define  CLIP(a,i,s)           (((a) > (s)) ? (s) : MAX(a,i))
#   define  INT(a)                ((Int) (floor((Double) a)))
#	define MNINT(a)    			  ((a) < 0 ? (Int)(a - 0.5) : (Int)(a + 0.5))
#	define MAX3(x,y,z)      	  MAX(MAX(x,y),z)
#	define MIN3(x,y,z)      	  MIN(MIN(x,y),z)
#	define MEDIAN(x,y,z)   		  ((x)+(y)+(z)-MAX3(x,y,z)-MIN3(x,y,z))

#   define  POW2(a)               ((a)*(a))
#   define  SQUARE(a)             ((a)*(a))
#   define  POW3(a)               ((a)*(a)*(a))
#   define  CUBE(a)               ((a)*(a)*(a))

#   define  ABS(x)                (((x) < 0) ? -(x) : (x))
#   define  SIGN(x)               (((x) < 0) ? -1 : 1)
#   define  EVEN(a)               ((a) % 2) == 0)
#   define  ODD(a)                ((a) % 2) == 1)

#   define  STRLEN(P_string)      ((P_string==NULL) ? strlen(P_string) : -1)
#   define  TYPE_MALLOC(type,nb) ((type *) malloc(sizeof(type)*nb))
#   define  NEW(type)             ((type *) malloc(sizeof(type)   ))

#define MOMCHECK(a)	if ((a) == 0) fprintf(stdout, "MOMCHECK failed in file %s, line %i\n", __FILE__, __LINE__)

                                             /********************************/
#   ifdef C_IS_ANSI                          /* IF C ANSI MODE IS SUPPORTED  */
                                             /********************************/
#      define _ANSI_ARGS_(argv)   argv       /* MACRO FOR PROTOTYPING        */
#      define _P_(argv)           argv       /* <=> _ANSI_ARGS, BUT SHORTER! */
                                             /********************************/
#   else                                     /* IF C ANSI IS NOT SUPPORTED   */
                                             /********************************/
#      define _ANSI_ARGS_(argv)   ()         /* MACRO FOR PROTOTYPING        */
#      define _P_(argv)           ()         /* <=> _ANSI_ARGS, BUT SHORTER! */
                                             /********************************/
#   endif

#include "mom_structs.h"

#endif /* _MOMUSYS_H_ */                     /* End _MOMUSYS_H_              */
                                             /********************************/


