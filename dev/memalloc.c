/*
* DMA memory allocation.  This kernel module allocates coherent, non-cached
* memory and returns the physical and virtual address of the allocated buffer.
*/

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>

#include "memalloc.h"

/* Buffer information */
struct buffer_info_t {
	int active;
	int size;
	dma_addr_t handle;
	int *kernel_address;
};
static struct buffer_info_t buffer_info[MEMALLOC_BUFFER_MAX_NUMBER];

/* Which buffer is currently active - for mmap */
static int active_buffer_id;

struct memalloc_if_t {
	struct device *device_p;
	dev_t dev_node;
	struct cdev cdev;
	struct class *class_p;
};
static struct memalloc_if_t interface;

static long memalloc_ioctl (struct file *, unsigned int, unsigned long);
static int memalloc_mmap (struct file *, struct vm_area_struct *);
static int memalloc_release (struct inode *, struct file *);
static int memalloc_open(struct inode *, struct file *);

static struct file_operations fops = {
	.unlocked_ioctl = memalloc_ioctl,
	.mmap = memalloc_mmap,
	.release = memalloc_release,
	.open = memalloc_open
};

static int reserve_buffer(ioctl_arg_t *);
static int release_buffer(ioctl_arg_t *ioctl_arg);
static int get_physical_address (ioctl_arg_t *ioctl_arg);
static void cleanup(void);

static long memalloc_ioctl (struct file *fd, unsigned int cmd, unsigned long arg)
{
	struct ioctl_arg_t ioctl_arg;
	long status;

	status = copy_from_user(&ioctl_arg, (void __user *)arg, sizeof(ioctl_arg_t));	
	if (status != 0)
	{
		printk(KERN_ERR "ERROR: copy_from_user failed (%ld bytes).\n", status);
		status = -1;
		return(status);
	}

	switch(cmd)
	{
		case MEMALLOC_RESERVE_CMD:
			printk(KERN_ERR "DEBUG: Module fops->ioctl: RESERVE (%zu bytes).\n", ioctl_arg.buffer_size);
			status = reserve_buffer(&ioctl_arg);
			if (status != 0)
			{
				status = -1;
				return(status);
			}

			status = copy_to_user((ioctl_arg_t*)arg, &ioctl_arg, sizeof(ioctl_arg_t));
			if (status != 0)
			{
				printk(KERN_ERR "ERROR: copy_to_user failed (%ld bytes).\n", status);
				status = -1;
				return(status);
			}
			break;
		case MEMALLOC_RELEASE_CMD:
			printk(KERN_ERR "DEBUG: Module fops->ioctl: RELEASE.\n");
			status = release_buffer(&ioctl_arg);
			break;
		case MEMALLOC_GET_PHYSICAL_CMD:
			printk(KERN_ERR "DEBUG: Module fops->ioctl: GET_PHYSICAL (id %d).\n", ioctl_arg.buffer_id);
			status = get_physical_address(&ioctl_arg);

			status = copy_to_user((ioctl_arg_t*)arg, &ioctl_arg, sizeof(ioctl_arg_t));
			if (status != 0)
			{
				printk(KERN_ERR "ERROR: copy_to_user failed (%ld bytes).\n", status);
				status = -1;
				return(status);
			}
			break;
		case MEMALLOC_ACTIVATE_BUFFER_CMD:
			printk(KERN_ERR "DEBUG: Module fops->ioctl: ACTIVATE_BUFFER (id %d).\n", ioctl_arg.buffer_id);
			if (ioctl_arg.buffer_id > MEMALLOC_BUFFER_MAX_NUMBER || buffer_info[ioctl_arg.buffer_id].active == 0)
			{
				printk(KERN_ERR "ERROR: Wrong bufferID %lu.\n", arg);
				status = -1;
				return(status);
			}
			active_buffer_id = ioctl_arg.buffer_id;
			status = 0;
			break;
		default:
			printk(KERN_ERR "ERROR: Wrong command: %d.\n", cmd);
			status = -1;
			return(status);
			break;
	}
	return(status);
}

static int memalloc_mmap (struct file *fd, struct vm_area_struct *vma)
{
        printk(KERN_ERR "DEBUG: Module fops->mmap.\n");

	//return dma_common_mmap(interface.device_p, vma, buffer_info[active_buffer_id].kernel_address, buffer_info[active_buffer_id].handle, vma->vm_end-vma->vm_start);
	return dma_mmap_coherent(NULL, vma, buffer_info[active_buffer_id].kernel_address, buffer_info[active_buffer_id].handle, vma->vm_end-vma->vm_start);
}

static int memalloc_release(struct inode *in, struct file *fd)
{
        printk(KERN_ERR "DEBUG: Module fops->release.\n");

	cleanup();
	return(0);
}

static int memalloc_open(struct inode *ino, struct file *file)
{
        printk(KERN_ERR "DEBUG: Module fops->open.\n");

	file->private_data = container_of(ino->i_cdev, struct memalloc_if_t, cdev);

	return(0);
}



static int reserve_buffer(ioctl_arg_t *ioctl_arg)
{
	int id;
	size_t size;
	void *vaddr;
	dma_addr_t paddr;

	size = ioctl_arg->buffer_size;

	for (id = 0; id < MEMALLOC_BUFFER_MAX_NUMBER; id++)
	{
		if (buffer_info[id].active == 0) /* First available buffer (non active). */
		{
			printk(KERN_ERR "DEBUG: Reserving buffer %d.\n", id);
			buffer_info[id].active = 1;
			break;
		}
	}
	
	if (id < MEMALLOC_BUFFER_MAX_NUMBER)
	{
#if 0
		long long unsigned dma_mask = dma_get_required_mask(interface.device_p);

		status = dma_set_mask(interface.device_p, dma_mask);

		if (status != 0)
		{
			printk(KERN_ERR "ERROR: Set dma mask failure (vaddr %llx).\n", dma_mask);
			return(-1);
		}

		vaddr = dma_alloc_coherent(interface.device_p, size, &paddr, GFP_KERNEL);
#else
		vaddr = dma_alloc_coherent(NULL, size, &paddr, GFP_KERNEL);

#endif
		if (vaddr == NULL)
		{
			printk(KERN_ERR "ERROR: Allocation failure (vaddr %p).\n", vaddr);
			return(-1);
		}
		//printk(KERN_ERR "DEBUG: Allocated buffer %d (paddr = 0x%p, k-vaddr = 0x%x).\n", id, paddr, vaddr);

		buffer_info[id].kernel_address = vaddr;
		buffer_info[id].handle = paddr;
		buffer_info[id].size = (int)size;

		ioctl_arg->buffer_id = id;
		return(0);
	}
	else
	{
		printk(KERN_ERR "ERROR: No buffer available.\n");
		return(-1);
	}

	return(0);
}

static int release_buffer(ioctl_arg_t *ioctl_arg)
{
	int id = ioctl_arg->buffer_id;

	if (id > MEMALLOC_BUFFER_MAX_NUMBER)
	{
		printk(KERN_ERR "ERROR: Wrong bufferID %d.\n", id);
		return(-1);
	}
	printk(KERN_ERR "DEBUG: Releasing buffer %d.\n", id);
	buffer_info[id].active = 0;
	dma_free_coherent(NULL, buffer_info[id].size, buffer_info[id].kernel_address, buffer_info[id].handle);
	return(0);
}

static int get_physical_address (ioctl_arg_t *ioctl_arg)
{
	int id = ioctl_arg->buffer_id;

	if (id > MEMALLOC_BUFFER_MAX_NUMBER || buffer_info[id].active == 0)
	{
		printk(KERN_ERR "ERROR: Wrong bufferID %d.\n", id);
		return(-1);
	}

	ioctl_arg->phys_addr = buffer_info[id].handle;

	return(0);
}

static void cleanup(void)
{
	int i;
	for (i = 0; i < MEMALLOC_BUFFER_MAX_NUMBER; i++)
	{
		if (buffer_info[i].active != 0)
		{
			dma_free_coherent(NULL, buffer_info[i].size, buffer_info[i].kernel_address, buffer_info[i].handle);
			buffer_info[i].active = 0;
		}
	}
}

static int __init memalloc_init(void)
{
	int rc;
	int i;
	static struct class *local_class_p = NULL;
	
	printk(KERN_ERR "DEBUG: Module init.\n");
	
	/* Allocate the character device from the kernel for this driver. */
	rc = alloc_chrdev_region(&interface.dev_node, 0, 1, DEVICE_NAME);
	if (rc) 
	{
		printk(KERN_ERR "ERROR: Allocate the character device: FAILED.\n");
		return(rc);
	}
	printk(KERN_ERR "DEBUG: Allocate the character device: DONE.\n");

	/* Initialize the character-device data structure before registering the character device with the kernel. */
	cdev_init(&interface.cdev, &fops);
	rc = cdev_add(&interface.cdev, interface.dev_node, 1);
	if (rc)
	{
		printk(KERN_ERR "ERROR: Initialize and add the character device: FAILED.\n");
		cdev_del(&interface.cdev);
		return(rc);
	}
	printk(KERN_ERR "DEBUG: Initialize and add the character device: DONE.\n");

	/* Create the device in sysfs which will allow the device node in /dev to be created. */
	local_class_p = class_create(THIS_MODULE, DEVICE_NAME);
	interface.class_p = local_class_p;
	
	/* Create the device node in /dev so the device is accessible as a character device. */
	interface.device_p = device_create(interface.class_p, NULL, interface.dev_node, NULL, DEVICE_NAME);

	if (IS_ERR(interface.device_p)) 
	{
		printk(KERN_ERR "ERROR: Create the device node /dev/%s: FAILED\n", DEVICE_NAME);
		class_destroy(interface.class_p);
		cdev_del(&interface.cdev);
		return(rc);
	}
	printk(KERN_ERR "DEBUG: Create the device node /dev/%s: DONE\n", DEVICE_NAME);

	for (i = 0; i < MEMALLOC_BUFFER_MAX_NUMBER; i++)
	{
		buffer_info[active_buffer_id].active = 0;
	}

	/* */
	/* dma_set_mask(interface.device_p, DMA_BIT_MASK(32)); */
	
	return(0);
}

static void __exit memalloc_exit(void)
{
	printk(KERN_ERR "DEBUG: Module exit.\n");

	cleanup();

	cdev_del(&interface.cdev);

	device_destroy(interface.class_p, interface.dev_node);

	class_destroy(interface.class_p);

	unregister_chrdev_region(interface.dev_node, 1);
}

module_init(memalloc_init);
module_exit(memalloc_exit);

MODULE_AUTHOR("Giuseppe Di Guglielmo");
MODULE_DESCRIPTION("Create a buffer and return physical and virtual address, for DMA userspace driver. Thanks to Massimiliano Giacometti.");
MODULE_LICENSE("GPL");
