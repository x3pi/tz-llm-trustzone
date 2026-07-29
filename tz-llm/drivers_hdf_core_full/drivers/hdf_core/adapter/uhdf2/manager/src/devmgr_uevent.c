/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <linux/netlink.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "devmgr_service.h"
#include "hdf_dlist.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "osal_thread.h"
#include "osal_time.h"
#include "securec.h"

#define HDF_LOG_TAG devmgr_uevent

#define UEVENT_SOCKET_BUFF_SIZE (256 * 1024)

#ifndef LINE_MAX
#define LINE_MAX 4096
#endif

#define ACTION_STR          "ACTION=="
#define PNP_EVENT_STR       "HDF_PNP_EVENT="
#define KEY_VALUE_DELIMITER "==\""
#define KEY_DELIMITER       "\","
#define UEVENT_DELIMITER    "="

enum DEVMGR_ACTION_TYPE {
    DEVMGR_ACTION_LOAD,
    DEVMGR_ACTION_UNLOAD,
    DEVMGR_ACTION_MAX,
};

struct DevMgrUeventRuleCfg {
    char *serviceName;
    enum DEVMGR_ACTION_TYPE action;
    struct DListHead matchKeyList;
    struct DListHead entry;
};

struct DevMgrMatchKey {
    char *key;
    char *value;
    struct DListHead entry;
};

static struct DListHead *DevMgrUeventRuleCfgList(void)
{
    static struct DListHead ruleCfgList;
    static bool initFlag = false;

    if (!initFlag) {
        DListHeadInit(&ruleCfgList);
        initFlag = true;
    }

    return &ruleCfgList;
}

static void DevMgrUeventReleaseKeyList(struct DListHead *keyList)
{
    struct DevMgrMatchKey *matchKey = NULL;
    struct DevMgrMatchKey *matchKeyTmp = NULL;

    DLIST_FOR_EACH_ENTRY_SAFE(matchKey, matchKeyTmp, keyList, struct DevMgrMatchKey, entry) {
        DListRemove(&matchKey->entry);
        OsalMemFree(matchKey->key);
        OsalMemFree(matchKey->value);
        OsalMemFree(matchKey);
    }
}

static void DevMgrUeventReleaseRuleCfgList(void)
{
    struct DevMgrUeventRuleCfg *ruleCfg = NULL;
    struct DevMgrUeventRuleCfg *ruleCfgTmp = NULL;
    struct DListHead *ruleCfgList = DevMgrUeventRuleCfgList();

    DLIST_FOR_EACH_ENTRY_SAFE(ruleCfg, ruleCfgTmp, ruleCfgList, struct DevMgrUeventRuleCfg, entry) {
        DListRemove(&ruleCfg->entry);
        DevMgrUeventReleaseKeyList(&ruleCfg->matchKeyList);
        OsalMemFree(ruleCfg->serviceName);
        OsalMemFree(ruleCfg);
    }
}

// replace substrings "\n" or "\r" in a string with characters '\n' or '\r'
static void DevMgrUeventReplaceLineFeed(char *str, const char *subStr, char c)
{
    char *ptr = NULL;

    while ((ptr = strstr(str, subStr)) != NULL) {
        ptr[0] = c;
        uint32_t i = 1;
        const size_t len = strlen(ptr) - 1;
        for (; i < len; i++) {
            ptr[i] = ptr[i + 1];
        }
        ptr[i] = '\0';
    }

    return;
}

static int32_t DevMgrUeventParseKeyValue(char *str, struct DevMgrMatchKey *matchKey, const char *sep)
{
    char *subPtr = strstr(str, sep);
    if (subPtr == NULL) {
        if (strstr(str, "@/") == NULL) {
            HDF_LOGE("parse Key value failed:[%{public}s]", str);
        }
        return HDF_FAILURE;
    }

    if (strlen(subPtr) < (strlen(sep) + 1)) {
        HDF_LOGE("value part invalid:[%{public}s]", str);
        return HDF_FAILURE;
    }

    char *value = subPtr + strlen(sep);
    *subPtr = '\0';
    HDF_LOGD("key:%{public}s,value:[%{public}s]", str, value);
    matchKey->key = strdup(str);
    matchKey->value = strdup(value);

    return HDF_SUCCESS;
}

static bool DevMgrUeventCheckRuleValid(const char *line)
{
    if (strlen(line) < strlen(PNP_EVENT_STR) || line[0] == '#') {
        return false;
    }

    if (strstr(line, PNP_EVENT_STR) == NULL) {
        HDF_LOGE("%s invalid rule: %s", __func__, line);
        return false;
    }

    return true;
}

static int32_t DevMgrUeventParseMatchKey(char *subStr, struct DListHead *matchKeyList)
{
    if (subStr == NULL || strlen(subStr) == 0) {
        HDF_LOGE("match key invalid [%{public}s]", subStr);
        return HDF_FAILURE;
    }

    struct DevMgrMatchKey *matchKey = (struct DevMgrMatchKey *)OsalMemCalloc(sizeof(*matchKey));
    if (matchKey == NULL) {
        HDF_LOGE("%{public}s OsalMemCalloc matchKey failed", __func__);
        return HDF_FAILURE;
    }
    DListHeadInit(&matchKey->entry);

    if (DevMgrUeventParseKeyValue(subStr, matchKey, KEY_VALUE_DELIMITER) == HDF_SUCCESS) {
        DevMgrUeventReplaceLineFeed(matchKey->value, "\\n", '\n');
        DevMgrUeventReplaceLineFeed(matchKey->value, "\\r", '\r');
        DListInsertTail(&matchKey->entry, matchKeyList);
        return HDF_SUCCESS;
    } else {
        OsalMemFree(matchKey);
        return HDF_FAILURE;
    }
}

static int32_t DevMgrUeventParseHdfEvent(char *subStr, struct DevMgrUeventRuleCfg *ruleCfg)
{
    if (strncmp(subStr, PNP_EVENT_STR, strlen(PNP_EVENT_STR)) != 0 || strlen(subStr) < strlen(PNP_EVENT_STR) + 1) {
        HDF_LOGE("parse hdf event %{public}s failed", subStr);
        return HDF_FAILURE;
    }

    char *event = subStr + strlen(PNP_EVENT_STR);
    char *ptr = strchr(event, ':');
    if (ptr == NULL) {
        HDF_LOGE("parse event %{public}s fail, no action", subStr);
        return HDF_FAILURE;
    }

    if (strlen(ptr) < strlen(":load")) {
        HDF_LOGE("event action part invalid [%{public}s]", ptr);
        return HDF_FAILURE;
    }

    *ptr = '\0';
    char *action = ptr + 1;

    // replace line feed
    ptr = strchr(action, '\n');
    if (ptr != NULL) {
        *ptr = '\0';
    }

    // replace carriage return
    ptr = strchr(action, '\r');
    if (ptr != NULL) {
        *ptr = '\0';
    }

    if (strcmp(action, "load") == 0) {
        ruleCfg->action = DEVMGR_ACTION_LOAD;
    } else if (strcmp(action, "unload") == 0) {
        ruleCfg->action = DEVMGR_ACTION_UNLOAD;
    } else {
        HDF_LOGE("parse event action fail [%{public}s]", action);
        return HDF_FAILURE;
    }

    HDF_LOGD("event:%{public}s:%{public}d\n", event, ruleCfg->action);
    ruleCfg->serviceName = strdup(event);
    if (DListGetCount(&ruleCfg->matchKeyList) == 0) {
        HDF_LOGE("parse failed, no match key");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t DevMgrUeventParseRule(char *line)
{
    if (!DevMgrUeventCheckRuleValid(line)) {
        return HDF_FAILURE;
    }

    struct DevMgrUeventRuleCfg *ruleCfg = (struct DevMgrUeventRuleCfg *)OsalMemCalloc(sizeof(*ruleCfg));
    if (ruleCfg == NULL) {
        HDF_LOGE("%{public}s OsalMemCalloc ruleCfg failed", __func__);
        return HDF_FAILURE;
    }
    DListHeadInit(&ruleCfg->matchKeyList);

    char *ptr = line;
    char *subStr = line;
    while (ptr != NULL && *ptr != '\0') {
        ptr = strstr(ptr, KEY_DELIMITER);
        if (ptr != NULL) {
            *ptr = '\0';
            if (DevMgrUeventParseMatchKey(subStr, &ruleCfg->matchKeyList) != HDF_SUCCESS) {
                goto FAIL;
            }
            ptr += strlen(KEY_DELIMITER);
            subStr = ptr;
        } else {
            if (DevMgrUeventParseHdfEvent(subStr, ruleCfg) != HDF_SUCCESS) {
                goto FAIL;
            }
            DListInsertTail(&ruleCfg->entry, DevMgrUeventRuleCfgList());
        }
    }

    return HDF_SUCCESS;

FAIL:
    DevMgrUeventReleaseKeyList(&ruleCfg->matchKeyList);
    OsalMemFree(ruleCfg->serviceName);
    OsalMemFree(ruleCfg);
    return HDF_FAILURE;
}

static void DevMgrUeventDisplayRuleCfgList(void)
{
    struct DevMgrUeventRuleCfg *ruleCfg = NULL;
    struct DevMgrMatchKey *matchKey = NULL;
    struct DListHead *ruleCfgList = DevMgrUeventRuleCfgList();

    DLIST_FOR_EACH_ENTRY(ruleCfg, ruleCfgList, struct DevMgrUeventRuleCfg, entry) {
        HDF_LOGD("service:%{public}s action:%{public}d", ruleCfg->serviceName, ruleCfg->action);
        DLIST_FOR_EACH_ENTRY(matchKey, &ruleCfg->matchKeyList, struct DevMgrMatchKey, entry) {
            HDF_LOGD("key:%{public}s value:%{public}s", matchKey->key, matchKey->value);
        }
    }
}

static int32_t DevMgrUeventParseConfig(void)
{
    char path[PATH_MAX] = {0};
    int ret = sprintf_s(path, PATH_MAX - 1, "%s/hdf_pnp.cfg", HDF_CONFIG_DIR);
    if (ret < 0) {
        HDF_LOGE("%{public}s generate file path failed", __func__);
        return HDF_FAILURE;
    }

    char resolvedPath[PATH_MAX] = {0};
    if (realpath(path, resolvedPath) == NULL) {
        HDF_LOGE("realpath file: %{public}s failed, errno:%{public}d", path, errno);
        return HDF_FAILURE;
    }
    if (strncmp(resolvedPath, HDF_CONFIG_DIR, strlen(HDF_CONFIG_DIR)) != 0) {
        HDF_LOGE("%{public}s invalid path %{public}s", __func__, resolvedPath);
        return HDF_FAILURE;
    }
    FILE *file = fopen(resolvedPath, "r");
    if (file == NULL) {
        HDF_LOGE("%{public}s fopen %{public}s failed:%{public}d", __func__, resolvedPath, errno);
        return HDF_FAILURE;
    }

    char line[LINE_MAX] = {0};
    while (fgets(line, sizeof(line), file) != NULL) {
        HDF_LOGD("%{public}s, %{public}s", __func__, line);
        DevMgrUeventParseRule(line);
        (void)memset_s(line, sizeof(line), 0, sizeof(line));
    }

    (void)fclose(file);
    DevMgrUeventDisplayRuleCfgList();

    return HDF_SUCCESS;
}

static int32_t DevMgrUeventSocketInit(void)
{
    struct sockaddr_nl addr;
    (void)memset_s(&addr, sizeof(addr), 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = (__u32)getpid();
    addr.nl_groups = 0xffffffff;

    int32_t sockfd = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, NETLINK_KOBJECT_UEVENT);
    if (sockfd < 0) {
        HDF_LOGE("create socket failed, err = %{public}d", errno);
        return HDF_FAILURE;
    }

    int32_t buffSize = UEVENT_SOCKET_BUFF_SIZE;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffSize, sizeof(buffSize)) != 0) {
        HDF_LOGE("setsockopt: SO_RCVBUF failed err = %{public}d", errno);
        close(sockfd);
        return HDF_FAILURE;
    }

    const int32_t on = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) != 0) {
        HDF_LOGE("setsockopt: SO_PASSCRED failed, err = %{public}d", errno);
        close(sockfd);
        return HDF_FAILURE;
    }

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        HDF_LOGE("bind socket failed, err = %{public}d", errno);
        close(sockfd);
        return HDF_FAILURE;
    }

    return sockfd;
}

static ssize_t DevMgrReadUeventMessage(int sockFd, char *buffer, size_t length)
{
    struct sockaddr_nl addr;
    (void)memset_s(&addr, sizeof(addr), 0, sizeof(addr));

    struct iovec iov;
    iov.iov_base = buffer;
    iov.iov_len = length;

    char credMsg[CMSG_SPACE(sizeof(struct ucred))] = {0};
    struct msghdr msghdr = {0};
    msghdr.msg_name = &addr;
    msghdr.msg_namelen = sizeof(addr);
    msghdr.msg_iov = &iov;
    msghdr.msg_iovlen = 1;
    msghdr.msg_control = credMsg;
    msghdr.msg_controllen = sizeof(credMsg);

    ssize_t n = recvmsg(sockFd, &msghdr, 0);
    if (n <= 0) {
        return HDF_FAILURE;
    }

    struct cmsghdr *hdr = CMSG_FIRSTHDR(&msghdr);
    if (hdr == NULL || hdr->cmsg_type != SCM_CREDENTIALS) {
        HDF_LOGI("Unexpected control message, ignored");
        // drop this message
        *buffer = '\0';
        return HDF_FAILURE;
    }

    return n;
}

static bool DevMgrUeventMsgCheckValid(const char *msg, ssize_t msgLen)
{
    (void)msgLen;
    if (strncmp(msg, "libudev", strlen("libudev")) == 0) {
        return false;
    }
    return true;
}

static bool DevMgrUeventMatchRule(struct DListHead *keyList, struct DListHead *ruleKeyList)
{
    struct DevMgrMatchKey *key = NULL;
    struct DevMgrMatchKey *keyMsg = NULL;

    DLIST_FOR_EACH_ENTRY(key, ruleKeyList, struct DevMgrMatchKey, entry) {
        bool match = false;
        DLIST_FOR_EACH_ENTRY(keyMsg, keyList, struct DevMgrMatchKey, entry) {
            if (strcmp(key->key, keyMsg->key) == 0) {
                if (strcmp(key->value, keyMsg->value) == 0) {
                    match = true;
                    break;
                } else {
                    return false;
                }
            }
        }
        if (!match) {
            return false;
        }
    }

    return true;
}

static void DevMgrUeventProcessPnPEvent(const struct DevMgrUeventRuleCfg *ruleCfg)
{
    struct IDevmgrService *instance = DevmgrServiceGetInstance();

    if (instance == NULL) {
        HDF_LOGE("Getting DevmgrService instance failed");
        return;
    }

    if (ruleCfg->action == DEVMGR_ACTION_LOAD) {
        instance->LoadDevice(instance, ruleCfg->serviceName);
    } else {
        instance->UnloadDevice(instance, ruleCfg->serviceName);
    }
}

static bool DevMgrUeventMatchRules(struct DListHead *keyList, struct DListHead *ruleList)
{
    struct DevMgrUeventRuleCfg *ruleCfg = NULL;

    DLIST_FOR_EACH_ENTRY(ruleCfg, ruleList, struct DevMgrUeventRuleCfg, entry) {
        if (DevMgrUeventMatchRule(keyList, &ruleCfg->matchKeyList)) {
            HDF_LOGI("%{public}s %{public}s %{public}d", __func__, ruleCfg->serviceName, ruleCfg->action);
            DevMgrUeventProcessPnPEvent(ruleCfg);
            return true;
        }
    }

    return false;
}

static int32_t DevMgrParseUevent(char *msg, ssize_t msgLen)
{
    if (!DevMgrUeventMsgCheckValid(msg, msgLen)) {
        return HDF_FAILURE;
    }

    struct DListHead keyList;
    DListHeadInit(&keyList);
    struct DevMgrMatchKey *key = NULL;

    for (char *ptr = msg; ptr < (msg + msgLen);) {
        if (*ptr == '0') {
            ptr++;
            continue;
        }

        if (key == NULL) {
            key = (struct DevMgrMatchKey *)OsalMemCalloc(sizeof(*key));
            if (key == NULL) {
                DevMgrUeventReleaseKeyList(&keyList);
                return HDF_FAILURE;
            }
            DListHeadInit(&key->entry);
        }
 
        uint32_t subStrLen = (uint32_t)strlen(ptr) + 1;
        HDF_LOGD("%{public}s %{public}d [%{public}s]", __func__, subStrLen, ptr);
        if (DevMgrUeventParseKeyValue(ptr, key, UEVENT_DELIMITER) == HDF_SUCCESS) {
            DListInsertHead(&key->entry, &keyList);
            key = NULL;
        }
        ptr += subStrLen;
    }

    HDF_LOGD("%{public}s {%{public}s} %{public}zd", __func__, msg, msgLen);
    DevMgrUeventMatchRules(&keyList, DevMgrUeventRuleCfgList());
    DevMgrUeventReleaseKeyList(&keyList);

    return HDF_SUCCESS;
}

#define DEVMGR_UEVENT_MSG_SIZE  2048
#define DEVMGR_UEVENT_WAIT_TIME 1000
static int32_t DevMgrUeventThread(void *arg)
{
    (void)arg;
    int32_t sfd = DevMgrUeventSocketInit();
    if (sfd < 0) {
        HDF_LOGE("DevMgrUeventSocketInit get sfd error");
        return HDF_FAILURE;
    }

    char msg[DEVMGR_UEVENT_MSG_SIZE + 1] = {0};
    ssize_t msgLen;
    struct pollfd fd;
    (void)memset_s(&fd, sizeof(fd), 0, sizeof(fd));
    fd.fd = sfd;
    fd.events = POLLIN | POLLERR;
    while (true) {
        if (poll(&fd, 1, -1) <= 0) {
            HDF_LOGE("%{public}s poll fail %{public}d", __func__, errno);
            OsalMSleep(DEVMGR_UEVENT_WAIT_TIME);
            continue;
        }
        if (((uint32_t)fd.revents & POLLIN) == POLLIN) {
            msgLen = DevMgrReadUeventMessage(sfd, msg, DEVMGR_UEVENT_MSG_SIZE);
            if (msgLen <= 0) {
                continue;
            }
            DevMgrParseUevent(msg, msgLen);
            (void)memset_s(&msg, sizeof(msg), 0, sizeof(msg));
        } else if (((uint32_t)fd.revents & POLLERR) == POLLERR) {
            HDF_LOGE("%{public}s poll error", __func__);
        }
    }

    DevMgrUeventReleaseRuleCfgList();
    close(sfd);
    return HDF_SUCCESS;
}

#define DEVMGR_UEVENT_STACK_SIZE 100000

int32_t DevMgrUeventReceiveStart(void)
{
    HDF_LOGI("DevMgrUeventReceiveStart");
    if (DevMgrUeventParseConfig() == HDF_FAILURE) {
        return HDF_FAILURE;
    }

    struct OsalThreadParam threadCfg;
    (void)memset_s(&threadCfg, sizeof(threadCfg), 0, sizeof(threadCfg));
    threadCfg.name = "DevMgrUevent";
    threadCfg.priority = OSAL_THREAD_PRI_HIGH;
    threadCfg.stackSize = DEVMGR_UEVENT_STACK_SIZE;

    OSAL_DECLARE_THREAD(thread);
    (void)OsalThreadCreate(&thread, (OsalThreadEntry)DevMgrUeventThread, NULL);
    (void)OsalThreadStart(&thread, &threadCfg);

    return HDF_SUCCESS;
}
