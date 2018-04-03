#include "GameConfig.h"
#include "../nlohmann/json.hpp"
#include "Point.h"

using nlohmann::json;

GameConfig::GameConfig(const json &data) {
  food_mass = data["FOOD_MASS"];
  game_width = data["GAME_WIDTH"];
  game_height = data["GAME_HEIGHT"];
  game_ticks = data["GAME_TICKS"];
  inertia_factor = data["INERTION_FACTOR"];
  max_fragments_cnt = data["MAX_FRAGS_CNT"];
  speed_factor = data["SPEED_FACTOR"];
  ticks_til_fusion = data["TICKS_TIL_FUSION"];
  virus_radius = data["VIRUS_RADIUS"];
  virus_split_mass = data["VIRUS_SPLIT_MASS"];
  viscosity = data["VISCOSITY"];
}

bool GameConfig::is_point_inside(const Point &point) const {
  return point.x >= 0.0 && point.y >= 0.0 && point.x < game_width &&
         point.y < game_height;
}

double GameConfig::distance_to_border(const Point& point) const
{
    return std::min ({fabs (point.x), fabs (point.y), fabs (point.x - game_width), fabs (point.y - game_height)});
}
