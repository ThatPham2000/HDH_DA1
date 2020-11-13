#include "myshell.h"

#define LIMIT 256 // Số lượng token lớn nhất cho command
#define MAXLINE 1024 // số lượng kí tự lớn nhất được người dùng nhập

// Hàm main
int main(int argc, char *argv[]) 
{
	int numTokens;
	char line[MAXLINE]; // buffer cho the user input
	char * tokens[LIMIT]; // mảng các token trong command
	
	no_reprint_prmpt = 0; 
	pid = -10; // khởi tạo pid thành một pid không thể thực hiện được

	//check value before adding to history
	char* beforehistory = "";

	// Gọi hàm khởi tạo
	Initiation();
	
	// set shell=<pathname>/myshell as an environment variable for the child
	setenv("shell",getcwd(currentDirectory, 1024),1);
	
	// Main loop, user input sẽ được đọc và lời nhắc sẽ được in
	while(TRUE)
	{
		// in lời nhắc của shell nếu cần thiết
		if (no_reprint_prmpt == 0) 
			Prompt();
		no_reprint_prmpt = 0;
		
		// làm rỗng line buffer ==> chèn 1024 \0 vào line, bắt đầu từ phần tử thứ nhất, đảm bảo chuỗi ban đầu là rỗng
		memset ( line, '\0', MAXLINE );

		// Chờ user input
		fgets(line, MAXLINE, stdin);	//line da bao gom dau xuong dong
		char* linetemp = (char*)malloc(strlen(line) + 1);
		strcpy(linetemp,line);
		
		line[strlen(line) -1] = '\0';
		
		if(strcmp(line,"!!") == 0)
		{
			if(strcmp(beforehistory,"") == 0)
			{
				printf("No commands in history\n");
			}
			else
			{
				strcpy(line, beforehistory);
				printf("%s", line);
				
			}
		}

		// Nếu không viết gì, vòng lặp chạy lại
		if((tokens[0] = strtok(line," \n\t")) == NULL) continue;
		
		// Đọc tất cả tokens của input 
		// Chuyển đến HandleCommand làm đối số

		numTokens = 1;
		while((tokens[numTokens] = strtok(NULL, " \n\t")) != NULL) 
		{
			numTokens++;
		}

		if(strcmp(beforehistory, linetemp) != 0)
		{
			if(strcmp(linetemp, "!!\n") != 0)
			{
				beforehistory = (char*)malloc(strlen(linetemp));
				strcpy(beforehistory, linetemp);
				char* temp = (char*)malloc(strlen(history) + strlen(linetemp) + 1);
				strcpy(temp, history);
				//strcat(temp , "\n");
				strcat(temp , linetemp);
				history = (char*)malloc(strlen(temp) + 1);
				strcpy(history, temp);
			}

			
		}

		
		HandleCommand(tokens);
		
	}          

	exit(0);
}

void ShowHistory()
{
	printf("%s", history);
}

//Ham tao shell
void Initiation()
{
        GBSH_PID = getpid();
        // kiểm tra xem bộ mô tả tệp có tham chiếu đến terminal hay không
        GBSH_IS_INTERACTIVE = isatty(STDIN_FILENO);  

		if (GBSH_IS_INTERACTIVE) 
		{
			// Vòng lặp cho đến khi chúng ta ở foreground
			while (tcgetpgrp(STDIN_FILENO) != (GBSH_PGID = getpgrp()))
			{
					kill(GBSH_PID, SIGTTIN);                
			}   
			
	        	// Đặt bộ xử lý tín hiệu cho SIGCHLD và SIGINT
			act_chld.sa_handler = signalHandler_chld;
			act_int.sa_handler = signalHandler_int;			
			
			//sigaction: được sử dụng để thay đổi hành động được thực hiện bởi một quá trình khi nhận được một tín hiệu cụ thể.
			sigaction(SIGCHLD, &act_chld, 0);
			sigaction(SIGINT, &act_int, 0);
			
			// Put ourselves in our own process group
			setpgid(GBSH_PID, GBSH_PID); // đặt shell process trở thành new process group leader
			GBSH_PGID = getpgrp();
			if (GBSH_PID != GBSH_PGID) 
			{
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


//Hiển thị lời nhắc cho shell
void Prompt()
{
	//lời nhắc là đường dẫn hiện tại
	printf("%s%s > ", teamName, getcwd(currentDirectory, 1024));
}

//signal handler SIGCHLD
void signalHandler_chld(int p)
{
	// Chờ cho tất cả các process chết.
	//Sử dụng non-blocking call (WNOHANG) để trình xử lý tín hiệu sẽ không chặn
	//nếu child được cleaned up trong phần khác của chương trình.
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
		
	}
	printf("\n");
}

//signal handler SIGINT
void signalHandler_int(int p)
{
	// Gửi một tín hiệu SIGTERM đến process con
	if (kill(pid,SIGTERM) == 0)
	{		//SIGTERM yêu cầu kết thúc một cách lịch sự
		printf("\nProcess %d received a SIGINT signal\n",pid);
		no_reprint_prmpt = 1;			
	}
	else
	{
		printf("\n");
	}
}

//Method to change directory
int changeDirectory(char* args[])
{
	// nếu không nhập đường dẫn mà chỉ gõ 'cd' thôi thì sẽ đến thư mục home/sv/... (noi goi terminal)
	if (args[1] == NULL) 
	{
		chdir(getenv("HOME")); 
		return 1;
	}
	// ngược lại sẽ thay đổi thư mục được chỉ định nếu có thể
	else
	{ 
		if (chdir(args[1]) == -1) 
		{
			printf(" %s: no such directory\n", args[1]);
            return -1;
		}
	}
	return 0;
}


 

//Phuong thuc launching chuong trinh. Co the duoc chay trong background hoac foreground
void launchProg(char **args, int background)
{	 
	 int err = -1;
	 
	 if((pid=fork())==-1)
	 {
		 printf("Child process could not be created\n");
		 return;
	 }
	 // pid == 0 implies the following code is related to the child process
	if(pid==0)
	{
		// We set the child to ignore SIGINT signals (we want the parent process to handle them with signalHandler_int)	
		signal(SIGINT, SIG_IGN);
		
		// We set parent=<pathname>/simple-c-shell as an environment variable for the child
		setenv("parent",getcwd(currentDirectory, 1024),1);	
		
		// If we launch non-existing commands we end the process
		if (execvp(args[0],args)==err)
		{
			printf("Command not found");
			kill(getpid(),SIGTERM);
		}
	 }
	 
	 // The following will be executed by the parent
	 
	 // Neu tien trinh khong yeu cau chay background, thi cho thang con ket thuc.
	 if (background == 0)
	 {
		 waitpid(pid,NULL,0);
	 }
	 else
	 {
		 // In order to create a background process, the current process should just skip the call to wait. 
		 // The SIGCHILD handler signalHandlerchild will take care of the returning values of the childs.
		 printf("Process created with PID: %d\n",pid);
	 }	 
}
 
//Manage I/O redirection
void fileIO(char * args[], char* inputFile, char* outputFile, int option)
{
	 
	int err = -1;
	
	int fileDescriptor; // between 0 and 19, describing the output or input file
	
	if((pid=fork())==-1)
	{
		printf("Child process could not be created\n");
		return;
	}
	if(pid==0)
	{
		// Option 0: output redirection
		if (option == 0)
		{
			// We open (create) the file truncating it at 0, for write only
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600); 
			// We replace de standard output with the appropriate file
			dup2(fileDescriptor, STDOUT_FILENO); 
			close(fileDescriptor);
		
		}
		// Option 1: input and output redirection
		else if (option == 1)
		{
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
		// Option 2: input redirection
		if (option == 2)
		{
			// We open file for read only (it's STDIN)
			fileDescriptor = open(inputFile, O_RDONLY, 0600);  
			// We replace de standard input with the appropriate file
			dup2(fileDescriptor, STDIN_FILENO);
			close(fileDescriptor);
		}
		 
		setenv("parent",getcwd(currentDirectory, 1024),1);
		
		if (execvp(args[0],args)==err)
		{
			printf("err");
			kill(getpid(),SIGTERM);
		}		 
	}
	waitpid(pid,NULL,0);
}

//Phuong thuc quan ly pipe
void HandlePipe(char * args[])
{
	// File descriptors
	int filedes[2]; // pos. 0 output, pos. 1 input of the pipe
	int filedes2[2];
	
	int num_cmds = 0;
	
	char *command[256];
	
	pid_t pid;
	
	int err = -1;
	int end = 0;
	
	// Bien su dung cho cac vong lap khac nhau
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
	
	// For each command between '|', the pipes will be configured and standard input and/or output will be replaced. Then it will be executed
	while (args[j] != NULL && end != 1)
	{
		k = 0;
		// Use an auxiliary array of pointers to store the command that will be executed on each iteration
		while (strcmp(args[j],"|") != 0)
		{
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
		// Last position of the command will be NULL to indicate that it is its end when we pass it to the exec function
		command[k] = NULL;
		j++;		
		
		// Depending on whether we are in an iteration or another, we will set different descriptors for the pipes inputs and output.
		// This way, a pipe will be shared between each two iterations, enabling us to connect the inputs and outputs of the two different commands.
		if (i % 2 != 0)
		{
			pipe(filedes); // for odd i
		}
		else
		{
			pipe(filedes2); // for even i
		}
		
		pid=fork();
		
		if(pid==-1)
		{			
			if (i != num_cmds - 1)
			{
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}
				else
				{
					close(filedes2[1]); // for even i
				} 
			}			
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0)
		{
			// If we are in the first command
			if (i == 0)
			{
				dup2(filedes2[1], STDOUT_FILENO);
			}
			// If we are in the last command, depending on whether it is placed in an odd or even position,
			// we will replace the standard input for one pipe or another. The standard output will be untouched because we want to see the 
			// output in the terminal
			else if (i == num_cmds - 1)
			{
				if (num_cmds % 2 != 0)
				{ // for odd number of commands
					dup2(filedes[0],STDIN_FILENO);		//change IO input by filedes[0]
				}
				else
				{
					// for even number of commands
					dup2(filedes2[0],STDIN_FILENO);
				}
			// If we are in a command that is in the middle, we will have to use two pipes, one for input and another for output.
			// The position is also important in order to choose which file descriptor corresponds to each input/output
			}
			else // for odd i
			{ 
				if (i % 2 != 0)
				{
					dup2(filedes2[0],STDIN_FILENO); 
					dup2(filedes[1],STDOUT_FILENO);
				}
				else // for even i
				{ 
					dup2(filedes[0],STDIN_FILENO); 
					dup2(filedes2[1],STDOUT_FILENO);					
				} 
			}
			
			if (execvp(command[0],command)==err)
			{
				kill(getpid(),SIGTERM);
			}		
		}
				
		// CLOSING DESCRIPTORS ON PARENT
		if (i == 0)
		{
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1)
		{
			if (num_cmds % 2 != 0)
			{					
				close(filedes[0]);
			}
			else
			{					
				close(filedes2[0]);
			}
		}
		else
		{
			if (i % 2 != 0)
			{					
				close(filedes2[0]);
				close(filedes[1]);
			}
			else
			{					
				close(filedes[0]);
				close(filedes2[1]);
			}
		}
				
		waitpid(pid,NULL,0);
				
		i++;	
	}
}
			

//Phương thuc được sử dụng để xử lý các lệnh được nhập thông qua standard input
int HandleCommand(char * args[])
{
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
		if ( (strcmp(args[j],">") == 0) || (strcmp(args[j],"<") == 0) || (strcmp(args[j],"&") == 0))
		{
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
		if (args[j] != NULL) //kiem tra neu la pwd > *.txt (example)
		{
			// Xuất ra file output
			if ( (strcmp(args[j],">") == 0) && (args[j+1] != NULL) )
			{
				fileDescriptor = open(args[j+1], O_CREAT | O_TRUNC | O_WRONLY, 0600);//pipe ghi: open(const char *pathname, int flags, mode_t mode);
				// We replace de standard output with the appropriate file
				standardOut = dup(STDOUT_FILENO); 	// first we make a copy of stdout
									// because we'll want it back
				dup2(fileDescriptor, STDOUT_FILENO); 
				close(fileDescriptor);
				printf("%s\n", getcwd(currentDirectory, 1024));
				dup2(standardOut, STDOUT_FILENO);
			}
		}
		else	//chi pwd
		{
			printf("%s\n", getcwd(currentDirectory, 1024));
		}
	} 
 	// Nhấn nút clear xóa màn hình
	else if (strcmp(args[0],"clear") == 0) system("clear");
	
	// nhấn cd thay đổi thư mục
	else if (strcmp(args[0],"cd") == 0) changeDirectory(args);
	
	else if (strcmp(args[0],"history") == 0)
	{
		ShowHistory();
	}
	
	else
	{
		// Nếu không có lệnh nào trước đó được sử dụng, gọi chương trình đã chỉ định. Tìm xem I/O redirection, thực thi đường ống hoặc thực thi nền
		while (args[i] != NULL && background == 0)
		{
			// Neu thuc thi nen (doi so cuoi cung la '&')
			// Thoat vong lap
			if (strcmp(args[i],"&") == 0)
			{
				background = 1;
			// Neu la '|', piping, goi phuong thuc phu hop de xu ly
			}
			else if (strcmp(args[i],"|") == 0)
			{
				HandlePipe(args);
				return 1;
			// Neu la '<', Input and Output redirection.
			// Kiểm tra xem cấu trúc đã cho có đúng không, thi gọi phương thức thích hợp
			}
			else if (strcmp(args[i],"<") == 0)
			{
				aux = i+1;
				if (args[aux] == NULL && args[aux+1] == NULL && args[aux+2] == NULL)
				{
					printf("Not enough input arguments\n");
					return -1;
				}
				if (args[aux + 2] != NULL)
				{
					fileIO(args_aux,args[i+1],args[i+3],1);
					return 1;
				}
				if (args[aux] != NULL && args[aux + 1] == NULL)
				{
					fileIO(args_aux,args[i+1],NULL,2);
					return 1;
				}
				return -2;
			}
			// Neu la '>', la output redirection.
			// Kiểm tra xem cấu trúc đã cho có đúng không, thi gọi phương thức thích hợp
			else if (strcmp(args[i],">") == 0)
			{
				if (args[i+1] == NULL)
				{
					printf("Not enough input arguments\n");
					return -1;
				}
				fileIO(args_aux,NULL,args[i+1],0);
				return 1;
			}
			i++;
		}
		
		// Khởi chạy chương trình với phương thức, có muốn thực thi nền hay không
		args_aux[i] = NULL;
		launchProg(args_aux,background);
		
	}
	
	return 1;
}
