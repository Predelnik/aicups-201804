#include "Context.h"
#include "KnownPlayer.h"
#include "Object.h"
#include "overload.h"

#include "Defines.h"
#include "GameHelpers.h"
#include "algorithm.h"
#include <set>
#include <variant>

namespace {
std::vector<KnownPlayer> to_my_parts(const json &data) {
  std::vector<KnownPlayer> out(data.size());
  std::transform(data.begin(), data.end(), out.begin(),
                 [](const json &data) { return KnownPlayer(data); });
  return out;
}

} // namespace
Context::Context() : m_re(DEBUG_RELEASE_VALUE(0, std::random_device()())) {}
Context::~Context() = default;

void Context::initialize(const json &data) { config = GameConfig{data}; }

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

void Context::remove_enemies_older_than(int n_ticks) const {
  for (auto it = enemy_seen_by_tick.begin(); it != enemy_seen_by_tick.end();) {
    if (it->first < tick - n_ticks) {
      enemy_by_id.erase(it->second);
      it = enemy_seen_by_tick.erase(it);
    } else
      break;
  }
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
    auto it = prev_tick_enemy_by_id.find(e.id);
    e.speed =
        it != prev_tick_enemy_by_id.end()
            ? e.pos - it->second.pos
            : (my_center - e.pos).normalized() * max_speed(e.mass, config);
    if (it != prev_tick_enemy_by_id.end()) {
      if (e.mass > it->second.mass * 1.75)
        e.ticks_to_fuse = config.ticks_til_fusion;
      else {
        e.ticks_to_fuse = it->second.ticks_to_fuse;
        --e.ticks_to_fuse;
      }
    } else
      e.ticks_to_fuse =
          std::uniform_int_distribution<int>(0, config.ticks_til_fusion / 3)(m_re);
  }
}

void Context::clean_up_eaten_or_fused_enemies() {
  std::set<PartId> current_ids;
  for (auto &e : enemies)
    current_ids.insert(e.id);

  for (auto &p : prev_tick_enemy_by_id) {
    bool should_be_visible = false;
    for (auto &part : my_parts) {
      if (current_ids.count(p.first) == 0 &&
          p.second.pos.is_in_circle(part.visibility_center(),
                                    part.visibility_radius(static_cast<int> (my_parts.size())) *
                                        0.9)) {
        should_be_visible = true;
        break;
      }
    }
    if (should_be_visible) {
      // supposedly was eaten or fused
      auto it = enemy_by_id.find(p.first);
      if (it != enemy_by_id.end()) {
        auto rng = enemy_seen_by_tick.equal_range(it->second.tick);
        for (auto jt = rng.first; jt != rng.second;)
          if (jt->second == p.second.id)
            jt = enemy_seen_by_tick.erase(jt);
          else
            ++jt;
        enemy_by_id.erase(it);
      }
    }
  }
}

void Context::update_enemies_by_id() {
  clean_up_eaten_or_fused_enemies();

  prev_tick_enemy_by_id.clear();

  for (auto &e : enemies) {
    prev_tick_enemy_by_id[e.id] = e;
    auto it = enemy_by_id.find(e.id);
    if (it != enemy_by_id.end()) {
      auto p = enemy_seen_by_tick.equal_range(it->second.tick);
      for (auto jt = p.first; jt != p.second;)
        if (jt->second == e.id)
          jt = enemy_seen_by_tick.erase(jt);
        else
          ++jt;
    }
    enemy_by_id[e.id] = {e, tick};
    enemy_seen_by_tick.insert({tick, e.id});
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
  update_enemies_by_id();
}

void Context::update_my_center() {
  std::vector<std::pair<Point, double>> centers;
  for (auto &p : my_parts)
    centers.emplace_back(p.pos, p.mass);
  my_center = weighted_center(centers);
}
