#include "MaxSpeedStrategy.h"
#include "Const.h"
#include "Context.h"
#include "GameHelpers.h"
#include "KnownPlayer.h"
#include "Matrix.h"
#include "Response.h"
#include "algorithm.h"
#include <set>
#include <unordered_set>

#include "debug.h"
#include "range.hpp"
#include <iostream>
#include <queue>

using namespace util::lang;
using namespace std::string_literals;

#ifdef CUSTOM_DEBUG
#define DEBUG_FUTURE_OUTCOMES 0
#define DEBUG_FUSION 0
#define DEBUG_MEMORIZED_ENEMIES 0
#endif

constexpr bool debug_bump_prediction = false;
constexpr bool debug_rect_between_wall_and_enemy = false;

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

void MaxSpeedStrategy::calculate_fusions(
    const std::vector<KnownPlayer> &enemies) {
  m_fused.resize(enemies.size());
  std::fill(m_fused.begin(), m_fused.end(), 0);
  for (auto enemy_index : indices(enemies)) {
    if (m_fused[enemy_index])
      continue;
    if (enemies[enemy_index].ticks_to_fuse > 50) {
      m_fused[enemy_index] = 1;
      continue;
    }

    m_fusions.clear();
    m_fusions.push_back(enemies[enemy_index]);
    m_fusions.back().pos *= m_fusions.back().mass;
    m_fused[enemy_index] = 2;
    int cnt = 1;
    bool fusion_happened = true;
    while (fusion_happened) {
      fusion_happened = false;
      for (auto ind : indices(enemies)) {
        if (m_fused[ind])
          continue;
        if ((m_fusions.back().pos / m_fusions.back().mass)
                .squared_distance_to(enemies[ind].pos) >
            pow(1.1 * (enemies[ind].radius +
                       radius_by_mass(m_fusions.back().mass)),
                2))
          continue;

        if (enemies[ind].id.player_num == m_fusions.back().id.player_num) {
          {
            if (enemies[ind].ticks_to_fuse > 50)
              continue;
          }
        } else {
          // fusing of different enemies (aka eating)
          auto m1 = enemies[ind].mass;
          auto m2 = m_fusions.back().mass;
          if (std::max(m1, m2) / std::min(m1, m2) <
              constant::eating_mass_coeff * 0.95)
            continue;
        }

        ++cnt;
        fusion_happened = true;
        m_fused[ind] = 2;
        m_fusions.back().pos += enemies[ind].pos * enemies[ind].mass;
        m_fusions.back().mass += enemies[ind].mass;
      }
    }

    if (cnt == 1) {
      m_fusions.pop_back();
      m_fused[enemy_index] = 1;
    } else
      m_fusions.back().pos /= m_fusions.back().mass;
  }
}

int MaxSpeedStrategy::get_scan_precision() const {
  return static_cast<int>(
      std::max(20.0 / ctx->my_parts.back().max_speed(ctx->config) /
                   sqrt(ctx->config.inertia_factor),
               1.0));
}

static std::string to_hex(int x) {
  std::stringstream stream;
  stream << std::hex << std::setfill('0') << std::setw(2) << x;
  return stream.str();
};

double MaxSpeedStrategy::calc_target_score(const Point &target) {
  if (ctx->my_parts.empty())
    return 0.0;
  int scan_precision = get_scan_precision();

#ifdef CUSTOM_DEBUG
  std::map<std::string, double> reasons_stacked;
#endif

  double score = 0;
  auto change_score = [&](double amount, const char *reason) {
    score += amount;
#ifdef CUSTOM_DEBUG
    if (ctx->is_debug_tick()) {
      reasons_stacked[reason] += amount;
    }
#endif
  };
  static std::vector<KnownPlayer> my_predicted_parts;
  my_predicted_parts = ctx->my_parts;
  std::set<size_t> alive_parts;
  for (auto my_part_index : indices(ctx->my_parts))
    alive_parts.insert(my_part_index);

  bool dangerous_enemy_present = false;
  for (auto [tick, enemy_id] : ctx->enemy_ids_seen_by_tick) {
    if (tick < ctx->tick - 50)
      continue;
    for (auto &p : ctx->my_parts)
      if (can_eat(ctx->enemy_vision_by_id[enemy_id].state.mass,
                  p.mass * 0.95)) {
        dangerous_enemy_present = true;
        break;
      }
    if (dangerous_enemy_present)
      break;
  }

  for (auto part_index : indices(ctx->my_parts)) {
    auto cur_speed = ctx->my_parts[part_index].speed.length();
    auto max_speed_diff =
        cur_speed / max_speed(ctx->my_parts[part_index].mass, ctx->config);
    advance(my_predicted_parts[part_index],
            target - my_predicted_parts[part_index].pos, scan_precision,
            ctx->config);
    auto speed_loss =
        (cur_speed - my_predicted_parts[part_index].speed.length()) / cur_speed;
    auto speed_loss_limit = 0.05;
    if (cur_speed >
        ctx->config.game_max_size() /
            100.0) /* with huge speed factors it's fine to lose speed */
      speed_loss_limit = 0.25;
    if (speed_loss > speed_loss_limit)
      change_score(-((speed_loss - speed_loss_limit) * 25000.0),
                   "Speed Limit Loss");

    auto dist_to_wall = distance_to_nearest_wall(
        my_predicted_parts[part_index].pos, ctx->config);
    auto half_min_size =
        (std::min(ctx->config.game_width, ctx->config.game_height) / 2.);
    change_score(500. * dist_to_wall / half_min_size / ctx->my_parts.size(),
                 "Bonus for Center");
    for (auto &f : m_fusions) {
      if (can_eat(f.mass, ctx->my_parts[part_index].mass * 0.95)) {
        dangerous_enemy_present = true;
        auto r = radius_by_mass(f.mass);
        auto eating_dist = eating_distance(r, ctx->my_parts[part_index].radius);
        auto dist = f.pos.distance_to(my_predicted_parts[part_index].pos);
        if (dist < 3 * eating_dist) {
          change_score(-(3 * eating_dist - dist) * 4000,
                       "Danger of enemy fusion");
        }
      }
    }
  }

  double deviation = 0.0;
  for (auto &p : my_predicted_parts)
    deviation += p.pos.squared_distance_to(ctx->my_center);
  deviation /= my_predicted_parts.size();
  deviation = sqrt(deviation);
  if (deviation > 100.0) {
    change_score(-(deviation - 100.0) * 1000.0, "Deviation from center");
  }
  std::unordered_set<size_t> food_taken, ejection_taken, virus_bumped;

  for (auto part_index : alive_parts) {
    auto ticks = static_cast<int>(
        200.0 / max_speed(ctx->my_parts[part_index].mass, ctx->config) /
        sqrt(ctx->config.inertia_factor));
    auto mp = ctx->my_parts[part_index];
    advance(mp, target - my_predicted_parts[part_index].pos, ticks,
            ctx->config);
#if CUSTOM_DEBUG
    if constexpr (debug_bump_prediction) {
      m_debug_lines.push_back({ctx->my_parts[part_index].pos, mp.pos});
      m_debug_line_colors.emplace_back("#000");
    }
#endif

    if (dangerous_enemy_present) {
      auto mh_dist_to_corner =
          distance_to_nearest_wall_by_x(mp.pos, mp.radius, ctx->config) +
          distance_to_nearest_wall_by_y(mp.pos, mp.radius, ctx->config);
      if (mh_dist_to_corner < 5 * mp.radius)
        change_score(-100.0 * (5 * mp.radius - mh_dist_to_corner),
                     "Avoid corners penalty");
    }

    if (distance_to_nearest_wall(mp.pos, mp.radius, ctx->config) < 1e-5)
      change_score(-100000.0, "Bump into walls penalty");

    for (auto &e : ctx->enemies) {
      auto check_rect = [&](const std::array<Point, 2> &rect) {
        if (fabs(rect[0].x - rect[1].x) * fabs(rect[0].y - rect[1].y) > 7600.0)
          return;
#ifdef CUSTOM_DEBUG
        if constexpr (debug_rect_between_wall_and_enemy) {
          m_debug_lines.push_back(
              {Point{rect[0].x, rect[0].y}, Point{rect[1].x, rect[0].y}});
          m_debug_lines.push_back(
              {Point{rect[0].x, rect[1].y}, Point{rect[1].x, rect[1].y}});
          m_debug_lines.push_back(
              {Point{rect[0].x, rect[0].y}, Point{rect[0].x, rect[1].y}});
          m_debug_lines.push_back(
              {Point{rect[1].x, rect[0].y}, Point{rect[1].x, rect[1].y}});
          for (auto i : range(0, 4))
            m_debug_line_colors.emplace_back("#000");
        }
#endif
        if (mp.pos.is_in_rect(rect)) {
          change_score(-20000.0, "Being stuck between wall and enemy");
        }
      };
      if (can_eat(e.mass, ctx->my_parts[part_index].mass)) {
        auto r = e.radius * 1.1;
        check_rect({Point{0, e.pos.y - r}, Point{e.pos.x + r, e.pos.y + r}});
        check_rect({Point{e.pos.x - r, e.pos.y - r},
                    Point{ctx->config.game_width, e.pos.y + r}});
        check_rect({Point{e.pos.x - r, 0}, Point{e.pos.x + r, e.pos.y + r}});
        check_rect({Point{e.pos.x - r, e.pos.y - r},
                    Point{e.pos.x + r, ctx->config.game_height}});
      }
    }
  }

  for (auto iteration : range(0, future_scan_iteration_count)) {
    auto check_food_like = [this, iteration, &change_score,
                            &alive_parts](auto &arr, auto &taken, double mass) {
      for (auto food_index : indices(arr)) {
        if (taken.count(food_index))
          continue;
        for (auto part_index : alive_parts) {
          if (arr[food_index].pos.is_in_circle(
                  my_predicted_parts[part_index].pos,
                  ctx->my_parts[part_index].radius)) {
            change_score((future_scan_iteration_count - iteration) * 10 * mass,
                         "Food bonus");
            taken.insert(food_index);
          }
        }
      }
    };
    check_food_like(m_food_seen, food_taken, ctx->config.food_mass);
    check_food_like(ctx->ejections, ejection_taken, constant::ejection_mass);
    if ((!ctx->enemies.empty() &&
         ctx->my_parts.size() < ctx->config.max_fragments_cnt) ||
        is_splitting_dangerous()) {
      for (auto virus_index : indices(ctx->viruses)) {
        if (virus_bumped.count(virus_index))
          continue;
        for (auto part_index : alive_parts) {
          auto mass = ctx->my_parts[part_index].mass * 1.05;
          if (mass < constant::virus_danger_mass)
            continue;

          auto radius = ctx->my_parts[part_index].radius * 1.05;

          if (ctx->config.virus_radius > radius)
            continue;

          auto dangerous_dist =
              ctx->config.virus_radius * constant::virus_hurt_factor + radius;

          auto dist = ctx->viruses[virus_index].pos.distance_to(
              my_predicted_parts[part_index].pos);
          if (dist < 2 * dangerous_dist) {
            const double score_per_virus = 1000;
            change_score(-(future_scan_iteration_count - iteration) *
                             ((2 * dangerous_dist - dist) / dangerous_dist) *
                             score_per_virus,
                         "Virus bumping in presence of the enemy penalty");
            virus_bumped.insert(virus_index);
          }
        }
      }
    }

    std::set<size_t> players_taken;
    for (auto enemy_index : indices(m_predicted_enemies[iteration])) {
      if (m_fused[enemy_index] == 2)
        continue;
      if (players_taken.count(enemy_index))
        continue;
      for (auto part_index : alive_parts) {
        auto &enemy = m_predicted_enemies[iteration][enemy_index];
        auto &p = ctx->my_parts[part_index];
        if (can_eat(p.mass, my_predicted_parts[part_index].pos, p.radius,
                    enemy.mass, enemy.pos, enemy.radius)) {
          change_score(50 * enemy.mass, "Eating enemy bonus");
          players_taken.insert(enemy_index);
        }
      }
    }

    for (auto part_index : alive_parts) {
#if DEBUG_FUTURE_OUTCOMES
      m_debug_lines.emplace_back();
      m_debug_lines.back()[0] = my_predicted_parts[part_index].pos;
#endif
      advance(my_predicted_parts[part_index],
              target - my_predicted_parts[part_index].pos, scan_precision,
              ctx->config);
#if DEBUG_FUTURE_OUTCOMES
      m_debug_lines.back()[1] = my_predicted_parts[part_index].pos;
#endif
    }

    for (auto it = alive_parts.begin(); it != alive_parts.end();) {
      bool do_continue = false;
      for (auto &enemy : m_predicted_enemies[iteration])
        if (can_eat(enemy.mass, ctx->my_parts[*it].mass * 0.95)) {
          auto eating_dist =
              eating_distance(enemy.radius, ctx->my_parts[*it].radius);
          auto dist = enemy.pos.distance_to(my_predicted_parts[*it].pos);
          if (dist < eating_dist * 2.0) {
            change_score(-(eating_dist * 2.0 - dist) * 100.0 *
                             ctx->my_parts[*it].mass,
                         "Being eaten penalty");
            it = alive_parts.erase(it); // what is eaten could never eat
            do_continue = true;
            break;
          }
        }
      if (do_continue)
        continue;
      ++it;
    }
  }
  change_score(std::uniform_real_distribution<double>(0, 100)(m_re), "Entropy");
#if DEBUG_FUTURE_OUTCOMES
  auto s = m_debug_line_colors.size();
  m_debug_line_colors.resize(m_debug_lines.size());
  using namespace std::string_literals;
  std::fill(m_debug_line_colors.begin() + s, m_debug_line_colors.end(),
            "#"s + to_hex(std::clamp(0, static_cast<int>(-score), 255)) +
                to_hex(std::clamp(0, static_cast<int>(score / 10.), 255)) +
                "00");
#endif
#if CUSTOM_DEBUG
  if (ctx->is_debug_tick()) {
    std::cout << "Score for ("s + std::to_string(target.x) + "," +
                     std::to_string(target.y) + ") = " + std::to_string(score) +
                     "\n";
    std::cout << "Details:\n";
    for (auto &[reason, amount] : reasons_stacked)
      std::cout << "reason:" << reason << " - " << amount << '\n';
  }
#endif
  return score;
}

bool MaxSpeedStrategy::is_splitting_dangerous() const {
  return false;
  // return ctx->config.inertia_factor < 4.0;
}

void MaxSpeedStrategy::calculate_predicted_enemies() {
  const auto scan_precision = get_scan_precision();
  m_predicted_enemies.front() = ctx->enemies;
  for (auto iteration : indices(m_predicted_enemies)) {
    if (iteration > 0)
      m_predicted_enemies[iteration] = m_predicted_enemies[iteration - 1];
    for (auto &enemy : m_predicted_enemies[iteration]) {
      advance(enemy, enemy.speed, scan_precision, ctx->config);
    }
  }
}

Response MaxSpeedStrategy::get_response_impl() {
#ifdef DEBUG_FUTURE_OUTCOMES
  m_debug_lines.clear();
  m_debug_line_colors.clear();
#endif
  m_debug.clear();

  Point best_target;
  double best_target_score = -std::numeric_limits<int>::min();

  double min_angle = 0;
  double max_angle = 2 * constant::pi;

#if DEBUG_FUSION
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j) {
      m_debug_lines.push_back(
          {f.pos + Point{(2 * i - 1) * r, (2 * j - 1) * r}, f.pos});
      m_debug_line_colors.emplace_back("black");
    }
#endif

#if DEBUG_MEMORIZED_ENEMIES
  for (auto &[id, enemy_vision] : ctx->enemy_by_id)
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j) {
        m_debug_lines.push_back(
            {enemy_vision.state.pos +
                 Point{(2 * i - 1) * enemy_vision.state.radius,
                       (2 * j - 1) * enemy_vision.state.radius},
             enemy_vision.state.pos});
        std::string s = "#";
        auto h = to_hex((ctx->tick - enemy_vision.tick) * 255 /
                        remember_enemies_tick_count);
        for (auto i : range(0, 3))
          s += h;
        m_debug_line_colors.emplace_back(s);
      }
#endif

  auto check_target = [&](const Point &target) {
    double score = calc_target_score(target);

    if (score > best_target_score) {
      best_target_score = score;
      best_target = target;
    }
  };
  calculate_predicted_enemies();
  calculate_fusions(m_predicted_enemies.front());

  for (auto angle_try : range(0, angle_partition_count)) {
    double angle =
        min_angle + (max_angle - min_angle) * angle_try / angle_partition_count;
    check_target(
        border_point_by_vector((Point(1., 0.) * Matrix::rotation(angle))));
  }
  check_target(ctx->my_center);
  auto r = Response{}.target(best_target);

  double max_enemy_mass = 0.0;
  if (!m_fusions.empty()) {
    auto get_mass = [](const auto &p) { return p.mass; };
    max_enemy_mass =
        std::max(max_enemy_mass,
                 0.9 * get_mass(*max_element_op(m_fusions.begin(),
                                                m_fusions.end(), get_mass)));
  }
  if (!ctx->enemies.empty())
    max_enemy_mass = std::max(max_enemy_mass, ctx->enemies.front().mass);

  for (auto [tick, enemy_id] : ctx->enemy_ids_seen_by_tick) {
    if (tick < ctx->tick - 50)
      continue;
    max_enemy_mass =
        std::max(ctx->enemy_vision_by_id[enemy_id].state.mass, max_enemy_mass);
  }

  r.debug(m_debug + "Best Score: " + std::to_string(best_target_score));
  if (!is_splitting_dangerous() &&
      ctx->my_parts.size() < ctx->config.max_fragments_cnt &&
      max_enemy_mass <
          ctx->my_parts.front().mass / 2. / constant::eating_mass_coeff)
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
  ctx->remove_enemies_older_than(remember_enemies_tick_count);
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
