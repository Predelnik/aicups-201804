#include "Response.h"

Response::Self &Response::debug_lines(std::vector<std::array<Point, 2>> lines) {
  m_debug_lines = std::move(lines);
  return *this;
}

json Response::to_json() const {
  json out;
  out["X"] = m_pos.x;
  out["Y"] = m_pos.y;
  out["DEBUG"] = m_debug;
  if (m_split)
    out["Split"] = true;
  if (m_eject)
    out["Eject"] = true;
#if CUSTOM_DEBUG
  {
    std::vector<std::map<std::string, double>> lines;
    for (auto line : m_debug_lines)
      lines.push_back({{"X1", line[0].x},
                       {"Y1", line[0].y},
                       {"X2", line[1].x},
                       {"Y2", line[1].y}});
    out["DebugLines"] = lines;
  }
#endif // CUSTOM_DEBUG
  return out;
}
