/*
 * FILE:    memory.c
 * AUTHORS:  Isidor Kouvelas / Colin Perkins / Mark Handley / Orion Hodson
 *
 * $Revision: 1.5 $
 * $Date: 2004/10/28 22:44:17 $
 *
 * Copyright (c) 1995-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "util.h"

#ifdef DEBUG_MEM

/* Custom memory routines are here, down to #else, defaults follow */

#define MAX_ADDRS         65536
#define MAGIC_MEMORY      0xdeadbeef
#define MAGIC_MEMORY_SIZE 4

/* Allocated block format is:
 * <chk_header> <memory chunk...> <trailing magic number>
 */

typedef struct {
        uint32_t key;   /* Original allocation number   */
        uint32_t size;  /* Size of allocation requested */
        uint32_t pad;   /* Alignment padding to 8 bytes */
        uint32_t magic; /* Magic number                 */
} chk_header;

typedef struct s_alloc_blk {
        uint32_t    key;     /* Key in table (ascending) */
        chk_header *addr;
        char       *filen;   /* file where allocated     */
        int         line;    /* line where allocated     */
        size_t      length;  /* size of allocation       */
        int         blen;    /* size passed to block_alloc (if relevent) */
        int         est;     /* time last touched in order of all allocation and reclaims */
} alloc_blk;

/* Table is ordered by key */
static alloc_blk mem_item[MAX_ADDRS];

static int   naddr = 0; /* number of allocations */
static int   tick  = 1; /* xmemchk assumes this is one, do not change without checking why */

static int   init  = 0;

/**
 * xdoneinit:
 * @void: 
 * 
 * Marks end of an applications initialization period.  For media
 * applications with real-time data transfer it's sometimes helpful to
 * distinguish between memory allocated during application
 * initialization and when application is running.
 **/

void xdoneinit(void) 
{
	init = tick++;
}

extern int chk_header_okay(const chk_header *ch);
int chk_header_okay(const chk_header *ch)
{
        const uint8_t *tm; /* tail magic */
        ASSERT(ch != NULL);

        if (ch->key == MAGIC_MEMORY) {
                fprintf(stderr, "ERROR: freed unit being checked\n");
                abort();
        }

        tm = (const uint8_t*)ch;
        tm += sizeof(chk_header) + ch->size;

        if (ch->magic != MAGIC_MEMORY) {
                fprintf(stderr, "ERROR: memory underrun\n");
                abort();
                return FALSE;
        } else if (memcmp(tm, &ch->magic, MAGIC_MEMORY_SIZE)) {
                fprintf(stderr, "ERROR: memory overrun\n");
                abort();
                return FALSE;
        }

        return TRUE;
}

static int mem_key_cmp(const void *a, const void *b) {
        const alloc_blk *g, *h;
        g = (const alloc_blk*)a;
        h = (const alloc_blk*)b;
        if (g->key < h->key) {
                return -1;
        } else if (g->key > h->key) {
                return +1;
        }
        return 0;
}

static alloc_blk *mem_item_find(uint32_t key) {
        void      *p;
        alloc_blk  t;
        t.key = key;
        p = bsearch((const void*)&t, mem_item, naddr, sizeof(alloc_blk), mem_key_cmp);
        return (alloc_blk*)p;
}

/**
 * xmemchk:
 * @void: 
 * 
 * Check for bounds overruns in all memory allocated with xmalloc(),
 * xrealloc(), and xstrdup().  Information on corrupted blocks is
 * rendered on the standard error stream.  This includes where the
 * block was allocated, the size of the block, and the number of
 * allocations made since the block was created.
 **/
void xmemchk(void)
{
        uint32_t    last_key;
        chk_header *ch;
        int         i;

	if (naddr > MAX_ADDRS) {
		fprintf(stderr, "ERROR: Too many addresses for xmemchk()!\n");
		abort();
	}

        last_key = 0;
	for (i = 0; i < naddr; i++) {
                /* Check for table corruption */
                if (mem_item[i].key  < last_key) {
                        fprintf(stderr, "Memory table keys out of order - fatal error");
                        abort();
                }
                last_key = mem_item[i].key;
                if (mem_item[i].addr == NULL) {
                        fprintf(stderr, "Memory table entry reference null block - fatal error");
                        abort();
                }
                if (mem_item[i].filen == NULL) {
                        fprintf(stderr, "Memory table filename missing - fatal error");
                        abort();
                }
                if ((size_t) strlen(mem_item[i].filen) != mem_item[i].length) {
                        fprintf(stderr, "Memory table filename length corrupted - fatal error");
                        abort();
                }
                
                /* Check memory */
                ch = mem_item[i].addr;
                if (chk_header_okay(ch) == FALSE) {
                        /* Chk header display which side has gone awry */
			fprintf(stderr, "Memory check failed!\n");
			fprintf(stderr, "addr: %p", mem_item[i].addr);
                        fprintf(stderr, "  size: %6d", ch->size);
                        fprintf(stderr, "  age: %6d", tick - mem_item[i].est);
                        fprintf(stderr, "  file: %s", mem_item[i].filen);
                        fprintf(stderr, "  line: %d", mem_item[i].line);
                        fprintf(stderr, "\n");
                        abort();
                }
        }
}

static int 
alloc_blk_cmp_origin(const void *vab1, const void *vab2)
{
        const alloc_blk *ab1, *ab2;
        int sc;
        
        ab1 = (const alloc_blk*)vab1;
        ab2 = (const alloc_blk*)vab2;

        if (ab1->filen == NULL || ab2->filen == NULL) {
                if (ab1->filen == NULL && ab2->filen == NULL) {
                        return 0;
                } else if (ab1->filen == NULL) {
                        return +1;
                } else /* (ab2->filen == NULL)*/ {
                        return -1;
                }
        }

        sc = strcmp(ab1->filen, ab2->filen);
        if (sc == 0) {
                if (ab1->line > ab2->line) {
                        return +1;
                } else if (ab1->line == ab2->line) {
                        return 0;
                } else /* (ab1->line < ab2->line) */{
                        return -1;
                }
        }

        return sc;
}

static int
alloc_blk_cmp_est(const void *vab1, const void *vab2)
{
        const alloc_blk *ab1, *ab2;

        ab1 = (const alloc_blk*)vab1;        
        ab2 = (const alloc_blk*)vab2;
        
        if (ab1->est > ab2->est) {
                return +1;
        } else if (ab1->est == ab2->est) {
                return 0;
        } else {
                return -1;
        }
}

/**
 * xmemdmp:
 * @void: 
 * 
 * Dumps the address, size, age, and point of allocation in code.
 *
 **/
void 
xmemdmp(void)
{
	int i;
        block_release_all();
	if (naddr > MAX_ADDRS) {
		printf("ERROR: Too many addresses for xmemdmp()!\n");
		abort();
	}

        qsort(mem_item, naddr, sizeof(mem_item[0]), alloc_blk_cmp_est);

        for (i=0; i<naddr; i++) {
                printf("%5d",i);                               fflush(stdout);
                printf("  addr: %p", mem_item[i].addr);        fflush(stdout);
                printf("  size: %5d", mem_item[i].addr->size); fflush(stdout);
                printf("  age: %6d", tick - mem_item[i].est);  fflush(stdout);
                printf("  file: %s", mem_item[i].filen);       fflush(stdout);
                printf(":%d", mem_item[i].line);               fflush(stdout);
                if (mem_item[i].blen != 0) { 
                        printf("  \tblen %d", mem_item[i].blen);   
                        fflush(stdout);
                }
                printf("\n");
        }
	printf("Program initialisation finished at age %6d\n", tick-init);
        qsort(mem_item, naddr, sizeof(mem_item[0]), mem_key_cmp);
}


/* Because block_alloc recycles blocks we need to know which code
 * fragment takes over responsibility for the memory.  */

/**
 * xclaim:
 * @addr: address
 * @filen: new filename
 * @line: new allocation line
 * 
 * Coerces information in allocation table about allocation file and
 * line to be @filen and @line.  This is used by the evil
 * block_alloc() and should probably not be used anywhere else ever.
 **/
void 
xclaim(void *addr, const char *filen, int line)
{
        alloc_blk *m;
        chk_header *ch;
        
        ch = ((chk_header *)addr) - 1;
        m  = mem_item_find(ch->key); 

        if (chk_header_okay(ch) == FALSE) {
                fprintf(stderr, "xclaim: memory corrupted\n");
                abort();
        }

        if (m == NULL) {
                fprintf(stderr, "xclaim: block not found\n");
                abort();
        }

        free(m->filen);
        m->filen  = strdup(filen);
        m->length = strlen(filen);
        m->line   = line;
        m->est    = tick++;
}

/**
 * xmemdist:
 * @fp: file pointer
 * 
 * Dumps information on existing memory allocations to file.
 **/
void 
xmemdist(FILE *fp)
{
        int i, last_line=-1, cnt=0, entry=0;
        char *last_filen = NULL;

        /* This is painful...I don't actually envisage running beyond this dump :-)
         * 2 sorts are needed, one into order by file, and then back to key order.
         */
        qsort(mem_item, naddr, sizeof(mem_item[0]), alloc_blk_cmp_origin);
        fprintf(fp, "# Distribution of memory allocated\n# <idx> <file> <line> <allocations>\n");
        for(i = 0; i < naddr; i++) {
                if (last_filen == NULL ||
                    last_line  != mem_item[i].line ||
                    strcmp(last_filen, mem_item[i].filen)) {
                        if (last_filen != NULL) {
                                fprintf(fp, "% 3d\n", cnt);
                        }
                        cnt = 0;
                        last_filen = mem_item[i].filen;
                        last_line  = mem_item[i].line;
                        fprintf(fp, "% 3d %20s % 4d ", entry, last_filen, last_line);
                        entry++;
                }
                cnt++;
        }
        fprintf(fp, "% 3d\n", cnt);
        /* Restore order */
        qsort(mem_item, naddr, sizeof(mem_item[0]), mem_key_cmp);
}

/**
 * xfree:
 * @p: pointer to block to freed.
 * 
 * Free block of memory.  Semantically equivalent to free(), but
 * checks for bounds overruns in @p and tidies up state associated
 * additional functionality.
 *
 * Must be used to free memory allocated with xmalloc(), xrealloc(),
 * and xstrdup().
 **/
void 
xfree(void *p)
{
        alloc_blk  *m;
        chk_header *ch;
        uint32_t size, delta, magic, idx;
        
	if (p == NULL) {
		printf("ERROR: Attempt to free NULL pointer!\n");
		abort();
	}
        ch = ((chk_header*)p) - 1;

        printf("free at %p\n", ch);
        /* Validate entry  */
        if (chk_header_okay(ch) == FALSE) {
                printf("ERROR: Freeing corrupted block\n");
                abort();
        }

        /* Locate in table */
        m = mem_item_find(ch->key);
        if (m == NULL) {
                printf("ERROR: Freeing unallocated or already free'd block\n");
                abort();
        }

        /* Trash memory of allocated block, maybe noticed by apps when    
         * deref'ing free'd */
        size  = ch->size + sizeof(chk_header) + MAGIC_MEMORY_SIZE;
        magic = MAGIC_MEMORY;
        p     = (uint8_t*)ch;
        while (size > 0) {
                delta = min(size, 4);
                memcpy(p, &magic, delta);
                (uint8_t*)p += delta;
                size        -= delta;
        }

        /* Free memory     */
        free(ch);
        free(m->filen);

        /* Remove from table */
        idx = m - mem_item;
        if (naddr - idx > 0) {
                memmove(&mem_item[idx], 
                        &mem_item[idx + 1], 
                        (naddr - idx - 1) * sizeof(alloc_blk));
        }
        naddr--;
        xmemchk();
}

void *
_xmalloc(unsigned size, const char *filen, int line)
{
        uint32_t   *data;
        void       *p;
        chk_header *ch;

        p = (void*) malloc (size + sizeof(chk_header) + MAGIC_MEMORY_SIZE);

	ASSERT(p     != NULL);
	ASSERT(filen != NULL);

        /* Fix block header */
        ch = (chk_header*)p;
        ch->key   = tick++;
        ch->size  = size;
        ch->magic = MAGIC_MEMORY;

	data = (uint32_t*)((chk_header *)p + 1);
#if 0
	memset((void*)data, 0xf0, size);
#else
	memset((void*)data, 0, size);
#endif
        
        /* Fix block tail */
        memcpy(((uint8_t*)data) + size, &ch->magic, MAGIC_MEMORY_SIZE);

        /* Check set up okay */
        if (chk_header_okay(ch) == FALSE) {
                fprintf(stderr, "Implementation Error\n");
                abort();
        }

        /* Add table entry */
        mem_item[naddr].key    = ch->key;
	mem_item[naddr].addr   = p;
	mem_item[naddr].filen  = (char *)strdup(filen);
	mem_item[naddr].line   = line;
	mem_item[naddr].length = strlen(filen);
        mem_item[naddr].blen   = 0;        /* block_alloc'ed len when appropriate     */
        mem_item[naddr].est    = ch->key;  /* changes when block_alloc recycles block */
        naddr ++;

	if (naddr >= MAX_ADDRS) {
		fprintf(stderr, "ERROR: Allocated too much! Table overflow in xmalloc()\n");
		fprintf(stderr, "       Do you really need to allocate more than %d items?\n", MAX_ADDRS);
		abort();
	}
        
        if (chk_header_okay(ch) == FALSE) {
                fprintf(stderr, "Implementation Error\n");
                abort();
        }

        printf("malloc at %p\n", p);
        return (uint8_t*)p + sizeof(chk_header);
}

void *
_xrealloc(void *p, unsigned size, const char *filen, int line)
{
        alloc_blk  *m;
        chk_header *ch;
        uint8_t    *t;
       
        ASSERT(p     != NULL);
        ASSERT(filen != NULL);
	
        ch = ((chk_header*) p) - 1;
        m  = mem_item_find(ch->key);
        if (m != NULL) {
                /* Attempt reallocation */
                m->addr = realloc((void*)ch, size + sizeof(chk_header) + MAGIC_MEMORY_SIZE);
                if (m->addr == NULL) {
                        debug_msg("realloc failed\n");
                        return NULL;
                }
		ch = (chk_header*)m->addr;
		p = (void *)(ch + 1);
                /* Update table */
                free(m->filen);
                m->filen  = (char *) strdup(filen);
                m->line   = line;
                m->length = strlen(filen);
                m->est    = tick++;
                /* Fix chunk header */
                ch->size  = size;
                /* Fix trailer */
                t = (uint8_t*)p + size;
                memcpy(t, &ch->magic, MAGIC_MEMORY_SIZE);
                /* Check block is okay */
                if (chk_header_okay(ch) == FALSE) {
                        fprintf(stderr, "Implementation Error\n");
                        abort();
                }
                return p;
	}
	debug_msg("Trying to xrealloc() memory not which is not allocated\n");
	abort();
        return 0;
}

char *_xstrdup(const char *s1, const char *filen, int line)
{
	char 	*s2;
  
	s2 = (char *) _xmalloc(strlen(s1)+1, filen, line);
	if (s2 != NULL) {
		strcpy(s2, s1);
        }
	return (s2);
}
#else

void xdoneinit (void) { return; }
void xmemchk   (void) { return; }
void xmemdmp   (void) { return; }

void xclaim    (void *p, const char *filen, int line)
{
        UNUSED(p);
        UNUSED(filen);
        UNUSED(line);
        return;
}

void xmemdist (FILE *f) { UNUSED(f); }

void xfree    (void *x) { free(x); }

void *_xmalloc(unsigned int size, const char *filen, int line) {
	void	*m;

	m = malloc(size);

#ifdef DEBUG
	/* This is just to check initialization errors in allocated structs */
	memset(m, 0xf0, size);
#endif
	UNUSED(filen);
	UNUSED(line);

	return m;
}

void *_xrealloc(void *p, unsigned int size,const char *filen,int line) {
        UNUSED(filen);
        UNUSED(line);
        return realloc(p, size);
}
char *_xstrdup(const char *s1, const char *filen, int line) {
        UNUSED(filen);
        UNUSED(line);
        return strdup(s1);
}

#endif /* DEBUG_MEM */
