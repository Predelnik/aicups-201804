#pragma once
#include "../nlohmann/json.hpp"
#include "Point.h"

using nlohmann::json;

class MyPart {
public:
  explicit MyPart(const json &data);
  explicit MyPart () = default;

public:
  double radius = 0.0;
  double mass = 0.0;
  std::string id;
  Point pos;
  Point speed;
  int ttf = 0;
};
