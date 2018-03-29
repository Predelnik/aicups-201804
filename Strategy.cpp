#include "Strategy.h"

#include <iostream>

#include "GameConfig.h"
#include "Response.h"

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
  return on_tick_impl(data).to_json();
}

Response Strategy::on_tick_impl(const json &data) {
  auto mine = data["Mine"];
  auto objects = data["Objects"];
  if (!mine.empty()) {
    auto first = mine[0];
    auto food = find_food(objects);
    if (!food.empty()) {
      return Response{}.x(food["X"]).y(food["Y"]);
    }
    return Response{}.x(0).y(0).debug("No food");
  }
  return Response{}.x(0).y(0).debug("Died");
}

template <class T> json Strategy::find_food(const T &objects) {
  for (auto &obj : objects) {
    if (obj["T"] == "F") {
      return obj;
    }
  }
  return json({});
}
