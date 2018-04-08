﻿#pragma once
#include "MyPart.h"

class Player;
class GameConfig;
class MovingPoint;
class Point;

double max_speed_circle_radius(const Player &player, const GameConfig &config);
MovingPoint next_moving_point(MovingPoint moving_point, double mass,
                              const Point &acceleration, int ticks,
                              const GameConfig &config);
double max_speed(double mass, const GameConfig &config);
double x_distance_to_wall(const MovingPoint &mp, double radius,
                          const GameConfig &config);
double y_distance_to_wall(const MovingPoint &mp, double radius,
                          const GameConfig &config);

bool can_eat(double eater_mass, double eatee_mass);
bool can_eat(double eater_mass, const Point &eater_pos, double eater_radius,
             double eatee_mass, const Point &eatee_pos, double eatee_radius);