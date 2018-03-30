#include "Strategy.h"
#include "Const.h"
#include "MyPart.h"
#include "Object.h"
#include "Response.h"
#include "algorithm.h"

Strategy::Strategy() : m_re(std::random_device()()) {}

const Player *Strategy::find_dangerous_enemy() {
  for (auto &p : ctx->players) {
    if (p.mass >= ctx->my_total_mass * constant::eating_mass_coeff)
      return &p;
  }
  return nullptr;
}

Response Strategy::run_away_from(const Point &pos) const {
  auto v = -(pos - ctx->my_center);
  if (v.length() < standing_speed)
    return move_randomly();

  return Response{}.pos(ctx->my_center + v);
}

Response Strategy::move_randomly() const {
  auto x =
      std::uniform_real_distribution<double>(0.0, ctx->config.game_width)(m_re);
  auto y = std::uniform_real_distribution<double>(0.0, ctx->config.game_height)(
      m_re);
  return Response{}.pos({x, y});
}

const Food *Strategy::find_nearest_food() {
  if (ctx->food.empty())
    return nullptr;
  auto dist = [&](const Food &food) {
    if (ctx->my_total_mass > constant::virus_danger_mass) {
      auto danger_dist =
          (ctx->config.virus_radius + ctx->my_radius) * 1.05 /*imprecision*/;
      for (auto &v : ctx->viruses) {
        if (v.pos.distance_to_line(ctx->my_center, food.pos) < danger_dist)
          return constant::infinity;
      }
    }

    return food.pos.squared_distance_to(ctx->my_center);
  };
  auto it = min_element_op(ctx->food.begin(), ctx->food.end(), dist);
  if (dist(*it) < constant::infinity)
    return &*it;

  return nullptr;
}

Point Strategy::future_center(double time) {
  return ctx->my_center + ctx->my_parts.front().speed * time;
}

Response Strategy::continue_movement() {
  if (ctx->my_parts.empty())
    return Response{}.debug("Too dead to move");

  return Response{}.pos(future_center(10.0));
}

Response Strategy::get_response(const Context &context) {
  ctx = &context;
  if (!ctx->my_parts.empty()) {
    if (auto enemy = find_dangerous_enemy()) {
      return run_away_from(enemy->pos);
    }

    if (auto food = find_nearest_food()) {
      return Response{}.pos(food->pos);
    }

    {
      static int last_tick = std::numeric_limits<int>::min();
      if (ctx->my_parts.front().speed.length() < standing_speed ||
          ctx->tick > last_tick + randomize_frequency) {

        last_tick = ctx->tick;
        return move_randomly();
      }

      return continue_movement();
    }
  }
  return Response{}.pos({}).debug("Died");
}
