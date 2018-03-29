#pragma once
#include "../nlohmann/json.hpp"
#include "GameConfig.h"
#include "Object.h"

using nlohmann::json;

class Response;
class MyPart;

class Strategy {
public:
    Strategy ();
    ~Strategy ();
    void run ();
    json on_tick(const json& data);

private:
    Response on_tick_impl(const json & data);
    const Food* find_food();
private:
    GameConfig m_cfg;
    std::vector<MyPart> m_my_parts;
    std::vector<Object> m_objects;
};
