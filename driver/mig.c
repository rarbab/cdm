#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/migrate.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include "uapi.h"

#ifdef CONFIG_HMM

static void alloc_and_copy(struct vm_area_struct *vma,
			   const unsigned long *src, unsigned long *dst,
			   unsigned long start, unsigned long end,
			   void *private)
{
	int node = *(int *)private;
	unsigned long i;

	for (i = 0; i < (end - start) >> PAGE_SHIFT; i++) {
		struct page *page;

		if (!(src[i] & MIGRATE_PFN_MIGRATE))
			continue;

		page = alloc_pages_node(node, GFP_HIGHUSER_MOVABLE, 0);
		if (!page) {
			pr_err("%s: failed to alloc dst[%ld]\n", __func__, i);
			dst[i] = 0;
			continue;
		}

		/* use dma here */
		copy_highpage(page, migrate_pfn_to_page(src[i]));

		lock_page(page);
		dst[i] = migrate_pfn(page_to_pfn(page)) | MIGRATE_PFN_LOCKED;
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

int cdm_migrate(struct cdm_migrate *mig)
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
				 &mig->node);
		if (rc)
			return rc;
	}

	return 0;
}

#else

int cdm_migrate(struct cdm_migrate *mig)
{
	return -ENOSYS;
}

#endif /* CONFIG_HMM */
