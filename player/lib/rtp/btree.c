/*
 * FILE:     btree.c
 * PROGRAM:  RAT
 * AUTHOR:   O.Hodson
 * MODIFIED: C.Perkins
 * 
 * Binary tree implementation - Mostly verbatim from:
 *
 * Introduction to Algorithms by Corman, Leisserson, and Rivest,
 * MIT Press / McGraw Hill, 1990.
 *
 */

#include "config_unix.h"
#include "config_win32.h"
#include "debug.h"
#include "memory.h"
#include "btree.h"

typedef struct s_btree_node {
        uint32_t       		 key;
        void         		*data;
        struct s_btree_node 	*parent;
        struct s_btree_node 	*left;
        struct s_btree_node 	*right;
	uint32_t		 magic;
} btree_node_t;

struct s_btree {
        btree_node_t   *root;
	uint32_t	magic;
	int		count;
};

/*****************************************************************************/
/* Debugging functions...                                                    */
/*****************************************************************************/

#define BTREE_MAGIC      0x10101010
#define BTREE_NODE_MAGIC 0x01010101

static int btree_count;

static void
btree_validate_node(btree_node_t *node, btree_node_t *parent)
{
	assert(node->magic  == BTREE_NODE_MAGIC);
	assert(node->parent == parent);
	btree_count++;
	if (node->left != NULL) {
		btree_validate_node(node->left, node);
	}
	if (node->right != NULL) {
		btree_validate_node(node->right, node);
	}
}

static void
btree_validate(btree_t *t)
{
	assert(t->magic == BTREE_MAGIC);
#ifdef DEBUG
	btree_count = 0;
	if (t->root != NULL) {
		btree_validate_node(t->root, NULL);
	}
	assert(btree_count == t->count);
#endif
}

/*****************************************************************************/
/* Utility functions                                                         */
/*****************************************************************************/

static btree_node_t*
btree_min(btree_node_t *x)
{
        if (x == NULL) {
                return NULL;
        }
        while(x->left) {
                x = x->left;
        }
        return x;
}

static btree_node_t*
btree_max(btree_node_t *x)
{
        if (x == NULL) {
                return NULL;
        }
        while(x->right) {
                x = x->right;
        }
        return x;
}

static btree_node_t*
btree_successor(btree_node_t *x)
{
        btree_node_t *y;

        if (x->right != NULL) {
                return btree_min(x->right);
        }

        y = x->parent;
        while (y != NULL && x == y->right) {
                x = y;
                y = y->parent;
        }

        return y;
}

static btree_node_t*
btree_search(btree_node_t *x, uint32_t key)
{
        while (x != NULL && key != x->key) {
                if (key < x->key) {
                        x = x->left;
                } else {
                        x = x->right;
                }
        }
        return x; 
}

static void
btree_insert_node(btree_t *tree, btree_node_t *z) {
        btree_node_t *x, *y;

	btree_validate(tree);
        y = NULL;
        x = tree->root;
        while (x != NULL) {
                y = x;
                assert(z->key != x->key);
                if (z->key < x->key) {
                        x = x->left;
                } else {
                        x = x->right;
                }
        }

        z->parent = y;
        if (y == NULL) {
                tree->root = z;
        } else if (z->key < y->key) {
                y->left = z;
        } else {
                y->right = z;
        }
	tree->count++;
	btree_validate(tree);
}

static btree_node_t*
btree_delete_node(btree_t *tree, btree_node_t *z)
{
        btree_node_t *x, *y;

	btree_validate(tree);
        if (z->left == NULL || z->right == NULL) {
                y = z;
        } else {
                y = btree_successor(z);
        }

        if (y->left != NULL) {
                x = y->left;
        } else {
                x = y->right;
        }

        if (x != NULL) {
                x->parent = y->parent;
        }

        if (y->parent == NULL) {
                tree->root = x;
        } else if (y == y->parent->left) {
                y->parent->left = x;
        } else {
                y->parent->right = x;
        }

        z->key  = y->key;
        z->data = y->data;

	tree->count--;

	btree_validate(tree);
        return y;
}

/*****************************************************************************/
/* Exported functions                                                        */
/*****************************************************************************/

int
btree_create(btree_t **tree)
{
        btree_t *t = (btree_t*)xmalloc(sizeof(btree_t));
        if (t) {
		t->count = 0;
		t->magic = BTREE_MAGIC;
                t->root  = NULL;
                *tree = t;
                return TRUE;
        }
        return FALSE;
}

int
btree_destroy(btree_t **tree)
{
        btree_t *t = *tree;

	btree_validate(t);
        if (t->root != NULL) {
                debug_msg("Tree not empty - cannot destroy\n");
                return FALSE;
        }

        xfree(t);
        *tree = NULL;
        return TRUE;
}

int
btree_find(btree_t *tree, uint32_t key, void **d)
{
        btree_node_t *x;

	btree_validate(tree);
        x = btree_search(tree->root, key);
        if (x != NULL) {
                *d = x->data;
                return TRUE;
        }
        return FALSE;
}

int 
btree_add(btree_t *tree, uint32_t key, void *data)
{
        btree_node_t *x;

	btree_validate(tree);
        x = btree_search(tree->root, key);
        if (x != NULL) {
                debug_msg("Item already exists - key %ul\n", key);
                return FALSE;
        }

        x = (btree_node_t *)xmalloc(sizeof(btree_node_t));
        x->key    = key;
        x->data   = data;
        x->parent = NULL;
	x->left   = NULL;
	x->right  = NULL;
	x->magic  = BTREE_NODE_MAGIC;
        btree_insert_node(tree, x);

        return TRUE;
}

int
btree_remove(btree_t *tree, uint32_t key, void **data)
{
        btree_node_t *x;

	btree_validate(tree);
        x = btree_search(tree->root, key);
        if (x == NULL) {
                debug_msg("Item not on tree - key %ul\n", key);
                *data = NULL;
                return FALSE;
        }

        /* Note value that gets freed is not necessarily the the same
         * as node that gets removed from tree since there is an
         * optimization to avoid pointer updates in tree which means
         * sometimes we just copy key and data from one node to
         * another.  
         */

        *data = x->data;
        x = btree_delete_node(tree, x);
        xfree(x);

        return TRUE;
}

int 
btree_get_min_key(btree_t *tree, uint32_t *key)
{
        btree_node_t *x;

	btree_validate(tree);
        if (tree->root == NULL) {
                return FALSE;
        }

        x = btree_min(tree->root);
        if (x == NULL) {
                return FALSE;
        }
        
        *key = x->key;
        return TRUE;
}

int 
btree_get_max_key(btree_t *tree, uint32_t *key)
{
        btree_node_t *x;

	btree_validate(tree);
        if (tree->root == NULL) {
                return FALSE;
        }

        x = btree_max(tree->root);
        if (x == NULL) {
                return FALSE;
        }
        
        *key = x->key;
        return TRUE;
}

int
btree_get_next_key(btree_t *tree, uint32_t cur_key, uint32_t *next_key)
{
        btree_node_t *x;

	btree_validate(tree);
        x = btree_search(tree->root, cur_key);
        if (x == NULL) {
                return FALSE;
        }
        
        x = btree_successor(x);
        if (x == NULL) {
                return FALSE;
        }
        
        *next_key = x->key;
        return TRUE;
}

/*****************************************************************************/
/* Test code                                                                 */
/*****************************************************************************/

#ifdef TEST_BTREE

static int
btree_depth(btree_node_t *x)
{
        int l, r;

        if (x == NULL) {
                return 0;
        }

        l = btree_depth(x->left);
        r = btree_depth(x->right);

        if (l > r) {
                return l + 1;
        } else {
                return r + 1;
        }
}

#include <curses.h>

static void
btree_dump_node(btree_node_t *x, int depth, int c, int w)
{
        if (x == NULL) {
                return;
        }
        
        move(depth * 2, c);
        printw("%lu", x->key);
        refresh();

        btree_dump_node(x->left,  depth + 1, c - w/2, w/2);
        btree_dump_node(x->right, depth + 1, c + w/2, w/2);

        return;
}

static void
btree_dump(btree_t *b)
{
        initscr();
        btree_dump_node(b->root, 0, 40, 48);
        refresh();
        endwin();
}

#include "stdlib.h"

int 
main()
{
        btree_t *b;
        uint32_t i, *x;
        uint32_t v[] = {15, 5, 16, 3, 12, 20, 10, 13, 18, 23, 6, 7}; 
        uint32_t nv = sizeof(v) / sizeof(v[0]);

        btree_create(&b);

        for(i = 0; i < nv; i++) {
                x = (uint32_t*)xmalloc(sizeof(uint32_t));
                *x = (uint32_t)random();
                if (btree_add(b, v[i], (void*)x) != TRUE) {
                        printf("Fail Add %lu %lu\n", v[i], *x);
                }
        }
    
        printf("depth %d\n", btree_depth(b->root));
        btree_dump(b);

        sleep(3);
        btree_remove(b, 5, (void*)&x);
        btree_dump(b);
        sleep(3);
        btree_remove(b, 16, (void*)&x);
        btree_dump(b);
        sleep(3);
        btree_remove(b, 13, (void*)&x);
        btree_dump(b);

        while (btree_get_root_key(b, &i)) {
                if (btree_remove(b, i, (void*)&x) == FALSE) {
                        fprintf(stderr, "Failed to remove %lu\n", i);
                }
                btree_dump(b);
                sleep(1); 
        }

        if (btree_destroy(&b) == FALSE) {
                printf("Failed to destroy \n");
        }
                
        return 0;
}

#endif /* TEST_BTREE*/


