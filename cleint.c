#include <stdio.h> // header for input and output from console : printf, perror
#include<string.h> // strcmp
#include<sys/socket.h> // for socket related functions
#include<arpa/inet.h> // htons
#include <netinet/in.h> // structures for addresses

#include<unistd.h> // contains fork() and unix standard functions
#include<stdlib.h>


#include<unistd.h> // header for unix specic functions declarations : fork(), getpid(), getppid()
#include<stdlib.h> // header for general fcuntions declarations: exit()

#include<dirent.h>
#ifndef MAX_BUF
#define MAX_BUF 200
#endif



int main()
{
	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	if(server_sd<0)
	{
		perror("socket:");
		exit(-1);
	}
	//setsock
	int value  = 1;
	setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(6000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP

	//connect
    if(connect(server_sd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect");
        exit(-1);
    }
	
	//accept
	char buffer[256];

	printf("Connected to server\n");
	while(1)
	{
		//tell user to enter a message
		printf("ftp> ");
		fgets(buffer,sizeof(buffer),stdin);
       	buffer[strcspn(buffer, "\n")] = 0;  //remove trailing newline char from buffer, fgets does not remove it
       	//if the message sent is BYE! then disconnect for server and terminate

       if(strcmp(buffer,"!LIST")==0)
        {

	        DIR *d;
			struct dirent *dir;
			d = opendir(".");
			if (d) 
			{
				while ((dir = readdir(d)) != NULL) 
				{
			    	printf("%s\n", dir->d_name);
				}
			    closedir(d);
			}
        }

        if(strcmp(buffer,"!PWD")==0)
        {

	       	char path[MAX_BUF];
		    getcwd(path, MAX_BUF);
		    printf("%s\n", path);

		}

		//char * token = strtok(buffer, " ");
       	if(strncmp(buffer,"!CWD",4)==0)
        {
        	char temp_buffer[256];
			strcpy(temp_buffer,buffer);
			char s[] = " ";
			char* name = strtok(temp_buffer,s);
			name = strtok(NULL,s);

			//report if dir name not provided
			if (name==NULL){
				printf("no directory name provided\n");
				continue;
			}
			chdir(name);
			//while(token != NULL ) 
			//{
				//printf( " %s\n", token ); //printing each token
			//	chdir(token);
			    //printf( " %s\n", token ); //printing each token
			//    token = strtok(NULL, " ");
			//}
        }

        if(strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0 || strncmp(buffer, "LIST", 4)==0)
		{
			char* port;
			send(server_sd,"PORT",4,0);

			struct sockaddr_in cliaddr;
		    bzero(&cliaddr,sizeof(cliaddr));
		    unsigned int len = sizeof(cliaddr);
		    char* client_ip = inet_ntoa(cliaddr.sin_addr);
		    
		    //change dots to commas
		    int i;
			int length = strlen(client_ip);
			
			for (i = 0; i < length; i++) 
			{
			    if (client_ip[i] == '.') 
			    {
			        client_ip[i] = ',';
			        //i = length; // or `break;`
			    }
			}
		    // convert port to p1 and p2
		    char p1 = ntohs(cliaddr.sin_port)/256;
		    char p2 = ntohs(cliaddr.sin_port)%256;
		    //concetenate it into client ip
		    strcat(client_ip,&p1);
		    strcat(client_ip,&p2);
		    send(server_sd,client_ip,sizeof(client_ip),0);



		}

       	if(strncmp(buffer,"USER",4)==0)
        {
        	//send
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recieve
			int rec_bytes = recv(server_sd,buffer,sizeof(buffer),0);
			
			//print
			printf("%s\n", buffer);
        }



		
		if (strncmp(buffer,"PASS",4)==0)
		{

			//send
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recieve
			int rec_bytes = recv(server_sd,buffer,sizeof(buffer),0);
			
			printf("%s\n", buffer);
		}

		if (strncmp(buffer,"PWD",3)==0){
			//send
			send(server_sd,buffer,sizeof(buffer),0);

			//receive
			char request_buff[MESSAGE_BUFFER_SIZE];
			bzero(request_buff,sizeof(request_buff));
			int rec_bytes = recv(server_sd,request_buff,sizeof(request_buff),0);
			
			printf("%s\n",request_buff);
		}

		if (strncmp(buffer,"CWD",3)==0){
			//send
			send(server_sd,buffer,sizeof(buffer),0);
			
			//receive
			char request_buff[MESSAGE_BUFFER_SIZE];
			bzero(request_buff,sizeof(request_buff));
			int rec_bytes = recv(server_sd,request_buff,sizeof(request_buff),0);
			
			printf("%s\n",request_buff);
		}



		//old codeee
       if(strcmp(buffer,"QUIT")==0)
        {

        	//printf("closing the connection to server \n");
        	send(server_sd,buffer,strlen(buffer),0);
        	close(server_sd);
            break;
        }
        //////

        if(send(server_sd,buffer,strlen(buffer),0)<0)
        {
            perror("send");
            exit(-1);
        }
        bzero(buffer,sizeof(buffer));
        recv(server_sd,buffer,sizeof(buffer),0);
        //print the recieved message from the server
        printf("Recieved message: %s \n",buffer);

	}

	return 0;
}
