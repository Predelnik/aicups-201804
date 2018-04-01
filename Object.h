#pragma once
#include "../nlohmann/json.hpp"

#include "Point.h"

#include <variant>

using nlohmann::json;

class ObjectBase {
public:
  ObjectBase(const json &data);

protected:
  ObjectBase () = default;
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
class Player : public ObjectBase {
public:
  explicit Player(const json &data);
  bool can_eat (double opponent_mass) const;

protected:
  Player () = default;

public:
  double mass = 0.0;
  double radius = 0.0;
  std::string id;
};

using Object = std::variant<std::monostate, Food, Ejection, Virus, Player>;

Object to_object(const json &data);
