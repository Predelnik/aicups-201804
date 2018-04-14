#pragma once

#include "../nlohmann/json.hpp"
using nlohmann::json;

class Point;

class GameConfig {
public:
  GameConfig() = default;
  explicit GameConfig(const json &data);
  bool is_point_inside (const Point &point) const;
  double distance_to_border (const Point &point) const;
  double game_max_size() const { return std::max (game_width, game_height);}
  Point game_size_vector () const;

public:
  double food_mass = 0;
  double game_width = 0;
  double game_height = 0;
  int game_ticks = 0;
  double inertia_factor = 0;
  int max_fragments_cnt = 0;
  double speed_factor = 0;
  int ticks_til_fusion = 0;
  double virus_radius = 0;
  double virus_split_mass = 0;
  double viscosity = 0;
};
