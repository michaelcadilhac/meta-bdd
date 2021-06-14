#pragma once

namespace MBDD {

  // These implementations are moved mostly because they talk about the global MMBDD.
  inline bool meta_bdd::accepts (std::span<const Bdd> w) const {
    auto cur_state = state;
    for (auto&& l : w) {
      cur_state = global_mmbdd.successor (cur_state, l);
      if (cur_state == STATE_FULL)
        return true;
      if (cur_state == STATE_EMPTY)
        return false;
    }
    return global_mmbdd.is_accepting (cur_state);
  }

  inline meta_bdd meta_bdd::one_step (const Bdd& l) const {
    return meta_bdd (global_mmbdd.successor (state, l));
  }

  inline auto master_meta_bdd::iterator::operator++ () {
    if (++state == global_mmbdd.delta.size ())
      state = -1u;
    return *this;
  }
}
