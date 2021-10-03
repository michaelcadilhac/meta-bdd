#include <optional>
#include <iostream>
#include <fstream>
#include <string>
#include <variant>
#include <iomanip>

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include <boost/iostreams/code_converter.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include "petri_net.hh"

/* This relies on Boost Spirit X3 to parse a Prolog file that describes the
 * Petri net. */
namespace petri_pl {
  namespace ast {

    namespace x3 = boost::spirit::x3;
    using namespace boost::fusion;

    using weight_t = int;

    // 'x14'
    using place = std::string;

    // 'x14' or ('x14', 2)
    struct place_and_weight {
        place                   name;
        std::optional<weight_t> weight;
    };

    // ['x13', ('x14', 2)]
    using place_and_weight_vec = std::vector<place_and_weight>;

    // ['x13', ('x14', 2)] or (['x13', ('x14', 2)], 4)
    struct place_and_weight_vec_and_weight {
        place_and_weight_vec    places;
        std::optional<weight_t> weight;
    };

    // [(['x4]', 2), ... ]
    using place_and_weight_vec_and_weight_vec =
      std::vector<place_and_weight_vec_and_weight>;

    struct transition {
        place name;
        place_and_weight_vec_and_weight from, to;
    };

    struct init {
        place name;
        weight_t weight;
    };

    struct target {
        int id;
        place_and_weight_vec_and_weight_vec targets;
    };

    struct statement : x3::variant<place, transition, init, target> {
        using base_type::base_type;
        using base_type::operator=;
    };

  }
}

BOOST_FUSION_ADAPT_STRUCT (petri_pl::ast::place_and_weight, name, weight)
BOOST_FUSION_ADAPT_STRUCT (petri_pl::ast::place_and_weight_vec_and_weight, places, weight)
BOOST_FUSION_ADAPT_STRUCT (petri_pl::ast::transition, name, from, to)
BOOST_FUSION_ADAPT_STRUCT (petri_pl::ast::init, name, weight)
BOOST_FUSION_ADAPT_STRUCT (petri_pl::ast::target, id, targets)

namespace petri_pl {
  namespace { namespace x3 = boost::spirit::x3; }

  struct savers {
      using place_id_and_weight = std::pair<petri_net::place_id_t, petri_net::weight_t>;
      using place_id_and_weight_vec = std::vector<place_id_and_weight>;

      template <typename Iterator, typename Context>
      static auto flatten_place_and_weights (
        const ast::place_and_weight_vec_and_weight& pawvaw,
        const Iterator& it,
        const Context& ctx) {
        place_id_and_weight_vec ret;

        auto& pnet = x3::get<savers> (ctx).get ();
        auto top_weight = pawvaw.weight.value_or (1);
        for (auto&& p : pawvaw.places) {
          try {
            ret.emplace_back (std::pair (pnet.place_name_to_place_id.at (p.name),
                                         p.weight.value_or (top_weight)));
          }
          catch (std::out_of_range& e) {
            throw x3::expectation_failure<Iterator> (it, "known place");
          }
        }
        return ret;
      }

      struct save_transition {
          template <typename Iterator, typename Context>
          void on_success(Iterator const& first, Iterator const& last,
                          ast::transition& trans, Context const& context) {
            auto& pnet = x3::get<savers> (context).get ();

            petri_net::transition_t t;
            for (auto&& [id, weight] : flatten_place_and_weights (trans.from, last, context))
              t[id].first = weight;
            for (auto&& [id, weight] : flatten_place_and_weights (trans.to, last, context))
              t[id].second = weight;
            pnet.transitions.push_back (std::move (t));
          }
      };

      struct save_place {
          template <typename Iterator, typename Context>
          void on_success(Iterator const& first, Iterator const& last,
                          ast::place& place, Context const& context) {
            auto& pnet = x3::get<savers> (context).get ();
            pnet.place_name_to_place_id[place] = pnet.nplaces++;
          }
      };

      struct save_target {
          template <typename Iterator, typename Context>
          void on_success(Iterator const& first, Iterator const& last,
                          ast::target& target, Context const& context) {
            auto& pnet = x3::get<savers> (context).get ();
            typename decltype (pnet.targets)::value_type target_to_push;

            for (auto&& t : target.targets)
              for (auto&& p : flatten_place_and_weights (t, last, context))
                target_to_push[p.first] = p.second;


            pnet.targets.push_back (std::move (target_to_push));
          }
      };

      struct save_init {
          template <typename Iterator, typename Context>
          void on_success(Iterator const& first, Iterator const& last,
                          ast::init& init, Context const& context) {
            auto& pnet = x3::get<savers> (context).get ();
            try {
              pnet.init[pnet.place_name_to_place_id.at (init.name)] = init.weight;
            }
            catch (const std::out_of_range& e) {
              throw x3::expectation_failure<Iterator> (first, "known place");
            }
          }
      };
  };

  namespace parser {
    namespace x3 = boost::spirit::x3;

    struct error_handler {
        template <typename Iterator, typename Exception, typename Context>
        x3::error_handler_result on_error (Iterator& first, Iterator const& last,
                                           Exception const& x, Context const& context) {
          auto& error_handler = x3::get<x3::error_handler_tag> (context).get ();
          std::string message = "Error! Expecting " + x.which () + " at:";
          error_handler (x.where (), message);
          return x3::error_handler_result::rethrow;
        }
    };


    namespace ascii = boost::spirit::x3::ascii;

    using x3::int_;
    using x3::double_;
    using x3::lexeme;
    using x3::lit;
    using x3::attr;
    using ascii::char_;
    using ascii::alnum;

    static auto const comment = lexeme [
      "/*" >> *(char_ - "*/") >> "*/"
      | "%" >> *~char_("\r\n") >> x3::eol
      ];

    struct quoted_string_class;
    x3::rule<quoted_string_class, std::string> const quoted_string = "quoted_string";

#define DEFINE_RULE(c)  x3::rule<class c ## _class, ast::c> const c = # c;

    DEFINE_RULE (place);
    DEFINE_RULE (place_and_weight);
    DEFINE_RULE (place_and_weight_vec);
    DEFINE_RULE (place_and_weight_vec_and_weight);
    DEFINE_RULE (place_and_weight_vec_and_weight_vec);
    DEFINE_RULE (transition);
    DEFINE_RULE (init);
    DEFINE_RULE (target);
    DEFINE_RULE (statement);

    auto const quoted_string_def = lexeme['\'' > + (char_ - '\'') > '\''];

    auto const place_def = lit ("place") > '(' > quoted_string > ')';

    auto const place_and_weight_def = (quoted_string > attr (std::nullopt))
      | ('(' > quoted_string > ',' > int_ > ')');

    auto const place_and_weight_vec_def = '[' > - (place_and_weight > * (',' > place_and_weight)) > ']';
    auto const place_and_weight_vec_and_weight_def = (place_and_weight_vec > attr (std::nullopt))
      | ('(' > place_and_weight_vec > ',' > int_ > ')');
    auto const place_and_weight_vec_and_weight_vec_def = '[' >
      - (place_and_weight_vec_and_weight > * (',' > place_and_weight_vec_and_weight))
      > ']';

    auto const transition_def = lit ("transition") > '(' > lexeme[+ (alnum)] > ',' >
      place_and_weight_vec_and_weight > ',' > place_and_weight_vec_and_weight > ')';

    auto const init_def = lit ("init") > '(' > quoted_string > ',' > int_ > ')';
    auto const target_def = lit ("target") > '(' > int_ > ',' > place_and_weight_vec_and_weight_vec > ')';

    auto const statement_def = (place | init | target | transition) > '.';

    auto const top = * (statement);

    BOOST_SPIRIT_DEFINE (quoted_string, place, place_and_weight, place_and_weight_vec,
                         place_and_weight_vec_and_weight, place_and_weight_vec_and_weight_vec,
                         transition, init, target, statement);

    struct statement_class : error_handler {};
    struct place_class : savers::save_place {};
    struct transition_class : savers::save_transition {};
    struct target_class : savers::save_target {};
    struct init_class : savers::save_init {};
  }
}


petri_net::petri_net (const std::string& fname) {
  using boost::spirit::x3::ascii::space;
  using boost::spirit::x3::with;
  using boost::spirit::x3::error_handler_tag;
  using petri_pl::parser::top;

  // Load up the file in a mmaped location
  boost::iostreams::mapped_file_source mm (fname);
  auto iter = mm.begin (), end = mm.end ();

  // Error handling
  using error_handler_type = boost::spirit::x3::error_handler<decltype (iter)>;
  error_handler_type error_handler (iter, end, std::cerr);

  // Pass to the parser the error_handler and the Petri net
  const auto parser = with<error_handler_tag> (std::ref (error_handler)) [
    with<petri_pl::savers> (std::ref (*this)) [
      top
      ]];


  try {
    bool r = boost::spirit::x3::phrase_parse (iter, end, parser, space | petri_pl::parser::comment);
    if (not r or iter != end) {
      error_handler (iter, end, "Error! Unexpected token here:");
      throw parse_error ();
    }
  }
  catch (boost::spirit::x3::expectation_failure<decltype (iter)>&) {
    throw parse_error ();
  }
}
