//#define DEBUG 1

#ifdef DEBUG
#   define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#   define debug(...)
// NDEBUG disables asserts
#   define NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>              // tolower, isdigit
#include <dirent.h>             // scandir
#include <unistd.h>             // chdir
#include <sys/stat.h>           // mkdir
// for run:
#include <sys/types.h>          // pid_t
#include <sys/wait.h>           // waitpid

#include "shellmemory.h"
#include "shell.h"
#include "scheduler.h"          //for helper function used in source()

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int badcommandMkdir() {
    printf("Bad command: my_mkdir\n");
    return 4;
}

int badcommandCd() {
    printf("Bad command: my_cd\n");
    return 5;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int echo(char *tok);
int ls();
int my_mkdir(char *name);
int touch(char *path);
int cd(char *path);
int source(char *script);
int run(char *args[], int args_size);
int badcommandFileDoesNotExist();
int exec(char *args[], int arg_size);   //declare exec function to avoid compilation errors
PCB *create_batch_script_pcb(int pid, FILE * batchFile);        //declare function that will create PCB for batch script process

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    // these bits of debug output were very helpful for debugging
    // the changes we made to the parser!
    debug("#args: %d\n", args_size);
#ifdef DEBUG
    for (size_t i = 0; i < args_size; ++i) {
        debug("  %ld: %s\n", i, command_args[i]);
    }
#endif

    if (args_size < 1) {
        // This shouldn't be possible but we are defensive programmers.
        fprintf(stderr, "interpreter called with no words?\n");
        exit(1);
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1)
            return badcommand();
        return ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2)
            return badcommand();
        return touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return cd(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2)
            return badcommand();
        return run(&command_args[1], args_size - 1);

    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3 || args_size > 6) {   //exec + 1-3 programs + policy + background option, so check for 3-6 args
            return badcommand();
        }
        return exec(&command_args[1], args_size - 1);

    } else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    char *value = mem_get_value(var);
    if (value) {
        printf("%s\n", value);
        free(value);
    } else {
        printf("Variable does not exist\n");
    }
    return 0;
}

int echo(char *tok) {
    int must_free = 0;
    // is it a var?
    if (tok[0] == '$') {
        tok++;                  // advance pointer, so that tok is now the stuff after '$'
        tok = mem_get_value(tok);
        if (tok == NULL) {
            tok = "";           // must use empty string, can't pass NULL to printf
        } else {
            must_free = 1;
        }
    }

    printf("%s\n", tok);

    // memory management technically optional for this assignment
    if (must_free)
        free(tok);

    return 0;
}

// We can hide dotfiles in ls using either the filter operand to scandir,
// or by checking the first character ourselves when we go to print
// the names. That would work, and is less code, but this is more robust.
// And this is also better since it won't allocate extra dirents.
int ls_filter(const struct dirent *d) {
    if (d->d_name[0] == '.')
        return 0;
    return 1;
}

int ls_compare_char(char a, char b) {
    // assumption: a,b are both either digits or letters.
    // If this is not true, the characters will be effectively compared
    // as ASCII when we do the lower_a - lower_b fallback.

    // if both are digits, compare them
    if (isdigit(a) && isdigit(b)) {
        return a - b;
    }
    // if only a is a digit, then b isn't, so a wins.
    if (isdigit(a)) {
        return -1;
    }

    // lowercase both letters so we can compare their alphabetic position.
    char lower_a = tolower(a), lower_b = tolower(b);
    if (lower_a == lower_b) {
        // a and b are the same letter, possibly in different cases.
        // If they are really the same letter, this returns 0.
        // Otherwise, it's negative if A was capital,
        // and positive if B is capital.
        return a - b;
    }

    // Otherwise, compare their alphabetic position by comparing
    // them at a known case.
    return lower_a - lower_b;
}

int ls_compare_str(const char *a, const char *b) {
    // a simple strcmp implementation that uses ls_compare_char.
    // We only check if *a is zero, since if *b is zero earlier,
    // it would've been unequal to *a at that time and we would return.
    // If *b is zero at the same point or later than *a, we'll exit the
    // loop and return the correct value with the last comparison.

    while (*a != '\0') {
        int d = ls_compare_char(*a, *b);
        if (d != 0)
            return d;
        a++, b++;
    }
    return ls_compare_char(*a, *b);
}

int ls_compare(const struct dirent **a, const struct dirent **b) {
    return ls_compare_str((*a)->d_name, (*b)->d_name);
}

int ls() {
    // straight out of the man page examples for scandir
    // alphasort uses strcoll instead of strcmp,
    // so we have to implement our own comparator to match the ls spec.
    // Note that the test cases weren't very picky about the specified order,
    // so if you just used alphasort with scandir, you should have passed.
    // This was intentional on our part.
    struct dirent **namelist;
    int n;

    n = scandir(".", &namelist, NULL, ls_compare);
    if (n == -1) {
        // something is catastrophically wrong, just give up.
        perror("my_ls couldn't scan the directory");
        return 0;
    }

    for (size_t i = 0; i < n; ++i) {
        printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);

    return 0;
}

int str_isalphanum(char *name) {
    for (char c = *name; c != '\0'; c = *++name) {
        if (!(isdigit(c) || isalpha(c)))
            return 0;
    }
    return 1;
}

int my_mkdir(char *name) {
    int must_free = 0;

    debug("my_mkdir: ->%s<-\n", name);

    if (name[0] == '$') {
        ++name;
        // lookup name
        name = mem_get_value(name);
        debug("  lookup: %s\n", name ? name : "(NULL)");
        if (name) {
            // name exists, should free whatever we got
            must_free = 1;
        }
    }
    if (!name || !str_isalphanum(name)) {
        // either name doesn't exist, or isn't valid, error.
        if (must_free)
            free(name);
        return badcommandMkdir();
    }
    // at this point name is definitely OK

    // 0777 means "777 in octal," aka 511. This value means
    // "give the new folder all permissions that we can."
    int result = mkdir(name, 0777);

    if (result) {
        // description doesn't specify what to do in this case,
        // (including if the directory already exists)
        // so we just give an error message on stderr and ignore it.
        perror("Something went wrong in my_mkdir");
    }

    if (must_free)
        free(name);
    return 0;
}

int touch(char *path) {
    // we're told we can assume this.
    assert(str_isalphanum(path));
    // if things go wrong, just ignore it.
    FILE *f = fopen(path, "a");
    fclose(f);
    return 0;
}

int cd(char *path) {
    // we're told we can assume this.
    assert(str_isalphanum(path));

    int result = chdir(path);
    if (result) {
        // chdir can fail for several reasons, but the only one we need
        // to handle here for the spec is the ENOENT reason,
        // aka Error NO ENTry -- the directory doesn't exist.
        // Since that's the only one we have to handle, we'll just assume
        // that that's what happened.
        // Alternatively, you can check if the directory exists
        // explicitly first using `stat`. However it is often better to
        // simply try to use a filesystem resource and then recover when
        // you can't, rather than trying to validate first. If you validate
        // first while two users are on the system, there's a race condition!
        return badcommandCd();
    }
    return 0;
}

int source(char *script) {
    FILE *p = fopen(script, "rt");      // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    char lines[MAX_PROGRAM_SIZE][MAX_PROGRAM_LINE_LENGTH];      //temp buffer for reading lines
    int line_count = 0;         //initialize line_count for one program

    while (fgets(lines[line_count], MAX_PROGRAM_LINE_LENGTH, p) != NULL) {      //read through all lines of script
        lines[line_count][strcspn(lines[line_count], "\r\n")] = '\0';   //on the line read from fgets
        //find the index of the first return or newline char
        //and replace it with null terminator char
        line_count++;           //increment for correct indexing

        if (line_count >= MAX_PROGRAM_SIZE) {   //if shared memory limit is reached
            break;              //stop
        }
    }
    fclose(p);

    int start_index = allocate_program_lines(line_count);       //call function to allocate space in shell memory
    if (start_index < 0) {      //if allocation fails, print error msg
        printf("error: not enough memory for script\n");
        return 1;
    }

    for (int i = 0; i < line_count; i++) {      //copy script into shared shell memory
        //copy line i of temporary buffer, lines, to index [start_index + i] of shared shell memory 
        strcpy(shell_program_memory.lines[start_index + i], lines[i]);
    }

    static int next_pid = 1;    //for unique PIDS
    PCB *pcb = create_pcb(next_pid++, start_index, line_count); //create a new pcb with the right inputs

    //if global queue doesn't exist yet
    if (!global_queue) {
        global_queue = create_queue();  //create new empty queue
    }
    enqueue(global_queue, pcb); //add newly made pcb to queue

    FCFS(global_queue);         //execute all processes in queue through FCFS
    destroy_queue(global_queue);        //free queue struct
    global_queue = NULL;

    return 0;
}

int run(char *args[], int arg_size) {
    // copy the args into a new NULL-terminated array.
    char **adj_args = calloc(arg_size + 1, sizeof(char *));
    for (int i = 0; i < arg_size; ++i) {
        adj_args[i] = args[i];
    }

    // always flush output streams before forking.
    fflush(stdout);
    // attempt to fork the shell
    pid_t pid = fork();
    if (pid < 0) {
        // fork failed. Report the error and move on.
        perror("fork() failed");
        return 1;
    } else if (pid == 0) {
        // we are the new child process.
        execvp(adj_args[0], adj_args);
        perror("exec failed");
        // The parent and child are sharing stdin, and according to
        // a part of the glibc documentation that you are **not**
        // expected to know for this course, a shared input handle
        // should be fflushed (if it is needed) or closed
        // (if it is not). Handling this exec error case is not even
        // necessary, but let's do it right.
        // (Failure to do this can result in the parent process
        // reading the remaining input twice in batch mode.)
        fclose(stdin);
        exit(1);
    } else {
        // we are the parent process.
        waitpid(pid, NULL, 0);
    }

    return 0;
}

//order of exec function
//1. check if background mode is enabled
//2. check for valid policy
//3. check each exec arguement is the name of a DIFFERENT script filename
//4. check the file does exist
//5. load code into shell memory, making sure there's enough space
//6. create global queue and pcbs, enqueueing correctly
//7. running the correct scheduling policy
//8. clean up of queue, clean up of pcbs and code in shell memory is handled in other functions
int exec(char *args[], int arg_size) {
    int background = 0;         //set background flag to false (0) for now
    if (strcmp(args[arg_size - 1], "#") == 0) { //check if # option is wanted
        background = 1;         //if yes, set flag to true (1)
        arg_size--;             //decrement arg_size to exclude "#" from more processing
    }


    char *policy = args[arg_size - 1];  //array starts at 0, so correctly index to policy by arg_size - 1
    //check for a valid policy out of 5 values
    if (!
        (strcmp(policy, "FCFS") == 0 || strcmp(policy, "SJF") == 0
         || strcmp(policy, "RR") == 0 || strcmp(policy, "AGING") == 0
         || strcmp(policy, "RR30") == 0)) {
        printf("Bad command: wrong scheduling policy, error!\n");       //outputs error msg
        return 1;               //exec terminates
    }

    int number_of_programs = arg_size - 1;      //how many programs to execute, after decrementing to account for policy
    //check for identical exec arguments
    for (int i = 0; i < number_of_programs; i++) {      //starting from 1st program name
        //if only 1 program, condition not met, 2nd for loop not entered
        for (int j = i + 1; j < number_of_programs; j++) {      //compare it with 2nd program name, and 3rd if passes the condition
            if (strcmp(args[i], args[j]) == 0) {        //if identical arguments
                printf("Error! 2 exec arguments are identical\n");      //print error
                return 1;       //exec terminates
            }
        }
    }

    //temp storage
    int start_indexes[number_of_programs];      //store where each program's 1st line is in shell memory
    int line_counts[number_of_programs];        //stores how many lines each program has
    char lines[MAX_PROGRAM_SIZE][MAX_PROGRAM_LINE_LENGTH];      //temp buffer for each program from its file
    int line_count_total = 0;   //counts total lines loaded

    //for loop to load all programs into memory
    for (int i = 0; i < number_of_programs; i++) {      //from first program to last program
        FILE *p = fopen(args[i], "rt"); //opens the file
        if (p == NULL) {        //if file can't be opened
            for (int j = 0; j < i; j++) {       //free up all program loaded in before current file
                free_program_lines(start_indexes[j], line_counts[j]);
            }
            return badcommandFileDoesNotExist();        //return immedietaly after
        }

        int line_count = 0;
        while (fgets(lines[line_count], MAX_PROGRAM_LINE_LENGTH, p) != NULL) {  //read through all lines of script
            lines[line_count][strcspn(lines[line_count], "\r\n")] = '\0';       //on the line read from fgets
            //find the index of the first return or newline char
            //and replace it with null terminator char
            line_count++;       //increment for correct indexing

            if (line_count >= MAX_PROGRAM_SIZE) {       //if shared memory limit is reached
                break;          //stop
            }
        }
        fclose(p);

        int start_index = allocate_program_lines(line_count);   //call function to allocate space in shell memory
        if (start_index < 0) {  //if allocation fails, print error msg
            printf("error: not enough memory for script\n");
            for (int j = 0; j < i; j++) {       //free up all program loaded in before current file
                free_program_lines(start_indexes[j], line_counts[j]);
            }
            return 1;
        }

        for (int j = 0; j < line_count; j++) {  //copy script into shared shell memory
            //copy line j of temporary buffer, lines, to index [start_index + j] of shared shell memory 
            strcpy(shell_program_memory.lines[start_index + j], lines[j]);
        }

        start_indexes[i] = start_index; //store where each program's first line is in shell memory in correct index 
        line_counts[i] = line_count;    //store each program's line count in correct index
        line_count_total += line_count; //update total number of lines
    }

    //if global queue doesn't exist yet
    if (!global_queue) {
        global_queue = create_queue();  //create new empty queue
    }

    //in source code, if(!global_queue) was after the creation of pcb
    //it must be switched now, or else pointers to pcb will be lost
    static int next_pid = 1;    //for unique PIDS

    if (background) {           //background mode # is on
        PCB *batch_script_pcb = create_batch_script_pcb(next_pid++, stdin);
        if (batch_script_pcb == NULL) { //if batch script pcb wasn't properly created
            for (int i = 0; i < number_of_programs; i++) {      //free up all program loaded in
                free_program_lines(start_indexes[i], line_counts[i]);
            }
        } else if (batch_script_pcb != NULL) {  //if successfully created batch script pcb for remaining lines of batch script process
            enqueueFront(global_queue, batch_script_pcb);       //new special enqueue, that will put batch pcb at the front, to ensure it'll run first
        }
    }

    for (int i = 0; i < number_of_programs; i++) {      //create a pcb for each program and enqueue it into queue
        PCB *pcb = create_pcb(next_pid++, start_indexes[i], line_counts[i]);    //create a new pcb with the right inputs
        enqueue(global_queue, pcb);     //add newly made pcb to queue
    }

    //we have to ensure that batch script process runs FIRST
    //for FCFS, RR, RR30 the order of queue is not changed upon calling their respective functions
    //however SJF and AGING reorder according to job length score
    //to adjust for this, we will save the batch script process PCB that is currently the head of the queue
    //reorder according to job length score, and then reattach batch script process PCB to the head of queue
    //to ensure batch script process will run first regardless of scheduling policy
    if (strcmp(policy, "FCFS") == 0) {
        FCFS(global_queue);     //execute all processes in queue through FCFS
    } else if (strcmp(policy, "SJF") == 0) {
        SJF(global_queue);      //execute all processes in queue through SJF
    } else if (strcmp(policy, "RR") == 0) {
        RR(global_queue, 2);    //execute all processes in queue through round robin    
    } else if (strcmp(policy, "RR30") == 0) {
        RR(global_queue, 30);   //execute all processes in queue through round robin, time slice = 30
    } else if (strcmp(policy, "AGING") == 0) {
        AGING(global_queue);    //execute all processes in queue with SJF with job Aging
    }

    destroy_queue(global_queue);        //free queue struct
    global_queue = NULL;

    return 0;
}

//helper function to create pcb for batch script process
PCB *create_batch_script_pcb(int pid, FILE *batchFile) {
    char lines[MAX_PROGRAM_SIZE][MAX_PROGRAM_LINE_LENGTH];      //temp buffer for reading lines
    int line_count = 0;         //initialize line_count for one program

    while (fgets(lines[line_count], MAX_PROGRAM_LINE_LENGTH, batchFile) != NULL) {      //read through all lines of batch script
        lines[line_count][strcspn(lines[line_count], "\r\n")] = '\0';   //on the line read from fgets
        //find the index of the first return or newline char
        //and replace it with null terminator char
        line_count++;           //increment for correct indexing

        if (line_count >= MAX_PROGRAM_SIZE) {   //if shared memory limit is reached
            break;              //stop
        }
    }

    if (line_count == 0) {      //if there was nothing else after
        return NULL;            //no need to create batch script process PCB
    }

    int start_index = allocate_program_lines(line_count);       //call function to allocate space in shell memory
    if (start_index < 0) {      //if allocation fails, print error msg
        printf("error: not enough memory for script\n");
        return NULL;            //return NULL, don't create PCB for batch script process
    }

    for (int i = 0; i < line_count; i++) {      //copy script into shared shell memory
        //copy line j of temporary buffer, lines, to index [start_index + i] of shared shell memory 
        strcpy(shell_program_memory.lines[start_index + i], lines[i]);
    }

    PCB *pcb = create_pcb(pid, start_index, line_count);        //create a PCB
    pcb->is_batch_script = 1;   //set priority flag to true (1)

    return pcb;
}
