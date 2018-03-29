#pragma once

#include <functional>

namespace detail {
template <typename ForwardIteratorType, typename Op, typename Comparison>
ForwardIteratorType min_max_element_op(ForwardIteratorType begin,
                                       ForwardIteratorType end, Op func,
                                       Comparison comp) {
  assert(begin != end);
  auto bestIt = begin;
  auto bestVal = func(*begin);
  ForwardIteratorType it = begin;
  ++it;
  for (; it != end; ++it) {
    auto val = func(*it);
    if (comp(val, bestVal)) {
      bestVal = val;
      bestIt = it;
    }
  }
  return bestIt;
};

} // namespace detail

template <typename ForwardIteratorType, typename Op>
ForwardIteratorType min_element_op(ForwardIteratorType begin,
                                   ForwardIteratorType end, Op func) {
  return detail::min_max_element_op(begin, end, func,
                                    std::less<decltype(func(*begin))>{});
};

template <typename ForwardIteratorType, typename Op>
ForwardIteratorType max_element_op(ForwardIteratorType begin,
                                   ForwardIteratorType end, Op func) {
  return detail::min_max_element_op(begin, end, func,
                                    std::greater<decltype(func(*begin))>{});
};
