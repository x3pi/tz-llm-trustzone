/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef OHOS_HDI_FILE_H
#define OHOS_HDI_FILE_H

#include <cstdio>
#include <set>
#include <string>

namespace OHOS {
namespace HDI {
class File {
public:
    File(const std::string &path, unsigned int mode);

    ~File();

    void OpenByRead(const std::string &path);

    inline bool IsValid() const
    {
        return fd_ != nullptr;
    }

    inline std::string GetPath() const
    {
        return path_;
    }

    char GetChar();

    char PeekChar();

    bool IsEof() const;

    inline size_t GetCharLineNumber() const
    {
        return lineNo_;
    }

    inline size_t GetCharColumnNumber() const
    {
        return columnNo_;
    }

    size_t ReadData(void *data, size_t size) const;

    bool WriteData(const void *data, size_t size) const;

    void Flush() const;

    bool Reset() const;

    bool Skip(long size) const;

    void Close();

    static bool CreateParentDir(const std::string &path);

    static std::string AdapterPath(const std::string &path);

    static std::string AdapterRealPath(const std::string &path);

    static std::string RealPath(const std::string &path);

    static bool CheckValid(const std::string &path);

    static std::set<std::string> FindFiles(const std::string &rootDir);

    size_t GetHashKey();

    static constexpr unsigned int READ = 0x1;
    static constexpr unsigned int WRITE = 0x2;
    static constexpr unsigned int APPEND = 0x4;

private:
    size_t Read();

    static constexpr int BUFFER_SIZE = 1024;

    char buffer_[BUFFER_SIZE] = {0};
    size_t size_ = 0;
    size_t position_ = 0;
    size_t columnNo_ = 1;
    size_t lineNo_ = 1;
    bool isEof_ = false;
    bool isError_ = false;

    FILE *fd_ = nullptr;
    std::string path_;
    unsigned int mode_ = 0;
};
} // namespace HDI
} // namespace OHOS

#endif // OHOS_HDI_FILE_H