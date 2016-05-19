#include "MK64F12.h"
#include <stdlib.h>
#include "encoding.h"
#include "bluetooth2.h"

// - Mode Selection
#define no_decode 0x09
#define scan_limit 0x0B
#define shut_down 0x0C

// Wait function
#define DATA_0()  PTD->PCOR |= (1 << 2);
#define DATA_1()  PTD->PSOR |= (1 << 2);

#define LOAD_0()  PTD->PCOR |= (1 << 0);
#define LOAD_1()  PTD->PSOR |= (1 << 0);

#define CLK_0()   PTD->PCOR |= (1 << 1);
#define CLK_1()   PTD->PSOR |= (1 << 1);

typedef struct overflow_queue {
  int col;
  struct overflow_queue* next;
} backlog;

typedef struct display_queue {
  char character;
  struct display_queue* next;
} queue;

void setup(void);
void init(void);
void clear(void);
void sendBit(int);
void sendWord(short);
void sendChars(char, char);
void lightAll(void);
void display(int);
void delay_ms(int);

void add_char(int);
void display_frame();
void scroll_display(int);

//current queue of letters to show
queue* current = NULL;
int current_size = 0;

//buffer queue, max 16
queue* buffer = NULL;
int buffer_size = 0;

//unbounded backlog buffer
backlog* extra = NULL;

int main() {
  int i;
  //setup port pins as GPIO output
  setup();
  //configure the LED matrix
  init();
  //setup the bluetooth receiver
  UARTsetup();
  Pit_Setup();
/*
  add_char(32);
  add_char(72);
  add_char(69);
  add_char(76);
  add_char(76);
  add_char(79);
  add_char(32);
  add_char(87);
  add_char(79);
  add_char(82);
  add_char(76);
  add_char(68);
  add_char(33);
  for(i=0; i< 100; i++) {
    add_char(i);
  }
*/
  delay_ms(100);
  lightAll();
  delay_ms(100);
  clear();
  delay_ms(100);
  scroll_display(20);
  
  while(1);
}

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

void sendBit(int value){
  if(value == 1){
    DATA_1();
  }
  else{
    DATA_0();
  }
  //rising clock edge
  PTD->PSOR |= (1 << 1);
  //delay_ms(1);
  //clock to 0
  PTD->PCOR |= (1 << 1);
}

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

void delay_ms(int ms)
{
  int i;
  for(i=0; i < ms * 20971; i++);
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

void display(int col) {
  int i;
  for(i=0; i < 8; i++) {
    sendChars(i+1, encoding[col][i]);
  }
}

void add_char(int col) {
  int i;
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
    else {
      add_to_tail(&buffer, new_node);
      buffer_size++;
    }
  }
}

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
    else {
      add_to_tail(&buffer, new_node);
      buffer_size++;
    }
  }
}

void display_frame() {
  int i = 0;
  queue* temp = current;
  for(i=0; i<8; i++) {
    if(temp != NULL) {
      sendChars(i+1, temp->character);
      temp = temp->next;
    }
    else {
      sendChars(i+1, 0x00);
    }
  }
}

void scroll_display(int delay) {
  while(1) {
    display_frame();
    free(take_from_head(&current));
    current_size--;
    if(buffer != NULL) {
      add_to_tail(&current, take_from_head(&buffer));
      current_size++;
    }
    else if(extra != NULL) {
      naive_add(take_from_extra(&extra)->col);
      add_to_tail(&current, take_from_head(&buffer));
      current_size++;
    }
    delay_ms(delay);
  }
}