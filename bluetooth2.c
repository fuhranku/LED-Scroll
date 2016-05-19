#include "MK64F12.h"

int count = 0;

int uart_getchar(void);

void UARTsetup() {
	//Clock UART4 and PTC
	SIM->SCGC1 |= SIM_SCGC1_UART4_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
	
	//Configure the pins as UART Rx and Tx
	PORTC_PCR14 |= PORT_PCR_MUX(3);
	PORTC_PCR15 |= PORT_PCR_MUX(3);
	
	//First disable R and T
	UART4_C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK );
	
	UART4_RWFIFO = 0xFF; 			//increase size of message
	
	UART4_C1 = 0; //Default

	UART4_BDL = 0x88;
	
	//Enable T and R
	UART4_C2 |=(UART_C2_TE_MASK |UART_C2_RE_MASK);
}

int uart_getchar() {
	int temp = 0;
	
	while(!UART4_RCFIFO && temp < 100) {				//waits until something in queue
		//large enough to capture all letters
		//small enough to not wait too long 
		temp ++;					//temp can be modified accordingly
		//return 1;
	}
	
	if(UART4_RCFIFO) {
		count ++;
		return 1;
	}
	
	/* Return the 8-bit data from the receiver */
	return 0;
}