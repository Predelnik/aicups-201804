#include "GameHelpers.h"
#include "Const.h"
#include "GameConfig.h"
#include "Matrix.h"
#include "MovingPoint.h"
#include "MyPart.h"

double max_speed_circle_radius(const MyPart &part, const GameConfig &config) {
  MovingPoint mp{{0, 0}, {part.max_speed(config), 0}};
  auto mp1 = mp;
  auto mp2 = next_moving_point(mp, part.mass,
                               mp.speed * Matrix::rotation(-constant::pi / 2.0),
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

double distance_to_nearest_wall(const Point &p, const GameConfig &config) {
  return std::min({std::max(p.x, 0.0), std::max(p.y, 0.0),
                   std::max(config.game_width - p.x, 0.0),
                   std::max(config.game_height - p.y, 0.0)});
}

double distance_to_nearest_wall(const Point &p, double radius,
                                const GameConfig &config) {
  return std::min({std::max(p.x - radius, 0.0), std::max(p.y - radius, 0.0),
                   std::max(config.game_width - p.x - radius, 0.0),
                   std::max(config.game_height - p.y - radius, 0.0)});
}

bool can_eat(double eater_mass, double eatee_mass) {
  return eater_mass >= eatee_mass * constant::eating_mass_coeff;
}

bool can_eat(double eater_mass, const Point &eater_pos, double eater_radius,
             double eatee_mass, const Point &eatee_pos, double eatee_radius) {
  return can_eat(eater_mass, eatee_mass) &&
         eatee_pos.is_in_circle(eater_pos,
                                eating_distance(eater_radius, eatee_radius));
}

double eating_distance(double eater_radius, double eatee_radius) {
  return eatee_radius + eater_radius -
         (2 * eatee_radius) * constant::interaction_dist_coeff;
}

bool is_virus_dangerous_for(const GameConfig &config, const Point &virus_pos,
                            const Point &pos, double radius, double mass) {
  if (mass < constant::virus_danger_mass)
    return false;

  if (config.virus_radius > radius)
    return false;

  auto dangerous_dist =
      config.virus_radius * constant::virus_hurt_factor + radius;
  return pos.is_in_circle(virus_pos, dangerous_dist);
}

bool is_object_reachable(const GameConfig &config, double my_radius,
                         const Point &object_pos) {
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j) {
      Point corner(i * config.game_width, j * config.game_height);
      if (corner.squared_distance_to(object_pos) > std::pow(my_radius, 2))
        return true;
      corner.x = fabs(corner.x - my_radius);
      corner.y = fabs(corner.y - my_radius);
      if (corner.squared_distance_to(object_pos) > std::pow(my_radius, 2))
        return false;
    }

  return true;
}
