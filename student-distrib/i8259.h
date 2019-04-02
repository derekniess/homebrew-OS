/**********************************************************************/
/* i8259.h - Functions to interact with the 8259 interrupt controller */
/**********************************************************************/
#ifndef I8259_H
#define I8259_H



#include "types.h"



/* Ports that each PIC sits on */
#define MASTER_8259_PORT 0x20
#define SLAVE_8259_PORT  0xA0

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning of each word */
#define ICW1          0x11
#define ICW2_MASTER   0x20
#define ICW2_SLAVE    0x28
#define ICW3_MASTER   0x04
#define ICW3_SLAVE    0x02
#define ICW4_MASTER   0x05
#define ICW4_SLAVE    0x01

/* End-of-interrupt byte. This gets OR'd with the interrupt number and sent out
 * to the PIC to declare the interrupt finished */
#define EOI           0x60

/* IRQ Constant */
#define SLAVE_IRQ		2



/* Initialize both PICs */
void i8259_init(void);

/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num);

/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num);

/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);



#endif /* I8259_H */
