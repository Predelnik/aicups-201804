#include "StrategyManager.h"

#include <iostream>

#include "Response.h"

#include "MaxSpeedStrategy.h"
#include "range.hpp"
#include <fstream>
#include "debug.h"
using namespace util::lang;

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
  std::istream::sync_with_stdio(false);
  m_context.initialize(json::parse(data));
  m_strategy->initialize(m_context.config);
  while (true) {
    std::cin >> data;
    auto command = on_tick(json::parse(data));
    std::cout << command.dump() << std::endl;
  }
}

json StrategyManager::on_tick(const json &data) {
  m_context.update(data);
  if (m_context.my_parts.empty())
    return {};
  return m_strategy->get_response(m_context).to_json();
}

void StrategyManager::run_feed(const std::string &path, int tick) {
  std::ifstream ifs(path.c_str());
  if (!ifs)
    return;
  std::string line;
  std::getline(ifs, line);
  m_context.initialize(json::parse(line));
  if (tick > 0)
  {
      m_context.debug_tick_start = tick;
      m_context.debug_tick_end = tick + 10;
  }
  m_strategy->initialize(m_context.config);
  std::getline(ifs, line); // empty line
  while (ifs) {
    std::getline(ifs, line);
    std::cout << "Parsed " << line << '\n';
    std::getline(ifs, line);
    m_context.update(json::parse(line));
    if (!m_context.my_parts.empty()) {
      std::cout << m_strategy->get_response(m_context).to_json() << '\n';
      std::getline(ifs, line); // answer
    }
#ifdef _MSC_VER
  if (is_debugger_present() && m_context.is_debug_tick())
    __debugbreak();
#endif
    std::getline(ifs, line); // empty line
  }
}
