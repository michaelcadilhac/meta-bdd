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
    std::set<size_t> already_printed = {};
    b.print (os, already_printed);
    return os;
  }

  void meta_bdd::print (std::ostream& os, std::set<size_t>& already_printed) const {
    std::set<size_t> successors;

    os << state;
    already_printed.insert (state);


    auto trans = global_mmbdd.delta[state];

    if (global_mmbdd.is_accepting (state))
      os << "(acc)";

    os << ": ";

    // To prettify, remove the EMPTY state.
    print_bdd (trans.Compose (BddMap (VARNUM_EMPTY, Bdd::bddZero ())),
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
      meta_bdd (succ_state).print (os, already_printed);
  }

}
std::ostream& std::operator<< (std::ostream& os, const sylvan::Bdd& l) {
  MBDD::print_bdd (l, os);
  return os;
}

std::ostream& std::operator<< (std::ostream& os, const std::vector<sylvan::Bdd>& w) {
  bool first = true;

  for (auto&& l : w) {
    if (not first)
      os << ", ";
    first = false;
    os << l;
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
