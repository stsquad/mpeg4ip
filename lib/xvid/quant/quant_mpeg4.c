/**************************************************************************
 *
 *    XVID MPEG-4 VIDEO CODEC
 *    mpeg-4 quantization/dequantization
 *
 *    This program is an implementation of a part of one or more MPEG-4
 *    Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *    to use this software module in hardware or software products are
 *    advised that its use may infringe existing patents or copyrights, and
 *    any such use would be at such party's own risk.  The original
 *    developer of this software module and his/her company, and subsequent
 *    editors and their companies, will have no liability for use of this
 *    software or modifications or derivatives thereof.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *    History:
 *
 *	26.01.2002    fixed  quant4_intra dcscalar signed/unsigned error
 *  20.01.2002    increased accuracy of >> divide
 *  26.12.2001    divide-by-multiplication optimization
 *  22.12.2001    [-127,127] clamping removed; minor tweaks
 *	19.11.2001    inital version <pross@cs.rmit.edu.au>
 *
 *************************************************************************/


#include "quant_mpeg4.h"
#include "quant_matrix.h"

// function pointers
quant_intraFuncPtr quant4_intra;
quant_intraFuncPtr dequant4_intra;
dequant_interFuncPtr dequant4_inter;
quant_interFuncPtr quant4_inter;


#define DIV_DIV(A,B)    ( (A) > 0 ? ((A)+((B)>>1))/(B) : ((A)-((B)>>1))/(B) )
#define SIGN(A)  ((A)>0?1:-1)
#define VM18P    3
#define VM18Q    4


// divide-by-multiply table
// need 17 bit shift (16 causes slight errors when q > 19)

#define SCALEBITS    17
#define FIX(X)        ((1UL << SCALEBITS) / (X) + 1)

static const uint32_t multipliers[32] =
{
    0,          FIX(2),     FIX(4),     FIX(6),
    FIX(8),     FIX(10),    FIX(12),    FIX(14),
    FIX(16),    FIX(18),    FIX(20),    FIX(22),
    FIX(24),    FIX(26),    FIX(28),    FIX(30),
    FIX(32),    FIX(34),    FIX(36),    FIX(38),
    FIX(40),    FIX(42),    FIX(44),    FIX(46),
    FIX(48),    FIX(50),    FIX(52),    FIX(54),
    FIX(56),    FIX(58),    FIX(60),    FIX(62)
}; 

/*    quantize intra-block

    // const int32_t quantd = DIV_DIV(VM18P*quant, VM18Q);
    //
    // level = DIV_DIV(16 * data[i], default_intra_matrix[i]);
    // coeff[i] = (level + quantd) / quant2;
*/

void quant4_intra_c(int16_t * coeff, const int16_t * data, const uint32_t quant, const uint32_t dcscalar)
{
    const uint32_t quantd = ((VM18P*quant) + (VM18Q/2)) / VM18Q;
    const uint32_t mult = multipliers[quant];
    uint32_t i;
	int16_t *intra_matrix;

	intra_matrix = get_intra_matrix();

    coeff[0] = DIV_DIV(data[0], (int32_t)dcscalar);

    for (i = 1; i < 64; i++)
    {
        if (data[i] < 0)
        {
            uint32_t level = -data[i];
            level = ((level<<4) + (intra_matrix[i]>>1)) / intra_matrix[i];
            level = ((level + quantd) * mult) >> 17;
            coeff[i] = -(int16_t)level;
        }
        else if (data[i] > 0)
        {
            uint32_t level = data[i];
            level = ((level<<4) + (intra_matrix[i]>>1)) / intra_matrix[i];
            level = ((level + quantd) * mult) >> 17;
            coeff[i] = level;
        }
        else
        {
            coeff[i] = 0;
        }
    }
}



/*    dequantize intra-block & clamp to [-2048,2047]
    // data[i] = (coeff[i] * default_intra_matrix[i] * quant2) >> 4;
*/

void dequant4_intra_c(int16_t *data, const int16_t *coeff, const uint32_t quant, const uint32_t dcscalar)
{
    uint32_t i;
	int16_t *intra_matrix;

	intra_matrix = get_intra_matrix();
    
    data[0] = coeff[0]  * dcscalar;
    if (data[0] < -2048)
    {
        data[0] = -2048;
    } 
    else if (data[0] > 2047)
    {
        data[0] = 2047;
    }

    for (i = 1; i < 64; i++)
    {
        if (coeff[i] == 0)
        {
            data[i] = 0;
        }
        else if (coeff[i] < 0)
        {
            uint32_t level = -coeff[i];
            level = (level * intra_matrix[i] * quant) >> 3;
            data[i] = (level <= 2048 ? -(int16_t)level : -2048);
        }
        else // if (coeff[i] > 0)
        {
            uint32_t level = coeff[i];
            level = (level * intra_matrix[i] * quant) >> 3;
            data[i] = (level <= 2047 ? level : 2047);
        }
    }
}



/*    quantize inter-block

    // level = DIV_DIV(16 * data[i], default_intra_matrix[i]);
    // coeff[i] = (level + quantd) / quant2;
    // sum += abs(level);
*/

uint32_t quant4_inter_c(int16_t * coeff, const int16_t * data, const uint32_t quant)
{
    const uint32_t mult = multipliers[quant];
    uint32_t sum = 0;
    uint32_t i;
	int16_t *inter_matrix;

	inter_matrix = get_inter_matrix();
    
    for (i = 0; i < 64; i++)
    {
        if (data[i] < 0)
        {
            uint32_t level = -data[i];
            level = ((level<<4) + (inter_matrix[i]>>1)) / inter_matrix[i];
            level = (level * mult) >> 17;
            sum += level;
            coeff[i] = -(int16_t)level;
        }
        else if (data[i] > 0)
        {
            uint32_t level = data[i];
            level = ((level<<4) + (inter_matrix[i]>>1)) / inter_matrix[i];
            level = (level * mult) >> 17;
            sum += level;
            coeff[i] = level;
        }
        else
        {
            coeff[i] = 0;
        }
    }
    return sum;
}



/* dequantize inter-block & clamp to [-2048,2047]
  data = ((2 * coeff + SIGN(coeff)) * inter_matrix[i] * quant) / 16
*/

void dequant4_inter_c(int16_t *data, const int16_t *coeff, const uint32_t quant)
{
    uint32_t sum = 0;
    uint32_t i;
	int16_t *inter_matrix;

	inter_matrix = get_inter_matrix();
    
    for (i = 0; i < 64; i++)
    {
        if (coeff[i] == 0)
        {
            data[i] = 0;
        }
        else if (coeff[i] < 0)
        {
            int32_t level = -coeff[i];
            level = ((2 * level + 1) * inter_matrix[i] * quant) >> 4;
            data[i] = (level <= 2048 ? -level : -2048);
        } 
        else // if (coeff[i] > 0)
        {
            uint32_t level = coeff[i];
            level = ((2 * level + 1) * inter_matrix[i] * quant) >> 4;
            data[i] = (level <= 2047 ? level : 2047);
        }

        sum ^= data[i];
    }

    // mismatch control

    if ((sum & 1) == 0)
    {
        data[63] ^= 1;
    }
}
