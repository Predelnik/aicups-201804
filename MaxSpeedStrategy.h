#pragma once

#include "Strategy.h"

#include "Point.h"
#include <random>
#include <deque>
#include <set>
#include "Defines.h"

class Context;

struct FoodSeen
{
  Point pos;
  int tick;
};

class MaxSpeedStrategy : public Strategy {
public:
  MaxSpeedStrategy();
  Response move_by_vector(const Point &v);
    Response response_for_angle(double min_angle, double max_angle);
    Response speed_case();
  Response no_speed_case();
  void remove_eaten_food();
  void remove_stale_food();
  void add_new_food_to_seen();
  void update();
  Response get_response(const Context &context) override;
  void initialize(const GameConfig &config) override;

private:
  std::deque<FoodSeen> m_food_seen;
  std::multiset<Point> m_food_seen_set;

  constexpr static int food_shelf_life = 350;
  constexpr static int angle_discretization = DEBUG_RELEASE_VALUE(20, 40);
  constexpr static int future_scan_iteration_count = DEBUG_RELEASE_VALUE(10, 20);
  const Context *ctx;
  mutable std::default_random_engine m_re;
};