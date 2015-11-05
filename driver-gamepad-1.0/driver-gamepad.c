/*
 * This is a demo Linux kernel module.
 */

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

struct cdev my_cdev;
dev_t devnum;
struct fasync_struct *gamepad_queue;
struct class *cl;
char* gamepad_buffer;

static int gamepad_fasync(int fd, struct file *filp, int mode)
{
	printk("fasync occured\n");
	int fasync_result = fasync_helper(fd, filp, mode, &gamepad_queue);
	if(fasync_result < 0) {
		printk("Failed to add/remove fasync entry\n");
	} else {
		printk("Succeded to add/remove fasync entry\n");
	}
	return fasync_result;
}

static ssize_t gamepad_write(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	printk("Write occured\n");
	return 0;
}

static ssize_t gamepad_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	printk("Read occureddd\n");
	unsigned btn_id = (~(*GPIO_PC_DIN) & 0xFF);
	printk("Read %u", btn_id);
	copy_to_user(buf, &btn_id, sizeof(int));
	return sizeof(int);
}

static ssize_t gamepad_open(struct inode *inode, struct file *filp) {
	printk("Open occured\n");
	return 0;
}

static ssize_t gamepad_release(struct inode *inode, struct file *filp) {
	printk("Release occured\n");
	return 0;
}

struct file_operations gamepad_fops = {
	.owner = THIS_MODULE,
	.open = gamepad_open,
	.release = gamepad_release,
	.write = gamepad_write,
	.read = gamepad_read,
	.fasync = gamepad_fasync,
};

irqreturn_t GPIO_handler(int irq, void* dev_id, struct pt_regs *regs) {
	printk("Registered GPIO interrupt\n");
	if (gamepad_queue) {
		printk("Sent signal\n");
		kill_fasync(&gamepad_queue, SIGIO, POLL_IN);
	}
	*GPIO_IFC = *GPIO_IF;
	return IRQ_HANDLED;
}

static int __init template_init(void)
{
	//Initialize cdev structure wih our file_operations
	cdev_init(&my_cdev, &gamepad_fops);
	my_cdev.owner = THIS_MODULE;

	// Allocate a device number to our driver
	int alloc_result = alloc_chrdev_region(&devnum, 0, 1, "gamepad");
	if(alloc_result != 0) {
		printk("Failed to allocate device number for gamepad!\n");
		return -1;
	}
	printk("Allocated major device number %u to gamepad\n", MAJOR(devnum));

	// Allocate memory for buffer
	gamepad_buffer = kmalloc(1, GFP_KERNEL);
	if(!gamepad_buffer) {
		printk("Failed to allocate memory for driver buffer\n");
	} else {
		printk("Allocated memory for driver buffer\n");
	}

	// Register device in /dev folder
	cl = class_create(THIS_MODULE, "driver_gamepad");
	device_create(cl, NULL, devnum, NULL, "driver_gamepad");

	// Setup registers to enable GPIO in hw
	*CMU_HFPERCLKEN0 |= CMU2_HFPERCLKEN0_GPIO; //enable GPIO clock
	*GPIO_PC_MODEL = 0x33333333; //Set pins C0 - C7 as input
	*GPIO_PC_DOUT = 0xFF; //Set pins to active low?
	*GPIO_EXTIPSELL = 0x22222222; //Set all pins to trigger interrupt
	*GPIO_EXTIFALL = 0xFF; //Set pins to trigger interrupt on falling edge	
	*GPIO_IEN = 0xFF; //Enable interrupt
	*ISER0 |= 0x802; //enable interrupt generation for gpio even and odd.	

	// Register handler for irq numbers 17 and 18 (odd and even GPIO)
	int result = request_irq(17, GPIO_handler, 0, "driver_gamepad", NULL);
	if(result) {
		printk("Failed to allocate GPIO odd IRQ\n");
	} else {
		printk("Allocated GPIO odd IRQ\n");
	}
	result = request_irq(18, GPIO_handler, 0, "driver_gamepad", NULL);
	if(result) {
		printk("Failed to allocate GPIO even IRQ\n");
	} else {
		printk("Allocated GPIO even IRQ\n");
	}

	// Inform the system of our device
	int cdev_result = cdev_add(&my_cdev, devnum, 1);
	if(cdev_result < 0) {
		printk("Failed to allocate cdev\n");
	} else {
		printk("Allocated cdev\n");
	}

	printk("Finished initialization");

	return 0;
}

static void __exit template_cleanup(void)
{
	 printk("Short life for a small module...\n");
}

module_init(template_init);
module_exit(template_cleanup);

MODULE_DESCRIPTION("Driver for a small, custom-made gamepad connected through GPIO pins");
MODULE_LICENSE("GPL");
