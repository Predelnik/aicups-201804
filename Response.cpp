#include "Response.h"

Response::Self &Response::debug_lines(std::vector<std::array<Point, 2>> lines) {
  m_debug_lines = std::move(lines);
  return *this;
}

Response::Self &
Response::debug_line_colors(std::vector<std::string> line_colors) {
  m_debug_line_colors = std::move(line_colors);
  return *this;
}

json Response::to_json() const {
  json out;
  out["X"] = m_pos.x;
  out["Y"] = m_pos.y;
  out["Debug"] = m_debug;
  if (m_split)
    out["Split"] = true;
  if (m_eject)
    out["Eject"] = true;
#if CUSTOM_DEBUG
  {
    std::vector<json> lines(m_debug_lines.size());
    for (int i = 0; i < m_debug_lines.size(); ++i) {
      json &obj = lines[i];
      obj["X1"] = m_debug_lines[i][0].x;
      obj["Y1"] = m_debug_lines[i][0].y;
      obj["X2"] = m_debug_lines[i][1].x;
      obj["Y2"] = m_debug_lines[i][1].y;
      if (!m_debug_line_colors.empty())
        obj["Color"] = m_debug_line_colors[i];
    }
    out["DebugLines"] = lines;
  }
#endif // CUSTOM_DEBUG
  return out;
}
