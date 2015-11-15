#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>

#include <linux/fcntl.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include <asm/io.h>

#include "globals.h"

struct cdev my_cdev;			// Character device handle
dev_t devnum;				// Device number for our device
struct fasync_struct *gamepad_queue;	// Queue of signal listeners
struct class *cl;			// Class handle

volatile void *gpio_mem;		// Pointer to base address of GPIO registers 

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

irqreturn_t GPIO_handler(int irq, void* dev_id, struct pt_regs *regs) {
	if (gamepad_queue) {
		kill_fasync(&gamepad_queue, SIGIO, POLL_IN); // Signal listeners in queue
	}
	iowrite32(ioread32(gpio_mem + GPIO_IF), gpio_mem + GPIO_IFC); // Set interrupt as handled in hardware

	return IRQ_HANDLED;
}


static int my_probe(struct platform_device *dev) {
	//Get GPIO information from platform device
	struct resource *res = platform_get_resource(dev, IORESOURCE_MEM, GPIO_MEM_INDEX);
	gpio_mem = res->start;
	if(res == NULL) {
		printk("Failed to get GPIO information from platform\n");
		return -1;
	}
	
	// Request exclusive access to GPIO registers during operation
	struct resource *mem_req = request_mem_region(res->start, res->end - res->start, "driver-gamepad");
	if(mem_req == NULL) {
		printk(KERN_ERR "Failed to request GPIO memory region\n");
		return -1;
	}

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
	iowrite32(0x33333333,	gpio_mem+GPIO_PC_MODEL);	//Set pins C0 - C7 as input
	iowrite32(0xFF,		gpio_mem+GPIO_PC_DOUT);		//Set pins to active low?
	iowrite32(0x22222222,	gpio_mem+GPIO_EXTIPSELL);	//Set all pins to trigger interrupt
	iowrite32(0xFF,		gpio_mem+GPIO_EXTIFALL);	//Set pins to trigger interrupt on falling edge
	iowrite32(0xFF,		gpio_mem+GPIO_EXTIRISE);	//Set pins to trigger interrupt on rising edge
	iowrite32(0xFF,		gpio_mem+GPIO_IEN);		//Set pins to trigger interrupt on rising edge
	
	// Register handler for irq numbers 17 and 18 (odd and even GPIO)
	int irq = platform_get_irq(dev, GPIO_ODD_IRQ_INDEX);
	if(irq == -ENXIO) {
		printk(KERN_ERR"Failed to retrieve GPIO odd irq index from platform\n");
		return -1;
	}
	int result = request_irq(irq, GPIO_handler, 0, "driver_gamepad", NULL);
	if(result) {
		printk(KERN_ERR"Failed to allocate GPIO odd handler %d\n", result);
		return -1;
	}

	irq = platform_get_irq(dev, GPIO_EVEN_IRQ_INDEX);
	if(irq == -ENXIO) {
		printk(KERN_ERR"Failed to retrieve GPIO even irq index from platform\n");
		return -1;
	}
	result = request_irq(irq, GPIO_handler, 0, "driver_gamepad", NULL);
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

static int my_remove(struct platform_device *dev) {
	printk("ran my_remove\n");
}

static const struct of_device_id my_of_match[] = {
	{.compatible = "tdt4258"},
	{},
};


MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_driver = {
	.probe	=	my_probe,
	.remove	=	my_remove,
	.driver	=	{
				.name = "gamepad-driver",
				.owner = THIS_MODULE,
				.of_match_table = my_of_match,
			},
};

static int __init template_init(void)
{
	platform_driver_register(&my_driver);

	return 0;
}

static void __exit template_cleanup(void)
{
	device_destroy(cl, devnum);		// Remove the device file
	class_destroy(cl);			// Remove the class handle

	cdev_del(&my_cdev);			// Remove device from system
	unregister_chrdev_region(devnum, 1);	// Release device number
}

module_init(template_init);
module_exit(template_cleanup);

MODULE_DESCRIPTION("Driver for a small, custom-made gamepad connected through GPIO pins on an EFM32GG");
MODULE_LICENSE("GPL");
