/*
 * FILE:    asarray.c
 *
 * AUTHORS: Orion Hodson
 *
 * Copyright (c) 1999-2000 University College London
 * All rights reserved.
 */
 
#include "config_unix.h"
#include "config_win32.h"

#include "debug.h"
#include "memory.h"
#include "util.h"

#include "asarray.h"

typedef struct s_hash_tuple {
        uint32_t hash;
        char *key;
        char *value;
        struct s_hash_tuple *next;
} hash_tuple;

#define ASARRAY_SIZE 11

struct _asarray {
        hash_tuple *table[ASARRAY_SIZE];
        int32_t     nitems[ASARRAY_SIZE];
};

static uint32_t 
asarray_hash(const char *key)
{
        uint32_t hash = 0;

        while(*key != '\0') {
                hash = hash * 31;
                hash += ((uint32_t)*key) + 1;
                key++;
        }

        return hash;
}

int32_t
asarray_add(asarray *pa, const char *key, const char *value)
{
        hash_tuple *t;
        int row;

        t = (hash_tuple*)xmalloc(sizeof(hash_tuple));
        if (t) {
                /* transfer values */
                t->hash  = asarray_hash(key);
                t->key   = xstrdup(key);
                t->value = xstrdup(value);
                /* Add to table */
                row            = t->hash % ASARRAY_SIZE;
                t->next        = pa->table[row];
                pa->table[row] = t;
                pa->nitems[row]++;
                return TRUE;
        }
        return FALSE;
}

void
asarray_remove(asarray *pa, const char *key)
{
        hash_tuple **t, *e;
        uint32_t hash;
        int row;

        hash = asarray_hash(key);
        row  = hash % ASARRAY_SIZE;
        t    = &pa->table[row];
        while((*t) != NULL) {
                if ((hash == (*t)->hash) && 
                    (strcmp(key, (*t)->key) == 0)) {
                        e = *t;
                        *t = e->next;
                        xfree(e->key);
                        xfree(e->value);
                        xfree(e);
                        pa->nitems[row]--;
                        ASSERT(pa->nitems[row] >= 0);
                        break;
                } else {
                        t = &(*t)->next;
                }
        }
}

const char* 
asarray_get_key_no(asarray *pa, int32_t index)
{
        int32_t row = 0;

        index += 1;
        while (row < ASARRAY_SIZE && index > pa->nitems[row]) {
                index -= pa->nitems[row];
                row++;
        }

        if (row < ASARRAY_SIZE) {
                hash_tuple *t;
                t = pa->table[row];
                while(--index > 0) {
                        ASSERT(t->next != NULL);
                        t = t->next;
                }
                return t->key;
        }
        return NULL;
}

/* asarray_lookup points value at actual value        */
/* and return TRUE if key found.                      */
int32_t
asarray_lookup(asarray *pa, const char *key, char **value)
{
        hash_tuple *t;
        int          row;
        uint32_t     hash;

        hash = asarray_hash(key);
        row  = hash % ASARRAY_SIZE;

        t = pa->table[row];
        while(t != NULL) {
                if (t->hash == hash && strcmp(key, t->key) == 0) {
                        *value = t->value;
                        return TRUE;
                }
                t = t->next;
        }
        *value = NULL;
        return FALSE;
}

int32_t
asarray_create(asarray **ppa)
{
        asarray *pa;
        pa = (asarray*)xmalloc(sizeof(asarray));
        if (pa != NULL) {
                memset(pa, 0, sizeof(asarray));
                *ppa = pa;
                return TRUE;
        }
        return FALSE;
}

void
asarray_destroy(asarray **ppa)
{
        asarray    *pa;
        const char *key;

        pa = *ppa;
        ASSERT(pa != NULL);

        while ((key = asarray_get_key_no(pa, 0)) != NULL) {
                asarray_remove(pa, key);
        }

        xfree(pa);
        *ppa = NULL;
        xmemchk();
}
