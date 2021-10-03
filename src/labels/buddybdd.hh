#pragma once

#include <utils/buddybdd.hh>

namespace labels {
  class buddybdd : public utils::buddybdd {
      using upper = utils::buddybdd;

    public:
      buddybdd () : utils::buddybdd () {}
      buddybdd (const utils::buddybdd& b) : utils::buddybdd (b) {}

      using letter_type = utils::buddybdd;

      static buddybdd fullset () {
        return buddybdd (utils::buddybdd::bddOne ());
      }

      buddybdd operator* (const buddybdd& other) const {
        return utils::buddybdd::operator* (other);
      }
      static buddybdd bddVar (int idx) { return upper::bddVar (idx); }
      static buddybdd bddZero () { return upper::bddZero (); }
      static buddybdd bddOne () { return upper::bddOne (); }

      bool empty () const {
        return this->isZero ();
      }

      std::tuple<buddybdd, buddybdd, buddybdd> partition (const buddybdd& other) const {
        return { *this * !other, *this * other, !*this * other };
      }

      auto operator<=> (const buddybdd& other) const {
        return this->GetBDD () <=> other.GetBDD ();
      }
  };
}
