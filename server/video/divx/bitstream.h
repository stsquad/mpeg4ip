
#ifndef _MOM_BITSTREAM_I_H_
#define _MOM_BITSTREAM_I_H_


/* this is a MACRO defined to accommondate the legacy MoMuSys calls. */
#define BitstreamPutBits(x, y, z) Bitstream_PutBits(z, y)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void  	Bitstream_Init (void *buffer);
void  	Bitstream_PutBits (	int n, unsigned int val);
int 	Bitstream_Close (void);
int  	Bitstream_NextStartCode (void);
int		Bitstream_GetLength(void);

#ifdef __cplusplus
}
#endif /* __cplusplus  */ 

#endif /* _MOM_BITSTREAM_I_H_ */

