#pragma once
#include <map>
#include <string>
#include <vector>

namespace petri_pl {
  struct savers;
}

class petri_net {
  public:

    using place_id_t = unsigned;
    using weight_t = int;
    using place_to_weight_t = std::map<place_id_t, weight_t>;

    struct parse_error : std::runtime_error {
        parse_error () : std::runtime_error ("petri Prolog parse error") {}
    };

    petri_net (const std::string& fname);


    using transition_t = std::map<place_id_t, std::pair<weight_t, weight_t>>;
    using transitions_t = std::vector<transition_t>;

    const transitions_t& get_transitions () const { return transitions; }
    unsigned num_places () const { return nplaces; }

    friend petri_pl::savers;

    const place_to_weight_t& get_init () const   { return init; }
    const place_to_weight_t& get_target () const { return target; }

  private:
    transitions_t transitions;
    std::map<std::string, place_id_t> place_name_to_place_id;
    place_to_weight_t init, target;
    unsigned nplaces = 0;
};
