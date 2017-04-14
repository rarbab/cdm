#ifndef CDM_H
#define CDM_H

#include <linux/miscdevice.h>

struct cdm_device {
	struct miscdevice miscdev;
	struct resource res;
};

#endif /* CDM_H */
