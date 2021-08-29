#include <vector>
#include "upset.hh"

static std::map<size_t, MBDD::meta_bdd> bit_identities_cache;

using sylvan::Bdd;

static MBDD::meta_bdd bit_identities (size_t nbits) {
  auto cached = bit_identities_cache.find (nbits);
  if (cached != bit_identities_cache.end ())
    return cached->second;

  size_t to = 1 << nbits;
  Bdd full_trans = Bdd::bddZero ();

  for (size_t x = 0; x < to; ++x) {
    auto n = x;
    auto trans = Bdd::bddOne ();

    for (size_t i = 0; i < nbits; ++i) {
      auto var = Bdd::bddVar (i);
      auto var_mapped = Bdd::bddVar (i + nbits);
      if (n & 1)
        trans *= var * var_mapped;
      else
        trans *= !var * !var_mapped;
      n >>= 1;
    }
    full_trans += trans * MBDD::self ();
  }
  auto ret = MBDD::make (full_trans, true);
  bit_identities_cache[nbits] = ret;
  return ret;
}

/* This takes a meta_bdd s s.t. s[0,...,0]* = s, and returns a meta_bdd x such
 * that u[0,...,0] in x iff u in x. */
MBDD::meta_bdd upset::full_zero_padded (const MBDD::meta_bdd& s, Bdd all_zero) {
  if (s == MBDD::full () or s == MBDD::empty ())
    return s;

  bool should_be_accepting = s.accepts ({}), zero_seen = false;
  auto to_make = Bdd::bddZero ();

  for (auto&& [label, dest] : s.neighbors ()) {
    if (dest == s)
      to_make += label * MBDD::self ();
    else {
      auto new_dest = full_zero_padded (dest, all_zero);
      if (not should_be_accepting and
          not zero_seen and not (label & all_zero).isZero ()) {
        should_be_accepting = new_dest.accepts ({});
        zero_seen = true;
      }
      to_make += label * new_dest;
    }
  }
  return MBDD::make (to_make, should_be_accepting);
}

MBDD::meta_bdd upset::plus_transducer (const std::vector<size_t>& delta,
                                       const std::vector<bool>& neg,
                                       const std::vector<bool>& carries) {
  // Caching
  using tuple_t = std::tuple<std::remove_cvref_t<decltype (delta)>,
                             std::remove_cvref_t<decltype (neg)>,
                             std::remove_cvref_t<decltype (carries)>>;
  static auto pt_cache = std::map<tuple_t, MBDD::meta_bdd> ();
  auto cache_entry = std::make_tuple (delta, neg, carries);
  {
    auto cached = pt_cache.find (cache_entry);
    if (cached != pt_cache.end ())
      return cached->second;
  }
  auto cache = [&] (MBDD::meta_bdd ret) { return (pt_cache[cache_entry] = ret); };

  // Main algo
  auto size = delta.size ();
  auto delta_shifted = std::vector<size_t> (size);
  Bdd full_trans = Bdd::bddZero ();
  bool all_zero_delta = true, all_zero_carries = true;

  for (size_t i = 0; i < size; ++i) {
    delta_shifted[i] = delta[i] >> 1;
    if (delta[i])
      all_zero_delta = false;
    if (carries[i])
      all_zero_carries = false;
  }

  if (all_zero_delta and all_zero_carries) // Stopping condition.
    return cache (bit_identities (size));

  auto new_carries = std::vector<bool> (size);
  size_t to = 1 << size;

  if (all_zero_delta) { // Nothing more to add, but carries need to be propagated.
    for (size_t x = 0; x < to; ++x) {
      auto n = x;
      auto trans = Bdd::bddOne ();
      bool at_least_one_carry_consummed = false;

      for (size_t i = 0; i < size; ++i) {
        auto var = Bdd::bddVar (i);
        auto var_mapped = Bdd::bddVar (i + size);
        if (neg[i])
          std::swap (var, var_mapped);
        bool b = n & 1;
        n >>= 1;

        if (b) {
          if (carries[i]) { // 1 and carry
            trans *= var * !var_mapped; // 1 -> 0
            new_carries[i] = true;
          }
          else { // 1 and no carry
            trans *= var * var_mapped; // 1 -> 1
            new_carries[i] = false;
          }
        }
        else {
          if (carries[i]) { // 0 and carry
            trans *= !var * var_mapped; // 0 -> 1
            new_carries[i] = false;
            at_least_one_carry_consummed = true;
          }
          else { // 0 and no carry
            trans *= !var * !var_mapped;
            new_carries[i] = false;
          }
        }
      }
      if (at_least_one_carry_consummed)
        full_trans += trans * plus_transducer (delta, neg, new_carries);
      else
        full_trans += trans * MBDD::self ();
    }
    return cache (MBDD::make (full_trans, false));
  }

  for (size_t x = 0; x < to; ++x) {
    auto n = x;
    auto trans = Bdd::bddOne ();

    for (size_t i = 0; i < size; ++i) {
      auto var = Bdd::bddVar (i);
      auto var_mapped = Bdd::bddVar (i + size);
      if (neg[i])
        std::swap (var, var_mapped);
      bool b = n & 1;
      n >>= 1;

      if (b) {
        if (delta[i] & 1) { // 1 and +1
          if (carries[i]) { // 1 and +1 and carry
            trans *= var * var_mapped; // 1 -> 1
            new_carries[i] = true;
          }
          else { // 1 and +1 and no carry
            trans *= var * !var_mapped; // 1 -> 0
            new_carries[i] = true;
          }
        }
        else { // 1 and +0
          if (carries[i]) { // 1 and + 0 and carry
            trans *= var * !var_mapped; // 1 -> 0
            new_carries[i] = true;
          }
          else { // 1 and +0 and no carry
            trans *= var * var_mapped; // 1 -> 1
            new_carries[i] = false;
          }
        }
      }
      else {
        if (delta[i] & 1) { // 0 and + 1
          if (carries[i]) { // 0 and + 1 and carry
            trans *= !var * !var_mapped; // 0 -> 0
            new_carries[i] = true;
          }
          else { // 0 and +1 and no carry
            trans *= !var * var_mapped; // 0 -> 1
            new_carries[i] = false;
          }
        }
        else { // 0 and +0
          if (carries[i]) { // 0 and +0 and carry
            trans *= !var * var_mapped; // 0 -> 1
            new_carries[i] = false;
          }
          else { // 0 and +0 and no carry
            trans *= !var * !var_mapped; // 0 -> 0
            new_carries[i] = false;
          }
        }
      }
    }
    full_trans += trans * plus_transducer (delta_shifted, neg, new_carries);
  }
  return cache (MBDD::make (full_trans, false));
}
