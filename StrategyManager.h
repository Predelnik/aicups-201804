#pragma once

#include "../nlohmann/json.hpp"
#include "Context.h"
#include "Strategy.h"

using nlohmann::json;

class Response;
class MyPart;

class StrategyManager {
public:
    StrategyManager ();
    ~StrategyManager ();
    void run ();
    json on_tick(const json& data);

private:
    void update_caches();
    Context m_context;
    Strategy m_strategy;
};
