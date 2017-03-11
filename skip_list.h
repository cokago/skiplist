
#define ZSKIPLIST_MAXLEVEL 32 /* should be enough for 2^32 elements */
#define ZSKIPLIST_P 0.25      /* skiplist p = 1/4 */

/* Struct to hold a inclusive/exclusive range spec by score comparison. */
typedef struct {
    double min, max;
    int minex, maxex; /* are min or max exclusive? */
} zrangespec;


/* ZSETs use a specialized version of Skiplists */
typedef struct zskiplistNode {
    long ele;
    double score;
    struct zskiplistNode *backward;
    struct zskiplistLevel {
        struct zskiplistNode *forward;
        unsigned int span;
    } level[];
} zskiplistNode;

typedef struct zskiplist {
    struct zskiplistNode *header, *tail;
    unsigned long length;
    int level;
} zskiplist;

/* Create a skiplist node with the specified number of levels.
 * The SDS string 'ele' is referenced by the node after the call. */
zskiplistNode *zslCreateNode(int level, double score, long ele);

/* Create a new skiplist. */
zskiplist *zslCreate(void);

/* Free the specified skiplist node. The referenced SDS string representation
 * of the element is freed too, unless node->ele is set to NULL before calling
 * this function. */
void zslFreeNode(zskiplistNode *node);

/* Free a whole skiplist. */
void zslFree(zskiplist *zsl);

/* Insert a new node in the skiplist. Assumes the element does not already
 * exist (up to the caller to enforce that). The skiplist takes ownership
 * of the passed SDS string 'ele'. */
zskiplistNode *zslInsert(zskiplist *zsl, double score, long ele);

/* Delete an element with matching score/element from the skiplist.
 * The function returns 1 if the node was found and deleted, otherwise
 * 0 is returned.
 *
 * If 'node' is NULL the deleted node is freed by zslFreeNode(), otherwise
 * it is not freed (but just unlinked) and *node is set to the node pointer,
 * so that it is possible for the caller to reuse the node (including the
 * referenced SDS string at node->ele). */
int zslDelete(zskiplist *zsl, double score, long ele, zskiplistNode **node);
 
/* Find the first node that is gt score
 * Returns NULL when no element is contained in the range. */
zskiplistNode *zslNodeGT(zskiplist *zsl, double score);
 
/* Find the last node that is lt score
 * Returns NULL when no element is contained in the range. */
zskiplistNode *zslNodeLT(zskiplist *zsl, double score);
 

// 向后拷贝至多limit个，且score<min
int zslNodeCopy(zskiplistNode* start, int limit, long lst[]);
int zslNodeCopy(zskiplistNode* start, int limit, double max, long lst[]);
// 向前拷贝至多limit个，且score>min
int zslNodeRevCopy(zskiplistNode* start, int limit, long lst[]);
int zslNodeRevCopy(zskiplistNode* start, int limit, double min, long lst[]);

