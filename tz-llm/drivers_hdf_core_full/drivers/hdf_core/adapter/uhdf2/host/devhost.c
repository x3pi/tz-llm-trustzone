/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <unistd.h>

#include "securec.h"
#include "parameter.h"
#include "devhost_dump.h"
#include "devhost_service.h"
#include "devhost_service_full.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "hdf_power_manager.h"

#define HDF_LOG_TAG                    hdf_device_host
#define DEVHOST_INPUT_PARAM_NUM        3
#define DEVHOST_PARAM_NUM_WITH_PRI     5
#define DEVHOST_INPUT_PARAM_HOSTID_POS 1
#define DEVHOST_HOST_NAME_POS          2
#define DEVHOST_PROCESS_PRI_POS        3
#define DEVHOST_THREAD_PRI_POS         4
#define PARAM_BUF_LEN 128

static void StartMemoryHook(const char* processName)
{
    const char defaultValue[PARAM_BUF_LEN] = { 0 };
    const char targetPrefix[] = "startup:";
    const int targetPrefixLen = strlen(targetPrefix);
    char paramValue[PARAM_BUF_LEN] = { 0 };
    int retParam = GetParameter("libc.hook_mode", defaultValue, paramValue, sizeof(paramValue));
    if (retParam <= 0 || strncmp(paramValue, targetPrefix, targetPrefixLen) != 0) {
        return;
    }
    if (strstr(paramValue + targetPrefixLen, processName) != NULL) {
        const int hookSignal = 36; // 36: native memory hooked signal
        HDF_LOGE("raise hook signal %{public}d to %{public}s", hookSignal, processName);
        raise(hookSignal);
    }
}

bool HdfStringToInt(const char *str, int *value)
{
    if (str == NULL || value == NULL) {
        return false;
    }

    char *end = NULL;
    errno = 0;
    const int base = 10;
    long result = strtol(str, &end, base);
    if (end == str || end[0] != '\0' || errno == ERANGE || result > INT_MAX || result < INT_MIN) {
        return false;
    }

    *value = (int)result;
    return true;
}

static void SetProcTitle(char **argv, const char *newTitle)
{
    size_t len = strlen(argv[0]);
    if (strlen(newTitle) > len) {
        HDF_LOGE("failed to set new process title because the '%{public}s' is too long", newTitle);
        return;
    }
    (void)memset_s(argv[0], len, 0, len);
    if (strcpy_s(argv[0], len + 1, newTitle) != EOK) {
        HDF_LOGE("failed to set new process title");
        return;
    }
    prctl(PR_SET_NAME, newTitle);
}

static void HdfSetProcPriority(char **argv)
{
    int32_t schedPriority = 0;
    int32_t procPriority = 0;

    if (!HdfStringToInt(argv[DEVHOST_PROCESS_PRI_POS], &procPriority)) {
        HDF_LOGE("procPriority parameter error: %{public}s", argv[DEVHOST_PROCESS_PRI_POS]);
        return;
    }
    if (!HdfStringToInt(argv[DEVHOST_THREAD_PRI_POS], &schedPriority)) {
        HDF_LOGE("schedPriority parameter error: %{public}s", argv[DEVHOST_THREAD_PRI_POS]);
        return;
    }

    if (setpriority(PRIO_PROCESS, 0, procPriority) != 0) {
        HDF_LOGE("host setpriority failed: %{public}d", errno);
    }

    struct sched_param param = {0};
    param.sched_priority = schedPriority;
    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        HDF_LOGE("host sched_setscheduler failed: %{public}d", errno);
    } else {
        HDF_LOGI("host sched_setscheduler succeed:%{public}d %{public}d", procPriority, schedPriority);
    }
}

int main(int argc, char **argv)
{
    if ((argc != DEVHOST_INPUT_PARAM_NUM) && (argc != DEVHOST_PARAM_NUM_WITH_PRI)) {
        HDF_LOGE("Devhost main parameter error, argc: %{public}d", argc);
        return HDF_ERR_INVALID_PARAM;
    }

    prctl(PR_SET_PDEATHSIG, SIGKILL); // host process should exit with devmgr process

    int hostId = 0;
    if (!HdfStringToInt(argv[DEVHOST_INPUT_PARAM_HOSTID_POS], &hostId)) {
        HDF_LOGE("Devhost main parameter error, argv[1]: %{public}s", argv[DEVHOST_INPUT_PARAM_HOSTID_POS]);
        return HDF_ERR_INVALID_PARAM;
    }

    const char *hostName = argv[DEVHOST_HOST_NAME_POS];
    HDF_LOGI("hdf device host %{public}s %{public}d start", hostName, hostId);
    SetProcTitle(argv, hostName);
    StartMemoryHook(hostName);
    if (argc == DEVHOST_PARAM_NUM_WITH_PRI) {
        HdfSetProcPriority(argv);
    }

    struct IDevHostService *instance = DevHostServiceNewInstance(hostId, hostName);
    if (instance == NULL || instance->StartService == NULL) {
        HDF_LOGE("DevHostServiceGetInstance fail");
        return HDF_ERR_INVALID_OBJECT;
    }

    DevHostDumpInit();
    int status = instance->StartService(instance);
    if (status != HDF_SUCCESS) {
        HDF_LOGE("Devhost StartService fail, return: %{public}d", status);
        DevHostServiceFreeInstance(instance);
        DevHostDumpDeInit();
        return status;
    }

    HdfPowerManagerInit();
    struct DevHostServiceFull *fullService = (struct DevHostServiceFull *)instance;
    struct HdfMessageLooper *looper = &fullService->looper;
    if ((looper != NULL) && (looper->Start != NULL)) {
        looper->Start(looper);
    }


    HdfPowerManagerExit();
    DevHostDumpDeInit();
    HDF_LOGI("hdf device host %{public}s %{public}d exit", hostName, hostId);
    if (strcmp(hostName, "camera_host") == 0) {
        _exit(status);
    }
    return status;
}
