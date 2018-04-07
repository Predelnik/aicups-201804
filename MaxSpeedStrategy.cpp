#include "MaxSpeedStrategy.h"
#include "Const.h"
#include "Context.h"
#include "GameHelpers.h"
#include "Matrix.h"
#include "MyPart.h"
#include "Response.h"

MaxSpeedStrategy::MaxSpeedStrategy() : m_re(std::random_device()()) {}

Response MaxSpeedStrategy::speed_case() {
  auto angle = std::uniform_real_distribution<double>(-constant::pi / 2.0,
                                                      constant::pi / 2.0)(m_re);
  return Response{}.target(
      ctx->my_center +
      (ctx->avg_speed * Matrix::rotation(angle)).normalized() * 50.0);
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
