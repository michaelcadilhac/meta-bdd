#pragma once

#include <cstring>

class imatrix {
  public:
    imatrix () : rows {NULL}, size {0} { }

    ~imatrix () {
      destroy ();
    }

    void reset (int newsize) {
      if (newsize == size) {
        for (int n=0 ; n<size ; n++)
          std::memset(rows[n], 0, size/8+1);
        return;
      }

      destroy ();

      size = newsize;

      if (size == 0)
        return;

      rows = NEW(char*,size);
      for (int n=0 ; n<size ; n++) {
        rows[n] = NEW(char,size/8+1);
        std::memset(rows[n], 0, size/8+1);
      }
    }

    char **rows;
    int size;

    void     imatrixFPrint(FILE *);
    void     imatrixPrint();
    void     imatrixSet(int,int);
    void     imatrixClr(int,int);
    int      imatrixDepends(int,int) const;

  private:
    void destroy () {
      if (size == 0)
        return;

      for (int n=0 ; n<size ; n++)
        free(rows[n]);
      free(rows);
    }
};


#include "imatrix.hxx"
