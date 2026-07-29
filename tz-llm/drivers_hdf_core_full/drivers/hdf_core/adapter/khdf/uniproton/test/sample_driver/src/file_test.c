/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include "prt_fs.h"
#include "fcntl.h"
#include "file_test.h"

#define TEST_FILE_PERMISSION 0777

void fs_test1(void)
{
    char tmpFileName[] = "/data/FILE0";

    int fd;
    int ret;
    fd = open(tmpFileName, O_CREAT | O_RDWR, TEST_FILE_PERMISSION);
    printf("open '%d\n", fd);

    (void)close(fd);

    ret = unlink(tmpFileName);
    printf("unlink '%d\n", ret);

    ret = mkdir("/data/FILE0", TEST_FILE_PERMISSION);
    printf("ret = %d\r\n", ret);
}

void fs_test2(void)
{
    DIR *dirp = NULL;
    dirp = opendir("/data");

    struct dirent *d = NULL;
    d = readdir(dirp);
    if (d != NULL) {
        printf("readdir %s\n", d->d_name);
    } else {
        printf("readdir null\n");
    }

    d = readdir(dirp);
    if (d != NULL) {
        printf("readdir %s\n", d->d_name);
    } else {
        printf("readdir null\n");
    }
    d = readdir(dirp);
    if (d != NULL) {
        printf("readdir %s\n", d->d_name);
    } else {
        printf("readdir null\n");
    }

    d = readdir(dirp);
    if (d != NULL) {
        printf("readdir %s\n", d->d_name);
    } else {
        printf("readdir null\n");
    }

    closedir(dirp);
}

#define TEST_WR_BUF_LEN 40
#define TEST_WR_LEN_20 20
#define TEST_WR_LEN_5 20
#define TEST_WR_LEN_10 20
#define TEST_LOOKUP_TIME 40
void fs_test3(void)
{
    int wfd;
    char writeBuf[TEST_WR_BUF_LEN] = "write test";
    char readBuf[TEST_WR_BUF_LEN];
    ssize_t size = 0;
    int i;

    wfd = open("/data/test", O_CREAT | O_RDWR, TEST_FILE_PERMISSION);
    if (wfd < 0) {
        printf("!!!!!!!!!!!!!!!!!! 2\r\n");
    }

    size = write(wfd, writeBuf, strlen(writeBuf));
    printf("write %d, return %d\n", strlen(writeBuf), size);

    size = write(wfd, writeBuf, strlen(writeBuf) + TEST_WR_LEN_20);
    printf("write %d, return %d\n", strlen(writeBuf) + TEST_WR_LEN_20, size);

    size = write(wfd, writeBuf, TEST_WR_LEN_10);
    printf("write %d, return %d\n", TEST_WR_LEN_10, size);

    for (i = 1; i < TEST_LOOKUP_TIME; i++) {
        size = write(wfd, writeBuf, i);
        printf("write %d, return %d\n", i, size);
    }
    (void)close(wfd);

    wfd = open("/data/test", O_CREAT | O_RDWR, TEST_FILE_PERMISSION);
    if (wfd < 0) {
        printf("!!!!!!!!!!!!!!!!!! 2\r\n");
    }

    size = read(wfd, readBuf, strlen(writeBuf));
    printf("read %s,  return %d\n", readBuf, size);
    size = read(wfd, readBuf, strlen(writeBuf) + TEST_WR_LEN_20);
    printf("read %s,  return %d\n", readBuf, size);
    size = read(wfd, readBuf, TEST_WR_LEN_10);
    printf("read %s,  return %d\n", readBuf, size);

    for (i = 1; i < TEST_LOOKUP_TIME; i++) {
        size = read(wfd, readBuf, i);
        printf("read %s,  return %d\n", readBuf, size);
    }

    (void)close(wfd);
}

void LfsTest(void)
{
    fs_test1();
    fs_test2();
    fs_test3();
}