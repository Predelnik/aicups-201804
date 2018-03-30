#include "Strategy.h"
#include "Object.h"
#include "Response.h"
#include "algorithm.h"

Strategy::Strategy() : m_re(std::random_device()()) {}

const Food *Strategy::find_nearest_food() {
  if (ctx->food.empty())
    return nullptr;
  auto it =
      min_element_op(ctx->food.begin(), ctx->food.end(), [&](const Food &food) {
        return food.pos.squared_distance_to(ctx->my_center);
      });
  return &*it;
}

Response Strategy::get_response(const Context &context) {
  ctx = &context;
  if (!ctx->my_parts.empty()) {
    auto &first = ctx->my_parts.front();
    auto food = find_nearest_food();
    if (food) {
      return Response{}.pos(food->pos);
    }

    // go to random point, act really nervous
    auto x = std::uniform_real_distribution<double>(
        0.0, ctx->config.game_width)(m_re);
    auto y = std::uniform_real_distribution<double>(
        0.0, ctx->config.game_height)(m_re);
    return Response{}.pos({x, y});
  }
  return Response{}.pos({}).debug("Died");
}
