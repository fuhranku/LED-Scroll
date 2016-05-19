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

void Pit_Setup() {
	SIM->SCGC6 = SIM_SCGC6_PIT_MASK;     //Enable the clock to the PIT module
	PIT_MCR = 0x00;										   //Enables timer
	PIT->CHANNEL[0].LDVAL = 1;   //Set the load value of the zeroth PIT 
	PIT->CHANNEL[0].TCTRL  = 3;
	NVIC_EnableIRQ(PIT0_IRQn);
}

void PIT0_IRQHandler(void) {
	PIT->CHANNEL[0].TFLG |= 0x1;		 		 //Have to reset the flag
	char d_reg = uart_getchar();
	if(d_reg != 0) {
		add_char(d_reg);
		UART4_D = 0;
	}
}

int uart_getchar() {
	int temp = 0;
	
	while(!UART4_RCFIFO && temp < 20970) {				//waits until something in queue
		temp ++;					//temp can be modified accordingly
	}
	
	if(UART4_RCFIFO)
		count ++;
	
	/* Return the 8-bit data from the receiver */
	return UART4_D;
}

int getD() {
	return UART4_D;
}