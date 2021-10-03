/*************************************************************************
  $Header: /Volumes/CVS/repository/spot/spot/buddy/src/cache.c,v 1.2 2003/05/05 13:45:05 aduret Exp $
  FILE:  cache.c
  DESCR: Cache class for caching apply/exist etc. results in BDD package
  AUTH:  Jorn Lind
  DATE:  (C) june 1997
*************************************************************************/
#include <stdlib.h>
#include <string.h>

void masterbdd::BddCache_reset(BddCache *cache)
{
  int n;
  for (n = 0; n < cache->tablesize; n++)
    cache->table[n].i.a = -1;
}

int masterbdd::BddCache_init(BddCache *cache, int size)
{
   size = bdd_prime_gte(size);

   if ((cache->table=NEW(BddCacheData,size)) == NULL)
      return bdd_error(BDD_MEMORY);

   cache->tablesize = size;
   BddCache_reset(cache);
   return 0;
}


void masterbdd::BddCache_done(BddCache *cache)
{
   free(cache->table);
   cache->table = NULL;
   cache->tablesize = 0;
}


int masterbdd::BddCache_resize(BddCache *cache, int newsize)
{
   free(cache->table);
   return BddCache_init(cache, newsize);
}



/* EOF */
