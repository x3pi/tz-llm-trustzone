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

// #include <libdrm/drm.h>

#include "rknpu-ioctl.h"
#include "rknpu-driver.h"
#include "npu_interface.h"
#include "npu_matmul.h"
#include <sys/time.h>

#include "rknpu-matmul.h"
#include <chcore/syscall.h>

#include <pthread.h>

#define MAX_M 384 
#define MAX_K 4096 
#define MAX_N 4096 

static inline int64_t getCurrentTimeUs() {
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void matmul_fp32(int m, int k, int n, _Float16 *src0 , _Float16 *src1, float* dst) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      float sum = 0;
      for (int l = 0; l < k; l++) {
        sum += src0[i*k + l] * src1[j*k + l];
      }
     dst[i*n + j] = sum;
    }
  }
}

float rand_float() {
  return rand()/(float)RAND_MAX;
}

void run_matmul(int M, int K, int N, int core_mask)
{
   printf("tid %d Multiplication fp16 of [%d,%d] x [%d,%d] \n",gettid(),M,K,N,K);	  

  // Test currently runs against kernel 5.10 haven't tested 6.1 kernel.

  // matrix A max size
  _Float16 *matrixA = malloc((M*K) * sizeof(_Float16));

  // matrix B max size
  _Float16 *matrixB = malloc((N*K) * sizeof(_Float16));

  // matrix C max size
  float *matrixC = malloc((M*N) * sizeof(float));

  // matrix C max size
  float *expected_result = malloc((M*N) * sizeof(float));

  uint64_t *npu_regs = malloc(112 * sizeof(uint64_t));
  // Need to use whole numbers for now as decimals return a slighty 
  // different result compared to ARM float calculations. Hence Rockchip
  // examples don't perform a exact comparison between expected and acutal
  // results from the matrix mutlipcation for fp16. Need to know why?
  for (int i = 0; i < M*K; i++) {
    matrixA[i] = (int)(10.0*rand_float()); 
  }
  
  for (int i = 0; i < N*K; i++) {
    matrixB[i] = (int)(10.0*rand_float());
 }

  matmul_fp32(M,K,N,(_Float16 *)matrixA, (_Float16 *)matrixB, (float *)expected_result);

  int ret = rknpu_matmul_fp16(matrixA, matrixB, matrixC, M, K, N, core_mask);

  printf("=========================================================================================================\n");
  for (int m=1;m<=M;m++) {
    for (int n=1;n<N;n++) {
      float actual = matrixC[((m-1)*N)+(n-1)];
      float expected = expected_result[((m-1)*N)+(n-1)];
      int32_t *e, *a;
      e = (int32_t *)&expected;
      a = (int32_t *)&actual;
      if (actual != expected) {
        printf("\ntid %d mismatch m:%d n:%d  expected:%6.5f acutal:%6.5f %x %x\n",gettid(),m,n,expected,actual,*e,*a);
        ret = -1;
      }
    }
  }
  if (ret == 0) {
   printf("tid %d Multiplication fp16 of [%d,%d] x [%d,%d] succesful \n",gettid(),M,K,N,K);	  
  }
  printf("=========================================================================================================\n");



  free(matrixA);
  free(matrixB);
  free(matrixC);
  free(expected_result);
  free(npu_regs);
  if (ret != 0){
    exit(-1);
  }
}

struct arg {
  unsigned int M;
  unsigned int K;
  unsigned int N;
  int core_mask;
};

void *test_routine(void *id) {
  usys_set_prio(0, 55);
  struct arg *arg = (struct arg *)id;
  // run_matmul(arg->M, arg->K, arg->N, arg->core_mask);
  // return NULL;
  while (1) {
    int M = rand() % arg->M;
    M = (M / 32 + 1) * 32;
    int K = rand() % arg->K;
    K = (K / 32 + 1) * 32;
    int N = rand() % arg->N;
    N = (N / 32 + 1) * 32;
    M = arg->M; K = arg->K; N = arg->N;
    run_matmul(M, K, N, arg->core_mask);
  }
  run_matmul(arg->M, arg->K, arg->N, arg->core_mask);
  run_matmul(arg->M, arg->K, arg->N, arg->core_mask);
  return NULL;
}

int main(int argc, char **argv) {

  unsigned int M=0;
  unsigned int K=0;
  unsigned int N=0;

  if (argc !=4) {
    printf("Invalid number of args %d, needs to supply M K N ie matmul_fp16 <M> <K> <N>\n",argc);
    return -1; 
  }

  M = atoi(argv[1]);
  K = atoi(argv[2]);
  N = atoi(argv[3]);
  
  srand(time(NULL));

  // usys_top(1);
  // while (1);

  // pthread_t t[3];
  // for (int i = 0; i < 2; i++) {
  //   struct arg *arg = malloc(sizeof(struct arg));
  //   arg->M = 100;
  //   arg->K = 1536;
  //   arg->N = 4096;
  //   arg->core_mask = 4 >> i;
  //   pthread_create(t + i, NULL, test_routine, (void *)arg);
  // }
  // for (int i = 0; i < 3; i++) {
  //   pthread_join(t[i], NULL);
  // }

  /*
   * This used to be `while (1) run_matmul(100, 1536, 4096, 1);` -- an
   * unconditional infinite loop that ignored the argv-supplied M/K/N
   * entirely and never returned. chanmgr's master() does
   * `waitpid(create_process("/rknpu.srv", ...))` right after launching this
   * and expects it to exit so it can print "chanmgr: rknpu test finish" and
   * then launch llama-cli -- with the loop, that waitpid() never returns and
   * llama-cli never runs. Verified live on-device that a bounded run works:
   * dozens of consecutive "Waiting for IRQ" -> "irq 142 handled" -> "Woke up
   * from IRQ, ret=0" -> "Multiplication ... succesful" cycles, no failures.
   * Restored the 5 calls below (dead code before this fix, never reached) as
   * the bounded self-test so this function actually returns.
   */
  run_matmul(M, K, N, 2);
  run_matmul(M, K, N, 4);
  run_matmul(M, K, N, 1);
  run_matmul(M, K, N, 2);
  run_matmul(M, K, N, 4);

	/* switch back to non-secure */
	// usys_top(0);
}
