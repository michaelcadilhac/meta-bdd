#pragma once

namespace MBDD {

  template <typename MMBdd>
  inline bool bmeta_bdd<MMBdd>::accepts (std::span<const letter_type> w) const {
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
  class bmeta_bdd<MMBdd>::neighbor_iterator {
    public:
      neighbor_iterator (MMBdd& mmbdd, Bdd dt) :
        mmbdd {mmbdd},
        dt {dt},
        // states is the OR of destination states.
        states {dt.isZero () ? Bdd::bddZero () : mmbdd.project_to_statevars (dt)},
        state {mmbdd}
      {
        ++(*this);
      }

      neighbor_iterator& operator++ () {
        if (states == Bdd::bddZero ()) {
          state.state = 0;
          label = Bdd::bddZero ();
          return *this;
        }

        assert (states.Then ().isOne ());
        assert (is_varnumstate (states.TopVar ()));

        auto bddstate = Bdd::bddVar (states.TopVar ());
        label = dt.ExistAbstract (bddstate).UnivAbstract (mmbdd.statevars);
        state.state = varnum_to_state (states.TopVar ());
        states = states.Else ();
        return *this;
      }

      bool operator!= (const neighbor_iterator& other) const {
        return states != other.states or state != other.state or label != other.label;
      }

      auto operator* () const { return std::pair (state, label); }
    private:
      MMBdd& mmbdd;
      Bdd dt, states;
      bmeta_bdd state;
      Bdd label;
  };

  template <typename MMBdd>
  inline auto bmeta_bdd<MMBdd>::neighbors () const {
    struct helper {
        MMBdd& mmbdd;
        Bdd dt;
        helper (MMBdd& mmbdd, Bdd dt) : mmbdd {mmbdd}, dt {dt} {}
        auto begin () const { return neighbor_iterator (mmbdd, dt); }
        auto end () const { return neighbor_iterator (mmbdd, Bdd::bddZero ()); }
    };
    return helper (mmbdd, mmbdd.delta[state]);
  }

  template <typename MMBdd>
  inline bmeta_bdd<MMBdd> bmeta_bdd<MMBdd>::one_step (const Bdd& l) const {
    return bmeta_bdd (mmbdd, mmbdd.successor (state, l));
  }

  template <typename MMBdd>
  template <typename Map, typename, typename EnableOnlyIfMMBddIsNotConst>
  bmeta_bdd<MMBdd> bmeta_bdd<MMBdd>::apply (Map map) const {
    if constexpr (std::is_same<Map, std::identity>::value) {
      return *this;
    }

    auto to_make = Bdd::bddZero ();

    for (auto&& [next_state, label] : neighbors ())
      to_make += map (label) *
        (next_state.state == state ? BDDVAR_SELF : Bdd (next_state.apply (map)));

    return mmbdd.make (to_make, mmbdd.is_accepting (state));
  }

  inline bool master_bmeta_bdd::is_trans_deterministic (Bdd trans) const {
    // Frankly unsure what Bdd::operator< is computing, so use raw bdd
    // type for set to sort.
    std::set<BDD> labels;
    auto state_statevars = project_to_statevars (trans);
    while (not state_statevars.isZero ()) {
      auto nextstate = state_statevars.TopVar ();
      assert (nextstate == VARNUM_SELF or
              is_varnumstate (nextstate));
      // There can't be a conjunction of states.
      assert (state_statevars.Then ().isOne ());
      auto gotonextstate = (trans * Bdd (nextstate))
        .ExistAbstract (state_statevars)
        .UnivAbstract (statevars);
      if (not labels.insert (gotonextstate.GetBDD ()).second)
        return false;
      state_statevars = state_statevars.Else ();
    }

    for (auto&& label : labels)
      for (auto&& other_label : labels)
        if ((label != other_label) and
            not (Bdd (label) * Bdd (other_label)).isZero ())
          return false;
    return true;
  }

  inline void master_bmeta_bdd::check_consistency () const {
    // Check that there are no valuation of the nonstate variables that lead to
    // two states.
#ifndef NDEBUG
    //   for (auto&& state : MBDD::mmbdd)
    //assert (is_trans_deterministic (mmbdd.delta[state.state]));
#endif
  }
}
