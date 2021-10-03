#pragma once

#include <utils/abcbdd.hh>

namespace labels {
  class abcbdd : public utils::abcbdd {
      using upper = utils::abcbdd;
    public:
      abcbdd () : utils::abcbdd () {}
      abcbdd (const utils::abcbdd& b) : utils::abcbdd (b) {}

      using letter_type = utils::abcbdd;

      static abcbdd fullset () {
        return abcbdd (utils::abcbdd::bddOne ());
      }

      abcbdd operator* (const abcbdd& other) const {
        return utils::abcbdd::operator* (other);
      }

      static abcbdd bddVar (int idx) { return upper::bddVar (idx); }
      static abcbdd bddZero () { return upper::bddZero (); }
      static abcbdd bddOne () { return upper::bddOne (); }


      bool empty () const {
        return this->isZero ();
      }

      std::tuple<abcbdd, abcbdd, abcbdd> partition (const abcbdd& other) const {
        return { *this * !other, *this * other, !*this * other };
      }

      auto operator<=> (const abcbdd& other) const {
        return this->GetBDD () <=> other.GetBDD ();
      }
  };
}
