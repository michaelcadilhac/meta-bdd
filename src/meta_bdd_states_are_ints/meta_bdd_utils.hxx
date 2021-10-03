#include <sstream>
#include <iostream>
#include <set>
#include <random>

#include "meta_bdd_states_are_ints/meta_bdd.hh"


namespace MBDD {

  template <typename MMBdd>
  std::vector<typename imeta_bdd<MMBdd>::letter_type> imeta_bdd<MMBdd>::one_word (bool accepted) const {
    std::vector<letter_type> w;
    state_t cur_state = state;

    std::random_device r;
    std::default_random_engine rnd_engine (r ());

    // Project delta on anything but self and !accepted
    while (true) {
      if (mmbdd.is_accepting (cur_state) == accepted)
        return w;

      assert ((accepted and cur_state != STATE_EMPTY) or
              ((not accepted) and cur_state != STATE_FULL));

      if (cur_state == STATE_EMPTY or cur_state == STATE_FULL)
        return w;

      auto& cur_neighbors = mmbdd.delta[cur_state];
      std::uniform_int_distribution<size_t> uniform_dist (0, cur_neighbors.size () - 1);
      auto next = uniform_dist (rnd_engine);

      if (next > 0 and
          ((accepted and mmbdd.delta[cur_state].begin ()->first == STATE_EMPTY) or
           ((not accepted) and (((mmbdd.delta[cur_state].begin ()->first == STATE_EMPTY and
                                 std::next(mmbdd.delta[cur_state].begin ())->first == STATE_FULL)) or
                                (mmbdd.delta[cur_state].begin ()->first == STATE_FULL)))))
        next--;

      state_t next_state = 0;

      for (auto& [dest, labels] : mmbdd.delta[cur_state]) {
        if (not accepted and dest == STATE_FULL)
          continue;
        if (accepted and dest == STATE_EMPTY)
          continue;

        if (next-- == 0) {
          next_state = dest;
          #warning temp
          //w.push_back (labels.one_letter ());
          break;
        }
      }
      assert (next_state != STATE_SELF);
      cur_state = next_state;
    }

    assert (false);
  }
}
