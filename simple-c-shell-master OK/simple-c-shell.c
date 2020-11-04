/*
  * simple-c-shell.c
  * 
  * Copyright (c) 2013 Juan Manuel Reyes
  * 
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  * 
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  * 
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "util.h"

#define LIMIT 256 // Số lượng token lớn nhất cho command
#define MAXLINE 1024 // số lượng kí tự lớn nhất được người dùng nhập



void ShowHistory()
{
	history[strlen(history) - 1] = '\0';
	int i;
	for(i = 0; i < strlen(history) - 1; i++)
	{
		history[i] = history[i + 1];
	}
	history[strlen(history) - 1] = '\0';
	printf("%s\n", history);
}

/**
 * Hàm tạo shell. Cách tiếp cận được giải thích ở
 * http://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html
 */
void init(){
		// See if we are running interactively
        GBSH_PID = getpid();
        // kiểm tra xem bộ mô tả tệp có tham chiếu đến terminal hay không
        GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);  

		if (GBSH_IS_INTERACTIVE) {
			// Vòng lặp cho đến khi chúng ta ở foreground
			while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp()))
					kill(GBSH_PID, SIGTTIN);             
	              
	              
	        // Set the signal handlers for SIGCHILD and SIGINT
			act_child.sa_handler = signalHandler_child;		//sửa thành SIGCHLD
			act_int.sa_handler = signalHandler_int;			
			
			/**The sigaction structure is defined as something like
			
			struct sigaction {
				void (*sa_handler)(int);
				void (*sa_sigaction)(int, siginfo_t *, void *);
				sigset_t sa_mask;
				int sa_flags;
				void (*sa_restorer)(void);
				
			}*/
			
			//sigaction: được sử dụng để thay đổi hành động được thực hiện bởi một quá trình khi nhận được một tín hiệu cụ thể.
			sigaction(SIGCHLD, &act_child, 0);
			sigaction(SIGINT, &act_int, 0);
			
			// Put ourselves in our own process group
			setpgid(GBSH_PID, GBSH_PID); // we make the shell process the new process group leader
			GBSH_PGID = getpgrp();
			if (GBSH_PID != GBSH_PGID) {
					printf("Error, the shell is not process group leader");
					exit(EXIT_FAILURE);	//EXIT_FAILURE is 8
			}
			// giữ quyền kiểm soát terminal
			tcsetpgrp(STDIN_FILENO, GBSH_PGID);  
			
			// Lưu thuộc tính terminal mặc định cho shell
			tcgetattr(STDIN_FILENO, &GBSH_TMODES);

			// Lấy thư mục hiện tại sẽ được sử dụng trong các phương thức khác
			currentDirectory = (char*) calloc(1024, sizeof(char));
        } 
		else 	//khong phai terminal
		{
                printf("COULD NOT MAKE THE SHELL INTERACTIVE.\n");
                exit(EXIT_FAILURE);
        }
}

//welcomeScreen
void welcomeScreen(){
        printf("\n============================================\n");
        printf(  "|     OUR SHELL - THAT - TUONG - TUNG      |\n");
		printf("============================================\n");
        printf("\n");
}

/**
 * SIGNAL HANDLERS
 */

/**
 * signal handler for SIGCHLD
 */
void signalHandler_child(int p){
	// Chờ cho tất cả các process chết.
	/* 	Sử dụng non-blocking call (WNOHANG) để trình xử lý tín hiệu sẽ không chặn
		nếu child được cleaned up trong phần khác của chương trình.	*/
	while (waitpid(-1, NULL, WNOHANG) > 0){
	}
	printf("\n");
}

/**
 * Signal handler for SIGINT
 */
void signalHandler_int(int p){
	// Gửi một tín hiệu SIGTERM đến process con
	if (kill(pid,SIGTERM) == 0){		//SIGTERM yêu cầu kết thúc một cách lịch sự
		printf("\nProcess %d received a SIGINT signal\n",pid);
		no_reprint_prmpt = 1;			
	}else{
		printf("\n");
	}
}

/**
 *	Hiển thị lời nhắc cho shell
 */
void shellPrompt(){
	//in lời nhắc là đường dẫn hiện tại
	printf("%s > ", getcwd(currentDirectory, 1024));
}

///
///Method to change directory
///
int changeDirectory(char* args[]){
	// nếu không nhập đường dẫn mà chỉ gõ 'cd' thôi thì sẽ đến thư mục home/sv
	if (args[1] == NULL) {
		chdir(getenv("HOME")); 
		return 1;
	}
	// ngược lại sẽ thay đổi thư mục được chỉ định nếu có thể
	else{ 
		if (chdir(args[1]) == -1) {
			printf(" %s: no such directory\n", args[1]);
            return -1;
		}
	}
	return 0;
}

///
///phương thức được sử dụng để quản lý các biến môi trường với những lựa chọn khác nhau
///
int manageEnviron(char * args[], int option){
	char **env_aux;
	switch(option){
		// Case 'environ': we print the environment variables along with
		// their values
		case 0: 
			for(env_aux = environ; *env_aux != 0; env_aux ++){
				printf("%s\n", *env_aux);
			}
			break;
		// Case 'setenv': we set an environment variable to a value
		case 1: 
			if((args[1] == NULL) && args[2] == NULL){
				printf("%s","Not enought input arguments\n");
				return -1;
			}
			
			// We use different output for new and overwritten variables
			if(getenv(args[1]) != NULL){
				printf("%s", "The variable has been overwritten\n");
			}else{
				printf("%s", "The variable has been created\n");
			}
			
			// If we specify no value for the variable, we set it to ""
			if (args[2] == NULL){
				setenv(args[1], "", 1);
			// We set the variable to the given value
			}else{
				setenv(args[1], args[2], 1);
			}
			break;
		// Case 'unsetenv': we delete an environment variable
		case 2:
			if(args[1] == NULL){
				printf("%s","Not enought input arguments\n");
				return -1;
			}
			if(getenv(args[1]) != NULL){
				unsetenv(args[1]);
				printf("%s", "The variable has been erased\n");
			}else{
				printf("%s", "The variable does not exist\n");
			}
		break;
			
			
	}
	return 0;
}
 
/**
* Method for launching a program. It can be run in the background
* or in the foreground
*/ 
void launchProg(char **args, int background){	 
	 int err = -1;
	 
	 if((pid=fork())==-1){
		 printf("Child process could not be created\n");
		 return;
	 }
	 // pid == 0 implies the following code is related to the child process
	if(pid==0){
		// We set the child to ignore SIGINT signals (we want the parent
		// process to handle them with signalHandler_int)	
		signal(SIGINT, SIG_IGN);
		
		// We set parent=<pathname>/simple-c-shell as an environment variable
		// for the child
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		
		// If we launch non-existing commands we end the process
		if (execvp(args[0],args)==err){
			printf("Command not found");
			kill(getpid(),SIGTERM);
		}
	 }
	 
	 // The following will be executed by the parent
	 
	 // If the process is not requested to be in background, we wait for
	 // the child to finish.
	 if (background == 0){
		 waitpid(pid,NULL,0);
	 }else{
		 // In order to create a background process, the current process
		 // should just skip the call to wait. The SIGCHILD handler
		 // signalHandler_child will take care of the returning values
		 // of the childs.
		 printf("Process created with PID: %d\n",pid);
	 }	 
}
 
/**
* Method used to manage I/O redirection
*/ 
void fileIO(char * args[], char* inputFile, char* outputFile, int option){
	 
	int err = -1;
	
	int fileDescriptor; // between 0 and 19, describing the output or input file
	
	if((pid=fork())==-1){
		printf("Child process could not be created\n");
		return;
	}
	if(pid==0){
		// Option 0: output redirection
		if (option == 0){
			// We open (create) the file truncating it at 0, for write only
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
			// We replace de standard output with the appropriate file
			dup2(fileDescriptor, STDOUT_FILENO); 
			close(fileDescriptor);
		// Option 1: input and output redirection
		}else if (option == 1){
			// We open file for read only (it's STDIN)
			fileDescriptor = open(inputFile, O_RDONLY, 0600);  
			// We replace de standard input with the appropriate file
			dup2(fileDescriptor, STDIN_FILENO);
			close(fileDescriptor);
			// Same as before for the output file
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
			dup2(fileDescriptor, STDOUT_FILENO);
			close(fileDescriptor);		 
		}
		 
		setenv("parent",getcwd(currentDirectory, 1024),1);
		
		if (execvp(args[0],args)==err){
			printf("err");
			kill(getpid(),SIGTERM);
		}		 
	}
	waitpid(pid,NULL,0);
}

/**
* Method used to manage pipes.
*/ 
void pipeHandler(char * args[]){
	// File descriptors
	int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
	int filedes2[2];
	
	int num_cmds = 0;
	
	char *command[256];
	
	pid_t pid;
	
	int err = -1;
	int end = 0;
	
	// Variables used for the different loops
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	
	// First we calculate the number of commands (they are separated
	// by '|')
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	
	// Main loop of this method. For each command between '|', the
	// pipes will be configured and standard input and/or output will
	// be replaced. Then it will be executed
	while (args[j] != NULL && end != 1){
		k = 0;
		// We use an auxiliary array of pointers to store the command
		// that will be executed on each iteration
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;	
			if (args[j] == NULL){
				// 'end' variable used to keep the program from entering
				// again in the loop when no more arguments are found
				end = 1;
				k++;
				break;
			}
			k++;
		}
		// Last position of the command will be NULL to indicate that
		// it is its end when we pass it to the exec function
		command[k] = NULL;
		j++;		
		
		// Depending on whether we are in an iteration or another, we
		// will set different descriptors for the pipes inputs and
		// output. This way, a pipe will be shared between each two
		// iterations, enabling us to connect the inputs and outputs of
		// the two different commands.
		if (i % 2 != 0){
			pipe(filedes); // for odd i
		}else{
			pipe(filedes2); // for even i
		}
		
		pid=fork();
		
		if(pid==-1){			
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}else{
					close(filedes2[1]); // for even i
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
			// If we are in the last command, depending on whether it
			// is placed in an odd or even position, we will replace
			// the standard input for one pipe or another. The standard
			// output will be untouched because we want to see the 
			// output in the terminal
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[0],STDIN_FILENO);
				}else{ // for even number of commands
					dup2(filedes2[0],STDIN_FILENO);
				}
			// If we are in a command that is in the middle, we will
			// have to use two pipes, one for input and another for
			// output. The position is also important in order to choose
			// which file descriptor corresponds to each input/output
			}else{ // for odd i
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}else{ // for even i
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
			
/**
* Method used to handle the commands entered via the standard input
*/ 
int commandHandler(char * args[]){
	int i = 0;
	int j = 0;
	
	int fileDescriptor;
	int standardOut;
	
	int aux;
	int background = 0;
	
	char *args_aux[256];
	
	// Tìm kiếm kí tự đặc biệt và tách lệnh thành các mảng các đối số mới
	while ( args[j] != NULL)
	{
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j],"&") == 0)){
			break;
		}
		args_aux[j] = args[j];
		j++;
	}
	
	// thoát shell khi nhập exit
	if(strcmp(args[0],"exit") == 0) 
	{
		exit(0);
	}
	
	// In thư mục hiện tại khi người dùng nhập pwd
 	else if (strcmp(args[0],"pwd") == 0)
	{
		if (args[j] != NULL)
		{
			// Xuất ra file output
			if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) )
			{
				fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600);	//pipe ghi: open(const char *pathname, int flags, mode_t mode);
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
													// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				printf("%s\n", getcwd(currentDirectory, 1024));
				dup2(standardOut, STDOUT_FILENO);
			}
		}else{
			printf("%s\n", getcwd(currentDirectory, 1024));
		}
	} 
 	// Nhấn nút clear xóa màn hình
	else if (strcmp(args[0],"clear") == 0) system("clear");
	
	// nhấn cd thay đổi thư mục
	else if (strcmp(args[0],"cd") == 0) changeDirectory(args);
	
	// lệnh environ để liệt kê các biến môi trường
	else if (strcmp(args[0],"environ") == 0){
		if (args[j] != NULL){
			// Nếu muốn xuất file
			if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) ){
				fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600); 
				
				// Thay thế standard output bằng file thích hợp
				standardOut = dup(STDOUT_FILENO); 	// đầu tiên tạo một bản sao của stdout
													// bởi vì chúng tôi sẽ muốn nó trở lại
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				manageEnviron(args,0);
				dup2(standardOut, STDOUT_FILENO);
			}
		}else{
			manageEnviron(args,0);
		}
	}
	
	// lệnh setenv cài đặt biến môi trường
	else if (strcmp(args[0],"setenv") == 0) 
		manageEnviron(args,1);
	
	// lệnh unsetenv để hủy xác định các biến môi trường
	else if (strcmp(args[0],"unsetenv") == 0) 
		manageEnviron(args,2);
	
	else if (strcmp(args[0],"history") == 0)
	{
		ShowHistory();
	}
	
	else{
		// If none of the preceding commands were used, we invoke the
		// specified program. We have to detect if I/O redirection,
		// piped execution or background execution were solicited
		while (args[i] != NULL && background == 0){
			// If background execution was solicited (last argument '&')
			// we exit the loop
			if (strcmp(args[i],"&") == 0){
				background = 1;
			// If '|' is detected, piping was solicited, and we call
			// the appropriate method that will handle the different
			// executions
			}else if (strcmp(args[i],"|") == 0){
				pipeHandler(args);
				return 1;
			// If '<' is detected, we have Input and Output redirection.
			// First we check if the structure given is the correct one,
			// and if that is the case we call the appropriate method
			}else if (strcmp(args[i],"<") == 0){
				aux = i+1;
				if (args[aux] == NULL || args[aux+1] == NULL || args[aux+2] == NULL ){
					printf("Not enough input arguments\n");
					return -1;
				}else{
					if (strcmp(args[aux+1],">") != 0){
						printf("Usage: Expected '>' and found %s\n",args[aux+1]);
						return -2;
					}
				}
				fileIO(args_aux,args[i+1],args[i+3],1);
				return 1;
			}
			// If '>' is detected, we have output redirection.
			// First we check if the structure given is the correct one,
			// and if that is the case we call the appropriate method
			else if (strcmp(args[i],">") == 0){
				if (args[i+1] == NULL){
					printf("Not enough input arguments\n");
					return -1;
				}
				fileIO(args_aux,NULL,args[i+1],0);
				return 1;
			}
			i++;
		}
		// We launch the program with our method, indicating if we
		// want background execution or not
		args_aux[i] = NULL;
		launchProg(args_aux,background);
		
		/**
		 * For the part 1.e, we only had to print the input that was not
		 * 'exit', 'pwd' or 'clear'. We did it the following way
		 */
		//	i = 0;
		//	while(args[i]!=NULL){
		//		printf("%s\n", args[i]);
		//		i++;
		//	}
	}
	
	return 1;
}


/**
* Hàm main
*/ 
int main(int argc, char *argv[], char ** envp) {
	char line[MAXLINE]; // buffer cho the user input
	char * tokens[LIMIT]; // mảng các token trong command
	int numTokens;
		
	no_reprint_prmpt = 0; 	//Ngăn chặn việc in các shell sau phương thức certain
	pid = -10; // khởi tạo pid thành một pid không thể thực hiện được
	
	// Gọi hàm khởi tạo và chào mừng
	init();
	welcomeScreen();
    
    // đặt char ** bên ngoài của mình thành môi trường để chúng tôi có thể xử lý nó sau này bằng các phương pháp khác
	environ = envp;
	
	// set shell=<pathname>/simple-c-shell as an environment variable for
	// the child
	setenv("shell",getcwd(currentDirectory, 1024),1);
	
	// Main loop, user input sẽ được đọc
	// và lời nhắc sẽ được in
	while(TRUE){
		// in lời nhắc của shell nếu cần thiết
		if (no_reprint_prmpt == 0) 
			shellPrompt();
		no_reprint_prmpt = 0;
		
		// làm rỗng line buffer ==> chèn 1024 \0 vào line, bắt đầu từ phần tử thứ nhất, đảm bảo chuỗi ban đầu là rỗng
		memset ( line, '\0', MAXLINE );

		// Chờ user input
		fgets(line, MAXLINE, stdin);
		char* linetemp = (char*)malloc(MAXLINE);
		strcpy(linetemp,line);
		// Nếu không viết gì, vòng lặp chạy lại
		if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
		
		// Đọc tất cả tokens của input 
		// Chuyển đến commandHandler làm đối số
		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) numTokens++;

		//char* linetemp = (char*)malloc(MAXLINE);
		//strcpy(linetemp, tokens[0]);
		//if(numTokens > 1)
		//{
		//	int i;
		//	for(i = 1; i < numTokens; i++)
		//	{
		//		strcat(linetemp, tokens[i]);
		//	} 
		//}
		
		char* temp = (char*)malloc(strlen(history) + strlen(linetemp));
		strcpy(temp, history);
		//strcat(temp , "\n");
		strcat(temp , linetemp);
		history = (char*)malloc(strlen(temp));
		strcpy(history, temp);
		commandHandler(tokens);
		
	}          

	exit(0);
}
