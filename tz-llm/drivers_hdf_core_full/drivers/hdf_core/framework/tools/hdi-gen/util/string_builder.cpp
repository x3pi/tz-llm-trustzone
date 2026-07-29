/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "util/string_builder.h"

#include <cstdlib>
#include <cstring>

#include "securec.h"
#include "util/common.h"
#include "util/logger.h"
#include "util/string_helper.h"

namespace OHOS {
namespace HDI {
StringBuilder::~StringBuilder()
{
    if (buffer_ != nullptr) {
        free(buffer_);
    }
}

StringBuilder &StringBuilder::Append(char c)
{
    if (position_ + 1 >= capacity_) {
        if (!Grow(1)) {
            return *this;
        }
    }

    buffer_[position_] = c;
    position_ += 1;
    return *this;
}

StringBuilder &StringBuilder::Append(const char *string)
{
    if (string == nullptr || string[0] == '\0') {
        return *this;
    }

    size_t len = strlen(string);
    if (position_ + len >= capacity_) {
        if (!Grow(len)) {
            return *this;
        }
    }

    (void)memcpy_s(buffer_ + position_, capacity_ - position_, string, len);
    position_ += len;
    return *this;
}

StringBuilder &StringBuilder::Append(const std::string &string)
{
    if (string.empty()) {
        return *this;
    }

    size_t len = string.size();
    if (position_ + len >= capacity_) {
        if (!Grow(len)) {
            return *this;
        }
    }

    (void)memcpy_s(buffer_ + position_, capacity_ - position_, string.c_str(), len);
    position_ += len;
    return *this;
}

StringBuilder &StringBuilder::AppendFormat(const char *format, ...)
{
    va_list args;
    va_list argsCopy;

    va_start(args, format);
    va_copy(argsCopy, args);

    char buf[StringHelper::lineMaxSize] = {0};
    int len = vsnprintf_s(buf, StringHelper::lineMaxSize, StringHelper::lineMaxSize - 1, format, args);
    if (len <= 0) {
        va_end(args);
        va_end(argsCopy);
        return *this;
    }

    size_t writeSize = static_cast<size_t>(len);
    if (position_ + writeSize >= capacity_) {
        if (!Grow(writeSize)) {
            va_end(args);
            va_end(argsCopy);
            return *this;
        }
    }

    if (vsnprintf_s(buffer_ + position_, writeSize + 1, writeSize, format, argsCopy) < 0) {
        va_end(args);
        va_end(argsCopy);
        return *this;
    }
    position_ += writeSize;

    va_end(args);
    va_end(argsCopy);

    return *this;
}

bool StringBuilder::Grow(size_t size)
{
    if (capacity_ > StringHelper::maxSize) {
        Logger::E(TAG, "The StringBuilder is full.");
        return false;
    }
    // 256->the default capacity.
    size_t newSize = (capacity_ == 0) ? 256 : (capacity_ * 2);
    if (newSize < capacity_ + size) {
        newSize = capacity_ + size;
    }
    if (newSize > StringHelper::maxSize) {
        newSize = StringHelper::maxSize;
    }
    if (newSize <= capacity_) {
        return false;
    }

    char *newBuffer = reinterpret_cast<char *>(calloc(newSize, 1));
    if (newBuffer == nullptr) {
        Logger::E(TAG, "Fail to malloc %lu bytes memory.", newSize);
        return false;
    }

    if (buffer_ != nullptr) {
        (void)memcpy_s(newBuffer, newSize, buffer_, capacity_);
        free(buffer_);
    }
    buffer_ = newBuffer;
    capacity_ = newSize;
    return true;
}

std::string StringBuilder::ToString() const
{
    return std::string(buffer_, position_);
}
} // namespace HDI
} // namespace OHOS