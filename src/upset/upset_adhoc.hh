#pragma once

#include <upset.hh>

#include <meta_bdd_states_are_bddvars/meta_bdd.hh>
#include <meta_bdd_states_are_ints/meta_bdd.hh>

#include <utils/cache.hh>
#include <utils/transduct_bdd.hh>

#include <labels/sylvanbdd.hh>

#include <sylvan.h>
#include <sylvan_obj.hpp>

namespace upset {
  // Sylvan requires that.
  using namespace sylvan;

  template <typename Bdd, typename StateType>
  class upset_adhoc<MBDD::master_meta_bdd<Bdd, StateType>> {
      // Make sure we're dealing with a Bdd implementation
      using has_bddone = decltype (Bdd::bddOne ());
      using master_meta_bdd = MBDD::master_meta_bdd<Bdd, StateType>;
      using meta_bdd = master_meta_bdd::meta_bdd;
      using transition_type = master_meta_bdd::transition_type;
      using letter_type = Bdd::letter_type;
    public:
      // This must be signed since plus_transducers can decrement values.
      using value_type = ssize_t;
    private:
      upset_adhoc (master_meta_bdd& mmbdd, const meta_bdd& m, size_t d)
        : mmbdd {mmbdd}, mbdd {m}, dim {d} {}

    public:
      upset_adhoc (master_meta_bdd& mmbdd, const std::vector<value_type>& v);

      upset_adhoc (master_meta_bdd& mmbdd,
             std::initializer_list<value_type> v) :
        upset_adhoc (mmbdd, std::vector (v)) {}

      bool operator== (const upset_adhoc& other) const {
        // Don't compare oranges and apples
        assert (&mmbdd == &other.mmbdd and dim == other.dim);
        return mbdd == other.mbdd;
      }

      upset_adhoc operator& (const upset_adhoc& other) const {
        assert (dim == other.dim and &mmbdd == &other.mmbdd);
        return upset_adhoc (mmbdd, mbdd & other.mbdd, dim);
      }
      upset_adhoc operator& (const std::vector<value_type>& v) const {
        return (*this) & upset_adhoc (mmbdd, v);
      }

      upset_adhoc& operator&= (const upset_adhoc& other) { return ((*this) = (*this) & other); }
      upset_adhoc& operator&= (std::initializer_list<value_type> v) { return ((*this) &= upset_adhoc (mmbdd, v)); }
      upset_adhoc& operator&= (const std::vector<value_type>& v) { return ((*this) &= upset_adhoc (mmbdd, v)); }

      upset_adhoc operator| (const upset_adhoc& other) const {
        assert (dim == other.dim and &mmbdd == &other.mmbdd);
        return upset_adhoc (mmbdd, mbdd | other.mbdd, dim);
      }
      upset_adhoc operator| (const std::vector<value_type>& v) const {
        return (*this) | upset_adhoc (mmbdd, v);
      }

      upset_adhoc& operator|= (const upset_adhoc& other) { return ((*this) = (*this) | other); }
      upset_adhoc& operator|= (std::initializer_list<value_type> v) { return ((*this) |= upset_adhoc (mmbdd, v)); }

      upset_adhoc operator+ (std::span<const value_type> v) const {
        auto new_upset = *this;

        for (size_t i = 0; i < dim; ++i) {
          if (v[i] == 0)
            continue;
          Bdd all_other_zeros = Bdd::bddOne ();
          for (size_t j = 0; j < dim; ++j)
            if (j != i)
              all_other_zeros *= !Bdd::bddVar (j);

          if constexpr (std::is_same_v<Bdd, labels::sylvanbdd>) {
            auto map = sylvan::BddMap (i, !Bdd::bddVar (i));
            auto&& [_, s] = plus_one_dim (new_upset.mbdd, Bdd::bddVar (i),
                                          [&] (const auto& l) { return l.Compose (map); },
                                          all_other_zeros,
                                          std::abs (v[i]), (v[i] < 0), false);
            new_upset.mbdd = s;
          }
          else {
            auto map = typename Bdd::remap_t ();
            map[i] = !Bdd::bddVar (i);
            auto&& [_, s] = plus_one_dim (new_upset.mbdd, Bdd::bddVar (i),
                                          [&] (const auto& l) { return l.Remap (map); },
                                          all_other_zeros,
                                          std::abs (v[i]), (v[i] < 0), false);
            new_upset.mbdd = s;
          }
        }
        return new_upset;
      }

      upset_adhoc& operator= (const upset_adhoc& other) {
        assert (&mmbdd == &other.mmbdd);
        mbdd = other.mbdd;
        dim = other.dim;
        return *this;
      }

      upset_adhoc& operator+= (std::span<const value_type> v) { return ((*this) = (*this) + v); }
      upset_adhoc& operator+= (std::initializer_list<value_type> v) { return (operator+=) (std::span (v)); }

      bool contains (std::span<const value_type> sv) const {
        // Make a copy as we're going to shift all these values.
        std::vector<value_type> v (sv.begin (), sv.end ());
        std::vector<letter_type> w;

        assert (v.size () == dim);
        bool is_zero = false;
        while (not is_zero) {
          is_zero = true;
          typename Bdd::letter_type bits;
          bits = Bdd::bddOne ();
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

      bool is_full () const { return (mbdd == mmbdd.full ()); }
      bool is_empty () const { return (mbdd == mmbdd.empty ()); }

      meta_bdd get_mbdd () const {
        return mbdd;
      }

      friend std::ostream& operator<< <> (std::ostream& out, const upset_adhoc& v);

    private:
      master_meta_bdd& mmbdd;
      meta_bdd mbdd;
      size_t dim;

      meta_bdd up_mbdd_high_branch (value_type value, size_t dim) {
        if (value == 0)
          return mmbdd.full ();
        if (value % 2 == 1)
          return mmbdd.make (Bdd::bddVar (dim) * up_mbdd_high_branch (value >> 1, dim) +
                             !Bdd::bddVar (dim) * up_mbdd_low_branch (value >> 1, dim), false);
        return mmbdd.make (Bdd::bddOne () * up_mbdd_high_branch (value >> 1, dim), false);
      }

      meta_bdd up_mbdd_low_branch (value_type value, size_t dim) {
        if (value == 0)
          return mmbdd.make (!Bdd::bddVar (dim) * mmbdd.self () +
                             Bdd::bddVar (dim) * mmbdd.full (), false);
        if (value % 2 == 0)
          return mmbdd.make (Bdd::bddVar (dim) * up_mbdd_high_branch (value >> 1, dim) +
                             !Bdd::bddVar (dim) * up_mbdd_low_branch (value >> 1, dim), false);
        return mmbdd.make (Bdd::bddOne () * up_mbdd_low_branch (value >> 1, dim), false);
      }

      template <typename Flip>
      auto plus_one_dim (const meta_bdd& s, const Bdd& var, Flip flip,
                         const Bdd& all_other_zeros,
                         value_type delta, bool neg, bool carry) const;
  };
}

#include <upset/upset_adhoc.hxx>
