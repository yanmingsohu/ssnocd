#include "queue.h"


void write_queue(Queue *q, uint8_t data) {
  q->buffer[q->wp] = data;
  if (++q->wp >= QUEUE_BUFF_SIZE) {
    q->wp = 0;
  }
}


uint8_t read_queue(Queue *q) {
  uint8_t d = q->buffer[q->rp];
  if (++q->rp >= QUEUE_BUFF_SIZE) {
    q->rp = 0;
  }
  return d;
}


uint8_t has_data(Queue *q) {
  return q->rp != q->wp;
}