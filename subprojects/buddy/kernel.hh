/*************************************************************************
  $Header: /Volumes/CVS/repository/spot/spot/buddy/src/kernel.h,v 1.5 2004/06/28 15:22:13 adl Exp $
  FILE:  kernel.h
  DESCR: Kernel specific definitions for BDD package
  AUTH:  Jorn Lind
  DATE:  (C) june 1997
*************************************************************************/

#pragma once

/*=== Includes =========================================================*/

#include <limits.h>
#include <setjmp.h>

#ifdef NDEBUG
#define CHECK(r) (void)(r);
#define CHECKa(r,a) (void)(r); (void)(a);
#define CHECKn(r) (void)(r);
#define CHECKnc(r) (void)(r);
#else
   /* Sanity check argument and return eventual error code */
#define CHECK(r)\
   if (!bddrunning) return bdd_error(BDD_RUNNING);\
   else if ((r) < 0  ||  (r) >= bddnodesize) return bdd_error(BDD_ILLBDD);\
   else if (r >= 2 && LOW(r) == -1) return bdd_error(BDD_ILLBDD)\

   /* Sanity check argument and return eventually the argument 'a' */
#define CHECKa(r,a)\
   if (!bddrunning) { bdd_error(BDD_RUNNING); return (a); }\
   else if ((r) < 0  ||  (r) >= bddnodesize)\
     { bdd_error(BDD_ILLBDD); return (a); }\
   else if (r >= 2 && LOW(r) == -1)\
     { bdd_error(BDD_ILLBDD); return (a); }

#define CHECKn(r)\
   if (!bddrunning) { bdd_error(BDD_RUNNING); return; }\
   else if ((r) < 0  ||  (r) >= bddnodesize)\
     { bdd_error(BDD_ILLBDD); return; }\
   else if (r >= 2 && LOW(r) == -1)\
     { bdd_error(BDD_ILLBDD); return; }

   /* r is non-constant */
#define CHECKnc(r)\
   if (root < 2) \
      return bdd_error(BDD_ILLBDD)

#endif

/*=== SEMI-INTERNAL TYPES ==============================================*/

typedef struct s_BddNode /* Node table entry */
{
   unsigned int refcou : 10;
   unsigned int level  : 22;
   int low;
   int high;
   int next;
} BddNode;


/*=== KERNEL DEFINITIONS ===============================================*/

#define MAXVAR 0x1FFFFF
#define MAXREF 0x3FF

   /* Reference counting */
#define DECREF(n) if (bddnodes[n].refcou!=MAXREF && bddnodes[n].refcou>0) bddnodes[n].refcou--
#define INCREF(n) if (bddnodes[n].refcou<MAXREF) bddnodes[n].refcou++
#define DECREFp(n) if (n->refcou!=MAXREF && n->refcou>0) n->refcou--
#define INCREFp(n) if (n->refcou<MAXREF) n->refcou++
#define HASREF(n) (bddnodes[n].refcou > 0)

   /* Marking BDD nodes */
#define MARKON   0x200000    /* Bit used to mark a node (1) */
#define MARKOFF  0x1FFFFF    /* - unmark */
#define MARKHIDE 0x1FFFFF
#define SETMARK(n)  (bddnodes[n].level |= MARKON)
#define UNMARK(n)   (bddnodes[n].level &= MARKOFF)
#define MARKED(n)   (bddnodes[n].level & MARKON)
#define SETMARKp(p) (node->level |= MARKON)
#define UNMARKp(p)  (node->level &= MARKOFF)
#define MARKEDp(p)  (node->level & MARKON)

   /* Hashfunctions */

#define PAIR(a,b)     (((unsigned)a)+((unsigned)b)*12582917U)
#define TRIPLE(a,b,c) (((unsigned)a)+((unsigned)b)*12582917U+((unsigned)c)*4256249U)

   /* Inspection of BDD nodes */
#define ISCONST(a) ((a) < 2)
#define ISNONCONST(a) ((a) >= 2)
#define ISONE(a)   ((a) == 1)
#define ISZERO(a)  ((a) == 0)
#define LEVEL(a)   (bddnodes[a].level)
#define LOW(a)     (bddnodes[a].low)
#define HIGH(a)    (bddnodes[a].high)
#define LEVELp(p)   ((p)->level)
#define LOWp(p)     ((p)->low)
#define HIGHp(p)    ((p)->high)

   /* Stacking for garbage collector */
#define INITREF    bddrefstacktop = bddrefstack
/*
** The following (original) definition of PUSHREF is incorrect
** when (a) can trigger a garbage collect:
**
**   #define PUSHREF(a) *(bddrefstacktop++) = (a)
**
** The reason is that bddrefstacktop can be incremented before computing
** (a) and assigning its value to bddrefstacktop[-1].  So if the garbage
** collector is triggered during the computation of (a), it will find
** uninitialized values in the stack.
**
** Such a situation occur in most algorithms of the form:
**
**   static int restrict_rec(int r) {
**     // ...
**     PUSHREF( restrict_rec(LOW(r)) );
**     PUSHREF( restrict_rec(HIGH(r)) );
**     res = bdd_makenode(LEVEL(r), READREF(2), READREF(1));
**     POPREF(2);
**
** Using a function forces the value of a to be computed before pushing
** it to the stack.
*/
#define PUSHREF(a) ([&] () { int x = (a); return (*(bddrefstacktop++) = x); }) ()
#define READREF(a) *(bddrefstacktop-(a))
#define POPREF(a)  bddrefstacktop -= (a)

#define BDDONE 1
#define BDDZERO 0

#ifndef CLOCKS_PER_SEC
  /* Pass `CPPFLAGS=-DDEFAULT_CLOCK=1000' as an argument to ./configure
     to override this setting.  */
# ifndef DEFAULT_CLOCK
#  define DEFAULT_CLOCK 60
# endif
# define CLOCKS_PER_SEC DEFAULT_CLOCK
#endif

#define DEFAULTMAXNODEINC 50000

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define NEW(t,n) ( (t*)malloc(sizeof(t)*(n)) )
