
#include "psc_cuda.h"

#define BLOCKSIZE_X 1
#define BLOCKSIZE_Y 4
#define BLOCKSIZE_Z 4

#define PFX(x) push_fields_yz_##x
#include "constants.c"

__global__ static void
push_fields_E_yz(real *d_flds)
{
  // FIXME, precalc
  real cny = .5f * d_consts.dt * d_consts.dxi[1];
  real cnz = .5f * d_consts.dt * d_consts.dxi[2];
  int iy = blockIdx.x * blockDim.x + threadIdx.x;
  int iz = blockIdx.y * blockDim.y + threadIdx.y;

  if (!(iy < d_consts.mx[1] - 2*3 && iz < d_consts.mx[2] - 2*3)) // FIXME
    return;

  F3_DEV(EX, 0,iy,iz) +=
    cny * (F3_DEV(HZ, 0,iy,iz) - F3_DEV(HZ, 0,iy-1,iz)) -
    cnz * (F3_DEV(HY, 0,iy,iz) - F3_DEV(HY, 0,iy,iz-1)) -
    .5f * d_consts.dt * F3_DEV(JXI, 0,iy,iz);
  
  F3_DEV(EY, 0,iy,iz) +=
    cnz * (F3_DEV(HX, 0,iy,iz) - F3_DEV(HX, 0,iy,iz-1)) -
    0.f -
    .5f * d_consts.dt * F3_DEV(JYI, 0,iy,iz);
  
  F3_DEV(EZ, 0,iy,iz) +=
    0.f -
    cny * (F3_DEV(HX, 0,iy,iz) - F3_DEV(HX, 0,iy-1,iz)) -
    .5f * d_consts.dt * F3_DEV(JZI, 0,iy,iz);
}

__global__ static void
push_fields_H_yz(real *d_flds)
{
  // FIXME, precalc
  real cny = .5f * d_consts.dt * d_consts.dxi[1];
  real cnz = .5f * d_consts.dt * d_consts.dxi[2];
  int iy = blockIdx.x * blockDim.x + threadIdx.x;
  int iz = blockIdx.y * blockDim.y + threadIdx.y;

  if (!(iy < d_consts.mx[1] - 2*3 && iz < d_consts.mx[2] - 2*3)) // FIXME
    return;

  F3_DEV(HX, 0,iy,iz) -=
    cny * (F3_DEV(EZ, 0,iy+1,iz) - F3_DEV(EZ, 0,iy,iz)) -
    cnz * (F3_DEV(EY, 0,iy,iz+1) - F3_DEV(EY, 0,iy,iz));
  
  F3_DEV(HY, 0,iy,iz) -=
    cnz * (F3_DEV(EX, 0,iy,iz+1) - F3_DEV(EX, 0,iy,iz)) -
    0.f;
  
  F3_DEV(HZ, 0,iy,iz) -=
    0.f -
    cny * (F3_DEV(EX, 0,iy+1,iz) - F3_DEV(EX, 0,iy,iz));
}

EXTERN_C void
cuda_push_fields_E_yz(int p, struct psc_fields *pf)
{
  struct psc_fields_cuda *pfc = psc_fields_cuda(pf);
  struct psc_patch *patch = &ppsc->patch[p];

  push_fields_yz_set_constants(NULL, pf);

  int dimBlock[2] = { BLOCKSIZE_Y, BLOCKSIZE_Z };
  int dimGrid[2]  = { (patch->ldims[1] + BLOCKSIZE_Y - 1) / BLOCKSIZE_Y,
		      (patch->ldims[2] + BLOCKSIZE_Z - 1) / BLOCKSIZE_Z };
  RUN_KERNEL(dimGrid, dimBlock,
	     push_fields_E_yz, (pfc->d_flds));
}

EXTERN_C void
cuda_push_fields_H_yz(int p, struct psc_fields *pf)
{
  struct psc_fields_cuda *pfc = psc_fields_cuda(pf);
  struct psc_patch *patch = &ppsc->patch[p];
  int dimBlock[2] = { BLOCKSIZE_Y, BLOCKSIZE_Z };
  int dimGrid[2]  = { (patch->ldims[1] + BLOCKSIZE_Y - 1) / BLOCKSIZE_Y,
		      (patch->ldims[2] + BLOCKSIZE_Z - 1) / BLOCKSIZE_Z };
  RUN_KERNEL(dimGrid, dimBlock,
	     push_fields_H_yz, (pfc->d_flds));
}

