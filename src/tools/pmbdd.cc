#include <boost/spirit/home/x3.hpp>
#include <iostream>
#include <fstream>
#include <queue>

#include <upset.hh>
#include "petri_net.hh"

auto mmbdd = MBDD::make_master_meta_bdd<sylvan::Bdd, MBDD::states_are_bddvars> ();
using upset_bdd = upset::upset<decltype (mmbdd)>;

using value_t = upset_bdd::value_type;

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


// Debug
template <typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
    if (!v.empty()) {
        out << '[';
        std::ranges::copy(v, std::ostream_iterator<T>(out, ", "));
        out << "\b\b]";
    }
    return out;
}

static bool backward_coverability (const std::vector<value_t>& init,
                                   const std::vector<value_t>& target,
                                   const std::vector<transition_view>& transitions) {
  auto B = upset_bdd (mmbdd, target), Bprime = B;
  size_t i = 0;

  do {
    std::cout << "Loop #" << i++ << std::endl;
    //std::cout << "Bprime is: " << Bprime << std::endl;


    B = Bprime;
    for (auto&& t : transitions) {
      std::cout << "Applying transition deltas " << t.backward_deltas << std::endl;
      auto mt = (B + t.backward_deltas);
      //std::cout << "mt is: " << mt << std::endl;
      std::cout << "Applying transition budgets " << t.budgets << std::endl;
      mt &= t.budgets;
      //std::cout << "mt is: " << mt << std::endl;
      std::cout << "Adding to Bprime" << std::endl;
      if (mt.contains (init))
        return true;
      Bprime |= mt;
    }
  } while (B != Bprime);

  return false;
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
  mmbdd.init ();

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
