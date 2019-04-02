/******************************************/
/* rtc.h - The RTC driver for the kernel. */
/******************************************/
#ifndef RTC_H
#define RTC_H



/* IO Constants. */
#define RTC_PORT		0x70
#define CMOS_PORT		0x71
#define INDEX_REGISTER_A	0x8A
#define	INDEX_REGISTER_B	0x8B
#define	INDEX_REGISTER_C	0x8C
#define	INDEX_REGISTER_D	0x8D

/* Useful init masks. */
#define KILL_DV_RS		0x80 
#define KILL_RS			0xF0 
#define DV_RS			0x2F
#define KILL_SET_PIE_AIE_UIE	0x0F 
#define SET_PIE_AIE_UIE		0x40 

/* Frequency constants. */
#define HZ0			0x00
#define HZ2			0x0F
#define HZ4			0x0E
#define HZ8			0x0D
#define HZ16			0x0C
#define HZ32			0x0B
#define HZ64			0x0A
#define HZ128			0x09
#define HZ256			0x08
#define HZ512			0x07
#define HZ1024			0x06

/* IRQ Constant. */
#define RTC_IRQ			8

/* Initializes the RTC for usage. */
void rtc_init(void);

/* The handler for an RTC interrupt. */
void clock_interruption(void); 

/* Should always return 0, but only after an interrupt has occurred. */
int32_t rtc_read (uint32_t a, int32_t b, int32_t c, int32_t d);

/* Sets the rate of periodic interrupts. */
int32_t rtc_write (int32_t * buf, int32_t nbytes);

/* Opens the RTC. */
int32_t rtc_open (void);

/* Closes the RTC. */
int32_t rtc_close (void);

/* Redraws the screen from the appropriate video buffer */
void update_vid( void );

#endif /* RTC_H */

