#pragma once

#include <span>
#include <set>
#include <vector>
#include <map>
#include <functional>
#include <deque>

#include <iostream>
#include <cassert>

#include <meta_bdd.hh>

#include <utils/ref_dlg_cmp.hh>

namespace MBDD {

  template <typename State, typename LetterSet>
  class transition : public std::map<State, LetterSet> {
    public:
      using transition_map = std::map<State, LetterSet>;
      using transition_map::size;
      using transition_map::begin;
      using transition_map::end;

      transition () {}

      transition (const std::map<State, LetterSet>& m) : transition_map (m) {
        // Erase STATE_SELF, storing it separately.
        auto&& f = this->find (STATE_SELF);
        if (f != this->end ()) {
          _has_self = true;
          _self_labels = f->second;
          this->erase (f);
        }
        complete ();
#ifndef NDEBUG
        LetterSet to_nonempty, to_empty;
        for (auto&& [s, ls] : *this) {
          assert (not ls.isZero ());
          if (s == STATE_EMPTY)
            to_empty |= ls;
          else
            to_nonempty |= ls;
        }
        if (not _has_self)
          assert (to_empty == !to_nonempty);
        else
          assert ((to_empty | _self_labels) == !to_nonempty or
                  !to_empty == (to_nonempty | _self_labels));
#endif
      }

      transition (State state, const LetterSet& ls) {
        if (state == STATE_SELF) {
          _has_self = true;
          _self_labels = ls;
        }
        else
          (*this)[state] = ls;
      }

      bool             has_self () const    { return _has_self;   }
      const LetterSet& self_labels () const { return _self_labels; }
      transition       self_to_fresh_state (State state) const {
        if (not _has_self)
          return (*this);
        transition ret;
        ret.transition_map::operator= (*this);
        ret._has_self = false;
        assert (not ret.contains (state));
        ret[state] = _self_labels;
        return ret;
      }

      transition_map& complete () {
        // Complete trans: make STATE_EMPTY explicit.
        LetterSet present = _self_labels;
        for (auto& [s, ls] : *(this))
          present |= ls;
        auto absent = !present;
        if (not absent.isZero ())
          (*this)[STATE_EMPTY] |= absent;
        return *this;
      }

      // This merges the two internal maps.
      transition operator+ (const transition& other) const {
        transition ret;

        // Relies on the fact that the keys (states) are ordered.
        for (auto&& trans_it = begin (), other_it = other.begin ();
             trans_it != end () or other_it != other.end ();
             /* in-loop update */) {
          if (trans_it == end () or
              (other_it != other.end () and other_it->first < trans_it->first))
            ret.emplace (*other_it++);
          else if (other_it == other.end () or trans_it->first < other_it->first)
            ret.emplace (*trans_it++);
          else {
            assert (trans_it->first == other_it->first);
            ret[trans_it->first] = trans_it->second + other_it->second;
            trans_it++, other_it++;
          }
        }
        ret._has_self = _has_self or other._has_self;
        if (_has_self and not other._has_self)
          ret._self_labels = _self_labels;
        else if (not _has_self and other._has_self)
          ret._self_labels = other._self_labels;
        else if (ret._has_self)
          ret._self_labels = _self_labels + other._self_labels;

        return ret;
      }

      transition& operator+= (const transition& other) {
        // stub
        return (*this = *this + other);
      }

      // Implementing <=> seems overkill for the moment:
      // https://stackoverflow.com/questions/69338826/
      constexpr bool operator< (const transition& other) const {
        auto t = size () <=> other.size ();
        if (t < 0) return true;
        if (t > 0) return false;

        if (_has_self and not other._has_self)
          return true; /* this is arbitrary. */
        if (other._has_self and not _has_self)
          return false;/* this is arbitrary. */

        if (_has_self and other._has_self) {
          auto t = _self_labels <=> other._self_labels;
          if (t < 0) return true;
          if (t > 0) return false;
        }

        // Relies on the fact that the order is the same on states.
        for (auto trans_it = begin (), other_it = other.begin ();
             trans_it != end () /* and other_it != other.end () unneeded as same sized */;
             ++trans_it, ++other_it) {
          auto&& [state,       letters] = *trans_it;
          auto&& [other_state, other_letters] = *other_it;
          auto t = state <=> other_state;
          if (t < 0) return true;
          if (t > 0) return false;

          auto tt = letters <=> other_letters;
          if (tt < 0) return true;
          if (tt > 0) return false;
        }
        return false; // they're equal
      }

      friend std::ostream& operator<< (std::ostream& os, const transition& trans) {
        bool first = true;
        if (trans._has_self) {
          first = false;
          os << "SELF * (" << trans._self_labels << ")";
        }
        for (auto&& [succ_state, labels] : trans) {
          if (succ_state == STATE_EMPTY)
            continue;
          if (not first)
            os << " + ";
          first = false;
          os << "q" << succ_state << " * " << "(" << labels << ")";
        }
        return os;
      }
    private:
      bool _has_self = false;
      LetterSet _self_labels;
  };


  template <typename MMBdd>
  class imeta_bdd {
      friend MMBdd;
      template <typename Iterator>
      class neighbor_iterator;

      template <typename T>
      using force_const = std::enable_if_t<not std::is_const<T>::value>;

      using letter_type = typename MMBdd::letter_type;
      using letter_set_type = typename MMBdd::letter_set_type;
      using state_t = typename MMBdd::state_t;
      using transition_type = typename MMBdd::transition_type;

    public:
      imeta_bdd (MMBdd& mmbdd) : mmbdd {mmbdd}, state (STATE_EMPTY) {}
      imeta_bdd (MMBdd& mmbdd, state_t state) : mmbdd {mmbdd}, state (state) {}

      operator imeta_bdd<const MMBdd> () const { return imeta_bdd<const MMBdd> (mmbdd, state); }

      bool accepts (std::span<const letter_type> w) const;
      bool accepts (std::initializer_list<letter_type> w) const { return accepts (std::span (w)); }

      template <typename T>
      bool rejects (T&& t) const { return not accepts (std::forward<T> (t)); }
      bool rejects (std::initializer_list<letter_type> w) const { return not accepts (w); }

      auto neighbors () const;

      std::vector<letter_type> one_word (bool accepted = true) const;

      imeta_bdd one_step (const letter_type& l) const;

      transition_type operator* (const letter_set_type& ls) const {
        return transition_type (state, ls);
      }
      friend transition_type operator* (const letter_set_type& ls, const imeta_bdd& trans) {
        return trans * ls;
      }

      transition_type operator* (const letter_type& l) const {
        return transition_type (state, letter_set_type (l));
      }
      friend transition_type operator* (const letter_type& l, const imeta_bdd& trans) {
        return trans * l;
      }

      auto operator<=> (const imeta_bdd& other) const { return state <=> other.state; }
      // see https://stackoverflow.com/questions/58780829/non-defaulted-operator-doesnt-generate-and-in-c20
      bool operator== (const imeta_bdd& other)  const { return state == other.state; }

      imeta_bdd& operator= (const imeta_bdd& other) {
        assert (&mmbdd == &other.mmbdd);
        state = other.state;
        return *this;
      }

      template <typename T = MMBdd, typename = force_const<T>>
      imeta_bdd operator& (const imeta_bdd& other) const;

      template <typename T = MMBdd, typename = force_const<T>>
      imeta_bdd operator| (const imeta_bdd& other) const;

      template <typename Map, typename Hash, typename T = MMBdd, typename = force_const<T>>
      imeta_bdd transduct (const imeta_bdd& other, Map map, const Hash& hash) const;

#warning cache apply
      template<typename Map,
               typename T = MMBdd, typename = force_const<T>>
      imeta_bdd apply (Map map) const;

      operator state_t () const { return state; }

    private:
      MMBdd& mmbdd;
      state_t state;

      template <typename Map = std::identity, typename Hash = size_t,
                typename T = MMBdd, typename = force_const<T>>
      imeta_bdd intersection_union (const imeta_bdd& other, bool intersection,
                                    const Map& map = {}, const Hash& map_hash = 0) const;

      template <typename T>
      friend std::ostream& operator<< (std::ostream& os, const imeta_bdd<T>& b);
      void print (std::ostream& os, std::set<state_t>& already_printed) const;
  };

  template <typename MMBdd>
  std::ostream& operator<< (std::ostream& os, const imeta_bdd<MMBdd>& b);

  template <typename LetterSet>
  class master_meta_bdd<LetterSet, states_are_ints> {
    public:
      using meta_bdd = imeta_bdd<master_meta_bdd>;
      using const_meta_bdd = imeta_bdd<const master_meta_bdd>;
      friend meta_bdd;
      friend const_meta_bdd;
      struct iterator;

#ifdef NDEBUG
      using state_t = size_t;
#else
      // This is a "type safe" state.
      class state_t {
          friend iterator;
        public:
          state_t () : state {0} {}
          state_t (size_t state) : state {state} {}
          auto operator+ (const state_t&) const = delete;
          auto operator* (const state_t&) const = delete;
          auto& operator++ () { ++state; return *this; }
          operator size_t () const { return state; }
          friend std::ostream& operator<< (std::ostream& os, const state_t& b) { return (os << b.state); }
        private:
          size_t state;
      };
#endif

      using enum constant_states;
      using letter_set_type = LetterSet;
      using letter_type = typename LetterSet::letter_type;
      using transition_type = transition<state_t, letter_set_type>;

      master_meta_bdd () : delta (1) { }

      void init () {
        auto self_trans = transition_type (STATE_SELF, letter_set_type::fullset ());

        // This has to go in order of constant_states
        // STATE_EMPTY
        auto& set = delta.emplace_back (transition_type (STATE_EMPTY, letter_set_type::fullset ()));
        trans_to_state_noself[REJ][set] = STATE_EMPTY;
        trans_to_state_self[REJ][self_trans] = STATE_EMPTY;

        // STATE_FULL
        auto& sft = delta.emplace_back (transition_type (STATE_FULL, letter_set_type::fullset ()));
        accepting_states.insert (STATE_FULL);
        trans_to_state_noself[ACC][sft] = STATE_FULL;
        trans_to_state_self[ACC][self_trans] = STATE_FULL;
      }

    public:
      auto full ()  { return imeta_bdd (*this, STATE_FULL); }
      auto empty () { return imeta_bdd (*this, STATE_EMPTY); }
      auto self ()  { return imeta_bdd (*this, STATE_SELF); }

      struct iterator {
          iterator (const master_meta_bdd& mmbdd, bool end) :
            mmbdd {mmbdd}, state { end ? -1u : 0} {}
          iterator (const master_meta_bdd& mmbdd, state_t state) :
            mmbdd {mmbdd}, state { state } {}

          bool operator== (const iterator& other) const { return state == other.state; }
          bool operator!= (const iterator& other) const { return state != other.state; }
          auto operator++ () { if (++state == state_t (mmbdd.delta.size ())) state = -1u; return *this; }
          auto operator* () const { return imeta_bdd (mmbdd, state); }

        private:
          const master_meta_bdd& mmbdd;
          state_t state;
      };

      const auto begin () const { return iterator (*this, false); }
      const auto end () const { return iterator (*this, true); }

    private:
      auto find (const transition_type& trans, bool is_accepting) const {
#define SEARCH_IN(map, t) do {                                          \
          auto search = map.find (t);                                   \
          if (search != map.end ()) return iterator (*this, search->second); \
        } while (0)

        if (not trans.has_self ()) {
          SEARCH_IN (trans_to_state_noself[is_accepting], trans);
          return iterator (*this, true);
        }
        if (not is_accepting and trans.empty ()) // Only a self loop and rejecting, that's empty
          return iterator (*this, (state_t) STATE_EMPTY);
        // There's a self in destination.  First try to see if it is literally in trans_to_state_self.
        SEARCH_IN (trans_to_state_self[is_accepting], trans);

        // Now compare it with _noself: go through all states in destination,
        // putting the self_label in conjunction with the current label.
        auto&& self_labels = trans.self_labels ();

        for (auto&& [other_trans_ref, src_state] : trans_to_state_noself[is_accepting]) {
          auto& other_trans = other_trans_ref.get ();
          if (other_trans.size () != trans.size ())
            continue;
          bool had_one_discrepency = false;
          bool match = true;
          for (auto&& trans_it = trans.begin (), other_trans_it = other_trans.begin ();
               trans_it != trans.end () /* and other_trans_it != other_trans.end () unneeded as same sized */;
               ++trans_it, ++other_trans_it) {
            auto&& [state, labels] = *trans_it;
            auto&& [other_state, other_labels] = *other_trans_it;
            if (state != other_state) { // Not the right one for sure.
              match = false;
              break;
            }
            if (labels != other_labels) {
              if (had_one_discrepency or
                  // this isn't a self loop:
                  &(delta[other_state]) != &other_trans) {
                match = false;
                break;
              }
              had_one_discrepency = true;
              if (labels + self_labels != other_labels) {
                match = false;
                break;
              }
            }
          }
          // We should have found one place to put self_labels.
          if (not had_one_discrepency)
            match = false;
          if (match)
            return iterator (*this, src_state);
        }
        return iterator (*this, true);
#undef SEARCH_IN
      }

    public:
      auto make (transition_type trans, bool is_accepting) {
        assert ([&] () {
          if (is_trans_deterministic (trans))
            return true;
          std::cout << "Transition label " << trans
                    << " is not deterministic." << std::endl;
          return false;
        } ());

        // Add STATE_EMPTY as needed.
        trans.complete ();

        auto search = find (trans, is_accepting);
        if (search != end ())
          return imeta_bdd<master_meta_bdd> (*this, (*search).state);

        // Not found: create from scratch.
        auto state = delta.size ();

        auto& tns = delta.emplace_back (trans.self_to_fresh_state (state));

        trans_to_state_self[is_accepting][trans] = state;
        trans_to_state_noself[is_accepting][tns] = state; // by reference

        if (is_accepting)
          accepting_states.insert (state);

        check_consistency ();

        return imeta_bdd<master_meta_bdd> (*this, state);
      }

    private:
      bool is_accepting (state_t state) const {
        return accepting_states.contains (state);
      }

      state_t successor (state_t state, const letter_type& l) const {
        for (auto&& [dest, labels] : delta[state])
#warning fixme
          //if (labels.contains (l))
          if (not (labels * l).isZero ())
            return dest;
        return STATE_EMPTY;
      }

      bool is_trans_deterministic (transition_type trans) const;
      void check_consistency () const;

      template <typename T>
      friend std::ostream& operator<< (std::ostream& os, const imeta_bdd<T>& b);

      std::deque<transition_type> delta;  // this is with noself, all distinct by construction
                                      // deque is necessary as we use its elements by reference while
                                      // modifying it.
      std::set<state_t> accepting_states;

      std::map<transition_type, state_t>
      /*  */ trans_to_state_self[2];

      std::map<std::reference_wrapper<const transition_type>, state_t,
               utils::ref_dlg_cmp<const transition_type>>
      /*  */ trans_to_state_noself[2];

      enum {
        ACC = true, REJ = false
      };
  };
}

#include "meta_bdd_states_are_ints/meta_bdd.hxx"
#include "meta_bdd_states_are_ints/meta_bdd_bool.hxx"
#include "meta_bdd_states_are_ints/meta_bdd_utils.hxx"
#include "meta_bdd_states_are_ints/meta_bdd_io.hxx"
