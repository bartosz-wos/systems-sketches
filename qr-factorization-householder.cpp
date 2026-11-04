#pragma once

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


} //namespace linalg
