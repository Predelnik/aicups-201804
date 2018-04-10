#pragma once

#include "Strategy.h"

#include "Defines.h"
#include "Point.h"
#include <deque>
#include <random>
#include <set>

class Context;

struct FoodSeen {
  Point pos;
  int tick;
};

class MaxSpeedStrategy : public Strategy {
public:
  MaxSpeedStrategy();
  Response get_response(const Context &context) override;
  void initialize(const GameConfig &config) override;

private:
  Response move_by_vector(const Point &v);
  double calc_angle_score(double angle);
  bool is_splitting_dangerous() const;
  Response get_response_impl(bool try_to_keep_speed);
  Response speed_case();
  Response no_speed_case();
  void remove_eaten_food();
  void remove_stale_food();
  void add_new_food_to_seen();
  void update();

private:
  std::deque<FoodSeen> m_food_seen;
  std::multiset<Point> m_food_seen_set;
#ifdef CUSTOM_DEBUG
  std::vector<std::array<Point, 2>> m_debug_lines;
#endif;

  constexpr static int food_shelf_life = 350;
  constexpr static int angle_discretization = DEBUG_RELEASE_VALUE(10, 20);
  constexpr static int future_scan_iteration_count =
      DEBUG_RELEASE_VALUE(10, 20);
  const Context *ctx;
  mutable std::default_random_engine m_re;
};