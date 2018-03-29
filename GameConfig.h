#pragma once

#include "../nlohmann/json.hpp"
using nlohmann::json;

class GameConfig {
public:
  GameConfig() = default;
  explicit GameConfig(const json &data);

public:
  int food_mass = 0;
  int game_width = 0;
  int game_height = 0;
  int game_ticks = 0;
  double inertia_factor = 0;
  int max_fragments_cnt = 0;
  double speed_factor = 0;
  int ticks_til_fusion = 0;
  double virus_radius = 0;
  double virus_split_mass = 0;
  double viscosity = 0;
};
