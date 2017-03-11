
#include "skip_list.h"

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "zmalloc.h"

/* Create a skiplist node with the specified number of levels.
 * The SDS string 'ele' is referenced by the node after the call. */
zskiplistNode *zslCreateNode(int level, double score, long ele) {
    zskiplistNode *zn =
        (zskiplistNode*)zmalloc(sizeof(*zn)+level*sizeof(struct zskiplistNode::zskiplistLevel));
    zn->score = score;
    zn->ele = ele;
    return zn;
}

/* Create a new skiplist. */
zskiplist *zslCreate(void) {
    int j;
    zskiplist *zsl;

    zsl = (zskiplist*)zmalloc(sizeof(*zsl));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0,0);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        zsl->header->level[j].forward = NULL;
        zsl->header->level[j].span = 0;
    }
    zsl->header->backward = NULL;
    zsl->tail = NULL;
    return zsl;
}

/* Free the specified skiplist node. The referenced SDS string representation
 * of the element is freed too, unless node->ele is set to NULL before calling
 * this function. */
void zslFreeNode(zskiplistNode *node) {
    zfree(node);
}

/* Free a whole skiplist. */
void zslFree(zskiplist *zsl) {
    zskiplistNode *node = zsl->header->level[0].forward, *next;

    zfree(zsl->header);
    while(node) {
        next = node->level[0].forward;
        zslFreeNode(node);
        node = next;
    }
    zfree(zsl);
}

/* Returns a random level for the new skiplist node we are going to create.
 * The return value of this function is between 1 and ZSKIPLIST_MAXLEVEL
 * (both inclusive), with a powerlaw-alike distribution where higher
 * levels are less likely to be returned. */
int zslRandomLevel(void) {
    int level = 1;
    while ((random()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
        level += 1;
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

/* Insert a new node in the skiplist. Assumes the element does not already
 * exist (up to the caller to enforce that). The skiplist takes ownership
 * of the passed SDS string 'ele'. */
zskiplistNode *zslInsert(zskiplist *zsl, double score, long ele) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    unsigned int rank[ZSKIPLIST_MAXLEVEL];
    int i, level;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* store rank that is crossed to reach the insert position */
        rank[i] = i == (zsl->level-1) ? 0 : rank[i+1];
        while (x->level[i].forward &&
                (x->level[i].forward->score < score ||
                    (x->level[i].forward->score == score &&
                    x->level[i].forward->ele < ele)))
        {
            rank[i] += x->level[i].span;
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* we assume the element is not already inside, since we allow duplicated
     * scores, reinserting the same element should never happen since the
     * caller of zslInsert() should test in the hash table if the element is
     * already inside or not. */
    level = zslRandomLevel();
    if (level > zsl->level) {
        for (i = zsl->level; i < level; i++) {
            rank[i] = 0;
            update[i] = zsl->header;
            update[i]->level[i].span = zsl->length;
        }
        zsl->level = level;
    }
    x = zslCreateNode(level,score,ele);
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;

        /* update span covered by update[i] as x is inserted here */
        x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (rank[0] - rank[i]) + 1;
    }

    /* increment span for untouched levels */
    for (i = level; i < zsl->level; i++) {
        update[i]->level[i].span++;
    }

    x->backward = (update[0] == zsl->header) ? NULL : update[0];
    if (x->level[0].forward)
        x->level[0].forward->backward = x;
    else
        zsl->tail = x;
    zsl->length++;
    return x;
}

/* Internal function used by zslDelete, zslDeleteByScore and zslDeleteByRank */
void zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update) {
    int i;
    for (i = 0; i < zsl->level; i++) {
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].span += x->level[i].span - 1;
            update[i]->level[i].forward = x->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (x->level[0].forward) {
        x->level[0].forward->backward = x->backward;
    } else {
        zsl->tail = x->backward;
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}

/* Delete an element with matching score/element from the skiplist.
 * The function returns 1 if the node was found and deleted, otherwise
 * 0 is returned.
 *
 * If 'node' is NULL the deleted node is freed by zslFreeNode(), otherwise
 * it is not freed (but just unlinked) and *node is set to the node pointer,
 * so that it is possible for the caller to reuse the node (including the
 * referenced SDS string at node->ele). */
int zslDelete(zskiplist *zsl, double score, long ele, zskiplistNode **node) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    int i;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
                (x->level[i].forward->score < score ||
                    (x->level[i].forward->score == score &&
                     x->level[i].forward->ele < ele)))
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    /* We may have multiple elements with the same score, what we need
     * is to find the element with both the right score and object. */
    x = x->level[0].forward;
    if (x && score == x->score && x->ele == ele) {
        zslDeleteNode(zsl, x, update);
        if (!node)
            zslFreeNode(x);
        else
            *node = x;
        return 1;
    }
    return 0; /* not found */
}

bool zslValueLTE(double val, double score) {
    return val<=score;
}
bool zslValueLT(double val, double score) {
    return val<score;
}

bool zslValueGTE(double val, double score) {
    return val>=score;
}
bool zslValueGT(double val, double score) {
    return val>score;
}

/* Returns if there is a part of the zset is in range. */
bool zslIsInRange(zskiplist *zsl, double score, bool eq) {
    zskiplistNode *x;

    x = zsl->tail;
    if (x == NULL || x->score < score)
        return false;
    x = zsl->header->level[0].forward;
    if (x == NULL || x->score > score)
        return false;
    return true;
}

/* Find the first node that is gt score
 * Returns NULL when no element is contained in the range. */
zskiplistNode *zslNodeGT(zskiplist *zsl, double score) {
    zskiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInRange(zsl, score, false)) return NULL;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *OUT* of range. */
        while (x->level[i].forward &&
            zslValueLTE(x->level[i].forward->score, score))
                x = x->level[i].forward;
    }

    x = x->level[0].forward;
    assert(x != NULL);

    return x;
}

/* Find the last node that is lt score
 * Returns NULL when no element is contained in the range. */
zskiplistNode *zslNodeLT(zskiplist *zsl, double score) {
    zskiplistNode *x;
    int i;

    /* If everything is out of range, return early. */
    if (!zslIsInRange(zsl, score, false)) return NULL;

    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        /* Go forward while *IN* range. */
        while (x->level[i].forward &&
            zslValueLT(x->level[i].forward->score, score))
                x = x->level[i].forward;
    }

    assert(x != NULL);

    return x;
}

int zslNodeCopy(zskiplistNode* start, int limit, double max, long lst[]) {
    int n = 0;
    if (start == NULL) return n;
    zskiplistNode* x = start;
    while(n < limit && x != NULL) {
        if (zslValueGTE(x->score, max)) break;
        lst[n++] = x->ele;
        x = x->level[0].forward;
    }
    return n;
}

int zslNodeRevCopy(zskiplistNode* start, int limit, double min, long lst[]) {
    int n = 0;
    if (start == NULL) return n;
    zskiplistNode* x = start;
    while(n < limit && x != NULL) {
        if (zslValueLTE(x->score, min)) break;
        lst[n++] = x->ele;
        x = x->backward;
    }
    return n;
}

int zslNodeCopy(zskiplistNode* start, int limit, long lst[]) {
    int n = 0;
    if (start == NULL) return n;
    zskiplistNode* x = start;
    while(n < limit && x != NULL) {
        lst[n++] = x->ele;
        x = x->level[0].forward;
    }
    return n;
}

int zslNodeRevCopy(zskiplistNode* start, int limit, long lst[]) {
    int n = 0;
    if (start == NULL) return n;
    zskiplistNode* x = start;
    while(n < limit && x != NULL) {
        lst[n++] = x->ele;
        x = x->backward;
    }
    return n;
}

