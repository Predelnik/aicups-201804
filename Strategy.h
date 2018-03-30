﻿#pragma once

#include <random>
#include "Context.h"

class Context;
class Response;

class Strategy
{
public:
  Strategy ();
  Response get_response (const Context &context);

private:
  const Food* find_nearest_food();

private:
  std::default_random_engine m_re;
  const Context *ctx = nullptr;
};
