#pragma once
#include "../nlohmann/json.hpp"
#include "Object.h"
#include "Point.h"

using nlohmann::json;

class KnownPlayer : public Player {
public:
  explicit KnownPlayer(const json &data);
  explicit KnownPlayer() = default;
  explicit KnownPlayer(const Player &player);

  MovingPoint as_moving_point() const;
  Point visibility_center() const { return pos + speed.normalized() * 10.0; }

  bool is_visible(int fragment_cnt, const Point &pos) const {
    return visibility_center().squared_distance_to(pos) <
           pow(visibility_radius(fragment_cnt), 2.0);
  }

public:
  Point speed;
  int ticks_to_fuse = 0;
};

bool is_visible(const std::vector<KnownPlayer> &parts, const Point &pos);
