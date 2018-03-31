#pragma once
#include "../nlohmann/json.hpp"
#include "Point.h"

using nlohmann::json;

class MyPart {
public:
  explicit MyPart(const json &data);
  explicit MyPart() = default;
  double visibility_radius(int fragment_cnt) const;

  Point visibility_center() const { return center + speed.normalized() * 10.0; }

  bool is_visible(int fragment_cnt, const Point &pos) const {
    return visibility_center().squared_distance_to(pos) <
           pow(visibility_radius(fragment_cnt), 2.0);
  }

public:
  double radius = 0.0;
  double mass = 0.0;
  std::string id;
  Point center;
  Point speed;
  int ttf = 0;
};

bool is_visible(const std::vector<MyPart> &parts, const Point &pos);
