#include "MK64F12.h"
#include "bluetooth2.h"

int ch;
int letter;

int main() {
		UARTsetup();
		Pit_Setup();

		while(1);
}

//use getD() to get the ASCII value
