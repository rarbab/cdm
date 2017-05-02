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

#ifndef CDM_SYSFS_H
#define CDM_SYSFS_H

#include <linux/types.h>

int store_auto_online_blocks(char *state);
int memory_probe_store(phys_addr_t addr, phys_addr_t size);
int store_mem_state(phys_addr_t addr, phys_addr_t size, char *state);
int store_hard_offline_page(phys_addr_t addr);

#endif /* CDM_SYSFS_H */
