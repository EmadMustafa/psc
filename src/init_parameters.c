
#include "psc.h"
#include "util/params.h"

struct psc_cmdline {
  const char *mod_particle;
};

#define VAR(x) (void *)offsetof(struct psc_domain, x)

static struct param psc_domain_descr[] = {
  { "length_x"      , VAR(length[0])       , PARAM_DOUBLE(1e-6)   },
  { "length_y"      , VAR(length[1])       , PARAM_DOUBLE(1e-6)   },
  { "length_z"      , VAR(length[2])       , PARAM_DOUBLE(20e-6)  },
  { "itot_x"        , VAR(itot[0])         , PARAM_INT(10)        },
  { "itot_y"        , VAR(itot[1])         , PARAM_INT(10)        },
  { "itot_z"        , VAR(itot[2])         , PARAM_INT(400)       },
  { "ilo_x"         , VAR(ilo[0])          , PARAM_INT(8)         },
  { "ilo_y"         , VAR(ilo[1])          , PARAM_INT(8)         },
  { "ilo_z"         , VAR(ilo[2])          , PARAM_INT(0)         },
  { "ihi_x"         , VAR(ihi[0])          , PARAM_INT(9)         },
  { "ihi_y"         , VAR(ihi[1])          , PARAM_INT(9)         },
  { "ihi_z"         , VAR(ihi[2])          , PARAM_INT(400)       },
  // FIXME, we should use something other than magic numbers here
  { "bnd_field_x"   , VAR(bnd_fld[0])      , PARAM_INT(1)         },
  { "bnd_field_y"   , VAR(bnd_fld[1])      , PARAM_INT(1)         },
  { "bnd_field_z"   , VAR(bnd_fld[2])      , PARAM_INT(1)         },
  { "bnd_particle_x", VAR(bnd_part[0])     , PARAM_INT(1)         },
  { "bnd_particle_y", VAR(bnd_part[1])     , PARAM_INT(1)         },
  { "bnd_particle_z", VAR(bnd_part[2])     , PARAM_INT(1)         },

  { "nproc_x",        VAR(nproc[0])        , PARAM_INT(1)         },
  { "nproc_y",        VAR(nproc[1])        , PARAM_INT(1)         },
  { "nproc_z",        VAR(nproc[2])        , PARAM_INT(1)         },
  {},
};

#undef VAR

void
init_param_domain()
{
  params_parse_cmdline_nodefault(&psc.domain, psc_domain_descr, "PSC domain",
				 MPI_COMM_WORLD);
  params_print(&psc.domain, psc_domain_descr, "PSC domain", MPI_COMM_WORLD);
}

#define VAR(x) (void *)offsetof(struct psc_param, x)

static struct param psc_param_descr[] = {
  { "qq"            , VAR(qq)              , PARAM_DOUBLE(1.6021e-19)   },
  { "mm"            , VAR(mm)              , PARAM_DOUBLE(9.1091e-31)   },
  { "tt"            , VAR(tt)              , PARAM_DOUBLE(1.6021e-16)   },
  { "cc"            , VAR(cc)              , PARAM_DOUBLE(3.0e8)        },
  { "eps0"          , VAR(eps0)            , PARAM_DOUBLE(8.8542e-12)   },
  {},
};

#undef VAR

void
init_param_psc()
{
  params_parse_cmdline_nodefault(&psc.prm, psc_param_descr, "PSC parameters",
				 MPI_COMM_WORLD);
  params_print(&psc.prm, psc_param_descr, "PSC parameters", MPI_COMM_WORLD);
}

// ======================================================================
// Fortran glue

#define C_init_param_domain_F77 F77_FUNC_(c_init_param_domain, C_INIT_PARAM_DOMAIN)
#define GET_param_domain_F77    F77_FUNC_(get_param_domain, GET_PARAM_DOMAIN)
#define SET_param_domain_F77    F77_FUNC_(set_param_domain, SET_PARAM_DOMAIN)
#define C_init_param_psc_F77    F77_FUNC_(c_init_param_psc, C_INIT_PARAM_PSC)
#define GET_param_psc_F77       F77_FUNC_(get_param_psc, GET_PARAM_PSC)
#define SET_param_psc_F77       F77_FUNC_(set_param_psc, SET_PARAM_PSC)

void GET_param_domain_F77(f_real *length, f_int *itot, f_int *in, f_int *ix,
			  f_int *bnd_fld, f_int *bnd_part, f_int *nproc);
void SET_param_domain_F77(f_real *length, f_int *itot, f_int *in, f_int *ix,
			  f_int *bnd_fld, f_int *bnd_part, f_int *nproc);
void GET_param_psc_F77(f_real *qq, f_real *mm, f_real *tt, f_real *cc, f_real *eps0);
void SET_param_psc_F77(f_real *qq, f_real *mm, f_real *tt, f_real *cc, f_real *eps0);

void
C_init_param_domain_F77()
{
  struct psc_domain *p = &psc.domain;
  int imax[3];

  GET_param_domain_F77(p->length, p->itot, p->ilo, imax,
			p->bnd_fld, p->bnd_part, p->nproc);
  for (int d = 0; d < 3; d++) {
    p->ihi[d] = imax[d] + 1;
  }

  init_param_domain();
  
  for (int d = 0; d < 3; d++) {
    imax[d] = p->ihi[d] - 1;
  }
  SET_param_domain_F77(p->length, p->itot, p->ilo, imax,
		       p->bnd_fld, p->bnd_part, p->nproc);
}

void
C_init_param_psc_F77()
{
  struct psc_param *p = &psc.prm;

  GET_param_psc_F77(&p->qq, &p->mm, &p->tt, &p->cc, &p->eps0);

  init_param_psc();
  
  SET_param_psc_F77(&p->qq, &p->mm, &p->tt, &p->cc, &p->eps0);
}

