#include "MaxSpeedStrategy.h"
#include "Const.h"
#include "Context.h"
#include "GameHelpers.h"
#include "Matrix.h"
#include "MovingPoint.h"
#include "MyPart.h"
#include "Response.h"
#include <set>
#include <unordered_set>

MaxSpeedStrategy::MaxSpeedStrategy()
    : m_re(DEBUG_RELEASE_VALUE(0, std::random_device()())) {}

Response MaxSpeedStrategy::move_by_vector(const Point &v) {
  auto &c = ctx->my_center;
  auto C = -v.y * c.x + v.x * c.y;
  double xl_int = v.x < 0.0 ? -C / -v.x : constant::infinity;
  double yl_int = v.y < 0.0 ? -C / v.y : constant::infinity;
  double xr_int = v.x > 0.0 ? (-C - v.y * ctx->config.game_width) / -v.x
                            : constant::infinity;
  double yr_int = v.y > 0.0 ? (-C + v.x * ctx->config.game_height) / v.y
                            : constant::infinity;
  if (xl_int >= 0.0 && xl_int <= ctx->config.game_height)
    return Response{}.target({0., xl_int});
  if (xr_int >= 0.0 && xr_int <= ctx->config.game_height)
    return Response{}.target({ctx->config.game_width, xr_int});
  if (yl_int >= 0.0 && yl_int <= ctx->config.game_width)
    return Response{}.target({yl_int, 0.});
  if (yr_int >= 0.0 && yr_int <= ctx->config.game_width)
    return Response{}.target({yr_int, ctx->config.game_height});
  return {};
}

double MaxSpeedStrategy::calc_angle_score(double angle) {
  auto accel = (Point(1., 0.) * Matrix::rotation(angle));
  if (ctx->my_parts.empty())
    return 0.0;
  int scan_precision = static_cast<int>(
      std::max(10.0 / ctx->my_parts.back().max_speed(ctx->config), 1.0));

  double score = 0;
  static std::vector<MovingPoint> mps;
  mps.resize(ctx->my_parts.size());
  std::transform(ctx->my_parts.begin(), ctx->my_parts.end(), mps.begin(),
                 [](const MyPart &p) { return p.as_moving_point(); });
  static std::vector<MovingPoint> next_mps;
  next_mps.resize(ctx->my_parts.size());
  std::set<int> eaten_parts;
  for (int part_index = 0; part_index < ctx->my_parts.size(); ++part_index) {
    auto &mp = next_mps[part_index];
    mp = next_moving_point(mps[part_index], ctx->my_parts[part_index].mass,
                           accel, scan_precision, ctx->config);

    score += 300. * distance_to_nearest_wall(mp.pos, ctx->config) /
             (std::min(ctx->config.game_width, ctx->config.game_height) / 2.) /
             ctx->my_parts.size();

    auto r = ctx->sustainable_circle_radii[part_index];
    auto x_to_wall =
        x_distance_to_wall(mp, ctx->my_parts[part_index].radius, ctx->config);
    if (x_to_wall < r)
      score -= 100000.0 * ((r - x_to_wall) / r);
    auto y_to_wall =
        y_distance_to_wall(mp, ctx->my_parts[part_index].radius, ctx->config);
    if (y_to_wall < r)
      score -= 100000.0 * ((r - y_to_wall) / r);
    for (auto &enemy : ctx->players)
      if (can_eat(enemy.mass, ctx->my_parts[part_index].mass)) {
        auto eating_dist =
            eating_distance(enemy.radius, ctx->my_parts[part_index].radius);
        auto dist = enemy.pos.distance_to(next_mps[part_index].pos);
        if (dist < 6 * eating_dist) {
          score += (dist - 3 * eating_dist) * 500;
        }
        if (dist < 2 * eating_dist)
          eaten_parts.insert(part_index); // what is eaten could never eat
      }
  }
  static std::vector<int> alive_parts;
  alive_parts.clear();
  for (int i = 0; i < ctx->my_parts.size(); ++i)
    if (!eaten_parts.count(i))
      alive_parts.push_back(i);
  std::unordered_set<int> food_taken, ejection_taken, virus_bumped;
  for (int iteration = 0; iteration < future_scan_iteration_count;
       ++iteration) {
    auto check_food_like = [this, &score, iteration](auto &arr, auto &taken,
                                                     double mass) {
      for (int food_index = 0; food_index < arr.size(); ++food_index) {
        if (taken.count(food_index))
          continue;
        for (auto part_index : alive_parts) {
          if (arr[food_index].pos.is_in_circle(
                  next_mps[part_index].pos, ctx->my_parts[part_index].radius)) {
            score += (future_scan_iteration_count - iteration) * 10 * mass;
            taken.insert(food_index);
          }
        }
      }
    };
    check_food_like(m_food_seen, food_taken, ctx->config.food_mass);
    check_food_like(ctx->ejections, ejection_taken, constant::ejection_mass);
    if (!ctx->players.empty() || is_splitting_dangerous()) {
      for (int virus_index = 0; virus_index < ctx->viruses.size();
           ++virus_index) {
        if (virus_bumped.count(virus_index))
          continue;
        for (auto part_index : alive_parts) {
          if (is_virus_dangerous_for(ctx->config, ctx->viruses[virus_index].pos,
                                     next_mps[part_index].pos,
                                     ctx->my_parts[part_index].radius,
                                     ctx->my_parts[part_index].mass)) {
            const double score_per_virus =
                is_splitting_dangerous() ? 10000 : 300;
            score -=
                (future_scan_iteration_count - iteration) * score_per_virus;
            virus_bumped.insert(virus_index);
          }
        }
      }
    }

    std::set<int> players_taken;
    for (int player_index = 0; player_index < ctx->players.size();
         ++player_index) {
      if (players_taken.count(player_index))
        continue;
      for (auto part_index : alive_parts) {
        auto &enemy = ctx->players[player_index];
        auto &p = ctx->my_parts[part_index];
        if (can_eat(p.mass, next_mps[part_index].pos, p.radius, enemy.mass,
                    enemy.pos, enemy.radius)) {
          score += 50 * enemy.mass;
          players_taken.insert(player_index);
        }
      }
    }

    for (auto part_index : alive_parts) {
#if 0 && defined CUSTOM_DEBUG
      m_debug_lines.emplace_back();
      m_debug_lines.back()[0] = next_mps[part_index].pos;
#endif
      next_mps[part_index] = next_moving_point(
          next_mps[part_index], ctx->my_parts[part_index].mass, accel,
          scan_precision, ctx->config);
#if 0 && defined CUSTOM_DEBUG
      m_debug_lines.back()[1] = next_mps[part_index].pos;
#endif
    }
  }
  score += std::uniform_real_distribution<double>(0, 100)(m_re);
  return score;
}

bool MaxSpeedStrategy::is_splitting_dangerous() const {
  return ctx->config.inertia_factor < 4.0;
}

Response MaxSpeedStrategy::get_response_impl(bool try_to_keep_speed) {
  double best_angle = 0.0;
  double best_angle_score = -std::numeric_limits<int>::min();
#ifdef CUSTOM_DEBUG
  std::vector<std::array<Point, 2>> lines;
#endif

  double min_angle = 0;
  double max_angle = 2 * constant::pi;
  if (try_to_keep_speed && ctx->avg_speed.squared_length() > 1e-5) {
    auto sp_angle = ctx->avg_speed.angle();
    min_angle = sp_angle - constant::pi / 2.;
    max_angle = sp_angle + constant::pi / 2.;
  }

  for (int angle_try = 0; angle_try <= angle_discretization; ++angle_try) {
    double angle =
        min_angle + (max_angle - min_angle) * angle_try / angle_discretization;
    double score = calc_angle_score(angle);

    if (score > best_angle_score) {
      best_angle_score = score;
      best_angle = angle;
    }
  }
  if (best_angle_score < 0.0 && try_to_keep_speed) {
    return get_response_impl(false); // desperate times - desperate measures
  }
  auto r = move_by_vector(Point(1., 0.) * Matrix::rotation(best_angle));
  r.debug("Score: " + std::to_string(best_angle_score));
  if (!is_splitting_dangerous() &&
      ctx->my_parts.size() < ctx->config.max_fragments_cnt &&
      (ctx->players.empty() ||
       ctx->players.front().mass < 0.5 * ctx->my_parts.front().mass))
    r.split();
#ifdef CUSTOM_DEBUG
#if 0
  for (auto &f : m_food_seen)
    m_debug_lines.push_back({ctx->my_center, f.pos});
#endif
  r.debug_lines(m_debug_lines);
#endif
  return r;
}

void MaxSpeedStrategy::remove_eaten_food() {
  m_food_seen.erase(
      std::remove_if(m_food_seen.begin(), m_food_seen.end(),
                     [this](const FoodSeen &fs) {
                       for (auto &p : ctx->my_parts) {
                         if (p.visibility_center().distance_to(fs.pos) <
                                 p.visibility_radius(
                                     static_cast<int>(ctx->my_parts.size())) &&
                             ctx->food_map.count(fs.pos) == 0) {
                           auto it = m_food_seen_set.find(fs.pos);
                           m_food_seen_set.erase(it);
                           return true;
                         }
                       }
                       return false;
                     }),
      m_food_seen.end());
}

void MaxSpeedStrategy::remove_stale_food() {
  while (!m_food_seen.empty() &&
         m_food_seen.front().tick < ctx->tick - food_shelf_life) {
    auto it = m_food_seen_set.find(m_food_seen.front().pos);
    m_food_seen_set.erase(it);
    m_food_seen.pop_front();
  }
}

void MaxSpeedStrategy::add_new_food_to_seen() {
  for (auto &f : ctx->food) {
    if (ctx->food_map.count(f.pos) > m_food_seen_set.count(f.pos)) {
      m_food_seen.push_back({f.pos, ctx->tick});
      m_food_seen_set.insert(f.pos);
    }
  }
}

void MaxSpeedStrategy::update() {
  remove_stale_food();
  remove_eaten_food();
  add_new_food_to_seen();
}

Response MaxSpeedStrategy::get_response(const Context &context) {
  ctx = &context;
#ifdef CUSTOM_DEBUG
  m_debug_lines.clear();
#endif
  update();

  return get_response_impl(true);
}

void MaxSpeedStrategy::initialize(const GameConfig &config) {}
