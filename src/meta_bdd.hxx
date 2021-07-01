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
        // xx

      //   p           q
      //   ->o ----  >o
      //       l


      // delta[p] = l & q | l' & q' | ...

      //     q | q' | ... = delta[p].project (state variables)

      //     l = delta[p] (q = 1) (q', ... = 0)

      //     r = q.apply (map)

      //     map (l) & r.

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
}
