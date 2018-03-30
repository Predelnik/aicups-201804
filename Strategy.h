#pragma once

#include "Context.h"
#include <random>

class Context;
class Response;

class Strategy {
public:
  Strategy();
  const Player *find_dangerous_enemy();
  Response run_away_from(const Point &pos) const;
    Response move_randomly() const;
    Response get_response(const Context &context);

private:
  const Food *find_nearest_food();
  Point future_center(double time);
  Response continue_movement();

private:
  mutable std::default_random_engine m_re;
  const Context *ctx = nullptr;

  static inline int randomize_frequency = 50;
  static inline double standing_speed = 1.0;
};
