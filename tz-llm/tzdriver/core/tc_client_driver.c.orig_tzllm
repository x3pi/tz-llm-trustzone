/*
 * Copyright (C) 2022 Huawei Technologies Co., Ltd.
 * Decription: function for proc open,close session and invoke.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "tc_client_driver.h"
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_reserved_mem.h>
#include <linux/atomic.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/pid.h>
#include <linux/security.h>
#include <linux/cred.h>
#include <linux/namei.h>
#include <linux/thread_info.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/security.h>
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#endif
#include <linux/acpi.h>
#include <linux/completion.h>
#include <linux/bitmap.h>
#include <linux/cma.h>
#include <linux/atomic.h>
#include <securec.h>
#include "smc_smp.h"
#include "teek_client_constants.h"
#include "agent.h"
#include "mem.h"
#include "gp_ops.h"
#include "tc_ns_log.h"
#include "tc_ns_client.h"
#include "mailbox_mempool.h"
#include "shared_mem.h"
#include "tz_spi_notify.h"
#include "client_hash_auth.h"
#include "auth_base_impl.h"
#include "tlogger.h"
#include "tzdebug.h"
#include "session_manager.h"
#include "internal_functions.h"
#include "ko_adapt.h"
#include "tz_pm.h"
#include "tui.h"
#include "dynamic_ion_mem.h"
#include "static_ion_mem.h"
#include "reserved_mempool.h"
#ifdef CONFIG_CMS_SIGNATURE
#include "tz_update_crl.h"
#endif
#ifdef CONFIG_TEE_REBOOT
#include "reboot.h"
#endif

#ifdef CONFIG_FFA_SUPPORT
#include "ffa_abi.h"
#endif

#include "smc_abi.h"

#define CMD_QUEUE_SHM_SIZE (256 * 4096)
static unsigned long g_llm_shm;

#define MAX_ORDER (10)

#define TZASC_NR               (4)
#define TZASC_TOTAL_MEM_SIZE   (9UL * (1UL << 30))
#define TZASC_PER_CMA_MEM_SIZE (TZASC_TOTAL_MEM_SIZE / TZASC_NR)

struct tzasc_cma_entry {
	unsigned long paddr;
	unsigned long size;
	struct page *cma_pages;
};

struct tzasc_cma_meta {
	unsigned long base;
	unsigned long size;
	unsigned long count;
	struct tzasc_cma_entry entry[((PAGE_SIZE << MAX_ORDER) / TZASC_NR - sizeof(unsigned long) * 4) / sizeof(struct tzasc_cma_entry)];
};

extern struct cma *tzasc_cma[TZASC_NR];
struct tzasc_cma_meta *g_tzasc_cma_meta_arr;
static spinlock_t tzasc_cma_lock;

struct page *tzasc_cma_alloc_with_index(int cma_index, unsigned long size) {
	if (cma_index < 0 || cma_index >= TZASC_NR)
		return NULL;
	return cma_alloc(tzasc_cma[cma_index], size >> PAGE_SHIFT, 0, GFP_KERNEL);
}

int tzasc_cma_free_with_index(int cma_index, struct page *cma_pages, unsigned long size) {
	if (cma_index < 0 || cma_index >= TZASC_NR)
		return -EINVAL;
	bool ret = cma_release(tzasc_cma[cma_index], cma_pages, size >> PAGE_SHIFT);
	if (ret)
		return 0;
	else
		return -EINVAL;
}

int tzasc_cma_push_pages_with_index(unsigned long size, int cma_index) {
	int ret;
	unsigned long flags;
	struct tzasc_cma_meta *g_tzasc_cma_meta = g_tzasc_cma_meta_arr + cma_index;

	if (cma_index < 0 || cma_index >= TZASC_NR)
		return -EINVAL;

	size = (size + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
	struct tzasc_cma_entry *entry = &g_tzasc_cma_meta->entry[g_tzasc_cma_meta->count];
	entry->cma_pages = tzasc_cma_alloc_with_index(cma_index, size);
	if (entry->cma_pages == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	entry->paddr = page_to_phys(entry->cma_pages);
	entry->size = size;
	// pr_info("%s %d: size %lx count %d\n", __func__, __LINE__, size, g_tzasc_cma_meta->count);
	ret = g_tzasc_cma_meta->count++;

out:
	return ret;
}

int tzasc_cma_pop_pages_with_index(int cma_index) {
	int ret;
	unsigned long flags;
	struct tzasc_cma_meta *g_tzasc_cma_meta = g_tzasc_cma_meta_arr + cma_index;

	if (cma_index < 0 || cma_index >= TZASC_NR)
		return -EINVAL;

	if (g_tzasc_cma_meta->count == 0) {
		ret = -ENOMEM;
		goto out;
	}
	g_tzasc_cma_meta->count--;
	struct tzasc_cma_entry *entry = &g_tzasc_cma_meta->entry[g_tzasc_cma_meta->count];
	// pr_info("%s %d: size %lx count %d\n", __func__, __LINE__, g_tzasc_cma_meta->entry[g_tzasc_cma_meta->count].size, g_tzasc_cma_meta->count);
	ret = tzasc_cma_free_with_index(cma_index, entry->cma_pages, entry->size);

out:
	return ret;
}

void tzasc_cma_free_pages_with_index(int cma_index) {
	struct tzasc_cma_meta *g_tzasc_cma_meta = g_tzasc_cma_meta_arr + cma_index;

	if (cma_index < 0 || cma_index >= TZASC_NR)
		return;
	while (g_tzasc_cma_meta->count)
		tzasc_cma_pop_pages_with_index(cma_index);
}

static int cur_cma_index = 0;

int tzasc_cma_push_pages(unsigned long size) {
	int ret = -EINVAL;

	for (; cur_cma_index < TZASC_NR; cur_cma_index++) {
		ret = tzasc_cma_push_pages_with_index(size, cur_cma_index);
		if (ret != -ENOMEM)
			break;
	}

	return ret;
}

int tzasc_cma_pop_pages(void) {
	int ret = -EINVAL;

	for (; cur_cma_index >= 0; cur_cma_index--) {
		ret = tzasc_cma_pop_pages_with_index(cur_cma_index);
		if (ret != -ENOMEM)
			break;
	}

	return ret;
}

void tzasc_cma_free_pages(void) {
	int cma_index;
	for (cma_index = 0; cma_index < TZASC_NR; cma_index++) {
		tzasc_cma_free_pages_with_index(cma_index);
	}
	cur_cma_index = 0;
}

int tzasc_cma_init(void) {
	int cma_index;
	struct tzasc_cma_meta *g_tzasc_cma_meta;

	spin_lock_init(&tzasc_cma_lock);
	tlogd("%s: meta size %lu, mem size %lu\n", __func__, sizeof(struct tzasc_cma_meta) * TZASC_NR, PAGE_SIZE << MAX_ORDER);
	g_tzasc_cma_meta_arr = (struct tzasc_cma_meta *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, MAX_ORDER);
	if (g_tzasc_cma_meta_arr == NULL)
		return -ENOMEM;
	// g_tzasc_cma_meta->base = xxx_cma->base_pfn << PAGE_SHIFT;
	for (cma_index = 0; cma_index < TZASC_NR; cma_index++) {
		g_tzasc_cma_meta = g_tzasc_cma_meta_arr + cma_index;
		g_tzasc_cma_meta->base = *(unsigned long *)tzasc_cma[cma_index] << PAGE_SHIFT;
		g_tzasc_cma_meta->size = TZASC_PER_CMA_MEM_SIZE;
		g_tzasc_cma_meta->count = 0;
	}
	return 0;
}

struct s2_l0_meta_entry {
    unsigned long paddr;
    unsigned long size;
};

struct s2_l0_meta {
    unsigned long entry_nr;
    struct s2_l0_meta_entry entry[0];
};

struct s2_l1_meta_entry {
    unsigned long paddr;
    unsigned long size;
};

#define LLM_MODEL_SIZE (8ul * 1024 * 1024 * 1024)

static struct s2_l0_meta *g_s2_l0_meta;
static struct s2_l1_meta_entry **g_s2_l1_meta_arr_s;
static struct s2_l1_meta_entry **g_s2_l1_meta_arr_ns;
static spinlock_t s2_lock;

inline struct s2_l1_meta_entry *get_s2_l1_meta_entry(struct s2_l1_meta_entry **base, unsigned long index) {
	for (unsigned long i = 0; i < g_s2_l0_meta->entry_nr; i++) {
		if (index < (g_s2_l0_meta->entry[i].size / sizeof(struct s2_l1_meta_entry))) {
			return &base[i][index];
		}
		index -= (g_s2_l0_meta->entry[i].size / sizeof(struct s2_l1_meta_entry));
	}
	return NULL;
}

int s2_alloc_pages(unsigned long entry_begin, unsigned long entry_end) {
	int ret = 0;
	unsigned long pages;
	unsigned long order, page_nr;
	struct s2_l1_meta_entry *entry;
	unsigned long flags;

    spin_lock_irqsave(&s2_lock, flags);

	while (entry_begin < entry_end) {
		for (order = MAX_ORDER; order >= 0; order--) {
			page_nr = 1 << order;
			if (entry_begin + page_nr > entry_end)
				continue;
			pages = __get_free_pages(GFP_KERNEL, order);
			if (pages != 0)
				break;
		}
		if (pages == 0) {
			ret = -ENOMEM;
			tloge("%s %d: ENOMEM\n", __func__, __LINE__);
			goto out;
		}
		entry = get_s2_l1_meta_entry(g_s2_l1_meta_arr_s, entry_begin);
		entry->paddr = virt_to_phys((const volatile void *)pages);
		entry->size = page_nr << PAGE_SHIFT;

		entry = get_s2_l1_meta_entry(g_s2_l1_meta_arr_ns, entry_begin);
		entry->paddr = pages; // vaddr
		entry->size = order; // order

		entry_begin += page_nr;
	}

	if (entry_begin != entry_end) {
		tloge("%s %d: begin %lu end %lu\n", __func__, __LINE__, entry_begin, entry_end);
		ret = -EINVAL;
	}

    spin_unlock_irqrestore(&s2_lock, flags);

out:
	return ret;
}

int s2_free_pages(unsigned long entry_begin, unsigned long entry_end) {
	int ret = 0;
	struct s2_l1_meta_entry *entry;
	unsigned long pages, order;
	unsigned long flags;

    spin_lock_irqsave(&s2_lock, flags);

	while (entry_begin < entry_end) {
		entry = get_s2_l1_meta_entry(g_s2_l1_meta_arr_ns, entry_begin);
		pages = entry->paddr;
		order = entry->size;
		free_pages(pages, order);
		entry->paddr = 0;
		entry_begin += (1 << order);
	}

	if (entry_begin != entry_end) {
		tloge("%s %d: begin %lu end %lu\n", __func__, __LINE__, entry_begin, entry_end);
		ret = -EINVAL;
	}

    spin_unlock_irqrestore(&s2_lock, flags);

	return ret;
}

void s2_free_all_pages(void) {
	unsigned long flags;

    spin_lock_irqsave(&s2_lock, flags);

	unsigned long pages, order;
	for (unsigned long i = 0; i < g_s2_l0_meta->entry_nr; i++) {
		unsigned long entry_nr = (g_s2_l0_meta->entry[i].size / sizeof(struct s2_l1_meta_entry));
		for (unsigned long index = 0; index < entry_nr; index++) {
			struct s2_l1_meta_entry *entry = &g_s2_l1_meta_arr_ns[i][index];
			pages = entry->paddr;
			order = entry->size;
			if (pages)
				free_pages(pages, order);
			entry->paddr = 0;
		}
	}

    spin_unlock_irqrestore(&s2_lock, flags);
}

int s2_init(void) {
	unsigned long l0_entry_nr = LLM_MODEL_SIZE / PAGE_SIZE / ((PAGE_SIZE << MAX_ORDER) / (sizeof(struct s2_l1_meta_entry))) + 1;
	unsigned long i;

	spin_lock_init(&s2_lock);

	tlogd("%s %d: l0_entry_nr %lu\n", __func__, __LINE__, l0_entry_nr);

	g_s2_l0_meta = (struct s2_l0_meta *)__get_free_page(GFP_KERNEL);
	if (g_s2_l0_meta == NULL) {
		return -ENOMEM;
	}

	g_s2_l0_meta->entry_nr = l0_entry_nr;
	g_s2_l1_meta_arr_s = kmalloc(l0_entry_nr * sizeof(*g_s2_l1_meta_arr_s), GFP_KERNEL);
	if (g_s2_l1_meta_arr_s == NULL) {
		return -ENOMEM;
	}
	g_s2_l1_meta_arr_ns = vmalloc(l0_entry_nr * sizeof(*g_s2_l1_meta_arr_ns));
	if (g_s2_l1_meta_arr_ns == NULL) {
		return -ENOMEM;
	}

	for (i = 0; i < l0_entry_nr; i++) {
		g_s2_l1_meta_arr_s[i] = (struct s2_l1_meta_entry *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, MAX_ORDER);
		if (g_s2_l1_meta_arr_s[i] == NULL) {
			return -ENOMEM;
		}
		g_s2_l1_meta_arr_ns[i] = (struct s2_l1_meta_entry *)vzalloc(PAGE_SIZE << MAX_ORDER);
		if (g_s2_l1_meta_arr_ns[i] == NULL) {
			return -ENOMEM;
		}
		g_s2_l0_meta->entry[i].paddr = virt_to_phys((const volatile void *)g_s2_l1_meta_arr_s[i]);
		g_s2_l0_meta->entry[i].size = PAGE_SIZE << MAX_ORDER;
	}

	return 0;
}

static struct class *g_driver_class;
static struct device_node *g_dev_node;

struct dev_node g_tc_client;
struct dev_node g_tc_private;

#ifdef CONFIG_ACPI
static int g_acpi_irq;
#endif

static DEFINE_MUTEX(g_set_ca_hash_lock);

/* dev_file_id rangde in (0, 32767), 32768 use 4k bitmap */
#define DEV_FILE_ID_MAX 32768u

unsigned long *g_dev_bit_map = NULL;
static DEFINE_MUTEX(g_dev_bit_map_lock);

static int alloc_dev_bitmap(void)
{
	mutex_lock(&g_dev_bit_map_lock);
	if (g_dev_bit_map == NULL) {
		g_dev_bit_map = bitmap_alloc(DEV_FILE_ID_MAX, GFP_KERNEL | __GFP_ZERO);
		if (g_dev_bit_map == NULL) {
			tloge("alloc bit map failed\n");
			mutex_unlock(&g_dev_bit_map_lock);
			return -1;
		}
	}

	mutex_unlock(&g_dev_bit_map_lock);
	return 0;
}

static void free_dev_bitmap(void)
{
	mutex_lock(&g_dev_bit_map_lock);
	if (g_dev_bit_map != NULL) {
		bitmap_free(g_dev_bit_map);
		g_dev_bit_map = NULL;
	}
	mutex_unlock(&g_dev_bit_map_lock);
}

static bool alloc_dev_file_id(unsigned int *dev_file_id)
{
	int pos;

	mutex_lock(&g_dev_bit_map_lock);
	if (dev_file_id == NULL || g_dev_bit_map == NULL) {
		tloge("invalid param\n");
		mutex_unlock(&g_dev_bit_map_lock);
		return false;
	}

	pos = bitmap_find_free_region(g_dev_bit_map, DEV_FILE_ID_MAX, 0);
	if (pos < 0) {
		tloge("dev file fd full, alloc failed, error = %d\n", pos);
		mutex_unlock(&g_dev_bit_map_lock);
		return false;
	}

	*dev_file_id = (unsigned int)pos;
	tlogd("alloc dev file id = %u", *dev_file_id);
	mutex_unlock(&g_dev_bit_map_lock);
	return true;
}

static void free_dev_file_id(unsigned int dev_file_id)
{
	mutex_lock(&g_dev_bit_map_lock);
	if (g_dev_bit_map == NULL) {
		tloge("dev file fd bitmap is null\n");
		mutex_unlock(&g_dev_bit_map_lock);
		return;
	}

	if (dev_file_id >= DEV_FILE_ID_MAX) {
		tloge("dev file fd invalid\n");
		mutex_unlock(&g_dev_bit_map_lock);
		return;
	}

	/* clear dev_file_id bit for reuse */
	bitmap_release_region(g_dev_bit_map, dev_file_id, 0);
	tlogd("clear dev file id %u\n", dev_file_id);
	mutex_unlock(&g_dev_bit_map_lock);
}

/* dev node list and itself has mutex to avoid race */
struct tc_ns_dev_list g_tc_ns_dev_list;

static bool g_init_succ = false;

static void set_tz_init_flag(void)
{
	g_init_succ = true;
}

static void clear_tz_init_flag(void)
{
	g_init_succ = false;
}

bool get_tz_init_flag(void)
{
	return g_init_succ;
}

struct tc_ns_dev_list *get_dev_list(void)
{
	return &g_tc_ns_dev_list;
}

static int tc_ns_get_tee_version(const struct tc_ns_dev_file *dev_file,
	void __user *argp)
{
	unsigned int version;
	struct tc_ns_smc_cmd smc_cmd = { {0}, 0 };
	int ret = 0;
	struct mb_cmd_pack *mb_pack = NULL;

	if (!argp) {
		tloge("error input parameter\n");
		return -EINVAL;
	}

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc mb pack failed\n");
		return -ENOMEM;
	}

	mb_pack->operation.paramtypes = TEEC_VALUE_OUTPUT;
	smc_cmd.cmd_type = CMD_TYPE_GLOBAL;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_GET_TEE_VERSION;
	smc_cmd.dev_file_id = dev_file->dev_file_id;
	smc_cmd.operation_phys = mailbox_virt_to_phys((uintptr_t)&mb_pack->operation);
	smc_cmd.operation_h_phys =
		(uint64_t)mailbox_virt_to_phys((uintptr_t)&mb_pack->operation) >> ADDR_TRANS_NUM;

	if (tc_ns_smc(&smc_cmd) != 0) {
		ret = -EPERM;
		tloge("smc call returns error ret 0x%x\n", smc_cmd.ret_val);
	}

	version = mb_pack->operation.params[0].value.a;
	if (copy_to_user(argp, &version, sizeof(unsigned int)) != 0)
		ret = -EFAULT;
	mailbox_free(mb_pack);

	return ret;
}

/*
 * This is the login information
 * and is set teecd when client opens a new session
 */
#define MAX_BUF_LEN 4096

static int get_pack_name_len(struct tc_ns_dev_file *dev_file,
	const uint8_t *cert_buffer)
{
	uint32_t tmp_len = 0;

	dev_file->pkg_name_len = 0;
	if (memcpy_s(&tmp_len, sizeof(tmp_len), cert_buffer, sizeof(tmp_len)) != 0)
		return -EFAULT;

	if (tmp_len == 0 || tmp_len >= MAX_PACKAGE_NAME_LEN) {
		tloge("invalid pack name len: %u\n", tmp_len);
		return -EINVAL;
	}
	dev_file->pkg_name_len = tmp_len;
	tlogd("package name len is %u\n", dev_file->pkg_name_len);

	return 0;
}

static int get_public_key_len(struct tc_ns_dev_file *dev_file,
	const uint8_t *cert_buffer)
{
	uint32_t tmp_len = 0;

	dev_file->pub_key_len = 0;
	if (memcpy_s(&tmp_len, sizeof(tmp_len), cert_buffer, sizeof(tmp_len)) != 0)
		return -EFAULT;

	if (tmp_len > MAX_PUBKEY_LEN) {
		tloge("invalid public key len: %u\n", tmp_len);
		return -EINVAL;
	}
	dev_file->pub_key_len = tmp_len;
	tlogd("publick key len is %u\n", dev_file->pub_key_len);

	return 0;
}

static int get_public_key(struct tc_ns_dev_file *dev_file,
	const uint8_t *cert_buffer)
{
	/* get public key */
	if (dev_file->pub_key_len == 0)
		return 0;

	if (memcpy_s(dev_file->pub_key, MAX_PUBKEY_LEN, cert_buffer,
		dev_file->pub_key_len) != 0) {
		tloge("failed to copy pub key len\n");
		return -EINVAL;
	}

	return 0;
}

static bool is_cert_buffer_size_valid(int cert_buffer_size)
{
	/*
	 * GET PACKAGE NAME AND APP CERTIFICATE:
	 * The proc_info format is as follows:
	 * package_name_len(4 bytes) || package_name ||
	 * apk_cert_len(4 bytes) || apk_cert.
	 * or package_name_len(4 bytes) || package_name
	 * || exe_uid_len(4 bytes) || exe_uid.
	 * The apk certificate format is as follows:
	 * modulus_size(4bytes) ||modulus buffer
	 * || exponent size || exponent buffer
	 */
	if (cert_buffer_size > MAX_BUF_LEN || cert_buffer_size == 0) {
		tloge("cert buffer size is invalid!\n");
		return false;
	}

	return true;
}

static int alloc_login_buf(struct tc_ns_dev_file *dev_file,
	uint8_t **cert_buffer, unsigned int *cert_buffer_size)
{
	*cert_buffer_size = (unsigned int)(MAX_PACKAGE_NAME_LEN +
		MAX_PUBKEY_LEN + sizeof(dev_file->pkg_name_len) +
		sizeof(dev_file->pub_key_len));

	*cert_buffer = kmalloc(*cert_buffer_size, GFP_KERNEL);
	if (ZERO_OR_NULL_PTR((unsigned long)(uintptr_t)(*cert_buffer))) {
		tloge("failed to allocate login buffer!");
		return -ENOMEM;
	}

	return 0;
}

static int client_login_prepare(uint8_t *cert_buffer,
	const void __user *buffer, unsigned int cert_buffer_size)
{
	if (!is_cert_buffer_size_valid(cert_buffer_size))
		return -EINVAL;

	if (copy_from_user(cert_buffer, buffer, cert_buffer_size) != 0) {
		tloge("Failed to get user login info!\n");
		return -EINVAL;
	}

	return 0;
}

static int tc_login_check(const struct tc_ns_dev_file *dev_file)
{
	int ret = check_teecd_auth();
#ifdef CONFIG_CADAEMON_AUTH
	if (ret != 0)
		ret = check_cadaemon_auth();
#endif
	if (ret != 0) {
		tloge("teec auth failed, ret %d\n", ret);
		return -EACCES;
	}

	if (!dev_file)
		return -EINVAL;

	return 0;
}

static int tc_ns_client_login_func(struct tc_ns_dev_file *dev_file,
	const void __user *buffer)
{
	int ret;
	uint8_t *cert_buffer = NULL;
	uint8_t *temp_cert_buffer = NULL;
	unsigned int cert_buffer_size = 0;

	if (tc_login_check(dev_file) != 0)
		return -EFAULT;

	if (!buffer) {
		/*
		 * We accept no debug information
		 * because the daemon might  have failed
		 */
		dev_file->pkg_name_len = 0;
		dev_file->pub_key_len = 0;
		return 0;
	}

	mutex_lock(&dev_file->login_setup_lock);
	if (dev_file->login_setup) {
		tloge("login information cannot be set twice!\n");
		mutex_unlock(&dev_file->login_setup_lock);
		return -EINVAL;
	}

	ret = alloc_login_buf(dev_file, &cert_buffer, &cert_buffer_size);
	if (ret != 0) {
		mutex_unlock(&dev_file->login_setup_lock);
		return ret;
	}

	temp_cert_buffer = cert_buffer;
	if (client_login_prepare(cert_buffer, buffer, cert_buffer_size) != 0) {
		ret = -EINVAL;
		goto error;
	}

	ret = get_pack_name_len(dev_file, cert_buffer);
	if (ret != 0)
		goto error;
	cert_buffer += sizeof(dev_file->pkg_name_len);

	if (strncpy_s(dev_file->pkg_name, MAX_PACKAGE_NAME_LEN, cert_buffer,
		dev_file->pkg_name_len) != 0) {
		ret = -ENOMEM;
		goto error;
	}
	cert_buffer += dev_file->pkg_name_len;

	ret = get_public_key_len(dev_file, cert_buffer);
	if (ret != 0)
		goto error;
	cert_buffer += sizeof(dev_file->pub_key_len);

	ret = get_public_key(dev_file, cert_buffer);
	dev_file->login_setup = true;

error:
	kfree(temp_cert_buffer);
	mutex_unlock(&dev_file->login_setup_lock);
	return ret;
}

int tc_ns_client_open(struct tc_ns_dev_file **dev_file, uint8_t kernel_api)
{
	struct tc_ns_dev_file *dev = NULL;

	tlogd("tc_client_open\n");
	if (!dev_file) {
		tloge("dev_file is NULL\n");
		return -EINVAL;
	}

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (ZERO_OR_NULL_PTR((unsigned long)(uintptr_t)dev)) {
		tloge("dev malloc failed\n");
		return -ENOMEM;
	}

	if (!alloc_dev_file_id(&(dev->dev_file_id))) {
		kfree(dev);
		return -ENOMEM;
	}

	mutex_lock(&g_tc_ns_dev_list.dev_lock);
	list_add_tail(&dev->head, &g_tc_ns_dev_list.dev_file_list);
	mutex_unlock(&g_tc_ns_dev_list.dev_lock);
	INIT_LIST_HEAD(&dev->shared_mem_list);
	dev->login_setup = 0;
#ifdef CONFIG_AUTH_HASH
	dev->cainfo_hash_setup = 0;
#endif
	dev->kernel_api = kernel_api;
	dev->load_app_flag = 0;
	mutex_init(&dev->service_lock);
	mutex_init(&dev->shared_mem_lock);
	mutex_init(&dev->login_setup_lock);
#ifdef CONFIG_AUTH_HASH
	mutex_init(&dev->cainfo_hash_setup_lock);
#endif
	init_completion(&dev->close_comp);
	*dev_file = dev;

	return 0;
}

static void del_dev_node(struct tc_ns_dev_file *dev)
{
	if (!dev)
		return;

	mutex_lock(&g_tc_ns_dev_list.dev_lock);
	list_del(&dev->head);
	mutex_unlock(&g_tc_ns_dev_list.dev_lock);
}

void free_dev(struct tc_ns_dev_file *dev)
{
	del_dev_node(dev);
	tee_agent_clear_dev_owner(dev);
	free_dev_file_id(dev->dev_file_id);
	if (memset_s(dev, sizeof(*dev), 0, sizeof(*dev)) != 0)
		tloge("Caution, memset dev fail!\n");
	kfree(dev);
}

int tc_ns_client_close(struct tc_ns_dev_file *dev)
{
	if (!dev) {
		tloge("invalid dev(null)\n");
		return -EINVAL;
	}

	close_unclosed_session_in_kthread(dev);

	if (dev->dev_file_id == tui_attach_device())
		free_tui_caller_info();

	kill_ion_by_cafd(dev->dev_file_id);
	/* for thirdparty agent, code runs here only when agent crashed */
	send_crashed_event_response_all(dev);
	free_dev(dev);

	return 0;
}

void shared_vma_open(struct vm_area_struct *vma)
{
	(void)vma;
}

void shared_vma_close(struct vm_area_struct *vma)
{
	struct tc_ns_shared_mem *shared_mem = NULL;
	struct tc_ns_shared_mem *shared_mem_temp = NULL;
	bool find = false;
	struct tc_ns_dev_file *dev_file = NULL;
	if (!vma) {
		tloge("vma is null\n");
		return;
	}
	dev_file = vma->vm_private_data;
	if (!dev_file) {
		tloge("vm private data is null\n");
		return;
	}

	mutex_lock(&dev_file->shared_mem_lock);
	list_for_each_entry_safe(shared_mem, shared_mem_temp,
			&dev_file->shared_mem_list, head) {
		if (shared_mem) {
			if (shared_mem->user_addr ==
				(void *)(uintptr_t)vma->vm_start) {
				shared_mem->user_addr = INVALID_MAP_ADDR;
				find = true;
			} else if (shared_mem->user_addr_ca ==
				(void *)(uintptr_t)vma->vm_start) {
				shared_mem->user_addr_ca = INVALID_MAP_ADDR;
				find = true;
			}

			if ((shared_mem->user_addr == INVALID_MAP_ADDR) &&
				(shared_mem->user_addr_ca == INVALID_MAP_ADDR))
				list_del(&shared_mem->head);

			/* pair with tc client mmap */
			if (find) {
				put_sharemem_struct(shared_mem);
				break;
			}
		}
	}
	mutex_unlock(&dev_file->shared_mem_lock);
}

static struct vm_operations_struct g_shared_remap_vm_ops = {
	.open = shared_vma_open,
	.close = shared_vma_close,
};

static struct tc_ns_shared_mem *find_sharedmem(
	const struct vm_area_struct *vma,
	const struct tc_ns_dev_file *dev_file, bool *only_remap)
{
	struct tc_ns_shared_mem *shm_tmp = NULL;
	unsigned long len = vma->vm_end - vma->vm_start;

	/*
	 * using vma->vm_pgoff as share_mem index
	 * check if aready allocated
	 */
	list_for_each_entry(shm_tmp, &dev_file->shared_mem_list, head) {
		if ((unsigned long)atomic_read(&shm_tmp->offset) == vma->vm_pgoff) {
			tlogd("sharemem already alloc, shm tmp->offset=%d\n",
				atomic_read(&shm_tmp->offset));
			/*
			 * args check:
			 * 1. this shared mem is already mapped
			 * 2. remap a different size shared_mem
			 */
			if ((shm_tmp->user_addr_ca != INVALID_MAP_ADDR) ||
				(vma->vm_end - vma->vm_start != shm_tmp->len)) {
				tloge("already remap once!\n");
				return NULL;
			}
			/* return the same sharedmem specified by vm_pgoff */
			*only_remap = true;
			get_sharemem_struct(shm_tmp);
			return shm_tmp;
		}
	}

	/* if not find, alloc a new sharemem */
	return tc_mem_allocate(len);
}

static int remap_shared_mem(struct vm_area_struct *vma,
	const struct tc_ns_shared_mem *shared_mem)
{
	int ret;
#ifdef CONFIG_LIBLINUX
	unsigned long pa = virt_to_phys(shared_mem->kernel_addr);
	unsigned long va = vma->vm_start;
	unsigned long size = vma->vm_end - vma->vm_start;

	pgprot_t pro;
	pro.pgprot = VM_READ | VM_WRITE;
	ret = remap_pfn_range(NULL, va, pa >> PAGE_SHIFT, size, pro);
#else
	if (shared_mem->mem_type == RESERVED_TYPE) {
		unsigned long pfn = res_mem_virt_to_phys((uintptr_t)(shared_mem->kernel_addr)) >> PAGE_SHIFT;
		unsigned long size = vma->vm_end - vma->vm_start;
		ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot); // PAGE_SHARED
		if (ret != 0)
			tloge("remap pfn for user failed, ret %d", ret);
		return ret;
	}
#if (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
	vma->__vm_flags |= VM_USERMAP;
#else
	vma->vm_flags |= VM_USERMAP;
#endif
	ret = remap_vmalloc_range(vma, shared_mem->kernel_addr, 0);
#endif
	if (ret != 0)
		tloge("can't remap to user, ret = %d\n", ret);

	return ret;
}

/*
 * in this func, we need to deal with follow cases:
 * vendor CA alloc sharedmem (alloc and remap);
 * HIDL alloc sharedmem (alloc and remap);
 * system CA alloc sharedmem (only just remap);
 */
static int tc_client_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret;
	struct tc_ns_dev_file *dev_file = NULL;
	struct tc_ns_shared_mem *shared_mem = NULL;
	bool only_remap = false;

	if (!filp || !vma || !filp->private_data) {
		tloge("invalid args for tc mmap\n");
		return -EINVAL;
	}
	dev_file = filp->private_data;

	mutex_lock(&dev_file->shared_mem_lock);
	shared_mem = find_sharedmem(vma, dev_file, &only_remap);
	if (IS_ERR_OR_NULL(shared_mem)) {
		tloge("alloc shared mem failed\n");
		mutex_unlock(&dev_file->shared_mem_lock);
		return -ENOMEM;
	}

	ret = remap_shared_mem(vma, shared_mem);
	if (ret != 0) {
		if (only_remap)
			put_sharemem_struct(shared_mem);
		else
			tc_mem_free(shared_mem);
		mutex_unlock(&dev_file->shared_mem_lock);
		return ret;
	}
#if (KERNEL_VERSION(6, 4, 0) <= LINUX_VERSION_CODE)
	vma->__vm_flags |= VM_DONTCOPY;
#else
	vma->vm_flags |= VM_DONTCOPY;
#endif
	vma->vm_ops = &g_shared_remap_vm_ops;
	shared_vma_open(vma);
	vma->vm_private_data = (void *)dev_file;

	if (only_remap) {
		shared_mem->user_addr_ca = (void *)(uintptr_t)vma->vm_start;
		mutex_unlock(&dev_file->shared_mem_lock);
		return ret;
	}
	shared_mem->user_addr = (void *)(uintptr_t)vma->vm_start;
	atomic_set(&shared_mem->offset, vma->vm_pgoff);
	get_sharemem_struct(shared_mem);
	list_add_tail(&shared_mem->head, &dev_file->shared_mem_list);
	mutex_unlock(&dev_file->shared_mem_lock);

	return ret;
}

static int ioctl_register_agent(struct tc_ns_dev_file *dev_file, unsigned long arg)
{
	int ret;
	struct agent_ioctl_args args;

	if (arg == 0) {
		tloge("arg is NULL\n");
		return -EFAULT;
	}

	if (copy_from_user(&args, (void *)(uintptr_t)arg, sizeof(args)) != 0) {
		tloge("copy agent args failed\n");
		return -EFAULT;
	}

	ret = tc_ns_register_agent(dev_file, args.id, args.buffer_size,
		&args.buffer, true);
	if (ret == 0) {
		if (copy_to_user((void *)(uintptr_t)arg, &args, sizeof(args)) != 0)
			tloge("copy agent user addr failed\n");
	}

	return ret;
}

static int ioctl_check_agent_owner(const struct tc_ns_dev_file *dev_file,
	unsigned int agent_id)
{
	struct smc_event_data *event_data = NULL;

	event_data = find_event_control(agent_id);
	if (event_data == NULL) {
		tloge("invalid agent id\n");
		return -EINVAL;
	}

	if (event_data->owner != dev_file) {
		tloge("invalid request, access denied!\n");
		put_agent_event(event_data);
		return -EPERM;
	}

	put_agent_event(event_data);
	return 0;
}

/* ioctls for the secure storage daemon */
static int public_ioctl(const struct file *file, unsigned int cmd, unsigned long arg, bool is_from_client_node)
{
	int ret = -EINVAL;
	struct tc_ns_dev_file *dev_file = file->private_data;
	void *argp = (void __user *)(uintptr_t)arg;
	if (!dev_file) {
		tloge("invalid params\n");
		return -EINVAL;
	}

	switch (cmd) {
	case TC_NS_CLIENT_IOCTL_WAIT_EVENT:
		if (ioctl_check_agent_owner(dev_file, (unsigned int)arg) != 0)
			return -EINVAL;
		ret = tc_ns_wait_event((unsigned int)arg);
		break;
	case TC_NS_CLIENT_IOCTL_SEND_EVENT_RESPONSE:
		if (ioctl_check_agent_owner(dev_file, (unsigned int)arg) != 0)
			return -EINVAL;
		ret = tc_ns_send_event_response((unsigned int)arg);
		break;
	case TC_NS_CLIENT_IOCTL_REGISTER_AGENT:
		ret = ioctl_register_agent(dev_file, arg);
		break;
	case TC_NS_CLIENT_IOCTL_UNREGISTER_AGENT:
		if (ioctl_check_agent_owner(dev_file, (unsigned int)arg) != 0)
			return -EINVAL;
		ret = tc_ns_unregister_agent((unsigned int)arg);
		break;
	case TC_NS_CLIENT_IOCTL_LOAD_APP_REQ:
		ret = tc_ns_load_secfile(file->private_data, argp, is_from_client_node);
		break;
	default:
		tloge("invalid cmd!");
		return ret;
	}
	tlogd("client ioctl ret = 0x%x\n", ret);
	return ret;
}

static int tc_ns_send_cancel_cmd(struct tc_ns_dev_file *dev_file, void *argp)
{
	struct tc_ns_client_context client_context = {{0}};
	(void)dev_file;

	if (!argp) {
		tloge("argp is NULL input buffer\n");
		return -EINVAL;
	}
	if (copy_from_user(&client_context, argp, sizeof(client_context)) != 0) {
		tloge("copy from user failed\n");
		return -ENOMEM;
	}

	client_context.returns.code = TEEC_ERROR_GENERIC;
	client_context.returns.origin = TEEC_ORIGIN_COMMS;
	tloge("not support send cancel cmd now\n");
	if (copy_to_user(argp, &client_context, sizeof(client_context)) != 0)
		return -EFAULT;

	return 0;
}

static int get_agent_id(unsigned long arg, unsigned int cmd, uint32_t *agent_id)
{
	struct agent_ioctl_args args;
	switch (cmd) {
	case TC_NS_CLIENT_IOCTL_WAIT_EVENT:
	case TC_NS_CLIENT_IOCTL_SEND_EVENT_RESPONSE:
	case TC_NS_CLIENT_IOCTL_UNREGISTER_AGENT:
		*agent_id = (unsigned int)arg;
		break;
	case TC_NS_CLIENT_IOCTL_REGISTER_AGENT:
		if (copy_from_user(&args, (void *)(uintptr_t)arg, sizeof(args))) {
			tloge("copy agent args failed\n");
			return -EFAULT;
		}
		*agent_id = args.id;
		break;
	default:
		return -EFAULT;
	}
	return 0;
}

static int tc_client_agent_ioctl(const struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = -EFAULT;
	uint32_t agent_id;

	switch (cmd) {
	case TC_NS_CLIENT_IOCTL_SEND_EVENT_RESPONSE:
	case TC_NS_CLIENT_IOCTL_WAIT_EVENT:
	case TC_NS_CLIENT_IOCTL_REGISTER_AGENT:
	case TC_NS_CLIENT_IOCTL_UNREGISTER_AGENT:
		if (get_agent_id(arg, cmd, &agent_id) != 0)
			return ret;
		if (check_ext_agent_access(agent_id) != 0) {
			tloge("the agent is not access\n");
			return -EPERM;
		}
		ret = public_ioctl(file, cmd, arg, true);
		break;
	default:
		tloge("invalid cmd 0x%x!", cmd);
		break;
	}

	return ret;
}

static void handle_cmd_prepare(unsigned int cmd)
{
	if (cmd != TC_NS_CLIENT_IOCTL_WAIT_EVENT &&
		cmd != TC_NS_CLIENT_IOCTL_SEND_EVENT_RESPONSE)
		livepatch_down_read_sem();
}

static void handle_cmd_finish(unsigned int cmd)
{
	if (cmd != TC_NS_CLIENT_IOCTL_WAIT_EVENT &&
		cmd != TC_NS_CLIENT_IOCTL_SEND_EVENT_RESPONSE)
		livepatch_up_read_sem();
}

static long tc_private_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = -EFAULT;
	void *argp = (void __user *)(uintptr_t)arg;
	handle_cmd_prepare(cmd);
	switch (cmd) {
	case TC_NS_CLIENT_IOCTL_GET_TEE_VERSION:
		ret = tc_ns_get_tee_version(file->private_data, argp);
		break;
	case TC_NS_CLIENT_IOCTL_SET_NATIVECA_IDENTITY:
		mutex_lock(&g_set_ca_hash_lock);
		ret = tc_ns_set_native_hash((unsigned long)(uintptr_t)argp, GLOBAL_CMD_ID_SET_CA_HASH);
		mutex_unlock(&g_set_ca_hash_lock);
		break;
	case TC_NS_CLIENT_IOCTL_LATEINIT:
		ret = tc_ns_late_init(arg);
		break;
	case TC_NS_CLIENT_IOCTL_SYC_SYS_TIME:
		ret = sync_system_time_from_user(
			(struct tc_ns_client_time *)(uintptr_t)arg);
		break;
	default:
		ret = public_ioctl(file, cmd, arg, false);
		break;
	}

	handle_cmd_finish(cmd);

	return ret;
}

static long tc_client_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = -EFAULT;
	void *argp = (void __user *)(uintptr_t)arg;

	handle_cmd_prepare(cmd);
	switch (cmd) {
	case TC_NS_CLIENT_IOCTL_SES_OPEN_REQ:
	case TC_NS_CLIENT_IOCTL_SES_CLOSE_REQ:
	case TC_NS_CLIENT_IOCTL_SEND_CMD_REQ:
		ret = tc_client_session_ioctl(file, cmd, arg);
		break;
	case TC_NS_CLIENT_IOCTL_CANCEL_CMD_REQ:
		ret = tc_ns_send_cancel_cmd(file->private_data, argp);
		break;
	case TC_NS_CLIENT_IOCTL_LOGIN:
		ret = tc_ns_client_login_func(file->private_data, argp);
		break;
	case TC_NS_CLIENT_IOCTL_LOAD_APP_REQ:
		ret = public_ioctl(file, cmd, arg, true);
		break;
#ifdef CONFIG_TEE_TUI
	case TC_NS_CLIENT_IOCTL_TUI_EVENT:
		ret = tc_ns_tui_event(file->private_data, argp);
		break;
#endif
#ifdef CONFIG_CMS_SIGNATURE
	case TC_NS_CLIENT_IOCTL_UPDATE_TA_CRL:
		ret = tc_ns_update_ta_crl(file->private_data, argp);
		break;
#endif
	default: {
		if (check_teecd_auth() == 0)
			ret = tc_private_ioctl(file, cmd, arg);
		else
			ret = tc_client_agent_ioctl(file, cmd, arg);
		break;
	}
	}

	handle_cmd_finish(cmd);

	tlogd("tc client ioctl ret = 0x%x\n", ret);
	return (long)ret;
}

static int tc_client_open(struct inode *inode, struct file *file)
{
	int ret;
	struct tc_ns_dev_file *dev = NULL;
	(void)inode;

	ret = check_teecd_auth();
#ifdef CONFIG_CADAEMON_AUTH
	if (ret != 0)
		ret = check_cadaemon_auth();
#endif
	if (ret != 0) {
		tloge("teec auth failed, ret %d\n", ret);
		return -EACCES;
	}

	file->private_data = NULL;
	ret = tc_ns_client_open(&dev, TEE_REQ_FROM_USER_MODE);
	if (ret == 0)
		file->private_data = dev;
#ifdef CONFIG_TEE_REBOOT
	get_teecd_pid();
#endif
	return ret;
}

static int tc_client_close(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct tc_ns_dev_file *dev = file->private_data;
#ifdef CONFIG_TEE_TUI
	/* release tui resource */
	struct teec_tui_parameter tui_param = {0};
#endif
	(void)inode;

#ifdef CONFIG_TEE_TUI
	if (dev->dev_file_id == tui_attach_device()) {
		ret = tui_send_event(TUI_POLL_CANCEL, &tui_param);
		/* tee tui service is dead, but we need release the buffer in ree */
		if (ret == TEEC_ERROR_TUI_NOT_AVAILABLE)
			do_ns_tui_release();
	}
#endif

	livepatch_down_read_sem();
	ret = tc_ns_client_close(dev);
	livepatch_up_read_sem();
	file->private_data = NULL;

	return ret;
}

static int tc_private_close(struct inode *inode, struct file *file)
{
	struct tc_ns_dev_file *dev = file->private_data;
	(void)inode;

	/* for teecd close fd */
	if (is_system_agent(dev)) {
		/* for teecd agent close fd */
		send_event_response_single(dev);
		free_dev(dev);
	} else {
		/* for ca damon close fd */
		free_dev(dev);
	}
	file->private_data = NULL;

	return 0;
}

struct tc_ns_dev_file *tc_find_dev_file(unsigned int dev_file_id)
{
	struct tc_ns_dev_file *dev_file = NULL;

	mutex_lock(&g_tc_ns_dev_list.dev_lock);
	list_for_each_entry(dev_file, &g_tc_ns_dev_list.dev_file_list, head) {
		if (dev_file->dev_file_id == dev_file_id) {
			mutex_unlock(&g_tc_ns_dev_list.dev_lock);
			return dev_file;
		}
	}
	mutex_unlock(&g_tc_ns_dev_list.dev_lock);
	return NULL;
}

#ifdef CONFIG_COMPAT
long tc_compat_client_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	long ret;

	if (!file)
		return -EINVAL;

	arg = (unsigned long)(uintptr_t)compat_ptr(arg);
	ret = tc_client_ioctl(file, cmd, arg);
	return ret;
}

long tc_compat_private_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	long ret;

	if (!file)
		return -EINVAL;

	arg = (unsigned long)(uintptr_t)compat_ptr(arg);
	ret = tc_private_ioctl(file, cmd, arg);
	return ret;
}
#endif

#define NPU_CORE_NUM (3)

static unsigned long waiting_npu_thread[2 << NPU_CORE_NUM];
static void *rknpu_dev;

void tzdriver_register_rknpu_dev(void *dev) {
	tlogd("%s %d %p\n", __func__, __LINE__, dev);
	rknpu_dev = dev;
}
EXPORT_SYMBOL(tzdriver_register_rknpu_dev);

enum smc_loop_exit {
	SMC_LOOP_EXIT_FINISH = 1,
	SMC_LOOP_EXIT_NPU_SUBMIT,
	SMC_LOOP_EXIT_NPU_DONE,
	SMC_LOOP_EXIT_IO_STEP,
};

struct out_result {
	enum smc_loop_exit exit;
	union {
		struct {

		} finish;
		struct {
			int npu_mask;
		} npu_submit;
		struct {
			void *job;
		} npu_done;
	};
};

extern void rknpu_job_done_s(struct rknpu_job *job, int ret);
extern int rknpu_submit_s(struct rknpu_device *rknpu_dev, u32 core_mask);

unsigned long on_fly_io_thread;

unsigned long smc_call_cpu_resume(struct out_result *result) {
	int ret;
	int ret_tee = 0;
	unsigned long req_thread = on_fly_io_thread;
	on_fly_io_thread = 0;
	while (true) {
		struct smc_in_params in = { TSP_REQUEST, ret_tee, req_thread };
		struct smc_out_params out;
		// tlogd("%s %d cpu %d\n", __func__, __LINE__, raw_smp_processor_id());
		do_smc_transport(&in, &out, 0);

		// tlogd("%s %d ret %#lx\n", __func__, __LINE__, out.ret);
		if (out.ret == SMC_EXIT_PREEMPTED) {
			// tlogd("%s %d SMC_PREEMPT\n", __func__, __LINE__);
			req_thread = 0;
			ret_tee = 0;
			cond_resched();
		} else if (out.ret == SMC_EXIT_SHADOW) {
			// tlogd("%s %d exit_reason %#lx\n", __func__, __LINE__, out.exit_reason);
			// tlogd("%s %d target %#lx\n", __func__, __LINE__, out.target);
			req_thread = out.target;
			ret = 0;
			switch (out.exit_reason) {
			case 0xdeadbeef:
				tzasc_cma_free_pages();
				ret_tee = 0;
				break;
			case 4:
				ret = SMC_LOOP_EXIT_IO_STEP;
				on_fly_io_thread = req_thread;
				break;
			case 3:
				result->npu_done.job = (void *)out.ta;
				// tlogd("%s %d job %lx\n", __func__, __LINE__, out.ta);
				rknpu_job_done_s(result->npu_done.job, 0);
				ret_tee = 0;
				break;
			case 2:
				result->npu_submit.npu_mask = out.ta;
				waiting_npu_thread[result->npu_submit.npu_mask] = req_thread;
				ret_tee = rknpu_submit_s(rknpu_dev, result->npu_submit.npu_mask);
				req_thread = 0;
				// return SMC_LOOP_EXIT_NPU_SUBMIT;
				break;
			case 1:
				// tlogd("%s %d ta %#lx\n", __func__, __LINE__, out.ta);
				ret_tee = tzasc_cma_push_pages_with_index(out.ta & ~(PAGE_SIZE-1), out.ta & (PAGE_SIZE-1));
				break;
			case 0:
				ret_tee = tzasc_cma_pop_pages_with_index(out.ta & (PAGE_SIZE-1));
				break;
			default:
				tloge("%s %d invalid exit_reason %ld ret %d\n", __func__, __LINE__, out.exit_reason, ret_tee);
				ret_tee = -EINVAL;
				break;
			}
			if (ret)
				break;
			cond_resched();
		} else if (out.ret == SMC_EXIT_NORMAL) {
			// return SMC_LOOP_EXIT_FINISH;
		} else {
			// tlogd("%s %d DONE\n", __func__, __LINE__);
			// return out.ret;
		}
	}

	return ret;
}

unsigned long smc_resume_npu_thread(int npu_core, void *job) {
	int npu_mask = 1 << npu_core;
	// tlogd("%s %d: resume core %d thread %lx job %lx\n", __func__, __LINE__, npu_core, waiting_npu_thread[npu_mask], (unsigned long)job);
	struct smc_in_params in = { TSP_REQUEST, (unsigned long)job, waiting_npu_thread[npu_mask] };
	struct smc_out_params out;
	do_smc_transport(&in, &out, 0);
	// tlogd("%s %d ret %d\n", __func__, __LINE__, out.ret);
	return 0;
}

EXPORT_SYMBOL(smc_resume_npu_thread);

static int llm_run(void __user * argp) {
	
	// tlogd("%s %d\n", __func__, __LINE__);
	struct out_result result;
	int copy_ret;
	// while (true) smc_call_cpu_resume(&result);
	unsigned long ret = smc_call_cpu_resume(&result);
	switch (ret) {
	case SMC_LOOP_EXIT_FINISH:
		// tlogd("%s %d finish llm inference\n", __func__, __LINE__);
		break;
	case SMC_LOOP_EXIT_NPU_SUBMIT:
		// tlogd("%s %d secure npu request core %d\n", __func__, __LINE__, result.npu_submit.npu_mask);
		if (copy_to_user(argp, &result.npu_submit.npu_mask, sizeof(int))) {
			tloge("copy llm npu core failed\n");
			return -EFAULT;
		}
		break;
	case SMC_LOOP_EXIT_IO_STEP:
		copy_ret = (int)ret;
		if (copy_to_user(argp, &copy_ret, sizeof(int))) {
			// tloge("copy io step ret failed\n");
			return -EFAULT;
		}
		break;
	default:
		// tloge("%s %d smc ret %d\n", __func__, __LINE__, ret);
		break;
	}

	return 0;
}

/*

struct mem_seg {
	unsigned long entry_begin;
	unsigned long entry_end;
};

static int llm_set_pages(struct file *file, void __user * argp) {
	struct llm_client_op_pages op;
	tlogd("%s %d\n", __func__, __LINE__);
	if (copy_from_user(&op, argp, sizeof(op))) {
		tloge("copy llm alloc_pages failed\n");
		return -EFAULT;
	}
	struct mem_seg *seg = (struct mem_seg *)file->private_data;
	seg->entry_begin = op.entry_begin;
	seg->entry_end = op.entry_end;
	return 0;
}

static int llm_alloc_pages(void __user * argp) {
	struct llm_client_op_pages op;
	tlogd("%s %d\n", __func__, __LINE__);
	if (copy_from_user(&op, argp, sizeof(op))) {
		tloge("copy llm alloc_pages failed\n");
		return -EFAULT;
	}
	return s2_alloc_pages(op.entry_begin, op.entry_end);
}

static int llm_free_pages(void __user * argp) {
	struct llm_client_op_pages op;
	tlogd("%s %d\n", __func__, __LINE__);
	if (copy_from_user(&op, argp, sizeof(op))) {
		tloge("copy llm free_pages failed\n");
		return -EFAULT;
	}
	return s2_free_pages(op.entry_begin, op.entry_end);
}

*/

static int llm_set_pages(struct file *file, void __user * argp) {
	struct llm_client_op_pages index;
	if (copy_from_user(&index, argp, sizeof(struct llm_client_op_pages))) {
		tloge("copy llm set_pages failed\n");
		return -EFAULT;
	}
	struct llm_client_op_pages *index_p = (struct llm_client_op_pages *)file->private_data;
	index_p->cma_index = index.cma_index;
	index_p->entry_index = index.entry_index;
	// tlogd("%s %d set index to %d\n", __func__, __LINE__, index);
	return 0;
}

static int llm_push_pages(void __user * argp) {
	struct llm_client_op_pages index;
	// tlogd("%s %d\n", __func__, __LINE__);
	if (copy_from_user(&index, argp, sizeof(index))) {
		tloge("copy llm alloc_pages failed\n");
		return -EFAULT;
	}
	index.entry_index = tzasc_cma_push_pages(index.size);
	index.cma_index = cur_cma_index;
	if (copy_to_user(argp, &index, sizeof(index))) {
		tloge("copy llm alloc_pages failed\n");
		return -EFAULT;
	}
	return 0;
}

static int llm_push_pages_with_index(void __user * argp) {
	struct llm_client_op_pages index;
	// tlogd("%s %d\n", __func__, __LINE__);
	if (copy_from_user(&index, argp, sizeof(index))) {
		tloge("copy llm alloc_pages failed\n");
		return -EFAULT;
	}
	index.entry_index = tzasc_cma_push_pages_with_index(index.size, index.cma_index);
	if (copy_to_user(argp, &index, sizeof(index))) {
		tloge("copy llm alloc_pages failed\n");
		return -EFAULT;
	}
	return 0;
}

static int llm_pop_pages(void __user * argp) {
	struct llm_client_op_pages index;
	// tlogd("%s %d\n", __func__, __LINE__);
	return tzasc_cma_pop_pages();
}

extern int rknpu_init_test(void);
extern void rknpu_exit_test(void);

static int llm_test_attach(void __user * argp) {
	(void)argp;
	return rknpu_init_test();
}

static int llm_test_detach(void __user * argp) {
	(void)argp;
	rknpu_exit_test();
	return 0;
}

static int llm_client_open(struct inode *inode, struct file *file) {
	file->private_data = kmalloc(sizeof(struct llm_client_op_pages), GFP_KERNEL);
	struct llm_client_op_pages *index_p = (struct llm_client_op_pages *)file->private_data;
	index_p->cma_index = -1;
	index_p->entry_index = -1;
	// tlogd("111 %s %d\n", __func__, __LINE__);
	return 0;
}

static int llm_client_close(struct inode *inode, struct file *file) {
	kfree(file->private_data);
	tzasc_cma_free_pages();
	// tlogd("%s %d\n", __func__, __LINE__);
	return 0;
}

static long llm_client_ioctl(struct file *file, unsigned int cmd,
	unsigned long arg)
{
	int ret = -EFAULT;
	void *argp = (void __user *)(uintptr_t)arg;

	handle_cmd_prepare(cmd);
	switch (cmd) {
	case LLM_CLIENT_IOCTL_RUN:
		// tlogd("%s %d\n", __func__, __LINE__);
		ret = llm_run(argp);
		// tlogd("%s %d: ret %d\n", __func__, __LINE__, ret);
		break;
	case LLM_CLIENT_IOCTL_PUSH_PAGES: {
		ret = llm_push_pages(argp);
		// tlogd("%s %d: ret %d\n", __func__, __LINE__, ret);
		break;
	}
	case LLM_CLIENT_IOCTL_PUSH_PAGES_WITH_INDEX: {
		ret = llm_push_pages_with_index(argp);
		// tlogd("%s %d: ret %d\n", __func__, __LINE__, ret);
		break;
	}
	case LLM_CLIENT_IOCTL_POP_PAGES: {
		ret = llm_pop_pages(argp);
		// tlogd("%s %d: ret %d\n", __func__, __LINE__, ret);
		break;
	}
	case LLM_CLIENT_IOCTL_SET_PAGES: {
		ret = llm_set_pages(file, argp);
		// tlogd("%s %d: ret %d\n", __func__, __LINE__, ret);
		break;
	}
	case LLM_CLIENT_IOCTL_RKNPU_ATTACH: {
		ret = llm_test_attach(argp);
		break;
	}
	case LLM_CLIENT_IOCTL_RKNPU_DETACH: {
		ret = llm_test_detach(argp);
		break;
	}
	default:
		ret = -ENOSYS;
	}

	// tlogd("%s ret = 0x%x\n", __func__, ret);
	return (long)ret;
}

static int llm_client_mmap(struct file *file, struct vm_area_struct *vma) {
	unsigned long size = vma->vm_end - vma->vm_start;

	unsigned long pfn;
	struct llm_client_op_pages *index_p = (struct llm_client_op_pages *)file->private_data;
	if (index_p->cma_index == -1) {
		if (size > CMD_QUEUE_SHM_SIZE) {
			tloge("%s %d try to mmap size=%lx\n", __func__, __LINE__, size);
			return -EINVAL;
		}
		pfn = virt_to_phys((const volatile void *)g_llm_shm) >> PAGE_SHIFT;
		if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
			tloge("%s %d\n", __func__, __LINE__);
			printk(KERN_ERR "Failed to map memory\n");
			return -EAGAIN;
		}
	} else {
		if (index_p->cma_index < 0 || index_p->cma_index >= TZASC_NR)
			return -EINVAL;
		struct tzasc_cma_meta *g_tzasc_cma_meta = g_tzasc_cma_meta_arr + index_p->cma_index;
		struct tzasc_cma_entry *entry = &g_tzasc_cma_meta->entry[index_p->entry_index];
		if (size > entry->size) {
			tloge("%s %d try to mmap size=%lx entry->size=%lx index=%d\n", __func__, __LINE__, size, entry->size, index_p->entry_index);
			return -EINVAL;
		}
		pfn = page_to_phys(entry->cma_pages) >> PAGE_SHIFT;
		if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
			tloge("%s %d\n", __func__, __LINE__);
			printk(KERN_ERR "Failed to map memory\n");
			return -EAGAIN;
		}
	}

	return 0;
}

static const struct file_operations g_llm_ns_client_fops = {
	.owner = THIS_MODULE,
	.open = llm_client_open,
	.release = llm_client_close,
	.unlocked_ioctl = llm_client_ioctl,
	.mmap = llm_client_mmap,
};

static const struct file_operations g_tc_ns_client_fops = {
	.owner = THIS_MODULE,
	.open = tc_client_open,
	.release = tc_client_close,
	.unlocked_ioctl = tc_client_ioctl,
	.mmap = tc_client_mmap,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tc_compat_client_ioctl,
#endif
};

static const struct file_operations g_teecd_fops = {
	.owner = THIS_MODULE,
	.open = tc_client_open,
	.release = tc_private_close,
	.unlocked_ioctl = tc_private_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tc_compat_private_ioctl,
#endif
};
#ifdef CONFIG_ACPI

static int tzdriver_probe(struct platform_device *pdev)
{
	tlogd("tzdriver probe is running");
	g_acpi_irq = platform_get_irq(pdev, 0);
	if (g_acpi_irq < 0) {
		dev_err(&pdev->dev, "get irq fail; irq:%d\n", g_acpi_irq);
		return g_acpi_irq;
	}

	return 0;
}

int get_acpi_tz_irq(void)
{
	return g_acpi_irq;
}

static const struct acpi_device_id g_tzdriver_acpi_match[] = {
	{ "HISI03C1", 0 },
	{},
};

MODULE_DEVICE_TABLE(acpi, g_tzdriver_acpi_match);

#else

static int tzdriver_probe(struct platform_device *pdev)
{
	(void)pdev;
	return 0;
}

struct of_device_id g_tzdriver_platform_match[] = {
	{ .compatible = "trusted_core" },
	{},
};

MODULE_DEVICE_TABLE(of, g_tzdriver_platform_match);

#endif

const struct dev_pm_ops g_tzdriver_pm_ops = {
	.freeze_noirq = tc_s4_pm_suspend,
	.restore_noirq = tc_s4_pm_resume,
};

static struct platform_driver g_tz_platform_driver = {
	.driver = {
		.name		= "trusted_core",
		.owner		= THIS_MODULE,
#ifdef CONFIG_ACPI
		.acpi_match_table = ACPI_PTR(g_tzdriver_acpi_match),
#else
		.of_match_table = of_match_ptr(g_tzdriver_platform_match),
#endif
		.pm = &g_tzdriver_pm_ops,
	},
	.probe = tzdriver_probe,
};

static int load_hw_info(void)
{
	if (platform_driver_register(&g_tz_platform_driver) != 0) {
		tloge("platform register driver failed\n");
		return -EFAULT;
	}

	/* load hardware info from dts and acpi */
	g_dev_node = of_find_compatible_node(NULL, NULL, "trusted_core");
	if (!g_dev_node) {
		tloge("no trusted_core compatible node found\n");
#ifndef CONFIG_ACPI
		platform_driver_unregister(&g_tz_platform_driver);
		return -ENODEV;
#endif
	}

	return 0;
}

static int create_dev_node(struct dev_node *node)
{
	int ret;
	if (!node || !(node->node_name)) {
		tloge("node or member is null\n");
		return -EFAULT;
	}
	if (alloc_chrdev_region(&(node->devt), 0, 1,
		node->node_name) != 0) {
		tloge("alloc chrdev region failed");
		ret = -EFAULT;
		return ret;
	}
	node->class_dev = device_create(node->driver_class, NULL, node->devt,
		NULL, node->node_name);
	if (IS_ERR_OR_NULL(node->class_dev)) {
		tloge("class device create failed");
		ret = -ENOMEM;
		goto chrdev_region_unregister;
	}
	node->class_dev->of_node = g_dev_node;

	cdev_init(&(node->char_dev), node->fops);
	(node->char_dev).owner = THIS_MODULE;

	return 0;

chrdev_region_unregister:
	unregister_chrdev_region(node->devt, 1);
	return ret;
}

static int init_dev_node(struct dev_node *node, char *node_name,
	struct class *driver_class, const struct file_operations *fops)
{
	int ret = -1;
	if (!node) {
		tloge("node is NULL\n");
		return ret;
	}
	node->node_name = node_name;
	node->driver_class = driver_class;
	node->fops = fops;

	ret = create_dev_node(node);
	return ret;
}

static void destory_dev_node(struct dev_node *node, struct class *driver_class)
{
	device_destroy(driver_class, node->devt);
	unregister_chrdev_region(node->devt, 1);
	return;
}

static int enable_dev_nodes(void)
{
	int ret;

	ret = cdev_add(&(g_tc_private.char_dev),
		MKDEV(MAJOR(g_tc_private.devt), 0), 1);
	if (ret < 0) {
		tloge("cdev add failed %d", ret);
		return ret;
	}

	ret = cdev_add(&(g_tc_client.char_dev),
		MKDEV(MAJOR(g_tc_client.devt), 0), 1);
	if (ret < 0) {
		tloge("cdev add failed %d", ret);
		cdev_del(&(g_tc_private.char_dev));
		return ret;
	}

	return 0;
}

static int llm_client_init(void)
{
	tlogd("%s %d\n", __func__, __LINE__);
	int ret = 0;

	g_driver_class = class_create(THIS_MODULE, TC_NS_CLIENT_DEV);
	if (IS_ERR_OR_NULL(g_driver_class)) {
		tloge("class create failed");
		return -ENOMEM;
	}

	ret = init_dev_node(&g_tc_client, TC_NS_CLIENT_DEV, g_driver_class, &g_llm_ns_client_fops);
	if (ret != 0) {
		tloge("init_dev_node failed");
		return ret;
	}

	ret = cdev_add(&(g_tc_client.char_dev),
		MKDEV(MAJOR(g_tc_client.devt), 0), 1);
	if (ret < 0) {
		tloge("cdev add failed %d", ret);
		return ret;
	}

	return 0;
}

static int llm_tee_os_init(void)
{
	tlogd("%s %d\n", __func__, __LINE__);
	int ret;
	g_llm_shm = __get_free_pages(GFP_KERNEL | __GFP_ZERO, 8);
	if (g_llm_shm == 0) {
		return -ENOMEM;
	} else {
		ret = 0;
	}

	ret = s2_init();
	if (ret < 0) {
		return -ENOMEM;
	}

	ret = tzasc_cma_init();
	if (ret < 0) {
		return ret;
	}

	while (true) {
		struct smc_in_params in = {
			.x0 = TSP_REQUEST,
			.x1 = virt_to_phys((const volatile void *)g_llm_shm),
			.x2 = CMD_QUEUE_SHM_SIZE,
			.x3 = virt_to_phys((const volatile void *)g_s2_l0_meta),
			.x4 = virt_to_phys((const volatile void *)g_tzasc_cma_meta_arr),
		};
		struct smc_out_params out;
		do_smc_transport(&in, &out, 0);

		if (out.ret == SMC_EXIT_PREEMPTED) {
			cond_resched();
		} else {
			tlogd("%s %d shm init DONE\n", __func__, __LINE__);
			break;
		}
	}

	// tlogd("test %lu\n", get_s2_l1_meta_entry(g_s2_l1_meta_arr_s, 600000)->size);

	return ret;
}

static int tc_ns_client_init(void)
{
	int ret;
	ret = load_hw_info();
	if (ret != 0)
		return ret;

	ret = load_reserved_mem();
	if (ret != 0)
		return ret;

	ret = load_tz_shared_mem(g_dev_node);
	if (ret != 0)
		goto unmap_res_mem;
	g_driver_class = class_create(THIS_MODULE, TC_NS_CLIENT_DEV);
	if (IS_ERR_OR_NULL(g_driver_class)) {
		tloge("class create failed");
		ret = -ENOMEM;
		goto unmap_res_mem;
	}

	ret = init_dev_node(&g_tc_client, TC_NS_CLIENT_DEV, g_driver_class, &g_tc_ns_client_fops);
	if (ret != 0) {
		class_destroy(g_driver_class);
		goto unmap_res_mem;
	}
	ret = init_dev_node(&g_tc_private, TC_PRIV_DEV, g_driver_class, &g_teecd_fops);
	if (ret != 0) {
		destory_dev_node(&g_tc_client, g_driver_class);
		class_destroy(g_driver_class);
		goto unmap_res_mem;
	}

	INIT_LIST_HEAD(&g_tc_ns_dev_list.dev_file_list);
	mutex_init(&g_tc_ns_dev_list.dev_lock);
	init_crypto_hash_lock();
	init_srvc_list();
	return ret;
unmap_res_mem:
	unmap_res_mem();
	return ret;
}

static int tc_teeos_init(struct device *class_dev)
{
	int ret;

	ret = smc_context_init(class_dev);
	if (ret != 0) {
		tloge("smc context init failed\n");
		return ret;
	}

	ret = tee_init_reboot_thread();
	if (ret != 0) {
		tloge("init reboot thread failed\n");
		goto smc_data_free;
	}

	ret = reserved_mempool_init();
	if (ret != 0) {
		tloge("reserved memory init failed\n");
		goto reboot_thread_free;
	}

	ret = mailbox_mempool_init();
	if (ret != 0) {
		tloge("tz mailbox init failed\n");
		goto release_resmem;
	}

	// ret = tz_spi_init(class_dev, g_dev_node);
	// if (ret != 0) {
	// 	tloge("tz spi init failed\n");
	// 	goto release_mempool;
	// }

	return 0;
release_mempool:
	free_mailbox_mempool();
release_resmem:
	free_reserved_mempool();
reboot_thread_free:
	free_reboot_thread();
smc_data_free:
	free_smc_data();
	return ret;
}

static void tc_re_init(const struct device *class_dev)
{
	int ret;

	agent_init();
	ret = tc_ns_register_ion_mem();
	if (ret != 0)
		tloge("Failed to register ion mem in tee\n");

#ifdef CONFIG_TZDRIVER_MODULE
	ret = init_tlogger_service();
	if (ret != 0)
		tloge("tlogger init failed\n");
#endif
	if (tzdebug_init() != 0)
		tloge("tzdebug init failed\n");

	ret = init_tui(class_dev);
	if (ret != 0)
		tloge("init_tui failed 0x%x\n", ret);

#ifndef CONFIG_DISABLE_SVC
	if (init_smc_svc_thread() != 0) {
		tloge("init svc thread\n");
		ret = -EFAULT;
	}
#endif

	if (init_dynamic_mem() != 0) {
		tloge("init dynamic mem Failed\n");
		ret = -EFAULT;
	}

#ifdef CONFIG_LIVEPATCH_ENABLE
	/*
	 * access this sys node only after this function is initialized
	 */
	if (livepatch_init(class_dev)) {
		tloge("livepatch init failed\n");
		ret = -EFAULT;
	}
#endif

	if (ret != 0)
		tloge("Caution! Running environment init failed!\n");
}

static __init int tc_init(void)
{
	int ret = 0;

	ret = llm_tee_os_init();
	if (ret != 0)
		return ret;
	ret = llm_client_init();
	if (ret != 0)
		return ret;
	tlogd("%s %d: tc_init finish\n", __func__, __LINE__);
	return ret;

	init_kthread_cpumask();
	ret = tc_ns_client_init();
	if (ret != 0)
		return ret;

#ifdef CONFIG_FFA_SUPPORT
	ffa_abi_register();
#endif

	ret = tc_teeos_init(g_tc_client.class_dev);
	if (ret != 0) {
		tloge("tc teeos init failed\n");
		goto class_device_destroy;
	}
	/* run-time environment init failure don't block tzdriver init proc */
	tc_re_init(g_tc_client.class_dev);

	/*
	 * Note: the enable_dev_nodes function must be called
	 * at the end of tc_init
	 */
	ret = enable_dev_nodes();
	if (ret != 0) {
		tloge("enable dev nodes failed\n");
		goto class_device_destroy;
	}

	ret = alloc_dev_bitmap();
	if (ret != 0) {
		tloge("alloc dev file id bitmap failed\n");
		goto class_device_destroy;
	}

	set_tz_init_flag();
	return 0;

class_device_destroy:
	free_dev_bitmap();
	destory_dev_node(&g_tc_client, g_driver_class);
	destory_dev_node(&g_tc_private, g_driver_class);
	class_destroy(g_driver_class);
	platform_driver_unregister(&g_tz_platform_driver);
	return ret;
}

static void free_dev_list(void)
{
	struct tc_ns_dev_file *dev_file = NULL, *temp = NULL;

	mutex_lock(&g_tc_ns_dev_list.dev_lock);
	list_for_each_entry_safe(dev_file, temp, &g_tc_ns_dev_list.dev_file_list, head) {
		list_del(&dev_file->head);
		kfree(dev_file);
	}
	mutex_unlock(&g_tc_ns_dev_list.dev_lock);
}

static void tc_exit(void)
{
	tlogi("tz client exit");
	clear_tz_init_flag();
	/*
	 * You should first execute "cdev_del" to 
	 * prevent access to the device node when uninstalling "tzdriver".
	 */
	cdev_del(&(g_tc_private.char_dev));
	cdev_del(&(g_tc_client.char_dev));
	free_agent();
	free_reboot_thread();
	free_tui();
	free_tz_spi(g_tc_client.class_dev);
	/* run-time environment exit should before teeos exit */

	destory_dev_node(&g_tc_client, g_driver_class);
	destory_dev_node(&g_tc_private, g_driver_class);
	free_dev_bitmap();
	platform_driver_unregister(&g_tz_platform_driver);
	class_destroy(g_driver_class);
	free_smc_data();
	free_event_mem();
#ifdef CONFIG_TZDRIVER_MODULE
	free_tzdebug();
	free_tlogger_service();
#endif
	free_interrupt_trace();
	free_mailbox_mempool();
	free_reserved_mempool();
	free_shash_handle();
	fault_monitor_end();
	free_livepatch();
	free_all_session();
	free_dev_list();
#ifdef CONFIG_FFA_SUPPORT
	ffa_abi_unregister();
#endif
	tlogi("tz client exit finished");
}

MODULE_AUTHOR("iTrustee");
MODULE_DESCRIPTION("TrustCore ns-client driver");
MODULE_VERSION("1.10");

#ifdef CONFIG_TZDRIVER_MODULE
module_init(tc_init);
#else
fs_initcall_sync(tc_init);
#endif
module_exit(tc_exit);
MODULE_LICENSE("GPL");
