#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/memory_hotplug.h>
#include <linux/mm.h>
#include "cdm.h"
#include "sysfs.h"

int cdm_up(struct cdm_device *cdmdev)
{
	resource_size_t addr = cdmdev->res.start;
	resource_size_t size = resource_size(&cdmdev->res);
	int rc, nid = dev_to_node(cdmdev->miscdev.this_device);

	memhp_auto_online = false;

	rc = add_memory(nid, addr, size);
	if (rc && rc != -EEXIST)
		return rc;

	/*
	 * FIXME: Find a non-sysfs way for a driver to online memory.
	 */
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
	resource_size_t size = resource_size(&cdmdev->res);
	int nid = dev_to_node(cdmdev->miscdev.this_device);

	/*
	 * FIXME: Find a non-sysfs way for a driver to offline memory.
	 */
	if (store_mem_state(addr, size, "offline"))
		pr_err("error offlining [0x%lx..0x%lx]\n",
		       (unsigned long)addr, (unsigned long)cdmdev->res.end);

	remove_memory(nid, addr, size);
}

#ifdef CONFIG_MEMORY_FAILURE
int cdm_poison(phys_addr_t addr)
{
	return memory_failure(PFN_DOWN(addr), 0, 0);
}
#endif
