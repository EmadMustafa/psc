
#ifndef PSC_CHECKS_PRIVATE_H
#define PSC_CHECKS_PRIVATE_H

#include <psc_checks.h>

struct psc_checks {
  struct mrc_obj obj;

  // parameters
  int continuity_every_step;   // check charge continuity eqn every so many steps
  double continuity_threshold; // acceptable error in continuity eqn
  bool continuity_verbose;     // always print continuity error, even if acceptable

  int gauss_every_step;   // check Gauss's Law every so many steps
  double gauss_threshold; // acceptable error in Gauss's Law
  bool gauss_verbose;     // always print Gauss's Law error, even if acceptable
};

#endif