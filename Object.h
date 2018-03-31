#pragma once
#include "../nlohmann/json.hpp"

#include "Point.h"

#include <variant>

using nlohmann::json;

class ObjectBase {
public:
  ObjectBase(const json &data);

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
  bool is_dangerous (double my_mass) const;

public:
  double mass;
  double radius;
  std::string id;
};

using Object = std::variant<std::monostate, Food, Ejection, Virus, Player>;

Object to_object(const json &data);
