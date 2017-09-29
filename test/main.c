#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "memalloc.h"

#define BUFFER_SIZE 1024*1024

int main(int argc, char **argv)
{
	int fd, buffer_id, status, i;
	int *vaddr;
	unsigned long paddr;

	struct ioctl_arg_t ioctl_arg;


	printf("INFO: Ready to allocate %d bytes of contiguous memory\n", BUFFER_SIZE);

	/* Open character device */
	fd = open("/dev/memalloc", O_RDWR);
	if (fd < 0)
	{
		printf("ERROR: Open /dev/memalloc: FAILED\n");
		return(-1);
	}
	printf("INFO: Open /dev/memalloc: DONE\n");

	/* Reserve a buffer of the given size, it returns an id */
	ioctl_arg.buffer_size = BUFFER_SIZE;
	status = ioctl(fd, MEMALLOC_RESERVE_CMD, &ioctl_arg);
	buffer_id = ioctl_arg.buffer_id;
	if (buffer_id < 0 || status < 0)
	{
		printf("ERROR: memalloc->RESERVE : FAILED (id %d, status %d)\n", buffer_id, status);
		return(-1);
	}
	printf("INFO: memalloc->RESERVE : DONE (id %d)\n", buffer_id);

	/* Get the physical address of the buffer of the given id */
	ioctl_arg.buffer_id = buffer_id;
	status = ioctl(fd, MEMALLOC_GET_PHYSICAL_CMD, &ioctl_arg);
        paddr = ioctl_arg.phys_addr;
	if (paddr == 0 || status < 0)
	{
		printf("ERROR: memalloc->GET-PHYSICAL : FAILED (paddr %p, status %d)\n", paddr, status);
		return(-1);
	}
	printf("INFO: memalloc->GET-PHYSICAL : DONE (paddr %p)\n", paddr);
	
	/* Activate the buffer of the given id */
	ioctl_arg.buffer_id = buffer_id;
	status = ioctl(fd, MEMALLOC_ACTIVATE_BUFFER_CMD, &ioctl_arg);
	if (status < 0)
	{
		printf("ERROR: memalloc->ACTIVATE-BUFFER : FAILED (status %d)\n", status);
		return(-1);
	}
	printf("INFO: memalloc->ACTIVATE-BUFFER : DONE (status %d)\n", status);

	vaddr = mmap(0, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (vaddr == NULL)
	{
		printf("ERROR: Get virtual address : FAILED\n");
		return(-1);
	}
	printf("INFO: Get virtual address : DONE (u-vaddr %p)\n", vaddr);

	printf("INFO: =============================\n");
	printf("INFO: |                           |\n");
	printf("INFO:   paddr:   %p\n", paddr);
	printf("INFO:   u-vaddr: %p\n", vaddr);
	printf("INFO:   bytes:   %d\n", BUFFER_SIZE);
	printf("INFO: |                           |\n");
	printf("INFO: =============================\n");

	printf("INFO: Test buffer (R/W operations %d)\n", BUFFER_SIZE/sizeof(int));
	for (i = 0; i < BUFFER_SIZE/sizeof(int); i++)
	{
		((int*)vaddr)[i] = i * 2 + 1;
	}

	status = 0;
	for (i = 0; i < BUFFER_SIZE/sizeof(int); i++)
	{
		if (((int*)vaddr)[i] != (i * 2 + 1));
	}
	if (status != 0)
	{
		printf("ERROR: R/W operations: FAILED (errors %d)\n", status);
	} else {
		printf("INFO: R/W operations: DONE\n");
	}

	/* Activate the buffer of the given id */
	ioctl_arg.buffer_id = buffer_id;
	status = ioctl(fd, MEMALLOC_RELEASE_CMD, &ioctl_arg);
	if (status < 0)
	{
		printf("ERROR: memalloc->RELEASE : FAILED (status %d)\n", status);
		return(-1);
	}
	printf("INFO: memalloc->RELEASE : DONE (status %d)\n", buffer_id);

	status = close(fd);
	if (status < 0)
	{
		printf("ERROR: Close /dev/memalloc: FAILED (status %d)\n", status);
		return(-1);
	}
	printf("INFO: Close /dev/memalloc: DONE (status %d)\n", status);

	return(0);
}
