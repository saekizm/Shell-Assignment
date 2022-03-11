
#ifndef UTIL
#define UTIL

#define TRUE 1
#define FALSE !TRUE

static char* currentDirectory;
extern char** environ;

int prompt_flag;

pid_t pid;


/**
 * SIGNAL HANDLERS
 */
// signal handler for SIGCHLD */
void signalHandler_child(int p);
// signal handler for SIGINT
void signalHandler_int(int p);

void shell_prompt();
void welcomeScreen();
int changeDirectory(char * args[]);
int dir(char **args);
int shell_environ(char **args);
int shell_echo(char **args);
int shell_pause();
int shell_help(char **args);
void execProg(char **args, int background);
void input_output(char *args[], char *inputFile, char *outputFile, int option);
void pipeHandler(char *args[]);
int commandHandler(char *args[]);
int shell_batchmode(char filename[100]);
char **shell_split_line(char *line);

#endif
