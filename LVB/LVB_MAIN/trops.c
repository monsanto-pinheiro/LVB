/* LVB
 * (c) Copyright 2003-2012 by Daniel Barker.
 * (c) Copyright 2013, 2014 by Daniel Barker and Maximilian Strobl.
 * Permission is granted to copy and use this program provided that no fee is
 * charged for it and provided that this copyright notice is not removed. */

/* ********** trops.c - tree operations ********** */

#include "lvb.h"

#define CLADESEP ","	/* clade separator for trees */

/* maximum number of object sets per tree */
#define MAX_SSET_SIZE (MAX_N - 3)

typedef	struct	/* object set derived from a cladogram */
{
	long *set;	/* arrays of object sets */
	long cnt;	/* sizes of object sets */
}	Objset;

static void cr_bpnc(const Branch *const barray, const long branch);
static void cr_chaf(const Branch *const barray, const long destination, 
 const long newchild);
static void cr_uxe(FILE *const stream, const char *const msg);
static void fillsets(Objset *const sstruct, const Branch *const tree,
 const long root);
static void getobjs(const Branch *const barray, const long root,
 long *const objarr, long *const cnt);
static long getsister(const Branch *const barray, const long branch);
static long makesets(const Branch *const tree_1, const long root_1,
 const Branch *const tree_2, const long root_2);
static int objnocmp(const void *o1, const void *o2);
static int osetcmp(const void *oset1, const void *oset2);
static long *randleaf(Dataptr, Branch *const barray,
 const Lvb_bool *const leafmask, const long objs);
static void realgetobjs(const Branch *const barray, const long root,
 long *const objarr, long *const cnt);
static Lvb_bool *randtopology(Dataptr, Branch *const barray, const long nobjs);
static long setstcmp(Objset *const oset_1, Objset *const oset_2, const long nels);
static void sort(Objset *const oset, const long nels);
static void ssarralloc(Objset *nobjset, const long nsets, const long setsize);
static void tree_make_canonical(Dataptr, Branch *const barray, long *objnos);
static void ur_print(Dataptr, FILE *const stream, const Branch *const barray, const long root);

/* object sets for tree 1 in comparison */
static Objset sset_1[MAX_N - 3] = { { NULL, 0 } };

/* object sets for tree 2 in comparison */
static Objset sset_2[MAX_N - 3] = { { NULL, 0 } };

void nodeclear(Branch *const barray, const long brnch)
/* Initialize all scalars in branch brnch to UNSET or zero as appropriate,
 * and mark it "dirty" */
{
    barray[brnch].left = UNSET;
    barray[brnch].right = UNSET;
    barray[brnch].parent = UNSET;
    barray[brnch].changes = UNSET;
    barray[brnch].sset[0] = 0U;		/* "dirty" */

} /* end nodeclear() */

static long tree_bytes(Dataptr matrix)
/* return bytes required for contiguous allocation of a tree for the data
 * accessible by matrix, if branches and their statesets are allocated
 * as one contiguous array */
{
    long bytes;		/* bytes required */
    long branches;	/* branches in the tree */

    branches = brcnt(matrix->n);
    bytes = branches * sizeof(Branch);
    bytes += branches * matrix->m * sizeof(unsigned char);

    return bytes;
} /* end tree_bytes() */

void treeclear(Dataptr matrix, Branch *const barray)
/* clear all branches in array barray, on the assumption that its size fits
 * the data matrix; mark all branches dirty */
{
    long nbranches = brcnt(matrix->n);		/* branch count */
    long i;					/* loop counter */

    for (i = 0; i < nbranches; i++)
	nodeclear(barray, i);

} /* end treeclear() */

long brcnt(long n)
/* return number of branches in unrooted binary tree structure
 * containing n tips */
{
    return (n << 1) - 3;

} /* end brcnt() */

static void make_dirty_below(Dataptr matrix, Branch *tree, long dirty_node)
/* mark nodes "dirty" from branch dirty_node, which must not be the root,
 * down to (but not including) the root branch of the tree tree; the true
 * root lies outside the LVB tree data structure so cannot be marked
 * dirty, but will always be dirty after any rearrangement */
{
    long dirty_parent;		/* parent of current dirty node */

    lvb_assert(dirty_node >= matrix->n);	/* not leaf/root */
    dirty_parent = tree[dirty_node].parent;
    lvb_assert(dirty_parent != UNSET);
    do {
	tree[dirty_node].sset[0] = 0U;	/* "dirty" */
	dirty_parent = tree[dirty_node].parent;
        dirty_node = dirty_parent;
    } while (tree[dirty_node].parent != UNSET);

} /* end make_dirty_below() */

static void make_dirty_tree(Dataptr matrix, Branch *tree)
/* mark all branches in tree tree as dirty: internal, external and root */
{
    long nbranches = brcnt(matrix->n);		/* branch count */
    long i;					/* loop counter */
    long j;					/* loop counter */
    
    for (i = 0; i < nbranches; i++){
    	/* overkill beyond j=0, but harmless */
    	for (j = 0; j < matrix->m; j++){
    		tree[i].sset[j] = 0U;
    	}
    }

} /* end make_dirty_tree() */

void mutate_deterministic(Dataptr matrix, Branch *const desttree,
    const Branch *const sourcetree, long root, long p, Lvb_bool left)
/* make a copy of the tree sourcetree (of root root) in desttree,
 * with a change in topology, the change being caused by nearest
 * neighbour interchange (NNI) rearrangement at branch p, involving the
 * right node if right is LVB_TRUE, otherwise the left node; N.B. code
 * is largely copied from mutate_nni() */
{
    Branch *tree;
    long u, v, a, b, c;

    lvb_assert(p != root);
    lvb_assert(p >= matrix->n);

    /* for ease of reading, make alias of desttree, tree */
    tree = desttree;
    treecopy(matrix, tree, sourcetree);

    u = p;
    v = tree[u].parent;
    a = tree[u].left;
    b = tree[u].right;
 
    if (tree[v].left == u) c = tree[v].right;
    else c = tree[v].left;

    if (left != LVB_TRUE)
    {
		if (tree[v].left == u) tree[v].right = b;
		else tree[v].left = b;
		tree[u].left = a;
		tree[u].right = c;
		tree[a].parent = tree[c].parent = u;
		tree[b].parent = v;
    }
    else
    {
		if (tree[v].left == u) tree[v].right = a;
		else tree[v].left = a;
		tree[u].left = b;
		tree[u].right = c;
		tree[b].parent = tree[c].parent = u;
		tree[a].parent = v;
    }

    make_dirty_below(matrix, tree, u);

} /* end mutate_nni() */

void mutate_nni(Dataptr matrix, Branch *const desttree, const Branch *const sourcetree, long root)
/* make a copy of the tree sourcetree (of root root) in desttree,
 * with a random change in topology, the change being caused by nearest
 * neighbour interchange (NNI) rearrangement; N.B. the code is mostly
 * the same in mutate_deterministic() */
{
    long nbranches = brcnt(matrix->n);		/* branch count */
    Branch *tree;
    long p, u, v, a, b, c;

    /* for ease of reading, make alias of desttree, tree */
    tree = desttree;
    treecopy(matrix, tree, sourcetree);

    /* get a random internal branch */
    p = randpint(nbranches - matrix->n - 1);
    p += matrix->n;

    u = p;
    v = tree[u].parent;
    a = tree[u].left;
    b = tree[u].right;
 
    if (tree[v].left == u) c = tree[v].right;
    else c = tree[v].left;

    if (uni() < 0.5)  {
		if (tree[v].left == u) tree[v].right = b;
		else tree[v].left = b;
		tree[u].left = a;
		tree[u].right = c;
		tree[a].parent = tree[c].parent = u;
		tree[b].parent = v;
    }
    else {
		if (tree[v].left == u) tree[v].right = a;
		else tree[v].left = a;
		tree[u].left = b;
		tree[u].right = c;
		tree[b].parent = tree[c].parent = u;
		tree[a].parent = v;
    }

    make_dirty_below(matrix, tree, u);

} /* end mutate_nni() */

static Lvb_bool is_descendant(Branch *tree, long root, long ancestor, long
 candidate)
/* return LVB_TRUE if candidate is among the branches in the clade descending
from ancestor, LVB_FALSE otherwise */
{
    Lvb_bool val = LVB_FALSE;		/* return value */
    long par = tree[candidate].parent;	/* current parent */
    long newpar;			/* next parent */

    while (par != UNSET)
    {
	if (par == ancestor)
	{
	    val = LVB_TRUE;
	    par = UNSET;	/* exit the loop */
	}
	else
	{
	    newpar = tree[par].parent;
	    par = newpar;
	}
    }

    return val;

} /* end is_descendant() */

void mutate_spr(Dataptr matrix, Branch *const desttree, const Branch *const sourcetree, long root)
/* make a copy of the tree sourcetree (of root root) in desttree,
 * with a random change in topology, the change being caused by subtree
 * pruning and regrafting (SPR) rearrangement */
{
    long src;				/* branch to move */
    long dest;				/* destination of branch to move */
    long dest_parent;			/* parent of destination branch */
    long src_parent;			/* parent of branch to move */
    long excess_br;			/* branch temporarily excised */
    long orig_child = UNSET;		/* original child of destination */
    long parents_par;			/* parent of parent of br. to move */
    long src_sister;			/* sister of branch to move */
    Branch *tree;			/* destination tree */
    long nbranches = brcnt(matrix->n);	/* branches in tree */

    /* for ease of reading, make alias of desttree, tree */
    tree = desttree;
    treecopy(matrix, tree, sourcetree);

    /* get random branch but not root and not root's immediate descendant */
    do {
    	src = randpint(nbranches - 1);
    } while ((src == root) || (src == tree[root].left) || (src == tree[root].right));

    src_parent = tree[src].parent;
    lvb_assert(src_parent != UNSET);
    src_sister = getsister(tree, src);
    lvb_assert(src_sister != UNSET);

    /* get destination that is not source or its parent, sister or descendant
     * or the root */
    do {
    	dest = randpint(nbranches - 1);
    } while ((dest == src) || (dest == src_parent) || (dest == src_sister)
       || (dest == root) || is_descendant(tree, root, src, dest));

    /* excise source branch, leaving a damaged data structure */
    if (tree[src_parent].left == src) {
    	tree[src_parent].left = UNSET;
    }
    else if (tree[src_parent].right == src) {
    	tree[src_parent].right = UNSET;
    }
    else {
    	cr_bpnc(tree, src);
    }
    tree[src].parent = UNSET;

    /* fix data structure by "freeing" the excess branch */
    parents_par = tree[src_parent].parent;
    lvb_assert(parents_par != UNSET);
    if (tree[parents_par].left == src_parent) {
    	tree[parents_par].left = src_sister;
    }
    else {
    	tree[parents_par].right = src_sister;
    }
    tree[src_sister].parent = parents_par;

    excess_br = src_parent;	/* for ease of human understanding */
    nodeclear(tree, excess_br);

    /* make space at destination, re-using the excess branch */
    dest_parent = tree[dest].parent;
    if (tree[dest_parent].left == dest) {
    	orig_child = tree[dest_parent].left;
    	tree[dest_parent].left = excess_br;
    }
    else if (tree[dest_parent].right == dest) {
    	orig_child = tree[dest_parent].right;
    	tree[dest_parent].right = excess_br;
    }
    else {
    	crash("destination %ld is not a child of it's parent %ld\n",
    			dest, dest_parent);
    }
    tree[excess_br].parent = dest_parent;
    tree[excess_br].left = dest;
    lvb_assert(orig_child != UNSET);
    tree[orig_child].parent = excess_br;

    /* add source branch to this new location */
    tree[excess_br].right = src;
    tree[src].parent = excess_br;

    /* ensure recalculation of lengths where necessary */
    make_dirty_below(matrix, tree, excess_br);
    if (parents_par != root){
    	make_dirty_below(matrix, tree, parents_par);
    }

} /* end mutate_spr() */

long lvb_reroot(Branch *const barray, const long oldroot, const long newroot)
/* Change the root of the tree in barray from oldroot to newroot, which
 * must not be the same. Mark all internal nodes (everything but the leaves
 * and root) as "dirty". Return oldroot. */
{
    long current;		/* current branch */
    long parnt;			/* parent of current branch */
    long sister = UNSET;	/* sister of current branch */
    long previous;		/* previous branch */
    extern Dataptr matrix;	/* data matrix */
    long nbranches = brcnt(matrix->n);		/* branches in tree */
    static long oldparent[MAX_BRANCHES];	/* element i was old
						 * parent of i */

    /* check new root is a leaf but not the current root */
    lvb_assert(newroot < matrix->n);
    lvb_assert(newroot != oldroot);

    /* create record of parents as they are now */
    for (current = 0; current < nbranches; current++)
	oldparent[current] = barray[current].parent;

    current = newroot;
    previous = UNSET;
    while (current != oldroot)
    {
	lvb_assert(current != UNSET);
	parnt = oldparent[current];		/* original parent */
	if (current == barray[parnt].left)
	    sister = barray[parnt].right;
	else if (current == barray[parnt].right)
	    sister = barray[parnt].left;
	else	/* error in tree structure */
	    crash("internal error in function lvb_reroot(): current\n"
	     "branch %ld has old parent %ld, but old parent does not\n"
	     "have it as a child", current, parnt);
	barray[current].parent = previous;	/* now chld of prev. */

	/* make former parent the new left child, and former sister the
	 * new right child of the current branch */
	barray[current].left = parnt;
	barray[current].right = sister;
	barray[parnt].parent = current;
	barray[sister].parent = current;
		
	/* move towards original root, i.e. to original parent of
	 * current branch */
	previous = current;
	current = parnt;
    }

    /* former root is now a normal leaf, without descendants */
    barray[oldroot].left = UNSET;
    barray[oldroot].right = UNSET;

    for (current = matrix->n; current < nbranches; current++)
	barray[current].sset[0] = 0U;

    return oldroot;

} /* end lvb_reroot() */

long arbreroot(Branch *const tree, const long oldroot)
/* Change tree's root arbitrarily, to a leaf other than oldroot.
 * Mark all nodes other than the leaves and root "dirty".
 * Return the number of the new root. */
{
    extern Dataptr matrix;	/* data matrix */
    long newroot;		/* new root */

    /* find a leaf that is not the current root */

    do {
	newroot = randpint(matrix->n - 1);
    } while (newroot == oldroot);

    lvb_reroot(tree, oldroot, newroot);
    return newroot;

} /* end arbreroot() */

static long getsister(const Branch *const barray, const long branch)
/* return number of sister of branch branch in tree in barray, or UNSET if
 * branch has none */
{
    long parnt;		/* parent of current branch */

    parnt = barray[branch].parent;
    if (parnt == UNSET)
	return UNSET;
    if (branch == barray[parnt].left)
	return barray[parnt].right;
    else if (branch == barray[parnt].right)
	return barray[parnt].left;
    else	/* error in tree structure */
    {
	cr_bpnc(barray, branch);
	return 0;	/* NEVER reached but it shuts up compilers */
    }

} /* end getsister() */

long childadd(Branch *const tree, const long destination,
 const long newchild)
/* replace unset child of destination with newchild, and return
 * destination */
{
    if (tree[destination].right == UNSET)
	tree[destination].right = newchild;
    else if (tree[destination].left == UNSET)
	tree[destination].left = newchild;
    else	/* error: destination already has 2 children */
	cr_chaf(tree, destination, newchild);
    tree[newchild].parent = destination;
    return destination;

} /* end childadd() */

static void cr_chaf(const Branch *const barray, const long destination, 
 const long newchild)
/* crash because we want to add branch newchild to the children of
 * branch destination in tree in barray, but it already has two so
 * there is no room */
{
    crash("internal error in tree array %p: cannot make branch %ld a\n"
     "child of branch %ld since this already has 2 children (left is\n"
     "branch %ld, right is branch %ld)", (const void *) barray,
     newchild, destination, barray[destination].left,
     barray[destination].right);

} /* end cr_chaf() */

static void cr_bpnc(const Branch *const barray, const long branch)
/* crash because branch branch in tree in barray is not connected to
 * its parent, i.e. it is not a child of the branch it claims as
 * parent, according to that 'parent's' record of its own children */
{
    const long parnt = barray[branch].parent;	/* parent of branch */

    crash("internal error in tree array %p: branch record %ld says\n"
     "it has parent %ld, but branch record %ld has left child %ld\n"
     "and right child %ld", (const void *) barray, branch, parnt,
     parnt, barray[parnt].left, barray[parnt].right);

}	/* end cr_bpnc() */

void treecopy(Dataptr matrix, Branch *const dest, const Branch *const src)
/* copy tree from src to dest; dest must be totally distinct from source
 * in memory, and have enough space; the approach used below may fail if
 * treealloc() is changed */
{
    long nbranches = brcnt(matrix->n);	/* branches per tree */
    long i;				/* loop counter */
    unsigned char *tmp_sset;		/* temporary variable used in copy */
    unsigned char *src_statesets_all;	/* start of source's statesets */
    unsigned char *dest_statesets_all;	/* start of dest's statesets */
    
    /* scalars */
    for (i = 0; i < nbranches; i++) {
		tmp_sset = dest[i].sset;
		dest[i] = src[i];
		dest[i].sset = tmp_sset;	/* keep dest's stateset arrs for dest */
    }

    /* stateset arrays */
    src_statesets_all = ((unsigned char *) src) + nbranches * sizeof(Branch);
    dest_statesets_all = ((unsigned char *) dest) + nbranches * sizeof(Branch);
    memcpy(dest_statesets_all, src_statesets_all, nbranches * matrix->m);

} /* end treecopy() */

void randtree(Dataptr matrix, Branch *const barray)
/* fill barray with a random tree, where barray[0] is the root; all branches
 * in this random tree are marked as "dirty" */
{
    Lvb_bool *leafmask;		/* LVB_TRUE where branch in array is a leaf */
    long *objnos;		/* element i is obj associated with branch i */

    treeclear(matrix, barray);
    leafmask = randtopology(matrix, barray, matrix->n);
    objnos = randleaf(matrix, barray, leafmask, matrix->n);
    tree_make_canonical(matrix, barray, objnos);

} /* end randtree() */

static void wherever_was_now_say(Dataptr matrix, Branch *const barray, long was, long now)
{
    long nbranches = brcnt(matrix->n);	/* branches in tree */
    long branchno;			/* loop counter */

    for (branchno = 0; branchno < nbranches; branchno++){
		if(barray[branchno].parent == was) barray[branchno].parent = now;
		if(barray[branchno].left == was) barray[branchno].left = now;
		if(barray[branchno].right == was) barray[branchno].right = now;
    }

} /* end wherever_was_now_say() */

static void tree_make_canonical(Dataptr matrix, Branch *const barray, long *objnos)
/* ensure that objects 0, 1, 2, ... n-1 are associated with branches 0, 1, 2,
 * ... n-1, respectively; objnos indicates for each branch the currently
 * assigned object or UNSET for internal branches */
{
    long i;				/* loop counter */
    long nbranches = brcnt(matrix->n);	/* number of branches */
    long obj_no;			/* current object number */
    long impossible_1 = nbranches;	/* an out-of-range branch index */
    long impossible_2 = nbranches + 1;	/* an out-of-range branch index */
    long root = UNSET;			/* root branch index */
    Branch tmp_1, tmp_2;		/* temporary branches for swapping */
    unsigned char *ss0_start = (unsigned char *) barray[0].sset;	/* start of state set memory */
    Lvb_bool swap_made;			/* flag to indicate swap made */
    long tmp;				/* for swapping */

    do {
		swap_made = LVB_FALSE;
		for (i = 0; i < nbranches; i++) {
			obj_no = objnos[i];
			if ((obj_no != UNSET) && (obj_no != i)) {
				tmp_1 = barray[obj_no];
				wherever_was_now_say(matrix, barray,  obj_no, impossible_1);
				tmp_2 = barray[i];
				wherever_was_now_say(matrix, barray, i, impossible_2);
				if (tmp_1.parent == i) tmp_1.parent = impossible_2;
				if (tmp_1.left == i) tmp_1.left = impossible_2;
				if (tmp_1.right == i) tmp_1.right = impossible_2;
				if (tmp_2.parent == obj_no) tmp_2.parent = impossible_1;
				if (tmp_2.left == obj_no) tmp_2.left = impossible_1;
				if (tmp_2.right == obj_no) tmp_2.right = impossible_1;
				barray[i] = tmp_1;
				barray[obj_no] = tmp_2;
				wherever_was_now_say(matrix, barray, impossible_1, i);
				wherever_was_now_say(matrix, barray, impossible_2, obj_no);
				tmp = objnos[i];
				objnos[i] = objnos[obj_no];
				objnos[obj_no] = tmp;
				swap_made = LVB_TRUE;
			}
		}
    } while (swap_made == LVB_TRUE);

    /* patch up assignment of sset memory to prevent trouble in treecopy() */
    for (i = 0; i < nbranches; i++){
    	barray[i].sset = ss0_start + i * matrix->m;
    }

    for (i = 0; i < matrix->n; i++) {
		if (barray[i].parent == UNSET){
			lvb_assert(root == UNSET);
			root = i;
		}
    }

    if (root != 0) {
    	lvb_reroot(barray, root, 0);
    }

    /* check things didn't go haywire */
    for (i = 0; i < matrix->n; i++) {
    	lvb_assert(objnos[i] == i);
    }
    for (i =  matrix->n; i < nbranches; i++) {
    	lvb_assert(objnos[i] == UNSET);
    }

} /* end tree_make_canonical() */

Branch *treealloc(Dataptr matrix)
/* Return array of nbranches branches with scalars all UNSET, and all
 * statesets allocated for m characters but marked "dirty". Crash
 * verbosely if impossible. Memory is allocated once only, as a contiguous
 * block for the branch data structures followed by all their statesets.
 * So, to deallocate the tree, call the standard library function free()
 * ONCE ONLY, passing it the address of the first branch struct. If this
 * allocation approach is changed, be sure to change treecopy() too. */
{
    Branch *barray;			/* tree */
    unsigned char *barray_uchar_star;	/* tree as unsigned char */
    unsigned char *ss0_start;		/* start of first stateset */
    long i;				/* loop counter */
    long nbranches = brcnt(matrix->n);

    lvb_assert(nbranches >= MIN_BRANCHES);
    lvb_assert(nbranches <= MAX_BRANCHES);

    barray = alloc(tree_bytes(matrix), "tree with statesets");
    barray_uchar_star = (unsigned char *) barray;
    ss0_start = barray_uchar_star + nbranches * sizeof(Branch);

    for (i = 0; i < nbranches; i++) {
    	barray[i].sset = ss0_start + i * matrix->m;
    }

    make_dirty_tree(matrix, barray);
    return barray;

} /* end treealloc() */

static Lvb_bool *randtopology(Dataptr matrix, Branch *const barray, const long nobjs)
/* fill barray with tree of random topology, where barray[0] is root;
 * return static array where element i is LVB_TRUE if barray[i] is a
 * leaf or LVB_FALSE if it is not; this array will be overwritten in
 * subsequent calls */
{
    long i;		/* loop counter */
    long leaves = 0;	/* number of leaves */
    long nextfree = 0;	/* next unused element of barray */
    long togrow;	/* random candidate for sprouting */
    long nbranches = brcnt(nobjs);		/* branches in tree */
    static Lvb_bool isleaf[MAX_BRANCHES];	/* return value */

    lvb_assert(nobjs == matrix->n);

    /* clear the leaf mask */
    for (i = 0; i < nbranches; i++)
	isleaf[i] = LVB_FALSE;

    /* start with initial tree of 3 leaves */
    barray[0].parent = UNSET;
    isleaf[nextfree++] = LVB_TRUE;
    barray[0].left = nextfree;
    barray[nextfree].parent = 0;
    isleaf[nextfree++] = LVB_TRUE;
    barray[0].right = nextfree;
    barray[nextfree].parent = 0;
    isleaf[nextfree++] = LVB_TRUE;
    leaves = 3;

    /* sprout! */
    while(leaves < nobjs)
    {
	do	/* select a random leaf other than the root */
	{
	    togrow = 1 + randpint(nextfree - 2);
	} while (isleaf[togrow] == LVB_FALSE);
	/* left child */
	barray[togrow].left = nextfree;
	barray[nextfree].parent = togrow;
	isleaf[nextfree++] = LVB_TRUE;
	/* right child */
	barray[togrow].right = nextfree;
	barray[nextfree].parent = togrow;
	isleaf[nextfree++] = LVB_TRUE;
	/* other updates */
	isleaf[togrow] = LVB_FALSE;
	leaves++;
    }

    return isleaf;

} /* end randtopology() */

static long *randleaf(Dataptr matrix, Branch *const barray, const Lvb_bool *const leafmask, const long objs)
/* randomly assign objects numbered 0 to objs - 1 to leaves of tree in
 * barray; leaves in barray must be indicated by corresponding
 * LVB_TRUEs in leafmask; returns static array of object numbers, where
 * elements 0..nbranches give the object associated with branches 0..nbranches
 * respectively (UNSET for internal branches); this array will be overwritten
 * in subsequent calls */
{
    long assigned = 0;	/* for safety: should == objs at end */
    long candidate;			/* random object */
    long i;				/* loop counter */
    long nbranches = brcnt(objs);	/* branches in tree */
    static Lvb_bool used[MAX_N];	/* element i LVB_TRUE if object
    					 * i has leaf */
    static long objnos[MAX_BRANCHES];	/* object associated with branches */

    lvb_assert(objs < MAX_N);
    lvb_assert(objs == matrix->n);

    /* clear 'used' array */
    for (i = 0; i < objs; i++) used[i] = LVB_FALSE;

    /* clear object nos array, defaulting to internal branch, i.e. UNSET */
    for (i = 0; i < nbranches; i++){
    	objnos[i] = UNSET;
    }

    /* assign an object to every leaf */
    for (i = 0; i < nbranches; i++){
		if (leafmask[i] == LVB_TRUE)	/* leaf, requires object */
		{
			do	/* get a new object number */
			{
			candidate = randpint(objs - 1);
			} while(used[candidate] == LVB_TRUE);
			/* assign object to leaf */
			objnos[i] = candidate;
			used[candidate] = LVB_TRUE;
			assigned++;
		}
    }

    lvb_assert(assigned == objs);

    return objnos;

} /* end randleaf() */

void treeswap(Branch **const tree1, long *const root1,
 Branch **const tree2, long *const root2)
/* swap trees pointed to by tree1 and tree2; also swap records of their
 * roots, as pointed to by root1 and root2 */
{
    long tmproot;	/* temporary value-holder for swapping roots */
    Branch *tmptree;	/* temporary value-holder for swapping trees */

    /* swap pointers to branch arrays */
    tmptree = *tree1;
    *tree1 = *tree2;
    *tree2 = tmptree;

    /* swap roots */
    tmproot = *root1;
    *root1 = *root2;
    *root2 = tmproot;

} /* end treeswap() */

void treedump(Dataptr matrix, FILE *const stream, const Branch *const tree)
/* send tree as table of integers to file pointed to by stream */
{
    long i;                             /* loop counter */
    long j;                             /* loop counter */
    long nbranches = brcnt(matrix->n);  /* branches in tree */

    fprintf(stream,
        "Branch\tParent\tLeft\tRight\tChanges\tDirty\tSset_arr\tSsets\n");
    for (i = 0; i < nbranches; i++)
    {
        fprintf(stream, "%ld\t%ld\t%ld\t%ld\t%ld", i, tree[i].parent,
         tree[i].left, tree[i].right, tree[i].changes);
        if (tree[i].sset[0] == 0U)
            fprintf(stream, "\tyes");
        else
            fprintf(stream, "\tno");
        fprintf(stream, "\t%p", tree[i].sset);
        for (j = 0; j < matrix->m; j++)
        {
            fprintf(stream, "\t0%o", (unsigned) tree[i].sset[j]);
        }
        fprintf(stream, "\n");
    }
    fprintf(stream, "\n");
    if(ferror(stream) != 0)
        cr_uxe(stream, "dumping tree");

} /* end treedump() */

static void cr_uxe(FILE *const stream, const char *const msg)
/* crash because of problem on file stream, with message consisting of
 * "FATAL ERROR: ", then "file error " if the error indicator for
 * stream is set or "unexpected end of file " if not, then "when ",
 * followed by msg */
{
    if (ferror(stream))
	crash("file error when %s", msg);
    else
	crash("unexpected end of file when %s", msg);

} /* end cr_uxe */

void lvb_treeprint (Dataptr matrix, FILE *const stream, const Branch *const barray, const long root)
/* print tree in barray (of root root) in bracketed text form to stream stream,
 * in unrooted form */
{
    ur_print(matrix, stream, barray, root);
} /* end lvb_treeprint() */

static void ur_print(Dataptr matrix, FILE *const stream, const Branch *const barray, const long root)
/* send tree in barray, of root root, to file pointed to by stream in
 * unrooted form */
{
    long obj;					/* current object */
    static Lvb_bool doneabsroot = LVB_FALSE;	/* have output root */
    static Lvb_bool usecomma;			/* output clade sep. */
    char *tmp_title;				/* temporary string */

    obj = root;

    if (doneabsroot == LVB_FALSE)	/* print whole tree */
    {
		/* start tree */
		tmp_title = salloc(strlen(matrix->rowtitle[obj]), "temp. title");
		strcpy(tmp_title, matrix->rowtitle[obj]);
		while(tmp_title[strlen(tmp_title) - 1] == ' '){
			tmp_title[strlen(tmp_title) - 1] = '\0';
		}
		fprintf(stream, "(%s", tmp_title);
		free(tmp_title);    /* VERY LOCAL dynamic heap memory */
		usecomma = LVB_TRUE;
		doneabsroot = LVB_TRUE;

		ur_print(matrix, stream, barray, barray[root].left);
		ur_print(matrix, stream, barray, barray[root].right);

		/* end tree */
		fprintf(stream, ");\n");
		if (ferror(stream))
			crash("file error when writing unrooted tree");

		/* clean up for next call */
		usecomma = LVB_FALSE;
		doneabsroot = LVB_FALSE;
    }
    else	/* print remainder of tree */
    {
		if (usecomma == LVB_TRUE) fprintf(stream, "%s", CLADESEP);
		if (root < matrix->n)	/* leaf */
		{
			tmp_title = salloc(strlen(matrix->rowtitle[obj]), "temp. title");
			strcpy(tmp_title, matrix->rowtitle[obj]);
			while(tmp_title[strlen(tmp_title) - 1] == ' ')
			{
			tmp_title[strlen(tmp_title) - 1] = '\0';
			}
			fprintf(stream, "%s", tmp_title);
			free(tmp_title);	/* VERY LOCAL dynamic heap memory */
			usecomma = LVB_TRUE;
		}
		else
		{
			fprintf(stream, "(");
			usecomma = LVB_FALSE;
			ur_print(matrix, stream, barray, barray[root].left);
			ur_print(matrix, stream, barray, barray[root].right);
			fputc(')', stream);
			usecomma = LVB_TRUE;
		}
    }

} /* end ur_print() */

long treecmp(Dataptr matrix, const Branch *const tree_1, const long root_1,
		const Branch *const tree_2, long root_2)
/* return 0 if the topology of tree_1 (of root root_1) is the same as
 * that of tree_2 (of root root_2), or non-zero if different */
{
    static Branch *copy_2 = NULL;	/* possibly re-rooted tree 2 */
    static long prev_m = 0;		/* m on previous call */
    static long prev_n = 0;		/* n on previous call */
    long nsets;				/* elements per set array */

    /* allocate "local" static heap memory - static - do not free! */
    if (copy_2 == NULL) {
		copy_2 = treealloc(matrix);
		prev_m = matrix->m;
		prev_n = matrix->n;
    }
    lvb_assert((prev_m == matrix->m) && (prev_n == matrix->n));

    treecopy(matrix, copy_2, tree_2);
    lvb_assert(root_1 < matrix->n);
    if(root_1 != root_2) lvb_reroot(copy_2, root_2, root_1);
    root_2 = root_1;
    nsets = makesets(tree_1, root_1, copy_2, root_2);

    return setstcmp(sset_1, sset_2, nsets);

} /* end treecmp() */

static long setstcmp(Objset *const oset_1, Objset *const oset_2, const long nels)
/* return 0 if the same sets of objects are in oset_1 and oset_2,
 * and non-zero otherwise */
{
    long i;		/* loop counter */
    long j;		/* loop counter */
    long *set_1;	/* current set in set array 1 */
    long *set_2;	/* current set in set array 2 */

    /* sort the set arrays and their constituent sets */
    sort(oset_1, nels);
    sort(oset_2, nels);

    /* compare the set arrays */
    for (i = 0; i < nels; i++)
    {
	if (oset_1[i].cnt != oset_2[i].cnt)
	    return 1;
	else
	{
	    set_1 = oset_1[i].set;
	    set_2 = oset_2[i].set;
	    for (j = 0; j < oset_1[i].cnt; ++j)
	    {
		if (set_1[j] != set_2[j])
		    return 1;
	    }
	}
    }

    return 0;

} /* end setstcmp() */

static void sort(Objset *const oset, const long nels)
/* sort the nels object sets in oset so that each is in order, and sort oset so
 * that the sets themselves are in order of size and content */
{
    long i;	/* loop counter */

    /* first sort each set member list */
    for (i = 0; i < nels; i++)
	qsort(oset[i].set, (size_t) oset[i].cnt, sizeof(long),
	 objnocmp);

    /* now sort the arrays of sets by size and content */
    for (i = 0; i < nels; i++)
	qsort(oset, (size_t) nels, sizeof(Objset), osetcmp);

} /* end sort() */

static int osetcmp(const void *oset1, const void *oset2)
/* comparison function for object sets (type Objset):
 * return negative if *oset1 is a smaller set of objects than *oset2 or
 * is the same size but with a list of elements that compares lower;
 * return positive if *ostet1 is bigger or the same size but with a
 * list of elements that compares higher; return 0 if they are the
 * same; N.B. the object numbers must be in numerical order within the
 * sets */
{
    long i;						/* loop cntr */
    const Objset loset_1 = *((const Objset *) oset1);	/* typed */
    const Objset loset_2 = *((const Objset *) oset2);	/* typed */

    /* sets of different size differ */
    if (loset_1.cnt < loset_2.cnt)
	return -1;
    else if (loset_1.cnt > loset_2.cnt)
	return +1;

    /* if we reach here, sets are equal size, so we see if sets'
     * contents differ */
    for (i = 0; i < loset_1.cnt; i++)
    {
	if (loset_1.set[i] < loset_2.set[i])
	    return -1;
	else if (loset_1.set[i] > loset_2.set[i])
	    return +1;
    }

    /* if we reach here, really the sets are the same */
    return 0;

} /* end osetcmp() */

static long makesets(const Branch *const tree_1, const long root_1,
 const Branch *const tree_2, const long root_2)
/* fill static sset_1 and static sset_2 with arrays of object sets for
 * tree_1 and tree_2 (of root_1 and root_2 respectively), and return
 * the extent of each array;
 * the trees must have the same object in the root branch;
 * arrays will be overwritten on subsequent calls */
{
    extern Dataptr matrix;		/* data matrix */
    const long nsets = matrix->n - 3;	/* sets per tree */
    const long mssz = matrix->n - 2;	/* maximum objects per set */

    if (sset_1[0].set == NULL)	/* first call, allocate memory */
    {
	ssarralloc(sset_1, nsets, mssz);
	ssarralloc(sset_2, nsets, mssz);
    }
	fillsets(sset_1, tree_1, root_1);
	fillsets(sset_2, tree_2, root_2);
	return nsets;

} /* end makesets() */

static void ssarralloc(Objset *nobjset, const long nsets,
 const long setsize)
/* Fill nobjset[0..nsets-1] with pointers each pointing to newly
 * allocated space for setsize objects; assumes nobjset points to the
 * first element of an array with at least nsets elements. */
{
    long i; 	/* loop counter */

    lvb_assert (nsets <= MAX_SSET_SIZE);

    for (i = 0; i < nsets; i++)
    {
	nobjset[i].set = alloc(setsize * sizeof(long),
	 "object set object arrays");
	nobjset[i].cnt = UNSET;
    }

} /* end ssarralloc() */

static void fillsets(Objset *const sstruct, const Branch *const tree,
 const long root)
/* fill object sets in sstruct with all sets of objects in tree tree,
 * descended from but not including root and not including sets of one
 * object */
{
    extern Dataptr matrix;	/* data matrix */
    static long i = UNSET;	/* current set being filled */

    if (i == UNSET)	/* not a recursive call */
    {
	i = 0;

	/* avoid generating sets for true root and leaves */
	if (tree[root].left >= matrix->n)	/* interior */
	    fillsets(sstruct, tree, tree[root].left);
	if (tree[root].right >= matrix->n)	/* interior */
	    fillsets(sstruct, tree, tree[root].right);

	i = UNSET;	/* clean up for next non-recursive call */
	return;
    }
    if (tree[root].left != UNSET)	/* not leaf */
    {
	getobjs(tree, root, sstruct[i].set, &sstruct[i].cnt);
	i++;
	fillsets(sstruct, tree, tree[root].left);
	fillsets(sstruct, tree, tree[root].right);
	return;
    }

} /* end fillsets */

static void getobjs(const Branch *const barray, const long root,
 long *const objarr, long *const cnt)
/* fill objarr (which must be large enough) with numbers of all objects
 * in the tree in barray in the clade starting at branch root;
 * fill the number pointed to by cnt with the number of objects found
 * (i.e. the number of elements written to objarr) */
{
    *cnt = 0;
    realgetobjs(barray, root, objarr, cnt);

} /* end getobjs() */

static void realgetobjs(const Branch *const barray, const long root,
 long *const objarr, long *const cnt)
/* fill objarr (which must be large enough) with numbers of all objects
 * in the tree in barray in the clade starting at branch root;
 * fill the number pointed to by cnt, which must initially be zero,
 * with the number of objects found (i.e. the number of elements
 * written to objarr); this function should not be called from anywhere
 * except getobjs(), which is a safer interface */
{
    extern Dataptr matrix;	/* data matrix */

    if (root < matrix->n)
    {
	objarr[*cnt] = root;
	++(*cnt);
    }
    else
    {
	if (barray[root].left != UNSET)
	    realgetobjs(barray, barray[root].left, objarr, cnt);
	if (barray[root].right != UNSET)
	    realgetobjs(barray, barray[root].right, objarr, cnt);
    }

} /* end realgetobjs() */

static int objnocmp(const void *o1, const void *o2)
/* comparison function for comparing object numbers:
 * return negative if *o1 < *o2, zero if *o1 == *o2,
 * or positive if *o1 > *o2 */
{
    const long *o1_typed = o1;	/* typed alias of o1 */
    const long *o2_typed = o2;	/* typed alias of o2 */
    
    if (*o1_typed < *o2_typed)
        return -1;
    else if (*o1_typed > *o2_typed)
        return +1;
    else
        return 0;

} /* end objnocmp() */

void ss_init(Branch *tree, unsigned char **enc_mat, long nbranches, long m)
/* copy m states from enc_mat to the stateset arrays for the leaves in tree;
 *  * the nth entry in enc_mat is assumed to be the encoded state sets for object
 *   * no. n in the tree; non-leaf branches in the tree are marked "dirty"; the
 *    * root branch struct is marked "clean" since it is also a terminal */
{
    extern Dataptr matrix;      /* data matrix */
    long i;                     /* loop counter */

    for (i = 0; i < matrix->n; i++)
        memcpy(tree[i].sset, enc_mat[i], m);
    for (i = matrix->n; i < nbranches; i++)
            tree[i].sset[0] = 0U;

} /* end ss_init() */
