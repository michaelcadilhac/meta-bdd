#pragma once

#include <sylvan.h>
#include <sylvan_obj.hpp>

#include <span>
#include <set>
#include <vector>
#include <map>
#include <functional>

#include <iostream>

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

  inline constexpr bool is_varnumstate (size_t varnum, Prime p = Prime::dontcare) {
    if (varnum >> 22 != 1) return false;
    if (p == Prime::dontcare) return true;
    return (p == Prime::no) ^ (varnum & 1);
  }

  inline constexpr auto varnum_to_state (size_t varnum) {
    assert (is_varnumstate (varnum));
    return (varnum ^ (1 << 22)) >> 1;
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

  class meta_bdd;
  class master_meta_bdd;
  std::ostream& operator<< (std::ostream& os, const meta_bdd& b);

  void init ();

  class meta_bdd {
    public:
      meta_bdd (size_t state) : state (state) {}
      bool accepts (std::span<const Bdd> w) const;
      bool accepts (std::initializer_list<Bdd> w) const { return accepts (std::span (w)); }

      template <typename T>
      bool rejects (T&& t) const { return not accepts (std::forward<T> (t)); }
      bool rejects (std::initializer_list<Bdd> w) const { return not accepts (w); }

      std::vector<Bdd> one_word (bool accepted = true) const;

      meta_bdd one_step (const Bdd& l) const;

      operator Bdd () const {
        return state_to_bddvar (state);
      }

      bool operator== (const meta_bdd&) const = default;

      meta_bdd operator& (const meta_bdd& other) const;
      meta_bdd operator| (const meta_bdd& other) const;

      meta_bdd transduct (const meta_bdd& other,
                          std::span<const Bdd> output_vars, std::span<const Bdd> to_vars) const;

      meta_bdd transduct (const meta_bdd& other,
                          std::initializer_list<const Bdd> output_vars,
                          std::initializer_list<const Bdd> to_vars) const {
        return transduct (other, std::span (output_vars), std::span (to_vars));
      }

      template <typename Map>
      meta_bdd apply (Map map) const;

    private:
      size_t state;

      template <typename Map = std::identity>
      meta_bdd intersection_union (const meta_bdd& other, bool intersection,
                                   Map map = {}) const;

      friend std::ostream& operator<< (std::ostream& os, const meta_bdd& b);
      void print (std::ostream& os, std::set<size_t>& already_printed) const;
  };

  class master_meta_bdd {
      friend class meta_bdd;
    public:
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

      struct iterator {
          iterator (bool end) : state { end ? -1u : 0} {}
          iterator (size_t state) : state { state } {}
          bool operator== (const iterator&) const = default;
          bool operator!= (const iterator&) const = default;
          auto operator++ ();
          auto operator* () const { return meta_bdd (state); }
        private:
          size_t state;
      };

      const auto begin () const { return iterator (false); }
      const auto end () const { return iterator (true); }

      auto find (Bdd trans, bool is_accepting) const {
        auto x = Bdd (sylvan_project (trans.GetBDD (), (statevars * BDDVAR_SELF).GetBDD ()));
        auto dests = BddSet (x.Support ());

#define SEARCH_IN(map, t) do {                                          \
          auto search = map.find (t.GetBDD ());                         \
          if (search != map.end ()) return iterator (search->second);   \
        } while (0)

        if (not dests.contains (VARNUM_SELF)) {
          SEARCH_IN (trans_to_state_noself[is_accepting], trans);
          return iterator (true);
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

        return iterator (true);
#undef SEARCH_IN
      }

      meta_bdd make (Bdd trans, bool is_accepting) {
        // Complete trans: make STATE_EMPTY explicit.
        auto covered_labels = trans.ExistAbstract (statevars);
        if (covered_labels != Bdd::bddOne ())
          trans += BDDVAR_EMPTY * !covered_labels;

        auto search = find (trans, is_accepting);
        if (search != end ())
          return *search;

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

        return meta_bdd (state);
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

      friend std::ostream& operator<< (std::ostream& os, const meta_bdd& b);

      std::vector<Bdd> delta;
      std::set<size_t> accepting_states;
      std::map<BDD, size_t> trans_to_state_self[2], trans_to_state_noself[2];
      enum {
        ACC = true, REJ = false
      };
      Bdd statevars, statevarsprime;
      BddMap statevars_to_statevarsprime;
  };

  extern master_meta_bdd global_mmbdd;

  /////////////////////////////////////////////////////////////// User interface
  inline void init () {
    global_mmbdd.init ();
  }

  inline meta_bdd full () {
    return meta_bdd (STATE_FULL);
  }

  inline meta_bdd empty () {
    return meta_bdd (STATE_EMPTY);
  }

  inline Bdd self () {
    return Bdd::bddVar (VARNUM_SELF);
  }

  inline meta_bdd make (Bdd trans, bool is_accepting) {
    return global_mmbdd.make (trans, is_accepting);
  }
}

#include "meta_bdd.hxx"
