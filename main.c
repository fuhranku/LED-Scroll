#include "MK64F12.h"
#include "led_control.h"

int main() {
  int i;
  //setup port pins as GPIO output
  setup();
  //configure the LED matrix
  init();
  //setup the bluetooth receiver
  UARTsetup();

  //flash once to show LED working
  delay(100);
  lightAll();
  delay(100);
  clear();
  delay(100);

  //beging our scrolling display
  scroll_display();
  
  while(1);
}