#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "util.h"
#include <dirent.h>



#define LIMIT 256 // max number of tokens for a command
#define MAXLINE 1024 // max number of characters from user input


int main(int argc, char *argv[], char ** envp) {
	char line[MAXLINE]; // buffer for the user input
	char * tokens[LIMIT]; // array for the different tokens in the command
	int num_tokens;
		
	prompt_flag = 0; 	// to prevent the printing of the shell
							// after certain methods
	pid = -10; // we initialize pid to an pid that is not possible
	
	// We call the method of initialization and the welcome screen
	welcomeScreen();
    
    // We set our extern char** environ to the environment, so that
    // we can treat it later in other methods
	environ = envp;
	
	// We set shell=<pathname>/simple-c-shell as an environment variable for
	// the child
	setenv("shell",getcwd(currentDirectory, 1024),1);

    if (argc == 2) {
        shell_batchmode(argv[1]);
        return 0;
    }
	
	// Main loop, where the user input will be read and the prompt
	// will be printed
	while(TRUE){
		// We print the shell prompt if necessary
		if (prompt_flag == 0) shell_prompt();
		prompt_flag = 0;
		
		// We empty the line buffer
		memset ( line, '\0', MAXLINE );

		// We wait for user input
		fgets(line, MAXLINE, stdin);
	
		// If nothing is written, the loop is executed again
		if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
		
		// We read all the tokens of the input and pass it to our
		// commandHandler as the argument
		num_tokens = 1;
		while((tokens[num_tokens] = strtok(NULL, " \n\t")) != NULL) {
            num_tokens++;
        }
		commandHandler(tokens);
		
	}          

	exit(0);
}