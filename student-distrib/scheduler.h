/******************************************************************/
/* scheduler.h - The scheduler and PIT controller for the kernel. */
/******************************************************************/
#ifndef SCHEDULER_H
#define SCHEDULER_H


/* PIT Chip's Command Register Port */
#define PIT_CMDREG        0x43

/* PIT Channel 0's Data Register Port */
#define PIT_CHANNEL0      0x40

/* Divisors for PIT Frequency setting 
 * DIVISOR_???HZ = 1193180 / HZ;  
 */
#define DIVISOR_100HZ	11932
#define DIVISOR_33HZ	36157
#define DIVISOR_20HZ	59659

/* Pit Mode 3 */
#define PIT_MODE3		0x36

/* IRQ Constant. */
#define PIT_IRQ			0



/* Initializes the PIT for usage. */
void pit_init(void);

/* The handler for an PIT interrupt. */
void pit_interruption(void); 



#endif /* SCHEDULER_H */
