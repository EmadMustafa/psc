
#include "psc_case_private.h"

// ----------------------------------------------------------------------
// _psc_case_create

static void
_psc_case_create(struct psc_case *_case)
{
  psc_create();
}

// ----------------------------------------------------------------------
// _psc_case_set_from_options

static void
_psc_case_set_from_options(struct psc_case *_case)
{
  // subclass set_from_options gets called first automatically 
  psc_set_from_options();
}

// ----------------------------------------------------------------------
// _psc_case_setup

static void
_psc_case_setup(struct psc_case *_case)
{
  // FIXME, probably broken, should go into sep subclass?
  if (psc.prm.from_checkpoint) {
    assert(0);
    psc_read_checkpoint();
  }
  // this sets up everything except allocating fields and particles,
  // and intializing them
  psc_setup();

  // alloc / initialize particles
  int particle_label_offset;
  psc_init_partition(&particle_label_offset);
  psc_init_particles(particle_label_offset);

  // alloc / initialize fields
  mfields_base_alloc(&psc.flds, NR_FIELDS);
  psc_init_field(&psc.flds);

  psc_setup_fortran();
}

// ----------------------------------------------------------------------
// _psc_case_view

static void
_psc_case_view(struct psc_case *_case)
{
  psc_view();
}

// ----------------------------------------------------------------------
// _psc_case_init_npt

void
psc_case_init_npt(struct psc_case *_case, int kind, double x[3],
		   struct psc_particle_npt *npt)
{
  if (!psc_case_ops(_case)->init_npt)
    return;

  psc_case_ops(_case)->init_npt(_case, kind, x, npt);
}

// ----------------------------------------------------------------------
// _psc_case_init_field

void
psc_case_init_field(struct psc_case *_case, mfields_base_t *flds)
{
  if (!psc_case_ops(_case)->init_field)
    return;

  psc_case_ops(_case)->init_field(_case, flds);
}

// ======================================================================
// psc_case_init

static struct psc_case_ops psc_case_default_ops = {
  .name                  = "default",
};

static void
psc_case_init()
{
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_default_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_test_yz_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_test_xz_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_harris_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_harris_xy_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_langmuir_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_wakefield_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_thinfoil_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_foils_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_curvedfoil_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_singlepart_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_collisions_ops);
  mrc_class_register_subclass(&mrc_class_psc_case, &psc_case_cone_ops);
}

// ======================================================================
// psc_case class

struct mrc_class_psc_case mrc_class_psc_case = {
  .name             = "psc_case",
  .size             = sizeof(struct psc_case),
  .init             = psc_case_init,
  .create           = _psc_case_create,
  .set_from_options = _psc_case_set_from_options,
  .setup            = _psc_case_setup,
  .view             = _psc_case_view,
};
