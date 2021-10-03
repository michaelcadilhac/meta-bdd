#pragma once

namespace utils {
  template <typename T>
  struct ref_dlg_cmp {
      constexpr bool operator() (const std::reference_wrapper<T>& lhs,
                                 const std::reference_wrapper<T>& rhs) const {
        return lhs.get () < rhs.get ();
      }

      constexpr bool operator() (const std::reference_wrapper<T>& lhs,
                                 const T& rhs) const {
        return lhs.get () < rhs;
      }

      constexpr bool operator() (const T& lhs,
                                 const std::reference_wrapper<T>& rhs) const {
        return lhs < rhs.get ();
      }
  };
}
