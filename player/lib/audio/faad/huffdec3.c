/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by 
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of 
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7, 
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
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the 
code to a third party and to inhibit third party from using the code for non 
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works." 
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/

#include "all.h"
#include "port.h"

static 
#ifdef _WIN32
int __cdecl
#else
int
#endif
huffcmp(const void *va, const void *vb)
{
    Huffman *a, *b;

    a = (Huffman *)va;
    b = (Huffman *)vb;
    if (a->len < b->len)
	return -1;
    if ( (a->len == b->len) && (a->cw < b->cw) )
	return -1;
    return 1;
}

/*
 * initialize the Hcb structure and sort the Huffman
 * codewords by length, shortest (most probable) first
 */
void
hufftab(Hcb *hcb, Huffman *hcw, int dim, int lav, int signed_cb)
{
    int i, n;
    
    if (!signed_cb) {
	hcb->mod = lav + 1;
        hcb->off = 0;
    }
    else {
	hcb->mod = 2*lav + 1;
        hcb->off = lav;
    }
    n=1;	    
    for (i=0; i<dim; i++)
	n *= hcb->mod;
    hcb->n = n;
    hcb->dim = dim;
    hcb->lav = lav;
    hcb->signed_cb = signed_cb;
    hcb->hcw = hcw;

    qsort(hcw, n, sizeof(Huffman), huffcmp);
}
 
/*
 * Cword is working buffer to which shorts from
 *   bitstream are written. Bits are read from msb to lsb
 * Nbits is number of lsb bits not yet consumed
 * 
 * this uses a minimum-memory method of Huffman decoding
 */
int decode_huff_cw(Huffman *h)
{
    int i, j;
    long cw;

    i = h->len;
    cw = getbits(i);

    while ((unsigned long)cw != h->cw)
	{
		h++;
		j = h->len-i;
		i += j;
		cw <<= j;
		cw |= getbits(j);
    }
    return(h->index);
}


void unpack_idx(int *qp, int idx, Hcb *hcb)
{
    if(hcb->dim == 4)
	{
		qp[0] = (idx/(hcb->mod*hcb->mod*hcb->mod)) - hcb->off;
		idx -= (qp[0] + hcb->off)*(hcb->mod*hcb->mod*hcb->mod);
		qp[1] = (idx/(hcb->mod*hcb->mod)) - hcb->off;
		idx -= (qp[1] + hcb->off)*(hcb->mod*hcb->mod);
		qp[2] = (idx/(hcb->mod)) - hcb->off;
		idx -= (qp[2] + hcb->off)*(hcb->mod);
		qp[3] = (idx) - hcb->off;
    }
    else
	{
		qp[0] = (idx/(hcb->mod)) - hcb->off;
		idx -= (qp[0] + hcb->off)*(hcb->mod);
		qp[1] = (idx) - hcb->off;
    }
}

/* get sign bits */
void get_sign_bits(int *q, int n)
{
    long sign_bits, mask;
    int i, bits;

    bits = 0;
    for (i = 0; i < n; i++) {
      if (q[i]) bits++;
    }
    sign_bits = getbits(bits);
    for (i = 0, mask = (1 << (bits - 1)); i < n; i++) {
      if (q[i]) {
	if (sign_bits & mask) {
	  q[i] = -q[i];
	}
	mask >>= 1;
      }
    }
}
