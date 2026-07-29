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

#include "gpio_if.h"
#include "hdf_log.h"
#include "prt_task.h"
#include "stm32f4xx_exti.h"
#include "gpio_test.h"

#define TEST_TIMES 10
#define TIME_COUNTER 5
#define TASK_DELAY_SHORT 500
#define TASK_DELAY_LONG 1000

static int32_t TestCaseGpioIrqHandler1(uint16_t gpio, void *data)
{
    HDF_LOGE("%s: irq triggered! on gpio:%u", __func__, gpio);
    uint16_t val = 0;
    GpioRead(GPIO_PIN_1, &val);
    if (val) {
        GpioWrite(GPIO_PIN_1, GPIO_PULL_DOWN);
    } else {
        GpioWrite(GPIO_PIN_1, GPIO_PULL_UP);
    }
    return HDF_SUCCESS;
}

static int32_t TestCaseGpioIrqHandler2(uint16_t gpio, void *data)
{
    HDF_LOGE("%s: irq triggered! on gpio:%u", __func__, gpio);
    uint16_t val = 0;
    GpioRead(GPIO_PIN_0, &val);
    if (val) {
        GpioWrite(GPIO_PIN_0, GPIO_PULL_DOWN);
    } else {
        GpioWrite(GPIO_PIN_0, GPIO_PULL_UP);
    }
    return HDF_SUCCESS;
}

static int32_t TestCaseGpioIrqHandler3(uint16_t gpio, void *data)
{
    HDF_LOGE("%s: irq triggered! on gpio:%u", __func__, gpio);
    uint16_t val = 0;
    GpioRead(GPIO_PIN_2, &val);
    if (val) {
        GpioWrite(GPIO_PIN_2, GPIO_PULL_DOWN);
    } else {
        GpioWrite(GPIO_PIN_2, GPIO_PULL_UP);
    }
    return HDF_SUCCESS;
}

static int32_t TestCaseGpioIrqHandler4(uint16_t gpio, void *data)
{
    EXTI_ClearITPendingBit(EXTI_Line1);
    EXTI_ClearITPendingBit(EXTI_Line2);
    EXTI_ClearITPendingBit(EXTI_Line3);
    EXTI_ClearITPendingBit(EXTI_Line4);
    EXTI_ClearITPendingBit(EXTI_Line0);
    return HDF_SUCCESS;
}

static int32_t TestCaseGpioDir(void)
{
    int32_t gpio;
    uint16_t dir;
    for (gpio = 0; gpio < GPIO_PIN_NUMS; gpio++) {
        int32_t ret = GpioGetDir(gpio, &dir);
        if (ret == HDF_SUCCESS && dir == GPIO_Mode_IN) {
            printf("HDF Gpio mode input.\n\r");
            ret = GpioSetDir(gpio, GPIO_Mode_OUT);
            if (ret != HDF_SUCCESS) {
                printf("    HDF GpioSetDir failed.\r\n");
                return HDF_FAILURE;
            }
        }
        if (ret == HDF_SUCCESS && dir == GPIO_Mode_OUT) {
            printf("HDF Gpio mode output.\n\r");
            ret = GpioSetDir(gpio, GPIO_Mode_IN);
            if (ret != HDF_SUCCESS) {
                printf("    HDF GpioSetDir failed.\r\n");
                return HDF_FAILURE;
            }
        }
        if (ret != HDF_SUCCESS) {
            printf("    HDF GpioGetDir failed.\r\n");
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

static int32_t TestCaseGpioSetIrq(void)
{
    int32_t ret;
    ret = GpioSetIrq(GPIO_PIN_3, EXTI_Trigger_Rising, TestCaseGpioIrqHandler1, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: set irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioSetIrq(GPIO_PIN_4, EXTI_Trigger_Rising, TestCaseGpioIrqHandler2, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: set irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioSetIrq(GPIO_PIN_5, EXTI_Trigger_Falling, TestCaseGpioIrqHandler3, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: set irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioSetIrq(GPIO_PIN_6, EXTI_Trigger_Falling, TestCaseGpioIrqHandler4, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: set irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t TestCaseGpioEnableIrq(void)
{
    int32_t ret;
    ret = GpioEnableIrq(GPIO_PIN_3);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: enable irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioEnableIrq(GPIO_PIN_4);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: enable irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioEnableIrq(GPIO_PIN_5);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: enable irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    ret = GpioEnableIrq(GPIO_PIN_6);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: enable irq fail! ret:%d\n", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t TestCaseGpioWriteRead(void)
{
    uint8_t val = 0;
    int32_t ret;
    uint16_t read;
    for (int gpio = 0; gpio < GPIO_PIN_NUMS; gpio++) {
        printf("GPIO test\r\n");
        if (val) {
            printf("val is %d\r\n", val);
            ret = GpioWrite(gpio, GPIO_PULL_DOWN);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%s: GpioWrite fail! ret:%d\n", __func__, ret);
                return HDF_FAILURE;
            }
            ret = GpioRead(gpio, &read);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%s: GpioRead fail! ret:%d\n", __func__, ret);
                return HDF_FAILURE;
            }
        } else {
            printf("val is %d\r\n", val);
            ret = GpioWrite(gpio, GPIO_PULL_UP);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%s: GpioWrite fail! ret:%d\n", __func__, ret);
                return HDF_FAILURE;
            }
            ret = GpioRead(gpio, &read);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%s: GpioRead fail! ret:%d\n", __func__, ret);
                return HDF_FAILURE;
            }
        }
        val = !val;
        PRT_TaskDelay(TASK_DELAY_SHORT);
        ret = GpioWrite(gpio, GPIO_PULL_DOWN);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: GpioWrite fail! ret:%d\n", __func__, ret);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

void HdfGpioTestAllEntry(void)
{
    int32_t timecounter;
    int32_t ret;
    int32_t testtimes;
    printf("GPIO test starts in 5secs!\r\n");
    for (timecounter = TIME_COUNTER; timecounter > 0; timecounter--) {
        PRT_TaskDelay(TASK_DELAY_LONG);
        printf("GPIO test starts in %dsecs!\r\n", timecounter);
    }
    printf("GPIO test init\r\n");
    for (testtimes = TEST_TIMES; testtimes >= 0; testtimes --) {
        ret = TestCaseGpioWriteRead();
        if (ret != HDF_SUCCESS) {
            printf("    TestCaseGpioWriteRead failed.\r\n");
        }
    }
    ret = TestCaseGpioDir();
    if (ret != HDF_SUCCESS) {
        printf("    TestCaseGpioDir failed.\r\n");
    }
    ret = TestCaseGpioSetIrq();
    if (ret != HDF_SUCCESS) {
        printf("    TestCaseGpioSetIrq failed.\r\n");
    }
    ret = TestCaseGpioEnableIrq();
    if (ret != HDF_SUCCESS) {
        printf("    TestCaseGpioEnableIrq failed.\r\n");
    }
    printf("gpio test finished!\r\n");
    printf("DONE.\r\n");
    while (1) {
        printf("    please press key0,key1,key2 or key up...\r\n");
        printf("    press key0 to enable bell.\r\n");
        printf("    press key1 to enable red light.\r\n");
        printf("    press key2 to enable green light.\r\n");
        printf("    press key up to quit irq.\r\n");
        PRT_TaskDelay(TASK_DELAY_LONG);
    }
}