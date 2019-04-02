/**********************************************/
/* idt.c - The idt config part of the kernel. */
/**********************************************/
#ifndef IDT_H
#define IDT_H



#define PIT_INT			0x20
#define KEYBOARD_INT	0x21
#define RTC_INT			0x28
#define SYSCALL_INT		0x80



/* Initialize the IDT */
void init_idt ();



#endif /* IDT_H */
