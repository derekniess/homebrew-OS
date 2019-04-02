/*********************************************************************/
/* interrupthandler.h -  Set up interrupt handler assembly wrappers. */
/*********************************************************************/
#ifndef INTERRUPT_HANDLER_H
#define INTERRUPT_HANDLER_H



#define SYS_HALT    1
#define SYS_EXECUTE 2
#define SYS_READ    3
#define SYS_WRITE   4
#define SYS_OPEN    5
#define SYS_CLOSE   6
#define SYS_GETARGS 7
#define SYS_VIDMAP  8
#define SYS_SET_HANDLER  9
#define SYS_SIGRETURN  10



/* Keyboard interrupt asm wrapper */
extern void keyboard_handler();

/* Clock interrupt asm wrapper */
extern void clock_handler();

/* PIT interrupt asm wrapper */
extern void pit_handler();

/* System Call interrupt asm wrapper */
extern void syscall_handler();

/* System Call interrupt asm wrapper */
extern void test_syscall(uint32_t syscallnum, uint32_t param1, uint32_t param2, uint32_t param3);

/* Jumps to user space. */
extern void to_the_user_space(int32_t newEIP);



#endif /* INTERRUPT_HANDLER_H*/
