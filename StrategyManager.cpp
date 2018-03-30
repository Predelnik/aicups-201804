#include "StrategyManager.h"

#include <iostream>

#include "MyPart.h"
#include "Response.h"

#include "overload.h"

StrategyManager::StrategyManager() = default;
StrategyManager::~StrategyManager() = default;

void StrategyManager::run() {
  std::string data;
  std::cin >> data;
  m_context.update_config (json::parse (data));
  while (true) {
    std::cin >> data;
    auto command = on_tick(json::parse(data));
    std::cout << command.dump() << std::endl;
  }
}

json StrategyManager::on_tick(const json &data) {
  m_context.update (data);
  return m_strategy.get_response(m_context).to_json();
}

