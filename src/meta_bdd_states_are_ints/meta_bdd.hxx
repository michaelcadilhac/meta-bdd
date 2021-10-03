#pragma once

namespace MBDD {

  template <typename MMBdd>
  inline bool imeta_bdd<MMBdd>::accepts (std::span<const letter_type> w) const {
    auto cur_state = state;
    for (auto&& l : w) {
      cur_state = mmbdd.successor (cur_state, l);
      if (cur_state == STATE_FULL)
        return true;
      if (cur_state == STATE_EMPTY)
        return false;
    }
    return mmbdd.is_accepting (cur_state);
  }

  template <typename MMBdd>
  template <typename Iterator>
  class imeta_bdd<MMBdd>::neighbor_iterator {
    public:
      neighbor_iterator (MMBdd& mmbdd, Iterator it) : mmbdd {mmbdd}, it {it} {}
      neighbor_iterator& operator++ () { ++it; return *this; }
      bool operator!= (const neighbor_iterator& other) const { return it != other.it; }
      auto operator* () const { return std::pair (imeta_bdd<MMBdd> (mmbdd, it->first), it->second); }
    private:
      MMBdd& mmbdd;
      Iterator it;
  };


  template <typename MMBdd>
  inline auto imeta_bdd<MMBdd>::neighbors () const {
    struct helper {
        MMBdd& mmbdd;
        const MMBdd::transition_type& trans;
        helper (MMBdd& mmbdd, const MMBdd::transition_type& trans) : mmbdd {mmbdd}, trans {trans} {}
        auto begin () const { return neighbor_iterator (mmbdd, trans.begin ()); }
        auto end () const { return neighbor_iterator (mmbdd, trans.end ()); }
    };
    return helper (mmbdd, mmbdd.delta[state]);
  }

  template <typename MMBdd>
  inline imeta_bdd<MMBdd> imeta_bdd<MMBdd>::one_step (const letter_type& l) const {
    return imeta_bdd (mmbdd, mmbdd.successor (state, l));
  }

  template <typename MMBdd>
  template <typename Map, typename, typename EnableOnlyIfMMBddIsNotConst>
  imeta_bdd<MMBdd> imeta_bdd<MMBdd>::apply (Map map) const {
    if constexpr (std::is_same<Map, std::identity>::value) {
      return *this;
    }

    std::map<state_t, letter_set_type> m;

    for (auto&& [next_state, labels] : mmbdd.delta[state]) {
      state_t next_applied_state = (next_state == state ?
                                    /*  */ STATE_SELF :
                                    /*  */ imeta_bdd (mmbdd, next_state).apply (map).state);
      m[next_applied_state] += map (labels);
    }

    return mmbdd.make (transition (m), mmbdd.is_accepting (state));
  }

  template <typename LetterSet>
  bool master_meta_bdd<LetterSet, states_are_ints>::is_trans_deterministic (transition_type trans) const {
    LetterSet all_letters;

    for (auto& [dest, labels] : trans) {
      if (not (labels * all_letters).empty ())
        return false;
      all_letters += labels;
    }
    return true;
  }

  template <typename LetterSet>
  void master_meta_bdd<LetterSet, states_are_ints>::check_consistency () const {
    // Check that there are no valuation of the nonstate variables that lead to
    // two states.
#ifndef NDEBUG
    //   for (auto&& state : MBDD::mmbdd)
    //assert (is_trans_deterministic (mmbdd.delta[state.state]));
#endif
  }
}
