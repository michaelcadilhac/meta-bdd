#include <algorithm>

#include <upset/upset_adhoc.hh>

using namespace std::literals;

#include <labels/sylvanbdd.hh>
#include <labels/abcbdd.hh>
#include <labels/buddybdd.hh>
#include <utils/bdd_io.hh>

#include "tests.hh"

masterbdd global_mbuddy;


auto mmbdd = MBDD::make_master_meta_bdd<labels::buddybdd, MBDD::states_are_ints> ();

using mmbdd_t = decltype (mmbdd);

using upset_bdd = upset::upset_adhoc<mmbdd_t>;

int main (int argc, char** argv) {
  constexpr static auto is_sylvan = std::is_same_v<mmbdd_t::letter_set_type, labels::sylvanbdd>;
  constexpr static auto is_abc = std::is_same_v<mmbdd_t::letter_set_type, labels::abcbdd>;
  constexpr static auto is_buddy = std::is_same_v<mmbdd_t::letter_set_type, labels::buddybdd>;

  if constexpr (is_sylvan) {
    // Initialize sylvan
    lace_start(0, 0);
    sylvan::sylvan_set_sizes (1LL<<23, 1LL<<26, 1LL<<23, 1LL<<26);
    sylvan::sylvan_init_package ();
    sylvan::sylvan_init_bdd ();
  }
  else if constexpr (is_abc) {
    utils::abcbdd::init (100, 1 << 20);
  }
  else if constexpr (is_buddy) {
    utils::buddybdd::init (100, 1 << 20);
  }

  // Initialize MBDD
  mmbdd.init ();

  std::pair<std::string, std::function<void ()>> tests[] = {
    { "A",
      [] () {
        auto u = upset_bdd (mmbdd, {2});
        std::cout << u.get_mbdd ();
        test (u.contains ({2}));
        test (u.contains ({3}));
        test (u.contains ({4}));
        test (u.contains ({5}));
        test (not u.contains ({0}));
        test (not u.contains ({1}));

        u += {-1};
        std::cout << "u: " << u.get_mbdd () << std::endl;
        test (not u.get_mbdd ().accepts ({}));
        test (u.contains ({1}));
        test (not u.contains ({0}));
      }},

    { "B",
      [] () {
        auto u = upset_bdd (mmbdd, {0, 0});
        std::cout << "Upset of {0, 0}: " << u.get_mbdd () << std::endl;
        u += {2, 0};
        std::cout << "Upset of {2, 0}: " << u.get_mbdd () << std::endl;
        test (not u.contains ({1, 0}));
      }},


    { "C",
      [] () {
        auto u = upset_bdd (mmbdd, {2, 1});
        std::cout << "Upset of {2, 1}: " << u.get_mbdd () << std::endl;
        u += {5, 1};

        test (not u.contains ({6, 1}));
        test (not u.contains ({7, 1}));
        test (u.contains ({9, 2}));
        test (u.contains ({10, 2}));
        test (not u.contains ({0, 1}));
        test (not u.contains ({1, 1}));
      }},


    { "D",
      [] () {
        auto u = upset_bdd (mmbdd, {3, 1, 4, 9});
        std::cout << "mbdd: " << u.get_mbdd ();
        test (u.contains ({3, 1, 4, 9}));
        test (u.contains ({4, 2, 4, 10}));
        test (not u.contains ({1, 1, 4, 9}));
        test (not u.contains ({3, 1, 4, 0}));
      }},


    { "E",
      [] () {
        auto u = upset_bdd (mmbdd, {0, 0});
        std::cout << "mbdd: " << u.get_mbdd ();
        u += {1, 0};
        std::cout << "mbdd: " << u.get_mbdd ();
        test (u.contains ({7, 3}));
        test (not u.contains ({0, 0}));
        test ((u | upset_bdd (mmbdd, {0, 0})).is_full ());
        test ((u & upset_bdd (mmbdd, {0, 0})) == u);
      }},


    { "F",
      [] () {
        auto u = upset_bdd (mmbdd, {0});
        std::cout << "mbdd: " << u.get_mbdd ();
        u += {1};
        std::cout << "mbdd: " << u.get_mbdd ();
        test (u.contains ({7}));
        test (not u.contains ({0}));
        test ((u | upset_bdd (mmbdd, {0})).is_full ());
        test ((u & upset_bdd (mmbdd, {0})) == u);
      }},


    { "G",
      [] () {
        auto u = upset_bdd (mmbdd, {2, 3});
        std::cout << u.get_mbdd () << std::endl;
        u += {-1, 0};
        std::cout << u.get_mbdd () << std::endl;
        test (u.contains ({2, 3}));
        test (u.contains ({1, 3}));
        test (not u.contains ({0, 3}));
        test (not u.contains ({2, 2}));
      }},


    { "H",
      [] () {
        auto u = upset_bdd (mmbdd, {3, 1, 4, 9});
        u += {3, 2, 3, 0};

        test (u.contains ({6, 3, 7, 9}));
        test (not u.contains ({5, 3, 7, 9}));
        test (not u.contains ({6, 2, 7, 9}));
        test (not u.contains ({6, 3, 6, 9}));
        test (not u.contains ({6, 3, 7, 8}));
      }},

    { "I1",
      [] () {
        auto u0 = upset_bdd (mmbdd, {0});
        auto v0 = u0 + std::vector<long> ({-1});
        test (u0 == v0);

        auto u = upset_bdd (mmbdd, {1, 0});
        auto v = u + std::vector<long> ({0, -1});
        test (u == v);
      }
    },

    { "I",
      [] () {
        auto v1 = upset_bdd (mmbdd, {1, 1});
        std::cout << "v1 is " << v1.get_mbdd ();

        auto w = v1 + std::vector<long> ({0, -1});
        std::cout << "w is " << w.get_mbdd ();

        auto v = upset_bdd (mmbdd, {1, 0});
        std::cout << "v is " << v.get_mbdd ();

        test (v == w);
      }},


    { "J",
      [] () {
        auto u = upset_bdd (mmbdd, {0, 1});
        std::cout << "u is " << u.get_mbdd ();

        auto w1 = u + std::vector<long> ({1, 0});
        auto v1 = upset_bdd (mmbdd, {1, 1});
        test (v1 == w1);

        auto w2 = u + std::vector<long> ({0, -1});
        auto v2 = upset_bdd (mmbdd, {0, 0});
        test (v2 == w2);

        auto w = w1 + std::vector<long> ({0, -1});
        auto v = upset_bdd (mmbdd, {1, 0});
        std::cout << "w is " << w.get_mbdd ();
        test (v == w);

        test (not w.get_mbdd ().accepts ({}));
        test (w.contains ({1, 1}));
        test (not w.contains ({0, 0}));
        test (not w.contains ({0, 1}));
        test (w.contains ({2, 2}));
      }},


    { "K",
      [] () {
        auto u = upset_bdd (mmbdd, {0, 1, 1});
        u |= upset_bdd (mmbdd, {0, 0, 2});
        u |= upset_bdd (mmbdd, {0, 2, 0});
        std::cout << "u is " << u.get_mbdd ();
        auto v = u + std::vector<long> ({0, -2, -2});
        test (v.get_mbdd () == mmbdd.full ());
        auto w = u + std::vector<long> ({1, -1, 0});
        std::cout << "w is " << w.get_mbdd ();
        test (w.contains ({1, 1, 1}));
        test (w.contains ({2, 1, 0}));
        test (not w.contains ({2, 0, 0}));
        test (w.contains ({1, 1, 0}));
        test (not w.get_mbdd ().accepts ({}));
        test (not w.contains ({1, 0, 0}));
        test (not w.contains ({0, 0, 0}));
      }},


    { "L",
      [] () {
        auto u = upset_bdd (mmbdd, {2});
        std::cout << "u: " << u.get_mbdd () << std::endl;
        auto v = upset_bdd (mmbdd, {0});
        std::cout << "v: " << v.get_mbdd () << std::endl;
        test (v.get_mbdd () == mmbdd.full ());
        u = u + std::vector ({-2l});
        std::cout << "u trans'd: " << u.get_mbdd () << std::endl;
        test (u == v);
      }},


    { "M",
      [] () {
        auto u = upset_bdd (mmbdd, {3, 1, 1, 3}) | upset_bdd (mmbdd, {1, 3, 2, 2});
        test (u.contains ({3, 1, 1, 3}));
        test (u.contains ({1, 3, 2, 2}));
        test (not u.contains ({1, 2, 2, 2}));
        test (not u.contains ({2, 0, 0, 3}));
        u += {-3, -3, -2, -3};
        test (u.get_mbdd () == mmbdd.full ());
      }},


    { "N",
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
      }},


    { "O",
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
      }},


    { "P",
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
      }},

  };

  if (argc > 1 and argv[1] == "-x"sv) {
    argv++; argc--;
    exit_on_fail = true;
  }

  auto args = std::set<std::string> (argv + 1, argv + argc);
  size_t i = 0;
  for (auto& f : tests) {
    ++i;
    if (argc > 1 and not (args.contains (std::to_string (i))
                          or args.contains (f.first)))
      continue;
    std::cout << "TEST " << f.first << std::endl;
    f.second ();
  }

  return global_res ? 0 : 1;
}
