﻿#pragma once

#include "Context.h"
#include "multi_vector.h"
#include <array>
#include <optional>
#include <random>

class Context;
class Response;

using Cell = std::array<int, 2>;

class Strategy {
public:
  Strategy();
  Response get_response(const Context &context);
  void initialize(const GameConfig &config);

private:
    bool try_run_away_from(const Point& enemy_pos);
  const Player *find_dangerous_enemy();
  const Player *find_weak_enemy();
  Response move_randomly() const;
  void check_visible_squares();
  void update_danger();
  void update_goal();
  void update_blocked_cells();
  void update();
  Response next_step_to_goal();
  Response move_to_goal_or_repriotize();
  const Food *find_nearest_food();
  Point future_center(double time);
  Response continue_movement();
  Response stop();
  Cell point_cell(const Point &point) const;
  Point cell_center(const Cell &cell) const;
  bool is_valid_cell(const Cell &cell) const;
  double cell_priority(const Cell &cell) const;

private:
  mutable std::default_random_engine m_re;
  const Context *ctx = nullptr;
  multi_vector<int, 2> last_tick_visited;
  multi_vector<int, 2> blocked_cell;
  multi_vector<double, 2> danger_map;
  int cell_x_cnt = 0;
  int cell_y_cnt = 0;
  std::optional<Point> goal;

  static constexpr int blocked_cell_recheck_frequency = 200;
  static constexpr int randomize_frequency = 50;
  static constexpr double standing_speed = 1.0;
  static constexpr double cell_size = 30.0;
  static constexpr double new_opportunity_coeff =
      2.0; // if expected food amount around near is more than this coeff *
           // amount in current cell then go there
  static constexpr int better_opportunity_search_resolution = 5;
  static constexpr int new_opportunity_frequency = 30;
};
