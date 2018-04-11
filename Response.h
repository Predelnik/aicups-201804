#pragma once

#include <string>
#include "Point.h"

#include "../nlohmann/json.hpp"
using nlohmann::json;

class Response
{
    using Self = Response;
public:
    Self &target (Point pos){ m_pos = pos; return *this; }
    Self &debug (std::string debug){ m_debug = std::move (debug); return *this; }
    Self &split (bool val = true) { m_split = val; return *this; }
    Self &eject (bool val = true) { m_eject = val; return *this; }
    Self& debug_lines(std::vector<std::array<Point, 2>> lines);
    Self& debug_line_colors(std::vector<std::string> line_colors);

    json to_json () const;

private:
    Point m_pos;
    std::vector<std::array<Point, 2>> m_debug_lines;
    std::vector<std::string> m_debug_line_colors;
    bool m_split = false;
    bool m_eject = false;
    std::string m_debug;
};

