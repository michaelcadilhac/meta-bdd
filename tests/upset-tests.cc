#include <algorithm>
#include <upset.hh>

using namespace sylvan;


int main () {
  // Initialize sylvan
  lace_start(0, 0);
  sylvan_set_sizes (1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
  sylvan_init_package ();
  sylvan_init_bdd ();

  // Initialize MBDD
  MBDD::init ();

  {
    auto u = upset ({2});
    std::cout << u.get_mbdd ();

    assert (u.contains ({2}));
    assert (u.contains ({3}));
    assert (u.contains ({4}));
    assert (u.contains ({5}));
    assert (not u.contains ({0}));
    assert (not u.contains ({1}));
  }

  {
    auto u = upset ({3, 1, 4, 9});
    std::cout << "mbdd: " << u.get_mbdd ();
    assert (u.contains ({3, 1, 4, 9}));
    assert (u.contains ({4, 2, 4, 10}));
    assert (not u.contains ({1, 1, 4, 9}));
    assert (not u.contains ({3, 1, 4, 0}));
  }

  {
    auto full_test = [] (std::vector<size_t> v) {
      assert (v.size () == 5);
      auto uplimit = *std::ranges::max_element (v) + 2,
        downlimit = uplimit / 2;

      auto u = upset (v);
      for (size_t i0 = downlimit; i0 < uplimit; ++i0)
        for (size_t i1 = downlimit; i1 < uplimit; ++i1)
          for (size_t i2 = downlimit; i2 < uplimit; ++i2)
            for (size_t i3 = downlimit; i3 < uplimit; ++i3)
              for (size_t i4 = downlimit; i4 < uplimit; ++i4)
                assert (u.contains ({ i0, i1, i2, i3, i4 }) ==
                        (i0 >= v[0] and i1 >= v[1] and i2 >= v[2]
                         and i3 >= v[3] and i4 >= v[4]));
    };
    full_test ({ 12, 11, 10, 9, 8 });
  }
}
