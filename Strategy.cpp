#include "Strategy.h"
#include "Const.h"
#include "MyPart.h"
#include "Object.h"
#include "Response.h"
#include "algorithm.h"

Strategy::Strategy() : m_re(std::random_device()()) {}

const Food *Strategy::find_nearest_food() {
  if (ctx->food.empty())
    return nullptr;
  auto dist = [&](const Food &food) {
    for (auto &v : ctx->viruses)
      if (v.pos.distance_to(food.pos) <
          (ctx->config.virus_radius + ctx->my_radius) * 1.05 /*imprecision*/)
        return constant::infinity;
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
    auto &first = ctx->my_parts.front();
    auto food = find_nearest_food();
    if (food) {
      return Response{}.pos(food->pos);
    }

    {
      static int last_tick = std::numeric_limits<int>::min();
      if (ctx->my_parts.front ().speed.length() < standing_speed ||
          ctx->tick > last_tick + randomize_frequency) {
        auto x = std::uniform_real_distribution<double>(
            0.0, ctx->config.game_width)(m_re);
        auto y = std::uniform_real_distribution<double>(
            0.0, ctx->config.game_height)(m_re);
        last_tick = ctx->tick;
        return Response{}.pos({x, y});
      } else {
        return continue_movement();
      }
    }
  }
  return Response{}.pos({}).debug("Died");
}
