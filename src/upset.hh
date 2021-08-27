#pragma once

#include <meta_bdd.hh>

class upset {
    using Bdd = sylvan::Bdd;

    using value_type = size_t;
  public:
    upset (std::span<const value_type> v) : mbdd {MBDD::full ()}, dim {v.size ()} {
      for (size_t i = 0; i < dim; ++i)
        mbdd = mbdd & up_mbdd_high_branch (v[i], i);
    }

    upset (std::initializer_list<value_type> v) : upset (std::span (v)) {}

    upset operator+ (std::span<const value_type> sv) const {

    }

    bool contains (std::span<const value_type> sv) const {
      // Make a copy as we're going to shift all these values.
      std::vector<value_type> v (sv.begin (), sv.end ());
      std::vector<Bdd> w;

      assert (v.size () == dim);
      bool is_zero = false;
      while (not is_zero) {
        is_zero = true;
        Bdd bits = Bdd::bddOne ();
        for (size_t i = 0; i < dim; ++i) {
          bits *= (v[i] % 2) ? Bdd::bddVar (i) : !Bdd::bddVar (i);
          if ((v[i] >>= 1))
            is_zero = false;
        }
        w.push_back (bits);
      }
      return mbdd.accepts (w);
    }

    bool contains (std::initializer_list<value_type> v) const {
      return contains (std::span (v));
    }

    MBDD::meta_bdd get_mbdd () const {
      return mbdd;
    }

  private:
    MBDD::meta_bdd mbdd;
    const size_t dim;

    MBDD::meta_bdd up_mbdd_high_branch (value_type value, size_t dim) {
      if (value == 0)
        return MBDD::full ();
      if (value % 2 == 1)
        return MBDD::make (Bdd::bddVar (dim) * up_mbdd_high_branch (value >> 1, dim) +
                           !Bdd::bddVar (dim) * up_mbdd_low_branch (value >> 1, dim), false);
      return MBDD::make (up_mbdd_high_branch (value >> 1, dim), false);
    }

    MBDD::meta_bdd up_mbdd_low_branch (value_type value, size_t dim) {
      if (value == 0)
        return MBDD::make (!Bdd::bddVar (dim) * MBDD::self () +
                           Bdd::bddVar (dim) * MBDD::full (), false);
      if (value % 2 == 0)
        return MBDD::make (Bdd::bddVar (dim) * up_mbdd_high_branch (value >> 1, dim) +
                           !Bdd::bddVar (dim) * up_mbdd_low_branch (value >> 1, dim), false);
      return MBDD::make (up_mbdd_low_branch (value >> 1, dim), false);
    }
};