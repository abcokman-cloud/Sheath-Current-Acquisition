#include "delay.h"
#include "sys.h"

/* Include OS header if OS is supported */
#if SYSTEM_SUPPORT_OS
#include "includes.h"
#endif

/* Delay configuration and multiplier variables */
static u8  fac_us = 0;   // us delay multiplier
static u16 fac_ms = 0;   // ms delay multiplier

#if SYSTEM_SUPPORT_OS
// Support for UCOSII
#ifdef 	OS_CRITICAL_METHOD
#define delay_osrunning		OSRunning
#define delay_ostickspersec	OS_TICKS_PER_SEC
#define delay_osintnesting 	OSIntNesting
#endif

// Support for UCOSIII
#ifdef 	CPU_CFG_CRITICAL_METHOD
#define delay_osrunning		OSRunning
#define delay_ostickspersec	OSCfg_TickRate_Hz
#define delay_osintnesting 	OSIntNestingCtr
#endif


// Lock OS task scheduling during us delay
void delay_osschedlock(void)
{
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err; 
	OSSchedLock(&err);
#else
	OSSchedLock();
#endif
}

// Unlock OS task scheduling
void delay_osschedunlock(void)
{	
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err; 
	OSSchedUnlock(&err);
#else
	OSSchedUnlock();
#endif
}

// Call OS built-in delay function
void delay_ostimedly(u32 ticks)
{
#ifdef CPU_CFG_CRITICAL_METHOD
	OS_ERR err; 
	OSTimeDly(ticks, OS_OPT_TIME_PERIODIC, &err);
#else
	OSTimeDly(ticks);
#endif 
}
#endif
			   
/* Initialize delay function based on clock frequency */
void delay_init(u8 SYSCLK)
{
#if SYSTEM_SUPPORT_OS
	u32 reload;
#endif
 	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8); 
	fac_us = SYSCLK / 8;

#if SYSTEM_SUPPORT_OS
	reload = SYSCLK / 8;
	reload *= 1000000 / delay_ostickspersec;
	fac_ms = 1000 / delay_ostickspersec;
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	SysTick->LOAD = reload;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
#else
	fac_ms = (u16)fac_us * 1000;
#endif
}								    

#if SYSTEM_SUPPORT_OS
/* Delay nus (microseconds) */
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told, tnow, tcnt = 0;
	u32 reload = SysTick->LOAD;
	ticks = nus * fac_us;
	delay_osschedlock();
	told = SysTick->VAL;
	while(1)
	{
		tnow = SysTick->VAL;	
		if(tnow != told)
		{	    
			if(tnow < told) tcnt += told - tnow;
			else tcnt += reload - tnow + told;	    
			told = tnow;
			if(tcnt >= ticks) break;
		}  
	};
	delay_osschedunlock();
}  

/* Delay nms (milliseconds) */
void delay_ms(u16 nms)
{	
	if(delay_osrunning && delay_osintnesting == 0)
	{		 
		if(nms >= fac_ms)
		{ 
   			delay_ostimedly(nms / fac_ms);
		}
		nms %= fac_ms;
	}
	delay_us((u32)(nms * 1000));
}
#else
/* Delay nus (microseconds) for non-OS */
void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD = nus * fac_us;
	SysTick->VAL = 0x00;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	do
	{
		temp = SysTick->CTRL;
	} while((temp & 0x01) && !(temp & (1 << 16)));
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL = 0X00;
}

/* Delay xms (milliseconds) limited by 24-bit timer */
void delay_xms(u16 nms)
{	 		  	  
	u32 temp;		   
	SysTick->LOAD = (u32)nms * fac_ms;
	SysTick->VAL = 0x00;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	do
	{
		temp = SysTick->CTRL;
	} while((temp & 0x01) && !(temp & (1 << 16)));
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL = 0X00;
} 

/* Delay nms (milliseconds) */
void delay_ms(u16 nms)
{	 	 
	u8 repeat = nms / 540;
	u16 remain = nms % 540;
	while(repeat)
	{
		delay_xms(540);
		repeat--;
	}
	if(remain) delay_xms(remain);
} 
#endif
