/*****************************************************************************
 *                                                                           *
"This software module was originally developed by 

Bodo Teichmann (Fraunhofer Gesellschaft IIS)

in the course of development of the MPEG-2 AAC/MPEG-4 Audio standard ISO/IEC 13818-7, 
14496-1,2 and 3. This software module is an implementation of a part of one or more 
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4 
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio 
standards free license to this software module or modifications thereof for use in 
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or 
software products are advised that this use may infringe existing patents. 
The original developer of this software module and his/her company, the subsequent 
editors and their companies, and ISO/IEC have no liability for use of this software 
module or modifications thereof in an implementation. Copyright is not released for 
non MPEG-2 AAC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 AAC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996, 1997, 1998.
 *                                                                           *
 ****************************************************************************/

int  samplFreqIndex[] = {
  96000,  88200,  64000,  48000,  44100,  32000,  24000,  22050,  16000,  12000,  11025,  8000,  7350,
  -1,
  -1,
  -1,
  -1
};
int coreSamplingRate[] = {
   8000,  7350,  8000,     8000,   7350,   8000,   8000,   7350,  8000,    12000,  11025, 8000,    0,0
};

int no_of_dc_groups[] = {
 4,         4,        5,     5,      5,     6,       8,      8,       8,     10,    10,     9,     0 ,   
 0
};

int diff_short_lines[] = {
   8,       8,       13,     18,     18,     26,    36,     36,     54,     104,   104,    104,     0,
0
};
