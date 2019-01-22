// Copyright 2015-2017 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_AXIS_INTERVAL_VIEW_HPP
#define BOOST_HISTOGRAM_AXIS_INTERVAL_VIEW_HPP

namespace boost {
namespace histogram {
namespace axis {

template <typename Axis>
class interval_view {
public:
  interval_view(const Axis& axis, int idx) : axis_(axis), idx_(idx) {}
  // avoid viewing a temporary that goes out of scope
  interval_view(Axis&& axis, int idx) = delete;

  /// Return lower edge of bin.
  decltype(auto) lower() const noexcept { return axis_.value(idx_); }
  /// Return upper edge of bin.
  decltype(auto) upper() const noexcept { return axis_.value(idx_ + 1); }
  /// Return center of bin.
  decltype(auto) center() const noexcept { return axis_.value(idx_ + 0.5); }
  /// Return width of bin.
  decltype(auto) width() const noexcept { return upper() - lower(); }

  template <typename BinType>
  bool operator==(const BinType& rhs) const noexcept {
    return lower() == rhs.lower() && upper() == rhs.upper();
  }

  template <typename BinType>
  bool operator!=(const BinType& rhs) const noexcept {
    return !operator==(rhs);
  }

private:
  const Axis& axis_;
  const int idx_;
};

} // namespace axis
} // namespace histogram
} // namespace boost

#endif
