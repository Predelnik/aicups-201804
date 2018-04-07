#pragma once

class Player;
class GameConfig;
class MovingPoint;
class Point;

double max_speed_circle_radius (const Player &player, const GameConfig &config);
MovingPoint next_moving_point(MovingPoint moving_point, double mass,
                              const Point &acceleration, int ticks,
                              const GameConfig &config);
double max_speed (double mass, const GameConfig &config);
