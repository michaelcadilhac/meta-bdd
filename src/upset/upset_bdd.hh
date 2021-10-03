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
  class upset<MBDD::master_meta_bdd<Bdd, StateType>> {
      // Make sure we're dealing with a Bdd implementation
      using has_bddone = decltype (Bdd::bddOne ());
      using master_meta_bdd = MBDD::master_meta_bdd<Bdd, StateType>;
      using meta_bdd = master_meta_bdd::meta_bdd;
      using transition_type = master_meta_bdd::transition_type;
    public:
      // This must be signed since plus_transducers can decrement values.
      using value_type = ssize_t;
    private:
      upset (master_meta_bdd& mmbdd, const meta_bdd& m, size_t d)
        : mmbdd {mmbdd}, mbdd {m}, dim {d} {}

    public:
      upset (master_meta_bdd& mmbdd, const std::vector<value_type>& v);

      upset (master_meta_bdd& mmbdd,
             std::initializer_list<value_type> v) :
        upset (mmbdd, std::vector (v)) {}

      bool operator== (const upset& other) const {
        // Don't compare oranges and apples
        assert (&mmbdd == &other.mmbdd and dim == other.dim);
        return mbdd == other.mbdd;
      }

      upset operator& (const upset& other) const {
        assert (dim == other.dim and &mmbdd == &other.mmbdd);
        return upset (mmbdd, mbdd & other.mbdd, dim);
      }
      upset operator& (const std::vector<value_type>& v) const {
        return (*this) & upset (mmbdd, v);
      }

      upset& operator&= (const upset& other) { return ((*this) = (*this) & other); }
      upset& operator&= (std::initializer_list<value_type> v) { return ((*this) &= upset (mmbdd, v)); }
      upset& operator&= (const std::vector<value_type>& v) { return ((*this) &= upset (mmbdd, v)); }

      upset operator| (const upset& other) const {
        assert (dim == other.dim and &mmbdd == &other.mmbdd);
        return upset (mmbdd, mbdd | other.mbdd, dim);
      }
      upset operator| (const std::vector<value_type>& v) const {
        return (*this) | upset (mmbdd, v);
      }

      upset& operator|= (const upset& other) { return ((*this) = (*this) | other); }
      upset& operator|= (std::initializer_list<value_type> v) { return ((*this) |= upset (mmbdd, v)); }

      upset operator^ (std::span<const value_type> v) const {
        std::vector<value_type> abs_v (v.size ());
        std::vector<bool>   neg (v.size ());
        size_t i = 0;
        for (auto x : v) {
          abs_v[i] = std::abs (x);
          neg[i] = (x < 0);
          ++i;
        }

        std::cout << "Computing transduction..." << std::endl;
        auto trans = plus_transducer (abs_v, neg, std::vector<bool> (v.size ()));
        std::cout << "TRANS: " << trans << std::endl;
        std::cout << "Transducting..." << std::endl;
        auto tctd = transduct (trans);
        std::cout << "TRANS'd: " << tctd.get_mbdd () << std::endl;
        std::cout << "Zero padding..." << std::endl;
        auto pdd = tctd.full_zero_padded ();
        std::cout << "ZPDD'd: " << pdd.get_mbdd () << std::endl;
        std::cout << "Done." << std::endl;
        return pdd;
      }

      upset operator+ (const std::vector<value_type> v) const {
        return *this + std::span (v);
      }

      upset operator+ (std::span<const value_type> v) const {
        auto new_upset = *this;

        for (size_t i = 0; i < dim; ++i) {
          if (v[i] == 0)
            continue;

          static auto self_trans_cache = utils::make_cache<Bdd> (dim, i);
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
          // Zero padding here can reduce the size of the mbdd, hence speed things up.
          new_upset = new_upset.transduct (trans).full_zero_padded ();
          std::cout << "Done dimension " << i << std::endl;
        }
        /* Alternatively, we could zero pad at the end.
         std::cout << "Zero padding..." << std::endl;
         auto pdd = new_upset.full_zero_padded (); */
        return new_upset;
      }

      upset& operator= (const upset& other) {
        assert (&mmbdd == &other.mmbdd);
        mbdd = other.mbdd;
        dim = other.dim;
        return *this;
      }

      upset& operator+= (std::span<const value_type> v) { return ((*this) = (*this) + v); }
      upset& operator+= (std::initializer_list<value_type> v) { return (operator+=) (std::span (v)); }

      bool contains (std::span<const value_type> sv) const {
        // Make a copy as we're going to shift all these values.
        std::vector<value_type> v (sv.begin (), sv.end ());
        std::vector<typename Bdd::letter_type> w;

        assert (v.size () == dim);
        bool is_zero = false;
        while (not is_zero) {
          is_zero = true;
          typename Bdd::letter_type bits;
          bits = Bdd::bddOne ();
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

      bool is_full () const { return (mbdd == mmbdd.full ()); }
      bool is_empty () const { return (mbdd == mmbdd.empty ()); }

      meta_bdd get_mbdd () const {
        return mbdd;
      }

      friend std::ostream& operator<< <> (std::ostream& out, const upset& v);

    private:
      master_meta_bdd& mmbdd;
      meta_bdd mbdd;
      size_t dim;

      meta_bdd up_mbdd_high_branch (value_type value, size_t dim) {
        if (value == 0)
          return mmbdd.full ();
        if (value % 2 == 1)
          return mmbdd.make (Bdd::bddVar (2 * dim) * up_mbdd_high_branch (value >> 1, dim) +
                             !Bdd::bddVar (2 * dim) * up_mbdd_low_branch (value >> 1, dim), false);
        return mmbdd.make (Bdd::bddOne () * up_mbdd_high_branch (value >> 1, dim), false);
      }

      meta_bdd up_mbdd_low_branch (value_type value, size_t dim) {
        if (value == 0)
          return mmbdd.make (!Bdd::bddVar (2 * dim) * mmbdd.self () +
                             Bdd::bddVar (2 * dim) * mmbdd.full (), false);
        if (value % 2 == 0)
          return mmbdd.make (Bdd::bddVar (2 * dim) * up_mbdd_high_branch (value >> 1, dim) +
                             !Bdd::bddVar (2 * dim) * up_mbdd_low_branch (value >> 1, dim), false);
        return mmbdd.make (Bdd::bddOne () * up_mbdd_low_branch (value >> 1, dim), false);
      }

      auto bit_identities (size_t nbits) const;

      auto full_zero_padded (const meta_bdd& s, Bdd all_zero) const;

      upset full_zero_padded () const {
        Bdd all_zero = Bdd::bddOne ();

        for (size_t i = 0; i < dim; ++i)
          all_zero *= !Bdd::bddVar (2 * i);

        return upset (mmbdd, full_zero_padded (mbdd, all_zero), dim);
      }

      auto plus_transducer_one_dim (size_t idx, size_t dim,
                                    value_type delta,
                                    bool neg, bool carry,
                                    Bdd untouched_components) const;

      auto plus_transducer (const std::vector<value_type>& delta,
                            const std::vector<bool>& neg,
                            const std::vector<bool>& carries) const;

      upset transduct (const meta_bdd& trans) const {
        std::vector<Bdd> ins (dim), outs (dim);
        for (size_t i = 0; i < dim; ++i) {
          ins[i] = Bdd::bddVar (2 * i);
          outs[i] = Bdd::bddVar (2 * i + 1);
        }

        return upset (mmbdd, utils::transduct (mbdd, trans, outs, ins), dim);
      }
  };
}

#include <upset/upset_bdd.hxx>
