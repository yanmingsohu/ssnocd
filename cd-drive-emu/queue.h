#ifndef _H_CDEMU_QUEUE
#define _H_CDEMU_QUEUE
#include "stm32f4xx.h"

#define QUEUE_BUFF_SIZE  512


typedef  struct {
  uint8_t  buffer[QUEUE_BUFF_SIZE];
  uint32_t wp;
  uint32_t rp;
} Queue;


void write_queue(Queue *q, uint8_t data);
uint8_t read_queue(Queue *q);
uint8_t has_data(Queue *q);


#endif