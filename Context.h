#pragma once
#include "../nlohmann/json.hpp"
#include "GameConfig.h"

#include "Point.h"

using nlohmann::json;

class MyPart;
class Food;
class Ejection;
class Player;
class Virus;

class Context {
public:
  Context();
  ~Context();
  void update_config(const json &data);
  void update(const json &data);

private:
  void fill_objects(const json &data);
  void update_my_radius();
  void update_total_mass();
  void update_largest_part();
  void update_caches();
  void update_my_center();

public:
  GameConfig config;
  std::vector<MyPart> my_parts;
  std::vector<Food> food;
  std::vector<Ejection> ejections;
  std::vector<Player> players;
  std::vector<Virus> viruses;

  Point my_center;
  double my_radius;
  double my_total_mass;
  int tick = 0;
  MyPart *my_largest_part = nullptr;
};
