/*************************************************/
/* files.c - The terminal driver for the kernel. */
/*************************************************/
#ifndef KEYBOARD_H
#define KEYBOARD_H



/*** I/O Constants ***/
#define KEYBOARD_PORT			0x60
#define KEYBOARD_STATUS_PORT	0x64

/*** keyboard flags ***/ 
#define FLAG_NOTHING			0x00
#define FLAG_SHIFT				0x01
#define FLAG_CAPS				0x02
#define FLAG_SHIFT_CAPS			0x03
#define FLAG_SHIFT_CAPS_MASK	0x03
#define FLAG_CTRL				0x04
#define BUFFER_NOT_EMPTY		0x02

/*** IRQ Constant ***/
#define KEYBOARD_IRQ		1

/*** Make/Break Constants ***/
#define MAKE_1						0x02
#define MAKE_EQUALS					0x0D
#define MAKE_Q						0x10
#define MAKE_R_SQUARE_BRACKET		0x1B
#define MAKE_A						0x1E
#define MAKE_ACCENT					0x29
#define MAKE_BACKSLASH				0x2B
#define MAKE_SLASH					0x35
#define MAKE_SPACE					0x39
#define MAKE_ENTER					0x1C
#define MAKE_BKSP					0x0E
#define MAKE_DELETE					0x53
#define MAKE_CAPS					0x3A
#define MAKE_L_SHFT					0x2A
#define MAKE_R_SHFT					0x36
#define BREAK_L_SHFT				0xAA
#define BREAK_R_SHFT				0xB6
#define MAKE_L_CTRL					0x1D
#define BREAK_L_CTRL				0x9D
#define MAKE_L_ALT					0x38
#define BREAK_L_ALT					0xB8
#define EXTRAS						0xE0
#define MAKE_L_ARROW				0x4B
#define MAKE_R_ARROW				0x4D
#define MAKE_L						0x26
#define MAKE_F1						0x3B
#define MAKE_F2						0x3C
#define MAKE_F3						0x3D

/*** Unneccessary Make/Break Constants ***/
#define MAKE_B						0x30
#define MAKE_C						0x2E
#define MAKE_D						0x20
#define MAKE_E						0x12
#define MAKE_F						0x21
#define MAKE_G						0x22
#define MAKE_H						0x23
#define MAKE_I						0x17
#define MAKE_J						0x24
#define MAKE_K						0x25
#define MAKE_M						0x32
#define MAKE_N						0x31
#define MAKE_O						0x18
#define MAKE_P						0x19
#define MAKE_R						0x13
#define MAKE_S						0x1F
#define MAKE_T						0x14
#define MAKE_U						0x16
#define MAKE_V						0x2F
#define MAKE_W						0x11
#define MAKE_X						0x2D
#define MAKE_Y						0x15
#define MAKE_Z						0x2C
#define MAKE_0						0x0B
#define MAKE_2						0x03
#define MAKE_3						0x04
#define MAKE_4						0x05
#define MAKE_5						0x06
#define MAKE_6						0x07
#define MAKE_7						0x08
#define MAKE_8						0x09
#define MAKE_9						0x0A
#define MAKE_HYPHEN					0x0C
#define MAKE_TAB					0x0F
#define MAKE_ESC					0x01
#define MAKE_F4						0x3E
#define MAKE_F5						0x3F
#define MAKE_F6						0x40
#define MAKE_F7						0x41
#define MAKE_F8						0x42
#define MAKE_F9						0x43
#define MAKE_F10					0x44
#define MAKE_F11					0x57
#define MAKE_F12					0x58
#define MAKE_SCROLL					0x46
#define MAKE_L_SQUARE_BRACKET		0x1A
#define MAKE_NUM					0x45
#define MAKE_SEMICOLON				0x27
#define MAKE_APOSTRPHE				0x28
#define MAKE_COMMA					0x33
#define MAKE_PERIOD					0x34
#define BREAK_A						0x9E
#define BREAK_B						0xB0
#define BREAK_C						0xAE
#define BREAK_D						0xA0
#define BREAK_E						0x92
#define BREAK_F						0xA1
#define BREAK_G						0xA2
#define BREAK_H						0xA3
#define BREAK_I						0x97
#define BREAK_J						0xA4
#define BREAK_K						0xA5
#define BREAK_L						0xA6
#define BREAK_M						0xB2
#define BREAK_N						0xB1
#define BREAK_O						0x98
#define BREAK_P						0x99
#define BREAK_Q						0x90
#define BREAK_R						0x93
#define BREAK_S						0x9F
#define BREAK_T						0x94
#define BREAK_U						0x96
#define BREAK_V						0xAF
#define BREAK_W						0x91
#define BREAK_X						0xAD
#define BREAK_Y						0x95
#define BREAK_Z						0xAC
#define BREAK_0						0x8B
#define BREAK_1						0x82
#define BREAK_2						0x83
#define BREAK_3						0x84
#define BREAK_4						0x85
#define BREAK_5						0x86
#define BREAK_6						0x87
#define BREAK_7						0x88
#define BREAK_8						0x89
#define BREAK_9						0x8A
#define BREAK_ACCENT				0x89
#define BREAK_HYPHEN				0x8C
#define BREAK_EQUALS				0x8D
#define BREAK_BACKSLASH				0xAB
#define BREAK_BKSP					0x8E
#define BREAK_SPACE					0xB9
#define BREAK_TAB					0x8F
#define BREAK_CAPS					0xBA
#define BREAK_ENTER					0x9C
#define BREAK_ESC					0x81
#define BREAK_F1					0xBB
#define BREAK_F2					0xBC
#define BREAK_F3					0xBD
#define BREAK_F4					0xBE
#define BREAK_F5					0xBF
#define BREAK_F6					0xC0
#define BREAK_F7					0xC1
#define BREAK_F8					0xC2
#define BREAK_F9					0xC3
#define BREAK_F10					0xC4
#define BREAK_F11					0xD7
#define BREAK_F12					0xD8
#define BREAK_SCROLL				0xC6
#define BREAK_L_SQUARE_BRACKET		0x9A
#define BREAK_NUM					0xC5
#define BREAK_R_SQUARE_BRACKET		0x9B
#define BREAK_SEMICOLON				0xA7
#define BREAK_APOSTRPHE				0xA8
#define BREAK_COMMA					0xB3
#define BREAK_PERIOD				0xB4
#define BREAK_SLASH					0xB5
#define BREAK_L_ARROW				0xCB
#define BREAK_R_ARROW				0xCD
#define BREAK_DELETE				0xD3

/*** Other Constants ***/
#define TERMINAL_BUFFER_MAX_SIZE   1024
#define CURSOR_START				7



/* Called to initialize keyboard before using it. */
void keyboard_open(void);

/* Called to read from command buffer */
int32_t terminal_read(unsigned char * buf, int32_t nbytes);

/* Called to read from command buffer */
int32_t terminal_write(const unsigned char * buf, int32_t nbytes);

/* Called to read from command buffer */
void printthebuffer(void);

/* Keyboard Interrupt */
void keyboard_interruption(void);

/* Keyboard Interrupt */
void keyboard_interruption(void);

/* Returns active terminal */
uint32_t get_active_terminal( void );



#endif /* KEYBOARD_H */
