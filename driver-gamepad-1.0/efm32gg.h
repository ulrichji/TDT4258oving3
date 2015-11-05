

// GPIO

#define GPIO_PA_BASE 0x40006000
#define GPIO_PB_BASE 0x40006024
#define GPIO_PC_BASE 0x40006048

#define GPIO_PA_CTRL     ((volatile int*)(GPIO_PA_BASE + 0x00))
#define GPIO_PA_MODEL    ((volatile int*)(GPIO_PA_BASE + 0x04))
#define GPIO_PA_MODEH    ((volatile int*)(GPIO_PA_BASE + 0x08))
#define GPIO_PA_DOUT     ((volatile int*)(GPIO_PA_BASE + 0x0c))
#define GPIO_PA_DOUTSET  ((volatile int*)(GPIO_PA_BASE + 0x10))
#define GPIO_PA_DOUTCLR  ((volatile int*)(GPIO_PA_BASE + 0x14))
#define GPIO_PA_DOUTTGL  ((volatile int*)(GPIO_PA_BASE + 0x18))
#define GPIO_PA_DIN      ((volatile int*)(GPIO_PA_BASE + 0x1c))
#define GPIO_PA_PINLOCKN ((volatile int*)(GPIO_PA_BASE + 0x20))

#define GPIO_PB_CTRL     ((volatile int*)(GPIO_PB_BASE + 0x00))
#define GPIO_PB_MODEL    ((volatile int*)(GPIO_PB_BASE + 0x04))
#define GPIO_PB_MODEH    ((volatile int*)(GPIO_PB_BASE + 0x08))
#define GPIO_PB_DOUT     ((volatile int*)(GPIO_PB_BASE + 0x0c))
#define GPIO_PB_DOUTSET  ((volatile int*)(GPIO_PB_BASE + 0x10))
#define GPIO_PB_DOUTCLR  ((volatile int*)(GPIO_PB_BASE + 0x14))
#define GPIO_PB_DOUTTGL  ((volatile int*)(GPIO_PB_BASE + 0x18))
#define GPIO_PB_DIN      ((volatile int*)(GPIO_PB_BASE + 0x1c))
#define GPIO_PB_PINLOCKN ((volatile int*)(GPIO_PB_BASE + 0x20))

#define GPIO_PC_CTRL     ((volatile int*)(GPIO_PC_BASE + 0x00))
#define GPIO_PC_MODEL    ((volatile int*)(GPIO_PC_BASE + 0x04))
#define GPIO_PC_MODEH    ((volatile int*)(GPIO_PC_BASE + 0x08))
#define GPIO_PC_DOUT     ((volatile int*)(GPIO_PC_BASE + 0x0c))
#define GPIO_PC_DOUTSET  ((volatile int*)(GPIO_PC_BASE + 0x10))
#define GPIO_PC_DOUTCLR  ((volatile int*)(GPIO_PC_BASE + 0x14))
#define GPIO_PC_DOUTTGL  ((volatile int*)(GPIO_PC_BASE + 0x18))
#define GPIO_PC_DIN      ((volatile int*)(GPIO_PC_BASE + 0x1c))
#define GPIO_PC_PINLOCKN ((volatile int*)(GPIO_PC_BASE + 0x20))

#define GPIO_EXTIPSELL ((volatile int*)(GPIO_PA_BASE + 0x100))
#define GPIO_EXTIPSELH ((volatile int*)(GPIO_PA_BASE + 0x104))
#define GPIO_EXTIRISE  ((volatile int*)(GPIO_PA_BASE + 0x108))
#define GPIO_EXTIFALL  ((volatile int*)(GPIO_PA_BASE + 0x10c))
#define GPIO_IEN       ((volatile int*)(GPIO_PA_BASE + 0x110))
#define GPIO_IFC       ((volatile int*)(GPIO_PA_BASE + 0x11c))
#define GPIO_IF        ((volatile int*)(GPIO_PA_BASE + 0x114))

// CMU

#define CMU_BASE2 0x400c8000

#define CMU_HFPERCLKDIV  ((volatile int*)(CMU_BASE2 + 0x008))
#define CMU_HFCORECLKEN0 ((volatile int*)(CMU_BASE2 + 0x040))
#define CMU_HFPERCLKEN0  ((volatile int*)(CMU_BASE2 + 0x044))
#define CMU_CMD          ((volatile int*)(CMU_BASE2 + 0x024))

#define CMU2_HFPERCLKEN0_DAC0   (1 << 17)
#define CMU2_HFPERCLKEN0_PRS    (1 << 15)
#define CMU2_HFPERCLKEN0_GPIO   (1 << 13)
#define CMU2_HFPERCLKEN0_TIMER1 (1 << 6)

#define CMU_HFCORECLKEN0_DMA (1 << 0)

// TIMER1

#define TIMER1_BASE 0x40010400

#define TIMER1_CMD ((volatile int*)(TIMER1_BASE + 0x04))
#define TIMER1_IEN ((volatile int*)(TIMER1_BASE + 0x0c))
#define TIMER1_IFC ((volatile int*)(TIMER1_BASE + 0x18))
#define TIMER1_TOP ((volatile int*)(TIMER1_BASE + 0x1c))
#define TIMER1_CNT ((volatile int*)(TIMER1_BASE + 0x24))

// NVIC

#define ISER0 ((volatile int*)0xe000e100)
#define ISER1 ((volatile int*)0xe000e104)
#define ICER0 ((volatile int*)0xe000e180)
#define ICER1 ((volatile int*)0xe000e184)
#define ISPR0 ((volatile int*)0xe000e200)
#define ISPR1 ((volatile int*)0xe000e204)
#define ICPR0 ((volatile int*)0xe000e280)
#define ICPR1 ((volatile int*)0xe000e284)
#define IABR0 ((volatile int*)0xe000e300)
#define IABR1 ((volatile int*)0xe000e304)

// IPR

#define IPR_BASE  0xe000e400

#define IPR0 ((volatile int*)(IPR_BASE + 0x00))
#define IPR1 ((volatile int*)(IPR_BASE + 0x04))
#define IPR2 ((volatile int*)(IPR_BASE + 0x08))
#define IPR3 ((volatile int*)(IPR_BASE + 0x0c))

// EMU

#define EMU_BASE2 0x400c6000

#define EMU_CTRL ((volatile int*)(EMU_BASE2 + 0x000))

// DAC0

#define DAC0_BASE2 0x40004000

#define DAC0_CTRL     ((volatile int*)(DAC0_BASE2 + 0x000))
#define DAC0_CH0CTRL  ((volatile int*)(DAC0_BASE2 + 0x008))
#define DAC0_CH1CTRL  ((volatile int*)(DAC0_BASE2 + 0x00c))
#define DAC0_IEN      ((volatile int*)(DAC0_BASE2 + 0x010))
#define DAC0_IF       ((volatile int*)(DAC0_BASE2 + 0x014))
#define DAC0_IFS      ((volatile int*)(DAC0_BASE2 + 0x018))
#define DAC0_IFC      ((volatile int*)(DAC0_BASE2 + 0x01c))
#define DAC0_CH0DATA  ((volatile int*)(DAC0_BASE2 + 0x020))
#define DAC0_CH1DATA  ((volatile int*)(DAC0_BASE2 + 0x024))
#define DAC0_COMBDATA ((volatile int*)(DAC0_BASE2 + 0x028))

// DMA

#define DMA_BASE 0x400c2000

#define DMA_STATUS      ((volatile int*)(DMA_BASE + 0x0000))
#define DMA_CONFIG      ((volatile int*)(DMA_BASE + 0x0004))
#define DMA_CTRLBASE    ((volatile int*)(DMA_BASE + 0x0008))
#define DMA_ALTCTRLBASE ((volatile int*)(DMA_BASE + 0x000c))
#define DMA_CHUSEBURSTS ((volatile int*)(DMA_BASE + 0x0018))
#define DMA_CHUSEBURSTC ((volatile int*)(DMA_BASE + 0x001c))
#define DMA_REQMASKC    ((volatile int*)(DMA_BASE + 0x0024))
#define DMA_CHENS       ((volatile int*)(DMA_BASE + 0x0028))
#define DMA_CHALTC      ((volatile int*)(DMA_BASE + 0x0034))
#define DMA_IFC         ((volatile int*)(DMA_BASE + 0x1008))
#define DMA_IEN         ((volatile int*)(DMA_BASE + 0x100c))
#define DMA_CH0_CTRL    ((volatile int*)(DMA_BASE + 0x1100))

// PRS

#define PRS_BASE 0x400cc000

#define PRS_CH0_CTRL ((volatile int*)(PRS_BASE + 0x010))

// System Control Block

#define SCR          ((volatile int*)0xe000ed10)
#define SYSTICK_CTRL ((volatile int*)0xe000e010)
#define SYSTICK_LOAD ((volatile int*)0xe000e014)

