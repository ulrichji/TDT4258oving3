#ifndef GLOBALS_H
#define GLOBALS_H

// Offset from GPIO base address to different registers
#define GPIO_PC_BASE	0x48
#define GPIO_EXTIPSELL	0x100
#define GPIO_EXTIRISE	0x108
#define GPIO_EXTIFALL	0x10c
#define GPIO_IEN	0x110
#define GPIO_IFC	0x11c
#define GPIO_IF		0x114

// Offset from C port base address to different C port register addresses
#define GPIO_PC_MODEL	(GPIO_PC_BASE + 0x04)
#define GPIO_PC_DOUT	(GPIO_PC_BASE + 0x0c)
#define GPIO_PC_DIN	(GPIO_PC_BASE + 0x1c)

// Platform device data
#define GPIO_EVEN_IRQ_INDEX	0
#define GPIO_ODD_IRQ_INDEX	1
#define GPIO_MEM_INDEX		0

#endif
