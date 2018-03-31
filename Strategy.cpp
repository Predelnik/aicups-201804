#include "Strategy.h"
#include "Const.h"
#include "MyPart.h"
#include "Object.h"
#include "Response.h"
#include "algorithm.h"
#include <queue>
#include <set>

Strategy::Strategy() : m_re(std::random_device()()) {}

const Player *Strategy::find_dangerous_enemy() {
  for (auto &p : ctx->players) {
    if (p.is_dangerous(ctx->my_total_mass))
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

Cell Strategy::mark_visited(const Point &point) const {
  return {static_cast<int>(point.x / cell_size),
          static_cast<int>(point.y / cell_size)};
}

void Strategy::check_visible_squares() {
  for (int i = 0; i < cell_x_cnt; ++i)
    for (int j = 0; j < cell_y_cnt; ++j) {
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
        last_tick_visited[i][j] = ctx->tick;
    }
}

void Strategy::update_danger() {
  danger_map.fill(0.0);
  auto mark_obj = [this](const Point &pos, double radius) {
    auto left_top = mark_visited(pos - Point{radius, radius});
    auto bottom_right = mark_visited(pos + Point{radius, radius});
    for (auto ptr : {&left_top, &bottom_right}) {
      (*ptr)[0] = std::clamp((*ptr)[0], 0, cell_x_cnt - 1);
      (*ptr)[1] = std::clamp((*ptr)[1], 0, cell_y_cnt - 1);
    }
    for (int i = left_top[0]; i <= bottom_right[0]; ++i)
      for (int j = left_top[1]; j <= bottom_right[1]; ++j)
        danger_map[i][j] = 1;
  };
  if (ctx->my_total_mass > constant::virus_danger_mass)
    for (auto &v : ctx->viruses) {
      mark_obj(v.pos, ctx->config.virus_radius + ctx->my_radius);
    }
  for (auto &p : ctx->players)
    if (p.is_dangerous(ctx->my_total_mass))
      mark_obj(p.pos,
               (p.radius + ctx->my_radius) * constant::eating_dist_coeff);
}

void Strategy::update_goal() {
  if (goal) {
    if (goal->distance_to(ctx->my_center) < ctx->my_radius)
      goal = std::nullopt;
  }
}

void Strategy::update_blocked_cells()
{
  for (auto &val : blocked_cell)
      if (val > 0)
          --val;
}

void Strategy::update() {
  update_goal();
  check_visible_squares();
  update_danger();
  update_blocked_cells ();
}

Response Strategy::next_step_to_goal() {
  auto goal_cell = mark_visited(*goal);
  if (danger_map[goal_cell] > 0.0) {
    goal = {};
    return stop();
  }

  auto cur_cell = mark_visited(ctx->my_center);
  if (cur_cell == goal_cell)
    return Response{}.pos(*goal);
  static multi_vector<Cell, 2> prev;
  prev.resize(cell_x_cnt, cell_y_cnt);
  prev.fill({-1, -1});
  std::queue<Cell> q;
  q.push(cur_cell);
  while (!q.empty()) {
    auto cur = q.front();
    q.pop();
    for (int x_dir = -1; x_dir <= 1; ++x_dir)
      for (int y_dir = -1; y_dir <= 1; ++y_dir) {
        if (x_dir == 0 && y_dir == 0)
          continue;
        auto next_cell = cur;
        next_cell[0] += x_dir;
        next_cell[1] += y_dir;
        if (!is_valid_cell(next_cell))
          continue;
        if (prev[next_cell][0] < 0) {
          prev[next_cell] = cur;
          if (next_cell == goal_cell) {
            while (prev[next_cell] != cur_cell)
              next_cell = prev[next_cell];
            return Response{}.pos(cell_center(next_cell));
          }
          q.push(next_cell);
        }
      }
  }
  blocked_cell[goal_cell] = blocked_cell_recheck_frequency;
  goal = {};
  return stop();
}

Point Strategy::cell_center(const Cell &cell) const {
  return {cell[0] * cell_size + cell_size * 0.5,
          cell[1] * cell_size + cell_size * 0.5};
}

double Strategy::cell_priority(const Cell &cell) const {
  constexpr auto tick_coeff = 1.0;
  constexpr auto center_coeff = 1.0;
  if (danger_map[cell] > 0.0)
    return 0.0;
  return (ctx->tick - last_tick_visited[cell]) * tick_coeff +
         center_coeff * (sqrt (2.) * ctx->config.game_width -
             Point{cell_x_cnt * 0.5, cell_y_cnt * 0.5}.distance_to(
                 Point (cell[0], cell[1])));
}

Response Strategy::move_to_more_food() {
  auto cell = mark_visited(ctx->my_center);
  auto search_resolution = better_opportunity_search_resolution;
  auto best_cell = cell;
  auto cur_priority = cell_priority(cell);
  auto best_priority = cur_priority;
  for (int i = cell[0] - search_resolution; i <= cell[0] + search_resolution;
       ++i)
    for (int j = cell[1] - search_resolution; j <= cell[1] + search_resolution;
         ++j) {
      if (!is_valid_cell({i, j}))
        continue;
      auto priority = cell_priority({i, j});
      if (priority > best_priority) {
        best_cell = {i, j};
        best_priority = priority;
      }
    }

  auto nearest_food = find_nearest_food();
  if (goal) {
    return next_step_to_goal();
  }

  if (!nearest_food || best_priority > cur_priority * new_opportunity_coeff)
    return Response{}.pos(*(goal = cell_center(best_cell)));
  if (nearest_food)
    return Response{}.pos(*(goal = nearest_food->pos));

  return continue_movement();
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

Response Strategy::stop() {
  if (ctx->my_parts.empty())
    return Response{}.debug("Too dead to even stop");

  return Response{}.pos(ctx->my_center);
}

Response Strategy::get_response(const Context &context) {
  ctx = &context;
  update();

  if (!ctx->my_parts.empty()) {
    if (auto enemy = find_dangerous_enemy()) {
      goal = {};
      return run_away_from(enemy->pos);
    }
    return move_to_more_food();
  }
  return Response{}.pos({}).debug("Died");
}

bool Strategy::is_valid_cell(const Cell &cell) const {
  return cell[0] >= 0 && cell[0] < cell_x_cnt && cell[1] >= 0 &&
         cell[1] < cell_y_cnt;
}

void Strategy::initialize(const GameConfig &config) {
  cell_x_cnt = static_cast<int>(config.game_width / cell_size);
  cell_y_cnt = static_cast<int>(config.game_height / cell_size);
  auto resize_arr = [this](auto &arr) { arr.resize(cell_x_cnt, cell_y_cnt); };
  resize_arr(danger_map);
  resize_arr(last_tick_visited);
  resize_arr(blocked_cell);
  last_tick_visited.fill(-100);
}
