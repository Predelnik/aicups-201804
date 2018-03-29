#pragma once

#include <string>

#include "../nlohmann/json.hpp"
using nlohmann::json;

class Response
{
    using Self = Response;
public:
    Self &x (double x){ m_x = x; return *this; }
    Self &y (double y){ m_y = y; return *this; }
    Self &debug (std::string debug){ m_debug = std::move (debug); return *this; }

    json to_json () const;

private:
    double m_x = 0;
    double m_y = 0;
    std::string m_debug;
};

