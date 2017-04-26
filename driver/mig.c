#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/migrate.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include "cdm.h"
#include "uapi.h"

extern nodemask_t cdm_nmask;

#ifdef MIGRATE_PFN_MIGRATE

static struct page *alloc_page_cdm(struct cdm_device *cdmdev,
				   struct page *old)
{
	gfp_t gfp = GFP_HIGHUSER_MOVABLE;
	struct zonelist *zl;
	nodemask_t nmask;

	if (cdmdev)
		node_set(cdmdev_to_node(cdmdev), nmask);
	else
		nodes_andnot(nmask, node_states[N_MEMORY], cdm_nmask);

	zl = node_zonelist(page_to_nid(old), gfp);
	return __alloc_pages_nodemask(gfp, 0, zl, &nmask);
}

static void alloc_and_copy(struct vm_area_struct *vma,
			   const unsigned long *src, unsigned long *dst,
			   unsigned long start, unsigned long end,
			   void *private)
{
	struct cdm_device *cdmdev = (struct cdm_device *)private;
	unsigned long i;

	for (i = 0; i < (end - start) >> PAGE_SHIFT; i++) {
		struct page *spage, *dpage;

		if (!(src[i] & MIGRATE_PFN_MIGRATE))
			continue;

		spage = migrate_pfn_to_page(src[i]);
		dpage = alloc_page_cdm(cdmdev, spage);

		if (!dpage) {
			pr_err("%s: failed to alloc dst[%ld]\n", __func__, i);
			dst[i] = 0;
			continue;
		}

		/* use dma here */
		copy_highpage(dpage, spage);

		lock_page(dpage);
		dst[i] = migrate_pfn(page_to_pfn(dpage)) | MIGRATE_PFN_LOCKED;
	}
}

static void finalize_and_map(struct vm_area_struct *vma,
			     const unsigned long *src,
			     const unsigned long *dst,
			     unsigned long start, unsigned long end,
			     void *private)
{
	unsigned long i;

	for (i = 0; i < (end - start) >> PAGE_SHIFT; i++) {
		if (src[i] & MIGRATE_PFN_MIGRATE)
			continue;

		pr_err("%s: src[%ld] not migrated\n", __func__, i);
	}
}

const struct migrate_vma_ops ops =
{
	.alloc_and_copy = alloc_and_copy,
	.finalize_and_map = finalize_and_map
};

int cdm_migrate(struct cdm_device *cdmdev, struct cdm_migrate *mig)
{
	struct vm_area_struct *vma;
	unsigned long addr, next;

	vma = find_vma_intersection(current->mm, mig->start, mig->end);

	for (addr = mig->start; addr < mig->end; addr = next) {
		unsigned long mentries, src[64], dst[64];
		int rc;

		next = min_t(unsigned long, mig->end,
			     addr + (64 << PAGE_SHIFT));
		mentries = (next - addr) >> PAGE_SHIFT;

		rc = migrate_vma(&ops, vma, mentries, addr, next, src, dst,
				 cdmdev);
		if (rc)
			return rc;
	}

	return 0;
}

#else

int cdm_migrate(struct cdm_device *cdmdev, struct cdm_migrate *mig)
{
	return -ENOSYS;
}

#endif /* CONFIG_HMM */
