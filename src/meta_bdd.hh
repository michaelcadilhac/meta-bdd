#pragma once

#include <span>
#include <set>
#include <vector>
#include <map>
#include <functional>

#include <iostream>
#include <cassert>

#ifdef NDEBUG
#define __assert_verbose(Cond, F)
#else
#define __assert_verbose(Cond, F)               \
  do {                                          \
    bool assertion = (Cond);                    \
    if (not assertion) {                        \
      F;                                        \
      assert (((void) #Cond, false));           \
    }                                           \
  } while (0)
#endif

namespace MBDD {
  // tags
  struct states_are_bddvars;
  struct states_are_ints;

  enum constant_states { STATE_SELF = 0ul, STATE_EMPTY, STATE_FULL };

  template <typename LetterSet, typename StateType>
  class master_meta_bdd;

  template <typename LetterSet, typename StateType>
  auto make_master_meta_bdd () {
    return master_meta_bdd<LetterSet, StateType> ();
  }
}
