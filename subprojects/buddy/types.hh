#pragma once

#define __purefn __attribute__((__pure__))
#define __constfn __attribute__((__const__))
#define __noreturnfn __attribute__((__noreturn__))
#define __likely(expr)   __builtin_expect(!!(expr), 1)
#define __unlikely(expr) __builtin_expect(!!(expr), 0)

#include <stdio.h>
#include <iostream>

/*=== Defined operators for apply calls ================================*/

#define bddop_and       0
#define bddop_xor       1
#define bddop_or        2
#define bddop_nand      3
#define bddop_nor       4
#define bddop_imp       5
#define bddop_biimp     6
#define bddop_diff      7
#define bddop_less      8
#define bddop_invimp    9

/* Should *not* be used in bdd_apply calls !!! */
#define bddop_not      10
#define bddop_simplify 11


/*=== User BDD types ===================================================*/

typedef int BDD;

typedef struct s_bddPair
{
   BDD *result;
   int last;
   int id;
   struct s_bddPair *next;
} bddPair;


/*=== Status information ===============================================*/

/*
NAME    {* bddStat *}
SECTION {* kernel *}
SHORT   {* Status information about the bdd package *}
PROTO   {* typedef struct s_bddStat
{
   long int produced;
   int nodenum;
   int maxnodenum;
   int freenodes;
   int minfreenodes;
   int varnum;
   int cachesize;
   int gbcnum;
} bddStat;  *}
DESCR   {* The fields are \\[\baselineskip] \begin{tabular}{lp{10cm}}
  {\tt produced}     & total number of new nodes ever produced \\
  {\tt nodenum}      & currently allocated number of bdd nodes \\
  {\tt maxnodenum}   & user defined maximum number of bdd nodes \\
  {\tt freenodes}    & number of currently free nodes \\
  {\tt minfreenodes} & minimum number of nodes that should be left after a
		       garbage collection. \\
  {\tt varnum}       & number of defined bdd variables \\
  {\tt cachesize}    & number of entries in the internal caches \\
  {\tt gbcnum}       & number of garbage collections done until now
  \end{tabular} *}
ALSO    {* bdd\_stats *}
*/
typedef struct s_bddStat
{
   long int produced;
   int nodenum;
   int maxnodenum;
   int freenodes;
   int minfreenodes;
   int varnum;
   int cachesize;
   int gbcnum;
} bddStat;


/*
NAME    {* bddGbcStat *}
SECTION {* kernel *}
SHORT   {* Status information about garbage collections *}
PROTO   {* typedef struct s_bddGbcStat
{
   int nodes;
   int freenodes;
   long time;
   long sumtime;
   int num;
} bddGbcStat;  *}
DESCR   {* The fields are \\[\baselineskip] \begin{tabular}{ll}
  {\tt nodes}     & Total number of allocated nodes in the nodetable \\
  {\tt freenodes} & Number of free nodes in the nodetable \\
  {\tt time}      & Time used for garbage collection this time \\
  {\tt sumtime}   & Total time used for garbage collection \\
  {\tt num}       & number of garbage collections done until now
  \end{tabular} *}
ALSO    {* bdd\_gbc\_hook *}
*/
typedef struct s_bddGbcStat
{
   int nodes;
   int freenodes;
   long time;
   long sumtime;
   int num;
} bddGbcStat;


/*
NAME    {* bddCacheStat *}
SECTION {* kernel *}
SHORT   {* Status information about cache usage *}
PROTO   {* typedef struct s_bddCacheStat
{
   long unsigned int uniqueAccess;
   long unsigned int uniqueChain;
   long unsigned int uniqueHit;
   long unsigned int uniqueMiss;
   long unsigned int opHit;
   long unsigned int opMiss;
   long unsigned int swapCount;
} bddCacheStat; *}
DESCR   {* The fields are \\[\baselineskip] \begin{tabular}{ll}
  {\bf Name}         & {\bf Number of } \\
  uniqueAccess & accesses to the unique node table \\
  uniqueChain  & iterations through the cache chains in the unique node table\\
  uniqueHit    & entries actually found in the the unique node table \\
  uniqueMiss   & entries not found in the the unique node table \\
  opHit        & entries found in the operator caches \\
  opMiss       & entries not found in the operator caches \\
  swapCount    & number of variable swaps in reordering \\
\end{tabular} *}
ALSO    {* bdd\_cachestats *}
*/
typedef struct s_bddCacheStat
{
   long unsigned int uniqueAccess;
   long unsigned int uniqueChain;
   long unsigned int uniqueHit;
   long unsigned int uniqueMiss;
   long unsigned int opHit;
   long unsigned int opMiss;
   long unsigned int swapCount;
} bddCacheStat;

/*=== BDD interface prototypes =========================================*/

/*
NAME    {* bdd\_relprod *}
SECTION {* operator *}
SHORT   {* relational product *}
PROTO   {* #define bdd_relprod(a,b,var) bdd_appex(a,b,bddop_and,var) *}
DESCR   {* Calculates the relational product of {\tt a} and {\tt b} as
	   {\tt a AND b} with the variables in {\tt var} quantified out
	   afterwards. *}
RETURN  {* The relational product or {\tt bddfalse} on errors. *}
ALSO    {* bdd\_appex *}
*/
#define bdd_relprod(a,b,var) bdd_appex((a),(b),bddop_and,(var))


/////////////////////////////////////////////////////////////////////// bdd tree

typedef struct s_BddTree
{
   int first, last;  /* First and last variable in this block */
   int pos;          /* Sifting position */
   int *seq;         /* Sequence of first...last in the current order */
   char fixed;       /* Are the sub-blocks fixed or may they be reordered */
   int id;           /* A sequential id number given by addblock */
   struct s_BddTree *next, *prev;
   struct s_BddTree *nextlevel;
} BddTree;

/////////////////////////////////////////////////////////////////////////// bvec

/*
NAME    {* bvec *}
SECTION {* bvec *}
SHORT   {* A boolean vector *}
PROTO   {* typedef struct s_bvec
{
   int bitnum;
   BDD *bitvec;
} BVEC;

typedef BVEC bvec; *}
DESCR   {* This data structure is used to store boolean vectors. The field
           {\tt bitnum} is the number of elements in the vector and the
	   field {\tt bitvec} contains the actual BDDs in the vector.
	   The C++ version of {\tt bvec} is documented at the beginning of
	   this document *}
*/
typedef struct s_bvec
{
   int bitnum;
   BDD *bitvec;
} BVEC;

typedef BVEC bvec;

////////////////////////////////////////////////////////////////////////// cache

typedef union
{
  struct {
    int a;
    int c;
    int b;
    int res;
  } i;
  struct {
    int a;
    int c;
    double res;
  } d;
} BddCacheData;

typedef struct
{
   BddCacheData *table;
   int tablesize;
} BddCache;

#define BddCache_lookup(cache, hash) (&(cache)->table[hash % (cache)->tablesize])

//////////////////////////////////////////////////////////////////////////// fdd

/*======================================================================*/
/* NOTE: ALL FDD operations works with LSB in top of the variable order */
/*       and in index zero of the domain tables                         */
/*======================================================================*/

typedef struct s_Domain
{
   int realsize;   /* The specified domain (0...N-1) */
   int binsize;    /* The number of BDD variables representing the domain */
   int *ivar;      /* Variable indeces for the variable set */
   BDD var;        /* The BDD variable set */
} Domain;


//  Handler types
typedef void (*bddstrmhandler)(std::ostream &, int);
typedef void (*bddfilehandler)(FILE *, int);

typedef void (*bddinthandler)(int);
typedef void (*bddgbchandler)(int,bddGbcStat*);
typedef void (*bdd2inthandler)(int,int);
typedef int  (*bddsizehandler)(void);
typedef void (*bddfilehandler)(FILE *, int);
typedef void (*bddallsathandler)(char*, int);


/*=== Reordering algorithms ============================================*/

#define BDD_REORDER_NONE     0
#define BDD_REORDER_WIN2     1
#define BDD_REORDER_WIN2ITE  2
#define BDD_REORDER_SIFT     3
#define BDD_REORDER_SIFTITE  4
#define BDD_REORDER_WIN3     5
#define BDD_REORDER_WIN3ITE  6
#define BDD_REORDER_RANDOM   7

#define BDD_REORDER_FREE     0
#define BDD_REORDER_FIXED    1


/*=== Error codes ======================================================*/

#define BDD_MEMORY (-1)   /* Out of memory */
#define BDD_VAR (-2)      /* Unknown variable */
#define BDD_RANGE (-3)    /* Variable value out of range (not in domain) */
#define BDD_DEREF (-4)    /* Removing external reference to unknown node */
#define BDD_RUNNING (-5)  /* Called bdd_init() twice whithout bdd_done() */
#define BDD_FILE (-6)     /* Some file operation failed */
#define BDD_FORMAT (-7)   /* Incorrect file format */
#define BDD_ORDER (-8)    /* Vars. not in order for vector based functions */
#define BDD_BREAK (-9)    /* User called break */
#define BDD_VARNUM (-10)  /* Different number of vars. for vector pair */
#define BDD_NODES (-11)   /* Tried to set max. number of nodes to be fewer */
			  /* than there already has been allocated */
#define BDD_OP (-12)      /* Unknown operator */
#define BDD_VARSET (-13)  /* Illegal variable set */
#define BDD_VARBLK (-14)  /* Bad variable block operation */
#define BDD_DECVNUM (-15) /* Trying to decrease the number of variables */
#define BDD_REPLACE (-16) /* Replacing to already existing variables */
#define BDD_NODENUM (-17) /* Number of nodes reached user defined maximum */
#define BDD_ILLBDD (-18)  /* Illegal bdd argument */
#define BDD_SIZE (-19)    /* Illegal size argument */

#define BVEC_SIZE (-20)    /* Mismatch in bitvector size */
#define BVEC_SHIFT (-21)   /* Illegal shift-left/right parameter */
#define BVEC_DIVZERO (-22) /* Division by zero */

#define BDD_INVMERGE (-23) /* Merging clashing rewriting rules */

#define BDD_ERRNUM 25
