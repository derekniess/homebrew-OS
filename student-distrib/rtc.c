/******************************************/
/* rtc.c - The RTC driver for the kernel. */
/******************************************/
#include "lib.h"
#include "rtc.h"
#include "i8259.h"
#include "keyboard.h"



/* Flag used to lock an rtc_read and prevent its returning until a
 * clock interrupt occurs.  */
volatile int interrupt_occurred = 0;



/*
 * rtc_init()
 *
 * Initializes the RTC.
 *
 * Inputs: none
 * Retvals: none
 */
void rtc_init(void) 
{
	/* Get previous values of the RTC's Registers A and B */
	outb(INDEX_REGISTER_A, RTC_PORT);
	unsigned char a_old = inb(CMOS_PORT);
	outb(INDEX_REGISTER_B, RTC_PORT);
	unsigned char b_old = inb(CMOS_PORT);

	/* 
	 * set A[6:4] (DV) to 010 - turn on oscillator/allow RTC to keep time
	 * set A[3:0] (RS) to 1111 - set interrupt rate to 2 Hz 
	 */
	outb(INDEX_REGISTER_A, RTC_PORT);
	outb((KILL_DV_RS & a_old) | DV_RS, CMOS_PORT);

	/* 
	 * set B[7] (SET) to 0 - to allow normal updating
	 * set B[6] (PIE) to 1 - to turn on period interrupts
	 * set B[5] (AIE) to 0 - not allow alarm flag to assert irq'
	 * set B[4] (UIE) to 0 - not allow update-end flag to assert irq' 
	 */
	outb(INDEX_REGISTER_B, RTC_PORT);
	outb((KILL_SET_PIE_AIE_UIE & b_old) | SET_PIE_AIE_UIE, CMOS_PORT);

	/* Set clock to 32 Hz.
	 * NOTE -- We need this to be fast enough since we are using the RTC
	 *         to repaint the screen
	 */
	int hertz = 32;
	rtc_write(&hertz, 4);
	
	enable_irq(RTC_IRQ);
}

/*
 * clock_interruption()
 *
 * The handler for an RTC interrupt.
 *
 * Inputs: none
 * Retvals: none
 */
void clock_interruption(void) 
{
	/* Mask interrupts */
	cli();

	/* We want to read in what Register C has got for us */
	outb(INDEX_REGISTER_C, RTC_PORT);
	
	/* We don't even care what it is at this point */
	inb(CMOS_PORT);

	/* Send End-of-Interrupt */
	send_eoi(RTC_IRQ);

	/* Set the interrupt_occured flag to one. */
	interrupt_occurred = 1;
	
	/* Update the video memory to match the appropriate video buffer */
	update_vid();
	
	/* Unmask interrupts */
	sti();
}

/*
 * rtc_read()
 *
 * Should always return 0, but only after an interrupt has occurred 
 * (set a flag and wait until the interrupt handler clears it, then 
 * return 0).
 *
 * Inputs: none
 * Retvals: none
 */
int32_t rtc_read (uint32_t a, int32_t b, int32_t c, int32_t d) 
{
	/* Spin until the interrupt has occurred */
	while (!interrupt_occurred) 
	{
		/* Do nothing. */
	}
	
	/* Clear the flag. */
	interrupt_occurred = 0;

	/* Always return 0. */
	return 0;
}

/*
 * rtc_write()
 *
 * Should always accept only a 4-byte integer specifying the interrupt 
 * rate in Hz, and should set the rate of periodic interrupts accordingly.
 *
 * Inputs: 
 * buf: hz to be set
 * nbytes: number of bytes to set
 * Retvals
 * -1: failure
 * n: number of bytes written
 */
int32_t rtc_write (int32_t * buf, int32_t nbytes) 
{
	/* Local variables. */
	int32_t freq;
	int8_t rs;
	
	/* If rtc_write doesn't receive 4 bytes, fail. */	
	if (4 != nbytes) 
	{
		return -1;
	}
	
	/* If pointer to frequency is null, fail. */
	if (NULL == buf) 
	{
		return -1;
	} 
	else 
	{
		freq = *buf;
	}

	/* Get the old value of RTC Register A to save values we don't change. */
	outb(INDEX_REGISTER_A, RTC_PORT);
	unsigned char a_old = inb(CMOS_PORT);

	/* Find the correct byte to send to the RTC based on the requested freq */
	switch(freq) {
		case 8192:
		case 4096:
		case 2048:
			return -1;
		case 1024: rs = HZ1024; break;
		case 512: rs = HZ512; break;
		case 256: rs = HZ256; break;
		case 128: rs = HZ128; break;
		case 64: rs = HZ64; break;
		case 32: rs = HZ32; break;
		case 16: rs = HZ16; break;
		case 8: rs = HZ8; break;
		case 4: rs = HZ4; break;
		case 2: rs = HZ2; break;
		case 0: rs = HZ0; break;
		default:
			return -1;
	}

	/* set A[3:0] (RS) to rs */
	outb(INDEX_REGISTER_A, RTC_PORT);
	outb((KILL_RS & a_old) | rs, CMOS_PORT);

	/* return number of bytes on success (always 0) */
	return 0;
}

/*
 * rtc_open()
 *
 * Opens the RTC.
 *
 * Inputs: none
 * Retvals: 0
 */
int32_t rtc_open (void) 
{
	return 0;
}

/*
 * rtc_close()
 *
 * Closes the RTC.
 *
 * Inputs: none
 * Retvals: 0 
 */
int32_t rtc_close (void) 
{
	return 0;
}


/*
 * update_vid()
 *
 * Updates the actual screen with the data from the appropriate video buffer.
 * NOTE -- Our three ttys will automatically update their own video buffers...
 *         We need to display the one that is "active". We want to run this
 *         screen update periodically, so we do it with rtc interrupts.
 *
 * Inputs: none
 * Retvals: none
 */
void update_vid( void )
{
	/* Get the active terminal number */
	uint32_t active_terminal = get_active_term();
	
	/* Copy the buffer for the active terminal into the actual video memory */
	switch( active_terminal )
	{
		case 0:
			memcpy((uint8_t *)VIDEO, (uint8_t *)VIDEO_BUF1, _4KB);
			break;
		case 1:
			memcpy((uint8_t *)VIDEO, (uint8_t *)VIDEO_BUF2, _4KB);
			break;
		case 2:
			memcpy((uint8_t *)VIDEO, (uint8_t *)VIDEO_BUF3, _4KB);
			break;
	}
}

