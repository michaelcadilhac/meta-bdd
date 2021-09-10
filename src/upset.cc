#include <vector>

#include "cache.hh"
#include "upset.hh"

#warning yi impair xi pair


using sylvan::Bdd;

static MBDD::meta_bdd bit_identities (size_t nbits) {
  static auto cache = MBDD::make_cache<MBDD::meta_bdd> (nbits);
  auto cached = cache.get (nbits);
  if (cached) return *cached;

  Bdd full_trans = Bdd::bddOne ();
  while (nbits--) {
    auto var = Bdd::bddVar (2 * nbits);
    auto var_mapped = Bdd::bddVar (2 * nbits + 1);
    full_trans *= !(var ^ var_mapped);
  }
  return cache (MBDD::make (full_trans * MBDD::self (), true), nbits);
}

/* This takes a meta_bdd s s.t. s[0,...,0]* = s, and returns a meta_bdd x such
 * that u[0,...,0] in x iff u in x. */
MBDD::meta_bdd upset::full_zero_padded (const MBDD::meta_bdd& s, Bdd all_zero) {

  static auto cache = MBDD::make_cache<MBDD::meta_bdd, sylvan::BDD, sylvan::BDD> ();

  auto cached = cache.get (Bdd (s).GetBDD (), all_zero.GetBDD ());
  if (cached)
    return *cached;

  if (s == MBDD::full () or s == MBDD::empty ())
    return cache (s, Bdd (s).GetBDD (), all_zero.GetBDD ());

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
  return cache (MBDD::make (to_make, should_be_accepting), Bdd (s).GetBDD (), all_zero.GetBDD ());
}

 MBDD::meta_bdd upset::plus_transducer_one_dim (size_t idx, size_t dim,
                                                size_t delta,
                                                bool neg, bool carry,
                                                Bdd untouched_components) {
#define local_args idx, dim, delta, neg, carry
   static auto cache = MBDD::make_cache<MBDD::meta_bdd> (local_args);
   auto cached = cache.get (idx, dim, delta, neg, carry);
   if (cached)
     return *cached;

   if (delta == 0 and carry == 0)
     return cache (bit_identities (dim), local_args);

   auto var = Bdd::bddVar (2 * idx);
   auto var_mapped = Bdd::bddVar (2 * idx + 1);
   if (neg)
     std::swap (var, var_mapped);
   bool b = delta & 1;

   Bdd trans_nocarry = MBDD::full (), trans_carry = MBDD::full ();
   if (delta == 0) {
     if (not carry) {
       trans_nocarry = MBDD::self ();
       /* trans_carry unused */
     }
     else {
       trans_nocarry = plus_transducer_one_dim (idx, dim, 0, neg, false, untouched_components);
       trans_carry = MBDD::self ();
     }
   }
   else {
     if (not (b and carry)) /* otherwise, a carry must be generated */
       trans_nocarry = plus_transducer_one_dim (idx, dim, delta >> 1, neg, false, untouched_components);
     if (b or carry) /* otherwise, no carry can be generated */
       trans_carry = plus_transducer_one_dim (idx, dim, delta >> 1, neg, true, untouched_components);
   }

   auto trans = Bdd::bddOne ();
   if (b and carry)     // + 2
     trans = !(var ^ var_mapped) * trans_carry;
   else if (b or carry) // + 1
     trans = var * !var_mapped * trans_carry +
       !var * var_mapped * trans_nocarry;
   else                 // + 0
     trans = !(var ^ var_mapped) * trans_nocarry;

   return cache (MBDD::make (trans * untouched_components, false), local_args);
#undef local_args
 }




MBDD::meta_bdd upset::plus_transducer (
  const std::vector<size_t>& delta,
  const std::vector<bool>& neg,
  const std::vector<bool>& carries) {
  static auto cache = MBDD::make_cache<MBDD::meta_bdd> (delta, neg, carries);
  auto cached = cache.get (delta, neg, carries);
  if (cached)
    return *cached;

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
    return cache (bit_identities (size), delta, neg, carries);

  auto new_carries = std::vector<bool> (size);
  size_t to = 1 << size;

  if (all_zero_delta) { // Nothing more to add, but carries need to be propagated.
    for (size_t x = 0; x < to; ++x) {
      auto n = x;
      auto trans = Bdd::bddOne ();
      bool at_least_one_carry_consummed = false;

      for (size_t i = 0; i < size; ++i) {
        auto var = Bdd::bddVar (2 * i);
        auto var_mapped = Bdd::bddVar (2 * i + 1);
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
    return cache (MBDD::make (full_trans, false), delta, neg, carries);
  }

  for (size_t x = 0; x < to; ++x) {
    auto n = x;
    auto trans = Bdd::bddOne ();

    for (size_t i = 0; i < size; ++i) {
      auto var = Bdd::bddVar (2 * i);
      auto var_mapped = Bdd::bddVar (2 * i + 1);
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
  return cache (MBDD::make (full_trans, false), delta, neg, carries);
}
