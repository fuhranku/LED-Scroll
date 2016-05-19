#include "MK64F12.h"
#include <stdlib.h>
#include "encoding.h"
#include "bluetooth2.h"

// Drive different pins high or low
#define DATA_0()  PTD->PCOR |= (1 << 2);
#define DATA_1()  PTD->PSOR |= (1 << 2);

#define LOAD_0()  PTD->PCOR |= (1 << 0);
#define LOAD_1()  PTD->PSOR |= (1 << 0);

#define CLK_0()   PTD->PCOR |= (1 << 1);
#define CLK_1()   PTD->PSOR |= (1 << 1);

//Simple dynamic queue data structures
//For backlogged characters, to optimize space
typedef struct overflow_queue {
  int col;
  struct overflow_queue* next;
} backlog;

//Column values to be displayer
typedef struct display_queue {
  char character;
  struct display_queue* next;
} queue;

//current queue of letters to show
queue* current = NULL;
int current_size = 0;

//buffer queue, max 16
queue* buffer = NULL;
int buffer_size = 0;

//unbounded backlog buffer
backlog* extra = NULL;

//global speed variable
//a lower speed is actually faster because it reduces wait-time delay
int speed = 15;

/************************************************
 * init(void)
 * Configures our MAX 7219 before turning it on
 ************************************************/
void init(void) {
  //set decode mode to no-decode
  sendWord(0x0900);
  //scan limit equal to all 8 rows
  sendWord(0x0B07);
  //clear the board initially
  clear();
  //shutdown mode off
  sendWord(0x0C01);
}

/*****************************************************
 * sendBit(int value)
 * Pushes a single bit into the shift register
 * Drives a 1 or 0 based on value into the GPIO
 * Raises the clock high to push data then back to low
 * for the next cycle
 *****************************************************/
void sendBit(int value){
  if(value == 1){
    DATA_1();
  }
  else{
    DATA_0();
  }
  //rising clock edge
  PTD->PSOR |= (1 << 1);
  //clock to 0
  PTD->PCOR |= (1 << 1);
}

/*****************************************************
 * sendWord(short data)
 * Pushes an entire word into the shift register
 * After pushing 16 bits, latch the data
 *****************************************************/
void sendWord(short data) {
  int i;
  int j;
  //Drive CS to 0
  LOAD_0();
  for(i=15;i>=0;i--) {
    j = (data >> i) & 1;
    sendBit(j);
  }
  //CS back to 1; on the positive rising edge the
  //MAX7219 will latch the data in the shift registers
  LOAD_1();
}

/*****************************************************
 * sendChars(char add, char reg)
 * Takes two chars, and pushes both of them, pushing
 * the entire word into the shift register
 * After the 16 bits, latches data
 *****************************************************/
void sendChars(char add, char reg) {
  int i;
  int j;
  //Drive CS to 0
  LOAD_0();
  for(i=7;i>=0;i--) {
    j = (add >> i) & 1;
    sendBit(j);
  }
  for(i=7;i>=0;i--) {
    j = (reg >> i) & 1;
    sendBit(j);
  }
  //CS back to 1; on the positive rising edge the
  //MAX7219 will latch the data in the shift registers
  LOAD_1();
}

//clears the entire screen
void clear() {
  sendWord(0x0100);
  sendWord(0x0200);
  sendWord(0x0300);
  sendWord(0x0400);
  sendWord(0x0500);
  sendWord(0x0600);
  sendWord(0x0700);
  sendWord(0x0800);
}

//lights the entire screen
void lightAll() {
  sendWord(0x01FF);
  sendWord(0x02FF);
  sendWord(0x03FF);
  sendWord(0x04FF);
  sendWord(0x05FF);
  sendWord(0x06FF);
  sendWord(0x07FF);
  sendWord(0x08FF);
}

/*****************************************************
 * setup(void)
 * Setup hardware -- Initialize port D pins as GPIO
 * output pins
 * Port D Pin 0 - CS
 * Port D Pin 1 - Clock
 * Port D Pin 2 - Data
 * Initialize our CS and Clock signals to low
 *****************************************************/
void setup(void)
{
  //clock port D
  SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;

  //set up the pins of port D as GPIO
  PORTD->PCR[0] = PORT_PCR_MUX(001);
  PORTD->PCR[1] = PORT_PCR_MUX(001);
  PORTD->PCR[2] = PORT_PCR_MUX(001);
   
  //enable the pins as output
  PTD->PDDR |= (1 << 0);
  PTD->PDDR |= (1 << 1);
  PTD->PDDR |= (1 << 2);

  LOAD_0();
  CLK_0();
}

//busy wait function
void delay(int ms)
{
  int i;
  for(i=0; i < ms * 20971; i++);
}

/*******************************************************
 * poll(int ms)
 * check if there is new bluetooth data in our register
 * and if there is, check if the character is in the
 * ASCII range for our supported characters. If it is
 * then push the character into our buffer for display,
 * otherwise it must be speed. Updates speed accordingly
 *******************************************************/
void poll(int ms) {
  int i;
  //During our usual delay, poll instead of busy-wait
  for(i=0; i < ms * 200; i++) {
    if(uart_getchar()) {
      if(UART4_D >= 20) {
        add_char(UART4_D);
      }
      else {
        speed = 15 - UART4_D;
      }
    }
  }
}

/* Add to the tail of queue */
void add_to_tail(queue** head_ref, queue* p) {
  /* Get pointer to the current head node */
  queue* current = *head_ref;
  
  /* If queue is currently empty, replace it with the new node */
  if (current == NULL) { *head_ref = p; }
  
  /* Otherwise, find the end of the list and append the new node */
  else {
    while (current->next != NULL) { current = current->next; }
    /* At this point, current points to the last node in the list */
    current->next = p;
  }
}

/* Remove and return (pop) head of queue */
queue* take_from_head(queue** head_ref) {
  /* We want to return the current head */
  queue* result = *head_ref;
  
  /* Remove the head, unless the queue is empty */
  if (result != NULL) {
    *head_ref = result->next; /* New head is the next in queue */
    result->next = NULL;    /* Removed no longer points to queue */
    return result;   /* return character */
  }
  return NULL;
}

/* Add to the tail of queue */
void buffer_extra(backlog** head_ref, backlog* p) {
  /* Get pointer to the current head node */
  backlog* current = *head_ref;
  
  /* If queue is currently empty, replace it with the new node */
  if (current == NULL) { *head_ref = p; }
  
  /* Otherwise, find the end of the list and append the new node */
  else {
    while (current->next != NULL) { current = current->next; }
    /* At this point, current points to the last node in the list */
    current->next = p;
  }
}

/* Remove and return (pop) head of queue */
backlog* take_from_extra(backlog** head_ref) {
  /* We want to return the current head */
  backlog* result = *head_ref;
  
  /* Remove the head, unless the queue is empty */
  if (result != NULL) {
    *head_ref = result->next; /* New head is the next in queue */
    result->next = NULL;    /* Removed no longer points to queue */
    return result;   /* return character */
  }
  return NULL;
}

/* Display a screen based on the column input from enocding matrix */
void display(int col) {
  int i;
  for(i=0; i < 8; i++) {
    sendChars(i+1, encoding[col][i]);
  }
}

/***************************************************
 * add_char(int col)
 * Add a character to our queue to be displayed
 * eventually
 ***************************************************/
void add_char(int col) {
  int i;
  //if our buffer queue is full we need to backlog this
  //character
  if(buffer_size >= 8 || extra != NULL) {
    backlog* extra_node = malloc(sizeof(backlog));
    if(extra_node == NULL) {
      return;
    }
    extra_node->col = col;
    extra_node->next = NULL;
    buffer_extra(&extra, extra_node);
    return;
  }
  
  //otherwise we should try adding to to current or buffer
  for(i=0; i<8; i++) {
    //create and initialize our new queue node
    queue* new_node = malloc(sizeof(queue));
    if(new_node == NULL) {
      return;
    }
    
    new_node->character = encoding[col][i];
    new_node->next = NULL;
    
    //check if current queue has room
    if(current_size < 8) {
      add_to_tail(&current, new_node);
      current_size++;
    }
    //otherwise add to buffer queue
    else {
      add_to_tail(&buffer, new_node);
      buffer_size++;
    }
  }
}

/***************************************************
 * naive_add(int col)
 * Add a character to our queue to be displayed
 * Similar to add_char but this disregards buffer
 * size, adding it to the buffer queue without 
 * checking if it is full
 ***************************************************/
void naive_add(int col) {
  int i;
  
  for(i=0; i<8; i++) {
    //create and initialize our new queue node
    queue* new_node = malloc(sizeof(queue));
    if(new_node == NULL) {
      return;
    }
    
    new_node->character = encoding[col][i];
    new_node->next = NULL;
    
    //check if current queue has room
    if(current_size < 8) {
      add_to_tail(&current, new_node);
      current_size++;
    }
    //otherwise add to the vuffer queue
    else {
      add_to_tail(&buffer, new_node);
      buffer_size++;
    }
  }
}

/***************************************************
 * display_frame()
 * Displays what is in our current queue, then pops
 * the current adds the next in buffer queue to the
 * end of current, shifting the message over
 * If there's nothing to display just clear screen
 ***************************************************/
void display_frame() {
  int i = 0;
  queue* temp = current;
  for(i=0; i<8; i++) {
    if(temp != NULL) {
      //display the current queue column
      sendChars(i+1, temp->character);
      temp = temp->next;
    }
    //clear column if nothing left in current queue
    else {
      sendChars(i+1, 0x00);
    }
  }
}

/*********************************************************
 * scroll_display()
 * Displays frames forever, converting backlogged character
 * frames into 8 columns and adding them to the buffer
 * whenever necessary
 ********************************************************/
void scroll_display() {
  while(1) {
    display_frame();
    free(take_from_head(&current));
    current_size--;
    //if there's something in the buffer, push next one to current
    if(buffer != NULL) {
      add_to_tail(&current, take_from_head(&buffer));
      current_size++;
    }
    //otherwise nothing in buffer
    //check extra backlogged characters
    else if(extra != NULL) {
      //if there's something, convert the character and add to buffer
      backlog* temp = take_from_extra(&extra);
      naive_add(temp->col);
      free(temp);
      add_to_tail(&current, take_from_head(&buffer));
      current_size++;
    }
    //between frames, poll for more values
    poll(speed);
  }
}