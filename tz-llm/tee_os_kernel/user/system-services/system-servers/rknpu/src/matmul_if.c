/*
 * Copyright (C) 2024  Jasbir Matharu, <jasjnuk@gmail.com>
 *
 * This file is part of rk3588-npu.
 *
 * rk3588-npu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * rk3588-npu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with rk3588-npu.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>
#include <pthread.h>

#include <chcore/syscall.h>

// #include <libdrm/drm.h>

#include "rknpu-ioctl.h"
#include "rknpu-driver.h"
#include "npu_interface.h"
#include "npu_matmul.h"
#include <sys/time.h>

#define MAX_M 384 
#define MAX_K 4096 
#define MAX_N 4096 

int rknpu_matmul_fp16(
  _Float16 *matrixA,
  _Float16 *matrixB,
  float *matrixC,
  unsigned int M,
  unsigned int K,
  unsigned int N,
  int core_mask
) {
  int ret=0;
  uint64_t npu_regs[112];

  if ((M<=0) || (M>MAX_M) | (((M%4)!=0) && (M!=1))) {
    printf("M [%d] is out of range or not a mutliple of 4 \n",M);
    return -1;
  }

  if ((K<=0) || (K>MAX_K) || ((K%32) != 0)) {
    printf("K [%d] is out of range or not a mutliple of 32\n",K);
    return -1;
  }

  if ((N<=0) || (N>MAX_N) || ((N%16) != 0)) {
    printf("N [%d] is out of range or not a mutliple of 16\n",N);
    return -1;
  }

  uint64_t regcmd_dma, regcmd_obj;
  uint64_t regcmd_handle;
  uint64_t *regcmd = mem_allocate(1024, &regcmd_dma, &regcmd_obj, 0, &regcmd_handle);

  uint64_t tasks_dma, tasks_obj;
  uint64_t tasks_handle;
  struct rknpu_task *tasks = mem_allocate(1024, &tasks_dma, &tasks_obj, RKNPU_MEM_KERNEL_MAPPING, &tasks_handle);

  uint64_t input_dma, input_obj;
  uint64_t input_handle;
  void *input = mem_allocate(M*K*sizeof(__fp16), &input_dma, &input_obj, RKNPU_MEM_CACHEABLE, &input_handle);

  uint64_t weights_dma, weights_obj;
  uint64_t weights_handle;
  void *weights = mem_allocate(N*K*sizeof(__fp16), &weights_dma, &weights_obj, RKNPU_MEM_CACHEABLE, &weights_handle);

  uint64_t output_dma, output_obj;
  uint64_t output_handle;
  void *output = mem_allocate(M*N*sizeof(float), &output_dma, &output_obj, RKNPU_MEM_CACHEABLE, &output_handle);

  printf("input dma is %lx, output dma is %lx, weights dma is %lx\n", input_dma, output_dma, weights_dma);
  if ((regcmd == NULL) || (tasks == NULL) || (input == NULL) || (weights == NULL) || (output == NULL)) {
    printf("Failed to allocate memory \n");
    exit(1);
  }

  // Reset the NPU
  npu_reset();

  matmul_params_t params;
  params.m = M;
  params.k = K;
  params.n = N;
  params.input_dma = input_dma;
  params.weights_dma = weights_dma;
  params.output_dma = output_dma;
  params.tasks = (uint64_t *) &npu_regs;
  params.fp32tofp16 = 0;
  ret = gen_matmul_fp16(&params);
  if (ret !=0) {
    printf("gen_matmul_fp16 failed %d\n",ret);
    goto cleanup;
  }
  
  memcpy(regcmd,npu_regs,sizeof(npu_regs));

  tasks[0].flags  = 0;
  tasks[0].op_idx = 0;
  tasks[0].enable_mask = 0xd;
  tasks[0].int_mask = 0x300; // wait for DPU to finish
  tasks[0].int_clear = 0x1ffff;
  tasks[0].int_status =0;
  tasks[0].regcfg_amount = sizeof(npu_regs)/sizeof(uint64_t)-(RKNPU_PC_DATA_EXTRA_AMOUNT+4);
  tasks[0].regcfg_offset = 0;
  tasks[0].regcmd_addr = regcmd_dma;

  memset((void *)input,0,M*K*sizeof(__fp16));
  memset((void *)weights,0,K*N*sizeof(__fp16));
  memset((void *)output,0,M*N*sizeof(float));
  usys_cache_flush((unsigned long)output, M*N*sizeof(float), CACHE_CLEAN);

  __fp16 *weights_fp16 = weights;
   
  for(int n=1;n<=N;n++) {
    for(int k=1;k<=K;k++) {
      weights_fp16[weight_fp16(K,n,k)]= matrixB[((n-1)*K)+(k-1)];
    }
  }
  usys_cache_flush((unsigned long)weights, K*N*sizeof(__fp16), CACHE_CLEAN);

  __fp16 *feature_data_fp16 = (__fp16*) input;

  for (int m=1;m<=M;m++) {
    for (int k=1;k<=K;k++) {
      feature_data_fp16[feature_data(K,M,1,8,k,m,1)]= matrixA[((m-1)*K)+(k-1)];
    }
  }
  usys_cache_flush((unsigned long)input, M*K*sizeof(__fp16), CACHE_CLEAN);

  npu_submit(tasks_obj, core_mask);

  if (ret < 0) {
    return ret;
  }

  usys_cache_flush((unsigned long)output, M*N*sizeof(float), CACHE_INVALIDATE);
  float *output_data = (float*) output;
  for (int m=1;m<=M;m++) {
    for (int n=1;n<N;n++) {
      matrixC[((m-1)*N)+(n-1)] = output_data[feature_data(N, M, 1, 4, n, m, 1)];
    }
  }

cleanup:

  mem_destroy(regcmd,1024, regcmd_handle, regcmd_obj);
  mem_destroy(tasks,1024, tasks_handle, tasks_obj);
  mem_destroy(input,M*K*sizeof(_Float16), input_handle, input_obj);
  mem_destroy(weights,N*K*sizeof(_Float16), weights_handle, weights_obj);
  mem_destroy(output,M*N*sizeof(float), output_handle, output_obj);

  return ret;
}
