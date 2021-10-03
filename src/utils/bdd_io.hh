#pragma once

#include <sylvan.h>
#include <sylvan_obj.hpp>
#include <iostream>
#include <string>

#include <labels/sylvanbdd.hh>
#include <labels/abcbdd.hh>

namespace utils {

  template <typename Bdd, typename F = std::string (size_t)>
  void print_bdd (const Bdd& t, std::ostream& os = std::cout, F var_to_name = std::to_string) {
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

  template <typename Bdd>
  struct bdd_io_traits {
      static constexpr auto printer = [] (const Bdd& bdd, std::ostream& os) {
        print_bdd (bdd, os);
      };
  };

}

namespace std {

  template <typename Bdd, typename T = decltype (Bdd::bddZero ())>
  ostream& operator<< (std::ostream& os, const Bdd& bdd) {
    utils::bdd_io_traits<Bdd>::printer (bdd, os);
    return os;
  }

  template <typename Bdd, typename T = decltype (Bdd::bddZero ())>
  ostream& operator<< (std::ostream& os, const std::vector<Bdd>& w) {
    bool first = true;

    for (auto&& l : w) {
      if (not first)
        os << ", ";
      first = false;
      os << l;
    }
    return os;
  }
}

#ifndef NDEBUG
#include <sstream>

template <typename Bdd, typename T = decltype (Bdd::bddZero ())>
[[maybe_unused]] static auto gdb_print_bdd (const Bdd& bdd) {
  std::ostringstream buffer;
  buffer << bdd;
  return buffer.str ();
}

template
auto gdb_print_bdd<sylvan::Bdd> (const sylvan::Bdd&);

template
auto gdb_print_bdd<utils::abcbdd> (const utils::abcbdd&);

#endif
