#pragma once

#include <sylvan.h>
#include <sylvan_obj.hpp>

#include <utils/cache.hh>

#include <labels/sylvanbdd.hh>
#include <labels/abcbdd.hh>

namespace utils {

  /* Transduction is seen as an intersection with projection. */
  static auto transduct (auto&& state, auto&& trans,
                         std::vector<labels::sylvanbdd> output_vars,
                         std::vector<labels::sylvanbdd> to_vars) {
    using sylvan::sylvan_project_RUN;
    using Bdd = sylvan::Bdd;
    using BddMap = sylvan::BddMap;

    assert (output_vars.size () == to_vars.size () and output_vars.size () != 0);
    // Construct the map output_var -> to_var.
    BddMap m;
    Bdd bddoutputvars = Bdd::bddOne ();
    for (size_t i = 0; i < output_vars.size (); ++i) {
      m.put (output_vars[i].TopVar (), to_vars[i]);
      bddoutputvars *= output_vars[i];
    }

    return state.transduct (trans,
                            [&bddoutputvars, &m] (const labels::sylvanbdd& b) -> labels::sylvanbdd {
                              return Bdd (sylvan_project (b.GetBDD (), bddoutputvars.GetBDD ())).Compose (m);
                            },
                            *((size_t*) &m) // This is because BddMap doesn't expose the Bdd.
      );
  }

  inline abcbdd transduct_map (const abcbdd& bdd,
                               const std::map<int, int>& map) {
    if (bdd.isZero () or bdd.isOne ()) return bdd;
    auto var = bdd.TopVar ();
    auto el = map.find (var);

    if (el == map.end ())
      return transduct_map (bdd.Then (), map) | transduct_map (bdd.Else (), map);
    return abcbdd::bddVar (el->second) * transduct_map (bdd.Then (), map) |
      !abcbdd::bddVar (el->second) * transduct_map (bdd.Else (), map);
  }

  /* Transduction is seen as an intersection with projection. */
  static auto transduct (auto&& state, auto&& trans,
                         std::vector<labels::abcbdd> output_vars,
                         std::vector<labels::abcbdd> to_vars) {
    static auto cache = make_cache<std::map<int, int>> (output_vars, to_vars);
    auto cached = cache.get (output_vars, to_vars);
    std::remove_reference_t<decltype (*cached)>* p_map = nullptr;
    if (cached)
      p_map = &(*cached);
    else {
      std::remove_cvref_t<decltype (*p_map)> map;
      for (size_t i = 0; i < output_vars.size (); ++i)
        map[output_vars[i].TopVar ()] = to_vars[i].TopVar ();
      p_map = &(cache (std::move (map), output_vars, to_vars));
    }

    return state.transduct (trans,
                            [p_map] (auto&& l) { return transduct_map (l, *p_map); },
                            p_map);
  }
}
