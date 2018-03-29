#include "Strategy.h"

#include <iostream>

#include "GameConfig.h"
#include "MyPart.h"
#include "Response.h"

#include "algorithm.h"
#include "overload.h"

Strategy::Strategy() : m_re(std::random_device()()) {}
Strategy::~Strategy() = default;

void Strategy::run() {
  std::string data;
  std::cin >> data;
  m_config = {json::parse(data)};
  while (true) {
    std::cin >> data;
    auto parsed = json::parse(data);
    auto command = on_tick(parsed);
    std::cout << command.dump() << std::endl;
  }
}

namespace {
std::vector<MyPart> to_my_parts(const json &data) {
  std::vector<MyPart> out(data.size());
  std::transform(data.begin(), data.end(), out.begin(),
                 [](const json &data) { return MyPart(data); });
  return out;
}

} // namespace

json Strategy::on_tick(const json &data) {
  m_my_parts = to_my_parts(data["Mine"]);
  fill_objects(data["Objects"]);
  update_caches();
  return generate_response().to_json();
}

void Strategy::fill_objects(const json &data) {
  m_food.clear();
  m_viruses.clear();
  m_players.clear();
  m_ejections.clear();
  for (auto &element : data) {
    std::visit(
        overload(
            [this](Food &&food) { m_food.push_back(food); },
            [this](Virus &&virus) { m_viruses.push_back(virus); },
            [this](Player &&player) { m_players.push_back(std::move(player)); },
            [this](Ejection &&ejection) { m_ejections.push_back(ejection); },
            [](std::monostate) {}),
        to_object(element));
  }
}

Response Strategy::generate_response() {
  if (!m_my_parts.empty()) {
    auto &first = m_my_parts.front();
    auto food = find_nearest_food();
    if (food) {
      return Response{}.pos(food->pos);
    }

    // go to random point, act really nervous
    auto x =
        std::uniform_real_distribution<double>(0.0, m_config.game_width)(m_re);
    auto y =
        std::uniform_real_distribution<double>(0.0, m_config.game_height)(m_re);
    return Response{}.pos({x, y});
  }
  return Response{}.pos({}).debug("Died");
}

const Food *Strategy::find_nearest_food() {
  if (m_food.empty())
    return nullptr;
  auto it = min_element_op(m_food.begin(), m_food.end(), [&](const Food &food) {
    return food.pos.squared_distance_to(m_my_center);
  });
  return &*it;
}

void Strategy::update_my_center() {
  std::vector<std::pair<Point, double>> centers;
  for (auto &p : m_my_parts)
    centers.emplace_back(p.center, p.mass);
  m_my_center = weighted_center(centers);
}

void Strategy::update_caches() { update_my_center(); }
