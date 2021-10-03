#pragma once
#include <map>
#define register /* register */
#include <buddy/masterbdd.hh>

extern masterbdd global_mbuddy;

namespace utils {
  class buddybdd {
    public:
      using remap_t = std::map<int, buddybdd>;

    private:
      buddybdd (const int bdd) : bdd {bdd} {}

    public:
      static void init (int vars, int objs) {
        global_mbuddy.bdd_init (/*nodesize*/ objs, /*cachesize*/ objs);
        global_mbuddy.bdd_setvarnum (vars);
      }

      buddybdd () {}
      buddybdd (const buddybdd& other) : bdd {other.bdd} { }
      static buddybdd bddVar (int index) { return global_mbuddy.bdd_ithvar (index); }
      static buddybdd bddOne () { return 1; }
      static buddybdd bddZero () { return 0; }

      buddybdd operator!() const { return global_mbuddy.bdd_not (bdd); }
      buddybdd operator~() const { return !(*this); }

      bool operator== (const buddybdd&) const = default;

      buddybdd operator* (const buddybdd& other) const { return global_mbuddy.bdd_apply (bdd, other.bdd, bddop_and); }
      buddybdd& operator*= (const buddybdd& other) { return *this = *this * other; }
      buddybdd operator& (const buddybdd& other) const { return *this * other; }
      buddybdd& operator&= (const buddybdd& other) { return *this = *this * other; }

      buddybdd operator+ (const buddybdd& other) const { return global_mbuddy.bdd_apply (bdd, other.bdd, bddop_or); }
      buddybdd& operator+= (const buddybdd& other) { return *this = *this + other; }
      buddybdd operator| (const buddybdd& other) const { return *this + other; }
      buddybdd& operator|= (const buddybdd& other) { return *this = *this + other; }

      buddybdd operator^ (const buddybdd& other) const { return (*this) * !other + !(*this) * other; }
      buddybdd& operator^= (const buddybdd& other) { return *this = *this ^ other; }

      int isTerminal () const { return bdd == 0 or bdd == 1; }
      int isOne () const { return bdd == 1; }
      int isZero () const { return bdd == 0; }

      int TopVar () const { return global_mbuddy.bdd_var (bdd); }
      buddybdd Then () const { return global_mbuddy.bdd_high (bdd); }
      buddybdd Else () const { return global_mbuddy.bdd_low (bdd); }

      buddybdd Remap (const remap_t& map) const {
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
