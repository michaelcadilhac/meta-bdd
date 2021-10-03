#pragma once
#include <map>
#include "abcbdd.hxx"

namespace utils {
  namespace d = detail;
  class abcbdd {
    public:
      using remap_t = std::map<int, abcbdd>;

    private:
      abcbdd (const int bdd) : bdd {bdd} {}

    public:
      static void init (int vars, int objs) { d::global_bddman = d::Abc_BddManAlloc (vars, objs); }

      abcbdd () {}
      abcbdd (const abcbdd& other) : bdd {other.bdd} { }
      static abcbdd bddVar (int index) { return d::Abc_BddIthVar (index); }
      static abcbdd bddOne () { return 1; }
      static abcbdd bddZero () { return 0; }

      abcbdd operator!() const { return d::Abc_LitNot (bdd); }
      abcbdd operator~() const { return d::Abc_LitNot (bdd); }

      bool operator== (const abcbdd&) const = default;

      abcbdd operator* (const abcbdd& other) const { return d::Abc_BddAnd (d::global_bddman, bdd, other.bdd); }
      abcbdd& operator*= (const abcbdd& other) { return *this = *this * other; }
      abcbdd operator& (const abcbdd& other) const { return *this * other; }
      abcbdd& operator&= (const abcbdd& other) { return *this = *this * other; }

      abcbdd operator+ (const abcbdd& other) const { return d::Abc_BddOr (d::global_bddman, bdd, other.bdd); }
      abcbdd& operator+= (const abcbdd& other) { return *this = *this + other; }
      abcbdd operator| (const abcbdd& other) const { return *this + other; }
      abcbdd& operator|= (const abcbdd& other) { return *this = *this + other; }

      abcbdd operator^ (const abcbdd& other) const { return (*this) * !other + !(*this) * other; }
      abcbdd& operator^= (const abcbdd& other) { return *this = *this ^ other; }

      int isTerminal () const { return bdd == 0 or bdd == 1; }
      int isOne () const { return bdd == 1; }
      int isZero () const { return bdd == 0; }

      int TopVar () const { return d::Abc_BddVar (d::global_bddman, bdd); }
      abcbdd Then () const { return d::Abc_BddThen (d::global_bddman, bdd); }
      abcbdd Else () const { return d::Abc_BddElse (d::global_bddman, bdd); }
      abcbdd And (const abcbdd& other) const { return d::Abc_BddAnd (d::global_bddman, bdd, other.bdd); }
      abcbdd Or (const abcbdd& other) const { return d::Abc_BddOr (d::global_bddman, bdd, other.bdd); }

      abcbdd Remap (const remap_t& map) const {
        if (isZero () or isOne ()) return *this;
        auto var = TopVar ();
        auto el = map.find (var);
        auto mapped_to = bddVar (var);
        if (el != map.end ()) // preserve
          mapped_to = el->second;
        return mapped_to * Then ().Remap (map) |
          !mapped_to * Else ().Remap (map);
      }

      auto GetBDD () const { return bdd; }

    private:
      int bdd = 0;
  };
}
