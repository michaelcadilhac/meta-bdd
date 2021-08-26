#include <sstream>
#include <iostream>
#include <list>
#include <set>

#include "meta_bdd.hh"


namespace MBDD {

  template <typename Map>
  meta_bdd meta_bdd::intersection_union (const meta_bdd& other, bool intersection,
                                         Map map) const {
    constexpr bool nomap = std::is_same<Map, std::identity>::value;

    if (state == other.state)
      return nomap ? state : apply (map);

    // Note: A map cannot turn STATE_EMPTY into anything else, but it can turn
    // STATE_FULL into a new state.
    if (intersection) {
      if (state == STATE_EMPTY or other.state == STATE_EMPTY)
        return STATE_EMPTY;
      if (nomap) {
        if (state == STATE_FULL)
          return other.state;
        if (other.state == STATE_FULL)
          return apply (map);
      }
    }
    else {
      if (nomap and (state == STATE_FULL or other.state == STATE_FULL))
        return STATE_FULL;
      if (state == STATE_EMPTY)
        return nomap ? other.state : other.apply (map);
      if (other.state == STATE_EMPTY)
        return nomap ? state : apply (map);
    }

    // Compute the conjunction of deltas with the second primed.
    auto conj = global_mmbdd.delta[state] *
      global_mmbdd.delta[other.state].Compose (global_mmbdd.statevars_to_statevarsprime);

    auto to_make = std::list<std::pair<Bdd, size_t>> ();
    auto all_labels = Bdd::bddZero ();

#warning Another (better?) approach would be to project delta[state] over statevars, iterate over states, project to the nonstatevars over that state, make the and with delta[other.state], project over state, etc.

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
      // Remove this transition (pair) from conj.
      conj -= conj_for_dest;

      // Do not add this to the target to be made if this is intersection with
      // empty.  This is not only a basic optimization, but some algorithms
      // expect this (transductions).
      if ((not intersection) or
          (intersection and dest_states[0] != STATE_EMPTY and dest_states[1] != STATE_EMPTY)) {
        size_t merge_state;
        // Going to the same state, no need to recurse if we're not changing labels
        if (dest_states[0] == dest_states[1]) {
          if (nomap)
            merge_state = dest_states[0];
          else
            merge_state = meta_bdd (dest_states[0]).apply (map).state;
        }
        else
          // Looping
          if (((dest_states[0] == state and dest_states[1] == other.state) or
               (dest_states[1] == state and dest_states[0] == other.state)))
            merge_state = -1u; // We use -1 to signal SELF.
          else
            // Recursive call:
            merge_state =
              meta_bdd (dest_states[0])
              .intersection_union (meta_bdd (dest_states[1]),
                                   intersection, map)
              .state;

        // Build the new transition.
        auto this_label = map (conj_for_dest);
        if (merge_state == -1u /* SELF */ or (all_labels & this_label).isZero ()) {
          // If we reached here because we're self looping, check that we are
          // indeed deterministic.  This is for transductions, and is guaranteed
          // by post-unambiguity:
          assert (((void) "Transduction is post-ambiguous.",
                   (all_labels & this_label).isZero ()));
          to_make.emplace_back (this_label, merge_state);
        }
        else {
          // Remove the conflict
          for (auto it = to_make.begin (); it != to_make.end (); ++it) {
            auto conj = this_label & it->first;
            if (not conj.isZero ()) {
              // Because of transductions, it can happen that it->second is
              // SELF, but this is ruled out by post-unambiguity.
              assert (((void) "Transduction is post-ambiguous.",
                       it->second != -1u));
              auto union_merge_state =
                /* union, no map, as this is after the map has been applied. */
                (meta_bdd (merge_state) | meta_bdd (it->second)).state;
              to_make.erase (it);
              to_make.emplace_back (conj, union_merge_state);
              conj = this_label & (!it->first);
              if (not conj.isZero ())
                to_make.emplace_back (conj, merge_state);
              conj = (!this_label) & it->first;
              if (not conj.isZero ())
                to_make.emplace_back (conj, it->second);
              break;
            }
          }
        }
        all_labels += this_label;
      }
    }

    Bdd target = Bdd::bddZero ();
    for (auto&& p : to_make)
      target += p.first * (p.second == -1u ? BDDVAR_SELF : state_to_bddvar (p.second));

    return
      global_mmbdd.make (target,
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

  /* Transduction is seen as an intersection with projection. */
  meta_bdd meta_bdd::transduct (const meta_bdd& other,
                                std::span<const Bdd> output_vars,
                                std::span<const Bdd> to_vars) const {
    assert (output_vars.size () == to_vars.size () and output_vars.size () != 0);
    // Construct the map output_var -> to_var.
    BddMap m;
    Bdd bddoutputvars = Bdd::bddOne ();
    for (size_t i = 0; i < output_vars.size (); ++i) {
      m.put (output_vars[i].TopVar (), to_vars[i]);
      bddoutputvars *= output_vars[i];
    }

    return intersection_union (
      other, true,
      [&bddoutputvars, &m] (const Bdd& b) {
        return Bdd (sylvan_project (b.GetBDD (), bddoutputvars.GetBDD ())).Compose (m);
      });
  }
}
