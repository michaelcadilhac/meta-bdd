/*************************************************************************
  $Header: /Volumes/CVS/repository/spot/spot/buddy/src/tree.c,v 1.2 2003/05/05 13:45:08 aduret Exp $
  FILE:  tree.c
  DESCR: Trees for BDD variables
  AUTH:  Jorn Lind
  DATE:  (C) march 1998
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>

void masterbdd::update_seq(BddTree *t)
{
   int n;
   int low = t->first;

   for (n=t->first ; n<=t->last ; n++)
      if (bddvar2level[n] < bddvar2level[low])
	 low = n;

   for (n=t->first ; n<=t->last ; n++)
      t->seq[bddvar2level[n]-bddvar2level[low]] = n;
}


BddTree *masterbdd::bddtree_new(int id)
{
   BddTree *t = NEW(BddTree,1);
   if (t == NULL)
      return NULL;

   t->first = t->last = -1;
   t->fixed = 1;
   t->next = t->prev = t->nextlevel = NULL;
   t->seq = NULL;
   t->id = id;
   return t;
}


void masterbdd::bddtree_del(BddTree *t)
{
   if (t == NULL)
      return;

   bddtree_del(t->nextlevel);
   bddtree_del(t->next);
   if (t->seq != NULL)
      free(t->seq);
   free(t);
}


BddTree *masterbdd::bddtree_addrange_rec(BddTree *t, BddTree *prev,
                                         int first, int last, int fixed, int id)
{
   if (first < 0  ||  last < 0  ||  last < first)
      return NULL;

      /* Empty tree -> build one */
   if (t == NULL)
   {
      if ((t=bddtree_new(id)) == NULL)
	 return NULL;
      t->first = first;
      t->fixed = fixed;
      t->seq = NEW(int,last-first+1);
      t->last = last;
      update_seq(t);
      t->prev = prev;
      return t;
   }

      /* Check for identity */
   if (first == t->first  &&  last == t->last)
      return t;

      /* Before this section -> insert */
   if (last < t->first)
   {
      BddTree *tnew = bddtree_new(id);
      if (tnew == NULL)
	 return NULL;
      tnew->first = first;
      tnew->last = last;
      tnew->fixed = fixed;
      tnew->seq = NEW(int,last-first+1);
      update_seq(tnew);
      tnew->next = t;
      tnew->prev = t->prev;
      t->prev = tnew;
      return tnew;
   }

      /* After this this section -> go to next */
   if (first > t->last)
   {
      t->next = bddtree_addrange_rec(t->next, t, first, last, fixed, id);
      return t;
   }

      /* Inside this section -> insert in next level */
   if (first >= t->first  &&  last <= t->last)
   {
      t->nextlevel =
	 bddtree_addrange_rec(t->nextlevel,NULL,first,last,fixed,id);
      return t;
   }

      /* Covering this section -> insert above this level */
   if (first <= t->first)
   {
      BddTree *tnew;
      BddTree *cur = t;

      while (1)
      {
	    /* Partial cover ->error */
	 if (last >= cur->first  &&  last < cur->last)
	    return NULL;

	 if (cur->next == NULL  ||  last < cur->next->first)
	 {
	    tnew = bddtree_new(id);
	    if (tnew == NULL)
	       return NULL;
	    tnew->first = first;
	    tnew->last = last;
	    tnew->fixed = fixed;
	    tnew->seq = NEW(int,last-first+1);
	    update_seq(tnew);
	    tnew->nextlevel = t;
	    tnew->next = cur->next;
	    tnew->prev = t->prev;
	    if (cur->next != NULL)
	       cur->next->prev = tnew;
	    cur->next = NULL;
	    t->prev = NULL;
	    return tnew;
	 }

	 cur = cur->next;
      }

   }

   return NULL;
}


BddTree *masterbdd::bddtree_addrange(BddTree *t, int first, int last, int fixed,int id)
{
   return bddtree_addrange_rec(t,NULL,first,last,fixed,id);
}
