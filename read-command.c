// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"      // allows use of checking memory allocation

#include <ctype.h>		// allows use of isalnum()
#include <locale.h>     // also allows use of isalnum() function
#include <error.h>
#include <stddef.h>     // allows use of NULL
#include <stdbool.h>    // allows use of boolean expressions
#include <stdio.h>      // allows use of string comparisons and fprintf
#include <string.h>     // allows use of strings
#include <limits.h>     // allows use of finding maximum value of integral types

//sort < a | cat b - | tr A-Z a-z > c
//sort -k2 d - < a | uniq -c > e
//diff a c > f

/* FIXME: You may need to add #include directives, macro definitions, static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
 complete the incomplete type declaration in command.h.  */

// command_stack -> command_stack
typedef struct command_stack command_stack;
struct command_stack
{
	command_t command[4];
	int numItems;
};

// push command onto command stack
void push(command_stack* m_stack, command_t m_command) {
    m_stack->numItems++;
    m_stack->command[m_stack->numItems-1] = m_command;
}

// pop top of stack and return
command_t pop(command_stack* m_stack) {
    m_stack->numItems--;
    command_t temp=m_stack->command[m_stack->numItems];
    return temp;
}

// returns top of stack (without popping)
command_t get(command_stack* m_stack) {
    command_t temp = m_stack->command[m_stack->numItems-1];
    return temp;
}

// returns current size of stack
int currentSize(command_stack* m_stack) {
    return m_stack->numItems;
}

// check if character is a valid character
bool isWord(char c) {
    bool temp1 = isalnum(c);
    bool temp2 = (c == '!') || (c == '%') || (c == '+') || (c == ',') || (c == '-') || (c == '.') || (c == '/') || (c == ':') || (c == '@') || (c == '^') || (c == '_');
    return (temp1 || temp2);
}

typedef struct token_temp token;
typedef struct token_stream_temp token_stream;

enum tokenType {
    TOP,
    SEMICOLON,
    PIPE,
    AND,
    OR,
    SUBSHELL,
    INPUT, // left
    OUTPUT, // right
    WORD
};

// each token consists of type, actual character, next token, and which line it is in
struct token_temp {
    enum tokenType type;
    char* character;
    token* nextToken;
    int line;
};

struct token_stream_temp
{
	token* head;
	token_stream* nextStream;
};

// create token; assign type, character, line, and next token
token* create_token(enum tokenType _type, int p_line, char* char_type) {
    token* _token = (token*) checked_malloc(sizeof(struct token_temp));
    
    _token->type = _type;
    _token->character = char_type;
    _token->line = p_line;
    _token->nextToken = NULL;
    return _token;
}

// create stream of tokens using a pointer to a char (technically an array)
token_stream* create_tstream (char *p_char, size_t p_buffer)
{
    token* head_symbol = create_token(TOP, -1, NULL);
    token* current_symbol = head_symbol;

    // check to allocate proper block of memory
    token_stream* head_stream = (token_stream*) checked_malloc(sizeof(token_stream));
    token_stream* current_stream = head_stream;
	current_stream->head = head_symbol;
    
    int current_line = 1; unsigned int _index = 0; char c = *p_char;
    
    // loops through entire input script, handles each case of valid character
	while (_index < p_buffer)
	{
        if (c == ';') {
            current_symbol->nextToken = create_token(SEMICOLON, current_line, NULL);
            current_symbol = current_symbol->nextToken;
            p_char++;
            c = *p_char;
            _index++;
        } else if (c == '|') {
            p_char++;
            c = *p_char;
            _index++;
            
            if (c == '|') { // double || is or
                current_symbol->nextToken = create_token(OR, current_line, NULL);
                current_symbol = current_symbol->nextToken;
                p_char++;
                c = *p_char;
                _index++;
            } else { // single | is a pipe
                current_symbol->nextToken = create_token(PIPE, current_line, NULL);
                current_symbol = current_symbol->nextToken;
            }
        } else if (c == '&') {
            p_char++;
            c = *p_char;
            _index++;
            
            if (c == '&') { // check for &&
                current_symbol->nextToken = create_token(AND, current_line, NULL);
                current_symbol = current_symbol->nextToken;
                p_char++;
                c = *p_char;
                _index++;
            } else { // single & cannot exist
                fprintf(stderr, "Line %d: Single & is not allowed", current_line);
                return NULL;
            }
        } else if (c == '(') { // beginning of the subshell
            int subshell_line = current_line;
            int n = 1;
            int sub_index = 0;
            size_t subshell_size = 512;
            char* subshell = (char*) checked_malloc(subshell_size);
            
            while (n>0) { // continuously check next characters until subshell closes
                p_char++;
                c = *p_char;
                _index++;
                
                if (_index == p_buffer) {
                    fprintf(stderr, "Line %d: End of file cannot be reached before subshell's end", current_line);
                    return NULL;
                } if (c == '\n') { // handles case for new line
                    while (p_char[1] == '\n' || p_char[1] == ' ' || p_char[1] == '\t') {
                        if (p_char[1] == '\n') current_line++;
                        p_char++;
                        _index++;
                    }
                    c = ';';
                    current_line++;
                } else if (c == '(') n++; // handles case for nested subshell;
                else if (c == ')') { // handles case for closing the subshell
                    n--;
                    if (n == 0) { // signifies close of entire subshell
                        p_char++;
                        c = *p_char;
                        _index++;
                        break;
                    }
                }
                
                subshell[sub_index] = c;
                sub_index++;
            }
            current_symbol->nextToken = create_token(SUBSHELL, subshell_line, subshell);
            current_symbol = current_symbol->nextToken;
        } else if (c == ')') { // cannot have closing parentheses without opening parentheses
            fprintf(stderr, "Line %d: There are no open parentheses that match", current_line);
            return NULL;
        } else if (c == '<') { // handles case for input file
            current_symbol->nextToken = create_token(INPUT, current_line, NULL);
            current_symbol = current_symbol->nextToken;
            p_char++;
            c = *p_char;
            _index++;
        } else if (c == '>') {// handles case for output file
            current_symbol->nextToken = create_token(OUTPUT, current_line, NULL);
            current_symbol = current_symbol->nextToken;
            p_char++;
            c = *p_char;
            _index++;
        } else if (c == ' ' || c == '\t') { // handles case for space character or tab
            p_char++;
            c = *p_char;
            _index++;
        } else if (c == '\n') { // handles case for new line
            current_line++;
            
            if (current_symbol->type == WORD || current_symbol->type == SUBSHELL) {
                current_stream->nextStream = (token_stream*) checked_malloc(sizeof(token_stream));
                current_stream = current_stream->nextStream;
                current_stream->head = create_token(TOP, -1, NULL);
                current_symbol = current_stream->head;
            } else if (current_symbol->type == INPUT || current_symbol->type == OUTPUT) {
                fprintf(stderr, "Line %d: New line after input/output", current_line);
                return NULL;
            }
            p_char++;
            c = *p_char;
            _index++;
        } else if (isWord(c)) { // handles case if character isn't any of the above but is a word
            size_t _size = 32;
            unsigned int count = 0;
            char* word = (char*) checked_malloc(_size);
            
            while (_index < _size && isWord(c)) {
                word[count] = c;
                count++;
                
                if (count == _size) {
                    _size *= 2;
                    word = (char*) checked_grow_alloc(word, &_size);
                }
                p_char++;
                c = *p_char;
                _index++;
            }
            
            current_symbol->nextToken = create_token(WORD, current_line, word);
            current_symbol = current_symbol->nextToken;
        } else { // character is not invalid, ERROR
            fprintf(stderr, "Line %d: Character is invalid", current_line);
            return NULL;
        }
    }
    return head_stream;
}

// put branch from command tree and place onto stack
// two operands and an op will be popped off
// returns 0 if bool is false, 1 if bool is true
int branchToStack(command_stack* operand_stack, command_stack* operands) {
    if (currentSize(operand_stack) <= 1) return 0;
    command_t temp_command = pop(operands);
    temp_command->u.command[0] = pop(operand_stack);
    temp_command->u.command[1] = pop(operand_stack);
    
    push(operand_stack, temp_command);
    return 1;
}


// using the linked list of tokens,
// handles all valid cases to make
// the command tree
command_t tokensToTree(token* first)
{
    command_t before, now; // before = prev_cmd, now = curr_cmd
    before = NULL;
    token *temp_token = first;
    int token_level = temp_token->line;
	command_stack* operand_stack = checked_malloc(sizeof(command_stack));
    command_stack* operands = checked_malloc(sizeof(command_stack));
    operand_stack->numItems = operand_stack->numItems = 0;

	// process tokens
	do {
		if( !(temp_token->type == INPUT || temp_token->type == OUTPUT) ) now = checked_malloc(sizeof( struct command )); // make new command
        
        // handles cases for semicolon, will pop it off stack
        if (temp_token->type == SEMICOLON) {
            now->type = SEQUENCE_COMMAND;
            if (currentSize(operands) != 0) {
                if (branchToStack(operand_stack, operands) == 0) {
                    error(1, 0, "Line %d: Tree needs more children to be created", token_level);
                    return NULL;
                }
            }
            push(operands, now);
        }
        // handles cases for pipe |
        // pop when operands have greater priority
        else if (temp_token->type == PIPE) {
            now->type = PIPE_COMMAND;
            
            // if PIPE has <= priority to operands, pop
            if (currentSize(operands) != 0 && get(operands)->type == PIPE_COMMAND)
            {
                if (branchToStack(operand_stack, operands) == 0)
                {
                    error(1, 0, "Line %d: Tree needs more children to be created", token_level);
                    return NULL;
                }
            }
            
            // push PIPE to ops
            push(operands, now);
        }
        
        // handles cases for AND
        // pop when operands have greater priority
        else if (temp_token->type == AND) {
            now->type = AND_COMMAND;
            enum command_type x = PIPE_COMMAND;
            enum command_type y = OR_COMMAND;
            enum command_type z = AND_COMMAND;
            if ( currentSize(operands) != 0 && (get(operands)->type == x || get(operands)->type == y || get(operands)->type == z) ) {
                if (branchToStack(operand_stack, operands) == 0) {
                    error(1, 0, "Line %d: Tree needs more children to be created", token_level);
                    return NULL;
                }
            }
            
            // push AND to ops
            push(operands, now);
        }
        // handles case for OR
        // pop when operands have greater priority
        else if (temp_token->type == OR) {
            now->type = OR_COMMAND;
            enum command_type x = PIPE_COMMAND;
            enum command_type y = OR_COMMAND;
            enum command_type z = AND_COMMAND;
            if ( currentSize(operands) != 0 && (get(operands)->type == x || get(operands)->type == y || get(operands)->type == z) ) {
                if (branchToStack(operand_stack, operands) == 0)
                {
                    error(2, 0, "Line %d: Syntax error. Not enough children to create new tree.", token_level);
                    return NULL;
                }
            }
            
            // push OR to ops
            push(operands, now);
        }
        // handles case for SUBSHELL
        // place subshell onto operand stack because there will be more
        else if (temp_token->type == SUBSHELL) {
            now->type = SUBSHELL_COMMAND;
            now->u.subshell_command = tokensToTree(create_tstream(temp_token->character, strlen(temp_token->character))->head);
            push(operand_stack, now);
        }
        // handles case for INPUT
        // subshell or word must be before INPUT
        else if (temp_token->type == INPUT) {
            bool _error = false;
            if (before == NULL || !(before->type == SUBSHELL_COMMAND || before->type == SIMPLE_COMMAND )) {
                error(1, 0, "Line %d: A redirection only comes after subshells or simple commands(words)", token_level);
                _error = true;
            } else if (before->input != NULL) {
                error(1, 0, "Line %d: Input already exists with the previous command", token_level);
                _error = true;
            } else if (before->output != NULL) {
                error(1, 0, "Line %d: Output already exists with the previous command", token_level);
                _error = true;
            }
            if (_error == true) return NULL;
            
            if (temp_token->nextToken->type == WORD) before->input = temp_token->nextToken->character;
            else
            {
                error(1, 0, "Line %d: A redirection only comes after subshells or simple commands(words)", token_level);
                return NULL;
            }
        }
        // handles case for OUTPUT
        // subshell or word must be before OUTPUT too
        else if (temp_token->type == OUTPUT) {
            bool _error = false;
            if (before == NULL || !(before->type == SUBSHELL_COMMAND || before->type == SIMPLE_COMMAND))
            {
                error(2, 0, "Line %d: A redirection only comes after subshells or simple commands(words)", token_level);
                _error = true;
            }
            else if (before->output != NULL)
            {
                error(2, 0, "Line %d: Output already exists with the previous command", token_level);
                _error = true;
            }
            if (_error == true) return NULL;
            
            if (temp_token->nextToken->type == WORD) before->output = temp_token->nextToken->character;
            else
            {
                error(2, 0, "Line %d: A redirection only comes after subshells or simple commands(words)", token_level);
                return NULL;
            }
        }
        // handles case for WORD
        // creates an array and finds number of words after WORD
        // the last word must point to null and word is pushed onto operand stack
        else if (temp_token->type == WORD) {
            now->type = SIMPLE_COMMAND;
            token *temp_token1 = temp_token;
            size_t word_count = 1;
            size_t temp_val;
            while (temp_token1->nextToken != NULL && temp_token1->nextToken->type == WORD) {
                temp_token1 = temp_token1->nextToken;
                word_count++;
            }
            now->u.word = checked_malloc((word_count+1) * sizeof(char*));
            now->u.word[0] = temp_token->character;
            
            for (temp_val = 1; temp_val < word_count; temp_val++) {
                temp_token = temp_token->nextToken;
                now->u.word[temp_val] = temp_token->character;
            }
            now->u.word[word_count] = NULL;
            push(operand_stack, now);
        }
        before = now;
    } while(temp_token != NULL && (temp_token = temp_token->nextToken) != NULL);

    
    // must be a certain number of operands in order to create the tree
    for ( ; currentSize(operands) > 0 ; ) {
        if (branchToStack(operand_stack, operands) == 0)
		{
			error(1, 0, "Line %d: Tree needs more children to be created", token_level);
			return NULL;
        }
    }
    
    // make sure that last part of tree is remaining in operand stack
    if (currentSize(operand_stack) != 1) {
        fprintf(stderr, "Line %d: No convergence of tree to the single root", token_level);
        return NULL;
    }
    
    command_t temporary = pop(operand_stack);
    return temporary;
    
}

// finds all of the input/output files that exist in command
IOfiles_t getIOfiles(command_t m_command)
{
	IOfiles_t current_file_list, file_list, subshell_file_list;
    current_file_list = file_list = subshell_file_list = NULL;
    
    // recursively call to get all input/output files from the first command
    if (m_command->type == AND_COMMAND || m_command->type == OR_COMMAND || m_command->type == PIPE_COMMAND || m_command->type == SEQUENCE_COMMAND) {
        file_list = getIOfiles(m_command->u.command[0]);
        
        // recursively call to get all the input/output files from the second command
        if (file_list != NULL) {
            current_file_list = file_list;
            for ( ; current_file_list->next != NULL ; )
                current_file_list = current_file_list->next;
            current_file_list->next = getIOfiles(m_command->u.command[1]);
        } else file_list = getIOfiles(m_command->u.command[1]);
    }
    // input/output files from the simple and subshell commands are added to the list
    else if ((m_command->type == SUBSHELL_COMMAND) || (m_command->type == SIMPLE_COMMAND)) {
        // handles case for subshells and adds its input/output files
        if (m_command->type == SUBSHELL_COMMAND) subshell_file_list = getIOfiles(m_command->u.subshell_command);
        
        current_file_list = checked_malloc(sizeof(struct IOfiles));
        if (m_command->input != NULL && (m_command->input != 0))
        {
            file_list = current_file_list;
            current_file_list->IOfile = m_command->input;
            current_file_list->next = NULL;
        }
        
        if (m_command->output != NULL && (m_command->output != 0))
        {
            current_file_list->IOfile = m_command->output;
            current_file_list->next = NULL;
            
            if (file_list != NULL) file_list->next = current_file_list;
            else file_list = current_file_list;
        }
        if (subshell_file_list != NULL)
        {
            current_file_list = file_list;
            file_list = subshell_file_list;
            file_list->next = current_file_list;
        }
    }
    else {
        error(3, 0, "Command type not recognized.");
        
    }
	return file_list;
}

// Makes a command stream out of an input shell script file
command_stream_t make_command_stream (int (*get_next_byte) (void *), void *get_next_byte_argument)
{
    /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    char next_char;
    char* mem = checked_malloc(BUFSIZ);
	size_t temp_bufsiz = 1024;
	size_t count = 0;
	
    
    // inserts into "mem" the input
    while (next_char != -1) {
        next_char = get_next_byte(get_next_byte_argument);
        
        // removes all comments (from the # onwards to the end of the line or end of file)
        if (next_char == '#') {
            while (next_char != EOF && next_char != '\n' && next_char != -1) next_char = get_next_byte(get_next_byte_argument);
        } if (next_char != -1) {
            mem[count] = next_char;
            count++;
            if (count == temp_bufsiz) {
                temp_bufsiz *= 2;
                mem = checked_grow_alloc(mem, &temp_bufsiz);
            } else if (count == INT_MAX) {
                fprintf(stderr, "The size of the input is greater than INT_MAX");
            }
        }
    }
    
	// create the token stream from what is currently in "mem"
	token_stream* top = create_tstream(mem, count);
	if (top == NULL)
	{
		error(1, 0, "The token stream couldn't be created");
		return NULL;
	}
    
    // get all of the linked lists of the commands from each line
    command_stream_t top_val, now_val, before_val;
    top_val = now_val = before_val = NULL;
    token_stream* top_stream = top;
    while ( (top_stream->head->nextToken != NULL) && (top_stream != NULL) ) {
        now_val = checked_malloc(sizeof(struct command_stream));
        now_val->com = tokensToTree(top_stream->head->nextToken);
        now_val->com_file_dependency = getIOfiles(tokensToTree(top_stream->head->nextToken));
        
        if (top_val == NULL) {
            top_val = now_val;
            before_val = now_val;
        } else {
            before_val = now_val;
            before_val->nextStream = now_val;
        }
        top_stream = top_stream->nextStream;
    }
    return top_val;
    
}

// return first command of the group of linked lists of commands
// shifts everything over
command_t read_command_stream (command_stream_t s)
{
/* FIXME: Replace this with your implementation too.  */
    
    // read whatever is pointed to by the pointer, advance pointer to the next node
	if (s == NULL || s->com == NULL) return NULL;
    
    command_t curr = s->com;
    if (s->nextStream != NULL) {
        command_stream_t next_comStream = s->nextStream;
        s->com = s->nextStream->com;
        s->nextStream = s->nextStream->nextStream;
    } else
        s->com = NULL;
    
    return curr;
}