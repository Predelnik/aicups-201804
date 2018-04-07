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
  return (mp2.pos.x) * std::tan(constant::pi / 2. - alpha) - mp2.pos.y;
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
    moving_point.pos += moving_point.speed;
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
    dist = std::min(dist, (config.game_width - radius - mp.pos.x) /
                              cos(mp.speed.angle()));
  if (mp.speed.x < 0)
    dist = std::min(dist, (mp.pos.x - radius) / -cos(mp.speed.angle()));
  return dist;
}

double y_distance_to_wall(const MovingPoint &mp, double radius,
                          const GameConfig &config) {
  double dist = constant::infinity;
  if (mp.speed.y > 0)
    dist = std::min(dist, (config.game_height - radius - mp.pos.y) /
                              sin(mp.speed.angle()));
  if (mp.speed.y < 0)
    dist = std::min(dist, (mp.pos.y - radius) / -sin(mp.speed.angle()));
  return dist;
}

bool can_eat(double eater_mass, double eatee_mass) {
  return eater_mass >= eatee_mass * constant::eating_mass_coeff;
}

bool can_eat(double eater_mass, const Point &eater_pos, double eater_radius,
             double eatee_mass, const Point &eatee_pos, double eatee_radius) {
  return can_eat(eater_mass, eatee_mass) &&
         eater_pos.distance_to(eatee_pos) - eatee_radius +
                 (2 * eatee_radius) * constant::interaction_dist_coeff <
             eater_radius;
}
