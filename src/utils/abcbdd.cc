/* This file is taken from the Simple BDD package of ABC by Alan Mishchenko. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "abcbdd.hh"
namespace utils {
  namespace detail {
    Abc_BddMan* global_bddman;

    Abc_BddMan * Abc_BddManAlloc( int nVars, int nObjs )
    {
      Abc_BddMan * p; int i;
      p = ABC_CALLOC( Abc_BddMan, 1 );
      p->nVars       = nVars;
      p->nObjsAlloc  = nObjs;
      p->nUniqueMask = (1 << Abc_Base2Log(nObjs)) - 1;
      p->nCacheMask  = (1 << Abc_Base2Log(nObjs)) - 1;
      p->pUnique     = ABC_CALLOC( int, p->nUniqueMask + 1 );
      p->pNexts      = ABC_CALLOC( int, p->nObjsAlloc );
      p->pCache      = ABC_CALLOC( int, 3*(p->nCacheMask + 1) );
      p->pObjs       = ABC_CALLOC( int, 2*p->nObjsAlloc );
      p->pMark       = ABC_CALLOC( unsigned char, p->nObjsAlloc );
      p->pVars       = ABC_CALLOC( unsigned char, p->nObjsAlloc );
      p->pVars[0]    = 0xff;
      p->nObjs       = 1;
      for ( i = 0; i < nVars; i++ )
        Abc_BddUniqueCreate( p, i, 1, 0 );
      assert( p->nObjs == nVars + 1 );
      p->nMemory = sizeof(Abc_BddMan)/4 +
        p->nUniqueMask + 1 + p->nObjsAlloc +
        (p->nCacheMask + 1) * 3 * sizeof(int)/4 +
        p->nObjsAlloc * 2 * sizeof(int)/4;
      return p;
    }
    void Abc_BddManFree( Abc_BddMan * p )
    {
      printf( "BDD stats: Var = %d  Obj = %d  Alloc = %d  Hit = %d  Miss = %d  ",
              p->nVars, p->nObjs, p->nObjsAlloc, p->nCacheLookups-p->nCacheMisses, p->nCacheMisses );
      printf( "Mem = %.2f MB\n", 4.0*(int)(p->nMemory/(1<<20)) );
      ABC_FREE( p->pUnique );
      ABC_FREE( p->pNexts );
      ABC_FREE( p->pCache );
      ABC_FREE( p->pObjs );
      ABC_FREE( p->pVars );
      ABC_FREE( p );
    }

    /**Function*************************************************************

     Synopsis    [Boolean AND.]

     Description []

     SideEffects []

     SeeAlso     []

     ***********************************************************************/
    int Abc_BddAnd( Abc_BddMan * p, int a, int b )
    {
      int r0, r1, r;
      if ( a == 0 ) return 0;
      if ( b == 0 ) return 0;
      if ( a == 1 ) return b;
      if ( b == 1 ) return a;
      if ( a == b ) return a;
      if ( a > b )  return Abc_BddAnd( p, b, a );
      if ( (r = Abc_BddCacheLookup(p, a, b)) >= 0 )
        return r;
      if ( Abc_BddVar(p, a) < Abc_BddVar(p, b) )
        r0 = Abc_BddAnd( p, Abc_BddElse(p, a), b ),
          r1 = Abc_BddAnd( p, Abc_BddThen(p, a), b );
      else if ( Abc_BddVar(p, a) > Abc_BddVar(p, b) )
        r0 = Abc_BddAnd( p, a, Abc_BddElse(p, b) ),
          r1 = Abc_BddAnd( p, a, Abc_BddThen(p, b) );
      else // if ( Abc_BddVar(p, a) == Abc_BddVar(p, b) )
        r0 = Abc_BddAnd( p, Abc_BddElse(p, a), Abc_BddElse(p, b) ),
          r1 = Abc_BddAnd( p, Abc_BddThen(p, a), Abc_BddThen(p, b) );
      r = Abc_BddUniqueCreate( p, Abc_MinInt(Abc_BddVar(p, a), Abc_BddVar(p, b)), r1, r0 );
      return Abc_BddCacheInsert( p, a, b, r );
    }

    int Abc_BddOr( Abc_BddMan * p, int a, int b )
    {
      return Abc_LitNot( Abc_BddAnd(p, Abc_LitNot(a), Abc_LitNot(b)) );
    }

    /**Function*************************************************************

     Synopsis    [Printing BDD.]

     Description []

     SideEffects []

     SeeAlso     []

     ***********************************************************************/
    void Abc_BddPrint_rec( Abc_BddMan * p, int a, int * pPath )
    {
      if ( a == 0 )
        return;
      if ( a == 1 )
      {
        int i;
        for ( i = 0; i < p->nVars; i++ )
          if ( pPath[i] == 0 || pPath[i] == 1 )
            printf( "%c%d", pPath[i] ? '+' : '-', i );
        printf( " " );
        return;
      }
      pPath[Abc_BddVar(p, a)] =  0;
      Abc_BddPrint_rec( p, Abc_BddElse(p, a), pPath );
      pPath[Abc_BddVar(p, a)] =  1;
      Abc_BddPrint_rec( p, Abc_BddThen(p, a), pPath );
      pPath[Abc_BddVar(p, a)] = -1;
    }

    void Abc_BddPrint( Abc_BddMan * p, int a )
    {
      int * pPath = ABC_FALLOC( int, p->nVars );
      printf( "BDD %d = ", a );
      Abc_BddPrint_rec( p, a, pPath );
      ABC_FREE( pPath );
      printf( "\n" );
    }
  }
}

int main () {
  using abcbdd = utils::abcbdd;

  abcbdd::init (10, 100);
  auto bVarA = abcbdd::bddVar (0);
  auto bVarB = abcbdd::bddVar (1);
  auto bVarC = abcbdd::bddVar (2);
  auto bVarD = abcbdd::bddVar (3);
  auto bAndAB = bVarA * bVarB;
  auto bAndCD = bVarC * bVarD;
  auto bFunc = bAndAB + bAndCD;
  Abc_BddPrint(utils::d::global_bddman, bFunc.GetBDD ());
}
