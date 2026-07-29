/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

/**
 * @addtogroup DriverHdi
 * @{
 *
 * @brief Provides APIs for a system ability to obtain hardware device interface (HDI) services,
 * load or unload a device, and listen for service status, and capabilities for the hdi-gen tool to
 * automatically generate code in interface description language (IDL).
 *
 * The HDF and IDL code generated allow the system ability to accesses HDI driver services.
 *
 * @since 1.0
 */

/**
 * @file hdi_smq_syncer.h
 *
 * @brief Provides communication mechanisms, including the wait() and wake() APIs, for the shared memory queue (SMQ).
 *
 * @since 1.0
 */

#ifndef HDI_SHARED_MEM_QUEUE_SYNCER_H
#define HDI_SHARED_MEM_QUEUE_SYNCER_H

#include <atomic>
#include <parcel.h>
#include <cstdint>

namespace OHOS {
namespace HDI {
namespace Base {
/**
 * @brief Defines the <b>SharedMemQueueSyncer</b> class.
 */
class SharedMemQueueSyncer {
public:
    explicit SharedMemQueueSyncer(std::atomic<uint32_t> *syncerPtr);
    ~SharedMemQueueSyncer() = default;
    /**
     * @brief Enumerates the synchronization types.
     */
    enum SyncWord : uint32_t {
        /** Synchronous write */
        SYNC_WORD_WRITE = 0x01,
        /** Synchronous read */
        SYNC_WORD_READ = 0x02,
    };

    /**
     * @brief Waits until a certain condition becomes true.
     * This API will invoke the private member function <b>FutexWait</b>.
     *
     * @param bitset Indicates the synchronization type.
     * @param timeoutNanoSec Indicates the time to wait, in nanoseconds.
     * @return Returns <b>0</b> if the caller is woken up.
     */
    int Wait(uint32_t bitset, int64_t timeoutNanoSec);

    /**
     * @brief Wakes up a waiter.
     *
     * @param bitset Indicates the synchronization type.
     * @return Returns <b>0</b> if the waiter is woken up.
     */
    int Wake(uint32_t bitset);

private:
    /**
     * @brief Waits until a certain condition becomes true.
     *
     * @param bitset Indicates the synchronization type.
     * @param timeoutNanoSec Indicates the time to wait, in nanoseconds.
     * @return Returns <b>0</b> if the caller is woken up.
     */
    int FutexWait(uint32_t bitset, int64_t timeoutNanoSec);

    /**
     * @brief Converts the wait time to real time.
     *
     * @param timeout Indicates the wait time.
     * @param realtime Indicates the real time.
     */
    void TimeoutToRealtime(int64_t timeout, struct timespec &realtime);
    std::atomic<uint32_t> *syncAddr_;
};
} // namespace Base
} // namespace HDI
} // namespace OHOS

#endif /* HDI_SHARED_MEM_QUEUE_SYNCER_H */
