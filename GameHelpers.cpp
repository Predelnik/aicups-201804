#include "GameHelpers.h"
#include "Const.h"
#include "GameConfig.h"
#include "Matrix.h"
#include "MovingPoint.h"
#include "MyPart.h"

double max_speed_circle_radius(const Player &player, const GameConfig &config) {
  MovingPoint mp{{0, 0}, {player.max_speed(config), 0}};
  auto mp1 = mp;
  auto mp2 = next_moving_point(mp, player.mass,
                               mp.speed * Matrix::rotation(constant::pi / 2.0),
                               1, config);
  auto alpha = -mp2.speed.angle();
  return (mp2.position.x) * std::tan(constant::pi / 2. - alpha) -
         mp2.position.y;
}

MovingPoint next_moving_point(MovingPoint moving_point, double mass,
                              const Point &acceleration, int ticks,
                              const GameConfig &config) {
  auto ms = max_speed(mass, config);
  for (int i = 0; i < ticks; ++i) {
    moving_point.speed +=
        (acceleration.normalized() * ms - moving_point.speed) *
        config.inertia_factor / mass;
    if (moving_point.speed.squared_length() > pow(ms, 2))
      moving_point.speed = moving_point.speed.normalized() * ms;
    moving_point.position += moving_point.speed;
  }
  return moving_point;
}

double max_speed(double mass, const GameConfig &config) {
  return config.speed_factor / sqrt(mass);
}

double x_distance_to_wall(const MovingPoint &mp, double radius,
                          const GameConfig &config) {
  double dist = constant::infinity;
  if (mp.speed.x > 0)
    dist = std::min(dist, (config.game_width - radius - mp.position.x) /
                              cos(mp.speed.angle()));
  if (mp.speed.x < 0)
    dist = std::min(dist, (mp.position.x - radius) / -cos(mp.speed.angle()));
  return dist;
}

double y_distance_to_wall(const MovingPoint &mp, double radius,
                          const GameConfig &config) {
  double dist = constant::infinity;
  if (mp.speed.y > 0)
    dist = std::min(dist, (config.game_height - radius - mp.position.y) /
                              sin(mp.speed.angle()));
  if (mp.speed.y < 0)
    dist = std::min(dist, (mp.position.y - radius) / -sin(mp.speed.angle()));
  return dist;
}
