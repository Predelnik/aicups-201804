#include "Strategy.h"
#include "Const.h"
#include "MyPart.h"
#include "Object.h"
#include "Response.h"
#include "algorithm.h"
#include <set>

Strategy::Strategy() : m_re(std::random_device()()) {}

const Player *Strategy::find_dangerous_enemy() {
  for (auto &p : ctx->players) {
    if (p.mass >= ctx->my_total_mass * constant::eating_mass_coeff)
      return &p;
  }
  return nullptr;
}

Response Strategy::run_away_from(const Point &pos) const {
  auto v = -(pos - ctx->my_center);
  if (v.length() < standing_speed)
    return move_randomly();

  return Response{}.pos(ctx->my_center + v);
}

Response Strategy::move_randomly() const {
  auto x =
      std::uniform_real_distribution<double>(0.0, ctx->config.game_width)(m_re);
  auto y = std::uniform_real_distribution<double>(0.0, ctx->config.game_height)(
      m_re);
  return Response{}.pos({x, y});
}

std::array<int, 2> Strategy::point_cell(const Point &point) const {
  return {static_cast<int>(point.x / cell_size),
          static_cast<int>(point.y / cell_size)};
}

void Strategy::check_visible_squares() {
  std::set<std::array<int, 2>> to_update;
  for (int i = 0; i < expected_food.size(0); ++i)
    for (int j = 0; j < expected_food.size(1); ++j) {
      bool all_visible = true;
      for (int h = 0; h < 2; ++h)
        for (int v = 0; v < 2; ++v) {
          all_visible = all_visible &&
                        is_visible(ctx->my_parts, Point((i + h) * cell_size,
                                                        (j + v) * cell_size));
          if (!all_visible)
            break;
        }
      if (all_visible)
        to_update.insert({i, j});
    }
  for (int i = 0; i < expected_food.size(0); ++i)
    for (int j = 0; j < expected_food.size(1); ++j)
      if (to_update.count({i, j}) == 0)
        expected_food[i][j] += food_expectancy_inc;
      else
        expected_food[i][j] = 0.0;
  for (auto &f : ctx->food) {
    auto cell = point_cell(f.pos);
    if (to_update.count(cell))
      expected_food[cell[0]][cell[1]] += 1.0;
  }
}

void Strategy::update() { check_visible_squares(); }

Point Strategy::cell_center(const std::array<int, 2> &cell) const {
  return {cell[0] * cell_size + cell_size * 0.5,
          cell[1] * cell_size + cell_size * 0.5};
}

Response Strategy::move_to_more_food() {
  auto cell = point_cell(ctx->my_center);
  constexpr int search_resolution = 3;
  auto best_cell = cell;
  for (int i = cell[0] - search_resolution; i <= cell[0] + search_resolution;
       ++i)
    for (int j = cell[1] - search_resolution; j <= cell[1] + search_resolution;
         ++j) {
      if (i < 0 || j < 0 || i >= expected_food.size(0) ||
          j >= expected_food.size(1))
        continue;
      if (expected_food[i][j] >
          expected_food[best_cell[0]][best_cell[1]])
        best_cell = {i, j};
    }
  return Response{}.pos(cell_center(best_cell));
}

const Food *Strategy::find_nearest_food() {
  if (ctx->food.empty())
    return nullptr;
  auto dist = [&](const Food &food) {
    if (ctx->my_total_mass > constant::virus_danger_mass) {
      auto danger_dist =
          (ctx->config.virus_radius + ctx->my_radius) * 1.05 /*imprecision*/;
      for (auto &v : ctx->viruses) {
        if (v.pos.distance_to_line(ctx->my_center, food.pos) < danger_dist)
          return constant::infinity;
      }
    }

    return food.pos.squared_distance_to(ctx->my_center);
  };
  auto it = min_element_op(ctx->food.begin(), ctx->food.end(), dist);
  if (dist(*it) < constant::infinity)
    return &*it;

  return nullptr;
}

Point Strategy::future_center(double time) {
  return ctx->my_center + ctx->my_parts.front().speed * time;
}

Response Strategy::continue_movement() {
  if (ctx->my_parts.empty())
    return Response{}.debug("Too dead to move");

  return Response{}.pos(future_center(10.0));
}

Response Strategy::get_response(const Context &context) {
  ctx = &context;
  update();

  if (!ctx->my_parts.empty()) {
    if (auto enemy = find_dangerous_enemy()) {
      return run_away_from(enemy->pos);
    }

    if (auto food = find_nearest_food()) {
      return Response{}.pos(food->pos);
    }

    {
      static int last_tick = std::numeric_limits<int>::min();
      if (ctx->my_parts.front().speed.length() < standing_speed ||
          ctx->tick > last_tick + randomize_frequency) {

        last_tick = ctx->tick;
        return move_to_more_food();
      }

      return continue_movement();
    }
  }
  return Response{}.pos({}).debug("Died");
}

void Strategy::initialize(const GameConfig &config) {
  expected_food.resize(config.game_width / cell_size,
                       config.game_height / cell_size);
  food_expectancy_inc = 4.0 / constant::food_spawn_delay /
                        (expected_food.size(0) * expected_food.size(1));
}
