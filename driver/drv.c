#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "cdm.h"
#include "uapi.h"

int cdm_up(struct cdm_device *cdmdev);
void cdm_down(struct cdm_device *cdmdev);
int cdm_migrate(struct cdm_migrate *mig);

#define CDM_DEVICE_MAX 6

static struct cdm_device *cdm_device[CDM_DEVICE_MAX];
static unsigned int ncdm = 0;

static long cdm_fops_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	struct miscdevice *miscdev = file->private_data;
	struct cdm_device *cdmdev;
	struct cdm_migrate mig;
	int rc;

	if (!miscdev)
		return -EINVAL;

	cdmdev = container_of(miscdev, struct cdm_device, miscdev);

	switch (cmd) {
	case CDM_IOC_UP:
		return cdm_up(cdmdev);
	case CDM_IOC_DOWN:
		cdm_down(cdmdev);
		return 0;
	case CDM_IOC_MIGRATE:
		rc = copy_from_user(&mig, (void __user *)arg, sizeof(mig));
		if (rc)
			return rc;

		return cdm_migrate(&mig);
	}

	return -ENOTTY;
}

static const struct file_operations cdm_device_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cdm_fops_ioctl
};

static int cdm_miscdev_init(struct miscdevice *miscdev)
{
	char *name;
	int rc;

	name = kmalloc(7, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	snprintf(name, 7, "cdm%d", ncdm);
	miscdev->name = name;

	miscdev->minor = MISC_DYNAMIC_MINOR;
	miscdev->fops = &cdm_device_fops;
	miscdev->mode = 0444;

	rc = misc_register(miscdev);
	if (rc)
		kfree(name);

	return rc;
}

static void cdm_miscdev_remove(struct miscdevice *miscdev)
{
	misc_deregister(miscdev);
	kfree(miscdev->name);
}

static int cdm_resource_init(struct resource *res, struct device_node *dn)
{
	const __be32 *addrp;
	u64 size;

	addrp = of_get_address(dn, 0, &size, NULL);
	res->start = of_translate_address(dn, addrp);

	if (!addrp || res->start == OF_BAD_ADDR)
		return -EINVAL;

	res->flags = IORESOURCE_MEM;
	res->end = res->start + size - 1;
	res->name = dn->full_name;

	return 0;
}

static int cdm_device_probe(struct device_node *dn)
{
	struct cdm_device *cdmdev;
	struct device *dev;
	int nid, rc;

	if (ncdm >= CDM_DEVICE_MAX)
		return 0;

	cdmdev = kzalloc(sizeof(*cdmdev), GFP_KERNEL);
	if (!cdmdev)
		return -ENOMEM;

	rc = cdm_resource_init(&cdmdev->res, dn);
	if (rc)
		goto err;

	nid = of_node_to_nid(dn);
	if (nid == NUMA_NO_NODE) {
		rc = -EINVAL;
		goto err;
	}

	rc = cdm_miscdev_init(&cdmdev->miscdev);
	if (rc)
		goto err;

	dev = cdmdev->miscdev.this_device;
	set_dev_node(dev, nid);
	dev->of_node = dn;

	cdm_device[ncdm++] = cdmdev;
	return 0;

err:
	kfree(cdmdev);
	return rc;
}

static void cdm_device_remove(struct cdm_device *cdmdev)
{
	cdm_miscdev_remove(&cdmdev->miscdev);
	kfree(cdmdev);
}

static void cdm_device_remove_all(void)
{
	while (ncdm)
		cdm_device_remove(cdm_device[--ncdm]);
}

static int __init cdm_init(void)
{
	struct device_node *dn;

	for_each_compatible_node(dn, NULL, "ibm,coherent-device-memory") {
		int rc = cdm_device_probe(dn);

		if (rc) {
			cdm_device_remove_all();
			of_node_put(dn);
			return rc;
		}
	}

	return 0;
}

static void __exit cdm_exit(void)
{
	cdm_device_remove_all();
}

module_init(cdm_init);
module_exit(cdm_exit);

MODULE_LICENSE("non-GPL");
