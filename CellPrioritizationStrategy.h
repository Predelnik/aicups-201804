#pragma once

#include "Const.h"
#include "Context.h"
#include "multi_vector.h"
#include <array>
#include <optional>
#include <random>
#include "Strategy.h"

class Context;
class Response;

using Cell = std::array<int, 2>;
using CellSpeed = std::array<int, 2>;
using CellAccel = std::array<int, 2>;

class CellPrioritizationStrategy : public Strategy {
public:
  CellPrioritizationStrategy();
  Response get_response(const Context &context) override;
  void initialize(const GameConfig &config) override;

private:
  const Player *find_caughtable_enemy();
  bool try_run_away_from(const Point &enemy_pos);
  const Player *find_dangerous_enemy();
  const Player *find_weak_enemy();
  Response move_randomly() const;
  CellSpeed to_cell_speed(const MyPart &p, const Point &val) const;
  void check_visible_squares();
  Point from_cell_speed (const  MyPart &p, const CellSpeed &sp) const;
  void update_danger();
  const MyPart *nearest_my_part_to(const Point &point) const;
  void check_if_goal_is_reached();
  void update_blocked_cells();
  void update_enemies_seen();
  void update_last_tick_enemy_seen();
  void check_subgoal();
  void update();
  std::optional<Point> next_step_to_goal(double max_danger_level);
  Response move_to_goal_or_repriotize();
  std::optional<Point> best_food_pos() const;
  const Food *find_nearest_food();
  Point future_center(double time);
  Response continue_movement();
  Response stop();
    std::optional<Point> reset_goal(std::optional<Point> point);
    Cell point_cell(const Point &point) const;
    static Point cell_center(const Cell &cell);
  bool is_valid_cell(const Cell &cell) const;
  double cell_priority(const Cell &cell) const;
  template <typename T>
  void fill_circle(multi_vector<T, 2> &target, const T &val, const Point &pos,
                   double radius);

private:
  mutable std::default_random_engine m_re;
  const Context *ctx = nullptr;
  multi_vector<int, 2> last_tick_visited;
  multi_vector<int, 2> blocked_cell;
  multi_vector<int, 2> dangerous_enemy_seen_tick;
  multi_vector<double, 2> danger_map;
  int cell_x_cnt = 0;
  int cell_y_cnt = 0;
  std::optional<Point> goal, subgoal;
  std::vector<std::array<Point, 2>> debug_lines;
  int last_tick_enemy_seen = -100;
  int subgoal_reset_tick = 0;
  std::vector<Cell> cell_order;

  static constexpr int blocked_cell_recheck_frequency = 200;
  static constexpr int randomize_frequency = 50;
  static constexpr double standing_speed = 1.0;
  static constexpr double cell_size = 15;
  static constexpr double new_opportunity_coeff =
      2.0; // if expected food amount around near is more than this coeff *
           // amount in current cell then go there
  static constexpr int better_opportunity_search_resolution = 5;
  static constexpr int new_opportunity_frequency = 30;
  static constexpr int ignore_viruses_when_enemy_was_not_seen_for = 400;
  static constexpr int split_if_enemy_was_not_seen_for = 200;
  static constexpr int max_parts_deliberately = 4;
  static constexpr auto goal_distance_to_justify_split = 200.0;
};
