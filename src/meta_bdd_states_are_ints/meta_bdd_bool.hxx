#include <sstream>
#include <iostream>
#include <list>
#include <unordered_map>
#include <set>

#include <utils/cache.hh>
#include <meta_bdd_states_are_ints/meta_bdd.hh>


namespace MBDD {


  template <typename MMBdd>
  template <typename Map, typename Hash, typename, typename EnabledOnlyIfMMBddIsNotConst>
  imeta_bdd<MMBdd> imeta_bdd<MMBdd>::intersection_union (const imeta_bdd& other, bool intersection,
                                                         const Map& map, const Hash& map_hash) const {
    size_t s1 = state, s2 = other.state;
    if (s1 > s2) { std::swap (s1, s2); }

#define local_args s1, s2, intersection, map_hash
    static auto iu_cache = utils::make_cache<state_t> (local_args);

    auto cached = iu_cache.get (local_args);
    if (cached)
      return imeta_bdd (mmbdd, *cached);

    auto cache = [&] (size_t s) {
      return imeta_bdd (mmbdd, iu_cache (s, local_args));
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

    std::map<state_t, letter_set_type> m;
    letter_set_type all_labels;

    for (auto&& [dest1, labels1] : mmbdd.delta[s1])
      for (auto&& [dest2, labels2] : mmbdd.delta[s2]) {
        letter_set_type conj = labels1 * labels2;

        if (conj.empty ())
          continue;
        letter_set_type this_label = map (conj);

        // Do not add this to the target to be made if this is intersection with
        // empty.  This is not only a basic optimization, but some algorithms
        // expect this (transductions).
        if ((not intersection) or
            (intersection and dest1 != STATE_EMPTY and dest2 != STATE_EMPTY)) {
          size_t merge_state;
          // Going to the same state, no need to recurse if we're not changing labels
          if (dest1 == dest2) {
            if (nomap)
              merge_state = dest1;
            else
              merge_state = imeta_bdd (mmbdd, dest1).apply (map).state;
          }
          else
            // looping
            if (((dest1 == state and dest2 == other.state) or
                 (dest2 == state and dest1 == other.state)))
              merge_state = STATE_SELF;
            else // recursion
              merge_state =
                imeta_bdd (mmbdd, dest1)
                .intersection_union (imeta_bdd (mmbdd, dest2),
                                     intersection, map)
                .state;
          if (merge_state == STATE_SELF or (all_labels * this_label).empty ()) {
            // If we reached here because we're self looping, check that we are
            // indeed deterministic.  This is for transductions, and is guaranteed
            // by post-unambiguity:
            __assert_verbose ((all_labels * this_label).empty (),
                              {
                                std::cerr << "Transduction is post-ambiguous, labels are:\n"
                                          << "- " << this_label << "\n"
                                          << "- " << all_labels << "\n"
                                          << "in states: " << state << ", " << other.state
                                          << std::endl;
                              });
            m[merge_state] |= this_label;
          }
          else {
            // Remove the conflict.
            for (auto&& it = m.begin (); it != m.end (); ++it) {
              auto&& [only_this, common, only_other] = this_label.partition (it->second);
              if (not common.empty ()) {
                // Because of transductions, it can happen that it->second is
                // SELF, but this is ruled out by post-unambiguity.
                __assert_verbose (it->first != STATE_SELF,
                                  std::cerr << "Transduction is post-ambiguous\n");
                auto union_merge_state =
                  /* union, no map, as this is after the map has been applied. */
                  (imeta_bdd (mmbdd, merge_state) | imeta_bdd (mmbdd, it->first)).state;
                auto node = m.extract (it);
                m[union_merge_state] |= common;
                if (not only_this.empty ())
                  m[merge_state] |= only_this;
                if (not only_other.empty ())
                  m[node.key ()] |= only_other;
                break;
              }
            }
          }
          all_labels += this_label;
        }
      }

    return cache (
      mmbdd.make (transition (m),
                  intersection ?
                  /*  */ mmbdd.is_accepting (state) and mmbdd.is_accepting (other.state) :
                  /*  */ mmbdd.is_accepting (state) or mmbdd.is_accepting (other.state)).state
      );
#undef local_args
  }

  template <typename MMBdd>
  template <typename, typename EnabledOnlyIfMMBddIsNotConst>
  imeta_bdd<MMBdd> imeta_bdd<MMBdd>::operator& (const imeta_bdd<MMBdd>& other) const {
    return intersection_union (other, true);
  }

  template <typename MMBdd>
  template <typename, typename EnabledOnlyIfMMBddIsNotConst>
  imeta_bdd<MMBdd> imeta_bdd<MMBdd>::operator| (const imeta_bdd<MMBdd>& other) const {
    return intersection_union (other, false);
  }

  /* Transduction is seen as an intersection with projection. */
  template <typename MMBdd>
  template <typename Map, typename Hash, typename, typename EnabledOnlyIfMMBddIsNotConst>
  imeta_bdd<MMBdd> imeta_bdd<MMBdd>::transduct (const imeta_bdd& other,
                                                Map map, const Hash& hash) const {
    return intersection_union (other, true, map, hash);
  }
}
