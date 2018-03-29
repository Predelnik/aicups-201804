#include "Strategy.h"

#include <iostream>

#include "GameConfig.h"
#include "MyPart.h"
#include "Response.h"

Strategy::Strategy() = default;
Strategy::~Strategy() = default;

void Strategy::run() {
  std::string data;
  std::cin >> data;
  m_cfg = {json::parse(data)};
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

std::vector<Object> to_objects(const json &data) {
  std::vector<Object> out(data.size());
  std::transform(data.begin(), data.end(), out.begin(), to_object);
  return out;
}
} // namespace

json Strategy::on_tick(const json &data) {
  m_my_parts = to_my_parts(data["Mine"]);
  m_objects = to_objects(data["Objects"]);

  return on_tick_impl(data).to_json();
}

Response Strategy::on_tick_impl(const json &data) {
  if (!m_my_parts.empty()) {
    auto &first = m_my_parts.front();
    auto food = find_food();
    if (food) {
      return Response{}.pos (food->pos);
    }
    return Response{}.pos ({}).debug("No food");
  }
  return Response{}.pos ({}).debug("Died");
}

const Food *Strategy::find_food() {
  for (auto &obj : m_objects) {
    if (auto ptr = std::get_if<Food>(&obj))
      return ptr;
  }
  return nullptr;
}
