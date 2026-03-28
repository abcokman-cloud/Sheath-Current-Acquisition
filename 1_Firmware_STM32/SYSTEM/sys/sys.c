#include "sys.h"  

/* System initialization and core assembly instructions */

// Execute WFI (Wait For Interrupt) instruction
__asm void WFI_SET(void)
{
	WFI;		  
}

// Disable all interrupts (except fault and NMI)
__asm void INTX_DISABLE(void)
{
	CPSID   I
	BX      LR	  
}

// Enable all interrupts
__asm void INTX_ENABLE(void)
{
	CPSIE   I
	BX      LR  
}

// Set stack pointer address
// addr: Stack pointer address
__asm void MSR_MSP(u32 addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14
}
