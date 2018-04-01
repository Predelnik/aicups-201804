#pragma once

namespace constant
{
  constexpr double pi = 3.14159265358979323846;
  constexpr double infinity = 1e200;
  constexpr double eps = 1e-10;
  constexpr double virus_danger_mass = 120.0;
  constexpr double eating_mass_coeff = 1.2; // mass should be eating_mass_coeff much larger to perform eating
  constexpr double interaction_dist_coeff = 2./3.; // distance between eater and eatee should be eating_dist_coeff * eater radius
  constexpr int food_spawn_delay = 40; // each 40 ticks food will spawn in  4 places on the map
}
