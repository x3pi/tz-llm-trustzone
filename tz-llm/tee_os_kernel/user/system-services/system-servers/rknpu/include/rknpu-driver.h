#ifndef NPU_DRIVER_H
#define NPU_DRIVER_H

#include "rknpu-ioctl.h"

struct rknpu_device;

int rknpu_init(struct rknpu_device **rknpu_dev,
               unsigned long base_paddr[], unsigned long iommu_base_paddr[],
               unsigned long rknpu_irqs[]);

// int rknpu_mem_create(struct rknpu_device *rknpu_dev, struct rknpu_mem_create *data);

// int rknpu_mem_destroy(struct rknpu_device *rknpu_dev, struct rknpu_mem_destroy *data);

int rknpu_submit(struct rknpu_device *rknpu_dev, struct rknpu_submit *data);

int rknpu_soft_reset(struct rknpu_device *rknpu_dev);

#endif /* NPU_DRIVER_H */
