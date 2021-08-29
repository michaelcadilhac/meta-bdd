#include <algorithm>
#include <upset.hh>
#include "tests.hh"

using namespace sylvan;

int main (int argc, char** argv) {
  // Initialize sylvan
  lace_start(0, 0);
  sylvan_set_sizes (1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
  sylvan_init_package ();
  sylvan_init_bdd ();

  // Initialize MBDD
  MBDD::init ();

  std::function<void ()> tests[] = {
    [] () {
      auto u = upset ({2});
      std::cout << u.get_mbdd ();

      test (u.contains ({2}));
      test (u.contains ({3}));
      test (u.contains ({4}));
      test (u.contains ({5}));
      test (not u.contains ({0}));
      test (not u.contains ({1}));
    },

    [] () {
      auto u = upset ({2, 1});
      u += {5, 1};

      test (not u.contains ({6, 1}));
      test (not u.contains ({7, 1}));
      test (u.contains ({9, 2}));
      test (u.contains ({10, 2}));
      test (not u.contains ({0, 1}));
      test (not u.contains ({1, 1}));
    },

    [] () {
      auto u = upset ({3, 1, 4, 9});
      std::cout << "mbdd: " << u.get_mbdd ();
      test (u.contains ({3, 1, 4, 9}));
      test (u.contains ({4, 2, 4, 10}));
      test (not u.contains ({1, 1, 4, 9}));
      test (not u.contains ({3, 1, 4, 0}));
    },

    [] () {
      auto u = upset ({0, 0});
      u += {1, 0};
      test (u.contains ({7, 3}));
      test (not u.contains ({0, 0}));
      test ((u | upset ({0, 0})).is_full ());
      test ((u & upset ({0, 0})) == u);
    },

    [] () {
      auto u = upset ({2, 3});
      u += {-1, 0};
      test (u.contains ({2, 3}));
      test (u.contains ({1, 3}));
      test (not u.contains ({0, 3}));
      test (not u.contains ({2, 2}));
    },

    [] () {
      auto u = upset ({3, 1, 4, 9});
      u += {3, 2, 3, 0};

      test (u.contains ({6, 3, 7, 9}));
      test (not u.contains ({5, 3, 7, 9}));
      test (not u.contains ({6, 2, 7, 9}));
      test (not u.contains ({6, 3, 6, 9}));
      test (not u.contains ({6, 3, 7, 8}));
    },

    [] () {
      auto u = upset ({2});
      std::cout << "u: " << u.get_mbdd () << std::endl;
      auto v = upset ({0});
      std::cout << "v: " << v.get_mbdd () << std::endl;
      test (v.get_mbdd () == MBDD::full ());
      u = u + std::vector ({-2l});
      std::cout << "u trans'd: " << u.get_mbdd () << std::endl;
      test (u == v);
    },

    [] () {
      auto u = upset ({3, 1, 1, 3}) | upset ({1, 3, 2, 2});
      test (u.contains ({3, 1, 1, 3}));
      test (u.contains ({1, 3, 2, 2}));
      test (not u.contains ({1, 2, 2, 2}));
      test (not u.contains ({2, 0, 0, 3}));
      u += {-3, -3, -2, -3};
      test (u.get_mbdd () == MBDD::full ());
    },

    [] () {
      auto u = upset ({3, 1, 4, 9});
      std::cout << "Computing transducer." << std::endl;
      u += {3, -2, 3, -1};
      // u = {6, 0, 7, 8}

      test (u.contains ({6, 3, 7, 9}));
      test (not u.contains ({5, 3, 7, 9}));
      test (u.contains ({6, 2, 7, 9}));
      test (u.contains ({6, 0, 7, 9}));
      test (not u.contains ({6, 3, 6, 9}));
      test (u.contains ({6, 3, 7, 8}));
      test (not u.contains ({6, 3, 7, 7}));

      u &= {2, 2, 1, 8};
      test (u.contains ({6, 3, 7, 9}));
      test (u.contains ({6, 2, 7, 8}));
      test (not u.contains ({5, 2, 7, 8}));
      test (not u.contains ({6, 2, 5, 8}));
    },

    [] () {
      auto full_test = [] (std::vector<size_t> v) {
        test (v.size () == 5);
        auto uplimit = *std::ranges::max_element (v) + 2,
          downlimit = uplimit / 2;

        auto u = upset (v);
        for (size_t i0 = downlimit; i0 < uplimit; ++i0)
          for (size_t i1 = downlimit; i1 < uplimit; ++i1)
            for (size_t i2 = downlimit; i2 < uplimit; ++i2)
              for (size_t i3 = downlimit; i3 < uplimit; ++i3)
                for (size_t i4 = downlimit; i4 < uplimit; ++i4)
                  test (u.contains ({ i0, i1, i2, i3, i4 }) ==
                        (i0 >= v[0] and i1 >= v[1] and i2 >= v[2]
                         and i3 >= v[3] and i4 >= v[4]));
      };
      full_test ({ 12, 11, 10, 9, 8 });
    }
  };

  if (argc > 1 && std::string (argv[1]) == "-x") {
    argv++; argc--;
    exit_on_fail = true;
  }

  auto args = std::set<std::string> (argv + 1, argv + argc);
  size_t i = 0;
  for (auto& f : tests) {
    ++i;
    if (argc > 1 and not args.contains (std::to_string (i)))
      continue;
    std::cout << "TEST " << i << std::endl;
    f ();
  }

  return global_res ? 0 : 1;
}
