#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "driver/uapi.h"

int main(int argc, char *argv[])
{
	int fd, cmd, rc;

	if (argc != 3)
		return -EINVAL;

	if (!strcmp(argv[2], "up"))
		cmd = CDM_IOC_UP;
	else if (!strcmp(argv[2], "down"))
		cmd = CDM_IOC_DOWN;
	else
		return -EINVAL;

	fd = open(argv[1], O_NONBLOCK);
	if (fd < 0) {
		perror(argv[1]);
		return fd;
	}

	rc = ioctl(fd, cmd);
	close(fd);
	if (rc) {
		perror("ioctl");
		return -errno;
	}

	return 0;
}
