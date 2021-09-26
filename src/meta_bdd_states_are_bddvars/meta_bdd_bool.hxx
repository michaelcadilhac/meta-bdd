#include <sstream>
#include <iostream>
#include <list>
#include <unordered_map>
#include <set>

#include "meta_bdd_states_are_bddvars/meta_bdd.hh"


namespace MBDD {


  template <typename MMBdd>
  template <typename Map, typename Hash, typename, typename EnabledOnlyIfMMBddIsNotConst>
  bmeta_bdd<MMBdd> bmeta_bdd<MMBdd>::intersection_union (const bmeta_bdd& other, bool intersection,
                                                       const Map& map, const Hash& map_hash) const {
    typedef std::tuple<size_t, size_t, bool, Hash> tuple_t;
    static std::map<tuple_t, size_t> iu_cache;

    size_t s1 = state, s2 = other.state;
    if (s1 > s2) { std::swap (s1, s2); }
    auto cache_tuple = std::make_tuple (s1, s2, intersection, map_hash);
    auto cached = iu_cache.find (cache_tuple);

    if (cached != iu_cache.end ())
      return bmeta_bdd (mmbdd, cached->second);

    auto cache = [&] (size_t s) {
      iu_cache[cache_tuple] = s;
      return bmeta_bdd (mmbdd, s);
    };

    constexpr bool nomap = std::is_same<Map, std::identity>::value;

    if (state == other.state)
      return cache (nomap ? state : apply (map).state);

    // Note: A map cannot turn STATE_EMPTY into anything else, but it can turn
    // STATE_FULL into a new state.
    if (intersection) {
      if (state == STATE_EMPTY or other.state == STATE_EMPTY)
        return cache (STATE_EMPTY);
      if (nomap) {
        if (state == STATE_FULL)
          return cache (other.state);
        if (other.state == STATE_FULL)
          return cache (state);
      }
    }
    else {
      if (nomap and (state == STATE_FULL or other.state == STATE_FULL))
        return cache (STATE_FULL);
      if (state == STATE_EMPTY)
        return cache (nomap ? other.state : other.apply (map).state);
      if (other.state == STATE_EMPTY)
        return cache (nomap ? state : apply (map).state);
    }

    // Compute the conjunction of deltas with the second primed.
    auto conj = mmbdd.delta[state] *
      mmbdd.delta[other.state].Compose (mmbdd.statevars_to_statevarsprime);

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
        conj_nostatevar_for_dest.UnivAbstract (mmbdd.statevars * mmbdd.statevarsprime);
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
            merge_state = bmeta_bdd (mmbdd, dest_states[0]).apply (map).state;
        }
        else
          // Looping
          if (((dest_states[0] == state and dest_states[1] == other.state) or
               (dest_states[1] == state and dest_states[0] == other.state)))
            merge_state = -1u; // We use -1 to signal SELF.
          else
            // Recursive call:
            merge_state =
              bmeta_bdd (mmbdd, dest_states[0])
              .intersection_union (bmeta_bdd (mmbdd, dest_states[1]),
                                   intersection, map)
              .state;

        // Build the new transition.
        auto this_label = map (conj_for_dest);
        if (merge_state == -1u /* SELF */ or (all_labels & this_label).isZero ()) {
          // If we reached here because we're self looping, check that we are
          // indeed deterministic.  This is for transductions, and is guaranteed
          // by post-unambiguity:
          __assert_verbose ((all_labels & this_label).isZero (),
                            {
                              std::cerr << "Transduction is post-ambiguous, labels are:\n"
                                        << "- " << this_label << "\n"
                                        << "- " << all_labels << "\n"
                                        << "in states: " << state << ", " << other.state
                                        << std::endl;
                            });
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
                (bmeta_bdd (mmbdd, merge_state) | bmeta_bdd (mmbdd, it->second)).state;
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

    return cache (
      mmbdd.make (target,
                  intersection ?
                  /*  */ mmbdd.is_accepting (state) and mmbdd.is_accepting (other.state) :
                  /*  */ mmbdd.is_accepting (state) or mmbdd.is_accepting (other.state)).state
      );
  }

  template <typename MMBdd>
  template <typename, typename EnabledOnlyIfMMBddIsNotConst>
  bmeta_bdd<MMBdd> bmeta_bdd<MMBdd>::operator& (const bmeta_bdd<MMBdd>& other) const {
    return intersection_union (other, true);
  }

  template <typename MMBdd>
  template <typename, typename EnabledOnlyIfMMBddIsNotConst>
  bmeta_bdd<MMBdd> bmeta_bdd<MMBdd>::operator| (const bmeta_bdd<MMBdd>& other) const {
    return intersection_union (other, false);
  }

  /* Transduction is seen as an intersection with projection. */
  template <typename MMBdd>
  template <typename, typename EnabledOnlyIfMMBddIsNotConst>
  bmeta_bdd<MMBdd> bmeta_bdd<MMBdd>::transduct (const bmeta_bdd& other,
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
      },
      *((size_t*) &m) // This is because BddMap doesn't expose the Bdd.
      );
  }
}
