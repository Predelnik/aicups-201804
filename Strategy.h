#pragma once
#include "../nlohmann/json.hpp"
#include "GameConfig.h"
using nlohmann::json;

class Strategy {
public:
    void run ();
    json on_tick(const json& data);

private:
    template <class T>
    json find_food(const T& objects);
private:
    GameConfig m_cfg;
};
