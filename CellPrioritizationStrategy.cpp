#include "Const.h"
#include "KnownPlayer.h"
#include "Matrix.h"
#include "Object.h"
#include "Response.h"
#include "algorithm.h"
#include <queue>

#include "CellPrioritizationStrategy.h"
#include "GameHelpers.h"
#include "MovingPoint.h"

CellPrioritizationStrategy::CellPrioritizationStrategy()
    : m_re(std::random_device()()) {}

const Player *CellPrioritizationStrategy::find_caughtable_enemy() {
  auto get_mass = [this](const Player &pl) {
    if (!can_eat(ctx->my_largest_part->mass, 2.0 * pl.mass))
      return constant::infinity;
    return pl.mass;
  };
  if (ctx->players.empty())
    return nullptr;

  auto it = min_element_op(ctx->players.begin(), ctx->players.end(), get_mass);
  if (get_mass(*it) < constant::infinity)
    return &*it;

  return nullptr;
}

const Player *CellPrioritizationStrategy::find_dangerous_enemy() {
  for (auto &p : ctx->players) {
    if (can_eat(p.mass, ctx->my_parts.back().mass * 0.9))
      return &p;
  }
  return nullptr;
}

const Player *CellPrioritizationStrategy::find_weak_enemy() {
  auto get_mass = [this](const Player &pl) {
    if (!ctx->my_largest_part || !can_eat(ctx->my_largest_part->mass, pl.mass))
      return constant::infinity;
    return pl.mass;
  };
  if (ctx->players.empty())
    return nullptr;

  auto it = max_element_op(ctx->players.begin(), ctx->players.end(), get_mass);
  if (get_mass(*it) < constant::infinity)
    return &*it;

  return nullptr;
}

Response CellPrioritizationStrategy::move_randomly() const {
  auto x =
      std::uniform_real_distribution<double>(0.0, ctx->config.game_width)(m_re);
  auto y = std::uniform_real_distribution<double>(0.0, ctx->config.game_height)(
      m_re);
  return Response{}.target({x, y});
}

Cell CellPrioritizationStrategy::point_cell(const Point &point) const {
  return {static_cast<int>(point.x / cell_size),
          static_cast<int>(point.y / cell_size)};
}

void CellPrioritizationStrategy::check_visible_squares() {
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

template <typename T>
void CellPrioritizationStrategy::fill_circle(multi_vector<T, 2> &target,
                                             const T &val, const Point &pos,
                                             double radius) {
  auto left_top = point_cell(pos - Point{radius, radius});
  auto bottom_right = point_cell(pos + Point{radius, radius});
  for (auto ptr : {&left_top, &bottom_right}) {
    (*ptr)[0] = std::clamp((*ptr)[0], 0, cell_x_cnt - 1);
    (*ptr)[1] = std::clamp((*ptr)[1], 0, cell_y_cnt - 1);
  }
  for (int i = left_top[0]; i <= bottom_right[0]; ++i)
    for (int j = left_top[1]; j <= bottom_right[1]; ++j)
      target[i][j] = val;
}

void CellPrioritizationStrategy::update_danger() {
  danger_map.fill(0.0);
  auto mark_obj = [this](const Point &pos, double radius) {
    auto left_top = point_cell(pos - Point{radius, radius});
    auto bottom_right = point_cell(pos + Point{radius, radius});
    for (auto ptr : {&left_top, &bottom_right}) {
      (*ptr)[0] = std::clamp((*ptr)[0], 0, cell_x_cnt - 1);
      (*ptr)[1] = std::clamp((*ptr)[1], 0, cell_y_cnt - 1);
    }
    for (int i = left_top[0]; i <= bottom_right[0]; ++i)
      for (int j = left_top[1]; j <= bottom_right[1]; ++j)
        danger_map[i][j] = 1;
  };
  int cells_radius = static_cast<int>(2 * ctx->my_radius / cell_size);
  for (int i = 0; i < cell_x_cnt; ++i)
    for (int j = 0; j < cell_y_cnt; ++j)
      if (i < cells_radius || cell_x_cnt - 1 - i < cells_radius ||
          j < cells_radius || cell_y_cnt - 1 - j < cells_radius)
        danger_map[i][j] = 1;
  if (ctx->my_total_mass > constant::virus_danger_mass &&
      ctx->tick - last_tick_enemy_seen <
          ignore_viruses_when_enemy_was_not_seen_for)
    for (auto &v : ctx->viruses) {
      fill_circle(danger_map, 0.5, v.pos,
                  ctx->config.virus_radius + ctx->my_radius);
    }
  for (auto &p : ctx->players)
    if (can_eat(p.mass, ctx->my_total_mass))
      fill_circle(danger_map, 1.0, p.pos,
                  p.radius * constant::interaction_dist_coeff + ctx->my_radius);
}

const KnownPlayer *
CellPrioritizationStrategy::nearest_my_part_to(const Point &point) const {
  if (ctx->my_parts.empty())
    return nullptr;
  auto it = min_element_op(ctx->my_parts.begin(), ctx->my_parts.end(),
                           [&](const KnownPlayer &part) {
                             return part.pos.squared_distance_to(point);
                           });
  return &*it;
}

void CellPrioritizationStrategy::check_if_goal_is_reached() {
  for (auto ptr : {&goal}) {
    if (*ptr) {
      auto p = nearest_my_part_to(**ptr);
      if (!p) {
        *ptr = {};
        return;
      }
      if (p->pos.distance_to(**ptr) < 10.0)
        *ptr = {};
    }
  }
}

void CellPrioritizationStrategy::update_blocked_cells() {
  for (auto &val : blocked_cell)
    if (val > 0)
      --val;
}

void CellPrioritizationStrategy::update_enemies_seen() {
  for (auto &p : ctx->players)
    if (can_eat(p.mass, ctx->my_total_mass))
      fill_circle(dangerous_enemy_seen_tick, ctx->tick, p.pos, p.radius);
}

void CellPrioritizationStrategy::update_last_tick_enemy_seen() {
  if (!ctx->players.empty())
    last_tick_enemy_seen = ctx->tick;
}

void CellPrioritizationStrategy::check_subgoal() {
  if (ctx->tick == subgoal_reset_tick)
    subgoal = {};
}

void CellPrioritizationStrategy::update() {
  check_subgoal();
  check_if_goal_is_reached();
  check_visible_squares();
  update_danger();
  update_blocked_cells();
  update_enemies_seen();
  update_last_tick_enemy_seen();
}

namespace {
class PathFindingPoint {
public:
  Cell cell;
  CellSpeed speed; // discretizied by max speed
};
} // namespace

CellSpeed CellPrioritizationStrategy::to_cell_speed(const KnownPlayer &p,
                                                    const Point &val) const {
  if (val.length() < p.max_speed(ctx->config) * 0.5)
    return {0, 0};
  constexpr auto unit = constant::pi / 4;
  double angle = std::atan2(val.y, val.x) + unit / 2.;
  if (angle < 0.0)
    angle += 2 * constant::pi;
  auto index = static_cast<int>(angle / unit);
  constexpr CellSpeed tbl[] = {{1, 0},  {1, 1},   {0, 1},  {-1, 1},
                               {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
  return tbl[index];
}

Point CellPrioritizationStrategy::from_cell_speed(const KnownPlayer &p,
                                                  const CellSpeed &sp) const {
  double speed_length = p.max_speed(ctx->config);
  if (sp[0] != 0 && sp[1] != 0)
    speed_length *= constant::sqrt2;
  return {speed_length * sp[0], speed_length * sp[1]};
}

struct move_result {
  int ticks = -1;
  Cell shift;
  CellSpeed speed;
};

namespace {
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
} // namespace

std::optional<Point>
CellPrioritizationStrategy::next_step_to_goal(double max_danger_level) {
  static multi_vector<move_result, 4> move_cache(3, 3, 3, 3);
  move_cache.fill({});
  auto goal_cell = point_cell(*goal);
  if (subgoal)
    return *subgoal;

  if (danger_map[goal_cell] > max_danger_level) {
    reset_goal({});
    return {};
  }

#if CUSTOM_DEBUG
  debug_lines.clear();
#endif

  auto p = nearest_my_part_to(*goal);
  if (!p)
    return {};
  auto initial_cell = point_cell(p->pos);
  if (initial_cell == goal_cell)
    return *goal;
  static multi_vector<std::pair<PathFindingPoint, CellAccel>, 4> way;
  way.resize(cell_x_cnt, cell_y_cnt, 3, 3);
  way.fill({{-1, -1}, {0, 0}});
  static multi_vector<int, 4> time;
  time.resize(cell_x_cnt, cell_y_cnt, 3, 3);
  time.fill(constant::int_infinity);
  static std::deque<PathFindingPoint> q;
  q.clear();
  auto get_time = [&](const PathFindingPoint &pfp) -> auto & {
    return time[pfp.cell[0]][pfp.cell[1]][pfp.speed[0] + 1][pfp.speed[1] + 1];
  };
  auto get_way = [&](const PathFindingPoint &pfp) -> auto & {
    return way[pfp.cell[0]][pfp.cell[1]][pfp.speed[0] + 1][pfp.speed[1] + 1];
  };
  PathFindingPoint initial_pfp{initial_cell, to_cell_speed(*p, p->speed)};
  get_time(initial_pfp) = 0;
  q.push_back(initial_pfp);
  while (!q.empty()) {
    auto cur = q.front();
    q.pop_front();
    for (int x_dir = -1; x_dir <= 1; ++x_dir)
      for (int y_dir = -1; y_dir <= 1; ++y_dir) {
        if (x_dir == 0 && y_dir == 0)
          continue;

        PathFindingPoint next_point;
        int time_taken = 0;
        {
          auto &res = move_cache[cur.speed[0] + 1][cur.speed[1] + 1][x_dir + 1]
                                [y_dir + 1];
          if (res.ticks < 0) {
            MovingPoint cur_moving_pnt = {cell_center(cur.cell),
                                          from_cell_speed(*p, cur.speed)};
            int t = 0;
            while (point_cell(cur_moving_pnt.pos) == cur.cell) {
              constexpr int tick_resolution = 1;
              cur_moving_pnt = next_moving_point(
                  cur_moving_pnt, p->mass, from_cell_speed(*p, {x_dir, y_dir}),
                  tick_resolution, ctx->config);
              t += tick_resolution;
            }
            next_point = {point_cell(cur_moving_pnt.pos),
                          to_cell_speed(*p, cur_moving_pnt.speed)};
            res.ticks = t;
            res.shift = {next_point.cell[0] - cur.cell[0],
                         next_point.cell[1] - cur.cell[1]};
            res.speed = next_point.speed;
          }
          time_taken = res.ticks;
          next_point.speed = res.speed;
          next_point.cell = {cur.cell[0] + res.shift[0],
                             cur.cell[1] + res.shift[1]};
        }
        if (!is_valid_cell(next_point.cell))
          continue;
        if (danger_map[next_point.cell] > max_danger_level)
          continue;
        if (get_time(cur) + time_taken < get_time(next_point)) {
          get_way(next_point) = {cur, {x_dir, y_dir}};
          get_time(next_point) = get_time(cur) + time_taken;
          if (cur.cell == goal_cell)
            break;
          q.push_back(next_point);
        }
      }
  }
  int best_time = constant::int_infinity;
  std::optional<CellSpeed> best_speed;
  for (int dx = -1; dx <= 1; ++dx)
    for (int dy = -1; dy <= 1; ++dy) {
      auto t = time[goal_cell[0]][goal_cell[1]][dx + 1][dy + 1];
      if (t < best_time) {
        best_time = t;
        best_speed = {dx, dy};
      }
    }
  if (best_speed) {
    auto speed = *best_speed;
    PathFindingPoint pfp = {goal_cell, speed};
    while (true) {
      auto &w = get_way(pfp);
#if CUSTOM_DEBUG
      debug_lines.push_back({cell_center(pfp.cell), cell_center(w.first.cell)});
#endif
      if (w.first.cell == initial_cell)
        break;
      pfp = w.first;
    }
#if CUSTOM_DEBUG
    debug_lines.push_back({p->pos, cell_center(get_way(pfp).first.cell)});
#endif
    auto cell_acc = get_way(pfp).second;
    subgoal_reset_tick = ctx->tick + get_time(pfp);
    if (pfp.cell == goal_cell)
      return goal;
    return *(subgoal = p->pos + Point(cell_acc[0], cell_acc[1]) * 100.0);
  }

  if (max_danger_level < 0.1)
    return next_step_to_goal(0.6);

  blocked_cell[goal_cell] = blocked_cell_recheck_frequency;
  reset_goal({});
  return {};
}

Point CellPrioritizationStrategy::cell_center(const Cell &cell) {
  return {cell[0] * cell_size + cell_size * 0.5,
          cell[1] * cell_size + cell_size * 0.5};
}

double CellPrioritizationStrategy::cell_priority(const Cell &cell) const {
  constexpr auto tick_coeff = 1.0;
  constexpr auto center_coeff = 0.000;
  constexpr auto enemy_seen_tick_coeff = 10.0;
  if (danger_map[cell] > 0.0)
    return 0.0;
  double tick_diff = (ctx->tick - last_tick_visited[cell]);
  double center_shift = (sqrt(2.) * ctx->config.game_width -
                         Point{cell_x_cnt * 0.5, cell_y_cnt * 0.5}.distance_to(
                             Point(cell[0], cell[1])));
  auto angle1 = (cell_center(cell) - ctx->my_center).angle();
  auto angle2 = ctx->speed_angle;
  auto angle_badness = abs(angle1 - angle2) * 100.0;

  double enemy_seen_tick_diff = (ctx->tick - dangerous_enemy_seen_tick[cell]);
  return tick_coeff * tick_diff + center_coeff * center_shift +
         enemy_seen_tick_coeff * enemy_seen_tick_diff -
         cell_center(cell).distance_to(ctx->my_center) - angle_badness;
}

Response CellPrioritizationStrategy::move_to_goal_or_repriotize() {
  if (goal) {
    Response rsp;
    auto pos = next_step_to_goal(0.1);
    if (!pos)
      rsp = stop();
    else {
      auto p = nearest_my_part_to(*pos);
      if (!p)
        return stop();
      rsp.target(p->pos + (*pos - p->pos) * 5.0);
    }
    if (goal->distance_to(ctx->my_center) > goal_distance_to_justify_split &&
        ctx->tick - last_tick_enemy_seen > split_if_enemy_was_not_seen_for &&
        ctx->my_parts.front().mass >= constant::min_split_mass &&
        ctx->my_parts.size() <= max_parts_deliberately / 2)
      rsp.split();
    return rsp;
  }

  auto cell = point_cell(ctx->my_center);
  auto search_resolution = better_opportunity_search_resolution;
  auto best_cell = cell;
  constexpr auto food_in_sight_priority = 100.0;
  auto cur_priority =
      cell_priority(cell) + ctx->food.size() * food_in_sight_priority;
  auto best_priority = cur_priority;
  for (auto &c : cell_order) {
    if (danger_map[c] > 0.0)
      continue;
    auto priority = cell_priority(c);
    if (priority > best_priority) {
      best_cell = c;
      best_priority = priority;
    }
  }

  auto food_pos = best_food_pos();

  if (!food_pos || best_priority > cur_priority * new_opportunity_coeff)
    return Response{}.target(*(reset_goal(cell_center(best_cell))));
  if (food_pos)
    return Response{}.target(*(reset_goal(food_pos)));

  return continue_movement();
}

std::optional<Point> CellPrioritizationStrategy::best_food_pos() const {
  if (ctx->my_parts.empty())
    return {};
  auto my_radius = ctx->my_parts.front().radius;
  auto scan_radius = ctx->my_parts.front().visibility_radius(
                         static_cast<int>(ctx->my_parts.size())) -
                     my_radius;
  int try_cnt = 100;
  std::optional<Point> ans;
  int best_cnt = 0;
  for (int i = 0; i < try_cnt; ++i) {
    auto r = std::uniform_real_distribution<double>(my_radius * 2.0,
                                                    scan_radius)(m_re);
    auto alpha =
        std::uniform_real_distribution<double>(0.0, 2.0 * constant::pi)(m_re);
    auto point = ctx->my_parts.front().pos +
                 Point{r * std::cos(alpha), r * std::sin(alpha)};
    if (ctx->config.distance_to_border(point) < my_radius)
      continue;
    int cnt = 0;
    for (auto &f : ctx->food) {
      if (f.pos.squared_distance_to(point) < pow(my_radius, 2.0))
        ++cnt;
    }
    if (cnt > best_cnt) {
      best_cnt = cnt;
      ans = point;
    }
  }
  return ans;
}

const Food *CellPrioritizationStrategy::find_nearest_food() {
  if (ctx->food.empty())
    return nullptr;
  auto dist = [&](const Food &food) {
    if (ctx->my_total_mass > constant::virus_danger_mass) {
      auto danger_dist =
          ctx->config.virus_radius * constant::interaction_dist_coeff +
          ctx->my_radius;
      for (auto &v : ctx->viruses) {
        if (v.pos.distance_to(food.pos) < danger_dist)
          return constant::infinity;
      }
    }
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j) {
        Point corner(i * ctx->config.game_width, j * ctx->config.game_height);
        if (corner.distance_to(food.pos) > ctx->my_radius)
          continue;
        corner.x = fabs(corner.x - ctx->my_radius);
        corner.y = fabs(corner.y - ctx->my_radius);
        if (corner.distance_to(food.pos) > ctx->my_radius)
          return constant::infinity;
      }

    return food.pos.squared_distance_to(ctx->my_center);
  };
  auto it = min_element_op(ctx->food.begin(), ctx->food.end(), dist);
  if (dist(*it) < constant::infinity)
    return &*it;

  return nullptr;
}

Point CellPrioritizationStrategy::future_center(double time) {
  return ctx->my_center + ctx->my_parts.front().speed * time;
}

Response CellPrioritizationStrategy::continue_movement() {
  if (ctx->my_parts.empty())
    return Response{}.debug("Too dead to move");

  return Response{}.target(future_center(10.0));
}

Response CellPrioritizationStrategy::stop() {
  if (ctx->my_parts.empty())
    return Response{}.debug("Too dead to even stop");

  return Response{}.target(ctx->my_center);
}

std::optional<Point>
CellPrioritizationStrategy::reset_goal(std::optional<Point> point) {
  goal = point;
  subgoal = {};
  return goal;
}

Response CellPrioritizationStrategy::get_response(const Context &context) {
  ctx = &context;
  update();

  if (!ctx->my_parts.empty()) {
    bool goal_set = false;
    if (auto enemy = find_dangerous_enemy()) {
      goal_set = try_run_away_from(enemy->pos);
    }
    if (!goal_set) {
      if (ctx->my_parts.size() == 1) {
        if (auto enemy = find_caughtable_enemy()) {
          if ((ctx->my_parts.front().speed.normalized() -
               (enemy->pos - ctx->my_center).normalized())
                  .length() > 1e-2)
            return Response{}.target(enemy->pos);
          else
            return Response{}.target(enemy->pos).split(true);
        }
      }
    }
    if (!goal_set) {
      if (auto enemy = find_weak_enemy()) {
        reset_goal(enemy->pos);
        goal_set = true;
      }
    }

    auto rsp = move_to_goal_or_repriotize();
#ifdef CUSTOM_DEBUG
    rsp.debug_lines(debug_lines);
#endif
    return rsp;
  }
  return Response{}.target({}).debug("Died");
}

bool CellPrioritizationStrategy::is_valid_cell(const Cell &cell) const {
  return cell[0] >= 0 && cell[0] < cell_x_cnt && cell[1] >= 0 &&
         cell[1] < cell_y_cnt;
}

void CellPrioritizationStrategy::initialize(const GameConfig &config) {
  cell_x_cnt = static_cast<int>(config.game_width / cell_size);
  cell_y_cnt = static_cast<int>(config.game_height / cell_size);
  auto resize_arr = [this](auto &arr) { arr.resize(cell_x_cnt, cell_y_cnt); };
  resize_arr(danger_map);
  resize_arr(last_tick_visited);
  resize_arr(blocked_cell);
  resize_arr(dangerous_enemy_seen_tick);
  last_tick_visited.fill(-100);
  dangerous_enemy_seen_tick.fill(-100);
  for (int i = 0; i < cell_x_cnt; ++i)
    for (int j = 0; j < cell_y_cnt; ++j)
      cell_order.push_back({i, j});

  std::shuffle(cell_order.begin(), cell_order.end(), m_re);
}

bool CellPrioritizationStrategy::try_run_away_from(const Point &enemy_pos) {
  auto try_point = [this](const Point &next_point) {
    auto cell = point_cell(next_point);
    if (ctx->config.is_point_inside(next_point) &&
        danger_map[cell] < constant::eps && blocked_cell[cell] == 0) {
      reset_goal(next_point);
      return true;
    }
    return false;
  };
  auto vec = (ctx->my_center - enemy_pos).normalized() * 150.0;
  double angle = 0;
  double angle_inc = constant::pi / 16;
  while (angle < constant::pi) {
    if (try_point(ctx->my_center + vec * Matrix::rotation(-angle)))
      return true;
    if (try_point(ctx->my_center + vec * Matrix::rotation(angle)))
      return true;
    angle += angle_inc;
  }

  return false;
}
