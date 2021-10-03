#include <sstream>
#include <iostream>
#include <set>

#include "meta_bdd_states_are_ints/meta_bdd.hh"

namespace MBDD {

  template <typename MMBdd>
  std::ostream& operator<< (std::ostream& os, const imeta_bdd<MMBdd>& b) {
    std::set<typename imeta_bdd<MMBdd>::state_t> already_printed = {};
    b.print (os, already_printed);
    return os;
  }

  template <typename MMBdd>
  void imeta_bdd<MMBdd>::print (std::ostream& os, std::set<state_t>& already_printed) const {
    os << state;
    already_printed.insert (state);

    if (mmbdd.is_accepting (state))
      os << "(acc)";

    auto trans = mmbdd.delta[state];

    os << ": " << trans << std::endl;

    for (auto&& [succ_state, _] : trans)
      if (succ_state != STATE_EMPTY)
        if (not already_printed.contains (succ_state))
          imeta_bdd (mmbdd, succ_state).print (os, already_printed);

  }
}
