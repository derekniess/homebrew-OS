/**********************************************/
/* idt.c - The idt config part of the kernel. */
/**********************************************/
#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "rtc.h"
#include "keyboard.h"
#include "interrupthandler.h"

/*
 * Exceptions!
 *
 * Description:
 * This macro defines exepctions. Exceptions, when fired, print a message 
 * and then spin forever.
 * Inputs: none
 * Retvals: none
 */
#define EXCEPTION(name,msg)	\
void name() {				\
	printf("%s\n",#msg);	\
	while(1);				\
}

/* Exceptions sent to the exception creator macro. */
EXCEPTION(exception_DE,"Divide Error!");
EXCEPTION(exception_DB,"Debug Exception!");
EXCEPTION(exception_NMI,"Non Maskable Interrupt Exception!");
EXCEPTION(exception_BP,"Breakpoint Exception!");
EXCEPTION(exception_OF,"Overflow Exception!");
EXCEPTION(exception_BR,"BOUND Range Exceeded Exception!");
EXCEPTION(exception_UD,"Invalid Opcode Exception!");
EXCEPTION(exception_NM,"Device Not Available Exception!");
EXCEPTION(exception_DF,"Double Fault Exception!");
EXCEPTION(exception_CS,"Coprocessor Segment Exception!");
EXCEPTION(exception_TS,"Invalid TSS Exception!");
EXCEPTION(exception_NP,"Segment Not Present!");
EXCEPTION(exception_SS,"Stack Fault Exception!");
EXCEPTION(exception_GP,"General Protection Exception!");
EXCEPTION(exception_PF,"Page Fault Exception!");
EXCEPTION(exception_MF,"Floating Point Exception");
EXCEPTION(exception_AC,"Alignment Check Exception!");
EXCEPTION(exception_MC,"Machine Check Exception!");
EXCEPTION(exception_XF,"SIMD Floating-Point Exception!");

/* Undefined Interrupt */
void general_interruption() {
	cli();
	printf("Undefined interruption!");
	sti();
}

/*
 * init_idt()
 *
 * Description:
 * Initializes the Interrupt Descriptor Table.
 * In order to initialize the IDT we fill in the values of the array of struct
 * with the values required of the interruption descriptor as defined in the 
 * ISA manual. Now because of the definition of the struct we don't have to 
 * worry about the size of individual bit manipulations, just put the desired 
 * value in the struct's parameters.
 *
 * Inputs: none
 *
 * Retvals: none
 */
void init_idt () {

	/* Initialize counter */
	int index;

	/* Load IDT size and base address to the IDTR */
	lidt(idt_desc_ptr);
    											
	for(index = 0; index < NUM_VEC; index++) {

		/* Set interruption vector present */
		idt[index].present = 0x1;

		/* Set privilege level to O */
		idt[index].dpl = 0x0;

		/* All vecotrs less than 32 are initialized as interrupts with types 01111 */				
		idt[index].reserved0 = 0x0;
		idt[index].size = 0x1;

		/* Unused bits */
		idt[index].reserved1 = 0x1;
		idt[index].reserved2 = 0x1;
		idt[index].reserved3 = 0x1;
		idt[index].reserved4 = 0x0;

		/* Set seg selector to Kernel CS */
		idt[index].seg_selector = KERNEL_CS;
		
		/* All vectors greater than 32 are initialized as interrupts with
		 * types 01111 and with a general handler for now */
		if(index >= 32) {
			idt[index].reserved3 = 0x0;
			SET_IDT_ENTRY(idt[index], general_interruption);
		}
		
		/* A syscall (int 0x80) comes from privilege level 3 */
		if(SYSCALL_INT == index) {
			idt[index].dpl = 0x3;
		}
	}

	/** Define INT 0x00 through INT 0x13. Route them to respective handlers. */
	SET_IDT_ENTRY(idt[0], exception_DE);
	SET_IDT_ENTRY(idt[1], exception_DF);
	SET_IDT_ENTRY(idt[2], exception_NMI);
	SET_IDT_ENTRY(idt[3], exception_BP);
	SET_IDT_ENTRY(idt[4], exception_OF);
	SET_IDT_ENTRY(idt[5], exception_BR);
	SET_IDT_ENTRY(idt[6], exception_UD);
	SET_IDT_ENTRY(idt[7], exception_NM);
	SET_IDT_ENTRY(idt[8], exception_DF);
	SET_IDT_ENTRY(idt[9], exception_CS);
	SET_IDT_ENTRY(idt[10], exception_TS);
	SET_IDT_ENTRY(idt[11], exception_NP);
	SET_IDT_ENTRY(idt[12], exception_SS);
	SET_IDT_ENTRY(idt[13], exception_GP);
	SET_IDT_ENTRY(idt[14], exception_PF);	
	SET_IDT_ENTRY(idt[16], exception_MF);
	SET_IDT_ENTRY(idt[17], exception_AC);
	SET_IDT_ENTRY(idt[18], exception_MC);
	SET_IDT_ENTRY(idt[19], exception_XF);

	/* PIT interrupt routed to asm wrapper named: pit_handler */
	SET_IDT_ENTRY(idt[PIT_INT], pit_handler);
	
	/* Keyboard interrupt routed to asm wrapper named: keyboard_handler */
	SET_IDT_ENTRY(idt[KEYBOARD_INT], keyboard_handler);

	/* RTC interrupt routed to asm wrapper named: clock_handler */
	SET_IDT_ENTRY(idt[RTC_INT], clock_handler);

	/* System Call interrupt routed to asm wrapper named: syscall_handler */
	SET_IDT_ENTRY(idt[SYSCALL_INT], syscall_handler);

}
