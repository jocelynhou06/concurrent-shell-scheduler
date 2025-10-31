#ifndef SCHEDULER_H
#   define SCHEDULER_H

#   include "pcb.h"
#   include "readyqueue.h"

extern ReadyQueue *global_queue;        //Declare a global ready queue, that'll be in scheduler.h

//function that will run all process in the given queue using FCFS
void FCFS(ReadyQueue * queue);

//function that will run all processes in the given queue using SJF
//we reorder queue having shortest job length pcb go first, and longest job length pcb go last, then call FCFS function
void SJF(ReadyQueue * queue);

//function that will run all processes in the given queue using RR
void RR(ReadyQueue * queue, int time_slice);

//function that will run all processes in the given queue using SJF with job aging
void AGING(ReadyQueue * queue);

//helper function for AGING
void age_queue(ReadyQueue * queue);     //function that will decrease a job's "job length score" by 1

#endif
