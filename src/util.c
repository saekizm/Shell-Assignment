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
#include <util.h>

#define LIMIT 256 // max number of tokens for a command
#define MAXLINE 1024 // max number of characters from user input

char **shell_split_line(char *line);

/**
 * Method used to print the welcome screen of our shell
 */
void welcomeScreen(){
        printf("\n\t============================================\n");
        printf("\t               Welcome to MyShell!\n");
        printf("\t--------------------------------------------\n");
        printf("\t               By Senan Warnock:\n");
        printf("\t============================================\n");
        printf("\n\n");
}

/**
 * SIGNAL HANDLERS
 */


//signal handler for SIGCHLD

void signalHandler_child(int p){
	/* Wait for all dead processes.
	 * We use a non-blocking call (WNOHANG) to be sure this signal handler will not
	 * block if a child was cleaned up in another part of the program. */
	while (waitpid(-1, NULL, WNOHANG) > 0) {
	}
	printf("\n");
}

 //Signal handler for SIGINT
void signalHandler_int(int p){
	// We send a SIGTERM signal to the child process
	if (kill(pid,SIGTERM) == 0){
		printf("\nProcess %d received a SIGINT signal\n",pid);
		prompt_flag = 1;			
	}else{
		printf("\n");
	}
}

//prompt display
void shell_prompt(){
	// We print the prompt in the form "<user>@<host> <cwd> >"
	char hostn[1204] = "";
	gethostname(hostn, sizeof(hostn));
	printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
}

//cd implementation - change directories
int changeDirectory(char* args[]){
	// If we write no path, then don't change directory
	if (args[1] == NULL) {
		chdir("."); 
		return 1;
	}
	//else change to specified directory
	else{ 
		if (chdir(args[1]) == -1) {
			printf(" %s: no such directory\n", args[1]);
            return -1;
		}
	}
	return 0;
}

//list contents of directory specified
int dir(char **args) {
  DIR *d;
  struct dirent *dir;
  d = opendir(args[1]);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      printf("%s\n", dir->d_name);
    }
    closedir(d);
  }
  else {
    fprintf(stderr, "Directory: %s doesn't exist\n", args[1]);
  }
  return(1);
}

//list environment variables
int shell_environ(char **args) {
  int i;
  for (i = 0; environ[i] != NULL; i++) {
    printf("%s\n", environ[i]);
  }
  return 1;
}

//shell implementation of echo
int shell_echo(char **args) {
  int i;
  for (i = 1; args[i] != NULL; i++) {
    printf("%s ", args[i]);
  }
  printf("\n");
  return 1;
}

//shell implementation of pause
int shell_pause() {
  char ch;
  puts("Press enter to continue: \n");
  ch = getchar();

  return 1;  
}

//shell implementation of help
int shell_help(char **args) {
    system("more ../manual/README.md");
    return 1;
}

//function to run programs either in background or not
void execProg(char **args, int background){	 
	 
     int err = -1;
	 
     //error creating child
	 if((pid=fork())==-1){
		 printf("Child process could not be created\n");
		 return;
	 }
	 //related to child
	if(pid==0){
		//set the child to ignore SIGINT signals	
		signal(SIGINT, SIG_IGN);
		
		//set env for the child
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		
		// If we launch non-existing commands we end the process
		if (execvp(args[0],args)==err){
			printf("Command not found");
			kill(getpid(),SIGTERM);
		}
	 }
	 
	 //parent process execution
	 if (background == 0){
		 waitpid(pid,NULL,0);
	 }else{
		 //if we are here, process is background executed
		 printf("Process created with PID: %d\n",pid);
	 }	 
}
 
//function handler for I/O redirection 
void input_output(char *args[], char *inputFile, char *outputFile, int option){
	 
	int err = -1;
	
	int file_desc;
	
    //error creating child
	if((pid=fork())==-1){
		printf("Child process could not be created\n");
		return;
	}
	if(pid==0){
		// for > redirection
		if (option == 0){
			// open file, write only
			file_desc = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
			// replace stdout with the file we want to write to
			dup2(file_desc, STDOUT_FILENO); 
			close(file_desc);
		// for < > redirection
		}else if (option == 1){
			// open for read only
			file_desc = open(inputFile, O_RDONLY, 0600);  
			//replace stdin with file
			dup2(file_desc, STDIN_FILENO);
			close(file_desc);
			// replace stdout with file
			file_desc = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
			dup2(file_desc, STDOUT_FILENO);
			close(file_desc);		 
		}
        else if (option == 2){
			// open file for write only
			file_desc = open(outputFile, O_RDWR | O_APPEND, 0600); 
            if (0 > file_desc) {
                perror("Open Failed\n");
                exit(EXIT_FAILURE);
            }
			// replace stdout with file
			if(dup2(file_desc, STDOUT_FILENO) == -1) {
                perror("Failed\n");
                exit(EXIT_FAILURE);
            }
			close(file_desc);
        }
		 
		setenv("parent",getcwd(currentDirectory, 1024),1);


		
		if (execvp(args[0],args)==err){
			perror("error");
			kill(getpid(),SIGTERM);
		}	 
	}
	waitpid(pid,NULL,0);
}

//function to handle pipes
void pipeHandler(char *args[]){
	// File descriptors, 0 - output 1 - input of pipe
	int filedes[2];
	int filedes2[2];
	
	int num_cmds = 0;
	
	char *command[256];
	
	pid_t pid;
	
	int err = -1;
	int end = 0;
	
    //loop variables
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	
	//get number of commands
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	
	
	while (args[j] != NULL && end != 1){
		k = 0;
		//use array to store commands that execute on each loop
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
				//end signals end of loop
				end = 1;
				k++;
				break;
			}
			k++;
		}
		//last command must be NULL for execution
		command[k] = NULL;
		j++;		
		
		
		if (i % 2 != 0){
			pipe(filedes);
		}else{
			pipe(filedes2); 
		}
		
		pid=fork();
		
		if(pid==-1){			
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); 
				}else{
					close(filedes2[1]); 
				} 
			}			
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0){
			// If we are in the first command
			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}
			// If we are in the last command
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ 
					dup2(filedes[0],STDIN_FILENO);
				}else{ 
					dup2(filedes2[0],STDIN_FILENO);
				}
			// If we are in a command that is in the middle
			}else{ 
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}else{ 
					dup2(filedes[0],STDIN_FILENO); 
					dup2(filedes2[1],STDOUT_FILENO);					
				} 
			}
			
			if (execvp(command[0],command)==err){
				kill(getpid(),SIGTERM);
			}		
		}
				
		// CLOSING DESCRIPTORS ON PARENT
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){					
				close(filedes[0]);
			}else{					
				close(filedes2[0]);
			}
		}else{
			if (i % 2 != 0){					
				close(filedes2[0]);
				close(filedes[1]);
			}else{					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}
				
		waitpid(pid,NULL,0);
				
		i++;	
	}
}
			
//function to handle commands entered by stdin
int commandHandler(char *args[]){
	int i = 0;
	int j = 0;
	
	
	int aux;
	int background = 0;
	
	char *args_2[256];
	
	// We look for redirection or background symbols and separate the command itself
	// in a new array for the arguments
	while ( args[j] != NULL){
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j], ">>") == 0) || (strcmp(args[j],"&") == 0)){
			break;
		}
		args_2[j] = args[j];
		j++;
	}
	
	// 'exit' command quits the shell
	if(strcmp(args[0],"quit") == 0) exit(0);
 	
 	// 'clr' command clears the screen
	else if (strcmp(args[0],"clr") == 0) system("clear");
	// 'cd' command to change directory
	else if (strcmp(args[0],"cd") == 0) changeDirectory(args);
    // 'dir' lists files in directory
    else if (strcmp(args[0], "dir") == 0) dir(args);
    // 'echo' prints back what was typed by user
    else if (strcmp(args[0], "echo") == 0) shell_echo(args);
    // 'environ' prints environment variables
    else if (strcmp(args[0], "environ") == 0) shell_environ(args);
    // 'pause'  pauses the shell until enter is hit
    else if (strcmp(args[0], "pause") == 0) shell_pause();
    // 'help' opens readme file
    else if (strcmp(args[0], "help") == 0) shell_help(args);
	
	else{
		// If no builtins, we start checking for symbols and then parse and execute
		while (args[i] != NULL && background == 0){
			//if background symbol, set background flag
			if (strcmp(args[i],"&") == 0){
				background = 1;
			//if pipe, call pipe handler
			}else if (strcmp(args[i],"|") == 0){
				pipeHandler(args);
				return 1;
			//if < we have input and output redirection
			}else if (strcmp(args[i],"<") == 0){
				aux = i+1;
				if (args[aux] == NULL || args[aux+1] == NULL || args[aux+2] == NULL ){
					printf("Not enough input arguments\n");
					return -1;
				}else{
					if (strcmp(args[aux+1],">") != 0){
						printf("Did you mean < instead of %s?\n",args[aux+1]);
						return -2;
					}
				}
				input_output(args_2,args[i+1],args[i+3],1);
				return 1;
			}
			// If > we have output redirection.
			else if (strcmp(args[i],">") == 0){
				if (args[i+1] == NULL){
					printf("Not enough input arguments\n");
					return -1;
				}
				input_output(args_2,NULL,args[i+1],0);
				return 1;
			}
            else if (strcmp(args[i],">>") == 0){
				if (args[i+1] == NULL){
					printf("Not enough input arguments\n");
					return -1;
				}
				input_output(args_2,NULL,args[i+1],2);
				return 1;
			}
			i++;
		}
		//exec commands with background flag set
		args_2[i] = NULL;
		execProg(args_2,background);

	}
return 1;
}

//function to deal with shell scripts
int shell_batchmode(char filename[100])
{
        printf("Received Script. Opening %s", filename);
        FILE *fptr;
        char line[200];
        char **args;
        fptr = fopen(filename, "r");
        if (fptr == NULL)
        {
                printf("\nUnable to open file.");
                return 1;
        }
        else
        {
                printf("\nFile Opened. Parsing. Displaying commands first");
                while(fgets(line, sizeof(line), fptr)!= NULL)
                {
                        printf("\n%s", line);
                        args = shell_split_line(line);
                        commandHandler(args);
                }
        }
        free(args);
        fclose(fptr);
        return 1;
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"


//function to split a line into tokens
char **shell_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}