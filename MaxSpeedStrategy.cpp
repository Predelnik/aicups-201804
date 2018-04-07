#include "MaxSpeedStrategy.h"
#include "Const.h"
#include "Context.h"
#include "GameHelpers.h"
#include "Matrix.h"
#include "MovingPoint.h"
#include "MyPart.h"
#include "Response.h"

MaxSpeedStrategy::MaxSpeedStrategy() : m_re(std::random_device()()) {}

Response MaxSpeedStrategy::speed_case() {
  double best_angle = 0.0;
  int best_angle_score = -std::numeric_limits<int>::min();
  for (int i = 0; i <= angle_discretization; ++i) {
    double angle =
        -constant::pi / 2.0 + constant::pi * i / angle_discretization;
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
      auto x_to_wall = x_distance_to_wall(next_mps[j], ctx->my_parts[j].radius, ctx->config);
      if (x_to_wall < r)
        score -= static_cast<int> (10000.0 * ((r - x_to_wall) / r));
      auto y_to_wall = y_distance_to_wall(next_mps[j], ctx->my_parts[j].radius, ctx->config);
      if (y_to_wall < r)
        score -= static_cast<int> (10000.0 * ((r - y_to_wall) / r));
    }
    score += std::uniform_int_distribution<int>(0, 255)(m_re);

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
