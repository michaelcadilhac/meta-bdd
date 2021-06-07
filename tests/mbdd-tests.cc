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

  auto q = MBDD::make ((x0 * !x1) * MBDD::full (), false);

  std::cout << q;

  assert (q.accepts ({ x0 * !x1 }));
  assert (q.accepts ({ x0 * !x1, x0 * x1, !x1 }));
  assert (not q.accepts ({ !x0 * x1 }));
  assert (not q.accepts ({ x0 * x1, x0 * x1, !x1 }));

  assert (MBDD::full ().accepts ({ x0, x0, x0, x0 }));
  assert (not MBDD::empty ().accepts ({ x0, x0, x0, x0 }));

  auto q1 = flat_automaton ({x0, !x0, Bdd::bddZero (), x1, x1});
  std::cout << q1;

  assert (q1.accepts ({ !x0 * x1, x1 }));
  assert (q1.accepts (
            {   x0 * !x1, x0 * x1, !x0 * x1, // First state
                x1 * x0, // Second state
                x1 * x0, x1 * !x0, x1 * x0
            }));
  assert (not q1.accepts (
            {   !x0 * !x1,
                !x0 * !x1
            }));

  auto q2 = flat_automaton ({x0, !x0, Bdd::bddZero (), x1, x1});
  assert (q1 == q2);
  assert (q1.one_step (!x0) == q2.one_step (!x0));

  auto q3 = flat_automaton ({x0, !x0, (x0 * x1) | (!x0 * !x1) | (x0 * !x1) | (!x0 * x1)});
  assert (q3.one_step (!x0) == MBDD::full ());

  std::cout << "q2\n" << q2;
  std::cout << "q1 & q2\n" << (q1 & q2);

  assert ((q1 & q2) == q2);
  assert ((q1 & q1) == q1);
  assert ((q1 & q2) == (q2 & q1));

  auto q4 = flat_automaton ({Bdd::bddZero (), !x0, !x1, x1, !x1, x1, Bdd::bddZero ()});

  assert (q4.accepts ( { !x0, x1, x1 } ));

  assert (not q4.accepts ( { x0, !x0, !x1, x1, x1 } ));

  auto q5 = q1 & q4;

  assert (q5.accepts ({ !x0, x1, x1 }));
  assert (not q5.accepts ({ !x0 * x1, x1 })); // in q1 not q4
  assert (not q5.accepts ({ !x0 * x1, !x1 * x0, x1, x1 })); // in q4 not q1

  assert (q5 == (q5 & MBDD::full ()));
  assert (MBDD::empty () == (q5 & MBDD::empty ()));

  assert ((q4 & q) == MBDD::empty ());

  auto q5p = MBDD::make (x0 * q5 + !x0 * MBDD::self (), false);
  auto q5pp = MBDD::make (x0 * q5 + x1 * q2 + !x0 * MBDD::self (), false);
  assert ((q5p & q5pp) != q5);

  auto q6 = MBDD::make (!(x0 * !x1) * MBDD::full (), false);
  auto q7 = MBDD::make (MBDD::full (), false);

  assert ((q | q6) == q7);
  sylvan_quit();
}
