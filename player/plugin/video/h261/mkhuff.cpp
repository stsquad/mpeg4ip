/*
 * Copyright (c) 1993-1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Network Research
 *	Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock.h>
extern "C" {
int getopt(int, char * const *, const char *);
}
#endif
#define HUFFSTRINGS
#include "p64-huff.h"

/*
 * Convert a binary string to an integer.
 */
int
btoi(const char* s)
{
	int v = 0;
	while (*s) {
		v <<= 1;
		v |= *s++ - '0';
	}
	return (v);
}

struct hufftab {
	int maxlen;
	short *prefix;
};

/*
 * Build a direct map huffman table from the array of codes given.
 * We build a prefix table such that we can directly index
 * table with the k-bit number at the head of the bit stream, where
 * k is the max-length code in the table.  The table tells us
 * the value and the actual length of the code we have.
 * We need the length so we can tear that many bits off
 * the input stream.  The length and value are packed as
 * two 16-bit quantities in a 32-bit word.  The value is stored
 * in the upper 16-bits, so when we right-shift it over,
 * it is automatically sign-extended.
 */
void
huffbuild(hufftab& ht, const huffcode* codes)
{
	int i = 0;
	int maxlen = 0;
	for (; codes[i].str != 0; ++i) {
		int v = strlen(codes[i].str);
		if (v > maxlen)
			maxlen = v;
	}
	int size = 1 << maxlen;
	if (size > 65536)
		abort();
		
	/*
	 * Build the direct-map lookup table.
	 */
	ht.prefix = new short[size];
	ht.maxlen = maxlen;

	/*
	 * Initialize states to illegal, and arrange
	 * for max bits to be stripped off the input.
	 */
	for (i = 0; i < size; ++i)
		ht.prefix[i] = (SYM_ILLEGAL << 5) | maxlen;

	for (i = 0; codes[i].str != 0; ++i) {
		int codelen = strlen(codes[i].str);
		int nbit = maxlen - codelen;
		int code = btoi(codes[i].str) << nbit;
		int map = (codes[i].val << 5) | codelen;
		/*
		 * The low nbit bits are don't cares.
		 * Spin through all possible combos.
		 */
		for (int n = 1 << nbit; --n >= 0; ) {
			if ((code | n) >= size)
				abort();
			ht.prefix[code | n] = map;
		}
	}
}

int
skipcode(int bs, int* code, int* len, int* symbol, int n)
{
	int nbit = 0;
	for (;;) {
		/*
		 * Find the matching huffman code that is a prefix
		 * of bs at offset off.  There is either zero
		 * or one such codes, since the huffman strings have
		 * unique prefixes.  The zero case means the given
		 * bit string is impossible.
		 */
		int pbit = nbit;
		for (int k = 0; k < n; ++k) {
			if (len[k] < 16 - nbit) {
				int v = bs >> (16 - nbit - len[k]);
				v &= (1 << len[k]) - 1;
				if (v != code[k])
					continue;
				nbit += len[k];
				if (symbol[k] == SYM_ESCAPE)
					/*
					 * This must end the prefix
					 * since it necessarily takes
					 * up more than 16 bits.
					 */
					return (nbit + 14);
				else if (symbol[k] == SYM_EOB)
					return (0x80 | nbit);
			}
		}
		if (nbit == 0)
			/*
			 * Didn't find any matches.
			 */
			return (0x40);
		/*
		 * If we didn't find a new match,
		 * return the current result.
		 */
		if (nbit == pbit)
			return (nbit);
	}
}
		
/*
 * Build a skip table.  The idea is to have a fast way
 * of finding the boundaries in the bit stream for a block
 * (i.e., entropy/run-length encoded set of 8x8 DCT coefficients).
 * The bit string can then be used to hash into a cache of
 * recently computed inverse DCTs.
 *
 * The table is indexed 16 bits at a time.  Each 8-bit entry
 * tells how many bits are taken up by the longest string of
 * whole codewords in the index.  The length might be greater
 * than 16 if there is an ESCAPE character (which is great
 * because we can just skip over the following 14-bits).
 * Bit 7 is set if the codes are terminated by EOB,; bit 6
 * is set if the string is impossible; the length is in bits 0-5.
 *
 * Note that this table works only for the AC coeffcients, because
 * the DC decoding is complicated by macroblock type context.
 * That's okay because we want the hash table to contain inverse
 * DCTs with 0 DC bias because adding in the DC component during
 * the block copy incurs no additional cost (memory is the bottleneck),
 * and using DC=0 significantly increases the probability of a
 * cache hit.
 */
void
skipbuild(huffcode* hc, u_char* skiptab)
{
	int len[1024];
	int code[1024];
	int symbol[1024];

	int n = 0;
	for (; hc->str != 0; ++hc) {
		len[n] = strlen(hc->str);
		code[n] = btoi(hc->str);
		symbol[n] = hc->val;
		++n;
	}
	for (int bs = 0; bs < 65536; ++bs)
		skiptab[bs] = skipcode(bs, code, len, symbol, n);
}

struct huff {
	char* name;
	huffcode* codes;
};
struct huff hc[] = {
	{ "htd_mtype", hc_mtype },
	{ "htd_mba", hc_mba },
	{ "htd_cbp", hc_cbp },
	{ "htd_dvm", hc_dvm },
	{ "htd_tcoeff", hc_tcoeff },
	{ 0, 0 }
};

huffcode*
mba_lookup(int mba)
{
	for (huffcode* hc = hc_mba; hc->str != 0; ++hc)
		if (hc->val == mba)
			return (hc);
	abort();
	return (0);  /*NOTREACHED*/
}

void
gen_hte_mba()
{
	printf("struct huffent hte_mba[33] = {\n");
	for (int mba = 0; mba < 33; ++mba) {
		huffcode* hc = mba_lookup(mba + 1);
		printf("\t{ %d, %d },\n", btoi(hc->str), strlen(hc->str));
	}
	printf("};\n");
}

huffcode*
tc_lookup(int run, int level)
{
	int v = TC_RUN(run) | TC_LEVEL(level);
	if (v <= 0)
		return (0);
	for (huffcode* hc = hc_tcoeff; hc->str != 0; ++hc)
		if (hc->val == v)
			return (hc);
	return (0);
}

void
gen_hte_tc()
{
	/* 5 bits of (signed) level, 6 bits of (unsigned) run */
	printf("struct huffent hte_tc[1 << 11] = {\n");
	for (int level = -16; level <= 15; ++level) {
		for (int run = 0; run < 64; ++run) {
			huffcode* hc = tc_lookup(run, (level + 16) & 0x1f);
			if (run < 32 &&
			    (hc = tc_lookup(run, (level + 16) & 0x1f)) != 0)
				printf("\t{ %d, %d },\n",
				       btoi(hc->str), strlen(hc->str));
			else
				printf("\t{ 0, 0 },\n");

		}
	}
	printf("};\n");
}

void
usage()
{
	printf("usage: mkhuff [-es]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int sflag = 0;
	int eflag = 0;

#ifdef notyet
	extern char *optarg;
	extern int optind, opterr;
#endif

	int op;
	while ((op = getopt(argc, argv, "es")) != -1) {
		switch (op) {
		case 's':
			++sflag;
			break;
		case 'e':
			++eflag;
			break;
		default:
			usage();
			break;
		}
	}
	printf("#include \"p64-huff.h\"\n");
	huff* p = hc;
	while (p->name) {
		hufftab ht;
		huffbuild(ht, p->codes);
		printf("const int %s_width = %d;\n", p->name, ht.maxlen);
		printf("const short %s[] = {", p->name);
		int n = 1 << ht.maxlen;
		for (int i = 0; i < n; ++i)
			printf("%s0x%04x,", (i & 7) ? " " : "\n\t",
			       ht.prefix[i] & 0xffff);
		printf("\n};\n");
		++p;
	}
	if (eflag) {
		gen_hte_mba();
		gen_hte_tc();
	}
	if (sflag) {
		u_char skiptab[65536];
		skipbuild(hc_tcoeff, skiptab);
		printf("const unsigned char skiptab[] = {");
		for (int i = 0; i < 65536; ++i)
			printf("%s0x%02x,", (i & 7) ? " " : "\n\t",
			       skiptab[i]);
		printf("\n};\n");
	}
	
	return (0);
}
