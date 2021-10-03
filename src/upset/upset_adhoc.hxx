#include <vector>

#include <utils/cache.hh>
#include <upset/upset_bdd.hh>

#warning yi impair xi pair

namespace upset {
  using sylvan::Bdd;

  template <typename Bdd, typename StateType>
  using mmbdd_t = MBDD::master_meta_bdd<Bdd, StateType>;

  template <typename Bdd, typename StateType>
  template <typename Flip>
  auto upset_adhoc<mmbdd_t<Bdd, StateType>>::plus_one_dim (
    const meta_bdd& s, const Bdd& var, Flip flip,
    const Bdd& all_other_zeros,
    value_type delta, bool neg, bool carry) const {
#define local_args s, var, dim, delta, neg, carry
    static auto cache = utils::make_cache<std::pair<bool, meta_bdd>> (local_args);
    auto cached = cache.get (local_args);
    if (cached)
      return *cached;

    if (delta == 0 and not carry)
      return cache (std::pair {s.accepts ({}), s}, local_args);

    transition_type trans;
    bool zero_reach = false;
    bool b = delta & 1;

    for (auto&& [dest, labels] : s.neighbors ()) {
      const auto dest_carry = [&] (bool carry, bool reaching_with_zero) {
        if (delta == 0 and dest == s and carry) // self-loop waiting for carry
          return mmbdd.self ();
        auto&& [zr, mbdd] = plus_one_dim (dest, var, flip, all_other_zeros, delta >> 1, neg, carry);
        if (reaching_with_zero)
          zero_reach = zero_reach or zr;
        return mbdd;
      };

      if (b and carry)     // + 2
        trans += labels * dest_carry (true, not (labels * !var * all_other_zeros).isZero ()); // Labels are preserved
      else if (b or carry) { // + 1
        auto var_true =  labels *  var;
        auto var_false = labels * !var;
        if (not var_true.isZero ())
          trans += flip (var_true) * dest_carry (not neg, not (all_other_zeros * var_true).isZero ());
        if (not var_false.isZero ())
          trans += flip (var_false) * dest_carry (neg, false);
      }
      else               // + 0
        trans += labels * dest_carry (false, not (labels * !var * all_other_zeros).isZero ()); // Labels are preserved
    }
    return cache (std::pair {zero_reach, mmbdd.make (trans, zero_reach)}, local_args);
  }

  template <typename Bdd, typename StateType>
  upset_adhoc<mmbdd_t<Bdd, StateType>>::upset_adhoc (master_meta_bdd& mmbdd,
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
  std::ostream& operator<< (std::ostream& out, const upset_adhoc<mmbdd_t<Bdd, StateType>>& u) {
    using value_t = upset_adhoc<mmbdd_t<Bdd, StateType>>::value_type;
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
