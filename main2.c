#include "MK64F12.h"
/*
******************************************************************************************************
* Chip        : MAX7219
* Author      : Burak HANÇERLI
* Mail        : bhancerli@gmail.com
* Description : MAX7219 LED Display Driver Routines - v1.1
*   
* 
*  The Maxim MAX7219 is a led display driver. It can control up to 64 individual leds or eight
*  7-segment-display.
*
*  Max7912 uses 3-wire to communicate with microcontroller.
*    - DATA   : Used to transmit data.
*    - CLOCK  : Used to generate clock signal which is required for transmitting each bite of data.
*    - LOAD   : Used to load data to the Max7913's Dual-Port SRam.
*                 
*
*
*  Advantages of Max7219 :
*  - Adjustable intensity. (More or less bright leds)
*  - Shutdown mode (much less power consumption)
*  - Test mode (To see if digits are working or not)
*  - Adjustable digit number. (1 to 8 seven-segment-display)
*  - Changeable decode mode. (codeB-decode-mode or no-decode-mode)
*      (Look over datasheet for more detailed information)
*
*
*  DESCRIPTION OF USER FUNCTIONS
* -------------------------------
*  init7219()                      : Required for initialize MAX7219. This function have has to be called before calling any function.
*  write7219(digit, data)          : Writes data to the specified digit number. If Decimal Point needed on any digit, just add
                                     128 to the data.
                                     For example : write7219(1,3)   = writes "3"  to first digit.
                                                   write7219(1,131) = writes "3." to first digit.
*  shutdown7219(operatingMode)     : Set operatingMode = 0 to Shutdown mode
*                                    Set operatingMode = 1 to Normal mode
*  decode7219(decodeMode)          : Sets digit-decode mode. code-B or no-decode mode. Look up datasheet for detailed instructions.
*                                    For example, if user sets decodeMode = 4, (4=0b00000100), then 3. digit will be
*                                    decoded as code-B algorithm, but other pins don't have any decode mode.
*  brightness7219(brightnessLevel) : Sets brightness level of digits.
*                                    brightnessLevel = 0  ; minimum brightness level
*                                    brightnessLevel = 15 ; maximum brigthness level
*                                   
*  scanLimit7219(totalDigitNumber) : Sets number of connected digits to the MAX7219. When init7912() function is called,
                                     number of digit will be setted to 4 as default.
                                     
*  test7219(testMode)              : Sets 7-segment-display test-mode on or off.
                                     testMode = 0 ; normal operation mode
                                     testMode = 1 ; display test mode
                                     
** THIS LIBRARY CAN BE USED, DEVELOPED OR SHARED WITH REFERRING THE AUTHOR.                                   
******************************************************************************************************/


// CONSTANTS //
// - Connection Pins (CHANGE THESE PINS AS YOU WISH)
#define CLK       PTD
#define LOAD      PTD
#define DATA      PTD

// - Mode Selection
#define decode 0x09
#define brightness 0x0A
#define scanLimit 0x0B
#define shutDown 0x0C
#define dispTest 0x0F

// Firt 4 bites (not used generally)
#define firstBites 0x0

// Wait function
#define wait delay_ms(1)

#define DATA_0()  DATA->PCOR |= (1 << 2);
#define DATA_1()  DATA->PSOR |= (1 << 2);

#define LOAD_0()  LOAD->PCOR |= (1 << 0);
#define LOAD_1()  LOAD->PSOR |= (1 << 0);

#define CLK_0()   CLK->PCOR |= (1 << 1);
#define CLK_1()   CLK->PSOR |= (1 << 1);

long serialData=0;

void delay_ms(int);
int get_bit(long, int);
void output_bit(int);
void init7219(void);
void shutdown7219(int);
void write7219(unsigned char, int);
int main() {
  long row = 0x02ff;
  init7219();
  shutdown7219(1); //try without this line as well
  while(1) {
    write7219(long,);
  }
}

void clock7219() // clock (CLK) pulse
{
   CLK_0();
   wait;
   CLK_1();
}

void load7219()  // load (LOAD) pulse
{
   LOAD_0();
   wait;
   LOAD_1();
}

void send7219(char data) // send 16 bit data word to the 7219
{

   int count;
   for (count=0; count<16; ++count)
   {
      output_bit(get_bit(data, count)); // set data (DIN) level
      clock7219(); // clock data in
   }
   load7219(); // latch the last 16 bits clocked
}

void dataMaker(unsigned char mode, int dataIncoming) // Standart data package function
{
   serialData=firstBites;
   serialData<<=4;
   serialData|=mode;
   serialData<<=8;
   serialData|=dataIncoming;
   send7219(serialData);
}

void write7219(unsigned char digit, int data) // Send data to digits
{
   dataMaker(digit, data);
}

void init7219()
{
   dataMaker(shutDown, 1);     // No-Shutdown mode. Normal Operation mode.
   dataMaker(decode, 15);      // All digits are programmed as code-B decode mode.
   dataMaker(scanLimit, 4);    // Total digit number set to 4.
   dataMaker(brightness, 15);  // Full brightness.
}

void delay_ms(int ms)
{
  int i;
  for(i=0; i < ms * 20970; i++);
}

int get_bit(char data, int num) 
{
  return (data << num) & 1;
}

void output_bit(int bit)
{
  if(bit) {
    DATA_1();
  }
  else {
    DATA_0();
  }
}