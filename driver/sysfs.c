/*
 * Copyright (C) 2017 IBM Corporation
 * Author: Reza Arbab <arbab@linux.vnet.ibm.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/uaccess.h>

#define SYSFS_MEM(file) __stringify(/sys/devices/system/memory/file)

static ssize_t write_buf(char *filename, char *buf)
{
	mm_segment_t old_fs;
	struct file *filp;
	loff_t pos = 0;
	int ret;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_WRONLY, 0);
	if (IS_ERR(filp)) {
		ret = PTR_ERR(filp);
		goto err;
	}

	ret = vfs_write(filp, buf, strlen(buf), &pos);
	filp_close(filp, NULL);
err:
	set_fs(old_fs);

	return ret;
}

static ssize_t read_buf(char *filename, char *buf, ssize_t count)
{
	mm_segment_t old_fs;
	struct file *filp;
	loff_t pos = 0;

	if (!count)
		return 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		count = PTR_ERR(filp);
		goto err;
	}

	count = vfs_read(filp, buf, count - 1, &pos);
	buf[count] = '\0';
	filp_close(filp, NULL);
err:
	set_fs(old_fs);

	return count;
}

static unsigned long read_0x(char *filename)
{
	unsigned long ret;
	char buf[32];

	if (read_buf(filename, buf, 32) <= 0)
		return 0;

	if (kstrtoul(buf, 16, &ret))
		return 0;

	return ret;
}

static unsigned long memory_block_size_bytes(void)
{
	static unsigned long block_sz = 0;

	if (!block_sz)
		block_sz = read_0x(SYSFS_MEM(block_size_bytes));

	return block_sz;
}

static inline unsigned int block_num(phys_addr_t addr)
{
	unsigned long block_sz = memory_block_size_bytes();

	if (block_sz)
		return addr / block_sz;

	return 0;
}

int store_auto_online_blocks(char *state)
{
	ssize_t count = write_buf(SYSFS_MEM(auto_online_blocks), state);

	if (count < 0)
		return count;

	return 0;
}

int memory_probe_store(phys_addr_t addr, phys_addr_t size)
{
	unsigned long block_sz = memory_block_size_bytes();
	phys_addr_t end = addr + size;

	for (; addr < end; addr += block_sz) {
		ssize_t count;
		char s[32];

		snprintf(s, 32, "0x%llx", addr);

		count = write_buf(SYSFS_MEM(probe), s);
		if (count < 0 && count != -EEXIST)
			return count;
	}

	return 0;
}

int store_mem_state(phys_addr_t addr, phys_addr_t size, char *state)
{
	unsigned int i = block_num(addr + size - 1);

	for (; i >= block_num(addr); i--) {
		char filename[64];
		ssize_t count;

		snprintf(filename, 64, SYSFS_MEM(memory%d/state), i);

		count = write_buf(filename, state);
		if (count < 0 && count != -EINVAL)
			return count;
	}

	return 0;
}

#ifdef CONFIG_MEMORY_FAILURE
int store_hard_offline_page(phys_addr_t addr)
{
	ssize_t count;
	char s[32];

	snprintf(s, 32, "0x%llx", addr);

	count = write_buf(SYSFS_MEM(hard_offline_page), s);
	if (count < 0)
		return count;

	return 0;
}
#endif
