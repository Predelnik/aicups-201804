#pragma once

#include "Strategy.h"

#include <random>
#include "Point.h"

class Context;

class MaxSpeedStrategy : public Strategy {
public:
  MaxSpeedStrategy();
    Response move_by_vector(const Point& v);
    Response speed_case();
  Response no_speed_case();
  Response get_response(const Context &context) override;
  void initialize(const GameConfig &config) override;

private:
  constexpr static int angle_discretization = 20;
  const Context *ctx;
  mutable std::default_random_engine m_re;
};