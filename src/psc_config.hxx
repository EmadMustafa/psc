
#pragma once

#include "psc_particles_single.h"
#include "psc_particles_double.h"
#include "psc_fields_c.h"

#include "../libpsc/psc_sort/psc_sort_impl.hxx"
#include "../libpsc/psc_collision/psc_collision_impl.hxx"
#include "../libpsc/psc_push_particles/1vb/psc_push_particles_1vb.h"
#include "psc_push_fields_impl.hxx"
#include "../libpsc/psc_bnd/psc_bnd_impl.hxx"
#include "../libpsc/psc_bnd_fields/psc_bnd_fields_impl.hxx"
#include "bnd_particles_impl.hxx"
#include "../libpsc/psc_balance/psc_balance_impl.hxx"
#include "../libpsc/psc_push_fields/marder_impl.hxx"
#include "../libpsc/psc_output_particles/output_particles_hdf5_impl.hxx"
#include "../libpsc/psc_output_particles/output_particles_none_impl.hxx"
#include "../libpsc/psc_output_fields/fields_item_moments_1st.hxx"
#include "../libpsc/psc_output_fields/fields_item_jeh.hxx"

#ifdef USE_CUDA
#include "../libpsc/cuda/push_particles_cuda_impl.hxx"
#include "../libpsc/cuda/push_fields_cuda_impl.hxx"
#include "../libpsc/cuda/bnd_cuda_impl.hxx"
#include "../libpsc/cuda/bnd_cuda_2_impl.hxx"
#include "../libpsc/cuda/bnd_cuda_3_impl.hxx"
#include "../libpsc/cuda/bnd_particles_cuda_impl.hxx"
#include "../libpsc/cuda/sort_cuda_impl.hxx"
#include "../libpsc/cuda/collision_cuda_impl.hxx"
#include "../libpsc/cuda/checks_cuda_impl.hxx"
#include "../libpsc/cuda/marder_cuda_impl.hxx"
#endif

#include "../libpsc/vpic/fields_item_vpic.hxx"

template<typename Mparticles>
using OutputParticlesDefault = OutputParticlesHdf5<Mparticles>;

struct SimulationNone
{
  using Species = void;
};

template<typename _Dim, typename Mparticles, typename MfieldsState>
struct PscConfigPushParticles2nd
{
  using PushParticles = PushParticlesEsirkepov<Config2nd<Mparticles, MfieldsState, _Dim>>;
};

template<typename _Dim, typename Mparticles, typename MfieldsState>
struct PscConfigPushParticles1vbec
{
  using PushParticles = PushParticlesVb<Config1vbec<Mparticles, MfieldsState, _Dim>>;
};

template<typename _Dim, typename Mparticles, typename MfieldsState>
struct PscConfigPushParticlesCuda
{
};

// need to use Config1vbecSplit when for dim_xyz, dim_xz

template<typename Mparticles, typename Mfields>
struct PscConfigPushParticles1vbec<dim_xyz, Mparticles, Mfields>
{
  using PushParticles = PushParticlesVb<Config1vbecSplit<Mparticles, Mfields, dim_xyz>>;
};

template<typename Mparticles, typename Mfields>
struct PscConfigPushParticles1vbec<dim_xz, Mparticles, Mfields>
{
  using PushParticles = PushParticlesVb<Config1vbecSplit<Mparticles, Mfields, dim_xz>>;
};

template<typename _Dim, typename _Mparticles, typename _MfieldsState,
	 typename _Mfields,
	 template<typename...> class ConfigPushParticles,
	 typename _Simulation = SimulationNone>
struct PscConfig_
{
  using Dim = _Dim;
  using Mparticles = _Mparticles;
  using MfieldsState = _MfieldsState;
  using Mfields = _Mfields;
  using ConfigPushp = ConfigPushParticles<_Dim, Mparticles, MfieldsState>;
  using PushParticles = typename ConfigPushp::PushParticles;
  using checks_order = typename PushParticles::checks_order;
  using Sort = SortCountsort2<Mparticles>;
  using Collision = Collision_<Mparticles, MfieldsState, Mfields>;
  using PushFields = ::PushFields<MfieldsState>;
  using BndParticles = BndParticles_<Mparticles>;
  using Bnd = Bnd_<MfieldsState>;
  using BndFields = BndFieldsNone<MfieldsState>;
  using Balance = Balance_<Mparticles, MfieldsState, Mfields>;
  using Checks = Checks_<Mparticles, MfieldsState, Mfields, checks_order>;
  using Marder = Marder_<Mparticles, MfieldsState, Mfields>;
  using Simulation = _Simulation;
  using OutputParticles = OutputParticlesDefault<Mparticles>;
};

#ifdef USE_CUDA

template<typename _Dim, typename _Mparticles, typename _MfieldsState, typename _Mfields>
struct PscConfig_<_Dim, _Mparticles, _MfieldsState, _Mfields, PscConfigPushParticlesCuda>
{
  using Dim = _Dim;
  using BS = typename _Mparticles::BS;
  using Mparticles = _Mparticles;
  using MfieldsState = _MfieldsState;
  using Mfields = _Mfields;
  using PushParticles = PushParticlesCuda<CudaConfig1vbec3d<Dim, BS>>;
  using Sort = SortCuda<BS>;
  using Collision = CollisionCuda<Mparticles>;
  using PushFields = PushFieldsCuda;
  using BndParticles = BndParticlesCuda<Mparticles, Dim>;
  using Bnd = BndCuda3<MfieldsState>;
  using BndFields = BndFieldsNone<MfieldsState>;
  using Balance = Balance_<MparticlesSingle, MfieldsStateSingle, MfieldsSingle>;
  using Checks = ChecksCuda<Mparticles>;
  using Marder = MarderCuda<BS>;
  using OutputParticles = OutputParticlesDefault<MparticlesSingle>;
};

template<typename _Mparticles, typename _MfieldsState, typename _Mfields>
struct PscConfig_<dim_xyz, _Mparticles, _MfieldsState, _Mfields, PscConfigPushParticlesCuda>
{
  using Dim = dim_xyz;
  using BS = typename _Mparticles::BS;
  using Mparticles = _Mparticles;
  using MfieldsState = _MfieldsState;
  using Mfields = _Mfields;
  using PushParticles = PushParticlesCuda<CudaConfig1vbec3dGmem<Dim, BS>>;
  using Sort = SortCuda<BS>;
  using Collision = CollisionCuda<Mparticles>;
  using PushFields = PushFieldsCuda;
  using BndParticles = BndParticlesCuda<Mparticles, Dim>;
  using Bnd_t = BndCuda3<MfieldsState>;
  using BndFields_t = BndFieldsNone<MfieldsState>;
  using Balance = Balance_<MparticlesSingle, MfieldsStateSingle, MfieldsSingle>;
  using Checks = ChecksCuda<Mparticles>;
  using Marder = MarderCuda<BS>;
  using OutputParticles = OutputParticlesNone;
};

#endif


template<typename dim>
using PscConfig2ndDouble = PscConfig_<dim, MparticlesDouble, MfieldsStateDouble, MfieldsC,
				      PscConfigPushParticles2nd>;

template<typename dim>
using PscConfig2ndSingle = PscConfig_<dim, MparticlesSingle, MfieldsStateSingle, MfieldsSingle,
				      PscConfigPushParticles2nd>;

template<typename dim>
using PscConfig1vbecSingle = PscConfig_<dim, MparticlesSingle, MfieldsStateSingle, MfieldsSingle,
					PscConfigPushParticles1vbec>;

template<typename dim>
using PscConfig1vbecDouble = PscConfig_<dim, MparticlesDouble, MfieldsStateDouble, MfieldsC,
					PscConfigPushParticles1vbec>;

#ifdef USE_CUDA

template<typename dim>
struct PscConfig1vbecCuda : PscConfig_<dim, MparticlesCuda<BS144>, MfieldsStateCuda, MfieldsCuda, PscConfigPushParticlesCuda>
{};

template<>
struct PscConfig1vbecCuda<dim_xyz> : PscConfig_<dim_xyz, MparticlesCuda<BS444>, MfieldsStateCuda, MfieldsCuda, PscConfigPushParticlesCuda>
{};

#endif

#include "../libpsc/vpic/sort_vpic.hxx"
#include "../libpsc/vpic/collision_vpic.hxx"
#include "../libpsc/vpic/push_particles_vpic.hxx"
#include "../libpsc/vpic/push_fields_vpic.hxx"
#include "../libpsc/vpic/bnd_vpic.hxx"
#include "../libpsc/vpic/bnd_fields_vpic.hxx"
#include "../libpsc/vpic/bnd_particles_vpic.hxx"
#include "../libpsc/vpic/marder_vpic.hxx"
#include "../libpsc/vpic/checks_vpic.hxx"

#ifdef USE_VPIC
struct PscConfigVpicWrap
{
  using VpicConfig = VpicConfigWrap;
  
  using MfieldsState = typename VpicConfig::MfieldsState;
  using Mparticles = typename VpicConfig::Mparticles;
  using MfieldsHydro = typename VpicConfig::MfieldsHydro;

  using Balance = Balance_<MparticlesSingle, MfieldsStateSingle, MfieldsSingle>;
  using Sort = SortVpicWrap<Mparticles>;
  using Collision = PscCollisionVpic;
  using PushParticles = PushParticlesVpic<Mparticles, MfieldsState,
					    typename VpicConfig::ParticlesOps,
					    typename VpicConfig::AccumulatorOps,
					    typename VpicConfig::AccumulateOps,
					    typename VpicConfig::InterpolatorOps>;
  using PushFields = PushFieldsVpicWrap<MfieldsState>;
  using Bnd = BndVpic<MfieldsState>;
  using BndFields = BndFieldsVpic<MfieldsState>;
  using BndParticles = BndParticlesVpic<Mparticles>;
  using Checks = ChecksVpic<Mparticles, MfieldsState>;
  using Marder = MarderVpicWrap<Mparticles, MfieldsState>;
  using OutputParticles = OutputParticlesHdf5<MparticlesSingle>;
  using OutputHydro = OutputHydroVpicWrap<Mparticles, MfieldsHydro, typename VpicConfig::MfieldsInterpolator>;
  using Dim = dim_xyz;

#if 0
  using DiagMixin = VpicDiagMixin<Mparticles, MfieldsState, MfieldsInterpolator, MfieldsHydro,
				  DiagOps, ParticlesOps, HydroArrayOps>;
#else
  using DiagMixin = NoneDiagMixin<Mparticles, MfieldsState,
				  typename VpicConfig::MfieldsInterpolator,
				  typename VpicConfig::MfieldsHydro>;
#endif
};
#endif

struct PscConfigVpicPsc
{
  using VpicConfig = VpicConfigPsc;

  using MfieldsState = typename VpicConfig::MfieldsState;
  using Mparticles = typename VpicConfig::Mparticles;
  using MfieldsHydro = typename VpicConfig::MfieldsHydro;
  
  using Balance = Balance_<MparticlesSingle, MfieldsStateSingle, MfieldsSingle>;
  using Sort = SortVpic<Mparticles>;
  using Collision = PscCollisionVpic;
  using PushParticles = PushParticlesVpic<Mparticles, MfieldsState,
					    typename VpicConfig::ParticlesOps,
					    typename VpicConfig::AccumulatorOps,
					    typename VpicConfig::AccumulateOps,
					    typename VpicConfig::InterpolatorOps>;
  using PushFields = PushFieldsVpic<MfieldsState>;
  using Bnd = BndVpic<MfieldsState>;
  using BndFields = BndFieldsVpic<MfieldsState>;
  using BndParticles = BndParticlesVpic<Mparticles>;
  using Checks = ChecksVpic<Mparticles, MfieldsState>;
  using Marder = MarderVpic<Mparticles, MfieldsState>;
  using OutputParticles = OutputParticlesHdf5<MparticlesSingle>;
  using OutputHydro = OutputHydroVpic<Mparticles, MfieldsHydro, typename VpicConfig::MfieldsInterpolator>;
  using Dim = dim_xyz;

#if 0
  using DiagMixin = VpicDiagMixin<Mparticles, MfieldsState, MfieldsInterpolator, MfieldsHydro,
				  DiagOps, ParticlesOps, HydroArrayOps>;
#else
  using DiagMixin = NoneDiagMixin<Mparticles, MfieldsState,
				  typename VpicConfig::MfieldsInterpolator,
				  typename VpicConfig::MfieldsHydro>;
#endif
};

using FieldsItem_E_cc = FieldsItemFields<ItemLoopPatches<Item_e_cc>>;
using FieldsItem_H_cc = FieldsItemFields<ItemLoopPatches<Item_h_cc>>;
using FieldsItem_J_cc = FieldsItemFields<ItemLoopPatches<Item_j_cc>>;

template <typename Mparticles>
using FieldsItem_n_1st_cc =
  FieldsItemMoment<ItemMomentAddBnd<Moment_n_1st<Mparticles, MfieldsC>>>;
template <typename Mparticles>
using FieldsItem_v_1st_cc =
  FieldsItemMoment<ItemMomentAddBnd<Moment_v_1st<Mparticles, MfieldsC>>>;
template <typename Mparticles>
using FieldsItem_p_1st_cc =
  FieldsItemMoment<ItemMomentAddBnd<Moment_p_1st<Mparticles, MfieldsC>>>;
template <typename Mparticles>
using FieldsItem_T_1st_cc =
  FieldsItemMoment<ItemMomentAddBnd<Moment_T_1st<Mparticles, MfieldsC>>>;

