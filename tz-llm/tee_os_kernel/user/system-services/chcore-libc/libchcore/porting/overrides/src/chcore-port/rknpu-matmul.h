#pragma once

int rknpu_matmul_fp16(
  _Float16 *matrixA,
  _Float16 *matrixB,
  float *matrixC,
  unsigned int M,
  unsigned int K,
  unsigned int N,
  int core_mask
);