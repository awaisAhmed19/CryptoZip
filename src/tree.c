// tree.c
/***************************************************************************
 *          Lempel, Ziv Encoding and Decoding
 *
 *   File    : tree.c
 *
 ***************************************************************************/

/***************************************************************************
 *                             INCLUDED FILES
 ***************************************************************************/
#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#define NULL_INDEX -1

struct node {
    int len, off;
    int parent;
    int left, right;
};

struct node *createTree(int size) {
    ASSERT(size > 0);
    struct node *tree = calloc(size, sizeof(struct node));
    ASSERT(tree != NULL);  // Make sure allocation succeeded
    return tree;
}

void destroyTree(struct node *tree) {
    ASSERT(tree != NULL);
    free(tree);
}

void insert(struct node *tree, int *root, unsigned char *window, int abs_off, int len, int max) {
    ASSERT(tree != NULL);
    ASSERT(root != NULL);
    ASSERT(window != NULL);
    ASSERT(len > 0);
    ASSERT(max > 0);

    int i, tmp;
    int off = abs_off % max;

    ASSERT(off >= 0 && off < max);

    if (*root == NULL_INDEX) {
        *root = off;
        tree[*root].parent = NULL_INDEX;
    } else {
        i = *root;
        while (1) {
            tmp = i;
            ASSERT(i >= 0 && i < max);
            // Compare sequences safely:
            int cmp = memcmp(&(window[abs_off]), &(window[tree[i].off]), len);
            if (cmp < 0) {
                i = tree[i].left;
                if (i == NULL_INDEX) {
                    tree[tmp].left = off;
                    tree[off].parent = tmp;
                    break;
                }
                ASSERT(i >= 0 && i < max);
            } else {
                i = tree[i].right;
                if (i == NULL_INDEX) {
                    tree[tmp].right = off;
                    tree[off].parent = tmp;
                    break;
                }
                ASSERT(i >= 0 && i < max);
            }
        }
    }

    tree[off].off = abs_off;
    tree[off].len = len;
    tree[off].left = NULL_INDEX;
    tree[off].right = NULL_INDEX;
}

struct ret find(struct node *tree, int root, unsigned char *window, int index, int size) {
    ASSERT(tree != NULL);
    ASSERT(window != NULL);
    ASSERT(size > 0);

    struct ret off_len = {0, 0};

    if (root == NULL_INDEX) return off_len;
    int j = root;

    while (1) {
        ASSERT(j >= 0);

        int i;
        for (i = 0; i < size && window[index + i] == window[tree[j].off + i]; i++) {
            // loop body empty
        }

        if (i > off_len.len) {
            off_len.off = index - tree[j].off;
            off_len.len = i;
        }

        if (window[index + i] < window[tree[j].off + i] && tree[j].left != NULL_INDEX) {
            j = tree[j].left;
            ASSERT(j >= 0);
        } else if (window[index + i] > window[tree[j].off + i] && tree[j].right != NULL_INDEX) {
            j = tree[j].right;
            ASSERT(j >= 0);
        } else
            break;
    }

    return off_len;
}

int minChild(struct node *tree, int index) {
    ASSERT(tree != NULL);
    ASSERT(index >= 0);

    int min = index;
    while (tree[min].left != NULL_INDEX) {
        min = tree[min].left;
        ASSERT(min >= 0);
    }
    return min;
}

void delete(struct node *tree, int *root, unsigned char *window, int abs_sb, int max) {
    ASSERT(tree != NULL);
    ASSERT(root != NULL);
    ASSERT(window != NULL);
    ASSERT(max > 0);

    int parent, child, sb;
    sb = abs_sb % max;

    ASSERT(sb >= 0 && sb < max);

    if (tree[sb].left == NULL_INDEX) {
        child = tree[sb].right;
        if (child != NULL_INDEX) {
            ASSERT(child >= 0 && child < max);
            tree[child].parent = tree[sb].parent;
        }
        parent = tree[sb].parent;
    } else if (tree[sb].right == NULL_INDEX) {
        child = tree[sb].left;
        ASSERT(child >= 0 && child < max);
        tree[child].parent = tree[sb].parent;
        parent = tree[sb].parent;
    } else {
        child = minChild(tree, tree[sb].right);

        ASSERT(child >= 0 && child < max);

        if (tree[child].parent == sb) {
            parent = tree[sb].parent;
            tree[child].parent = parent;
        } else {
            parent = tree[child].parent;
            ASSERT(parent >= 0 && parent < max);
            tree[parent].left = tree[child].right;

            if (tree[child].right != NULL_INDEX) {
                ASSERT(tree[child].right >= 0 && tree[child].right < max);
                tree[tree[child].right].parent = parent;
            }

            tree[child].right = tree[sb].right;
            tree[child].parent = tree[sb].parent;

            if (tree[child].right != NULL_INDEX) {
                ASSERT(tree[child].right >= 0 && tree[child].right < max);
                tree[tree[child].right].parent = child;
            }
            parent = tree[child].parent;
        }

        tree[child].left = tree[sb].left;
        if (tree[child].left != NULL_INDEX) {
            ASSERT(tree[child].left >= 0 && tree[child].left < max);
            tree[tree[child].left].parent = child;
        }
    }

    if (parent != NULL_INDEX) {
        ASSERT(parent >= 0 && parent < max);
        if (tree[parent].right == sb)
            tree[parent].right = child;
        else
            tree[parent].left = child;
    } else {
        *root = child;
    }
}

void updateOffset(struct node *tree, int n, int max) {
    ASSERT(tree != NULL);
    ASSERT(max > 0);
    ASSERT(n >= 0);

    for (int i = 0; i < max; i++) {
        if (tree[i].off != NULL_INDEX) {
            ASSERT(tree[i].off >= n);  // so that off-n won't go negative
            tree[i].off -= n;
        }
    }
}

void check_tree(struct node *tree, int max) {
    ASSERT(tree != NULL);
    ASSERT(max > 0);

    for (int i = 0; i < max; i++) {
        if (tree[i].off != -1 &&
            (tree[i].parent >= max || tree[i].left >= max || tree[i].right >= max)) {
            printf("Corruption at node %d\n", i);
        }
    }
}

void printtree(struct node *tree, int root) {
    ASSERT(tree != NULL);

    if (root == NULL_INDEX) return;

    ASSERT(root >= 0);

    if (tree[root].left != NULL_INDEX) printtree(tree, tree[root].left);

    printf("%d\n", tree[root].off);
    printf("Node[%d]: off=%d len=%d parent=%d left=%d right=%d\n", root, tree[root].off,
           tree[root].len, tree[root].parent, tree[root].left, tree[root].right);

    if (tree[root].right != NULL_INDEX) printtree(tree, tree[root].right);
}
