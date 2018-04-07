#include "MaxSpeedStrategy.h"
#include "Const.h"
#include "Context.h"
#include "GameHelpers.h"
#include "Matrix.h"
#include "MovingPoint.h"
#include "MyPart.h"
#include "Response.h"
#include <set>

MaxSpeedStrategy::MaxSpeedStrategy() : m_re(std::random_device()()) {}

Response MaxSpeedStrategy::speed_case() {
  double best_angle = 0.0;
  int best_angle_score = -std::numeric_limits<int>::min();
  for (int angle_try = 0; angle_try <= angle_discretization; ++angle_try) {
    double angle =
        -constant::pi / 2.0 + constant::pi * angle_try / angle_discretization;
    auto accel = (ctx->avg_speed * Matrix::rotation(angle)).normalized();
    int score = 0;
    static std::vector<MovingPoint> mps;
    mps.resize(ctx->my_parts.size());
    std::transform(ctx->my_parts.begin(), ctx->my_parts.end(), mps.begin(),
                   [](const MyPart &p) { return p.as_moving_point(); });
    static std::vector<MovingPoint> next_mps;
    next_mps.resize(ctx->my_parts.size());
    for (int j = 0; j < ctx->my_parts.size(); ++j) {
      next_mps[j] = next_moving_point(mps[j], ctx->my_parts[j].mass, accel, 1,
                                      ctx->config);
      auto r = max_speed_circle_radius(ctx->my_parts[j], ctx->config);
      auto x_to_wall =
          x_distance_to_wall(next_mps[j], ctx->my_parts[j].radius, ctx->config);
      if (x_to_wall < r)
        score -= static_cast<int>(10000.0 * ((r - x_to_wall) / r));
      auto y_to_wall =
          y_distance_to_wall(next_mps[j], ctx->my_parts[j].radius, ctx->config);
      if (y_to_wall < r)
        score -= static_cast<int>(10000.0 * ((r - y_to_wall) / r));
    }
    for (int iteration = 0; iteration < 10; ++iteration) {
      std::set<int> food_taken;

      for (int food_index = 0; food_index < ctx->food.size(); ++food_index) {
        if (food_taken.count(food_index))
          continue;
        for (int part_index = 0; part_index < ctx->my_parts.size();
             ++part_index) {
          if (next_mps[part_index].position.squared_distance_to(
                  ctx->food[food_index].pos) < ctx->my_parts[part_index].radius)
              {
                score += 100 * ctx->config.food_mass;
                food_taken.insert(food_index);
              }
        }
      }
      for (int part_index = 0; part_index < ctx->my_parts.size();
           ++part_index) {
        next_mps[part_index] = next_moving_point(next_mps[part_index],
                                                 ctx->my_parts[part_index].mass,
                                                 accel, 1, ctx->config);
      }
    }
    score += std::uniform_int_distribution<int>(0, 29)(m_re);

    if (score > best_angle_score) {
      best_angle_score = score;
      best_angle = angle;
    }
  }
  return Response{}.target(
      ctx->my_center +
      (ctx->avg_speed * Matrix::rotation(best_angle)).normalized() * 50.0);
}

Response MaxSpeedStrategy::no_speed_case() {
  auto angle =
      std::uniform_real_distribution<double>(0, 2 * constant::pi)(m_re);
  return Response{}.target(ctx->my_center +
                           Point{50.0, 0.} * Matrix::rotation(angle));
}

Response MaxSpeedStrategy::get_response(const Context &context) {
  ctx = &context;

  if (ctx->avg_speed.squared_length() < 0.001)
    return no_speed_case();

  return speed_case();
  return {};
}

void MaxSpeedStrategy::initialize(const GameConfig &config) {}
