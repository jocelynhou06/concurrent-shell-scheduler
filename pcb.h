#ifndef PCB_H
#   define PCB_H

//PCB struct for a script process
typedef struct PCB {
    int pid;                    //each process has unique PID
    int start_index;            //Spot in shell memory where script instructions are loaded, keeping track of start position of script
    int number_of_lines;        //Keeping track of the length of the script
    int pc;                     //program counter, but really an index of the next instruction for an array of program lines
    int job_length_score;       //for AGING policy
    int is_batch_script;        //flag to signal whether PCB is for a batch script process
    struct PCB *next;           //pointer which will point to the next PCB in the ready queue
} PCB;

PCB *create_pcb(int pid, int start_index, int number_of_lines); // Function that'll create a new PCB
#endif
