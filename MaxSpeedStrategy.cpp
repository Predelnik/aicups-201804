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

#include "range.hpp"
#include <queue>
using namespace util::lang;

#ifdef CUSTOM_DEBUG
#define DEBUG_FUTURE_OUTCOMES 0
#define DEBUG_FUSION 0
#define DEBUG_MEMORIZED_ENEMIES 0
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

void MaxSpeedStrategy::calculate_fusions(
    const std::vector<KnownPlayer> &enemies) {
  std::vector<bool> used(enemies.size());
  for (auto enemy_index : indices(enemies)) {
    if (used[enemy_index])
      continue;
    if (enemies[enemy_index].ticks_to_fuse > 50)
      continue;

    m_fusions.clear();
    m_fusions.push_back(enemies[enemy_index]);
    m_fusions.back().pos *= m_fusions.back().mass;
    used[enemy_index] = true;
    int cnt = 1;
    bool fusion_happened = true;
    while (fusion_happened) {
      fusion_happened = false;
      for (auto ind : indices(enemies)) {
        if (enemies[ind].id.player_num != m_fusions.back().id.player_num)
          continue;
        if (used[ind])
          continue;
        if ((m_fusions.back().pos / m_fusions.back().mass)
                .squared_distance_to(enemies[ind].pos) >
            pow(enemies[ind].radius + radius_by_mass(m_fusions.back().mass), 2))
          continue;

        if (enemies[enemy_index].ticks_to_fuse > 50)
          continue;

        ++cnt;
        fusion_happened = true;
        used[ind] = true;
        m_fusions.back().pos += enemies[ind].pos * enemies[ind].mass;
        m_fusions.back().mass += enemies[ind].mass;
      }
    }

    if (cnt == 1)
      m_fusions.pop_back();
    else
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

  double score = 0;
  static std::vector<KnownPlayer> my_predicted_parts;
  my_predicted_parts = ctx->my_parts;
  std::set<size_t> alive_parts;
  for (auto my_part_index : indices(ctx->my_parts))
    alive_parts.insert(my_part_index);

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
      score -= ((speed_loss - speed_loss_limit) * 25000.0);

    score += 500. *
             distance_to_nearest_wall(my_predicted_parts[part_index].pos,
                                      ctx->config) /
             (std::min(ctx->config.game_width, ctx->config.game_height) / 2.) /
             ctx->my_parts.size();
    for (auto &f : m_fusions) {
      if (can_eat(f.mass, ctx->my_parts[part_index].mass * 0.95)) {
        auto r = radius_by_mass(f.mass);
        auto eating_dist = eating_distance(r, ctx->my_parts[part_index].radius);
        auto dist = f.pos.distance_to(my_predicted_parts[part_index].pos);
        if (dist < 3 * eating_dist) {
          score -= (3 * eating_dist - dist) * 4000;
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
    score -= (deviation - 100.0) * 1000.0;
  }
  std::unordered_set<size_t> food_taken, ejection_taken, virus_bumped;

  for (auto part_index : alive_parts) {
    auto ticks = static_cast<int>(
        200.0 / max_speed(ctx->my_parts[part_index].mass, ctx->config) /
        sqrt(ctx->config.inertia_factor));
    auto mp = ctx->my_parts[part_index];
    advance(mp, target - my_predicted_parts[part_index].pos, ticks,
            ctx->config);

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
    for (auto enemy_index : indices(m_predicted_enemies[iteration])) {
      if (players_taken.count(enemy_index))
        continue;
      for (auto part_index : alive_parts) {
        auto &enemy = m_predicted_enemies[iteration][enemy_index];
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
            it = alive_parts.erase(it); // what is eaten could never eat
            score -= (eating_dist * 2.0 - dist) * 5000.0;
            do_continue = true;
            break;
          }
        }
      if (do_continue)
        continue;
      ++it;
    }
  }
  score += std::uniform_real_distribution<double>(0, 100)(m_re);
#if DEBUG_FUTURE_OUTCOMES
  auto s = m_debug_line_colors.size();
  m_debug_line_colors.resize(m_debug_lines.size());
  using namespace std::string_literals;
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
  Point best_target;
  double best_angle_score = -std::numeric_limits<int>::min();

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
        auto h = to_hex ((ctx->tick - enemy_vision.tick) * 255 / 200);
        for (auto i : range (0, 3))
            s += h;
        m_debug_line_colors.emplace_back(s);
      }
#endif

  auto check_target = [&](const Point &target) {
    double score = calc_target_score(target);

    if (score > best_angle_score) {
      best_angle_score = score;
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

  for (auto it = ctx->enemy_seen_by_tick.begin (); it != ctx->enemy_seen_by_tick.end (); ++it)
  {
    if (it->first < ctx->tick - 50)
        break;
    max_enemy_mass = std::max (ctx->enemy_by_id[it->second].state.mass, max_enemy_mass);
  }

  r.debug("Score: " + std::to_string(best_angle_score));
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
  ctx->remove_enemies_older_than(200);
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
