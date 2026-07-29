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

#include <stdio.h>

#include "i2c_if.h"
#include "hdf_log.h"
#include "prt_task.h"
#include "i2c_test.h"

#define I2C_READ_MSG_NUM   2
#define I2C_WRITE_MSG_NUM  1
#define EEPROM_WAIT_TIME   5000
#define EEPROM_ADDR        0xA0

static int I2cReadData(DevHandle handle, uint8_t *writeBuf, uint16_t writeLen, uint8_t *readBuf, uint16_t readLen)
{
    struct I2cMsg msg[2];
    (void)memset_s(msg, sizeof(msg), 0, sizeof(msg));

    msg[0].addr = EEPROM_ADDR;
    msg[0].flags = 0;
    msg[0].len = writeLen;
    msg[0].buf = writeBuf;

    msg[1].addr = EEPROM_ADDR;
    msg[1].flags = I2C_FLAG_READ;
    msg[1].len = readLen;
    msg[1].buf = readBuf;

    if (I2cTransfer(handle, msg, I2C_READ_MSG_NUM) != I2C_READ_MSG_NUM) {
        HDF_LOGE("%s: i2c read err\n", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int I2cWriteData(DevHandle handle, uint8_t *writeBuf, uint16_t writeLen)
{
    struct I2cMsg msg[1];
    (void)memset_s(msg, sizeof(msg), 0, sizeof(msg));

    msg[0].addr = EEPROM_ADDR;
    msg[0].flags = 0;
    msg[0].len = writeLen;
    msg[0].buf = writeBuf;

    if (I2cTransfer(handle, msg, I2C_WRITE_MSG_NUM) != I2C_WRITE_MSG_NUM) {
        HDF_LOGE("%s: i2c write err\n", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t TestCaseI2cWriteRead(void)
{
    int32_t ret;
    unsigned char busId = 1;

    DevHandle i2cHandle = I2cOpen(busId);
    if (i2cHandle == NULL) {
        HDF_LOGE("%s: Open I2c:%u fail!\n", __func__, busId);
        return HDF_FAILURE;
    }

    unsigned char writeBuf[9] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xaa, 0xbb, 0xcc, 0xdd};
    for (int i = 0; i < sizeof(writeBuf); i++) {
        printf("0x%x, ", writeBuf[i]);
    }
    printf("\r\n");

    ret = I2cWriteData(i2cHandle, writeBuf, sizeof(writeBuf));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: I2cWriteData fail! ret:%d\n", __func__, ret);
        I2cClose(i2cHandle);
        return HDF_FAILURE;
    }

    PRT_TaskDelay(EEPROM_WAIT_TIME);

    unsigned char readBuf[8] = {0};
    uint8_t regAddr = 0x00;

    ret = I2cReadData(i2cHandle, &regAddr, 1, readBuf, sizeof(readBuf));
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: I2cReadData fail! ret:%d\n", __func__, ret);
        I2cClose(i2cHandle);
        return HDF_FAILURE;
    }

    for (int i = 0; i < sizeof(readBuf); i++) {
        printf("0x%x, ", readBuf[i]);
    }
    printf("\r\n");

    I2cClose(i2cHandle);

    for (int i = 0; i < sizeof(readBuf); i++) {
        if (readBuf[i] != writeBuf[i + 1]) {
            HDF_LOGE("%s: I2cWriteReadData fail! i:%d, read=0x%x, write=0x%x\n", __func__, i, readBuf[i], writeBuf[i]);
        }
    }
    printf("\r\n");

    return HDF_SUCCESS;
}

void HdfI2cTestAllEntry(void)
{
    int32_t ret;
    printf("I2C test init\r\n");
    ret = TestCaseI2cWriteRead();
    if (ret != HDF_SUCCESS) {
        printf("    TestCaseI2cWriteRead failed.\r\n");
    }
    printf("I2C test finished!\r\n");
    printf("DONE.\r\n");
}