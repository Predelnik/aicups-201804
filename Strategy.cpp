#include "Strategy.h"

#include <iostream>

#include "GameConfig.h"

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

json Strategy::on_tick(const json &data) {
  auto mine = data["Mine"];
  auto objects = data["Objects"];
  if (!mine.empty()) {
    auto first = mine[0];
    auto food = find_food(objects);
    if (!food.empty()) {
      return {{"X", food["X"]}, {"Y", food["Y"]}};
    }
    return {{"X", 0}, {"Y", 0}, {"Debug", "No food"}};
  }
  return {{"X", 0}, {"Y", 0}, {"Debug", "Died"}};
}

template <class T> json Strategy::find_food(const T &objects) {
  for (auto &obj : objects) {
    if (obj["T"] == "F") {
      return obj;
    }
  }
  return json({});
}