#include <stdio.h>
#include <stdlib.h>
#include "pcb.h"
#include "readyqueue.h"
#include "scheduler.h"
#include "shellmemory.h"
#include "shell.h"

//Define global queue
ReadyQueue *global_queue = NULL;

//run all processes in queue using FCFS
void FCFS(ReadyQueue *queue) {
    while (!is_empty(queue)) {  //runs until queue is empty
        PCB *current = dequeue(queue);  //gets next process in queue to execute

        while (current->pc < current->number_of_lines) {        //keep looping until all instructions of current process are accounted for
            parseInput(shell_program_memory.lines[current->start_index + current->pc]); //sends current instruction to parser
            current->pc++;      //increment program counter
        }

        //Clean-up
        free_program_lines(current->start_index, current->number_of_lines);     //remove SCRIPT source code from shell memory
        free(current);          //free the PCB
    }
}

//run all processes in queue using SJF
//pass in number of programs we have(equal to # of PCBs we have from interpreter)
void SJF(ReadyQueue *queue) {
    //reorder the queue according to shortest job length.
    if (is_empty(queue)) {      //if queue is empty
        return;
    }
    //to account for batch script process PCBs
    PCB *batch_pcb = NULL;      //declare a NULL PCB pointer
    if (queue->head->is_batch_script) { //if the head of queue has batch script process priority flag set to true (1)
        batch_pcb = queue->head;        //save it
        queue->head = batch_pcb->next;  // temporarily remove it from the queue
        batch_pcb->next = NULL; //detach from queue
    }
    //since we only have a max of 3 programs, I just implemented a quick bubble sort to reorder by shortest job length
    int swapped;                //this will keep track of whether any nodes were swapped during a pass
    //bubble sort will keep interating over the list until a full pass is made without swaps
    do {
        swapped = 0;            //currently, no swaps made
        PCB *current = queue->head;     //we start at head of the queue
        PCB *prev = NULL;       //will track node before current

        while (current && current->next) {
            PCB *next_node = current->next;     //compare current node with its next node
            if (current->number_of_lines > next_node->number_of_lines) {        //if node after current node has shorter job length, we swap!
                if (prev) {     //not at the head of the queue
                    prev->next = next_node;     //if prev exists, link it to next_node beccause next_node will move in front of current
                    //example 1: A-prev B-current C-next D-next_next, and we swap B and C
                    //A.next points to C
                } else {        //if at the head of the queue
                    queue->head = next_node;
                    //example 2: A-prev B-current C-next, and we swap A and B
                    //head points to B
                }
                current->next = next_node->next;        //skip next_node, so current points to the node after next_node
                next_node->next = current;      //complete the swap, next_node now points to current
                //example 1: B-current
                //B.next = D
                //C.next = B
                //A -> C -> B -> D

                //example 2: A-current
                //current.next = C
                //B.next = A
                //B -> A -> C

                swapped = 1;    //signal swap happens, so another pass is needed
                prev = next_node;       //move prev to next_node because we need the node before new current for the next comparison
            } else {            //no need to swap
                prev = current; //so update previous node
                current = current->next;        //move on to the next node
            }
        }
    } while (swapped);          //keep going until no swaps made in a pass

    //to account for batch script process PCBs
    if (batch_pcb) {            //if there is a priority batch script process PCB
        batch_pcb->next = queue->head;  //reattach back to queue
        queue->head = batch_pcb;        //set batch PCB as head of queue
    }
    //then run FCFS because both are non preemptive policies
    FCFS(queue);
}

//run all processes in queue with Round Robin policy
void RR(ReadyQueue *queue, int time_slice) {
    while (!is_empty(queue)) {  //runs until queue is empty
        PCB *current = dequeue(queue);

        int instructions_left_to_run = time_slice;      //define time slice
        //keep looping until all instructions of current process are accounted for, or timer is up (2 instructions executed)
        while (instructions_left_to_run > 0
               && current->pc < current->number_of_lines) {
            parseInput(shell_program_memory.lines[current->start_index + current->pc]); //sends current instruction to parser
            current->pc++;      //increment program counter
            instructions_left_to_run--;
        }

        if (current->pc >= current->number_of_lines) {  //process finished!
            //Clean-up
            free_program_lines(current->start_index, current->number_of_lines); //remove SCRIPT source code from shell memory
            free(current);      //free the PCB
        } else {                //process not finished
            enqueue(queue, current);    //add it to back of queue
        }
    }
}

//run all processes in queue with SJF Aging policy
void AGING(ReadyQueue *queue) {
    //reorder the queue according to shortest job length.
    if (is_empty(queue)) {      //if queue is empty
        return;
    }
    //to account for batch script process PCBs
    PCB *batch_pcb = NULL;      //declare a NULL PCB pointer
    if (queue->head->is_batch_script) { //if the head of queue has batch script process priority
        batch_pcb = queue->head;        //save it
        queue->head = batch_pcb->next;  // temporarily remove it from the queue
        batch_pcb->next = NULL; //detach from queue
    }
    //since we only have a max of 3 programs, I just implemented a quick bubble sort to reorder by shortest job length
    int swapped;                //this will keep track of whether any nodes were swapped during a pass
    //bubble sort will keep interating over the list until a full pass is made without swaps
    do {
        swapped = 0;            //currently, no swaps made
        PCB *current = queue->head;     //we start at head of the queue
        PCB *prev = NULL;       //will track node before current

        while (current && current->next) {
            PCB *next_node = current->next;     //compare current node with its next node
            if (current->number_of_lines > next_node->number_of_lines) {        //if node after current node has shorter job length, we swap!
                if (prev) {     //not at the head of the queue
                    prev->next = next_node;     //if prev exists, link it to next_node beccause next_node will move in front of current
                    //example 1: A-prev B-current C-next D-next_next, and we swap B and C
                    //A.next points to C
                } else {        //if at the head of the queue
                    queue->head = next_node;
                    //example 2: A-prev B-current C-next, and we swap A and B
                    //head points to B
                }
                current->next = next_node->next;        //skip next_node, so current points to the node after next_node
                next_node->next = current;      //complete the swap, next_node now points to current
                //example 1: B-current
                //B.next = D
                //C.next = B
                //A -> C -> B -> D

                //example 2: A-current
                //current.next = C
                //B.next = A
                //B -> A -> C

                swapped = 1;    //signal swap happens, so another pass is needed
                prev = next_node;       //move prev to next_node because we need the node before new current for the next comparison
            } else {            //no need to swap
                prev = current; //so update previous node
                current = current->next;        //move on to the next node
            }
        }
    } while (swapped);          //keep going until no swaps made in a pass

    //to account for batch script process PCBs
    if (batch_pcb) {            //if there is a priority batch script process PCB
        batch_pcb->next = queue->head;  //reattach back to queue
        queue->head = batch_pcb;        //set batch PCB as head of queue
    }
    //now we start on the SJF with Aging
    while (!is_empty(queue)) {  //runs until queue is empty
        PCB *current = dequeue(queue);  //takes head process

        if (current->pc < current->number_of_lines) {   //if instructions haven't been execute, enter if statement
            parseInput(shell_program_memory.lines[current->start_index + current->pc]); //sends current instruction to parser
            current->pc++;      //increment program counter
        }

        if (!is_empty(queue)) { //if queue is not empty
            age_queue(queue);   //age all other processes in queue
        }


        if (current->pc >= current->number_of_lines) {  //process finished!
            //Clean-up
            free_program_lines(current->start_index, current->number_of_lines); //remove SCRIPT source code from shell memory
            free(current);      //free the PCB
        } else {                //process not finished
            enqueueAGING(queue, current);       //reinsert dequeued PCB correctly
        }
    }
}

//After each instruction, all jobs in the queue get aged
void age_queue(ReadyQueue *queue) {
    PCB *current = queue->head; //get the head of queue
    while (current) {           //while loop keeps going until we reach end of the loop
        if (current->job_length_score > 0) {    //if job length score is greater than 0
            current->job_length_score--;        //decreament job length score
        }
        current = current->next;        //move on to the next node in the queue
    }
}
