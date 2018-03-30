#include "Strategy.h"
#include "Object.h"
#include "Response.h"
#include "algorithm.h"
#include "MyPart.h"

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

Response Strategy::continue_movement ()
{
    if (ctx->my_parts.empty())
        return Response{}.debug("Too dead to move");

    return Response{}.pos (ctx->my_center + ctx->my_parts.front ().speed * 10.0);
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
      static int last_tick = std::numeric_limits<int>::min ();
      if (ctx->tick > last_tick + randomize_frequency) {
        auto x = std::uniform_real_distribution<double>(
            0.0, ctx->config.game_width)(m_re);
        auto y = std::uniform_real_distribution<double>(
            0.0, ctx->config.game_height)(m_re);
        last_tick = ctx->tick;
        return Response{}.pos({x, y});
      } else
        return continue_movement ();
    }
  }
  return Response{}.pos({}).debug("Died");
}
