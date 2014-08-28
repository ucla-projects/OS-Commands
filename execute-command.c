// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"      // allows use of checking memory allocation

#include <error.h>
#include <stdlib.h>     // allows use of exit() function
#include <sys/types.h>  // allows use of pid_t for parent process ID
#include <unistd.h>     // allows use of pipe()
#include <stdio.h>      // allows use of fprintf
#include <sys/wait.h>   // allows use of waitpid(pid_t pid, int *status, int options) function
#include <sys/stat.h>   // open(const char *pathname, int flags, mode_t mode)
#include <fcntl.h>
#include <stdbool.h>    // allows use of boolean expressions
#include <string.h>     // allows use of strcmp

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    pid_t child;
    int pipefd[2];
    
    if (c->type == AND_COMMAND) {
        execute_command(c->u.command[0], time_travel);
        c->status = c->u.command[0]->status;
        if (c->status == 0) {
            execute_command(c->u.command[1], time_travel);
            c->status = c->u.command[1]->status;
        }
    } else if (c->type == SEQUENCE_COMMAND) {
        execute_command(c->u.command[0], time_travel);
        execute_command(c->u.command[1], time_travel);
        c->status = c->u.command[1]->status;
    } else if (c->type == OR_COMMAND) {
        execute_command(c->u.command[0], time_travel);
        c->status = c->u.command[0]->status;
        if (c->status != 0) {
            execute_command(c->u.command[1], time_travel);
            c->status = c->u.command[1]->status;
        }
    } else if (c->type == PIPE_COMMAND) {
        int temp = pipe(pipefd);
        if (temp == -1) {
            fprintf(stderr, "Error in creating pipe");
        }
        child = fork();
        if (child > 0) {
            int _temp;
            pid_t parentOfChild = fork();
            if (parentOfChild > 0) {
                close(pipefd[0]);
                close(pipefd[1]);
                pid_t val = waitpid(child, &_temp, 0); // wait for any child process whose process group ID is equal to that of the calling process
                if (val == child) {
                    c->status = _temp;
                    waitpid(parentOfChild, &_temp, 0);
                } else if (val == parentOfChild) {
                    waitpid(child, &_temp, 0);
                    c->status = _temp;
                }
            } else if (parentOfChild == 0) {
                close(pipefd[0]);
                int new_val = dup2(pipefd[1], 1);
                if (new_val == -1) {
                    fprintf(stderr, "Error in duplicating from pipe file descriptor");
                }
                else {
                    execute_command(c->u.command[0], time_travel);
                }
            } else {
                fprintf(stderr, "Error in using fork in pipe command");
            }
        } else if (child == 0) {
            close(pipefd[1]);
            int temp_1 = dup2(pipefd[0], 0);
            if (temp_1 == -1) {
                fprintf(stderr, "Error in writing to pipe");
            }
            execute_command(c->u.command[0], time_travel);
            c->status = c->u.command[0]->status;
        }
        else {
            fprintf(stderr, "Error in making child process");
        }
    } else if (c->type == SIMPLE_COMMAND) {
        child = fork();
        if (child == 0) {
            if (c->input != 0) {
                int num = open(c->input, O_RDONLY);
                if (num == -1)
                    // check if error occurred
                    fprintf(stderr, "Error in reading the input file");
                int num_t = dup2(num, 0);
                if (num_t == -1) fprintf(stderr, "Error with system call returning new descriptor");
                close(num);
            }
            if (c->output != 0) {
                int num = open(c->output, O_WRONLY|O_CREAT, 0666);
                if (num == -1) {
                    // check if error occurred
                    fprintf(stderr, "Error in opening output file");
                }
                int num_t = dup2(num, 1);
                if (num_t == -1) fprintf(stderr, "Error with system call returning new descriptor");
                close(num);
            }
            
            int num = execvp(c->u.word[0], c->u.word);
            if (num == -1) fprintf(stderr, "Error in executing command"); // error in executing command
        } else if (child > 0) {
            int _temp;
            waitpid(child, &_temp, 0); // wait for any child process whose process group ID is equal to that of the calling process
            c->status = _temp;
        } else {
            fprintf(stderr, "Error in making child process");
        }
    } else if (c->type == SUBSHELL_COMMAND) {
        execute_command(c->u.subshell_command, time_travel);
        c->status = c->u.subshell_command->status;
    } else {
        fprintf(stderr, "Error in identifying command");
    }
}

// allows for extra parallelism
// execute the script faster than the standard shell
void timeTravel(command_stream_t c_stream) {
    while (c_stream) {
        command_stream_t current_command_list, command_list, previous_command_list;
        current_command_list = command_list = previous_command_list;
        int temp = 0;
        command_stream_t val = c_stream;
        while (val) {
            if (command_list == NULL) {
                // list is currently empty
                // the command list will contain commands
                temp = 1;
                current_command_list = val;
                // add command to currently empty list
                command_list = val;
                val = val->nextStream;
                c_stream = val;
            } else {
                bool dependency = false;
                command_stream_t temp_stream = command_list;
                // find if any of the commands are dependent on each other
                while (temp_stream) {
                    if (temp_stream->com_file_dependency == NULL || val->com_file_dependency == NULL) dependency = 0;
                    // no dependency if both the dependencies are null
                    else {
                        IOfiles_t val_t = val->com_file_dependency;
                        IOfiles_t temp_stream_t = NULL;
                        // continue checking dependencies while there exists commands in the stream
                        while (val_t) {
                            temp_stream_t = temp_stream->com_file_dependency;
                            while (temp_stream_t) {
                                int x = strcmp(val_t->IOfile,temp_stream_t->IOfile);
                                if (x == 0) dependency = true;
                                temp_stream_t = temp_stream_t->next;
                            }
                            val_t = val_t->next;
                        }
                    }

                    if (dependency == true) break;
                    else temp_stream = temp_stream->nextStream;
                }
                if (dependency == false) {
                    // no dependency and the command list is empty
                    if (previous_command_list == NULL) {
                        current_command_list->nextStream = val;
                        current_command_list = val;
                        val = val->nextStream;
                        c_stream = val;
                    } else {
                        current_command_list->nextStream = val;
                        current_command_list = val;
                        val = val->nextStream;
                        previous_command_list->nextStream = val;
                    }
                    temp+=1;
                } else {
                    previous_command_list = val;
                    val = val->nextStream;
                }
            }
            current_command_list->nextStream = NULL;
        }
        
        // for each command that is ready, fork command
        int new_temp = 0;
        pid_t *forked = checked_malloc(temp * sizeof(pid_t));
        
        if (command_list) {
            val = command_list;
            while (val) {
                pid_t just_forked = fork();
                if (just_forked > 0) forked[new_temp] = just_forked;
                else if (just_forked == 0) {
                    execute_command(val->com, 1);
                    exit(0);
                } else { // couldn't be forked to make a child
                    fprintf(stderr, "Cannot fork from this parent to make a child");
                }
                new_temp+=1;
                val = val->nextStream;
            }
            
            // continue forking unless forked process is running
            bool forked_running = true;
            do {
                int temp1;
                forked_running = false;
                for (temp1 = 0; temp1<new_temp; temp1++) {
                    if (forked[temp1] > 0) {
                        pid_t temp2 = waitpid(forked[temp1], NULL, 0);
                        if (temp2 != 0) forked[temp1] = 0;
                        else forked_running = true;
                    }
                    sleep(0); // child currently running, sleep until not running
                }
            } while (forked_running == true);
        }
    }
}
