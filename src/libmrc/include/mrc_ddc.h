
#ifndef MRC_DDC_H
#define MRC_DDC_H

#include <mrc_common.h>
#include <mrc_obj.h>

#include <mpi.h>

struct mrc_ddc_funcs {
  void (*copy_to_buf)(int mb, int me, int ilo[3], int ihi[3], void *buf, void *ctx);
  void (*copy_from_buf)(int mb, int me, int ilo[3], int ihi[3], void *buf, void *ctx);
  void (*add_from_buf)(int mb, int me, int ilo[3], int ihi[3], void *buf, void *ctx);
};

struct mrc_domain;

extern struct mrc_class mrc_class_mrc_ddc;
MRC_OBJ_DEFINE_STANDARD_METHODS(mrc_ddc, struct mrc_ddc)

void mrc_ddc_set_funcs(struct mrc_ddc *ddc, struct mrc_ddc_funcs *funcs);
void mrc_ddc_set_domain(struct mrc_ddc *ddc, struct mrc_domain *domain);
void mrc_ddc_setup(struct mrc_ddc *ddc);
void mrc_ddc_destroy(struct mrc_ddc *ddc);
void mrc_ddc_add_ghosts(struct mrc_ddc *ddc, int mb, int me, void *ctx);
void mrc_ddc_fill_ghosts(struct mrc_ddc *ddc, int mb, int me, void *ctx);
int mrc_ddc_get_rank_nei(struct mrc_ddc *ddc, int dir[3]);

#define MRC_DDC_BUF3(buf,m, ix,iy,iz)		\
  (buf[(((m) * (ihi[2] - ilo[2]) +		\
	 iz - ilo[2]) * (ihi[1] - ilo[1]) +	\
	iy - ilo[1]) * (ihi[0] - ilo[0]) +	\
       ix - ilo[0]])

struct mrc_ddc_funcs mrc_ddc_funcs_f3;

static inline int
mrc_ddc_dir2idx(int dir[3])
{
  return ((dir[2] + 1) * 3 + dir[1] + 1) * 3 + dir[0] + 1;
}

#endif

