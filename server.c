//
// Name: server.c 
// Author: Damon Murdoch (s290548)
// Date: 24/08/2017
// Description: Server-side application for 2803ICT 
//

#include "common.c"

int list();
int get();
int put();
int sys(); 
int error();
int delay();
int evaluate();
int execsys();

void handle_zombie();
void handle_int();

int pid_list[MAX];
int pid_count=0;
int sock;

int main()
{		
	// Ctrl-C signal handler
	signal(SIGINT,handle_int);
	
	// Zombie Process Handler
	signal(SIGCHLD,handle_zombie);

	// Creating Socket
	struct sockaddr_in myaddr;
	sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		perror("sock");
		return 1;
	}	
	
	// Creating Address Structure
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_family = AF_INET;
	//myaddr.sin_port = htons(atoi(argv[1]));
	myaddr.sin_port = 80;
	
	// Bind Address to Socket
	int err = bind(sock,(struct sockaddr*)&myaddr,sizeof(myaddr));
	if(err<0)
	{
		perror("bind");
		return 1;
	}
	
	// Listen for connectins
	listen(sock,1);
	
	int pid,psd;
	
	// Program Loop
	for(;;){
		
		int psd = accept(sock,0,0);

		pid=fork();
		
		// If child process failed:
		if(pid==-1){
			close(psd);
			continue;
		}
		
		// Child process code
		else if(pid==0){
			char buf[MAX];
			int cc = recv(psd,buf,sizeof(buf),0);
			evaluate(pid,psd,sock,buf);
			close(psd);
			exit(0);
		}
		
		// Parent process code
		else if(pid>0){
			
			pid_list[pid_count]=pid;
			pid_count++;
			
			//Cleanup Zombies
			if (waitpid(pid, NULL, 0) < 0) {
                perror("Failed to collect child process");
                break;
            }
		}
	}
	return 0;
}

// handle_int(sig)
// Interrupt handler for Ctrl-C
void handle_int(int sig){
	close(sock);
	int i;
	for(i=0;i<pid_count;i++){
		kill(pid_list[i],SIGKILL);
	}
	printf("Cleaned up %i child processes. ",i);
	printf("Server closed.\n");
	exit(0);
}

void handle_zombie(int sig){
	if(waitpid(-1,NULL,WNOHANG)<0){}
}

// evaluate(Int pid,Int psd,Int sock,Int buf): Int
// Runs a function based on the value of buf, and passes it int and buf.
int evaluate(int pid, int psd,int sock, char * buf){
	
	if(strstr(buf,"ls")==&buf[0]){
		printf("List function triggered by %i.\n",pid);
		list(psd,buf);
	}
	
	else if(strstr(buf,"get")==&buf[0]){
		printf("Get function triggered by %i.\n",pid);
		get(psd,buf);
	}
	
	else if(strstr(buf,"put")==&buf[0]){
		printf("Put function triggered by %i.\n",pid);
		put(psd,buf);
	}
	
	else if(strstr(buf,"sys")==&buf[0]){
		printf("sys function triggered by %i.\n",pid);
		sys(psd,buf);
	}
	
	else if(strstr(buf,"delay")==&buf[0]){
		printf("Delay function triggered by %i.\n",pid);
		delay(psd,buf);
	}

	else {
		printf("Invalid function triggered.\n");
		error(psd,buf);
	}
	
	return 0;
}

// execsys(int sock, int argc, char * argv[]): Int
// executes a system command using the arguments in argv and sends the result to sock.
int execsys(int sock,int argc,char * argv[]){
	
	// Pipe Handling
	int fd[2];
	if(pipe(fd)==-1){
		perror("pipe");
		exit(1);
	}
	
	// Fork Handling
	pid_t pid = fork();
	
	if(pid==-1){
		perror("fork");
		exit(1);
	}
	
	// Child Process
	else if(pid==0){
		while((dup2(fd[1],STDOUT_FILENO)==-1)&&(errno==EINTR)){}
		close(fd[1]);
		close(fd[0]);
	
		// execute program if at least one argument
		if((argc) > 0){
			execvp(argv[0],argv);
		}
		
		// This code won't run if execvp is successful
		perror("execv");
		send(sock,strerror(errno),MAX,0);
		exit(1);
	}
	
	close(fd[1]);
	char buffer[MAX];
	
	// Send Result to sock
	while(1){
		ssize_t count = read(fd[0],buffer,sizeof(buffer));
		if(count==-1){
			if(errno==EINTR){
				continue;
			} else {
				perror("read");
				exit(1);
			}
		}else if(count==0){
			break;
		} else {
			send(sock,buffer,MAX*4,0);
		}
	}
	close(fd[0]);
	wait(0);
}

// list(int sock, char * buf): Int
// Parses buf for arguments and passes them to execsys.
int list(int sock, char * buf){
	int argc = argcount(buf);
	printf("%i arguments.\n",argc);
	char * tmp;
	if(argc>0){
		int i = 1;
		char * argv[argc];
		argv[0]="ls";
		tmp = strtok(buf," ");
		while(tmp != NULL){
			tmp = strtok(NULL," ");
			argv[i++]=tmp;
		}
		for(i=0;i<argc;i++){
			printf("%s ",argv[i]);
		}
		printf("\n");
		
		char *send[argc];	
		execsys(sock,argc,argv);
	}
	return 0;
}

// get(int sock, char * buf): Int
// Opens a local file specified by buf and sends it to the client.
int get(int sock, char * buf){
	int argc = argcount(buf);
	printf("%i arguments.\n",argc);
	char * tmp;
	int f=0;
	int l=0;	
	char * addr;
	
	// If at least two arguments in buf:
	if(argc>1){
		int i = 0;
		char * argv[argc];
		tmp = strtok(buf," ");
		
		// Split buf into array
		while(tmp != NULL){
			argv[i++]=tmp;
			tmp = strtok(NULL," ");
		}
		
		for(i=0;i<argc;i++){
			printf("%s ",argv[i]);
		}
		
		printf("\n");		
		
		FILE * f;
		f = fopen(argv[1],"r");
		
		// If specified file exists, send the contents to the client.
		if(f){
			char buffer[MAX];
			while(fgets(buffer,MAX,f)){
				send(sock,buffer,MAX,0);
			}
			// Specify when stream has ended.
			send(sock,"END_OF_STREAM",MAX,0);
		}
		else{
			perror("file");
			send(sock,"FILE_NOT_FOUND",MAX,0);
			return 1;
		}
		printf("\n");
	}
	else {
		send(sock,"ARGUMENT_ERROR",MAX,0);
		return 1;
	}
	return 0;
}

// put(int sock, char * buf): Int
// Accepts a filestream from the client and stores it in a local file
int put(int sock, char * buf){
	char fstream[MAX] = "";
	int i=0;
	char * pos;
	char * tmp;
	int force=0;
	FILE * f;
	int can_access=1;
	int argc=argcount(buf);
	
	if(argc > 1){
		char * argv[argc];
		tmp = strtok(buf," ");
		// Read args into array
		while(tmp != NULL){
			argv[i++]=tmp;
			tmp = strtok(NULL," ");
		}
		
		char * dest=argv[1];
		
		// If more than two argumnents
		if (argc > 2){
			// If index [2] is -f
			if(strstr(argv[2],"-f")!=NULL){
				force=1;
				if(argc>3){
					dest = argv[3];
					// Strip newline character
					cnl(argv[3]);
				}
			}
			// Otherwise it must be a filename
			else{
				dest = argv[2];
				cnl(argv[2]);
			}				
		}
		
		// Decide if dest can be written to 
		if(force==0){
			f = fopen(dest,"r");
			if(f){
				can_access=0;
				printf("File already exists! use -f to overwrite.\n");
				send(sock,"File already exists! use -f to overwrite.",MAX,0);
				fclose(f);
			}
		}
		
		// If yes, write to dest
		if(can_access) {
			f=fopen(dest,"w");
			send(sock,"File opened successfully.",MAX,0);
			while(1){
				recv(sock,fstream,sizeof(fstream),0);
				if(strstr(fstream,"END_OF_STREAM")==&fstream[0]) break;
				fprintf(f,"%s",fstream);
			}	
		}
	}
	return 0;
}

// sys(int sock, char * buf): Int
// Sends the system information of the server to the client.
int sys(int sock, char * buf){
	struct utsname sysinfo;
	int inf = uname(&sysinfo);
	if(inf==0){
		char sysdata[MAX]="";
		strcat(sysdata,"\nSystem Name: ");
		strcat(sysdata,sysinfo.sysname);
		
		strcat(sysdata,"\nNode Name: ");
		strcat(sysdata,sysinfo.nodename);
		
		strcat(sysdata,"\nRelease Info: ");
		strcat(sysdata,sysinfo.release);
		
		strcat(sysdata,"\nVersion Info: ");
		strcat(sysdata,sysinfo.version);
		
		strcat(sysdata,"\nMachine Info: ");
		strcat(sysdata,sysinfo.machine);

		send(sock,sysdata,MAX,0);
		
	}else if(inf == -1){
		send(sock,strerror(errno),MAX,0);
	}	
	return 0;
}

// delay(int sock, char * buf): Int
// Delays the client for the number of seconds provided by 'buf'.
int delay(int sock, char * buf){
	char * tmp;
	tmp = strtok(buf," \n");
	tmp = strtok(NULL," \n");
	int sec = atoi(tmp);
	usleep(1000000*sec);
	fflush(stdout);
	if(sec==0)
		send(sock,"Invalid integer submitted!",MAX,0);
	else
		send(sock,tmp,MAX,0);
}

// error(int sock, char * buf)
// Sends an invalid function message to the client.
int error(int sock, char * buf){
	send(sock,strerror(errno),MAX,0);
	return 0;
}