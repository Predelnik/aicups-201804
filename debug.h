#pragma once

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

inline bool is_debugger_present ()
{
#ifdef _WIN32
    return IsDebuggerPresent ();
#endif
    return false;
}
