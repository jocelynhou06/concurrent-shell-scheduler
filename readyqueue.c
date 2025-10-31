#include <stdlib.h>
#include "readyqueue.h"

//create a new empty ready queue with initial values
ReadyQueue *create_queue() {
    ReadyQueue *queue = (ReadyQueue *) malloc(sizeof(ReadyQueue));      //malloc allocates enough memory to store 1 ReadyQueue struct
    //cast the return pointer to type ReadyQueue *
    //so now queue points to a block of memory large enough to hold the queue struct

    if (!queue) {               //check if malloc failed
        return NULL;            //return NULL to indicate failure
    }
    //initialize queue fields
    queue->head = NULL;         //no PCB at the head
    queue->tail = NULL;         //no PCB at the tail
    queue->size = 0;            //empty queue initially
    return queue;               //returns pointer to newly created empty queue
}

//free memory allocated for the queue struct itself
void destroy_queue(ReadyQueue *queue) {
    free(queue);
}

//add a PCB to tail of the queue
void enqueue(ReadyQueue *queue, PCB *process) {
    process->next = NULL;       //process will be last in queue so set NEXT to null

    if (!queue->head) {         //if queue is empty
        //let head and tail both point to PCB
        queue->head = process;
        queue->tail = process;
    } else {                    //queue is not empty
        queue->tail->next = process;    //link current tail to new PCB
        queue->tail = process;  //update tail pointer
    }

    queue->size++;              //increment size of queue to keep track of number of PCBs in queue
}

//remove a PCB from the head of the queue
PCB *dequeue(ReadyQueue *queue) {
    if (!queue->head) {         //if queue is empty
        return NULL;
    }

    PCB *process = queue->head; //save the head(first PCB in queue)
    queue->head = queue->head->next;    //move head to next PCB in queue

    if (!queue->head) {         //if the queue is NOW empty
        queue->tail = NULL;     //set tail to NULL because no more PCBs
    }

    process->next = NULL;       //remove the process completely from the queue
    queue->size--;              //decrement queue size
    return process;             //return the dequeued PCB we saved earlier
}

//check if queue is empty
int is_empty(ReadyQueue *queue) {
    return queue->size == 0;
    //return 1(true) if the queue has no PCBs
    //return 0(false) if the queue has 1 or more PCBs
}

//Works like an insertion sort, adding PCB to correct part of queue
void enqueueAGING(ReadyQueue *queue, PCB *pcb) {
    //if queue is empty or the dequeued job has a score <= the head of the queue, insert dequeued job into the front
    if (!queue->head
        || (pcb->job_length_score <= queue->head->job_length_score)) {
        pcb->next = queue->head;        //have dequeued PCB's next pointer point to the current head
        queue->head = pcb;      //dequeued PCB becomes new head
        if (!queue->tail) {     // if queue was orginally empty, aka no tail
            queue->tail = pcb;  // update tail
        }
        queue->size++;          //increment size of queue to keep track of number of PCBs in queue
        return;
    }
    //if not, we must find where to insert dequeued PCB
    PCB *current = queue->head; //get the head of queue
    while (current->next && current->next->job_length_score < pcb->job_length_score) {  //loop through queue until we reach the end
        //or the current job's score is less than dequeued job's score
        current = current->next;        //move to next node in queue
    }

    pcb->next = current->next;  //set dequeued PCB's next point to the PCB that comes after current
    current->next = pcb;        //links current to the dequeued PCB, finishing insrtion

    if (!pcb->next) {           //if pcb inserted at the end, aka no node after it
        queue->tail = pcb;      //update tail
    }
    queue->size++;              //increment size of queue to keep track of number of PCBs in queue
    return;
}

//will insert batch script pcb at the front of queue
void enqueueFront(ReadyQueue *queue, PCB *pcb) {
    if (queue->head == NULL) {  //if queue is empty, use regular enqueue
        //let head and tail both point to PCB
        queue->head = pcb;
        queue->tail = pcb;
        pcb->next = NULL;       //only one node in queue, so no next node
    } else {
        pcb->next = queue->head;        //link pcb's next pointer to current head
        queue->head = pcb;      //set new head to be pcb
    }
    queue->size++;              //increment size of queue to keep track of number of PCBs in queue
    return;
}
