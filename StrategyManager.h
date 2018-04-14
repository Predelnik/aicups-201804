#pragma once

#include "../nlohmann/json.hpp"
#include "Context.h"
#include "CellPrioritizationStrategy.h"

#include <memory>

using nlohmann::json;

class Response;
class KnownPlayer;

class StrategyManager {
public:
    StrategyManager ();
    ~StrategyManager ();
    void run ();
    json on_tick(const json& data);

private:
    void update_caches();
    Context m_context;
    std::unique_ptr<Strategy> m_strategy;
};
