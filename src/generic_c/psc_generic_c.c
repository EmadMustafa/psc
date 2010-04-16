
#include "psc_generic_c.h"

#include <stdlib.h>

static void
genc_particles_from_fortran()
{
  struct psc_genc *genc = psc.c_ctx;
  if (!psc.c_ctx) {
    psc.c_ctx = malloc(sizeof(*psc.c_ctx));
    genc = psc.c_ctx;
    genc->part = calloc(psc.n_part, sizeof(*genc->part));
    // FIXME, this is all leaked..
  }
  for (int n = 0; n < psc.n_part; n++) {
    struct f_particle *f_part = &psc.f_part[n];
    struct c_particle *part = &genc->part[n];

    part->xi  = f_part->xi;
    part->yi  = f_part->yi;
    part->zi  = f_part->zi;
    part->pxi = f_part->pxi;
    part->pyi = f_part->pyi;
    part->pzi = f_part->pzi;
    part->qni = f_part->qni;
    part->mni = f_part->mni;
    part->wni = f_part->wni;
  }
}

static void
genc_particles_to_fortran()
{
  struct psc_genc *genc = psc.c_ctx;

  for (int n = 0; n < psc.n_part; n++) {
    struct f_particle *f_part = &psc.f_part[n];
    struct c_particle *part = &genc->part[n];

    f_part->xi  = part->xi;
    f_part->yi  = part->yi;
    f_part->zi  = part->zi;
    f_part->pxi = part->pxi;
    f_part->pyi = part->pyi;
    f_part->pzi = part->pzi;
    f_part->qni = part->qni;
    f_part->mni = part->mni;
    f_part->wni = part->wni;
  }
}

static void
genc_fields_from_fortran()
{
  struct psc_genc *genc = psc.c_ctx;

  genc->flds = calloc(NR_FIELDS * psc.fld_size, sizeof(float));

  for (int m = EX; m <= BZ; m++) {
    for (int jz = psc.ilg[2]; jz < psc.ihg[2]; jz++) {
      for (int jy = psc.ilg[1]; jy < psc.ihg[1]; jy++) {
	for (int jx = psc.ilg[0]; jx < psc.ihg[0]; jx++) {
	  F3(m, jx,jy,jz) = FF3(m, jx,jy,jz);
	}
      }
    }
  }
}

static void
genc_fields_to_fortran()
{
  struct psc_genc *genc = psc.c_ctx;
  free(genc->flds);
}

struct psc_ops psc_ops_generic_c = {
  .name = "generic_c",
  .particles_from_fortran = genc_particles_from_fortran,
  .particles_to_fortran   = genc_particles_to_fortran,
  .fields_from_fortran    = genc_fields_from_fortran,
  .fields_to_fortran      = genc_fields_to_fortran,
  .push_part_yz_a         = genc_push_part_yz_a,
  .push_part_yz_b         = genc_push_part_yz_b,
};
