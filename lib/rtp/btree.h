/*
 * FILE:    btree.h
 * PROGRAM: RAT
 * AUTHOR:  O.Hodson
 * 
 * Binary tree implementation - Mostly verbatim from:
 *
 * Introduction to Algorithms by Corman, Leisserson, and Rivest,
 * MIT Press / McGraw Hill, 1990.
 *
 * Implementation assumes one data element per key is assigned.
 *
 * Thanks to Markus Geimeier <mager@tzi.de> for pointing out want
 * btree_get_min_key and not btree_get_root_key for start point for tree
 * iteration.
 * 
 */

#ifndef __BTREE_H__
#define __BTREE_H__

typedef struct s_btree btree_t;

/* btree_create fills the *tree with a new binary tree.  Returns TRUE on     */
/* success and FALSE on failure.                                             */

int  btree_create  (btree_t **tree);

/* btree_destroy destroys tree.  If tree is not empty it cannot be destroyed */
/* and call returns FALSE.  On success returns TRUE.                         */

int btree_destroy (btree_t **tree);
 
/* btree_add adds data for key to tree.  Returns TRUE on success, or FALSE   */
/* if key is already on tree.  */

int btree_add    (btree_t *tree, uint32_t key, void *data);

/* btree_remove attempts to remove data from tree.  Returns TRUE on success  */
/* and points *d at value stored for key.  FALSE is returned if key is not   */
/* on tree.                                                                  */

int btree_remove (btree_t *tree, uint32_t key, void **d);

/* btree_find locates data for key and make *d point to it.  Returns TRUE if */
/* data for key is found, FALSE otherwise.                                   */

int btree_find   (btree_t *tree, uint32_t key, void **d);

/* btree_get_min_key attempts to return minimum key of tree.  Function       */
/* intended to help if list of keys is not stored elsewhere.  Returns TRUE   */
/* and fills key with key if possible, FALSE otherwise                       */

int btree_get_min_key (btree_t *tree, uint32_t *key);
int btree_get_max_key (btree_t *tree, uint32_t *key);

/* btree_get_next_key attempts to get the key above cur_key.  Returns        */
/* TRUE and fills key with key if possible, FALSE otherwise.                 */

int btree_get_next_key (btree_t *tree, uint32_t cur_key, uint32_t *next_key);

#endif /* __BTREE_H__ */

