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

#include <linux/hmm.h>
#include "cdm.h"

#define PAGE_USED 0xdeadbeef

struct page *cdm_devmem_alloc(struct cdm_device *cdmdev)
{
	struct hmm_devmem *devmem = cdmdev->devmem;
	unsigned long pfn;

	for (pfn = devmem->pfn_first; pfn < devmem->pfn_last; pfn++) {
		struct page *page = pfn_to_page(pfn);

		if (hmm_devmem_page_get_drvdata(page) == PAGE_USED)
			continue;

		hmm_devmem_page_set_drvdata(page, PAGE_USED);
		get_page(page);
		return page;
	}

	return NULL;
}

static void cdm_devmem_free(struct hmm_devmem *devmem, struct page *page)
{
	hmm_devmem_page_set_drvdata(page, 0);
}

static int cdm_devmem_fault(struct hmm_devmem *devmem,
			    struct vm_area_struct *vma, unsigned long addr,
			    const struct page *page, unsigned int flags,
			    pmd_t *pmdp)
{
	/* Will this ever happen with CDM? */
	WARN_ON(1);

	return VM_FAULT_SIGSEGV;
}

static const struct hmm_devmem_ops cdm_devmem_ops = {
	.free = cdm_devmem_free,
	.fault = cdm_devmem_fault
};

int cdm_devmem_init(struct cdm_device *cdmdev)
{
	cdmdev->devmem = hmm_devmem_add_resource(&cdm_devmem_ops,
					 cdmdev_dev(cdmdev), &cdmdev->res);

	return PTR_ERR_OR_ZERO(cdmdev->devmem);
}

void cdm_devmem_remove(struct cdm_device *cdmdev)
{
	hmm_devmem_remove(cdmdev->devmem);
	cdmdev->devmem = NULL;
}
