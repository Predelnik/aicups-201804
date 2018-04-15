#include "MaxSpeedStrategy.h"
#include "Const.h"
#include "Context.h"
#include "GameHelpers.h"
#include "KnownPlayer.h"
#include "Matrix.h"
#include "MovingPoint.h"
#include "Response.h"
#include <set>
#include <unordered_set>

#include "range.hpp"
using namespace util::lang;

#ifdef CUSTOM_DEBUG
#define DEBUG_FUTURE_OUTCOMES 1
#endif

MaxSpeedStrategy::MaxSpeedStrategy()
    : m_re(DEBUG_RELEASE_VALUE(0, std::random_device()())) {}

Point MaxSpeedStrategy::border_point_by_vector(const Point &v) {
  auto &c = ctx->my_center;
  auto C = -v.y * c.x + v.x * c.y;
  double xl_int = v.x < 0.0 ? -C / -v.x : constant::infinity;
  double yl_int = v.y < 0.0 ? -C / v.y : constant::infinity;
  double xr_int = v.x > 0.0 ? (-C - v.y * ctx->config.game_width) / -v.x
                            : constant::infinity;
  double yr_int = v.y > 0.0 ? (-C + v.x * ctx->config.game_height) / v.y
                            : constant::infinity;
  if (xl_int >= 0.0 && xl_int <= ctx->config.game_height)
    return {0., xl_int};
  if (xr_int >= 0.0 && xr_int <= ctx->config.game_height)
    return {ctx->config.game_width, xr_int};
  if (yl_int >= 0.0 && yl_int <= ctx->config.game_width)
    return {yl_int, 0.};
  if (yr_int >= 0.0 && yr_int <= ctx->config.game_width)
    return {yr_int, ctx->config.game_height};
  return ctx->my_center;
}

Response MaxSpeedStrategy::move_by_vector(const Point &v) {
  return Response{}.target(border_point_by_vector(v));
}

double MaxSpeedStrategy::calc_target_score(const Point& target) {
  if (ctx->my_parts.empty())
    return 0.0;
  int scan_precision = static_cast<int>(
      std::max(20.0 / ctx->my_parts.back().max_speed(ctx->config) /
                   sqrt(ctx->config.inertia_factor),
               1.0));

  double score = 0;
  static std::vector<KnownPlayer> my_predicted_parts, predicted_enemies;
  my_predicted_parts = ctx->my_parts;
  predicted_enemies = ctx->enemies;
  std::set<size_t> alive_parts;
  for (auto my_part_index : indices(ctx->my_parts))
    alive_parts.insert(my_part_index);
  for (auto &e : predicted_enemies)
    advance(e, e.speed, scan_precision, ctx->config);
  for (auto part_index : indices(ctx->my_parts)) {
    auto cur_speed = ctx->my_parts[part_index].speed.length();
    auto max_speed_diff =
        cur_speed / max_speed(ctx->my_parts[part_index].mass, ctx->config);
    advance(my_predicted_parts[part_index], target - my_predicted_parts[part_index].pos, scan_precision, ctx->config);
    auto speed_loss =
        (cur_speed - my_predicted_parts[part_index].speed.length()) / cur_speed;
    if (speed_loss > 0.25)
      score -= (speed_loss * 50000.0);
    else if (speed_loss > 0.05 &&
             cur_speed < ctx->config.game_max_size() / 100.0)
      score -= 50000.0;

    score += 200. *
             distance_to_nearest_wall(my_predicted_parts[part_index].pos,
                                      ctx->config) /
             (std::min(ctx->config.game_width, ctx->config.game_height) / 2.) /
             ctx->my_parts.size();
    for (auto &enemy : predicted_enemies)
      if (can_eat(enemy.mass, ctx->my_parts[part_index].mass)) {
        auto eating_dist =
            eating_distance(enemy.radius, ctx->my_parts[part_index].radius);
        auto dist = enemy.pos.distance_to(my_predicted_parts[part_index].pos);
        if (dist < 6 * eating_dist) {
          score -= (6 * eating_dist - dist) * 5000;
        }
        if (dist < 2 * eating_dist)
          alive_parts.erase(part_index); // what is eaten could never eat
      }
  }
  double deviation = 0.0;
  for (auto &p : my_predicted_parts)
    deviation += p.pos.squared_distance_to(ctx->my_center);
  deviation /= my_predicted_parts.size();
  deviation = sqrt(deviation);
  if (deviation > 100.0) {
    score -= (deviation - 100.0) * 1000.0;
  }
  std::unordered_set<size_t> food_taken, ejection_taken, virus_bumped;

  for (auto part_index : alive_parts) {
    auto ticks = static_cast<int>(
        200.0 / max_speed(ctx->my_parts[part_index].mass, ctx->config) /
        sqrt(ctx->config.inertia_factor));
    auto mp = ctx->my_parts[part_index];
    advance(mp, target - my_predicted_parts[part_index].pos, ticks, ctx->config);

    if (distance_to_nearest_wall(mp.pos, mp.radius, ctx->config) < 1e-5)
      score -= 100000.0;
  }

  for (auto iteration : range(0, future_scan_iteration_count)) {
    auto check_food_like = [this, &score, iteration,
                            &alive_parts](auto &arr, auto &taken, double mass) {
      for (auto food_index : indices(arr)) {
        if (taken.count(food_index))
          continue;
        for (auto part_index : alive_parts) {
          if (arr[food_index].pos.is_in_circle(
                  my_predicted_parts[part_index].pos,
                  ctx->my_parts[part_index].radius)) {
            score += (future_scan_iteration_count - iteration) * 10 * mass;
            taken.insert(food_index);
          }
        }
      }
    };
    check_food_like(m_food_seen, food_taken, ctx->config.food_mass);
    check_food_like(ctx->ejections, ejection_taken, constant::ejection_mass);
    if (!ctx->enemies.empty() || is_splitting_dangerous()) {
      for (auto virus_index : indices(ctx->viruses)) {
        if (virus_bumped.count(virus_index))
          continue;
        for (auto part_index : alive_parts) {
          if (is_virus_dangerous_for(ctx->config, ctx->viruses[virus_index].pos,
                                     my_predicted_parts[part_index].pos,
                                     ctx->my_parts[part_index].radius * 1.05,
                                     ctx->my_parts[part_index].mass * 1.05)) {
            const double score_per_virus =
                is_splitting_dangerous() ? 10000 : 300;
            score -=
                (future_scan_iteration_count - iteration) * score_per_virus;
            virus_bumped.insert(virus_index);
          }
        }
      }
    }

    std::set<size_t> players_taken;
    for (auto enemy_index : indices(predicted_enemies)) {
      if (players_taken.count(enemy_index))
        continue;
      for (auto part_index : alive_parts) {
        auto &enemy = predicted_enemies[enemy_index];
        auto &p = ctx->my_parts[part_index];
        if (can_eat(p.mass, my_predicted_parts[part_index].pos, p.radius,
                    enemy.mass, enemy.pos, enemy.radius)) {
          score += 50 * enemy.mass;
          players_taken.insert(enemy_index);
        }
      }
    }

    for (auto part_index : alive_parts) {
#if DEBUG_FUTURE_OUTCOMES
      m_debug_lines.emplace_back();
      m_debug_lines.back()[0] = my_predicted_parts[part_index].pos;
#endif
      advance(my_predicted_parts[part_index], target - my_predicted_parts[part_index].pos, scan_precision,
              ctx->config);
#if DEBUG_FUTURE_OUTCOMES
      m_debug_lines.back()[1] = my_predicted_parts[part_index].pos;
#endif
    }

    for (auto &e : predicted_enemies)
      advance(e, e.speed, scan_precision, ctx->config);
  }
  score += std::uniform_real_distribution<double>(0, 100)(m_re);
#if DEBUG_FUTURE_OUTCOMES
  auto s = m_debug_line_colors.size();
  m_debug_line_colors.resize(m_debug_lines.size());
  using namespace std::string_literals;

  auto to_hex = [](int x) {
    std::stringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(2) << x;
    return stream.str();
  };
  std::fill(m_debug_line_colors.begin() + s, m_debug_line_colors.end(),
            "#"s + to_hex(std::clamp(0, static_cast<int>(-score), 255)) +
                to_hex(std::clamp(0, static_cast<int>(score / 10.), 255)) +
                "00");
#endif
  return score;
}

bool MaxSpeedStrategy::is_splitting_dangerous() const {
  return false;
  // return ctx->config.inertia_factor < 4.0;
}

Response MaxSpeedStrategy::get_response_impl() {
#ifdef DEBUG_FUTURE_OUTCOMES
  m_debug_lines.clear();
  m_debug_line_colors.clear();
#endif
  double best_angle = 0.0;
  double best_angle_score = -std::numeric_limits<int>::min();

  double min_angle = 0;
  double max_angle = 2 * constant::pi;

  for (auto angle_try : range(0, angle_partition_count)) {
    double angle =
        min_angle + (max_angle - min_angle) * angle_try / angle_partition_count;
    auto target = border_point_by_vector((Point(1., 0.) * Matrix::rotation(angle)));
    double score = calc_target_score(target);

    if (score > best_angle_score) {
      best_angle_score = score;
      best_angle = angle;
    }
  }
  auto r = move_by_vector(Point(1., 0.) * Matrix::rotation(best_angle));
  r.debug("Score: " + std::to_string(best_angle_score));
  if (!is_splitting_dangerous() &&
      ctx->my_parts.size() < ctx->config.max_fragments_cnt &&
      (ctx->enemies.empty() ||
       ctx->enemies.front().mass < 0.5 * ctx->my_parts.front().mass))
    r.split();
#ifdef CUSTOM_DEBUG
#if 0
  for (auto &f : m_food_seen)
    m_debug_lines.push_back({ctx->my_center, f.pos});
#endif
  r.debug_lines(m_debug_lines).debug_line_colors(m_debug_line_colors);
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

  return get_response_impl();
}

void MaxSpeedStrategy::initialize(const GameConfig &config) {}
