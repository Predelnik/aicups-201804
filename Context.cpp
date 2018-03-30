#include "Context.h"
#include "MyPart.h"
#include "Object.h"
#include "overload.h"

#include <variant>

namespace {
std::vector<MyPart> to_my_parts(const json &data) {
  std::vector<MyPart> out(data.size());
  std::transform(data.begin(), data.end(), out.begin(),
                 [](const json &data) { return MyPart(data); });
  return out;
}

} // namespace
Context::Context() = default;
Context::~Context() = default;

void Context::update_config(const json &data) { config = GameConfig{data}; }

void Context::update(const json &data) {
  ++tick;
  my_parts = to_my_parts(data["Mine"]);
  fill_objects(data["Objects"]);
  update_caches();
}

void Context::fill_objects(const json &data) {
  food.clear();
  viruses.clear();
  players.clear();
  ejections.clear();
  for (auto &element : data) {
    std::visit(
        overload(
            [this](Food &&food_piece) { food.push_back(food_piece); },
            [this](Virus &&virus) { viruses.push_back(virus); },
            [this](Player &&player) { players.push_back(std::move(player)); },
            [this](Ejection &&ejection) { ejections.push_back(ejection); },
            [](std::monostate) {}),
        to_object(element));
  }
}

void Context::update_my_radius() {
  my_radius = 0.0;
  for (auto &p : my_parts)
    my_radius = std::max(my_radius, p.center.distance_to(my_center) + p.radius);
}

void Context::update_total_mass() {
  my_total_mass = 0.0;
  for (auto &p : my_parts)
    my_total_mass += p.mass;
}

void Context::update_caches() {
  update_my_center();
  update_my_radius();
  update_total_mass();
}

void Context::update_my_center() {
  std::vector<std::pair<Point, double>> centers;
  for (auto &p : my_parts)
    centers.emplace_back(p.center, p.mass);
  my_center = weighted_center(centers);
}
