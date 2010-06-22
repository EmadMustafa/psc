
#include "psc_sse2.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "psc_sse2.h"
#include "simd_wrap.h"

static void
sse2_create()
{
  struct psc_sse2 *sse2 = malloc(sizeof(*sse2));
  psc.c_ctx = sse2;
}

static void
sse2_destroy()
{
  struct psc_sse2 *sse2 = psc.c_ctx;
  free(sse2);
}

// For now this is all more or less identical to kai's generic_c. 
static void
sse2_particles_from_fortran()
{
  struct psc_sse2 *sse2 = psc.c_ctx;
  int pad = 0;
  if((psc.n_part % VEC_SIZE) != 0){
    pad = VEC_SIZE - (psc.n_part % VEC_SIZE);
  }
  sse2->part = calloc((psc.n_part + pad), sizeof(*sse2->part));
  for (int n = 0; n < psc.n_part; n++) {
    struct f_particle *f_part = &psc.f_part[n];
    struct sse2_particle *part = &sse2->part[n];

    part->xi  = f_part->xi;
    part->yi  = f_part->yi;
    part->zi  = f_part->zi;
    part->pxi = f_part->pxi;
    part->pyi = f_part->pyi;
    part->pzi = f_part->pzi;
    part->qni = f_part->qni;
    part->mni = f_part->mni;
    part->wni = f_part->wni;
    assert(round(part->xi) == 0); //FIXME: ensures we have true 2-D with x<.5 for all parts
  }
  // We need to give the padding a non-zero mass to avoid NaNs
  for(int n = psc.n_part; n < (psc.n_part + pad); n++){
    struct f_particle *f_part = &psc.f_part[psc.n_part-1];
    struct sse2_particle *part = &sse2->part[n];
    part->xi  = f_part->xi; //We need to be sure the padding loads fields inside the local domain
    part->yi  = f_part->yi;
    part->zi  = f_part->zi;
    part->mni = 1.0;
  }
}

static void
sse2_particles_to_fortran()
{
   struct psc_sse2 *sse2 = psc.c_ctx;
   
   for(int n = 0; n < psc.n_part; n++) {
     struct f_particle *f_part = &psc.f_part[n];
     struct sse2_particle *part = &sse2->part[n];
     
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
sse2_fields_from_fortran(){
  struct psc_sse2 *sse2 = psc.c_ctx;
  void *m = _mm_malloc(NR_FIELDS*psc.fld_size*sizeof(sse2_real), 16);
  sse2->fields = m;

  for(int m = 0; m < NR_FIELDS; m++){
    for(int n = 0; n < psc.fld_size; n++){
      //preserve Fortran ordering for now
      sse2->fields[m * psc.fld_size + n] = (sse2_real)psc.f_fields[m][n];
    }
  }
}

static void
sse2_fields_to_fortran(){
  struct psc_sse2 *sse2 = psc.c_ctx;
  assert(sse2->fields != NULL);
  for(int m = 0; m < NR_FIELDS; m++){
    for(int n = 0; n < psc.fld_size; n++){
      psc.f_fields[m][n] = sse2->fields[m * psc.fld_size + n];
    }
  }
}


struct psc_ops psc_ops_sse2 = {
  .name = "sse2",
  .create                 = sse2_create,
  .destroy                = sse2_destroy,
  .particles_from_fortran = sse2_particles_from_fortran,
  .particles_to_fortran   = sse2_particles_to_fortran,
  .fields_from_fortran    = sse2_fields_from_fortran,
  .fields_to_fortran      = sse2_fields_to_fortran,
  .push_part_yz_a         = sse2_push_part_yz_a,
  .push_part_yz_b         = sse2_push_part_yz_b,
  .push_part_yz           = sse2_push_part_yz,
}; 
