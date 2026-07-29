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

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <chcore/memory.h>

#include "rknpu-ioctl.h"
#include "rknpu-driver.h"
#include "npu_hw.h"
#include "npu_interface.h"

#include <pthread.h>

void *rknpu_dev;
pthread_once_t rknpu_dev_once;
void *npu_open(void);

void rknpu_dev_init(void) {
  rknpu_dev = npu_open();
}

void* mem_allocate(size_t size, uint64_t *dma_addr, uint64_t *obj, uint32_t flags, uint64_t *handle) {
  pthread_once(&rknpu_dev_once, rknpu_dev_init);
  void *ret;
  struct chcore_dma_handle *dma_handle;
  // struct rknpu_mem_create mem_create = {
  //   .flags = flags | RKNPU_MEM_NON_CACHEABLE,
  //   .size = size,
  // };

  // ret = ioctl(fd, DRM_IOCTL_RKNPU_MEM_CREATE, &mem_create);
  // ret = rknpu_mem_create(rknpu_dev, &mem_create);

  // if(ret < 0)  {
  //   printf("RKNPU_MEM_CREATE failed %d\n",ret);
  //   return NULL;
  // }

  // struct rknpu_mem_map mem_map = { .handle = mem_create.handle, .offset=0 };
  // ret = ioctl(fd, DRM_IOCTL_RKNPU_MEM_MAP, &mem_map);
  // if(ret < 0) {
  //   printf("RKNPU_MEM_MAP failed %d\n",ret);
  //   return NULL;
  // }

  // void *map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mem_map.offset);

  // *dma_addr = mem_create.dma_addr;
  // *obj = mem_create.obj_addr;

  dma_handle = malloc(sizeof(*dma_handle));
  assert(dma_handle);

  ret = chcore_alloc_dma_mem(size, dma_handle, flags & RKNPU_MEM_CACHEABLE);
  assert(ret);

  *dma_addr = dma_handle->paddr;
  *obj = dma_handle->vaddr;
  *handle = (uint64_t)dma_handle;
  return (void *)dma_handle->vaddr;
}

void mem_destroy(void *addr, size_t len, uint64_t handle, uint64_t obj_addr) {
  pthread_once(&rknpu_dev_once, rknpu_dev_init);

  // int ret;
  // struct rknpu_mem_destroy destroy = {
  //   .handle = handle ,
  //   .obj_addr = obj_addr
  // };

  // // ret = ioctl(fd, DRM_IOCTL_RKNPU_MEM_DESTROY, &destroy);
  // ret = rknpu_mem_destroy(rknpu_dev, &destroy);
  // if (ret < 0) {
  //   printf("RKNPU_MEM_DESTROY failed %d\n",ret);
  // }

  chcore_free_dma_mem((void *)handle);
}

void *npu_open(void) {

  // char buf1[256], buf2[256], buf3[256];

  // memset(buf1, 0 ,sizeof(buf1));
  // memset(buf2, 0 ,sizeof(buf2));
  // memset(buf3, 0, sizeof(buf3));

  // // Open DRI called "rknpu"
  // int fd = open("/dev/dri/card1", O_RDWR);
  // if(fd<0) {
  //   printf("Failed to open /dev/dri/card1 %d\n",errno);
  //   return fd;
  // }

  // struct drm_version dv;
  // memset(&dv, 0, sizeof(dv));
  // dv.name = buf1;
  // dv.name_len = sizeof(buf1);
  // dv.date = buf2;
  // dv.date_len = sizeof(buf2);
  // dv.desc = buf3;
  // dv.desc_len = sizeof(buf3);

  // int ret = ioctl(fd, DRM_IOCTL_VERSION, &dv);
  // if (ret <0) {
  //   printf("DRM_IOCTL_VERISON failed %d\n",ret);
  //   return ret;
  // }
  // printf("drm name is %s - %s - %s\n", dv.name, dv.date, dv.desc);
  // return fd;

  struct rknpu_device *rknpu_dev = NULL;
  unsigned long base_paddr[3] = {0xfdab0000, 0xfdac0000, 0xfdad0000};
  unsigned long iommu_base_paddr[4] = {0xfdab9000, 0xfdaba000, 0xfdaca000, 0xfdada000};
  unsigned long rknpu_irqs[3] = {142, 143, 144};
  
  rknpu_init(&rknpu_dev, base_paddr, iommu_base_paddr, rknpu_irqs);

  return rknpu_dev;
}

int npu_reset(void) {
  pthread_once(&rknpu_dev_once, rknpu_dev_init);

  // Reset the NPU
  // struct rknpu_action act = {
  //   .flags = RKNPU_ACT_RESET,
  // };
  // return ioctl(fd, DRM_IOCTL_RKNPU_ACTION, &act);	
  return rknpu_soft_reset(rknpu_dev);
}

int npu_submit(__u64 task_obj_addr, __u32 core_mask)
{
  pthread_once(&rknpu_dev_once, rknpu_dev_init);
  struct rknpu_submit submit = {
    .flags = RKNPU_JOB_PC | RKNPU_JOB_BLOCK | RKNPU_JOB_PINGPONG,
    .timeout = 6000,
    .task_start = 0,
    .task_number = 1,
    .task_counter = 0,
    .priority = 0,
    .task_obj_addr = task_obj_addr,
    .regcfg_obj_addr = 0,
    .task_base_addr = 0,
    .user_data = 0,
    .core_mask = core_mask,
    .fence_fd = -1,
    .subcore_task = {
      {0, 1}, {0, 1}, {0, 1}, {0, 0}, {0, 0}
    },
  };
  return rknpu_submit(rknpu_dev, &submit);
}

int npu_submit_multi(__u64 task_obj_addr[], int task_num, void *polling)
{
  assert(task_num <= 3);
  pthread_once(&rknpu_dev_once, rknpu_dev_init);
  struct rknpu_submit submit_base = {
    .flags = RKNPU_JOB_PC | RKNPU_JOB_BLOCK | RKNPU_JOB_PINGPONG,
    .timeout = 6000,
    .task_start = 0,
    .task_number = 1,
    .task_counter = 0,
    .priority = 0,
    // .task_obj_addr = task_obj_addr,
    .regcfg_obj_addr = 0,
    .task_base_addr = 0,
    .user_data = 0,
    // .core_mask = core_mask,
    .fence_fd = -1,
    .subcore_task = {
      {0, 1}, {0, 1}, {0, 1}, {0, 0}, {0, 0}
    },
  };
  struct rknpu_submit submits[task_num];
  struct rknpu_submit *submit_ptrs[task_num];
  for (int core_index = 0; core_index < task_num; core_index++) {
    memcpy(submits + core_index, &submit_base, sizeof(submit_base));
    submits[core_index].core_mask = (1 << core_index);
    submits[core_index].task_obj_addr = task_obj_addr[core_index];
    submit_ptrs[core_index] = &submits[core_index];
  }
  return rknpu_submit_multi(rknpu_dev, submit_ptrs, task_num, polling);
}

void npu_dump_measure(bool clear) {
  extern void rknpu_dump_measure(bool clear);
  rknpu_dump_measure(clear);
}
