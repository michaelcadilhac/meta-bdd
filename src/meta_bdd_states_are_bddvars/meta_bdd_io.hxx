#include <sstream>
#include <iostream>
#include <set>

#include <utils/bdd_io.hh>
#include <meta_bdd_states_are_bddvars/meta_bdd.hh>

namespace MBDD {

  static auto varnum_to_name_state_aware (size_t varnum) {
    std::string var;
    if (is_varnumstate (varnum)) {
      auto state = varnum_to_state (varnum);

      if (state == STATE_SELF)
        return std::string ("SELF");
      var = "q" + std::to_string (state);
      if (is_varnumstate (varnum, Prime::yes))
        var += "'";
    }
    else
      var = "x" + std::to_string (varnum);
    return var;
  }

  template <typename MMBdd>
  std::ostream& operator<< (std::ostream& os, const bmeta_bdd<MMBdd>& b) {
    std::set<size_t> already_printed = {};
    b.print (os, already_printed);
    return os;
  }

  template <typename MMBdd>
  void bmeta_bdd<MMBdd>::print (std::ostream& os, std::set<size_t>& already_printed) const {
    std::set<size_t> successors;

    os << state;
    already_printed.insert (state);

    auto trans = mmbdd.delta[state];

    if (mmbdd.is_accepting (state))
      os << "(acc)";

    os << ": ";

    // To prettify, remove the EMPTY state.
    utils::print_bdd (trans.Compose (sylvan::BddMap (VARNUM_EMPTY, Bdd::bddZero ())),
                      os, [&successors, &already_printed] (size_t varnum) {
                        if (is_varnumstate (varnum)) {
                          auto succ = varnum_to_state (varnum);
                          if (not already_printed.contains (succ))
                            successors.insert (succ);
                        }
                        return varnum_to_name_state_aware (varnum);
                      });

    os << "\n";

    for (auto&& succ_state : successors)
      bmeta_bdd (mmbdd, succ_state).print (os, already_printed);
  }
}

template <>
struct utils::bdd_io_traits<labels::sylvanbdd::letter_type> {
    static constexpr auto printer = [] (const labels::sylvanbdd::letter_type& bdd, std::ostream& os) {
      print_bdd (bdd, os, MBDD::varnum_to_name_state_aware);
    };
};
