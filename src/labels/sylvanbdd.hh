#pragma once

#include <sylvan.h>
#include <sylvan_obj.hpp>

namespace labels {
  struct sylvanbdd_letter : public sylvan::Bdd {
      // This is rarely needed, only by implementations that are not letter-agnostic.
      // (currently, upset::contains).
      sylvanbdd_letter& operator= (const sylvan::Bdd& other) {
        sylvan::Bdd::operator= (other);
        return *this;
      }
  };

  class sylvanbdd : public sylvan::Bdd {
      using upper = sylvan::Bdd;
    public:
      sylvanbdd () : upper () {}
      sylvanbdd (const upper& b) : upper (b) {}
      using letter_type = sylvanbdd_letter;

      static sylvanbdd fullset () {
        return sylvanbdd (upper::bddOne ());
      }

      sylvanbdd operator* (const sylvanbdd& other) const {
        return upper::operator* (other);
      }

      static sylvanbdd bddVar (int idx) { return upper::bddVar (idx); }
      static sylvanbdd bddZero () { return upper::bddZero (); }
      static sylvanbdd bddOne () { return upper::bddOne (); }

      bool empty () const {
        return this->isZero ();
      }

      std::tuple<sylvanbdd, sylvanbdd, sylvanbdd>
      partition (const sylvanbdd& other) const {
        return { *this * !other, *this * other, !*this * other };
      }

      auto operator<=> (const sylvanbdd& other) const {
        return this->GetBDD () <=> other.GetBDD ();
      }
  };
}
