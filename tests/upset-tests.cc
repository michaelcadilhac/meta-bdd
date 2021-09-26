#include <algorithm>
#include <upset.hh>
using namespace std::literals;

#include "tests.hh"

auto mmbdd = MBDD::make_master_meta_bdd<sylvan::Bdd, MBDD::states_are_bddvars> ();

using upset_bdd = upset::upset<decltype (mmbdd)>;

int main (int argc, char** argv) {
  // Initialize sylvan
  lace_start(0, 0);
  sylvan::sylvan_set_sizes (1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
  sylvan::sylvan_init_package ();
  sylvan::sylvan_init_bdd ();

  // Initialize MBDD
  mmbdd.init ();

  std::function<void ()> tests[] = {
    [] () {
      auto u = upset_bdd (mmbdd, {2});
      std::cout << u.get_mbdd ();

      test (u.contains ({2}));
      test (u.contains ({3}));
      test (u.contains ({4}));
      test (u.contains ({5}));
      test (not u.contains ({0}));
      test (not u.contains ({1}));
    },

    [] () {
      auto x = upset_bdd (mmbdd, {1,1,1,1,8,1,1});
      std::cout << "Upset of 1^7: " << x.get_mbdd () << std::endl;
      auto u = upset_bdd (mmbdd, {2, 1});
      std::cout << "Upset of {2, 1}: " << u.get_mbdd () << std::endl;
      u += {5, 1};

      test (not u.contains ({6, 1}));
      test (not u.contains ({7, 1}));
      test (u.contains ({9, 2}));
      test (u.contains ({10, 2}));
      test (not u.contains ({0, 1}));
      test (not u.contains ({1, 1}));
    },

    [] () {
      auto u = upset_bdd (mmbdd, {3, 1, 4, 9});
      std::cout << "mbdd: " << u.get_mbdd ();
      test (u.contains ({3, 1, 4, 9}));
      test (u.contains ({4, 2, 4, 10}));
      test (not u.contains ({1, 1, 4, 9}));
      test (not u.contains ({3, 1, 4, 0}));
    },

    [] () {
      auto u = upset_bdd (mmbdd, {0, 0});
      u += {1, 0};
      test (u.contains ({7, 3}));
      test (not u.contains ({0, 0}));
      test ((u | upset_bdd (mmbdd, {0, 0})).is_full ());
      test ((u & upset_bdd (mmbdd, {0, 0})) == u);
    },

    [] () {
      auto u = upset_bdd (mmbdd, {2, 3});
      u += {-1, 0};
      test (u.contains ({2, 3}));
      test (u.contains ({1, 3}));
      test (not u.contains ({0, 3}));
      test (not u.contains ({2, 2}));
    },

    [] () {
      auto u = upset_bdd (mmbdd, {3, 1, 4, 9});
      u += {3, 2, 3, 0};

      test (u.contains ({6, 3, 7, 9}));
      test (not u.contains ({5, 3, 7, 9}));
      test (not u.contains ({6, 2, 7, 9}));
      test (not u.contains ({6, 3, 6, 9}));
      test (not u.contains ({6, 3, 7, 8}));
    },

    [] () {
      auto u = upset_bdd (mmbdd, {2});
      std::cout << "u: " << u.get_mbdd () << std::endl;
      auto v = upset_bdd (mmbdd, {0});
      std::cout << "v: " << v.get_mbdd () << std::endl;
      test (v.get_mbdd () == mmbdd.full ());
      u = u + std::vector ({-2l});
      std::cout << "u trans'd: " << u.get_mbdd () << std::endl;
      test (u == v);
    },

    [] () {
      auto u = upset_bdd (mmbdd, {3, 1, 1, 3}) | upset_bdd (mmbdd, {1, 3, 2, 2});
      test (u.contains ({3, 1, 1, 3}));
      test (u.contains ({1, 3, 2, 2}));
      test (not u.contains ({1, 2, 2, 2}));
      test (not u.contains ({2, 0, 0, 3}));
      u += {-3, -3, -2, -3};
      test (u.get_mbdd () == mmbdd.full ());
    },

    [] () {
#ifdef NDEBUG
      auto u = upset_bdd (mmbdd, {1, 2, 3, 4, 5, 6, 7});
      std::cout << u.get_mbdd () << std::endl;
      u = u + std::vector<ssize_t> {-1l, -2l, -3l, -4l, -5l, -6l, -7l};
      test (u.is_full ());
      /*std::cout << "Once again!" << std::endl;
      auto v = upset_bdd (mmbdd, {1, 2, 3, 4, 5, 6, 7});
      v += {-1, -2, -3, -4, -5, -6, -7};
      test (v.is_full ());
       std::cout << "Done." << std::endl;*/
#else
      std::cout << "Test disabled because NDEBUG is not defined and it would take too long."
                << std::endl;
#endif
    },

    [] () {
      auto u = upset_bdd (mmbdd, {3, 1, 4, 9});
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
      auto full_test = [] (std::vector<ssize_t> v) {
        test (v.size () == 5);
        auto uplimit = *std::ranges::max_element (v) + 2,
          downlimit = uplimit / 2;

        auto u = upset_bdd (mmbdd, v);
        for (ssize_t i0 = downlimit; i0 < uplimit; ++i0)
          for (ssize_t i1 = downlimit; i1 < uplimit; ++i1)
            for (ssize_t i2 = downlimit; i2 < uplimit; ++i2)
              for (ssize_t i3 = downlimit; i3 < uplimit; ++i3)
                for (ssize_t i4 = downlimit; i4 < uplimit; ++i4)
                  test (u.contains ({ i0, i1, i2, i3, i4 }) ==
                        (i0 >= v[0] and i1 >= v[1] and i2 >= v[2]
                         and i3 >= v[3] and i4 >= v[4]));
      };
      full_test ({ 12, 11, 10, 9, 8 });
    }
  };

  if (argc > 1 and argv[1] == "-x"sv) {
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
