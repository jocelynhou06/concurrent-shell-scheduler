#define MEM_SIZE 1000
#define MAX_PROGRAM_SIZE 1000   //shell memory can hold 1000 lines max
#define MAX_PROGRAM_LINE_LENGTH 100     //each command(line) in script limited to 100 characters
void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);

//memory structure for storing program lines
typedef struct ProgramMemoryShared {
    char lines[MAX_PROGRAM_SIZE][MAX_PROGRAM_LINE_LENGTH];      // 2D array. Each row is a line of a script. 100 characters in each row.
    int next_free;              // Index of the next free line in memory. Allocator will use it to assign new lines to programs.
} ProgramMemoryShared;

extern ProgramMemoryShared shell_program_memory;        //Declare a global shared memory variable, that'll exist in shellmemory.c

int allocate_program_lines(int number_of_lines);        //Function that will reserve a block of lines in shared memory for a new script
void free_program_lines(int start, int number_of_lines);        //Free previously allocated block of program lines in shared memory
