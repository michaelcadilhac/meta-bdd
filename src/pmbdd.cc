#include <boost/spirit/home/x3.hpp>
#include <iostream>
#include <fstream>

#include "upset.hh"
#include "petri_net.hh"

using value_t = upset::value_type;

struct transition_view {
    std::vector<value_t> budgets;
    std::vector<value_t> backward_deltas;
};

static auto get_transition_views (const petri_net& pnet) {
  std::vector<transition_view> ret;
  ret.reserve (pnet.get_transitions ().size ());

  for (auto&& t : pnet.get_transitions ()) {
    transition_view t_view;

    t_view.budgets.resize (pnet.num_places ());
    t_view.backward_deltas.resize (pnet.num_places ());

    for (auto&& [place, delta] : t) {
      auto& [budget, gain] = delta;
      t_view.budgets[place] = budget;
      t_view.backward_deltas[place] = budget - gain;
    }
    ret.emplace_back (std::move (t_view));
  }
  return ret;
}

static auto map_to_vec (const petri_net::place_to_weight_t& m, size_t num_places) {
  std::vector<value_t> ret (num_places);
  for (auto&& [p, w] : m)
    ret[p] = w;
  return ret;
}

static bool backward_coverability (const std::vector<value_t>& init,
                                   const std::vector<value_t>& target,
                                   const std::vector<transition_view>& transitions) {
  auto u = upset (init), prev_u = u;
  do {
    prev_u = u;
    for (auto&& t : transitions) {
      u += t.backward_deltas;
      u = (u & t.budgets) | u;
    }
  } while (prev_u == u);

  return u.contains (target);
}

int main (int argc, char* argv[]) {
  if (argc != 2)
    return 1;

  // Initialize sylvan
  lace_start(0, 0);
  sylvan::sylvan_set_sizes (1LL<<22, 1LL<<26, 1LL<<22, 1LL<<26);
  sylvan::sylvan_init_package ();
  sylvan::sylvan_init_bdd ();

  // Initialize MBDD
  MBDD::init ();

  try {
    petri_net pnet (argv[1]);

    auto transitions = get_transition_views (pnet);
    auto init = map_to_vec (pnet.get_init (), pnet.num_places ());
    auto target = map_to_vec (pnet.get_target (), pnet.num_places ());

    if (backward_coverability (init, target, transitions))
      std::cout << "COVERABLE" << std::endl;
    else
      std::cout << "NOT COVERABLE" << std::endl;
  }
  catch (const std::ios_base::failure& e) {
    std::cerr << e.what () << std::endl;
    return 2;
  }
  catch (const petri_net::parse_error& e) {
    return 3;
  }
}
