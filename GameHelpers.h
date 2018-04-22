#pragma once

class Player;
class GameConfig;
class MovingPoint;
class KnownPlayer;
class Point;

double max_speed_circle_radius(const KnownPlayer &part,
                               const GameConfig &config);
void advance(KnownPlayer &known_player, const Point &acceleration, int ticks,
             const GameConfig &config);
double max_speed(double mass, const GameConfig &config);
double x_distance_to_wall(const MovingPoint &mp, double radius,
                          const GameConfig &config);
double y_distance_to_wall(const MovingPoint &mp, double radius,
                          const GameConfig &config);
double distance_to_nearest_wall(const Point &p, const GameConfig &config);
double distance_to_nearest_wall(const Point &p, double radius,
                                const GameConfig &config);

bool can_eat(double eater_mass, double eatee_mass);
bool can_eat(double eater_mass, const Point &eater_pos, double eater_radius,
             double eatee_mass, const Point &eatee_pos, double eatee_radius);

double eating_distance(double eater_radius, double eatee_radius);

bool is_virus_dangerous_for(const GameConfig &config, const Point &virus_pos,
                            const Point &pos, double radius, double mass);

bool is_object_reachable(const GameConfig &config, double my_radius,
                         const Point &object_pos);

double radius_by_mass (double mass);

double distance_to_nearest_wall_by_x(const Point &p, double radius, const GameConfig &config);
double distance_to_nearest_wall_by_y(const Point &p, double radius, const GameConfig &config);