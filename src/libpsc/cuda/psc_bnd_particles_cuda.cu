
#include "psc_bnd_particles_private.h"
#include "psc_particles_cuda.h"

#include "cuda_bndp_iface.h"

#include <mrc_profile.h>

// ======================================================================
// psc_bnd_particles: subclass "cuda"

struct psc_bnd_particles_ops_cuda : psc_bnd_particles_ops {
  using PscBndParticles_t = PscBndParticlesWrapper<psc_bnd_particles_cuda>;
  psc_bnd_particles_ops_cuda() {
    name                    = "cuda";
    size                    = sizeof(psc_bnd_particles_cuda);
    destroy                 = PscBndParticles_t::destroy;
    setup                   = PscBndParticles_t::setup;
  }
} psc_bnd_particles_cuda_ops;

