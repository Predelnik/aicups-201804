#pragma once
#include "../nlohmann/json.hpp"
#include "Point.h"
#include "Object.h"

using nlohmann::json;

class MyPart : public Player {
public:
  explicit MyPart(const json &data);
  explicit MyPart() = default;
  double visibility_radius(int fragment_cnt) const;

  Point visibility_center() const { return pos + speed.normalized() * 10.0; }

  bool is_visible(int fragment_cnt, const Point &pos) const {
    return visibility_center().squared_distance_to(pos) <
           pow(visibility_radius(fragment_cnt), 2.0);
  }

public:
  std::string id;
  Point speed;
  int ttf = 0;
};

bool is_visible(const std::vector<MyPart> &parts, const Point &pos);
