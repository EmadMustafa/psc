
#include <mrc_fld_as_double.h>

#define ggcm_mhd_step_mhdcc_ops ggcm_mhd_step_mhdcc_double_ops
#define ggcm_mhd_step_mhdcc_name "mhdcc_double"

#include "ggcm_mhd_step_private.h"
#include "ggcm_mhd_private.h"
#include "ggcm_mhd_defs.h"
#include "ggcm_mhd_crds.h"
#include "ggcm_mhd_diag_private.h"
#include "mhd_util.h"

#include <mrc_domain.h>
#include <mrc_profile.h>
#include <mrc_io.h>

#include <math.h>
#include <string.h>

#include "pde/pde_defs.h"

// pde options

#define OPT_FLD1D       OPT_FLD1D_C_ARRAY
#define OPT_FLD1D_VEC   OPT_FLD1D_PTR_ARRAY
#define OPT_FLD1D_STATE OPT_FLD1D_PTR_ARRAY

// mhd options

#define OPT_EQN OPT_EQN_MHD_FCONS
#define OPT_BACKGROUND false

#include "pde/pde_setup.c"
#include "pde/pde_mhd_setup.c"
#include "pde/pde_mhd_line.c"
#include "pde/pde_mhd_convert.c"
#include "pde/pde_mhd_reconstruct.c"
#include "pde/pde_mhd_current.c"
#include "pde/pde_mhd_divb_glm.c"
#include "pde/pde_mhd_riemann.c"
#include "pde/pde_mhd_resistive.c"
#include "pde/pde_mhd_stage.c"
#include "pde/pde_mhd_get_dt.c"

#include "mhd_3d.c"
#include "mhd_sc.c"

// ======================================================================
// ggcm_mhd_step subclass "mhdcc"

struct ggcm_mhd_step_mhdcc {
  struct mhd_options opt;

  fld1d_state_t U;
  fld1d_state_t U_l;
  fld1d_state_t U_r;
  fld1d_state_t W;
  fld1d_state_t W_l;
  fld1d_state_t W_r;
  fld1d_state_t F;

  struct mrc_fld *x_star;
};

#define ggcm_mhd_step_mhdcc(step) mrc_to_subobj(step, struct ggcm_mhd_step_mhdcc)

// TODO:
// - handle various resistivity models

// ----------------------------------------------------------------------
// ggcm_mhd_step_mhdcc_setup

static void
ggcm_mhd_step_mhdcc_setup(struct ggcm_mhd_step *step)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  struct ggcm_mhd *mhd = step->mhd;

  assert(mhd);

  pde_setup(mhd->fld);
  pde_mhd_setup(mhd);

  fld1d_state_setup(&sub->U);
  fld1d_state_setup(&sub->U_l);
  fld1d_state_setup(&sub->U_r);
  fld1d_state_setup(&sub->W);
  fld1d_state_setup(&sub->W_l);
  fld1d_state_setup(&sub->W_r);
  fld1d_state_setup(&sub->F);
  pde_mhd_aux_setup();

  mhd->ymask = ggcm_mhd_get_3d_fld(mhd, 1);
  mrc_fld_set(mhd->ymask, 1.);

  mhd->bnd_mask = ggcm_mhd_get_3d_fld(mhd, 1);

  sub->x_star = ggcm_mhd_get_3d_fld(mhd, s_n_comps);
  mrc_fld_dict_add_int(sub->x_star, "mhd_type", MT_FULLY_CONSERVATIVE_CC);

  ggcm_mhd_step_setup_member_objs_sub(step);
  ggcm_mhd_step_setup_super(step);
}

// ----------------------------------------------------------------------
// ggcm_mhd_step_mhdcc_destroy

static void
ggcm_mhd_step_mhdcc_destroy(struct ggcm_mhd_step *step)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  struct ggcm_mhd *mhd = step->mhd;

  ggcm_mhd_put_3d_fld(mhd, mhd->ymask);
  ggcm_mhd_put_3d_fld(mhd, mhd->bnd_mask);
  ggcm_mhd_put_3d_fld(mhd, sub->x_star);
}

// ----------------------------------------------------------------------
// ggcm_mhd_step_mhdcc_setup_flds

static void
ggcm_mhd_step_mhdcc_setup_flds(struct ggcm_mhd_step *step)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  struct ggcm_mhd *mhd = step->mhd;

  pde_mhd_set_options(mhd, &sub->opt);

  int n_comps = 8;
  if (s_opt_divb == OPT_DIVB_GLM) {
    n_comps++;
  }

  mrc_fld_set_type(mhd->fld, FLD_TYPE);
  mrc_fld_set_param_int(mhd->fld, "nr_ghosts", 2);
  mrc_fld_dict_add_int(mhd->fld, "mhd_type", MT_FULLY_CONSERVATIVE_CC);
  mrc_fld_set_param_int(mhd->fld, "nr_comps", n_comps);
}

// ----------------------------------------------------------------------
// mhd_flux_pt1

static void
mhd_flux_pt1(struct ggcm_mhd_step *step, struct mrc_fld *x,
	     int j, int k, int dir, int p, int ib, int ie)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  fld1d_state_t U = sub->U, U_l = sub->U_l, U_r = sub->U_r;
  fld1d_state_t W = sub->W, W_l = sub->W_l, W_r = sub->W_r;

  // FIXME: +2,+2 is specifically for PLM reconstr (and enough for PCM)
  mhd_get_line_state(U, x, j, k, dir, p, ib - 2, ie + 2);
  mhd_get_line_1(s_aux.bnd_mask, step->mhd->bnd_mask, j, k, dir, p, ib - 2, ie + 2);
  mhd_prim_from_cons(W, U, ib - 2, ie + 2);
  mhd_reconstruct(U_l, U_r, W_l, W_r, W, (fld1d_t) {}, ib, ie + 1);
}

// ----------------------------------------------------------------------
// mhd_flux_pt2

static void
mhd_flux_pt2(struct ggcm_mhd_step *step, struct mrc_fld *fluxes[3], struct mrc_fld *x,
	     int j, int k, int dir, int p, int ib, int ie)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  fld1d_state_t U_l = sub->U_l, U_r = sub->U_r;
  fld1d_state_t W = sub->W, W_l = sub->W_l, W_r = sub->W_r, F = sub->F;

  mhd_line_get_current(x, j, k, dir, p, ib, ie + 1);
  mhd_riemann(F, U_l, U_r, W_l, W_r, ib, ie + 1);
  mhd_add_resistive_flux(F, W, ib, ie + 1);
  mhd_put_line_state(fluxes[dir], F, j, k, dir, p, ib, ie + 1);
}

// ----------------------------------------------------------------------
// pushstage_c

static void
pushstage_c(struct ggcm_mhd_step *step, mrc_fld_data_t dt, mrc_fld_data_t time_curr,
	    struct mrc_fld *x_curr, struct mrc_fld *x_next)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  struct ggcm_mhd *mhd = step->mhd;
  struct mrc_fld *fluxes[3] = { ggcm_mhd_get_3d_fld(mhd, s_n_comps),
				ggcm_mhd_get_3d_fld(mhd, s_n_comps),
				ggcm_mhd_get_3d_fld(mhd, s_n_comps), };
  ggcm_mhd_fill_ghosts(mhd, x_curr, 0, time_curr);

  if (s_opt_bc_reconstruct) {
    struct mrc_fld *U_l[3] = { ggcm_mhd_get_3d_fld(mhd, s_n_comps),
			       ggcm_mhd_get_3d_fld(mhd, s_n_comps),
			       ggcm_mhd_get_3d_fld(mhd, s_n_comps), };
    struct mrc_fld *U_r[3] = { ggcm_mhd_get_3d_fld(mhd, s_n_comps),
			       ggcm_mhd_get_3d_fld(mhd, s_n_comps),
			       ggcm_mhd_get_3d_fld(mhd, s_n_comps), };
    
    // reconstruct
    for (int p = 0; p < mrc_fld_nr_patches(x_curr); p++) {
      pde_for_each_dir(dir) {
	pde_for_each_line(dir, j, k, 0) {
	  int ib = 0, ie = s_ldims[dir];
	  mhd_flux_pt1(step, x_curr, j, k, dir, p, ib, ie);
	  mhd_put_line_state(U_l[dir], sub->U_l, j, k, dir, p, ib, ie + 1);
	  mhd_put_line_state(U_r[dir], sub->U_r, j, k, dir, p, ib, ie + 1);
	}
      }
    }
    
    ggcm_mhd_fill_ghosts_reconstr(mhd, U_l, U_r);
    
    // riemann solve
    for (int p = 0; p < mrc_fld_nr_patches(x_curr); p++) {
      pde_for_each_dir(dir) {
	pde_for_each_line(dir, j, k, 0) {
	  int ib = 0, ie = s_ldims[dir];
	  mhd_get_line_state(sub->U_l, U_l[dir], j, k, dir, p, ib, ie + 1);
	  mhd_get_line_state(sub->U_r, U_r[dir], j, k, dir, p, ib, ie + 1);
	  mhd_prim_from_cons(sub->W_l, sub->U_l, ib, ie + 1);
	  mhd_prim_from_cons(sub->W_r, sub->U_r, ib, ie + 1);
	  
	  mhd_flux_pt2(step, fluxes, x_curr, j, k, dir, p, 0, s_ldims[dir]);
	}
      }
    } 

    ggcm_mhd_put_3d_fld(mhd, U_l[0]);
    ggcm_mhd_put_3d_fld(mhd, U_l[1]);
    ggcm_mhd_put_3d_fld(mhd, U_l[2]);
    ggcm_mhd_put_3d_fld(mhd, U_r[0]);
    ggcm_mhd_put_3d_fld(mhd, U_r[1]);
    ggcm_mhd_put_3d_fld(mhd, U_r[2]);
  } else {
    for (int p = 0; p < mrc_fld_nr_patches(x_curr); p++) {
      pde_for_each_dir(dir) {
	pde_for_each_line(dir, j, k, 0) {
	  int ib = 0, ie = s_ldims[dir];
	  mhd_flux_pt1(step, x_curr, j, k, dir, p, ib, ie);
	  mhd_flux_pt2(step, fluxes, x_curr, j, k, dir, p, ib, ie);
	}
      }
    }
  }
    
  mhd_update_finite_volume(mhd, x_next, fluxes, mhd->ymask, dt, true, 0, 0);

  ggcm_mhd_put_3d_fld(mhd, fluxes[0]);
  ggcm_mhd_put_3d_fld(mhd, fluxes[1]);
  ggcm_mhd_put_3d_fld(mhd, fluxes[2]);
}

// ----------------------------------------------------------------------
// ggcm_mhd_step_mhdcc_get_dt

static double
ggcm_mhd_step_mhdcc_get_dt(struct ggcm_mhd_step *step, struct mrc_fld *x)
{
  return pde_mhd_get_dt(step->mhd, x);
}

// ----------------------------------------------------------------------
// ggcm_mhd_step_euler

static void
ggcm_mhd_step_euler(struct ggcm_mhd_step *step, struct mrc_fld *x, double dt)
{
  struct ggcm_mhd *mhd = step->mhd;

  static int pr_A;
  if (!pr_A) {
    pr_A = prof_register("mhd_step_euler", 0, 0, 0);
  }

  prof_start(pr_A);
  pushstage_c(step, dt, mhd->time, x, x);
  prof_stop(pr_A);
}

// ----------------------------------------------------------------------
// ggcm_mhd_step_predcorr

static void
ggcm_mhd_step_predcorr(struct ggcm_mhd_step *step, struct mrc_fld *x, double dt)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  struct ggcm_mhd *mhd = step->mhd;
  struct mrc_fld *x_star = sub->x_star;

  static int pr_A, pr_B;
  if (!pr_A) {
    pr_A = prof_register("mhd_step_pred", 0, 0, 0);
    pr_B = prof_register("mhd_step_corr", 0, 0, 0);
  }

  // --- PREDICTOR
  prof_start(pr_A);
  // set x* = x^n, then advance to n+1/2
  mrc_fld_copy(x_star, x);
  pushstage_c(step, .5f * dt, mhd->time, x, x_star);
  // now x^* = x^n + .5 * dt rhs(x^n)
  prof_stop(pr_A);

  // --- CORRECTOR
  prof_start(pr_B);
  // x^{n+1} = x^n + dt rhs(x_star)
  pushstage_c(step, dt, mhd->time + .5f * mhd->bndt, x_star, x);
  prof_stop(pr_B);
}

// ----------------------------------------------------------------------
// ggcm_mhd_step_tvd_rk2

static void
ggcm_mhd_step_tvd_rk2(struct ggcm_mhd_step *step, struct mrc_fld *x, double dt)
{
  struct ggcm_mhd_step_mhdcc *sub = ggcm_mhd_step_mhdcc(step);
  struct ggcm_mhd *mhd = step->mhd;
  struct mrc_fld *x_star = sub->x_star;
  
  // set x* = x^n
  mrc_fld_copy(x_star, x);

  // stage 1
  // advance x*
  pushstage_c(step, dt, mhd->time, x_star, x_star);
  // now x* = x^n + dt rhs(x^n)
  
  // stage 2
  // advance x* again (now called x**)
  pushstage_c(step, dt, mhd->time + mhd->bndt, x_star, x_star);
  // now x** = x* + dt rhs(x*)
  
  // finally advance x^{n+1} = .5 * x** + .5 * x^n;
  mrc_fld_axpby(x, .5, x_star, .5);
}


// ----------------------------------------------------------------------
// ggcm_mhd_step_mhdcc_run

static void
ggcm_mhd_step_mhdcc_run(struct ggcm_mhd_step *step, struct mrc_fld *x)
{
  struct ggcm_mhd *mhd = step->mhd;

  mhd_divb_glm_source(x, .5f * mhd->dt);

  if (s_opt_time_integrator == OPT_TIME_INTEGRATOR_EULER) {
    ggcm_mhd_step_euler(step, x, mhd->dt);
  } else if (s_opt_time_integrator == OPT_TIME_INTEGRATOR_PREDCORR) {
    ggcm_mhd_step_predcorr(step, x, mhd->dt);
  } else if (s_opt_time_integrator == OPT_TIME_INTEGRATOR_TVD_RK2) {
    ggcm_mhd_step_tvd_rk2(step, x, mhd->dt);
  } else {
    assert(0);
  }

  mhd_divb_glm_source(x, .5f * mhd->dt);
}

// ----------------------------------------------------------------------
// subclass description

#define VAR(x) (void *)offsetof(struct ggcm_mhd_step_mhdcc, x)
static struct param ggcm_mhd_step_mhdcc_descr[] = {
  { "eqn"                , VAR(opt.eqn)            , PARAM_SELECT(OPT_EQN,
								  opt_eqn_descr)                },
  { "limiter"            , VAR(opt.limiter)        , PARAM_SELECT(OPT_LIMITER_FLAT,
								  opt_limiter_descr)            },
  { "resistivity"        , VAR(opt.resistivity)    , PARAM_SELECT(OPT_RESISTIVITY_NONE,
								  opt_resistivity_descr)        },
  { "hall"               , VAR(opt.hall)           , PARAM_SELECT(OPT_HALL_NONE,
								  opt_hall_descr)               },
  { "divb"               , VAR(opt.divb)           , PARAM_SELECT(OPT_DIVB_NONE,
								  opt_divb_descr)               },
  { "riemann"            , VAR(opt.riemann)        , PARAM_SELECT(OPT_RIEMANN_RUSANOV,
								  opt_riemann_descr)            },
  { "time_integrator"    , VAR(opt.time_integrator), PARAM_SELECT(OPT_TIME_INTEGRATOR_PREDCORR,
								  opt_time_integrator_descr)    },
  { "get_dt"             , VAR(opt.get_dt)         , PARAM_SELECT(OPT_GET_DT_MHD,
								  opt_get_dt_descr)             },
  { "background"         , VAR(opt.background)     , PARAM_BOOL(false)                          },
  { "bc_reconstruct"     , VAR(opt.bc_reconstruct) , PARAM_BOOL(false)                          },
  { "limiter_mc_beta"    , VAR(opt.limiter_mc_beta), PARAM_DOUBLE(2.)                           },
  { "divb_glm_alpha"     , VAR(opt.divb_glm_alpha) , PARAM_DOUBLE(.1)                           },
  { "divb_glm_ch_fac"    , VAR(opt.divb_glm_ch_fac), PARAM_DOUBLE(1.)                           },

  {},
};
#undef VAR

// ----------------------------------------------------------------------
// ggcm_mhd_step subclass "mhdcc_*"

struct ggcm_mhd_step_ops ggcm_mhd_step_mhdcc_ops = {
  .name                = ggcm_mhd_step_mhdcc_name,
  .size                = sizeof(struct ggcm_mhd_step_mhdcc),
  .param_descr         = ggcm_mhd_step_mhdcc_descr,
  .setup               = ggcm_mhd_step_mhdcc_setup,
  .get_dt              = ggcm_mhd_step_mhdcc_get_dt,
  .run                 = ggcm_mhd_step_mhdcc_run,
  .destroy             = ggcm_mhd_step_mhdcc_destroy,
  .setup_flds          = ggcm_mhd_step_mhdcc_setup_flds,
};
