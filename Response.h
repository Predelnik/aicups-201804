#pragma once

#include <string>
#include "Point.h"

#include "../nlohmann/json.hpp"
using nlohmann::json;

class Response
{
    using Self = Response;
public:
    Self &pos (Point pos){ m_pos = pos; return *this; }
    Self &debug (std::string debug){ m_debug = std::move (debug); return *this; }
    Self &split (bool val = true) { m_split = val; return *this; }
    Self &eject (bool val = true) { m_eject = val; return *this; }

    json to_json () const;

private:
    Point m_pos;
    bool m_split = false;
    bool m_eject = false;
    std::string m_debug;
};

