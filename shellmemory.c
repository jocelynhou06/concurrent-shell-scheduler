#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

struct memory_struct {
    char *var;
    char *value;
};

struct memory_struct shellmemory[MEM_SIZE];

//Create global variable named shell_program_memory
//Initialize next_free field inside struct to 0
ProgramMemoryShared shell_program_memory = {.next_free = 0 };

// Helper functions
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i])
            matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else
        return 0;
}

// Shell memory functions

void mem_init() {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    return;
}

//get value based on input key
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            return strdup(shellmemory[i].value);
        }
    }
    return NULL;
}

//reserve block of lines in shared memory for script
int allocate_program_lines(int number_of_lines) {
    if (shell_program_memory.next_free + number_of_lines > MAX_PROGRAM_SIZE) {  //check that there's enough space in memory for new script
        return -1;              //if not enough space, return -1
    }
    int start_index = shell_program_memory.next_free;   //save starting index of allocated block
    shell_program_memory.next_free = shell_program_memory.next_free + number_of_lines;  //change next free index value
    return start_index;         //if all goes well, return starting index of the allocated block
}

//clear block of lines in shared memory
void free_program_lines(int start, int number_of_lines) {
    if (start < 0 || start + number_of_lines > MAX_PROGRAM_SIZE) {      //check starting index ins valid and block doesn't extend past end of memory
        return;
    }

    for (int i = start; i < start + number_of_lines; i++) {     //loop through all lines in the block
        for (int j = 0; j < MAX_PROGRAM_LINE_LENGTH; j++) {     //goes through each character on each line
            shell_program_memory.lines[i][0] = '\0';    //set character to null terminator, fully zero out each line
        }
    }

    if (start + number_of_lines == shell_program_memory.next_free) {    //if this block was at the end of shared memory, we can move back next_free
        shell_program_memory.next_free = start;
    }
}
