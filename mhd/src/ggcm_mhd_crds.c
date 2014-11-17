
#include "ggcm_mhd_crds_private.h"

#include "ggcm_mhd_defs.h"
#include "ggcm_mhd_private.h"
#include "ggcm_mhd_crds_gen.h"

#include <mrc_fld.h>
#include <mrc_domain.h>
#include <mrc_io.h>
#include <mrc_params.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

static const char *crdname[NR_CRDS] = {
  [FX1] = "FX1",
  [FD1] = "FD1",
  [FX2] = "FX2",
  [BD1] = "BD1",
  [BD2] = "BD2",
  [BD3] = "BD3",
  [BD4] = "BD4",
};

// ======================================================================
// ggcm_mhd_crds class

#define ggcm_mhd_crds_ops(crds) ((struct ggcm_mhd_crds_ops *)((crds)->obj.ops))

// ----------------------------------------------------------------------
// ggcm_mhd_crds_create

static void
_ggcm_mhd_crds_create(struct ggcm_mhd_crds *crds)
{
  for (int d = 0; d < 3; d++) {
    crds->f1[d] = mrc_fld_create(MPI_COMM_SELF);
    crds->global_f1[d] = mrc_fld_create(MPI_COMM_SELF);
    char s[20]; sprintf(s, "f1[%d]", d);
    char gs[20]; sprintf(gs, "global_f1[%d]", d);
    mrc_fld_set_name(crds->f1[d], s);
    mrc_fld_set_name(crds->global_f1[d], gs);
  }
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_setup

static void
_ggcm_mhd_crds_setup(struct ggcm_mhd_crds *crds)
{
  struct mrc_crds *mrc_crds = mrc_domain_get_crds(crds->domain);
  int gdims[3];
  mrc_domain_get_global_dims(crds->domain, gdims);

  for (int d = 0; d < 3; d++) {
    mrc_fld_set_param_obj(crds->f1[d], "domain", crds->domain);
    mrc_fld_set_param_int(crds->f1[d], "nr_spatial_dims", 1);
    mrc_fld_set_param_int(crds->f1[d], "dim", d);
    mrc_fld_set_param_int(crds->f1[d], "nr_comps", NR_CRDS);
    mrc_fld_set_param_int(crds->f1[d], "nr_ghosts", mrc_crds->sw);
    for (int m = 0; m < NR_CRDS; m++) {
      mrc_fld_set_comp_name(crds->f1[d], m, crdname[m]);
    }
    mrc_fld_setup(crds->f1[d]);

    mrc_fld_set_param_int_array(crds->global_f1[d], "dims", 1, (int[1]) { gdims[d] });
    mrc_fld_set_param_int_array(crds->global_f1[d], "sw"  , 1, (int[1]) { mrc_crds->sw });
    mrc_fld_setup(crds->global_f1[d]);
  }

  // set up values for Fortran coordinate arrays
  assert(crds->crds_gen);
  ggcm_mhd_crds_gen_run(crds->crds_gen, crds);

  // set up mrc_crds
  struct mrc_patch_info info;
  mrc_domain_get_local_patch_info(crds->domain, 0, &info);

  for (int d = 0; d < 3; d++) {
    memcpy(mrc_crds->crd[d]->_arr, ggcm_mhd_crds_get_crd(crds, d, FX1) - mrc_crds->sw,
	   mrc_crds->crd[d]->_len * sizeof(float));

    memcpy(mrc_crds->global_crd[d]->_arr, ggcm_mhd_crds_get_global_crd(crds, d) - mrc_crds->sw,
     mrc_crds->crd[d]->_len * sizeof(float));
  }
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_destroy

static void
_ggcm_mhd_crds_destroy(struct ggcm_mhd_crds *crds)
{
  for (int d = 0; d < 3; d++) {
    mrc_fld_destroy(crds->f1[d]);
    mrc_fld_destroy(crds->global_f1[d]);
  }
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_read

static void
_ggcm_mhd_crds_read(struct ggcm_mhd_crds *crds, struct mrc_io *io)
{
  ggcm_mhd_crds_read_member_objs(crds, io);
  crds->f1[0] = mrc_io_read_ref(io, crds, "f1[0]", mrc_fld);
  crds->f1[1] = mrc_io_read_ref(io, crds, "f1[1]", mrc_fld);
  crds->f1[2] = mrc_io_read_ref(io, crds, "f1[2]", mrc_fld);
  crds->global_f1[0] = mrc_io_read_ref(io, crds, "global_f1[0]", mrc_fld);
  crds->global_f1[1] = mrc_io_read_ref(io, crds, "global_f1[1]", mrc_fld);
  crds->global_f1[2] = mrc_io_read_ref(io, crds, "global_f1[2]", mrc_fld);
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_write

static void
_ggcm_mhd_crds_write(struct ggcm_mhd_crds *crds, struct mrc_io *io)
{
  mrc_io_write_ref(io, crds, "f1[0]", crds->f1[0]);
  mrc_io_write_ref(io, crds, "f1[1]", crds->f1[1]);
  mrc_io_write_ref(io, crds, "f1[2]", crds->f1[2]);
  mrc_io_write_ref(io, crds, "global_f1[0]", crds->global_f1[0]);
  mrc_io_write_ref(io, crds, "global_f1[1]", crds->global_f1[1]);
  mrc_io_write_ref(io, crds, "global_f1[2]", crds->global_f1[2]);
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_get_crd

float *
ggcm_mhd_crds_get_crd(struct ggcm_mhd_crds *crds, int d, int m)
{
  return &MRC_F1(crds->f1[d], m, 0);
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_get_global_crd

float *
ggcm_mhd_crds_get_global_crd(struct ggcm_mhd_crds *crds, int d)
{
  return &MRC_F1(crds->global_f1[d], 0, 0);
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_get_cc

void
ggcm_mhd_crds_get_cc(struct ggcm_mhd_crds *crds, int ix, int iy, int iz, float crd_cc[3])
{
  crd_cc[0] = ggcm_mhd_crds_get_crd(crds, 0, FX1)[ix];
  crd_cc[1] = ggcm_mhd_crds_get_crd(crds, 1, FX1)[iy];
  crd_cc[2] = ggcm_mhd_crds_get_crd(crds, 2, FX1)[iz];
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_get_nc

void
ggcm_mhd_crds_get_nc(struct ggcm_mhd_crds *crds, int ix, int iy, int iz, float crd_nc[3])
{
  float crd_cc[3];
  float crd_cc_m[3];
  ggcm_mhd_crds_get_cc(crds, ix, iy, iz, crd_cc);
  ggcm_mhd_crds_get_cc(crds, ix - 1, iy - 1, iz - 1, crd_cc_m);

  crd_nc[0] = .5f * (crd_cc_m[0] + crd_cc[0]);
  crd_nc[1] = .5f * (crd_cc_m[1] + crd_cc[1]);
  crd_nc[2] = .5f * (crd_cc_m[2] + crd_cc[2]);
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_get_fc

void
ggcm_mhd_crds_get_fc(struct ggcm_mhd_crds *crds, int ix, int iy, int iz, int d,
		    float crd_fc[3])
{
  float crd_cc[3];
  float crd_nc[3];
  ggcm_mhd_crds_get_cc(crds, ix, iy, iz, crd_cc);
  ggcm_mhd_crds_get_nc(crds, ix, iy, iz, crd_nc);

  if (d == 0) {
    // Bx located at i, j+.5, k+.5
    crd_fc[0] = crd_nc[0];
    crd_fc[1] = crd_cc[1];
    crd_fc[2] = crd_cc[2];
  } else if (d == 1) {
    // By located at i+.5, j, k+.5
    crd_fc[0] = crd_cc[0];
    crd_fc[1] = crd_nc[1];
    crd_fc[2] = crd_cc[2];
  } else if (d == 2) {
    // Bz located at i+.5, j+.5, k
    crd_fc[0] = crd_cc[0];
    crd_fc[1] = crd_cc[1];
    crd_fc[2] = crd_nc[2];
  } else {
    assert(0);
  }
}

// ----------------------------------------------------------------------
// ggcm_crd_get_ec

void
ggcm_mhd_crds_get_ec(struct ggcm_mhd_crds *crds, int ix, int iy, int iz, int d,
		     float crd_ec[3])
{
  float crd_cc[3];
  float crd_nc[3];
  ggcm_mhd_crds_get_cc(crds, ix, iy, iz, crd_cc);
  ggcm_mhd_crds_get_nc(crds, ix, iy, iz, crd_nc);

  if (d == 0) {
    // Ex located at i+.5, j, k
    crd_ec[0] = crd_cc[0];
    crd_ec[1] = crd_nc[1];
    crd_ec[2] = crd_nc[2];
  } else if (d == 1) {
    // Ey located at i, j+.5, k
    crd_ec[0] = crd_nc[0];
    crd_ec[1] = crd_cc[1];
    crd_ec[2] = crd_nc[2];
  } else if (d == 2) {
    // Ez located at i, j, k+.5
    crd_ec[0] = crd_nc[0];
    crd_ec[1] = crd_nc[1];
    crd_ec[2] = crd_cc[2];
  } else {
    assert(0);
  }
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds_init

static void
ggcm_mhd_crds_init()
{
  mrc_class_register_subclass(&mrc_class_ggcm_mhd_crds, &ggcm_mhd_crds_c_ops);
}

// ----------------------------------------------------------------------
// ggcm_mhd_crds class description

#define VAR(x) (void *)offsetof(struct ggcm_mhd_crds, x)
static struct param ggcm_mhd_crds_descr[] = {
  { "domain"          , VAR(domain)         , PARAM_OBJ(mrc_domain)          },

  { "crds_gen"        , VAR(crds_gen)       , MRC_VAR_OBJ(ggcm_mhd_crds_gen) },
  {},
};
#undef VAR

// ----------------------------------------------------------------------
// ggcm_mhd_crds class

struct mrc_class_ggcm_mhd_crds mrc_class_ggcm_mhd_crds = {
  .name             = "ggcm_mhd_crds",
  .size             = sizeof(struct ggcm_mhd_crds),
  .param_descr      = ggcm_mhd_crds_descr,
  .init             = ggcm_mhd_crds_init,
  .create           = _ggcm_mhd_crds_create,
  .setup            = _ggcm_mhd_crds_setup,
  .destroy          = _ggcm_mhd_crds_destroy,
  .read             = _ggcm_mhd_crds_read,
  .write            = _ggcm_mhd_crds_write,
};

