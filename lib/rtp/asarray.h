/*
 * FILE:    asarray.h
 * AUTHORS: Orion Hodson 
 *
 * Associative array for strings.  Perloined from RAT settings code.
 *
 * Copyright (c) 1999-2000 University College London
 * All rights reserved.
 *
 * $Id: asarray.h,v 1.1 2001/08/01 00:34:01 wmaycisco Exp $
 */

#ifndef __AS_ARRAY_H
#define __AS_ARRAY_H

typedef struct _asarray asarray;

#if defined(__cplusplus)
extern "C" {
#endif

/* Associative array for strings only.  Makes own internal copies of
 * keys and values.
 *
 * Functions that return use TRUE for success and FALSE for failure.
 * Double pointers in arguments are filled in by the function being 
 * called.
 */

int32_t     asarray_create  (asarray **ppa);
void        asarray_destroy (asarray **ppa);

int32_t     asarray_add    (asarray *pa, const char *key, const char *value);
void        asarray_remove (asarray *pa, const char *key);
int32_t     asarray_lookup (asarray *pa, const char *key, char **value);

/* asarray_get_key - gets key corresponding to index'th entry in
 * internal representation, has not relation to order <key,value>
 * tuples added in.  This function exists to provide easy way to drain
 * array one item at a time. 
 */
const char* asarray_get_key_no(asarray *pa, int32_t index);

#if defined(__cplusplus)
}
#endif

#endif /* __AS_ARRAY_H */
