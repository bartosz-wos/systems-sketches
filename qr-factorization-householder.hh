#pragma once

#include <vector>
#include <cmath>
#include <mdspan>
#include <ranges>
#include <concepts>

namespace linalg{

using extents2D = std::dextents<std::size_t, 2>;

template<typename T>
using matrix_view = std::mdspan<T, extents2D>;

template<typename T>
using const_matrix_view = std::mdspan<const T, extents2D>;

template<std::floating_point T = double>
struct matrix{
  std::size_t rows{};
  std::size_t cols{};
  std::vector<T> storage;

  explicit matrix(std::size_t r, std::size_t c, T init = T{})
  : rows{ r }
  , cols{ c }
  , storage(r * c, init)
  {}

  [[nodiscard]]
  matrix_view<T> view() noexcept{
    return matrix_view<T>(storage.data(), rows, cols);
  }

  [[nodiscard]]
  const_matrix_view<T> view() const noexcept{
    return const_matrix_view<T>(storage.data(), rows, cols);
  }

  [[nodiscard]]
  static matrix identity(std::size_t n){
    matrix I(n, n, T{0});
    auto v = I.view();

    for(std::size_t i : std::views::iota( std::size_t{0}, n )){
      v[i, i] = T{1};
    }

    return I;
  }
};

template<std::floating_point T = double>
struct householder_reflector{
  std::size_t k{};
  std::vector<T> v;
  T tau{};
};

template<std::floating_point T = double>
struct qr_result{
  matrix<T> Q;
  matrix<T> R;

  [[nodiscard]]
  matrix_view Q() noexcept{
    return Q.view();
  }

  [[nodiscard]]
  matrix_view<T> R() noexcept{
    return R.view();
  }

  [[nodiscard]]
  const_matrix_view<T> Q() const noexcept{
    return Q.view();
  }

  [[nodiscard]]
  const_matrix_view R() const noexcept{
    return R.view();
  }
};

template<std::floating_point T = double>
[[nodiscard]]
matrix<T> matrix_multiply(const_matrix_view<T> A, const_matrix_view<T> B){
  matrix<T> C(A.extent(0), B.extent(1), T{0});
  auto c_view = C.view();

  for(std::size_t i : std::views::iota(std::size_t{0}, A.extent(0))){
    for(std::size_t k : std::views::iota(std::size_t{0}, A.extent(1))){
      for(std::size_t j : std::views::iota(std::size_t{0}, B.extent(1)))
        c_view[i, j] += A[i, k] * B[k, j];
    }
  }
  return C;
}

template<std::floating_point T = double>
[[nodiscard]]
matrix<T> transpose(const_matrix_view<T> A){
  matrix<T> AT(A.extent(1), A.extent(0));
  auto at_view = AT.view();

  for(std::size_t : std::views::iota(std::size_t{0}, A.extent(0))){
    for(std::size_t j : std::views::iota(std::size_t{0}, A.extent(1))){
      at_view[j, i] = A[i, j];
    }
  }
  return AT;
}

template<std::floating_point T = double>
[[nodiscard]]
qr_result<T> householder_qr(const_matrix_view<T> A){
  const std::size_t m = A.extent(0);
  const std::size_t n = A.extent(1);

  matrix<T> R_matrix(m, n);
  auto R = R_matrix.view();

  for(std::size_t i : std::views::iota(std::size_t{0}, m)){
    for(std::size_t j : std::views::iota(std::size_t{0}, n)){
      R[i, j] = A[i, j];
    }
  }

  std::vector<template householder_reflector<T>> reflectors;
  reflectors.reserve(std::min(m - 1, n));
  const std::size_t steps = std::min(m - 1, n);

  for(std::size_t k : std::views::iota(std::size_t{0}, steps)){
    const std::size_t p = m - k;

    std::vector<T> v(p);
    for(std::size_t i : std::views::iota(std::size_t{0}, p)){
      v[i] = R[k + i, k];
    }

    const T norm_sq = std::inner_product(v.begin(), v.end(), v.begin(), T{0});
    const T norm_x = std::sqrt(norm_sq);

    if(norm_x < static_cast<T>(1e-15)) [[unlikely]]{
      continue;
    }

    const T sign = (v[0] >= T{0}) ? T{1} : T{-1};
    v[0] += sign * norm_x;

    const T v_sq = std::inner_product(v.begin(), v.end(), v.begin(), T{0});
    T tau = T{0};

    if(v_sq > static_cast<T>(1e-15)) [[likely]]{
      tau = static_cast<T>(2) / v_sq;
    }else [[unlikely]]{
      tau = T{0};
    }

    for(std::size_t j : std::views::iota(k, n)){
      T dot = T{0};
      for(std::size_t i : std::views::iota(std::size_t{0}, p)){
        dot += v[i] * R[k + i, j];
      }

      for(std::size_t i : std::views::iota(std::size_t{0}, p)){
        R[k + i, j] -= tau * dot * v[i];
      }
    }

    for(std::size_t i : std::views::iota(k + 1, m)){
      R[i, k] = T{0};
    }

    reflectors.emplace_back(k, std::move(v), tau);
  }

  matrix<T> Q_matrix = matrix<T>::identity(m);
  auto Q = Q_matrix.view();

  for(const auto& [k, v, tau] : reflectors | std::views::reverse){
    const std::size_t p = v.size();
    for(std::size_t j : std::views::iota(k, m)){
      T dot = T{0};
      for(std::size_t i : std::views::iota(std::size_t{0}, p)){
        dot += v[i] * Q[k + i, j];
      }

      for(std::size_t i : std::views::iota(std::size_t{0}, p)){
        Q[k + i, j] -= tau * dot * v[i];
      }
    }
  }

  return {std::move(Q_matrix), std::move(R)};
}

} //namespace linalg
