#include "Response.h"

json Response::to_json() const {
  json out;
  out["X"] = m_pos.x;
  out["Y"] = m_pos.y;
  out["DEBUG"] = m_debug;
  if (m_split)
    out["Split"] = true;
  if (m_eject)
    out["Eject"] = true;
  return out;
}
