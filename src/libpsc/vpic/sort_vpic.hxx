
#pragma once

#include "vpic_iface.h"

template<typename ParticlesOps, typename Mparticles>
struct SortVpic_
{
  void operator()(Mparticles& mprts)
  {
    auto step = mprts.grid().timestep();
    // Sort the particles for performance if desired.
    
    for (auto& sp : mprts) {
      if (sp.sort_interval > 0 && (step % sp.sort_interval) == 0) {
	mpi_printf(MPI_COMM_WORLD, "Performance sorting \"%s\"\n", sp.name);
	TIC ParticlesOps::sort_p(&sp); TOC(sort_p, 1);
      }
    }
  }
};

template<typename Mparticles>
using SortVpic = SortVpic_<PscSortOps<Mparticles>, Mparticles>;

template<typename Mparticles>
struct SortVpicWrap
{
  void operator()(Mparticles& mprts)
  {
    auto step = mprts.grid().timestep();
    // Sort the particles for performance if desired.
    
    for (auto& sp : mprts) {
      if (sp.sort_interval > 0 && (step % sp.sort_interval) == 0) {
	mpi_printf(MPI_COMM_WORLD, "Performance sorting \"%s\"\n", sp.name);
	TIC ::sort_p(&sp); TOC(sort_p, 1);
      }
    }
  }
};

