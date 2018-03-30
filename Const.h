#pragma once

namespace constant
{
  inline double infinity = 1e200;
  inline double virus_danger_mass = 120.0;
  inline double eating_mass_coeff = 1.2; // mass should be eating_mass_coeff much larger to perform eating
  inline double eating_dist_coeff = 2./3.; // distance between eater and eatee should be eating_dist_coeff * eater radius
}
