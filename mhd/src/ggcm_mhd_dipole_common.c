
#include "ggcm_mhd_dipole_private.h"

#include "ggcm_mhd_defs.h"
#include "ggcm_mhd_private.h"
#include "ggcm_mhd_crds.h"

#include <mrc_ddc.h>
#include <mrc_io.h>

#include <assert.h>
#include <string.h>
#include <math.h>

// ======================================================================
// ggcm_mhd_dipole_sub

// ----------------------------------------------------------------------
// ggcm_mhd_dipole_sub_vect_pot

static double
ggcm_mhd_dipole_sub_vect_pot_crd(struct ggcm_mhd *mhd, int m, double x[3],
				 float x0[3], float moment[3], float xmir)
{
  // find x_prime (x - x0), and r3i (1 / r**3)
  mrc_fld_data_t x_prime[3], r2 = 0.0;
  for (int j = 0; j < 3; j++) {
    x_prime[j] = x[j] - x0[j];
    r2 += sqr(x_prime[j]);
  }
  r2 = fmax(r2, .01f); // make sure r**2 >= 0.01
  mrc_fld_data_t r3i = powf(r2, -1.5f); // r3i = 1 / r**3

  // set A = 0 if we are sunward of xmir
  if (xmir != 0.0 && x[0] < xmir) {
    return 0.f;
  }

  // A = m x r / r**3
  switch (m) {
  case 0: return (moment[1] * x_prime[2] - moment[2] * x_prime[1]) * r3i;
  case 1: return (moment[2] * x_prime[0] - moment[0] * x_prime[2]) * r3i;
  case 2: return (moment[0] * x_prime[1] - moment[1] * x_prime[0]) * r3i;
  default: assert(0);
  }
}

// ----------------------------------------------------------------------
// ggcm_mhd_dipole_sub_vect_pot

static double
ggcm_mhd_dipole_sub_vect_pot(struct ggcm_mhd_dipole *mhd_dipole, int m,
			     int ix, int iy, int iz, int p,
			     float x0[3], float moment[3], float xmir)
{
  struct ggcm_mhd *mhd = mhd_dipole->mhd;

  int mhd_type;
  mrc_fld_get_param_int(mhd->fld, "mhd_type", &mhd_type);

  float crd[3];
  if (mhd_type == MT_PRIMITIVE_CC ||
      mhd_type == MT_FULLY_CONSERVATIVE_CC) { // cell-centered B
    ggcm_mhd_get_crds_cc(mhd, ix,iy,iz, p, crd);
  } else {
    ggcm_mhd_get_crds_ec(mhd, ix,iy,iz, p, m, crd);
  }
  double x[3] = { crd[0], crd[1], crd[2] };
  return ggcm_mhd_dipole_sub_vect_pot_crd(mhd, m, x, x0, moment, xmir);
}
 
// ----------------------------------------------------------------------
// ggcm_mhd_dipole_sub_add_dipole

static void
ggcm_mhd_dipole_sub_add_dipole(struct ggcm_mhd_dipole *mhd_dipole, struct mrc_fld *b_base,
			       float x0[3], float moment[3], float xmir,
			       float keep)
{
  struct ggcm_mhd *mhd = mhd_dipole->mhd;
  assert(mhd);

  int mhd_type;
  mrc_fld_get_param_int(mhd->fld, "mhd_type", &mhd_type);

  struct mrc_fld *a_base = ggcm_mhd_get_3d_fld(mhd, 3);
  struct mrc_fld *a = mrc_fld_get_as(a_base, FLD_TYPE);
  struct mrc_fld *b = mrc_fld_get_as(b_base, FLD_TYPE);

  mrc_fld_data_t curl_a[3];

  // calculate A first, then take its curl
  for (int p = 0; p < mrc_fld_nr_patches(a); p++) {
    mrc_fld_foreach(a, ix,iy,iz, 2, 2) {
      for (int m = 0; m < 3; m++) {
	M3(a, m, ix,iy,iz, p) = ggcm_mhd_dipole_sub_vect_pot(mhd_dipole, m, ix,iy,iz, p, x0, moment, xmir);
      }
    } mrc_fld_foreach_end;
  }
  if (mhd->amr > 0) {
    mrc_ddc_amr_apply(mhd->ddc_amr_E, a);
  }
  
  // B = keep * B + curl A

  if (mhd_type == MT_PRIMITIVE_CC ||
      mhd_type == MT_FULLY_CONSERVATIVE_CC) { // cell-centered B
    for (int p = 0; p < mrc_fld_nr_patches(b); p++) {
      float *fd1x = ggcm_mhd_crds_get_crd_p(mhd->crds, 0, FD1, p);
      float *fd1y = ggcm_mhd_crds_get_crd_p(mhd->crds, 1, FD1, p);
      float *fd1z = ggcm_mhd_crds_get_crd_p(mhd->crds, 2, FD1, p);
      
      mrc_fld_foreach(b, ix,iy,iz, 1, 1) {
	curl_a[0] = ((M3(a, 2, ix,iy+1,iz, p) - M3(a, 2, ix,iy-1,iz, p)) * .5f * fd1y[iy] -
		     (M3(a, 1, ix,iy,iz+1, p) - M3(a, 1, ix,iy,iz-1, p)) * .5f * fd1z[iz]);
	curl_a[1] = ((M3(a, 0, ix,iy,iz+1, p) - M3(a, 0, ix,iy,iz-1, p)) * .5f * fd1z[iz] -
		     (M3(a, 2, ix+1,iy,iz, p) - M3(a, 2, ix-1,iy,iz, p)) * .5f * fd1x[ix]);
	curl_a[2] = ((M3(a, 1, ix+1,iy,iz, p) - M3(a, 1, ix-1,iy,iz, p)) * .5f * fd1x[ix] -
		     (M3(a, 0, ix,iy+1,iz, p) - M3(a, 0, ix,iy-1,iz, p)) * .5f * fd1y[iy]);
	
	float crd_cc[3];
	ggcm_mhd_get_crds_cc(mhd, ix,iy,iz, p, crd_cc);
	float r = sqrtf(sqr(crd_cc[0]) + sqr(crd_cc[1]) + sqr(crd_cc[2]));
	// only set B outside of r1lim
	if (r >= mhd_dipole->r1lim) {
	  for (int d = 0; d < 3; d++){
	    M3(b, d, ix,iy,iz, p) = keep * M3(b, d, ix,iy,iz, p) + curl_a[d];
	  }
	}
      } mrc_fld_foreach_end;
    }
  } else { // face-centered B
    // FIXME, this doesn't fill Bnormal one ghost cell out on the right (high) side
    for (int p = 0; p < mrc_fld_nr_patches(b); p++) {
      float *bd3x = ggcm_mhd_crds_get_crd_p(mhd->crds, 0, BD3, p);
      float *bd3y = ggcm_mhd_crds_get_crd_p(mhd->crds, 1, BD3, p);
      float *bd3z = ggcm_mhd_crds_get_crd_p(mhd->crds, 2, BD3, p);
      
      mrc_fld_foreach(b, ix,iy,iz, 2, 1) {
	curl_a[0] = ((M3(a, 2, ix,iy+1,iz, p) - M3(a, 2, ix,iy,iz, p)) * bd3y[iy] -
		     (M3(a, 1, ix,iy,iz+1, p) - M3(a, 1, ix,iy,iz, p)) * bd3z[iz]);
	curl_a[1] = ((M3(a, 0, ix,iy,iz+1, p) - M3(a, 0, ix,iy,iz, p)) * bd3z[iz] -
		     (M3(a, 2, ix+1,iy,iz, p) - M3(a, 2, ix,iy,iz, p)) * bd3x[ix]);
	curl_a[2] = ((M3(a, 1, ix+1,iy,iz, p) - M3(a, 1, ix,iy,iz, p)) * bd3x[ix] -
		     (M3(a, 0, ix,iy+1,iz, p) - M3(a, 0, ix,iy,iz, p)) * bd3y[iy]);
	
	switch (mhd_type) {
	case MT_SEMI_CONSERVATIVE_GGCM:
	  M3(b, 0, ix-1,iy,iz, p) = keep * M3(b, 0, ix-1,iy,iz, p) + curl_a[0];
	  M3(b, 1, ix,iy-1,iz, p) = keep * M3(b, 1, ix,iy-1,iz, p) + curl_a[1];
	  M3(b, 2, ix,iy,iz-1, p) = keep * M3(b, 2, ix,iy,iz-1, p) + curl_a[2];
	  break;
	case MT_PRIMITIVE:
	case MT_SEMI_CONSERVATIVE:
	case MT_FULLY_CONSERVATIVE:
	  for (int d = 0; d < 3; d++){
	    float crd_fc[3];
	    ggcm_mhd_get_crds_fc(mhd, ix,iy,iz, p, d, crd_fc);
	    
	    // only set B outside of r1lim
	    float r = sqrtf(sqr(crd_fc[0]) + sqr(crd_fc[1]) + sqr(crd_fc[2]));
	    if (r >= mhd_dipole->r1lim) {
	      M3(b, d, ix,iy,iz, p) = keep * M3(b, d, ix,iy,iz, p) + curl_a[d];
	    }
	  }
	  break;
	default:
	  assert(0);
	}
      } mrc_fld_foreach_end;
    }
  }

  mrc_fld_put_as(a, a_base);
  mrc_fld_put_as(b, b_base);
  ggcm_mhd_put_3d_fld(mhd, a_base);
}

// ----------------------------------------------------------------------
// ggcm_mhd_dipole subclass

struct ggcm_mhd_dipole_ops ggcm_mhd_dipole_sub_ops = {
  .name            = ggcm_mhd_dipole_sub_name,
  .add_dipole      = ggcm_mhd_dipole_sub_add_dipole,
};
