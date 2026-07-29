/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_COMMON_H
#define OHOS_HDI_COMMON_H

namespace OHOS {
namespace HDI {
    constexpr const char *TAB = "    ";
    constexpr const char *TAG = "HDI-GEN";

#ifndef __MINGW32__
    constexpr char SEPARATOR = '/';
#else
    constexpr char SEPARATOR = '\\';
#endif

    constexpr const char *MAX_BUFF_SIZE_MACRO = "HDI_BUFF_MAX_SIZE";
    constexpr const char *MAX_BUFF_SIZE_VALUE = "1024 * 200";    // 200KB
    constexpr const char *CHECK_VALUE_RETURN_MACRO = "HDI_CHECK_VALUE_RETURN";
    constexpr const char *CHECK_VALUE_RET_GOTO_MACRO = "HDI_CHECK_VALUE_RET_GOTO";

    enum class SystemLevel {
        /** mini system */
        MINI,
        /** lite system */
        LITE,
        /** std system */
        FULL,
    };

    enum class GenMode {
        /** generate hdi code of low mode, it is only supported by 'MINI' SystemLevel */
        LOW,
        /** generate hdi code of pass through mode, it is only supported by 'LITE' or 'std' SystemLevel */
        PASSTHROUGH,
        /** generate hdi code of ipc mode, it is only supported by 'std' SystemLevel */
        IPC,
        /** generate hdi code of kernel mode, it is only supported by 'LITE' or 'std' SystemLevel */
        KERNEL,
    };

    enum class Language {
        C,
        CPP,
        JAVA,
    };
} // namespace HDI
} // namespace OHOS
#endif // OHOS_HDI_COMMON_H
