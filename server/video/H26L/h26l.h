
#ifndef __H26L_INCLUDED__
#define __H26L_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;

int H26L_Init(char *config);
int H26L_Encode(byte* pY, byte* pU, byte* pV, 
	byte* pEncoded, int* pEncodedLength);
int H26L_GetReconstructed(byte* pY, byte* pU, byte* pV);
int H26L_Close();

#ifdef __cplusplus
}
#endif

#endif /* __H26L_INCLUDED__ */
