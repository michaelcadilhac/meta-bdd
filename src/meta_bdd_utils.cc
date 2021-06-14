#include <sstream>
#include <iostream>
#include <set>

#include "meta_bdd.hh"


namespace MBDD {

  std::vector<Bdd> meta_bdd::one_word (bool accepted) const {
    std::vector<Bdd> w;
    size_t cur_state = state;
    // Project delta on anything but self and !accepted
    while (true) {
      if (global_mmbdd.is_accepting (cur_state) == accepted)
        return w;

      assert ((accepted and cur_state != STATE_EMPTY) or
              ((not accepted) and cur_state != STATE_FULL));

      if (cur_state == STATE_EMPTY or cur_state == STATE_FULL)
        return w;

      auto dontwant = state_to_bddvar (cur_state) * (accepted ? BDDVAR_EMPTY : BDDVAR_FULL);
      auto future = global_mmbdd.delta[cur_state].UnivAbstract (dontwant).PickOneCube ();

      // Extract the state and label
      auto label = Bdd::bddOne ();
      size_t next_state = -1u;
      while (true) {
        assert (not future.isTerminal ());
        auto var = future.TopVar ();
        if (is_varnumstate (var)) {
          next_state = varnum_to_state (var);
          label *= future.Then ();
          break;
        }
        if (future.Else ().isZero ()) {
          label *= Bdd::bddVar (var);
          future = future.Then ();
        }
        else {
          label *= !Bdd::bddVar (var);
          future = future.Else ();
        }
      }

      w.push_back (label);
      cur_state = next_state;
    }

    assert (false);
  }

}
