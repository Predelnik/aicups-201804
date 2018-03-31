#pragma once


#include <vector>
#include <array>
#include <numeric>
#include <assert.h>

template <typename T, std::size_t DimensionCnt>
struct multi_vector;

namespace detail
{
  template <bool Condition, typename Expression>
  using add_const_if_t = std::conditional_t<Condition, std::add_const_t<Expression>, Expression>;

  template <typename T, std::size_t DimensionCnt, std::size_t DimensionLeftCnt, bool is_const>
  struct multi_dim_index_helper
  {
    multi_dim_index_helper (add_const_if_t<is_const, multi_vector<T, DimensionCnt>> &vec, std::size_t index_so_far) : m_vec (&vec), m_index_so_far (index_so_far) {}
    multi_dim_index_helper<T, DimensionCnt, DimensionLeftCnt - 1, is_const> operator[](std::size_t index)
    {
      assert (index < m_vec->sizes[DimensionCnt - DimensionLeftCnt]);
      return {*m_vec, m_index_so_far * m_vec->sizes[DimensionCnt - DimensionLeftCnt] + index};
    }
  private:
    add_const_if_t<is_const, multi_vector<T, DimensionCnt>> *m_vec;
    std::size_t m_index_so_far;
  };

  template <typename T, std::size_t DimensionCnt>
  struct multi_dim_index_helper<T, DimensionCnt, 1, false>
  {
    multi_dim_index_helper (multi_vector<T, DimensionCnt> &vec, std::size_t index_so_far) : m_vec (&vec), m_index_so_far (index_so_far) {}
    typename std::vector<T>::reference operator[](std::size_t index)
    {
      assert (index < m_vec->sizes[DimensionCnt - 1]);
      return m_vec->v[m_index_so_far * m_vec->sizes[DimensionCnt - 1]  + index];
    }
  private:
    multi_vector<T, DimensionCnt> *m_vec;
    std::size_t m_index_so_far;
  };

  template <typename T, std::size_t DimensionCnt>
  struct multi_dim_index_helper<T, DimensionCnt, 1, true>
  {
    multi_dim_index_helper (const multi_vector<T, DimensionCnt> &vec, std::size_t index_so_far) : m_vec (&vec), m_index_so_far (index_so_far) {}
    const T &operator[](std::size_t index)
    {
      assert (index < m_vec->sizes[DimensionCnt - 1]);
      return m_vec->v[m_index_so_far * m_vec->sizes[DimensionCnt - 1]  + index];
    }
  private:
    const multi_vector<T, DimensionCnt> *m_vec;
    std::size_t m_index_so_far;
  };
}

template <typename T, std::size_t Times>
struct n_times_initializer_list {
  using type = std::initializer_list<typename n_times_initializer_list<T, Times - 1>::type>;
};

template <typename T>
struct n_times_initializer_list<T, 1> {
  using type = std::initializer_list<T>;
};

template <typename T, std::size_t Times>
using n_times_initializer_list_t = typename n_times_initializer_list<T, Times>::type;

namespace detail
{
  template <typename T, std::size_t DimensionCnt, std::size_t CurLevel>
  struct fill_sizes_recursive {
    static void perform (multi_vector<T, DimensionCnt> &vec, n_times_initializer_list_t<T, CurLevel> ini_list) {
      vec.sizes[DimensionCnt - CurLevel] = ini_list.size ();
      fill_sizes_recursive<T, DimensionCnt, CurLevel - 1>::perform (vec, *ini_list.begin ());
    }
  };

  template <typename T, std::size_t DimensionCnt>
  struct fill_sizes_recursive<T, DimensionCnt, 1> {
    static void perform (multi_vector<T ,DimensionCnt> &vec, std::initializer_list<T> ini_list) {
      vec.sizes[DimensionCnt - 1] = ini_list.size ();
    }
  };

  template <typename T, std::size_t DimensionCnt, std::size_t CurLevel>
  struct fill_data_recursive {
  static void perform (multi_vector<T, DimensionCnt> &vec, std::size_t index_so_far, n_times_initializer_list_t<T, CurLevel> ini_list) {
    assert (ini_list.size () == vec.sizes[DimensionCnt - CurLevel]);
    for (std::size_t i = 0; i < vec.sizes[DimensionCnt - CurLevel]; ++i) {
      fill_data_recursive<T, DimensionCnt, CurLevel - 1>::perform (vec, index_so_far * vec.sizes[DimensionCnt - CurLevel] + i, *(ini_list.begin () + i));
    }
  }
  };

  template <typename T, std::size_t DimensionCnt>
  struct fill_data_recursive<T, DimensionCnt, 1> {
  static void perform (multi_vector<T, DimensionCnt> &vec, std::size_t index_so_far, std::initializer_list<T> ini_list) {
    assert (ini_list.size () == vec.sizes[DimensionCnt - 1]);
    for (std::size_t i = 0; i < vec.sizes[DimensionCnt - 1]; ++i) {
      vec.v[index_so_far * vec.sizes[DimensionCnt - 1] + i] = *(ini_list.begin () + i);
    }
  }
  };
}

template <typename T, std::size_t DimensionCnt>
struct multi_vector
{
  // multi dimensional vector helper class

private:
  using self = multi_vector;
  using inner_iterator = typename std::vector<T>::iterator;

public:
  multi_vector (){}
  template <typename ...ArgTypes>
  multi_vector (ArgTypes... dim)
  {
    static_assert (sizeof... (ArgTypes) == DimensionCnt, "Specify all dimension sizes");
    sizes = {{static_cast<size_t> (dim)...}};
    update_storage_size ();
  }

  multi_vector (n_times_initializer_list_t<T, DimensionCnt> ini_list) {
    detail::fill_sizes_recursive<T, DimensionCnt, DimensionCnt>::perform (*this, ini_list);
    update_storage_size ();
    detail::fill_data_recursive<T, DimensionCnt, DimensionCnt>::perform (*this, 0, ini_list);
  }

  detail::multi_dim_index_helper<T, DimensionCnt, DimensionCnt - 1, false> operator[](std::size_t index)
  {
    return detail::multi_dim_index_helper<T, DimensionCnt, DimensionCnt, false> {*this, 0}[index];
  }

  detail::multi_dim_index_helper<T, DimensionCnt, DimensionCnt - 1, true> operator[](std::size_t index) const
  {
    return detail::multi_dim_index_helper<T, DimensionCnt, DimensionCnt, true> {*this, 0}[index];
  }

  inner_iterator begin ()        { return v.begin (); }
  inner_iterator end ()          { return v.end (); }
  inner_iterator begin ()  const { return v.begin (); }
  inner_iterator end ()    const { return v.end (); }
  inner_iterator cbegin () const { return v.begin (); }
  inner_iterator cend ()   const { return v.end (); }
  std::size_t size (size_t dim) const { assert (dim < DimensionCnt); return sizes[dim]; }

  friend bool operator== (const self &lhs, const self &rhs) { return lhs.v == rhs.v; }
  friend bool operator!= (const self &lhs, const self &rhs) { return lhs.v != rhs.v; }
  friend bool operator< (const self &lhs, const self &rhs) { return lhs.v < rhs.v; }
  friend bool operator<= (const self &lhs, const self &rhs) { return lhs.v >= rhs.v; }
  friend bool operator> (const self &lhs, const self &rhs) { return lhs.v > rhs.v; }
  friend bool operator>= (const self &lhs, const self &rhs) { return lhs.v >= rhs.v; }

  template <typename ...ArgTypes>
  void resize (ArgTypes... dim)
  {
    static_assert (sizeof... (ArgTypes) == DimensionCnt, "Specify all dimension sizes");
    sizes = {{static_cast<size_t> (dim)...}};
    update_storage_size ();
    std::fill (v.begin (), v.end (), T{});
  }
  template <int Dimension>
  std::size_t get_size () const { return sizes[Dimension]; }

private:
  void update_storage_size () {
    v.resize (std::accumulate (sizes.begin (), sizes.end (), (size_t)1, std::multiplies<size_t>()));
  }

private:
  std::array<std::size_t, DimensionCnt> sizes;
  std::vector<T> v;
  template <typename, std::size_t, std::size_t, bool>
  friend struct detail::multi_dim_index_helper;
  template <typename, std::size_t, std::size_t>
  friend struct detail::fill_sizes_recursive;
    template <typename, std::size_t, std::size_t>
  friend struct detail::fill_data_recursive;
};
