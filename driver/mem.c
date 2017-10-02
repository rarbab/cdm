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

extern const struct migrate_dma_ops cdm_migrate_ops;

struct cdm_drvdata {
	struct list_head page_list;
	unsigned long pfn;
	unsigned long expires;
};

static inline struct cdm_drvdata *get_drvdata(struct page *page)
{
	return (struct cdm_drvdata *)hmm_devmem_page_get_drvdata(page);
}

static inline void set_drvdata(struct page *page, struct cdm_drvdata *drvdata)
{
	hmm_devmem_page_set_drvdata(page, (unsigned long)drvdata);
}

struct page *cdm_devmem_alloc(struct cdm_device *cdmdev)
{
	struct hmm_devmem *devmem = cdmdev->devmem;
	unsigned long pfn;

	for (pfn = devmem->pfn_first; pfn < devmem->pfn_last; pfn++) {
		struct page *page = pfn_to_page(pfn);
		struct cdm_drvdata *drvdata;

		if (get_drvdata(page))
			continue;

		drvdata = kmalloc(sizeof(*drvdata), GFP_KERNEL);
		if (!drvdata)
			break;

		drvdata->pfn = pfn;
		drvdata->expires = jiffies + msecs_to_jiffies(5 * 1000);

		INIT_LIST_HEAD(&drvdata->page_list);
		list_add_tail(&drvdata->page_list, &cdmdev->page_list);

		set_drvdata(page, drvdata);
		get_page(page);
		return page;
	}

	return NULL;
}

static void cdm_devmem_free(struct hmm_devmem *devmem, struct page *page)
{
	struct cdm_drvdata *drvdata = get_drvdata(page);

	list_del(&drvdata->page_list);

	kfree(drvdata);
	set_drvdata(page, NULL);
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

static inline void schedule_evict(struct delayed_work *dwork)
{
	schedule_delayed_work(dwork, msecs_to_jiffies(1000));
}

static struct list_head *cdm_devmem_evict_collect(struct cdm_device *cdmdev,
						  struct migrate_dma_ctx *ctx,
						  struct list_head *cursor,
						  unsigned long size)
{
	struct cdm_drvdata *drvdata;
	unsigned long i;

	for (i = 0; i < size; i++) {
		if (cursor == &cdmdev->page_list)
			break;

		drvdata = list_entry(cursor, typeof(*drvdata), page_list);

		if (time_before(jiffies, drvdata->expires))
			break;

		get_page(pfn_to_page(drvdata->pfn));

		ctx->src[ctx->npages] = migrate_pfn(drvdata->pfn);
		ctx->src[ctx->npages++] |= MIGRATE_PFN_MIGRATE;
		ctx->cpages++;

		cursor = cursor->next;
	}

	return cursor;
}

static void cdm_devmem_evict(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct cdm_device *cdmdev;
	struct list_head *cursor;

	cdmdev = container_of(dwork, struct cdm_device, dwork);
	cursor = cdmdev->page_list.next;

	while (cursor != &cdmdev->page_list) {
		unsigned long src[64], dst[64];
		struct migrate_dma_ctx ctx = {
			.ops = &cdm_migrate_ops,
			.src = src,
			.dst = dst,
		};
		int rc;

		cursor = cdm_devmem_evict_collect(cdmdev, &ctx, cursor,
						  ARRAY_SIZE(src));

		rc = migrate_dma(&ctx);
		if (rc)
			pr_err("%s: migrate_dma() returns %d\n", __func__, rc);
	}

	schedule_evict(dwork);
}

static const struct hmm_devmem_ops cdm_devmem_ops = {
	.free = cdm_devmem_free,
	.fault = cdm_devmem_fault
};

static void cdm_devmem_init_drvdata(struct hmm_devmem *devmem)
{
	unsigned long pfn;

	for (pfn = devmem->pfn_first; pfn < devmem->pfn_last; pfn++) {
		struct page *page = pfn_to_page(pfn);

		set_drvdata(page, NULL);
	}
}

int cdm_devmem_add(struct cdm_device *cdmdev)
{
	struct hmm_devmem *devmem = hmm_devmem_add_resource(&cdm_devmem_ops,
					cdmdev_dev(cdmdev), &cdmdev->res);

	if (IS_ERR(devmem))
		return PTR_ERR(devmem);

	cdm_devmem_init_drvdata(devmem);
	cdmdev->devmem = devmem;

	INIT_DELAYED_WORK(&cdmdev->dwork, cdm_devmem_evict);
	schedule_evict(&cdmdev->dwork);

	return 0;
}

void cdm_devmem_remove(struct cdm_device *cdmdev)
{
	if (!cancel_delayed_work(&cdmdev->dwork))
		flush_delayed_work(&cdmdev->dwork);

	hmm_devmem_remove(cdmdev->devmem);
	cdmdev->devmem = NULL;
}
