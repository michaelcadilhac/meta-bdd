#pragma once

namespace MBDD {

  // These implementations are moved mostly because they talk about the global MMBDD.
  inline bool meta_bdd::accepts (std::span<const Bdd> w) const {
    auto cur_state = state;
    for (auto&& l : w) {
      cur_state = global_mmbdd.successor (cur_state, l);
      if (cur_state == STATE_FULL)
        return true;
      if (cur_state == STATE_EMPTY)
        return false;
    }
    return global_mmbdd.is_accepting (cur_state);
  }

  inline meta_bdd meta_bdd::one_step (const Bdd& l) const {
    return meta_bdd (global_mmbdd.successor (state, l));
  }

  template <typename Map>
  meta_bdd meta_bdd::apply (Map map) const {
    if constexpr (std::is_same<Map, std::identity>::value) {
      return *this;
    }

    auto to_make = Bdd::bddZero ();
    auto trans = global_mmbdd.delta[state];
    // dests is the OR of destination states.
    auto dests = global_mmbdd.project_to_statevars (trans);

    while (dests != Bdd::bddZero ()) {
      assert (not dests.isTerminal ());
      assert (dests.Then ().isOne ());
      assert (is_varnumstate (dests.TopVar ()));

      auto bddstate = Bdd::bddVar (dests.TopVar ());
      // The labels that go to bddstate are computed by removing all labels that
      // still have a state after bddstate is existentially quantified.
      auto labels_to_state = trans.ExistAbstract (bddstate)
        .UnivAbstract (global_mmbdd.statevars);

      if (varnum_to_state (dests.TopVar ()) == state) // Self loop
        bddstate = BDDVAR_SELF;
      else
        to_make += map (labels_to_state) * bddstate;
      dests = dests.Else ();
    }
    return global_mmbdd.make (to_make, global_mmbdd.is_accepting (state));
  }

  inline auto master_meta_bdd::iterator::operator++ () {
    if (++state == global_mmbdd.delta.size ())
      state = -1u;
    return *this;
  }

  inline bool master_meta_bdd::is_trans_deterministic (Bdd trans) const {
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

  inline void master_meta_bdd::check_consistency () const {
    // Check that there are no valuation of the nonstate variables that lead to
    // two states.
    for (auto&& state : MBDD::global_mmbdd)
      assert (is_trans_deterministic (global_mmbdd.delta[state.state]));
  }
}
