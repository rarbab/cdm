#ifndef CDM_SYSFS_H
#define CDM_SYSFS_H

#include <linux/types.h>

int store_auto_online_blocks(char *state);
int memory_probe_store(phys_addr_t addr, phys_addr_t size);
int store_mem_state(phys_addr_t addr, phys_addr_t size, char *state);
int store_hard_offline_page(phys_addr_t addr);

#endif /* CDM_SYSFS_H */
