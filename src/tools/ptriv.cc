#include <boost/spirit/home/x3.hpp>
#include <iostream>
#include <fstream>
#include <queue>
#include <list>
#include <set>

#include "petri_net.hh"

using value_t = ssize_t;

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

class upset : public std::set<std::vector<value_t>> {
    using vec_t = std::vector<value_t>;
  public:
    bool all_components_geq (const vec_t& v1, const vec_t& v2) {
      for (size_t i = 0; i < v1.size (); ++i)
        if (v1[i] < v2[i])
          return false;
      return true;
    }

    upset operator+ (const vec_t& v) const {
      upset ret;
      for (auto x : (*this)) {
        for (size_t i = 0; i < v.size (); ++i)
          x[i] += v[i];
        ret.insert (std::move (x));
      }
      return ret;
    }

    upset& operator&= (const vec_t& v) {
      upset ret;
      for (auto x : (*this)) {
        for (size_t i = 0; i < v.size (); ++i)
          x[i] = std::max (v[i], x[i]);
        ret.insert (std::move (x));
      }
      return *this = ret;
    }

    upset& operator|= (const upset& other) {
      this->insert (other.begin (), other.end());
      return *this;
    }

    void minimize () {
      for (auto& x : (*this)) {
        for (auto&& it_y = this->begin (); it_y != this->end (); /* in-loop */) {
          if (&x != &(*it_y) and all_components_geq (*it_y, x))
            it_y = this->erase (it_y);
          else
            ++it_y;
        }
      }
    }
};

static bool backward_coverability (const std::vector<value_t>& init,
                                   std::list<std::vector<value_t>>& targets,
                                   const std::vector<transition_view>& transitions) {
  auto B = upset (), Bprime = B;

  for (auto&& el : targets)
    Bprime.insert (el);
  Bprime.minimize ();

  size_t i = 0;

  do {
    std::cout << "Loop #" << i++ << std::endl;
    B = Bprime;
    for (auto&& t : transitions) {
      std::cout << "Applying transition deltas " << t.backward_deltas << std::endl;
      auto mt = (B + t.backward_deltas);
      std::cout << "Applying transition budgets " << t.budgets << std::endl;
      mt &= t.budgets;
      std::cout << "Adding to Bprime" << std::endl;
      if (mt.contains (init))
        return true;
      Bprime |= mt;
    }
    Bprime.minimize ();
  } while (B != Bprime);

  return false;
}

int main (int argc, char* argv[]) {
  if (argc != 2)
    return 1;

  try {
    petri_net pnet (argv[1]);

    auto transitions = get_transition_views (pnet);
    auto init = map_to_vec (pnet.get_init (), pnet.num_places ());
    auto targets = std::list<decltype (init)> ();
    for (auto&& el : pnet.get_targets ())
      targets.push_back (map_to_vec (el, pnet.num_places ()));

    if (backward_coverability (init, targets, transitions))
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
