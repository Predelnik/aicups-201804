#include "Response.h"

json Response::to_json() const
{
    json out;
    out["X"] = m_x;
    out["Y"] = m_y;
    out["DEBUG"] = m_debug;
    return out;
}
