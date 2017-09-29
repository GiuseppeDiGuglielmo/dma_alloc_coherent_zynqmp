#ifndef MEMALLOC_H
#define MEMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include <asm/ioctl.h>

#define DEVICE_NAME "memalloc"

#define MEMALLOC_ERR -1

#define MEMALLOC_BUFFER_MAX_NUMBER 16

#define MEMALLOC_IOCTL_BASE 1
#define MEMALLOC_RESERVE_CMD         _IO(MEMALLOC_IOCTL_BASE, 0) 
#define MEMALLOC_RELEASE_CMD         _IO(MEMALLOC_IOCTL_BASE, 1)
#define MEMALLOC_GET_PHYSICAL_CMD    _IO(MEMALLOC_IOCTL_BASE, 2)
#define MEMALLOC_ACTIVATE_BUFFER_CMD _IO(MEMALLOC_IOCTL_BASE, 3)

typedef struct ioctl_arg_t
{
	size_t buffer_size;      /* in */
	int buffer_id;           /* in, out */
	unsigned long phys_addr; /* out */
} ioctl_arg_t;

#ifdef __cplusplus
}
#endif
#endif /* MEMALLOC_H */
