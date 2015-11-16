#include "file_operations.h"

static int gamepad_fasync(int fd, struct file *filp, int mode)
{
	int fasync_result = fasync_helper(fd, filp, mode, &gamepad_queue); // Register signal listener
	if(fasync_result < 0) {
		printk(KERN_ERR "Failed to add/remove fasync entry\n");
	}

	return fasync_result;
}

static ssize_t gamepad_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	unsigned btn_id = ~(ioread32(gpio_mem+GPIO_PC_DIN)); // Use active high instead of active low
	copy_to_user(buf, &btn_id, sizeof(int)); // Copy input to user space

	return sizeof(int); // Return number of bytes copied
}

struct file_operations gamepad_fops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.release = NULL,
	.read = gamepad_read,
	.fasync = gamepad_fasync,
};
