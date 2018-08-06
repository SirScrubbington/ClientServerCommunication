//
// Name: client.c 
// Author: Damon Murdoch (s290548)
// Date: 24/08/2017
// Description: Client-side application for 2803ICT 
//

#include "common.c"

int get();
int put();
int list();


int main(int argc, char *argv[])
{	
	// Program Loop
	for(;;){
		char buf[MAX]="";
		char rcv[MAX]="";
		struct timeval t1,t2;
		
		fflush(stdin);
		
		// Accept Command
		printf("Command: ");
		fgets(buf,MAX,stdin);
		
		cnl(buf);
		
		printf("\n");		
		
		if(strstr(buf,"quit")==&buf[0]){
			exit(0);
		}
		
		// get Starting Time
		gettimeofday(&t1,NULL);
		
		// Create Socket		
		struct sockaddr_in server;
		int sock = socket(AF_INET,SOCK_STREAM,0);
		if(sock<0)
		{
			printf("Server cannot be contacted!\n");
			return 1;
		}
		
		// Create Host Entity
		struct hostent * hp = gethostbyname(argv[1]);
		bcopy(hp->h_addr,&(server.sin_addr.s_addr),hp->h_length);
		server.sin_family=AF_INET;
		//server.sin_port=htons(atoi(argv[2]));
		server.sin_port=80;
		
		// Connect to socket
		int conn = connect(sock,(struct sockaddr*)&server,sizeof(server));
		if(conn < 0){
			printf("Could not connect to server!");
			exit(1);
		}
		
		// List Command-Specific
		if(strstr(buf,"list")==&buf[0]){
			list(buf,sock);
		}
		
		// Other commands
		else if(strstr(buf,"get")==&buf[0]){
			send(sock,buf,MAX,0);
			get(buf,sock);
		}
		
		else if(strstr(buf,"put")==&buf[0]){
			send(sock,buf,MAX,0);
			put(buf,sock);
		}
		
		else {
			send(sock,buf,MAX,0);
			recv(sock,rcv,sizeof(rcv),0);
			if(strstr(buf,"sys")==&buf[0]){
				printf("Server Response: \n%s\n",rcv);
			}else{
				printf("Server Response: %s\n",rcv);
			}
		}
		
		// get second time
		gettimeofday(&t2,NULL);
		
		// Get response time
		int ms = (t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_usec - t1.tv_usec)/1000;
		printf("Delay: %i ms\n",ms);
		
		// Close the connection
		close(sock);
	}
	return 0;
}

// list(char * buf,int sock): Int
// Parses buf for arguments, submits them to the server and prints the response to stdout or a file.
int list(char * buf, int sock){
	int argc = argcount(buf);
	char * argv[argc];
	int force = 0;
	int can_access=1;
	char rcv[MAX*4];
	char * tmp;
	char * dest=NULL;
	char * cmd = (char *) malloc(MAX);
	int args = 0;
	FILE * f;
	int i=0;
	int status;
    struct stat st_buf;
	
	tmp=strtok(buf," ");
	
	// Split buf into array
	while(tmp!=NULL){
		argv[i++]=tmp;
		tmp=strtok(NULL," ");
	}	

	// Identify Arguments
	for(i=0;i<argc;i++){
		status = stat (argv[i], &st_buf);
		
		// function call
		if(strstr(argv[i],"list")){
			strcpy(cmd,"ls");
			args++;
		}		
		
		// long operator
		if(strstr(argv[i],"-l")){
			strcat(cmd," -l");
			args++;
		}
		
		// force operator
		else if(strstr(argv[i],"-f")){
			force=1;
			args++;
		}
		
		// target file
		else if (strstr(argv[i],".")){
			dest = argv[i];
		}
		
		// directory to list
		else if (S_ISDIR (st_buf.st_mode)) {
			strcat(cmd," ");
			strcat(cmd,argv[i]);
		}
	}

	// Send the command to the server and recieve the response
	send(sock,cmd,MAX,0);
	recv(sock,rcv,sizeof(rcv),0);
	
	// If a destination file is provided
	if(dest){
		// If a file cannot be overwritten
		if(force==0){
			f = fopen(dest,"r");
			if(f){
				can_access=0;
				printf("Access denied! Use '-f' to override existing files.\n");
			}
			fclose(f);
		}
		if(can_access){
			f = fopen(dest,"w");
		}
		else printf("failed to open '%s'\n",dest);
		// If file exists, writ to it and close
		if(f){
			fputs(rcv,f);
			fclose(f);
		}else printf("%s\n",strerror(errno));
	} else {
		int l=0;
		char * token;
		token = strtok(rcv,"\n");
		while(token != NULL){
			if(l>39){
				printf("Press Enter to Continue\n");  
				getchar();
				l=0;
			}
			l++;
			printf("%s\n",token);
			token = strtok(NULL,"\n");
		}		
	}
	return 0;
}

// put(char * buf, int sock): Int
// Sends a filestream to be stored on the server, with an optional new filename.
int put(char * buf, int sock){	
	int i=0;
	int force=0;
	int can_access=1;
	int argc=argcount(buf);
	char * tmp;
	char * dest="NONE";
	char * argv[argc];
	char sendstr[MAX]="";
	
	FILE * f;

	if(argc > 1){
		tmp = strtok(buf," ");
		while(tmp != NULL){
			argv[i++]=tmp;
			tmp = strtok(NULL," ");
		}
		
		FILE * f;
		f = fopen(argv[1],"r");
		
		
		if(f){
			printf("Opened %s ",argv[1]);
			char sendstr[MAX];
			char rcv[MAX];
			recv(sock,rcv,MAX,0);
			if(strstr(rcv,"File already exists! use -f to overwrite.")){
				printf(rcv);
				return 0;
			}
			while(fgets(sendstr,MAX,f)){
				send(sock,sendstr,MAX,0);
			}
			send(sock,"END_OF_STREAM",MAX,0);
		}
		else{
			perror("file");
			send(sock,strerror(errno),MAX,0);
			return 1;
		}
		printf("\n");
	}	
	else printf("Not enough arguments!\n");
	return 0;
}

// get(char * buf, int sock): Int
// Gets a file from the server specified by buf and displays or stores it on the local machine.
int get(char * buf, int sock){
	char rcv[MAX]="";
	int i=0;
	
	char * tmp;
	char * dest="NONE";
	int force=0;
	FILE * f;
	int can_access=1;
	int argc=argcount(buf);
	char * argv[argc];
	tmp = strtok(buf," ");
	if (argc > 1){
		while(tmp != NULL){
			argv[i++]=tmp;
			tmp = strtok(NULL," ");
		}
		if (argc > 2){
			if(strstr(argv[2],"-f")!=NULL){
				force=1;
				if(argc>3){
					dest = argv[3];
					cnl(dest);
				}
			}
			else{
				
				dest = argv[2];
				cnl(dest);
			}				
		}		
		if(strstr(dest,"NONE")!=&dest[0]){
			if(force==0){
				f = fopen(dest,"r");
				if(f){
					can_access=0;
				printf("Access denied! Use '-f' to override existing files.\n");
				}
				fclose(f);
			}		
			if(can_access){
				f = fopen(dest,"w");
		
			}else printf("failed to open '%s'\n",dest);
		}
		while(1){
			recv(sock,rcv,sizeof(rcv),0);
			if(strstr(rcv,"END_OF_STREAM")==&rcv[0]){
				break;
			}
			if(strstr(rcv,"FILE_NOT_FOUND")==&rcv[0]){
				printf("The requested file could not be opened!\n");
				break;
			}
			if(f){
				if(force || can_access) fprintf(f,"%s",rcv);
			}else{
				printf("%s",rcv);
			}
		}
		if(f) fclose(f);
	}
	else printf("Not enough arguments!\n"); 
	return 0;
}