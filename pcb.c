#include <stdlib.h>
#include "pcb.h"

//Create a new PCB with initial values
PCB *create_pcb(int pid, int start_index, int number_of_lines) {
    PCB *new_pcb = (PCB *) malloc(sizeof(PCB)); //malloc allocates enough memory to store 1 PCB struct
    //cast the return pointer to type PCB *
    //so now new_pcb points to a block of memory large enough to hold the PCB

    if (!new_pcb) {             //check if malloc failed
        return NULL;            //return NULL to indicate failure
    }
    //initialize PCB fields
    new_pcb->pid = pid;
    new_pcb->start_index = start_index;
    new_pcb->number_of_lines = number_of_lines;
    new_pcb->pc = 0;            //set to zero since execution starts at 1st line
    new_pcb->next = NULL;       //initally not linked to other PCB
    new_pcb->job_length_score = number_of_lines;        //in the beginning, job length score = number of lines of code in the script
    new_pcb->is_batch_script = 0;       //default set to false (0)

    return new_pcb;             //returns pointer to newly allocated PCB
}
