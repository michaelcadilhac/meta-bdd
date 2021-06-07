#include <sstream>
#include <iostream>
#include <set>

#include "meta_bdd.hh"


namespace MBDD {
  master_meta_bdd global_mmbdd;

  void init () {
    global_mmbdd.init ();
  }

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

  meta_bdd meta_bdd::intersection_union (const meta_bdd& other, bool intersection) const {
    if (state == other.state)
      return state;

    if (intersection) {
      if (state == STATE_EMPTY or other.state == STATE_EMPTY)
        return STATE_EMPTY;
      if (state == STATE_FULL)
        return other.state;
      if (other.state == STATE_FULL)
        return state;
    }
    else {
      if (state == STATE_FULL or other.state == STATE_FULL)
        return STATE_FULL;
      if (state == STATE_EMPTY)
        return other.state;
      if (other.state == STATE_EMPTY)
        return state;
    }

    // Compute the conjunction of deltas with SELF replaced by the correct state.
    auto conj = global_mmbdd.self_to_state (global_mmbdd.delta[state], state) *
      global_mmbdd.self_to_state (global_mmbdd.delta[other.state], other.state)
      /* */.Compose (global_mmbdd.statevars_to_statevarsprime);

    auto to_make = Bdd::bddZero ();

    // All satisfying assignments should have two state variables.
    while (conj != Bdd::bddZero ()) {
      auto one_dest = conj.PickOneCube ();
      // Now extract the two states that are True in one_dest
      size_t dest_states[2] = {-1ul, -1ul};
      while (true) {
        assert (not one_dest.isTerminal ());
        if (one_dest.Then ().isZero ()) { // Negative literal
          one_dest = one_dest.Else ();
          continue;
        }
        auto topvar = one_dest.TopVar ();
        one_dest = one_dest.Then ();
        if (is_varnumstate (topvar)) {
          auto pos = is_varnumstate (topvar, Prime::yes) ? 1 : 0;
          assert (dest_states[pos] == -1ul);
          dest_states[pos] = varnum_to_state (topvar);
          if (dest_states[pos ^ 1] != -1ul) break;
        }
      }

      Bdd merge_statebdd;
      // Going to the same state, no need to recurse.
      if (dest_states[0] == dest_states[1])
        merge_statebdd = state_to_bddvar (dest_states[0]);
      else
        // Looping
        if ((dest_states[0] == state and dest_states[1] == other.state) or
            (dest_states[1] == state and dest_states[2] == other.state))
          merge_statebdd = BDDVAR_SELF;
        else
          // Recursive call:
          merge_statebdd = meta_bdd (dest_states[0]).intersection_union (meta_bdd (dest_states[1]), intersection);

      // Reconstruct the pair q * q'
      auto destination_bdd = state_to_bddvar (dest_states[0]) *
        state_to_bddvar (dest_states[1], Prime::yes);
      // Project the user variables compatible with destination_bdd so that
      // these have no state dependency.
      auto conj_nostatevar_for_dest = conj.ExistAbstract (destination_bdd);
      // Remove the user variables with state dependency.  We need the prime
      // states since we projected out dest_states[0] unprimed.
      auto conj_for_dest =
        conj_nostatevar_for_dest.UnivAbstract (global_mmbdd.statevars * global_mmbdd.statevarsprime);
      // Build the new transition.
      auto new_dest = conj_for_dest * merge_statebdd;
      to_make += new_dest;

      // Remove this transition (pair) from conj.
      conj -= conj_for_dest;
    }

    return
      global_mmbdd.make (to_make,
                         intersection ?
                         /*  */ global_mmbdd.is_accepting (state) and global_mmbdd.is_accepting (other.state) :
                         /*  */ global_mmbdd.is_accepting (state) or global_mmbdd.is_accepting (other.state));
  }

  meta_bdd meta_bdd::operator& (const meta_bdd& other) const {
    return intersection_union (other, true);
  }

  meta_bdd meta_bdd::operator| (const meta_bdd& other) const {
    return intersection_union (other, false);
  }

}

#ifndef NDEBUG
[[maybe_unused]] static auto gdb_print_bdd (const sylvan::Bdd& bdd) {
  std::ostringstream buffer;
  MBDD::print_bdd (bdd, buffer);
  return buffer.str ();
}
#endif
