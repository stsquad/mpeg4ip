#include <stdlib.h>
#include <stdio.h>
#include "mem_align.h"

void *xvid_malloc(size_t size, uint8_t alignment)
{
	uint8_t *mem_ptr;
  
	if(!alignment)
	{

		/* We have not to satisfy any alignment */
		if((mem_ptr = (uint8_t *) malloc(size + 1)) != NULL)
		{

			/* Store (mem_ptr - "real allocated memory") in *(mem_ptr-1) */
			*mem_ptr = 0;

			/* Return the mem_ptr pointer */
			return (void *) mem_ptr++;

		}

	}
	else
	{
		uint8_t *tmp;
	
		/*
		 * Allocate the required size memory + alignment so we
		 * can realign the data if necessary
		 */

		if((tmp = (uint8_t *) malloc(size + alignment)) != NULL)
		{

			/* Align the tmp pointer */
			mem_ptr = (uint8_t *)((uint32_t)(tmp + alignment - 1)&(~(uint32_t)(alignment - 1)));

			/*
			 * Special case where malloc have already satisfied the alignment
			 * We must add alignment to mem_ptr because we must store
			 * (mem_ptr - tmp) in *(mem_ptr-1)
			 * If we do not add alignment to mem_ptr then *(mem_ptr-1) points
			 * to a forbidden memory space
			 */
			if(mem_ptr == tmp) mem_ptr += alignment;

			/*
			 * (mem_ptr - tmp) is stored in *(mem_ptr-1) so we are able to retrieve
			 * the real malloc block allocated and free it in xvid_free
			 */
			*(mem_ptr - 1) = (uint8_t)(mem_ptr - tmp);

			/* Return the aligned pointer */
			return (void *) mem_ptr;

		}
	}

	return NULL;

}

void xvid_free(void *mem_ptr)
{

	/* *(mem_ptr - 1) give us the offset to the real malloc block */
	free((uint8_t*)mem_ptr - *((uint8_t*)mem_ptr - 1));

}
