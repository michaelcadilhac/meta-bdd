#include <sstream>
#include <iostream>
#include <set>

#include "meta_bdd.hh"

namespace MBDD {

  static auto varnum_to_name_state_aware (size_t varnum) {
    std::string var;
    if (is_varnumstate (varnum)) {
      auto state = varnum_to_state (varnum);
      var = "q" + std::to_string (state);
      if (is_varnumstate (varnum, Prime::yes))
        var += "'";
    }
    else if (varnum == VARNUM_SELF)
      var = "SELF";
    else
      var = "x" + std::to_string (varnum);
    return var;
  }

  template <typename F = decltype (varnum_to_name_state_aware)>
  static void print_bdd (const Bdd& t, std::ostream& os = std::cout, F var_to_name = varnum_to_name_state_aware) {
    if (t.isZero ()) { os << "F"; return; }
    if (t.isOne ())  { os << "T"; return; }

    std::string var = var_to_name (t.TopVar ());

    if (t.Then ().isTerminal () and t.Else ().isTerminal ()) { // Boolean
      if (t.Then ().isOne () and t.Else ().isZero ())
        os << var;
      else {
        assert (t.Then ().isZero () and t.Else ().isOne ());
        os << "!" << var;
      }
    }
    else if (t.Then ().isTerminal ()) {
      if (t.Then ().isZero ())
        os << "(!" << var << "&&";
      else
        os << "(" << var << "||";
      print_bdd (t.Else (), os, var_to_name);
      os << ")";
    }
    else if (t.Else ().isTerminal ()) {
      if (t.Else ().isZero ())
        os << "(" << var << "&&";
      else
        os << "(!" << var << "||";
      print_bdd (t.Then (), os, var_to_name);
      os << ")";
    }
    else {
      os << "(" << var << "&&";
      print_bdd (t.Then (), os, var_to_name);
      os << "||!" << var << "&&";
      print_bdd (t.Else (), os, var_to_name);
      os << ")";
    }
  }

  std::ostream& operator<< (std::ostream& os, const meta_bdd& b) {
    std::set<size_t> successors;

    os << b.state;

    auto trans = global_mmbdd.delta[b.state];

    if (global_mmbdd.is_accepting (b.state))
      os << "(acc)";

    os << ": ";

    print_bdd (trans, os, [&successors] (size_t varnum) {
      if (is_varnumstate (varnum))
        successors.insert (varnum_to_state (varnum));
      return varnum_to_name_state_aware (varnum);
    });

    os << "\n";

    for (auto&& state : successors)
      if (state != b.state)
        os << meta_bdd (state);

    return os;
  }

}

std::ostream& std::operator<< (std::ostream& os, const std::vector<sylvan::Bdd>& w) {
  bool first = true;

  for (auto&& l : w) {
    if (not first)
      os << ", ";
    first = false;
    MBDD::print_bdd (l, os);
  }
  return os;
}

#ifndef NDEBUG
[[maybe_unused]] static auto gdb_print_bdd (const sylvan::Bdd& bdd) {
  std::ostringstream buffer;
  MBDD::print_bdd (bdd, buffer);
  return buffer.str ();
}
#endif
