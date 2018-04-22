#pragma once
#include "../nlohmann/json.hpp"
#include "GameConfig.h"

#include "KnownPlayer.h"
#include "Object.h"
#include "Point.h"
#include <random>

using nlohmann::json;

class KnownPlayer;
class Food;
class Ejection;
class Player;
class Virus;
class Enemy;

class EnemyVision {
public:
  KnownPlayer state;
  int tick = 0;
};

class Context {
public:
  Context();
  ~Context();
  void initialize(const json &data);
  void update(const json &data);
  void remove_enemies_older_than(int n_ticks) const;
  inline bool is_debug_tick() const
  {
#ifdef CUSTOM_DEBUG
      return tick >= debug_tick_start && tick < debug_tick_end;
#else
      return false;
#endif
  }

private:
  void fill_objects(const json &data);
  void update_my_radius();
  void update_total_mass();
  void update_largest_part();
  void update_speed_data();
  void update_food_map();
  void update_enemy_speed();
  void clean_up_eaten_or_fused_enemies();
  void update_enemies_by_id();
  void update_caches();
  void update_my_center();

public:
  GameConfig config;
  std::vector<KnownPlayer> my_parts;
  std::vector<Food> food;
  std::vector<Ejection> ejections;
  std::vector<KnownPlayer> enemies;
  std::vector<Virus> viruses;

  std::multimap<Point, Food *> food_map;
  std::map<PartId, KnownPlayer> prev_tick_enemy_by_id;
  mutable std::map<PartId, EnemyVision> enemy_by_id;
  mutable std::multimap<int, PartId> enemy_ids_seen_by_tick;

  Point my_center;
  double my_radius;
  double my_total_mass;
  double speed_angle;
  int tick = 0;
  Point avg_speed;
  KnownPlayer *my_largest_part = nullptr;
  int debug_tick_start = -1, debug_tick_end = -1;

private:
  std::default_random_engine m_re;
};
