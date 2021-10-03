#include <meta_bdd.hh>
#include <meta_bdd_states_are_bddvars/meta_bdd.hh>
#include <labels/sylvanbdd.hh>
#include <utils/transduct_bdd.hh>
#include <utils/bdd_io.hh>

using utils::transduct;

#include "tests.hh"


auto mmbdd = MBDD::make_master_meta_bdd<labels::sylvanbdd, MBDD::states_are_bddvars> ();
using mmbdd_t = decltype (mmbdd);
using Bdd = mmbdd_t::letter_set_type;

auto flat_automaton (std::span<const Bdd> w) {
  assert (w.size () > 0);
  if (w.size () == 1) // End of the list.
    return mmbdd.make (w[0] * mmbdd.self (), true);
  return mmbdd.make (w[0] * mmbdd.self () | w[1] * flat_automaton (w.subspan (2)), false);
}


auto flat_automaton (std::initializer_list<Bdd> w) {
  return flat_automaton (std::span (w));
}

int main () {
  // Initialize sylvan
  lace_start(0, 0);
  sylvan::sylvan_set_sizes (1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
  sylvan::sylvan_init_package ();
  sylvan::sylvan_init_bdd ();

  // Initialize MBDD
  mmbdd.init ();

  auto x0 = Bdd::bddVar (0),
    x1 = Bdd::bddVar (1);

  auto qfull = mmbdd.make (x0 * mmbdd.self () + !x0 * mmbdd.full (), true);
  test (qfull == mmbdd.full ());

  auto qempty = mmbdd.make ((x0 * x1) * mmbdd.empty () + !(x0 * x1) * mmbdd.self (), false);
  test (qempty == mmbdd.empty ());

  test (mmbdd.make (mmbdd.full (), true) == mmbdd.full ());
  test (mmbdd.make (mmbdd.self (), true) == mmbdd.full ());
  test (mmbdd.make (mmbdd.empty (), false) == mmbdd.empty ());
  test (mmbdd.make (mmbdd.self (), false) == mmbdd.empty ());

  auto q = mmbdd.make ((x0 * !x1) * mmbdd.full (), false);

  std::cout << q;
  std::cout << "q accepts " << q.one_word ()
            << " and rejects " << q.one_word (false) << std::endl;

  test (q.accepts ({ x0 * !x1 }));
  test (q.accepts ({ x0 * !x1, x0 * x1, !x1 }));
  test (not q.accepts ({ !x0 * x1 }));
  test (not q.accepts ({ x0 * x1, x0 * x1, !x1 }));

  test (mmbdd.full ().accepts ({ x0, x0, x0, x0 }));
  test (not mmbdd.empty ().accepts ({ x0, x0, x0, x0 }));

  auto q1 = flat_automaton ({x0, !x0, Bdd::bddZero (), x1, x1});
  std::cout << q1;

  test (q1.accepts ({ !x0 * x1, x1 }));
  test (q1.accepts (
            {   x0 * !x1, x0 * x1, !x0 * x1, // First state
                x1 * x0, // Second state
                x1 * x0, x1 * !x0, x1 * x0
            }));
  test (not q1.accepts (
            {   !x0 * !x1,
                !x0 * !x1
            }));

  auto q2 = flat_automaton ({x0, !x0, Bdd::bddZero (), x1, x1});
  test (q1 == q2);
  test (q1.one_step (!x0) == q2.one_step (!x0));

  auto q3 = flat_automaton ({x0, !x0, (x0 * x1) | (!x0 * !x1) | (x0 * !x1) | (!x0 * x1)});
  test (q3.one_step (!x0) == mmbdd.full ());

  std::cout << "q2\n" << q2;
  std::cout << "q1 & q2\n" << (q1 & q2);

  test ((q1 & q2) == q2);
  test ((q1 & q1) == q1);
  test ((q1 & q2) == (q2 & q1));

  auto q4 = flat_automaton ({Bdd::bddZero (), !x0, !x1, x1, !x1, x1, Bdd::bddZero ()});

  std::cout << "q4 accepts " << q4.one_word ()
            << " and rejects " << q4.one_word (false) << std::endl;

  test (q4.accepts ( { !x0, x1, x1 } ));

  test (not q4.accepts ( { x0, !x0, !x1, x1, x1 } ));

  auto q5 = q1 & q4;

  test (q5.accepts ({ !x0, x1, x1 }));
  test (not q5.accepts ({ !x0 * x1, x1 })); // in q1 not q4
  test (not q5.accepts ({ !x0 * x1, !x1 * x0, x1, x1 })); // in q4 not q1

  test (q5 == (q5 & mmbdd.full ()));
  test (mmbdd.empty () == (q5 & mmbdd.empty ()));

  test ((q4 & q) == mmbdd.empty ());

  auto q5p = mmbdd.make (x0 * mmbdd.full () + !x0 * mmbdd.self (), false);
  auto q5pp = mmbdd.make (x0 * mmbdd.full () + (x1 * !x0) * q2 + (!x1 * !x0) * mmbdd.self (), false);
  auto q5pi = q5p & q5pp;
  std::cout << "q5pi: " << q5pi;
  test (q5pi == (q5pp & q5p));

  auto q6 = mmbdd.make (!(x0 * !x1) * mmbdd.full (), false);
  auto q7 = mmbdd.make (mmbdd.full (), false);

  test ((q | q6) == q7);
  test ((q | q6) == (q6 | q));

  // Print a word per state
  for (auto&& state : mmbdd) {
    if (state == mmbdd.self ())
      continue;
    std::cout << "State";
    std::cout << state;
    if (state != mmbdd.empty ())
      std::cout << " accepts [" << state.one_word () << "]";
    if (state != mmbdd.full ())
      std::cout << " rejects [" <<  state.one_word (false) << "]";
    std::cout << std::endl;
  }

  // Transductions
  std::cout << "TRANSDUCTIONS." << std::endl;

  auto x2 = Bdd::bddVar (2), x3 = Bdd::bddVar (3);
  auto ttr = mmbdd.make (x0 * x2 * mmbdd.self () + !x0 * !x2 * mmbdd.self (), true);
  auto qq8 = mmbdd.make (x0 * mmbdd.self () + !x0 * mmbdd.empty (), true);

  auto res = transduct (qq8, ttr, {x2}, {x2});
  std::cout << "only x2*:" << qq8;
  test (res.accepts ({x2, x2}));
  test (not res.accepts ({x2, !x2}));

  // Simple transduction
  auto tr = mmbdd.make  ((x0 * x1) * x2 * !x3 * mmbdd.self () +
                        !(x0 * x1) * !x2 * x3 * mmbdd.self (), true);

  auto q8 = mmbdd.make (x0 * x1 * mmbdd.empty () + !(x0 * x1) * mmbdd.full (), false);

  std::cout << q8;
  std::cout << "AND" << (q8 & tr) << "END" << std::endl;

  auto comp = transduct (q8, tr, {x2, x3}, {x0, x1});

  std::cout << comp;
  test (comp.accepts ({!x0 * x1}));
  test (not comp.accepts ({x0 * !x1}));

  auto comp2 = transduct (q8, ttr, {x1, x2}, {x1, x2});
  std::cout << "COMP2:" << comp2;
  test (comp2.accepts ({!x2 * x1}));
  test (comp2.rejects ({x1 * x2}));

  auto comp3 = transduct (q8, ttr, {x1, x2}, {x0, x1});

  std::cout << "COMP3:" << comp3;
  test (comp2 != comp3);

  {
    sylvan::BddMap m;
    m.put (x0.TopVar (), x1);
    m.put (x1.TopVar (), x2);
    auto mod = comp3.apply ([&] (Bdd b) { return b.Compose (m); });
    std::cout << "MOD: " << mod;
    test (mod == comp2);
  }

  // This one is not output deterministic, but is post-unambiguous
  {
    // The one T transition.
    auto q = mmbdd.make (mmbdd.full (), false);

    // Changing q to mmbdd.self would make it post-ambiguous.
    auto tr = mmbdd.make ((x0 * x2 * x3 * q) +
                          (!x0 * x1 * x2 * x3 + !x0 * !x1 * !x2 * x3) *
                          mmbdd.full (), false);

    auto comp = transduct (q, tr, {x2, x3}, {x2, x3});

    test (comp.accepts ({x3 * x2}));
    test (comp.rejects ({!x2 * !x3}));
  }

  {
    auto q1 = flat_automaton ({x0, !x0, Bdd::bddZero (), x1, x1});
    auto q = q1.apply ([&] (const Bdd& b) {
      return b.Compose (sylvan::BddMap (x1.TopVar (), x2));
    });
    test (q.accepts ({ !x0 * x2, x2 }));
    test (q.accepts (
            {   x0 * !x2, x0 * x2, !x0 * x2, // First state
                x2 * x0, // Second state
                x2 * x0, x2 * !x0, x2 * x0
            }));
    test (not q.accepts (
            {   !x0 * !x2,
                !x0 * !x2
            }));
  }

  sylvan::sylvan_quit();

  return global_res ? 0 : 1;
}
