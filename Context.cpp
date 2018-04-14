﻿#include "Context.h"
#include "KnownPlayer.h"
#include "Object.h"
#include "overload.h"

#include "GameHelpers.h"
#include "algorithm.h"
#include <variant>

namespace {
std::vector<KnownPlayer> to_my_parts(const json &data) {
  std::vector<KnownPlayer> out(data.size());
  std::transform(data.begin(), data.end(), out.begin(),
                 [](const json &data) { return KnownPlayer(data); });
  return out;
}

} // namespace
Context::Context() = default;
Context::~Context() = default;

void Context::update_config(const json &data) { config = GameConfig{data}; }

void Context::update(const json &data) {
  ++tick;
  my_parts = to_my_parts(data["Mine"]);
  std::sort(
      my_parts.begin(), my_parts.end(),
      [](const auto &lhs, const auto &rhs) { return lhs.mass > rhs.mass; });
  fill_objects(data["Objects"]);
  std::sort(
      enemies.begin(), enemies.end(),
      [](const auto &lhs, const auto &rhs) { return lhs.mass > rhs.mass; });
  update_caches();
}

void Context::fill_objects(const json &data) {
  food.clear();
  viruses.clear();
  enemies.clear();
  ejections.clear();
  for (auto &element : data) {
    std::visit(
        overload(
            [this](const Food &food_piece) { food.push_back(food_piece); },
            [this](const Virus &virus) { viruses.push_back(virus); },
            [this](const Player &player) { enemies.emplace_back(player); },
            [this](const Ejection &ejection) { ejections.push_back(ejection); },
            [](std::monostate) {}),
        to_object(element));
  }
}

void Context::update_my_radius() {
  my_radius = 0.0;
  for (auto &p : my_parts)
    my_radius = std::max(my_radius, p.pos.distance_to(my_center) + p.radius);
}

void Context::update_total_mass() {
  my_total_mass = 0.0;
  for (auto &p : my_parts)
    my_total_mass += p.mass;
}

void Context::update_largest_part() {
  if (my_parts.empty()) {
    my_largest_part = nullptr;
    return;
  }
  my_largest_part =
      &*max_element_op(my_parts.begin(), my_parts.end(),
                       [](const KnownPlayer &part) { return part.mass; });
}

void Context::update_speed_data() {
  std::vector<std::pair<Point, double>> speed;
  for (auto &p : my_parts)
    speed.emplace_back(p.speed, p.mass);
  avg_speed = weighted_center(speed);
  speed_angle = avg_speed.angle();
}

void Context::update_food_map() {
  food_map.clear();
  for (auto &f : food)
    food_map.insert({f.pos, &f});
}

void Context::update_enemy_speed() {
  for (auto &e : enemies) {
    auto it = prev_pos.find(e.id);
    enemies.back().speed =
        it != prev_pos.end()
            ? e.pos - it->second
            : (my_center - e.pos).normalized() * max_speed(e.mass, config);
    prev_pos[e.id] = e.pos;
  }
}

void Context::update_caches() {
  update_my_center();
  update_my_radius();
  update_total_mass();
  update_largest_part();
  update_speed_data();
  update_food_map();
  update_enemy_speed();
}

void Context::update_my_center() {
  std::vector<std::pair<Point, double>> centers;
  for (auto &p : my_parts)
    centers.emplace_back(p.pos, p.mass);
  my_center = weighted_center(centers);
}
