/*
 * ppc470 interrupt setup
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/arch-ppc4xx/ppc470s_itrpt.h>  
#include <asm/arch-ppc4xx/ppc470s_itrpt_fields.h>
#include <asm/arch-ppc4xx/ppc470s_reg_access.h>

DECLARE_GLOBAL_DATA_PTR;

struct irq_action {
	interrupt_handler_t *handler;
	void *arg;
	ulong count;
};

//Declare all handlers
void irq_handler__Critical_input(void);
void irq_handler__Machine_check(void);
void irq_handler__Data_storage(void);
void irq_handler__Instruction_storage(void);
void irq_handler__External_input(void);
void irq_handler__Alignment(void);
void irq_handler__Program(void);
void irq_handler__FP_unavailable(void);
void irq_handler__System_call(void);
void irq_handler__AP_unavailable(void);
void irq_handler__Decrementer(void);
void irq_handler__FIT(void);
void irq_handler__Watchdog_timer(void);
void irq_handler__DTLB_miss(void);
void irq_handler__ITLB_miss(void);
void irq_handler__Debug(void);


/*
static inline void mpic128_set_interrupt_borders(uint32_t base_address, uint32_t mcb, uint32_t crb)
{
    write_MPIC128_VITC( base_address, (mcb << IBM_BIT_INDEX(32, MPIC128_VITC_MCB)) | (crb << IBM_BIT_INDEX(32, MPIC128_VITC_CRB)));
}


static inline void mpic128_setup_interrupt(uint32_t base_address, int vector)
{
    mtdcrx(base_address + mpic128_vector_table[vector].VP, mpic128_vector_table[vector].VP_val);
}

static inline void mpic128_disable_pass_through(uint32_t base_address)
{
    uint32_t tmp = read_MPIC128_GCF0(base_address);
    tmp |= 1 << IBM_BIT_INDEX(32, MPIC128_GCF0_P);
    write_MPIC128_GCF0( base_address, tmp );
}
*/

void interrupt_init_cpu (unsigned *decrementer_count)
{
    //Disable interrupts on the current processor
    write_SPR_MSR( read_SPR_MSR() & ~(
               (0b1 << ITRPT_XSR_CE_i) //MSR[CE] - Critical interrupt.
             | (0b1 << ITRPT_XSR_EE_i) //MSR[EE] - External interrupt.
             | (0b1 << ITRPT_XSR_ME_i) //MSR[ME] - Machine check.
             | (0b1 << ITRPT_XSR_DE_i) //MSR[DE] - Debug interrupt.
             ));

    //Setup interrupt handlers
    SET_INTERRUPT_HANDLER( ITRPT_CRITICAL_INPUT,        irq_handler__Critical_input );
    SET_INTERRUPT_HANDLER( ITRPT_MACHINE_CHECK,         irq_handler__Machine_check );
    SET_INTERRUPT_HANDLER( ITRPT_DATA_STORAGE,          irq_handler__Data_storage );
    SET_INTERRUPT_HANDLER( ITRPT_INSTRUCTION_STORAGE,   irq_handler__Instruction_storage );
    SET_INTERRUPT_HANDLER( ITRPT_EXTERNAL_INPUT,        irq_handler__External_input );
    SET_INTERRUPT_HANDLER( ITRPT_ALIGNMENT,             irq_handler__Alignment );
    SET_INTERRUPT_HANDLER( ITRPT_PROGRAM,               irq_handler__Program );
    SET_INTERRUPT_HANDLER( ITRPT_FP_UNAVAILABLE,        irq_handler__FP_unavailable );
    SET_INTERRUPT_HANDLER( ITRPT_SYSTEM_CALL,           irq_handler__System_call );
    SET_INTERRUPT_HANDLER( ITRPT_AP_UNAVAILABLE,        irq_handler__AP_unavailable );
    SET_INTERRUPT_HANDLER( ITRPT_DECREMENTER,           irq_handler__Decrementer );
    SET_INTERRUPT_HANDLER( ITRPT_FIXED_INTERVAL_TIMER,  irq_handler__FIT );
    SET_INTERRUPT_HANDLER( ITRPT_WATCHDOG_TIMER,        irq_handler__Watchdog_timer );
    SET_INTERRUPT_HANDLER( ITRPT_DATA_TLB_ERROR,        irq_handler__DTLB_miss );
    SET_INTERRUPT_HANDLER( ITRPT_INSTRUCTION_TLB_ERROR, irq_handler__ITLB_miss );
    SET_INTERRUPT_HANDLER( ITRPT_DEBUG,                 irq_handler__Debug );

/*
    mpic128_set_interrupt_borders( MPICx_DCR, MPIC128_INTERRUPT_TYPE__MACHINE_CHECK, MPIC128_INTERRUPT_TYPE__CRITICAL );

    mpic128_disable_pass_through( MPICx_DCR );

    //Setup interrupt lines on the MPIC controller
    mpic128_vector vector;
    for (vector = MPIC128_VECTOR__BEGIN; vector != MPIC128_VECTOR__END; ++vector)
    {
        mpic128_setup_interrupt(MPICx_DCR, vector);
    }

    //Permit all interrupts with priorities higher MPIC128_PRIORITY__0 to be passed to current processor
    mpic128_set_current_processor_task_priority_level( MPICx_DCR, MPIC128_PRIORITY__0 );
*/
}

/*
 * Handle external interrupts
 */

void external_interrupt (struct pt_regs *regs)
{
}


/*
 * Install and free an interrupt handler.
 */

void
irq_install_handler (int irq, interrupt_handler_t * handler, void *arg)
{
}


void irq_free_handler (int irq)
{
}


void timer_interrupt_cpu (struct pt_regs *regs)
{
	/* nothing to do here */
	return;
}


#if defined(CONFIG_CMD_IRQ)

/* ripped this out of ppc4xx/interrupts.c */

/*
 * irqinfo - print information about PCI devices
 */

void
do_irqinfo(cmd_tbl_t *cmdtp, bd_t *bd, int flag, int argc, char * const argv[])
{
}

#endif

typedef void (*voidcall)(void);
static voidcall bootrom_main = (voidcall)BOOT_ROM_MAIN;

__attribute__( ( always_inline ) ) static inline uint32_t __get_LR(void) 
{ 
  register uint32_t result; 

  asm volatile ("lwz %0, 48(r1)\n" : "=r" (result) ); 
//  asm volatile ("mflr %0\n" : "=r" (result) ); 
  return(result); 
} 

__attribute__( ( always_inline ) ) static inline uint32_t __get_SRR0(void) 
{ 
  register uint32_t result; 

  asm volatile ("mfspr %0, srr0\n" : "=r" (result) ); 
  return(result); 
} 

__attribute__( ( always_inline ) ) static inline uint32_t __get_SRR1(void) 
{ 
  register uint32_t result; 

  asm volatile ("mfspr %0, srr1\n" : "=r" (result) ); 
  return(result); 
} 


__attribute__( ( always_inline ) ) static inline uint32_t __get_CSRR0(void) 
{ 
  register uint32_t result; 

  asm volatile ("mfspr %0, 58\n" : "=r" (result) ); 
  return(result); 
} 

__attribute__( ( always_inline ) ) static inline uint32_t __get_CSRR1(void) 
{ 
  register uint32_t result; 

  asm volatile ("mfspr %0, 59\n" : "=r" (result) ); 
  return(result); 
} 

__attribute__( ( always_inline ) ) static inline uint32_t __get_R1(void) 
{ 
  register uint32_t result; 

  asm volatile ("mr %0, r1\n" : "=r" (result) ); 
  return(result); 
} 

__attribute__( ( always_inline ) ) static inline uint32_t __get_R3(void) 
{ 
  register uint32_t result; 

  asm volatile ("mr %0, r3\n" : "=r" (result) ); 
  return(result); 
} 


#define DEFINE_INT_HANDLER(handler) \
void irq_handler__##handler (void) 	\
{					\
	printf("\n\n !!! Interrupt happens (" #handler ")\n"); \
    printf("\tR1:\t0x%08X\n\tR3:\t0x%08X\n\tLR:\t0x%08X\n\tSRR0:\t0x%08X\n\tSRR1:\t0x%08X\n\tCSRR0:\t0x%08X\n\tCSRR1:\t0x%08X\n\n", \
                             __get_R1(), __get_R3(), __get_LR(), __get_SRR0(), __get_SRR1(), __get_CSRR0(), __get_CSRR1()); \
	bootrom_main();\
}					


DEFINE_INT_HANDLER(Critical_input)
DEFINE_INT_HANDLER(Machine_check)
DEFINE_INT_HANDLER(Data_storage)
DEFINE_INT_HANDLER(Instruction_storage)
DEFINE_INT_HANDLER(External_input)
DEFINE_INT_HANDLER(Alignment)
DEFINE_INT_HANDLER(Program)
DEFINE_INT_HANDLER(FP_unavailable)
DEFINE_INT_HANDLER(System_call)
DEFINE_INT_HANDLER(AP_unavailable)
DEFINE_INT_HANDLER(Decrementer)
DEFINE_INT_HANDLER(FIT)
DEFINE_INT_HANDLER(Watchdog_timer)
DEFINE_INT_HANDLER(DTLB_miss)
DEFINE_INT_HANDLER(ITLB_miss)
DEFINE_INT_HANDLER(Debug)
