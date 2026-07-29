/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef CLOCK_CORE_H
#define CLOCK_CORE_H

#include "platform_core.h"
#include "osal_spinlock.h"
#include "hdf_base.h"
#include "clock_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define CLOCK_DEVICES_MAX 61
#define CLOCK_HANDLE_SHIFT 0xFF00U

struct ClockDevice;
struct ClockMethod;
struct ClockLockMethod;

struct ClockDevice {
    const struct ClockMethod *ops;
    OsalSpinlock spin;
    const char *deviceName;
    const char *clockName;
    uint32_t deviceIndex;
    const struct ClockLockMethod *lockOps;
    void *clk;
    void *priv;
    struct ClockDevice *parent;
};

struct ClockMethod {
    int32_t (*start)(struct ClockDevice *device);
    int32_t (*stop)(struct ClockDevice *device);
    int32_t (*setRate)(struct ClockDevice *device, uint32_t rate);
    int32_t (*getRate)(struct ClockDevice *device, uint32_t *rate);
    int32_t (*disable)(struct ClockDevice *device);
    int32_t (*enable)(struct ClockDevice *device);
    struct ClockDevice *(*getParent)(struct ClockDevice *device);
    int32_t (*setParent)(struct ClockDevice *device, struct ClockDevice *parent);
};

struct ClockLockMethod {
    int32_t (*lock)(struct ClockDevice *device);
    void (*unlock)(struct ClockDevice *device);
};

int32_t ClockDeviceAdd(struct ClockDevice *device);

int32_t ClockManagerGetAIdleDeviceId();

void ClockDeviceRemove(struct ClockDevice *device);

struct ClockDevice *ClockDeviceGet(uint32_t number);

struct ClockDevice *ClockDeviceOpen(uint32_t number);

int32_t ClockDeviceClose(struct ClockDevice *device);

int32_t ClockDeviceEnable(struct ClockDevice *device);

int32_t ClockDeviceDisable(struct ClockDevice *device);

int32_t ClockDeviceSetRate(struct ClockDevice *device, uint32_t rate);

int32_t ClockDeviceGetRate(struct ClockDevice *device, uint32_t *rate);

struct ClockDevice *ClockDeviceGetParent(struct ClockDevice *device);

int32_t ClockDeviceSetParent(struct ClockDevice *device, struct ClockDevice *parent);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* CLOCK_CORE_H */
