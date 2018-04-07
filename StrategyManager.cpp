#include "StrategyManager.h"

#include <iostream>

#include "MyPart.h"
#include "Response.h"

#include "MaxSpeedStrategy.h"
#include "overload.h"

#ifdef _MSC_VER
#include <windows.h>
#endif

StrategyManager::StrategyManager() {
  m_strategy = std::make_unique<MaxSpeedStrategy>();
}
StrategyManager::~StrategyManager() = default;

void StrategyManager::run() {
#if defined _MSC_VER && defined _DEBUG
  while (!IsDebuggerPresent()) {
    Sleep(500);
  }
#endif
  std::string data;
  std::cin >> data;
  std::cin.sync_with_stdio(false);
  m_context.update_config(json::parse(data));
  m_strategy->initialize(m_context.config);
  while (true) {
    std::cin >> data;
    auto command = on_tick(json::parse(data));
    std::cout << command.dump() << std::endl;
  }
}

json StrategyManager::on_tick(const json &data) {
  m_context.update(data);
  return m_strategy->get_response(m_context).to_json();
}
