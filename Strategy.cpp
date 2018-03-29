#include "Strategy.h"

#include <iostream>

#include "GameConfig.h"
#include "Response.h"
#include "MyPart.h"

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

namespace
{
    std::vector<MyPart> to_my_parts (const json &data)
    {
        std::vector<MyPart> out (data.size());
        std::transform(data.begin (), data.end (), out.begin (), [](const json &data){ return MyPart (data); });
        return out;
    }
}

json Strategy::on_tick(const json &data) {
  m_my_parts = to_my_parts(data["Mine"]);

  return on_tick_impl(data).to_json();
}

Response Strategy::on_tick_impl(const json &data) {
  auto objects = data["Objects"];
  if (!m_my_parts.empty()) {
    auto &first = m_my_parts.front ();
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
