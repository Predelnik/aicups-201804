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
    void fill_objects(const json& data);
    Response generate_response();
    const Food* find_nearest_food();
    void update_my_center();
    void update_caches();
    Point my_center () const;

private:
    GameConfig m_cfg;
    std::vector<MyPart> m_my_parts;
    std::vector<Food> m_food;
    std::vector<Ejection> m_ejections;
    std::vector<Player> m_players;
    std::vector<Virus> m_viruses;

    Point m_my_center;
};
