// Copyright 2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_STORAGE_ADAPTOR_HPP
#define BOOST_HISTOGRAM_STORAGE_ADAPTOR_HPP

#include <algorithm>
#include <array>
#include <boost/assert.hpp>
#include <boost/histogram/detail/cat.hpp>
#include <boost/histogram/detail/meta.hpp>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace boost {
namespace histogram {
namespace detail {

template <typename B, typename T>
struct element_adaptor_impl;

template <typename T>
struct element_adaptor_impl<std::true_type, T> {
  static void inc(T& t) { t(); }
  template <typename U>
  static void add(T& t, U&& u) {
    t(std::forward<U>(u));
  }
};

template <typename T>
struct element_adaptor_impl<std::false_type, T> {
  static void inc(T& t) { ++t; }
  template <typename U>
  static void add(T& t, U&& u) {
    t += std::forward<U>(u);
  }
};

template <typename T>
using element_adaptor = element_adaptor_impl<is_callable<T>, T>;

template <typename T>
struct augmentation_error {};

template <typename T>
struct vector_augmentation : T {
  using value_type = typename T::value_type;

  using T::T;
  vector_augmentation(const T& t) : T(t) {}
  vector_augmentation(T&& t) : T(std::move(t)) {}

  void reset(std::size_t n) {
    this->resize(n);
    std::fill(this->begin(), this->end(), value_type());
  }

  template <typename U>
  void set(std::size_t i, U&& u) {
    (*this)[i] = std::forward<U>(u);
  }
};

template <typename T>
struct array_augmentation : T {
  using value_type = typename T::value_type;

  using T::T;
  array_augmentation(const T& t) : T(t) {}
  array_augmentation(T&& t) : T(std::move(t)) {}

  void reset(std::size_t n) {
    if (n > this->max_size()) // for std::array
      throw std::runtime_error(
          detail::cat("size ", n, " exceeds maximum capacity ", this->max_size()));
    std::fill_n(this->begin(), n, value_type());
    size_ = n;
  }

  template <typename U>
  void set(std::size_t i, U&& u) {
    (*this)[i] = std::forward<U>(u);
  }

  std::size_t size_ = 0;
  std::size_t size() const { return size_; }
};

template <typename T>
struct map_augmentation : T {
  static_assert(std::is_same<typename T::key_type, std::size_t>::value,
                "map must have key_type equal to std::size_t");
  using value_type = typename T::mapped_type;

  using T::T;
  map_augmentation(const T& t) : T(t) {}
  map_augmentation(T&& t) : T(std::move(t)) {}

  void reset(std::size_t n) {
    this->clear();
    size_ = n;
  }

  value_type operator[](std::size_t i) const {
    auto it = this->find(i);
    return it == this->end() ? value_type() : *it;
  }

  template <typename U>
  void set(std::size_t i, U&& u) {
    auto it = this->find(i);
    if (u == value_type()) {
      if (it != this->end()) this->erase(it);
    } else if (it != this->end())
      *it = std::forward<U>(u);
    else
      this->insert(i, std::forward<U>(u));
  }

  std::size_t size_ = 0;
  std::size_t size() const { return size_; }
};

template <typename T>
using storage_augmentation = mp11::mp_if<
    is_vector_like<T>, vector_augmentation<T>,
    mp11::mp_if<is_array_like<T>, array_augmentation<T>,
                mp11::mp_if<is_map_like<T>, map_augmentation<T>, augmentation_error<T>>>>;

} // namespace detail

/// generic implementation for std::array, vector-like, and map-like containers
template <typename T>
struct storage_adaptor : detail::storage_augmentation<T>, storage_tag {

  using base_type = detail::storage_augmentation<T>;
  using value_type = typename base_type::value_type;
  using element_adaptor = detail::element_adaptor<value_type>;
  using const_reference = const value_type&;

  using base_type::base_type;
  storage_adaptor() = default;
  storage_adaptor(const storage_adaptor&) = default;
  storage_adaptor& operator=(const storage_adaptor&) = default;
  storage_adaptor(storage_adaptor&&) = default;
  storage_adaptor& operator=(storage_adaptor&&) = default;

  storage_adaptor(const T& t) : base_type(t) {}
  storage_adaptor(T&& t) : base_type(std::move(t)) {}

  template <typename U, typename = detail::requires_storage<U>>
  storage_adaptor(const U& rhs) {
    (*this) = rhs;
  }

  template <typename U, typename = detail::requires_storage<U>>
  storage_adaptor& operator=(const U& rhs) {
    this->reset(rhs.size());
    for (std::size_t i = 0, n = this->size(); i < n; ++i) this->set(i, rhs[i]);
    return *this;
  }

  void operator()(std::size_t i) {
    BOOST_ASSERT(i < this->size());
    element_adaptor::inc((*this)[i]);
  }

  template <typename U>
  void operator()(std::size_t i, U&& u) {
    BOOST_ASSERT(i < this->size());
    element_adaptor::add((*this)[i], std::forward<U>(u));
  }

  // precondition: storages have equal size
  template <typename U, typename = detail::requires_storage<U>>
  storage_adaptor& operator+=(const U& u) {
    const auto n = this->size();
    BOOST_ASSERT_MSG(n == u.size(), "sizes must be equal");
    for (std::size_t i = 0; i < n; ++i) element_adaptor::add((*this)[i], u[i]);
    return *this;
  }

  storage_adaptor& operator*=(const double x) {
    for (std::size_t i = 0, n = this->size(); i < n; ++i) (*this)[i] *= x;
    return *this;
  }

  storage_adaptor& operator/=(const double x) { return operator*=(1.0 / x); }

  template <typename U, typename = detail::requires_storage<U>>
  bool operator==(const U& u) const {
    const auto n = this->size();
    if (n != u.size()) return false;
    for (std::size_t i = 0; i < n; ++i)
      if ((*this)[i] != u[i]) return false;
    return true;
  }
};
} // namespace histogram
} // namespace boost

#endif