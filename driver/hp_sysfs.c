#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/printk.h>
#include "cdm.h"
#include "sysfs.h"

int cdm_up(struct cdm_device *cdmdev)
{
	resource_size_t addr = cdmdev->res.start;
	resource_size_t size = resource_size(&cdmdev->res);
	int rc;

	rc = store_auto_online_blocks("offline");
	if (rc)
		return rc;

	rc = memory_probe_store(addr, size);
	if (rc)
		return rc;

	rc = store_mem_state(addr, size, "online_movable");
	if (rc) {
		store_mem_state(addr, size, "offline");
		return rc;
	}

	return 0;
}

void cdm_down(struct cdm_device *cdmdev)
{
	resource_size_t addr = cdmdev->res.start;

	/*
	 * We can offline memory, but there is no non-GPL way to remove it.
	 */
	if (store_mem_state(addr, resource_size(&cdmdev->res), "offline"))
		pr_err("error offlining [0x%lx..0x%lx]\n",
		       (unsigned long)addr, (unsigned long)cdmdev->res.end);
}

#ifdef CONFIG_MEMORY_FAILURE
int cdm_poison(phys_addr_t addr)
{
	return store_hard_offline_page(addr);
}
#endif
