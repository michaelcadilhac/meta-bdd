/*========================================================================
               Copyright (C) 1996-2002 by Jorn Lind-Nielsen
                            All rights reserved

    Permission is hereby granted, without written agreement and without
    license or royalty fees, to use, reproduce, prepare derivative
    works, distribute, and display this software and its documentation
    for any purpose, provided that (1) the above copyright notice and
    the following two paragraphs appear in all copies of the source code
    and (2) redistributions, including without limitation binaries,
    reproduce these notices in the supporting documentation. Substantial
    modifications to this software may be copyrighted by their authors
    and need not follow the licensing terms described here, provided
    that the new terms are clearly indicated in all files where they apply.

    IN NO EVENT SHALL JORN LIND-NIELSEN, OR DISTRIBUTORS OF THIS
    SOFTWARE BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
    INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS
    SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE AUTHORS OR ANY OF THE
    ABOVE PARTIES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    JORN LIND-NIELSEN SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
    BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
    FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS
    ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO
    OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
    MODIFICATIONS.
========================================================================*/

/*************************************************************************
  FILE:  imatrix.cc
  DESCR: Interaction matrix
  AUTH:  Jorn Lind
  DATE:  (C) february 2000
*************************************************************************/
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void imatrix::imatrixFPrint(FILE *ofile)
{
   int x,y;

   fprintf(ofile, "    ");
   for (x=0 ; x<this->size ; x++)
      fprintf(ofile, "%c", x < 26 ? (x+'a') : (x-26)+'A');
   fprintf(ofile, "\n");

   for (y=0 ; y<this->size ; y++)
   {
      fprintf(ofile, "%2d %c", y, y < 26 ? (y+'a') : (y-26)+'A');
      for (x=0 ; x<this->size ; x++)
	 fprintf(ofile, "%c", imatrixDepends(y,x) ? 'x' : ' ');
      fprintf(ofile, "\n");
   }
}


void imatrix::imatrixPrint()
{
   imatrixFPrint(stdout);
}


void imatrix::imatrixSet(int a, int b)
{
   this->rows[a][b/8] |= 1<<(b%8);
}


void imatrix::imatrixClr(int a, int b)
{
   this->rows[a][b/8] &= ~(1<<(b%8));
}


int imatrix::imatrixDepends(int a, int b) const
{
   return this->rows[a][b/8] & (1<<(b%8));
}
