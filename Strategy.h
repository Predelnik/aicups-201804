#pragma once

#include "Context.h"
#include "multi_vector.h"
#include <random>
#include <array>

class Context;
class Response;

class Strategy {
public:
  Strategy();
  const Player *find_dangerous_enemy();
  Response run_away_from(const Point &pos) const;
  Response move_randomly() const;
  void check_visible_squares();
  void update();
  Response move_to_more_food();
  Response get_response(const Context &context);
  void initialize(const GameConfig &config);

private:
  const Food *find_nearest_food();
  Point future_center(double time);
  Response continue_movement();
  std::array<int, 2> point_cell (const Point &point) const;
  Point cell_center(const std::array<int, 2> &cell) const;

private:
  mutable std::default_random_engine m_re;
  const Context *ctx = nullptr;
  multi_vector<double, 2> expected_food;
  double food_expectancy_inc = 0.0;

  static inline int randomize_frequency = 50;
  static inline double standing_speed = 1.0;
  static inline double cell_size = 30.0;
};
