#pragma once
#include "../nlohmann/json.hpp"
#include "GameConfig.h"
using nlohmann::json;

class Response;

class Strategy {
public:
    void run ();
    json on_tick(const json& data);

private:
    Response on_tick_impl(const json & data);
    template <class T>
    json find_food(const T& objects);
private:
    GameConfig m_cfg;
};
