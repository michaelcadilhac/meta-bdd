/* This file is taken from the Simple BDD package of ABC by Alan Mishchenko. */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
namespace utils {
  namespace detail {

    typedef struct Abc_BddMan_ Abc_BddMan;
    struct Abc_BddMan_
    {
        int                nVars;         // the number of variables
        int                nObjs;         // the number of nodes used
        int                nObjsAlloc;    // the number of nodes allocated
        int *              pUnique;       // unique table for nodes
        int *              pNexts;        // next pointer for nodes
        int *              pCache;        // array of triples <arg0, arg1, AND(arg0, arg1)>
        int *              pObjs;         // array of pairs <cof0, cof1> for each node
        unsigned char *    pVars;         // array of variables for each node
        unsigned char *    pMark;         // array of marks for each BDD node
        unsigned           nUniqueMask;   // selection mask for unique table
        unsigned           nCacheMask;    // selection mask for computed table
        int                nCacheLookups; // the number of computed table lookups
        int                nCacheMisses;  // the number of computed table misses
        int                nMemory;       // total amount of memory used (in bytes)
    };

    extern Abc_BddMan* global_bddman;

    static inline int      Abc_Lit2Var( int Lit )                 { assert(Lit >= 0); return Lit >> 1;                          }
    static inline int      Abc_LitRegular( int Lit )              { assert(Lit >= 0); return Lit & ~01;                         }
    static inline int      Abc_LitIsCompl( int Lit )              { assert(Lit >= 0); return Lit & 1;                           }
    static inline int      Abc_LitNotCond( int Lit, int c )       { assert(Lit >= 0); return Lit ^ (int)(c > 0);                }
    static inline int      Abc_LitNot( int Lit )                  { assert(Lit >= 0); return Lit ^ 1;                           }
#define ABC_CALLOC(type, num)    ((type *) calloc((num), sizeof(type)))
#define ABC_FREE(obj)            ((obj) ? (free((char *) (obj)), (obj) = 0) : 0)
#define ABC_FALLOC(type, num)    ((type *) memset(malloc(sizeof(type) * (num)), 0xff, sizeof(type) * (num)))

    static inline int      Abc_MinInt( int a, int b )             { return a < b ?  a : b; }

    static inline int      Abc_Base2Log( unsigned n )             { int r; if ( n < 2 ) return n; for ( r = 0, n--; n; n >>= 1, r++ ) {}; return r; }


    static inline int      Abc_Var2Lit( int Var, int c )          { assert(Var >= 0 && !(c >> 1)); return Var + Var + c;        }

    static inline int      Abc_BddIthVar( int i )                        { return Abc_Var2Lit(i + 1, 0);                            }
    static inline unsigned Abc_BddHash( int Arg0, int Arg1, int Arg2 )   { return 12582917 * Arg0 + 4256249 * Arg1 + 741457 * Arg2; }

    static inline int      Abc_BddVar( Abc_BddMan * p, int i )           { return (int)p->pVars[Abc_Lit2Var(i)];                    }
    static inline int      Abc_BddThen( Abc_BddMan * p, int i )          { return Abc_LitNotCond(p->pObjs[Abc_LitRegular(i)], Abc_LitIsCompl(i));    }
    static inline int      Abc_BddElse( Abc_BddMan * p, int i )          { return Abc_LitNotCond(p->pObjs[Abc_LitRegular(i)+1], Abc_LitIsCompl(i));  }

    static inline int      Abc_BddMark( Abc_BddMan * p, int i )          { return (int)p->pMark[Abc_Lit2Var(i)];                    }
    static inline void     Abc_BddSetMark( Abc_BddMan * p, int i, int m ){ p->pMark[Abc_Lit2Var(i)] = m;                            }

    ////////////////////////////////////////////////////////////////////////
    ///                     FUNCTION DEFINITIONS                         ///
    ////////////////////////////////////////////////////////////////////////

    /**Function*************************************************************

     Synopsis    [Creating new node.]

     Description []

     SideEffects []

     SeeAlso     []

     ***********************************************************************/
    static inline int Abc_BddUniqueCreateInt( Abc_BddMan * p, int Var, int Then, int Else )
    {
      int *q = p->pUnique + (Abc_BddHash(Var, Then, Else) & p->nUniqueMask);
      for ( ; *q; q = p->pNexts + *q )
        if ( (int)p->pVars[*q] == Var && p->pObjs[*q+*q] == Then && p->pObjs[*q+*q+1] == Else )
          return Abc_Var2Lit(*q, 0);
      if ( p->nObjs == p->nObjsAlloc )
        printf( "Aborting because the number of nodes exceeded %d.\n", p->nObjsAlloc ), fflush(stdout);
      assert( p->nObjs < p->nObjsAlloc );
      *q = p->nObjs++;
      p->pVars[*q] = Var;
      p->pObjs[*q+*q] = Then;
      p->pObjs[*q+*q+1] = Else;
      //    printf( "Added node %3d: Var = %3d.  Then = %3d.  Else = %3d\n", *q, Var, Then, Else );
      assert( !Abc_LitIsCompl(Else) );
      return Abc_Var2Lit(*q, 0);
    }
    static inline int Abc_BddUniqueCreate( Abc_BddMan * p, int Var, int Then, int Else )
    {
      assert( Var >= 0 && Var < p->nVars );
      assert( Var < Abc_BddVar(p, Then)  );
      assert( Var < Abc_BddVar(p, Else) );
      if ( Then == Else )
        return Else;
      if ( !Abc_LitIsCompl(Else) )
        return Abc_BddUniqueCreateInt( p, Var, Then, Else );
      return Abc_LitNot( Abc_BddUniqueCreateInt(p, Var, Abc_LitNot(Then), Abc_LitNot(Else)) );
    }

    /**Function*************************************************************

     Synopsis    []

     Description []

     SideEffects []

     SeeAlso     []

     ***********************************************************************/
    static inline int Abc_BddCacheLookup( Abc_BddMan * p, int Arg1, int Arg2 )
    {
      int * pEnt = p->pCache + 3*(Abc_BddHash(0, Arg1, Arg2) & p->nCacheMask);
      p->nCacheLookups++;
      return (pEnt[0] == Arg1 && pEnt[1] == Arg2) ? pEnt[2] : -1;
    }
    static inline int Abc_BddCacheInsert( Abc_BddMan * p, int Arg1, int Arg2, int Res )
    {
      int * pEnt = p->pCache + 3*(Abc_BddHash(0, Arg1, Arg2) & p->nCacheMask);
      pEnt[0] = Arg1;  pEnt[1] = Arg2;  pEnt[2] = Res;
      p->nCacheMisses++;
      assert( Res >= 0 );
      return Res;
    }

    Abc_BddMan * Abc_BddManAlloc( int nVars, int nObjs );
    void Abc_BddManFree( Abc_BddMan * p );
    int Abc_BddAnd( Abc_BddMan * p, int a, int b );
    int Abc_BddOr( Abc_BddMan * p, int a, int b );
    void Abc_BddPrint( Abc_BddMan * p, int a );
  }
}
