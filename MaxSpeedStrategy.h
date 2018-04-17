#pragma once

#include "Strategy.h"

#include "Defines.h"
#include "KnownPlayer.h"
#include "Point.h"
#include <deque>
#include <random>
#include <set>

class Context;

struct FoodSeen {
  Point pos;
  int tick;
};

struct EnemySeen
{
    PartId id;
    int tick;
};

class MaxSpeedStrategy : public Strategy {
  constexpr static int food_shelf_life = 350;
  constexpr static int angle_partition_count = DEBUG_RELEASE_VALUE(20, 40);
  constexpr static int future_scan_iteration_count =
      DEBUG_RELEASE_VALUE(10, 20);

public:
  MaxSpeedStrategy();
  Point border_point_by_vector(const Point &v);
  Response get_response(const Context &context) override;
  void initialize(const GameConfig &config) override;

private:
  Response move_by_vector(const Point &v);
  void calculate_fusions(const std::vector<KnownPlayer> &enemies);
  int get_scan_precision() const;
  double calc_target_score(const Point &target);
  bool is_splitting_dangerous() const;
  void calculate_predicted_enemies();
  Response get_response_impl();
  void remove_eaten_food();
  void remove_stale_food();
  void add_new_food_to_seen();
  void update();

private:
  std::deque<FoodSeen> m_food_seen;
  std::multiset<Point> m_food_seen_set;
  std::vector<KnownPlayer> m_fusions;
  std::array<std::vector<KnownPlayer>, future_scan_iteration_count>
      m_predicted_enemies;

  std::deque<EnemySeen> m_enemies_seen;
  std::multiset<double> m_enemies_masses;
#ifdef CUSTOM_DEBUG
  std::vector<std::array<Point, 2>> m_debug_lines;
  std::vector<std::string> m_debug_line_colors;
#endif;
  const Context *ctx;
  mutable std::default_random_engine m_re;
};