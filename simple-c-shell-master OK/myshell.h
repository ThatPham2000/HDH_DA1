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

char* teamName = "That.Tung.Tuong";

// Shell pid, pgid, terminal modes
static pid_t GBSH_PID;
static pid_t GBSH_PGID;
static int GBSH_IS_INTERACTIVE;
static struct termios GBSH_TMODES;

struct sigaction act_chld;
struct sigaction act_int;

static char* currentDirectory;

int no_reprint_prmpt;

pid_t pid;

char* history = "";

int changeDirectory(char * args[]);

void ShowHistory();

void Initiation();

void signalHandler_chld(int p);

void signalHandler_int(int p);

void Prompt();

int changeDirectory(char* args[]);

void launchProg(char **args, int background);

void fileIO(char * args[], char* inputFile, char* outputFile, int option);

void HandlePipe(char * args[]);

int HandleCommand(char * args[]);