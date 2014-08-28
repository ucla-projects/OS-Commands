typedef struct command *command_t;
typedef struct command_stream *command_stream_t;
typedef struct IOfiles *IOfiles_t;

struct IOfiles
{
	char* IOfile;
	IOfiles_t next;
};

struct command_stream
{
	command_t com;
	IOfiles_t com_file_dependency;
	command_stream_t nextStream;
};

/* Create a command stream from LABEL, GETBYTE, and ARG.  A reader of
 the command stream will invoke GETBYTE (ARG) to get the next byte.
 GETBYTE will return the next input byte, or a negative number
 (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

///* Deallocates all allocated memory associated with a command tree  */
//void free_command (command_t cmd);
//
///* Traverses a command tree and returns a list of all redirect files.  */
//filelist_t get_depends (command_t c);
//
///* Returns 1 if a filelist shares dependencies with a command_stream  */
//int is_dependent (filelist_t f1, filelist_t f2);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
 an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

///* Executes command_stream with time travel parallelism.  */
//int execute_time_travel (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the integer flag is
 nonzero.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously been executed.
 Wait for the command, if it is not already finished.  */
int command_status (command_t);