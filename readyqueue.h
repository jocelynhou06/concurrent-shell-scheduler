#ifndef READYQUEUE_H
#   define READYQUEUE_H

#   include "pcb.h"

//ready queue struct
typedef struct ReadyQueue {
    PCB *head;                  //pointer to 1st PCB in queue
    PCB *tail;                  //pointer to last PCB in queue
    int size;                   //number of PCBs in queue
} ReadyQueue;

ReadyQueue *create_queue();     //function that will create a new empty ready queue
void destroy_queue(ReadyQueue * queue); //function that will free memory for the queue
void enqueue(ReadyQueue * queue, PCB * process);        //fuction to add a PCB to tail of the queue
PCB *dequeue(ReadyQueue * queue);       //function to remove a PCB from the head of the queue
int is_empty(ReadyQueue * queue);       //functino to check if queue is empty
void enqueueAGING(ReadyQueue * queue, PCB * pcb);       //function to insert PCB back into queue correctly
void enqueueFront(ReadyQueue * queue, PCB * pcb);       //function for background mode, will insert batch script process at the front of queue

#endif
