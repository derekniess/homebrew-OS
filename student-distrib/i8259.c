/**********************************************************************/
/* i8259.c - Functions to interact with the 8259 interrupt controller */
/**********************************************************************/
#include "i8259.h"
#include "lib.h"


/*** Interrupt masks to determine which interrupts are enabled and disabled ***/

/* IRQs 0-7 */
uint8_t master_mask = 0xFF; 

/* IRQs 8-15 */
uint8_t slave_mask = 0xFF;



/*
 * i8259_init()
 *
 * Description:
 * Initializes the 8259 PIC by sending the correct bytes to the controller as
 * specified its datasheet.
 *
 * Inputs: none
 * 
 * Retvals: none
 */
void i8259_init(void)
{
	/*** Send the 4 ICWs to both the slave and master: ***/

	/** ICW1 **/
		/* Master port */
		outb( ICW1, MASTER_8259_PORT );
		/* Slave port */
		outb( ICW1, SLAVE_8259_PORT );
	
	/** ICW2 **/
		/* Master port */
		outb( ICW2_MASTER, MASTER_8259_PORT + 1 );
		/* Slave port */
		outb( ICW2_SLAVE , SLAVE_8259_PORT  + 1 );
	
	/** ICW3 **/
		/* Master port */
		outb( ICW3_MASTER, MASTER_8259_PORT + 1 );
		/* Slave port */
		outb( ICW3_SLAVE , SLAVE_8259_PORT  + 1 );
	
	/** ICW4 **/
		/* Master port */
		outb( ICW4_MASTER, MASTER_8259_PORT + 1 );
		/* Slave port */
		outb( ICW4_SLAVE , SLAVE_8259_PORT  + 1 );

	/*** Enable Slave PIC ***/

	enable_irq(SLAVE_IRQ);
}

/*
 * enable_irq()
 *
 * Description:
 * Enables (e.g. unmasks) the specified IRQ with an ACTIVE LOW bitmask. This 
 * function writes an 8-bit value the mask (master or slave) corresponding to
 * the integer input: irq_num.
 *
 * Inputs: none
 *
 * Retvals: none
 */
void enable_irq(uint32_t irq_num)
{

	/* Return if irq_num is invalid */
	if ((irq_num > 15) || (irq_num < 0)) {
		return;
	}

	/* initially mask = 11111110 */
	uint8_t mask = 0xFE;

	/* Here we shift the '0' in "mask" to correspond to the proper irq. */

	/**** If irq_num is in master bounds: ****/
	if ((irq_num >= 0) && (irq_num <= 7)) {
		/* left circular shift by irq_num */
		int b;
		for (b = 0; b < irq_num; b++) {
			mask = (mask << 1) + 1;
		}

		master_mask = master_mask & mask;
		outb( master_mask, MASTER_8259_PORT + 1 );
		return;
	}

	/**** If irq_num is in slave bounds: ****/
	if ((irq_num >= 8) && (irq_num <= 15)) {
		irq_num -= 8; /* Get irq_num into 0-7 range */
		/* left circular shift by irq_num */
		int b;
		for (b = 0; b < irq_num; b++) {
			mask = (mask << 1) + 1;
		}

		slave_mask = slave_mask & mask;
		outb( slave_mask, SLAVE_8259_PORT + 1 );
		return;
	}
}

/*
 * disable_irq()
 *
 * Description:
 * Disable (e.g. masks) the specified IRQ with an INACTIVE HIGH bitmask. This 
 * function writes an 8-bit value the mask (master or slave) corresponding to
 * the integer input: irq_num.
 *
 * Inputs: none
 *
 * Retvals: none
 */
void disable_irq(uint32_t irq_num)
{
	/* Return if irq_num is invalid */
	if ((irq_num > 15) || (irq_num < 0)) {
		return;
	}

	/* initially mask = 00000001 */
	uint8_t mask = 0x01;

	/* Here we shift the '1' in "mask" to correspond to the proper irq. */

	/**** If irq_num is in master bounds: ****/
	if ((irq_num >= 0) && (irq_num <= 7)) {
		/* left circular shift by irq_num */
		int b;
		for (b = 0; b < irq_num; b++) {
			mask = (mask << 1);
		}

		master_mask = master_mask | mask;
		outb( master_mask, MASTER_8259_PORT + 1 );
		return;
	}

	/**** If irq_num is in slave bounds: ****/
	if ((irq_num >= 8) && (irq_num <= 15)) {
		irq_num -= 8; /* Get irq_num into 0-7 range */
		/* left circular shift by irq_num */
		int b;
		for (b = 0; b < irq_num; b++) {
			mask = (mask << 1);
		}

		slave_mask = slave_mask | mask;
		outb( slave_mask, SLAVE_8259_PORT + 1 );
		return;
	}
}

/*
 * send_eoi()
 *
 * Description:
 * Send end-of-interrupt signal for the specified IRQ
 *
 * Inputs: none
 *
 * Retvals: none
 */
void send_eoi(uint32_t irq_num)
{
	/**** If irq_num is in master bounds: ****/

	if ((irq_num >= 0) && (irq_num <= 7)) {
		outb( EOI | irq_num, MASTER_8259_PORT);
	}

	/**** If irq_num is in slave bounds: ****/

	if ((irq_num >= 8) && (irq_num <= 15)) {
		outb( EOI | (irq_num - 8), SLAVE_8259_PORT );
		outb( EOI + 2, MASTER_8259_PORT);
	}

}
