/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include "platform_trace.h"

const char *PlatformTraceModuleExplainGet(PLATFORM_TRACE_MODULE traceModule)
{
    (void)traceModule;
    return NULL;
}

const char *PlatformTraceFunExplainGet(PLATFORM_TRACE_MODULE_FUN traceFun)
{
    (void)traceFun;
    return NULL;
}

const struct PlatformTraceModuleExplain *PlatformTraceModuleExplainGetAll(void)
{
    return NULL;
}

int32_t PlatformTraceModuleExplainCount(void)
{
    return 0;
}

int32_t PlatformTraceStart(void)
{
    return HDF_SUCCESS;
}

int32_t PlatformTraceStop(void)
{
    return HDF_SUCCESS;
}

void PlatformTraceInfoDump(void)
{
    return;
}

void PlatformTraceReset(void)
{
    return;
}

void PlatformTraceAddMsg(const char *module, const char *moduleFun, const char *fmt, ...)
{
    (void)module;
    (void)moduleFun;
    (void)fmt;
}

void PlatformTraceAddUintMsg(int module, int moduleFun, const unsigned int infos[], uint8_t size)
{
    (void)module;
    (void)moduleFun;
    (void)infos;
    (void)size;
}
