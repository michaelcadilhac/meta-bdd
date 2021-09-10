#pragma once

#include <meta_bdd.hh>
#include <cache.hh>

class upset {
    using Bdd = sylvan::Bdd;

    using value_type = size_t;
  private:
    upset (const MBDD::meta_bdd& m, size_t d) : mbdd {m}, dim {d} {}

  public:
    upset (std::span<const value_type> v) : mbdd {MBDD::full ()}, dim {v.size ()} {
      for (size_t i = 0; i < dim; ++i)
        mbdd = mbdd & up_mbdd_high_branch (v[i], i);
    }

    upset (std::initializer_list<value_type> v) : upset (std::span (v)) {}

    upset operator& (const upset& other) const {
      assert (dim == other.dim);
      return upset (mbdd & other.mbdd, dim);
    }

    upset& operator&= (const upset& other) { return ((*this) = (*this) & other); }

    upset operator| (const upset& other) const {
      assert (dim == other.dim);
      return upset (mbdd | other.mbdd, dim);
    }

    upset& operator|= (const upset& other) { return ((*this) = (*this) | other); }

    upset operator^ (std::span<const ssize_t> v) const {
      std::vector<size_t> abs_v (v.size ());
      std::vector<bool>   neg (v.size ());
      size_t i = 0;
      for (auto x : v) {
        abs_v[i] = std::abs (x);
        neg[i] = (x < 0);
        ++i;
      }

      std::cout << "Computing transduction..." << std::endl;
      auto trans = plus_transducer (abs_v, neg, std::vector<bool> (v.size ()));
      std::cout << "Transducting..." << std::endl;
      auto tctd = transduct (trans);
      std::cout << "Zero padding..." << std::endl;
      auto pdd = tctd.full_zero_padded ();
      std::cout << "Done." << std::endl;
      return pdd;
    }

    upset operator+ (std::span<const ssize_t> v) const {
      auto new_upset = *this;

      for (size_t i = 0; i < dim; ++i) {
        static auto self_trans_cache = MBDD::make_cache<Bdd> (dim, i);
        auto st_cached = self_trans_cache.get (dim, i);
        auto untouched = Bdd::bddOne ();
        if (st_cached)
          untouched = *st_cached;
        else {
          for (size_t idx = 0; idx < dim; ++idx) {
            if (i == idx)
              continue;
            auto var = Bdd::bddVar (2 * idx);
            auto var_mapped = Bdd::bddVar (2 * idx + 1);
            untouched *= !(var ^ var_mapped);
          }
          self_trans_cache (untouched, dim, i);
        }
        std::cout << "Computing transduction..." << std::endl;
        auto trans = plus_transducer_one_dim (i, dim, std::abs (v[i]), (v[i] < 0), 0, untouched);
        std::cout << "Transducting..." << std::endl;
        new_upset = new_upset.transduct (trans);
        std::cout << "Done dimension " << i << std::endl;
      }
      std::cout << "Zero padding..." << std::endl;
      auto pdd = new_upset.full_zero_padded ();
      std::cout << "Done." << std::endl;
      return pdd;
   }


    upset& operator+= (std::span<const ssize_t> v) { return ((*this) = (*this) + v); }
    upset& operator+= (std::initializer_list<ssize_t> v) { return (operator+=) (std::span (v)); }

    bool operator== (const upset& other) const = default;

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
          bits *= (v[i] % 2) ? Bdd::bddVar (2 * i) : !Bdd::bddVar (2 * i);
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

    bool is_full () const { return (mbdd == MBDD::full ()); }
    bool is_empty () const { return (mbdd == MBDD::empty ()); }

    MBDD::meta_bdd get_mbdd () const {
      return mbdd;
    }

  private:
    MBDD::meta_bdd mbdd;
    size_t dim;

    MBDD::meta_bdd up_mbdd_high_branch (value_type value, size_t dim) {
      if (value == 0)
        return MBDD::full ();
      if (value % 2 == 1)
        return MBDD::make (Bdd::bddVar (2 * dim) * up_mbdd_high_branch (value >> 1, dim) +
                           !Bdd::bddVar (2 * dim) * up_mbdd_low_branch (value >> 1, dim), false);
      return MBDD::make (up_mbdd_high_branch (value >> 1, dim), false);
    }

    MBDD::meta_bdd up_mbdd_low_branch (value_type value, size_t dim) {
      if (value == 0)
        return MBDD::make (!Bdd::bddVar (2 * dim) * MBDD::self () +
                           Bdd::bddVar (2 * dim) * MBDD::full (), false);
      if (value % 2 == 0)
        return MBDD::make (Bdd::bddVar (2 * dim) * up_mbdd_high_branch (value >> 1, dim) +
                           !Bdd::bddVar (2 * dim) * up_mbdd_low_branch (value >> 1, dim), false);
      return MBDD::make (up_mbdd_low_branch (value >> 1, dim), false);
    }

    static MBDD::meta_bdd full_zero_padded (const MBDD::meta_bdd& s, Bdd all_zero);

    upset full_zero_padded () const {
      Bdd all_zero = Bdd::bddOne ();

      for (size_t i = 0; i < dim; ++i)
        all_zero *= !Bdd::bddVar (2 * i);

      return upset (full_zero_padded (mbdd, all_zero), dim);
    }

    static MBDD::meta_bdd plus_transducer_one_dim (size_t idx, size_t dim,
                                                size_t delta,
                                                bool neg, bool carry,
                                                Bdd untouched_components);

    static MBDD::meta_bdd plus_transducer (const std::vector<size_t>& delta,
                                           const std::vector<bool>& neg,
                                           const std::vector<bool>& carries);

    upset transduct (const MBDD::meta_bdd& trans) const {
      std::vector<Bdd> ins (dim), outs (dim);
      for (size_t i = 0; i < dim; ++i) {
        ins[i] = Bdd::bddVar (2 * i);
        outs[i] = Bdd::bddVar (2 * i + 1);
      }

      return upset (mbdd.transduct (trans, outs, ins), dim);
    }
};
