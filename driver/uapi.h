#ifndef CDM_UAPI_H
#define CDM_UAPI_H

#include <asm/types.h>

struct cdm_migrate {
	/* from */
	__u64 start;
	__u64 end;
	/* to */
	__u32 node;

	__u8 pad[4];
};

#define CDM_IOC_MAGIC 0xBB

#define CDM_IOC_UP _IO(CDM_IOC_MAGIC, 1)
#define CDM_IOC_DOWN _IO(CDM_IOC_MAGIC, 2)
#define CDM_IOC_MIGRATE _IOW(CDM_IOC_MAGIC, 3, struct cdm_migrate)

#endif /* CDM_UAPI_H */
