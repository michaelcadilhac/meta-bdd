#pragma once

#include <sylvan.h>
#include <sylvan_obj.hpp>

#include <span>
#include <set>
#include <vector>
#include <map>
#include <functional>

#include <iostream>
#include <cassert>

#include <meta_bdd.hh>

// Helper pretty printing.
namespace std {
  std::ostream& operator<< (std::ostream& os, const sylvan::Bdd& w);
  std::ostream& operator<< (std::ostream& os, const std::vector<sylvan::Bdd>& w);
}


namespace MBDD {
  using namespace sylvan;

  // For each state, we have both a regular version and a "prime" version that can be
  // useful when dealing with two meta bdds.
  // - A state i  is encoded as (i << 1) | (1 << 22)
  // - A state i' is encoded as ((i << 1) + 1) | (1 << 22)
  enum class Prime { yes, no, dontcare };
  inline constexpr auto state_to_varnum (size_t state, Prime p = Prime::no) {
    // Variables are bounded by 2^23; we encode states with a leading 1 in binary.
    assert (state >> 22 == 0);
    return ((state << 1) + (p == Prime::yes ? 1 : 0)) | (1 << 22);
  }

  inline auto state_to_bddvar (size_t state, Prime p = Prime::no) {
    return sylvan::Bdd::bddVar (state_to_varnum (state, p));
  }

  static const size_t VARNUM_SELF = state_to_varnum (0) - 1;
#define BDDVAR_SELF (Bdd::bddVar (VARNUM_SELF))

  static const size_t STATE_EMPTY = 0, STATE_FULL = 1;

#define VARNUM_FULL (state_to_varnum (STATE_FULL))
#define BDDVAR_FULL (Bdd::bddVar (VARNUM_FULL))

#define VARNUM_EMPTY (state_to_varnum (STATE_EMPTY))
#define BDDVAR_EMPTY (Bdd::bddVar (VARNUM_EMPTY))

#define VARNUM_FULL_PRIME (state_to_varnum (STATE_FULL, Prime::yes))
#define BDDVAR_FULL_PRIME (Bdd::bddVar (VARNUM_FULL_PRIME))

#define VARNUM_EMPTY_PRIME (state_to_varnum (STATE_EMPTY, Prime::yes))
#define BDDVAR_EMPTY_PRIME (Bdd::bddVar (VARNUM_EMPTY_PRIME))

  // Note: VARNUM_SELF is not considered a varnumstate.
  inline constexpr bool is_varnumstate (size_t varnum, Prime p = Prime::dontcare) {
    if (varnum >> 22 != 1) return false;
    if (p == Prime::dontcare) return true;
    return (p == Prime::no) ^ (varnum & 1);
  }

  inline constexpr auto varnum_to_state (size_t varnum) {
    assert (is_varnumstate (varnum));
    return (varnum ^ (1 << 22)) >> 1;
  }

  // "local" aliases
  using master_bmeta_bdd = master_meta_bdd<sylvan::Bdd, states_are_bddvars>;

  template <typename MMBdd>
  class bmeta_bdd {
      friend master_bmeta_bdd;
      class neighbor_iterator;

      template <typename T>
      using force_const = std::enable_if_t<not std::is_const<T>::value>;

    public:
      bmeta_bdd (MMBdd& mmbdd) : mmbdd {mmbdd}, state (STATE_EMPTY) {}
      bmeta_bdd (MMBdd& mmbdd, Bdd bdd) :
        mmbdd {mmbdd}, state (varnum_to_state (bdd.TopVar ())) {
        assert (is_varnumstate (bdd.TopVar ()));
      }
      bmeta_bdd (MMBdd& mmbdd, size_t state) : mmbdd {mmbdd}, state (state) {}

      template <typename T = MMBdd>//, std::enable_if_t<std::is_const<T>::value>>
      operator bmeta_bdd<const MMBdd> () const { return bmeta_bdd<const MMBdd> (mmbdd, state); }

      bool accepts (std::span<const Bdd> w) const;
      bool accepts (std::initializer_list<Bdd> w) const { return accepts (std::span (w)); }

      template <typename T>
      bool rejects (T&& t) const { return not accepts (std::forward<T> (t)); }
      bool rejects (std::initializer_list<Bdd> w) const { return not accepts (w); }

      auto neighbors () const;

      std::vector<Bdd> one_word (bool accepted = true) const;

      bmeta_bdd one_step (const Bdd& l) const;

      operator Bdd () const {
        return state_to_bddvar (state);
      }

      auto operator<=> (const bmeta_bdd& other) const { return state <=> other.state; }
      // see https://stackoverflow.com/questions/58780829/non-defaulted-operator-doesnt-generate-and-in-c20
      bool operator== (const bmeta_bdd& other)  const { return state == other.state; }

      bmeta_bdd& operator= (const bmeta_bdd& other) {
        assert (&mmbdd == &other.mmbdd);
        state = other.state;
        return *this;
      }

      template <typename T = MMBdd, typename = force_const<T>>
      bmeta_bdd operator& (const bmeta_bdd& other) const;

      template <typename T = MMBdd, typename = force_const<T>>
      bmeta_bdd operator| (const bmeta_bdd& other) const;

      template <typename T = MMBdd, typename = force_const<T>>
      bmeta_bdd transduct (const bmeta_bdd& other,
                           std::span<const Bdd> output_vars, std::span<const Bdd> to_vars) const;

      template <typename T = MMBdd, typename = force_const<T>>
      bmeta_bdd transduct (const bmeta_bdd& other,
                           std::initializer_list<const Bdd> output_vars,
                           std::initializer_list<const Bdd> to_vars) const {
        return transduct (other, std::span (output_vars), std::span (to_vars));
      }

      template<typename Map,
               typename T = MMBdd, typename = force_const<T>>
      bmeta_bdd apply (Map map) const;

    private:
      MMBdd& mmbdd;
      size_t state;

      template <typename Map = std::identity, typename Hash = size_t,
                typename T = MMBdd, typename = force_const<T>>
      bmeta_bdd intersection_union (const bmeta_bdd& other, bool intersection,
                                    const Map& map = {}, const Hash& map_hash = 0) const;

      template <typename T>
      friend std::ostream& operator<< (std::ostream& os, const bmeta_bdd<T>& b);
      void print (std::ostream& os, std::set<size_t>& already_printed) const;
  };

  template <typename MMBdd>
  std::ostream& operator<< (std::ostream& os, const bmeta_bdd<MMBdd>& b);

  template <>
  class master_meta_bdd<sylvan::Bdd, states_are_bddvars> {
    public:
      using meta_bdd = bmeta_bdd<master_meta_bdd>;
      using const_meta_bdd = bmeta_bdd<const master_meta_bdd>;
      friend meta_bdd;
      friend const_meta_bdd;

      master_meta_bdd () : delta (2) { }

      void init () {
        statevars = BDDVAR_SELF * BDDVAR_FULL * BDDVAR_EMPTY;
        statevarsprime = BDDVAR_FULL_PRIME * BDDVAR_EMPTY_PRIME;
        statevars_to_statevarsprime.put (VARNUM_FULL, BDDVAR_FULL_PRIME);
        statevars_to_statevarsprime.put (VARNUM_EMPTY, BDDVAR_EMPTY_PRIME);

        delta[STATE_FULL] = BDDVAR_FULL;
        delta[STATE_EMPTY] = BDDVAR_EMPTY;
        accepting_states.insert (STATE_FULL);

        trans_to_state_noself[ACC][BDDVAR_FULL.GetBDD ()] = STATE_FULL;
        trans_to_state_self[ACC][BDDVAR_SELF.GetBDD ()] = STATE_FULL;

        trans_to_state_noself[REJ][BDDVAR_EMPTY.GetBDD ()] = STATE_EMPTY;
        trans_to_state_self[REJ][BDDVAR_SELF.GetBDD ()] = STATE_EMPTY;
      }

      auto full ()  { return bmeta_bdd (*this,   STATE_FULL); }
      auto empty () { return bmeta_bdd (*this,   STATE_EMPTY); }
      auto self ()  { return Bdd::bddVar (VARNUM_SELF); }

      struct iterator {
          iterator (const master_bmeta_bdd& mmbdd, bool end) :
            mmbdd {mmbdd}, state { end ? -1u : 0} {}
          iterator (const master_bmeta_bdd& mmbdd, size_t state) :
            mmbdd {mmbdd}, state { state } {}

          bool operator== (const iterator& other) const { return state == other.state; }
          bool operator!= (const iterator& other) const { return state != other.state; }
          auto operator++ ();
          auto operator* () const { return bmeta_bdd (mmbdd, state); }

        private:
          const master_bmeta_bdd& mmbdd;
          size_t state;
      };

      const auto begin () const { return iterator (*this, false); }
      const auto end () const { return iterator (*this, true); }

      auto find (Bdd trans, bool is_accepting) const {
        auto x = Bdd (sylvan_project (trans.GetBDD (), (statevars * BDDVAR_SELF).GetBDD ()));
        auto dests = BddSet (x.Support ());

#define SEARCH_IN(map, t) do {                                          \
          auto search = map.find (t.GetBDD ());                         \
          if (search != map.end ()) return iterator (*this, search->second); \
        } while (0)

        if (not dests.contains (VARNUM_SELF)) {
          SEARCH_IN (trans_to_state_noself[is_accepting], trans);
          return iterator (*this, true);
        }
        // There's a self in destination.  First try to see if it is literally in trans_to_state_self.
        SEARCH_IN (trans_to_state_self[is_accepting], trans);

        do {
          auto var = dests.TopVar ();
          dests = dests.Next ();

          if (var == VARNUM_SELF)
            continue;

          // Self can be replaced by any destination that has the same acceptance.
          if (accepting_states.contains (varnum_to_state (var)) != is_accepting)
            continue;

          auto map_self = BddMap (VARNUM_SELF, Bdd::bddVar (var));
          SEARCH_IN (trans_to_state_noself[is_accepting], trans.Compose (map_self));
        } while (not dests.isEmpty ());

        return iterator (*this, true);
#undef SEARCH_IN
      }

      auto make (Bdd trans, bool is_accepting) {
        assert ([&] () {
          if (is_trans_deterministic (trans))
            return true;
          std::cout << "Transition label " << trans
                    << " is not deterministic." << std::endl;
          return false;
        } ());

        // Complete trans: make STATE_EMPTY explicit.
        auto covered_labels = trans.ExistAbstract (statevars);
        if (covered_labels != Bdd::bddOne ())
          trans += BDDVAR_EMPTY * !covered_labels;

        auto search = find (trans, is_accepting);
        if (search != end ())
          return bmeta_bdd<master_bmeta_bdd> (*this, (*search).state);

        auto state = delta.size ();

        auto trans_noself = self_to_state (trans, state);
        delta.emplace_back (trans_noself);

        trans_to_state_self[is_accepting][trans.GetBDD ()] = state;
        trans_to_state_noself[is_accepting][trans_noself.GetBDD ()] = state;

        if (is_accepting)
          accepting_states.insert (state);

        auto state_varnum = state_to_varnum (state);
        statevars *= Bdd::bddVar (state_varnum);
        auto state_varnum_prime_bdd = state_to_bddvar (state, Prime::yes);
        statevarsprime *= state_varnum_prime_bdd;
        statevars_to_statevarsprime.put (state_varnum, state_varnum_prime_bdd);

        check_consistency ();

        return bmeta_bdd<master_meta_bdd> (*this, state);
      }

    private:
      bool is_accepting (size_t state) const {
        return accepting_states.contains (state);
      }

      Bdd self_to_state (const Bdd& b, size_t state) const {
        auto map = BddMap (VARNUM_SELF, state_to_bddvar (state));
        return b.Compose (map);
      }

      Bdd project_to_statevars (const Bdd& t) const {
        return Bdd (sylvan_project (t.GetBDD (), statevars.GetBDD ()));
      }

      size_t successor (size_t state, const Bdd& l) const {
        auto t = project_to_statevars (delta[state] * l);
        assert ([&] () {
          if (t.isTerminal () or not (t.Then ().isOne () and t.Else ().isZero ())) {
            std::cout << "label: " << delta[state]
                      << ", letter: " << l
                      << ", combine to: " << delta[state] * l
                      << ", which is not valid.\n";
            return false;
          }
          return true;
        } ());
        return varnum_to_state (t.TopVar ());
      }

      bool is_trans_deterministic (Bdd trans) const;
      void check_consistency () const;

      template <typename T>
      friend std::ostream& operator<< (std::ostream& os, const bmeta_bdd<T>& b);

      std::vector<Bdd> delta;
      std::set<size_t> accepting_states;
      std::map<BDD, size_t> trans_to_state_self[2], trans_to_state_noself[2];
      enum {
        ACC = true, REJ = false
      };
      Bdd statevars, statevarsprime;
      BddMap statevars_to_statevarsprime;
  };
}

#include "meta_bdd_states_are_bddvars/meta_bdd.hxx"
#include "meta_bdd_states_are_bddvars/meta_bdd_bool.hxx"
#include "meta_bdd_states_are_bddvars/meta_bdd_utils.hxx"
#include "meta_bdd_states_are_bddvars/meta_bdd_io.hxx"
