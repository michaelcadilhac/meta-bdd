#include <vector>

#include <utils/cache.hh>
#include <sylvan.h>
#include <sylvan_obj.hpp>
#include <upset/upset_bdd.hh>

#warning yi impair xi pair

namespace upset {
  using sylvan::Bdd;

  template <typename Bdd, typename StateType>
  using mmbdd_t = MBDD::master_meta_bdd<Bdd, StateType>;

  template <typename Bdd, typename StateType>
  auto upset<mmbdd_t<Bdd, StateType>>::bit_identities (size_t nbits) const {
    static auto cache = utils::make_cache<meta_bdd> (nbits);
    auto cached = cache.get (nbits);
    if (cached) return *cached;

    Bdd full_trans = Bdd::bddOne ();
    while (nbits--) {
      auto var = Bdd::bddVar (2 * nbits);
      auto var_mapped = Bdd::bddVar (2 * nbits + 1);
      full_trans *= !(var ^ var_mapped);
    }
    return cache (mmbdd.make (full_trans * mmbdd.self (), true), nbits);
  }

  /* This takes a meta_bdd s s.t. s[0,...,0]* = s, and returns a meta_bdd x such
   * that u[0,...,0] in x iff u in x. */
  // Maybe todo: see if we can restrict to just a few dimensions
  template <typename Bdd, typename StateType>
  auto upset<mmbdd_t<Bdd, StateType>>::full_zero_padded (const meta_bdd& s, Bdd all_zero) const {
    static auto cache = utils::make_cache<meta_bdd, typename master_meta_bdd::state_t, int> ();

    auto cached = cache.get (s, all_zero.GetBDD ());
    if (cached)
      return *cached;

    if (s == mmbdd.full () or s == mmbdd.empty ())
      return cache (s, s, all_zero.GetBDD ());

    bool should_be_accepting = s.accepts ({}), zero_seen = false;
    auto to_make = transition_type ();

    for (auto&& [dest, labels] : s.neighbors ()) {
      if (dest == s)
        to_make += labels * mmbdd.self ();
      else {
        auto new_dest = full_zero_padded (dest, all_zero);
        if (not should_be_accepting and
            not zero_seen and not (labels & all_zero).isZero ()) {
          should_be_accepting = new_dest.accepts ({});
          zero_seen = true;
        }
        to_make += labels * new_dest;
      }
    }
    return cache (mmbdd.make (to_make, should_be_accepting), s, all_zero.GetBDD ());
  }

    template <typename Bdd, typename StateType>
    auto upset<mmbdd_t<Bdd, StateType>>::plus_transducer_one_dim (size_t idx, size_t dim,
                                                                  upset::value_type delta,
                                                                  bool neg, bool carry,
                                                                  Bdd untouched_components) const {
#define local_args idx, dim, delta, neg, carry
    static auto cache = utils::make_cache<meta_bdd> (local_args);
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

    auto dest_nocarry = mmbdd.full (), dest_carry = mmbdd.full ();
    if (delta == 0) {
      if (not carry) {
        dest_nocarry = mmbdd.self ();
        /* dest_carry unused */
      }
      else {
        dest_nocarry = plus_transducer_one_dim (idx, dim, 0, neg, false, untouched_components);
        dest_carry = mmbdd.self ();
      }
    }
    else {
      if (not (b and carry)) /* otherwise, a carry must be generated */
        dest_nocarry = plus_transducer_one_dim (idx, dim, delta >> 1, neg, false, untouched_components);
      if (b or carry) /* otherwise, no carry can be generated */
        dest_carry = plus_transducer_one_dim (idx, dim, delta >> 1, neg, true, untouched_components);
    }

    transition_type trans;
    if (b and carry)     // + 2
      trans = !(var ^ var_mapped) * untouched_components * dest_carry;
    else if (b or carry) // + 1
      trans = var * !var_mapped * untouched_components * dest_carry +
        !var * var_mapped * untouched_components * dest_nocarry;
    else                 // + 0
      trans = !(var ^ var_mapped) * untouched_components * dest_nocarry;

    return cache (mmbdd.make (trans, false), local_args);
#undef local_args
  }


  template <typename Bdd, typename StateType>
  auto upset<mmbdd_t<Bdd, StateType>>::plus_transducer (
    const std::vector<upset::value_type>& delta,
    const std::vector<bool>& neg,
    const std::vector<bool>& carries) const {
    static auto cache = utils::make_cache<meta_bdd> (delta, neg, carries);
    auto cached = cache.get (delta, neg, carries);
    if (cached)
      return *cached;

    // Main algo
    auto size = delta.size ();
    auto delta_shifted = std::vector<upset::value_type> (size);
    transition_type full_trans;
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
          full_trans += trans * mmbdd.self ();
      }
      return cache (mmbdd.make (full_trans, false), delta, neg, carries);
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
    return cache (mmbdd.make (full_trans, false), delta, neg, carries);
  }

  template <typename Bdd, typename StateType>
  upset<mmbdd_t<Bdd, StateType>>::upset (master_meta_bdd& mmbdd,
                                         const std::vector<value_type>& v) :
    mmbdd {mmbdd}, mbdd {mmbdd.full ()}, dim {v.size ()} {
    static auto cache = utils::make_cache<meta_bdd> (v);
    auto cached = cache.get (v);
    if (cached) {
      mbdd = *cached;
      return;
    }

    for (size_t i = 0; i < dim; ++i)
      mbdd = mbdd & up_mbdd_high_branch (v[i], i);

    cache (mbdd, v);
  }

  template <typename Bdd, typename StateType>
  std::ostream& operator<< (std::ostream& out, const upset<mmbdd_t<Bdd, StateType>>& u) {
    using value_t = upset<mmbdd_t<Bdd, StateType>>::value_type;
    std::set<std::vector<value_t>> snapshot, done;
    snapshot.emplace (u.dim, 1);

    while (not snapshot.empty ()) {
      auto v = snapshot.extract (snapshot.begin ()).value ();
      if (done.contains (v))
        continue;
      done.insert (v);
      bool min_elt = true;
      for (size_t i = 0; i < u.dim; ++i) {
        if (v[i] > 0) {
          v[i]--;
          if (u.contains (v)) {
            min_elt = false;
            snapshot.insert (v);
          }
          v[i]++;
        }
      }
      if (min_elt) {
        out << '[';
        std::ranges::copy(v, std::ostream_iterator<value_t>(out, ", "));
        out << "\b\b] ";
      }
    }
    return out;
  }

}
