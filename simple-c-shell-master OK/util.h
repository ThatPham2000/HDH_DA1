#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

#define TRUE 1
#define FALSE !TRUE

// Shell pid, pgid, terminal modes
static pid_t GBSH_PID;
static pid_t GBSH_PGID;
static int GBSH_IS_INTERACTIVE;
static struct termios GBSH_TMODES;

static char* currentDirectory;
//extern char** environ;

struct sigaction act_child;
struct sigaction act_int;

int no_reprint_prmpt;

pid_t pid;


char* history = "";
char* teamName = "That.Tung.Tuong";


int changeDirectory(char * args[]);

void ShowHistory();

void init();

// signal handler for SIGCHLD 
void signalHandler_child(int p);
// signal handler for SIGINT
void signalHandler_int(int p);

void shellPrompt();

int changeDirectory(char* args[]);

void launchProg(char **args, int background);

void fileIO(char * args[], char* inputFile, char* outputFile, int option);

void pipeHandler(char * args[]);

int commandHandler(char * args[]);