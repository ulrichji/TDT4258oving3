#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "efm32gg.h"

struct cdev my_cdev; // Character device handle
dev_t devnum; // Device number for our device
struct fasync_struct *gamepad_queue; // Queue of signal listeners
struct class *cl; // Class handle

static int gamepad_fasync(int fd, struct file *filp, int mode)
{
	int fasync_result = fasync_helper(fd, filp, mode, &gamepad_queue); // Register signal listener
	if(fasync_result < 0) {
		printk(KERN_ERR "Failed to add/remove fasync entry\n");
	}

	return fasync_result;
}

static ssize_t gamepad_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	unsigned btn_id = ~(*GPIO_PC_DIN); // Use active high instead of active low
	btn_id &= 0xFF; // Clear the unused bits
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

irqreturn_t GPIO_handler(int irq, void* dev_id, struct pt_regs *regs) {
	if (gamepad_queue) {
		kill_fasync(&gamepad_queue, SIGIO, POLL_IN); // Signal listeners in queue
	}
	*GPIO_IFC = *GPIO_IF; // Set interrupt as handled in hardware
	return IRQ_HANDLED;
}

static int __init template_init(void)
{
	cdev_init(&my_cdev, &gamepad_fops); // Initialize character device with our file operations
	my_cdev.owner = THIS_MODULE; // Set character device module pointer to this module

	// Allocate a device number to our driver
	int alloc_result = alloc_chrdev_region(&devnum, 0, 1, "gamepad"); // Allocate one device number for this driver
	if(alloc_result != 0) {
		printk(KERN_ERR "Failed to allocate device number for gamepad!\n");
		return -1;
	}

	// Register device in /dev folder
	cl = class_create(THIS_MODULE, "driver_gamepad"); // Create module class and class handle
	device_create(cl, NULL, devnum, NULL, "driver_gamepad"); // Create device file for our module

	// Setup registers to enable GPIO in hardware
	*CMU_HFPERCLKEN0 |= CMU2_HFPERCLKEN0_GPIO; //enable GPIO clock
	*GPIO_PC_MODEL = 0x33333333; //Set pins C0 - C7 as input
	*GPIO_PC_DOUT = 0xFF; //Set pins to active low?
	*GPIO_EXTIPSELL = 0x22222222; //Set all pins to trigger interrupt
	*GPIO_EXTIFALL = 0xFF; //Set pins to trigger interrupt on falling edge
	*GPIO_EXTIRISE = 0xFF; //Set pins to trigger interrupt on rising edge	
	*GPIO_IEN = 0xFF; //Enable interrupt
	*ISER0 |= 0x802; //enable interrupt generation for gpio even and odd.	

	// Register handler for irq numbers 17 and 18 (odd and even GPIO)
	int result = request_irq(17, GPIO_handler, 0, "driver_gamepad", NULL); // Register GPIO odd handler
	if(result) {
		printk(KERN_ERR"Failed to allocate GPIO odd handler\n");
		return -1;
	}
	result = request_irq(18, GPIO_handler, 0, "driver_gamepad", NULL); // Register GPIO even handler
	if(result) {
		printk(KERN_ERR "Failed to allocate GPIO even handler\n");
		return -1;
	}

	int cdev_result = cdev_add(&my_cdev, devnum, 1); // Inform the system of our device
	if(cdev_result < 0) {
		printk(KERN_ERR "Failed to add cdev\n");
		return -1;
	}

	return 0;
}

static void __exit template_cleanup(void)
{
	device_destroy(cl, devnum); // Remove the device file
	class_destroy(cl); // Remove the class handle

	cdev_del(&my_cdev); // Remove device from system
	unregister_chrdev_region(devnum, 1); // Release device number
	printk("Stopped driver\n");

	return 0;
}

module_init(template_init);
module_exit(template_cleanup);

MODULE_DESCRIPTION("Driver for a small, custom-made gamepad connected through GPIO pins on an EFM32GG");
MODULE_LICENSE("GPL");
