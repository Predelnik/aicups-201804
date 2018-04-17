#include "StrategyManager.h"

#include <iostream>

#include "Response.h"

#include "MaxSpeedStrategy.h"
#include "range.hpp"
#include <fstream>
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

void StrategyManager::run_feed(const std::string &path) {
  std::ifstream ifs(path.c_str());
  if (!ifs)
    return;
  std::string line;
  std::getline(ifs, line);
  m_context.initialize(json::parse(line));
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
    std::getline(ifs, line); // empty line
  }
}
