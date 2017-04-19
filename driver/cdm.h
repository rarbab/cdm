#ifndef CDM_H
#define CDM_H

#include <linux/miscdevice.h>

struct cdm_device {
	struct miscdevice miscdev;
	struct resource res;
};

static inline int cdmdev_to_node(struct cdm_device *cdmdev)
{
	return dev_to_node(cdmdev->miscdev.this_device);
}

#endif /* CDM_H */
