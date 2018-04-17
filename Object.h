#pragma once
#include "../nlohmann/json.hpp"

#include "Point.h"

#include <variant>

class GameConfig;
class MovingPoint;
using nlohmann::json;

class ObjectBase {
public:
  ObjectBase(const json &data);

protected:
  ObjectBase() = default;

public:
  Point pos;
};

class Food : public ObjectBase {
  using ObjectBase::ObjectBase;
};
class Ejection : public ObjectBase {
  using ObjectBase::ObjectBase;
};
class Virus : public ObjectBase {
public:
  explicit Virus(const json &data);

public:
  double mass;
};

class PartId {
public:
  int player_num;
  int part_num;

public:
  bool operator<(const PartId &other) const {
    return std::tie(player_num, part_num) <
           std::tie(other.player_num, other.part_num);
  }
  bool operator==(const PartId &other) const {
    return std::tie(player_num, part_num) ==
           std::tie(other.player_num, other.part_num);
  }
};

class Player : public ObjectBase {
public:
  explicit Player(const json &data);
  double visibility_radius(int fragment_cnt) const;
  double max_speed(const GameConfig &config) const;
  Player(const Player &) = default;
  Player &operator=(const Player &) = default;

protected:
  Player() = default;

public:
  double mass = 0.0;
  double radius = 0.0;
  PartId id;
};

using Object = std::variant<std::monostate, Food, Ejection, Virus, Player>;

Object to_object(const json &data);
