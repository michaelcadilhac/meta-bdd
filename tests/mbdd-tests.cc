#include <meta_bdd.hh>

using namespace sylvan;

auto flat_automaton (std::span<const Bdd> w) {
  assert (w.size () > 0);
  if (w.size () == 1) // End of the list.
    return MBDD::make (w[0] * MBDD::self (), true);
  return MBDD::make (w[0] * MBDD::self () | w[1] * flat_automaton (w.subspan (2)), false);
}


auto flat_automaton (std::initializer_list<Bdd> w) {
  return flat_automaton (std::span (w));
}

static bool global_res = true;

#define test(T) do {                                                    \
    bool __res = (T);                                                   \
    std::cout << (__res ? "[PASS] " : "[FAIL] ")                        \
              << __FILE__ << ":" << __LINE__ << ": " << #T << std::endl; \
    global_res &= __res;                                               \
  } while (0)


int main () {
  // Initialize sylvan
  lace_start(0, 0);
  sylvan_set_sizes (1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
  sylvan_init_package ();
  sylvan_init_bdd ();

  // Initialize MBDD
  MBDD::init ();

  auto x0 = Bdd::bddVar (0),
    x1 = Bdd::bddVar (1);

  auto qfull = MBDD::make (x0 * MBDD::self () + !x0 * MBDD::full (), true);
  test (qfull == MBDD::full ());

  auto qempty = MBDD::make ((x0 * x1) * MBDD::empty () + !(x0 * x1) * MBDD::self (), false);
  test (qempty == MBDD::empty ());

  test (MBDD::make (MBDD::full (), true) == MBDD::full ());
  test (MBDD::make (MBDD::self (), true) == MBDD::full ());
  test (MBDD::make (MBDD::empty (), false) == MBDD::empty ());
  test (MBDD::make (MBDD::self (), false) == MBDD::empty ());

  auto q = MBDD::make ((x0 * !x1) * MBDD::full (), false);

  std::cout << q;
  std::cout << "q accepts " << q.one_word ()
            << " and rejects " << q.one_word (false) << std::endl;

  test (q.accepts ({ x0 * !x1 }));
  test (q.accepts ({ x0 * !x1, x0 * x1, !x1 }));
  test (not q.accepts ({ !x0 * x1 }));
  test (not q.accepts ({ x0 * x1, x0 * x1, !x1 }));

  test (MBDD::full ().accepts ({ x0, x0, x0, x0 }));
  test (not MBDD::empty ().accepts ({ x0, x0, x0, x0 }));

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
  test (q3.one_step (!x0) == MBDD::full ());

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

  test (q5 == (q5 & MBDD::full ()));
  test (MBDD::empty () == (q5 & MBDD::empty ()));

  test ((q4 & q) == MBDD::empty ());

  auto q5p = MBDD::make (x0 * MBDD::full () + !x0 * MBDD::self (), false);
  auto q5pp = MBDD::make (x0 * MBDD::full () + (x1 * !x0) * q2 + (!x1 * !x0) * MBDD::self (), false);
  auto q5pi = q5p & q5pp;
  std::cout << "q5pi: " << q5pi;
  test (q5pi == (q5pp & q5p));

  auto q6 = MBDD::make (!(x0 * !x1) * MBDD::full (), false);
  auto q7 = MBDD::make (MBDD::full (), false);

  test ((q | q6) == q7);
  test ((q | q6) == (q6 | q));

  // Print a word per state
  for (auto&& state : MBDD::global_mmbdd) {
    std::cout << "State";
    if (state != MBDD::empty ())
      std::cout << " accepts [" << state.one_word () << "]";
    if (state != MBDD::full ())
      std::cout << " rejects [" <<  state.one_word (false) << "]";
    std::cout << std::endl;
  }

  // Transductions
  std::cout << "TRANSDUCTIONS." << std::endl;

  auto x2 = Bdd::bddVar (2), x3 = Bdd::bddVar (3);
  auto ttr = MBDD::make (x0 * x2 * MBDD::self () + !x0 * !x2 * MBDD::self (), true);
  auto qq8 = MBDD::make (x0 * MBDD::self () + !x0 * MBDD::empty (), true);

  auto res = qq8.transduct (ttr, {x2}, {x2});
  std::cout << "only x2*:" << qq8;
  test (res.accepts ({x2, x2}));
  test (not res.accepts ({x2, !x2}));

  // Simple transduction
  auto tr = MBDD::make  ((x0 * x1) * x2 * !x3 * MBDD::self () +
                        !(x0 * x1) * !x2 * x3 * MBDD::self (), true);

  auto q8 = MBDD::make (x0 * x1 * MBDD::empty () + !(x0 * x1) * MBDD::full (), false);

  std::cout << q8;
  std::cout << "AND" << (q8 & tr) << "END" << std::endl;

  auto comp = q8.transduct (tr, {x2, x3}, {x0, x1});

  std::cout << comp;
  test (comp.accepts ({!x0 * x1}));
  test (not comp.accepts ({x0 * !x1}));

  auto comp2 = q8.transduct (ttr, {x1, x2}, {x1, x2});
  std::cout << "COMP2:" << comp2;
  test (comp2.accepts ({!x2 * x1}));
  test (comp2.rejects ({x1 * x2}));

  auto comp3 = q8.transduct (ttr, {x1, x2}, {x0, x1});

  std::cout << "COMP3:" << comp3;

  {
    BddMap m;
    m.put (x0.TopVar (), x1);
    m.put (x1.TopVar (), x2);
    auto mod = comp3.apply ([&] (Bdd b) { return b.Compose (m); });
    test (mod == comp2);
  }

  // This one is not output deterministic, but is post-unambiguous
  {
    // The one T transition.
    auto q = MBDD::make (MBDD::full (), false);

    // Changing q to MBDD::self would make it post-ambiguous.
    auto tr = MBDD::make ((x0 * x2 * x3 * q) +
                          (!x0 * x1 * x2 * x3 + !x0 * !x1 * !x2 * x3) *
                          MBDD::full (), false);

    auto comp = q.transduct (tr, {x2, x3}, {x2, x3});

    test (comp.accepts ({x3 * x2}));
    test (comp.rejects ({!x2 * !x3}));
  }

  sylvan_quit();

  return global_res ? 0 : 1;
}
