﻿#pragma once

#ifdef _DEBUG
#define DEBUG_RELEASE_VALUE(DEBUG_VALUE,RELEASE_VALUE) DEBUG_VALUE
#else
#define DEBUG_RELEASE_VALUE(DEBUG_VALUE,RELEASE_VALUE) RELEASE_VALUE
#endif // _DEBUG
