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

int fsize(FILE *fp)
{
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); 
    return sz;
}


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
	char buffer[4200];
	char log_in[3]="yes";


	while(1)
	{
		recv(server_sd,buffer,sizeof(buffer),0);
		printf("%s",buffer);
		//tell user to enter a message
		printf("ftp> ");
		fgets(buffer,sizeof(buffer),stdin);
       	buffer[strcspn(buffer, "\n")] = 0;  

       	if(strncmp(buffer,"USER",4)==0)
        {
        	//send
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recieve
			recv(server_sd,buffer,sizeof(buffer),0);
			
			//print
			printf("%s\n", buffer);
			if (strncmp(buffer,"331",3)!=0)
			{	
				strncpy(log_in, "no", 3);

			}
			
        }

		
		if (strncmp(buffer,"PASS",4)==0)
		{

			//send
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recieve
			recv(server_sd,buffer,sizeof(buffer),0);
			
			printf("%s\n", buffer);
			if (strncmp(buffer,"230",3)!=0)
			{	
				strncpy(log_in, "no", 3);

			}
			
		}
       	

		while (strncmp(log_in,"yes",3)==0)
		{
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
				
				//check missing filename argument and report
				if (strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0){
					char nf_buff[4200];
					strcpy(nf_buff,buffer);
					char delim[] = " ";
					char* no_filename = strtok(nf_buff,delim);
					no_filename = strtok(NULL,delim);

					if (no_filename==NULL){
						printf("Please provide a filename\n");
						continue;
					}
				}


				char* port;

				send(server_sd,"PORT",4,0);

				
				int channel;
				int rec_bytes = recv(server_sd,&channel,sizeof(channel),0);
				
				if (rec_bytes<=0){
					printf("Server has shutdown\n");
					return 0;
				}


				struct sockaddr_in curr_addr;
			    bzero(&curr_addr,sizeof(curr_addr));
			    unsigned int len = sizeof(curr_addr);
			    int client_port = ntohs(curr_addr.sin_port)+channel;
			    char* client_ip = inet_ntoa(curr_addr.sin_addr);
			    
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
			    char p1 = client_port/256;
			    char p2 = client_port%256;
			    //concetenate it into client ip
			    strcat(client_ip,&p1);
			    strcat(client_ip,&p2);

			    send(server_sd,client_ip,sizeof(client_ip),0);


			    //create socket for data exchange
				int client_data_sock = socket(AF_INET,SOCK_STREAM,0);

				if (client_data_sock<0)
				{
					perror("data sock: ");
					continue;
				}

				
				setsockopt(client_data_sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
				struct sockaddr_in data_addr;
				bzero(&data_addr,sizeof(data_addr));
				data_addr.sin_family = AF_INET;
				data_addr.sin_port = htons(6000);
				data_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP

				//connect
			    if(connect(client_data_sock ,(struct sockaddr*)&data_addr,sizeof(data_addr))<0)
			    {
			        perror("connect");
			        exit(-1);
			    }

			    //recieve ack of PORT request
			    recv(server_sd,buffer,sizeof(buffer),0);
				
				//socket to connect with server
				int server_data_sock = accept(client_data_sock, 0, 0);


				if (strncmp(buffer,"LIST",4)==0)
				{
					//send LIST on control socket
					send(server_sd,buffer,sizeof(buffer),0);

					//receive DATA OK
					char check_data_buff[4200];
					recv(server_data_sock,check_data_buff,sizeof(check_data_buff),0);
					printf("%s\n",check_data_buff);

					//receive and report the directory file info
					int size_incoming;
					char listBuff[4200];
					recv(server_data_sock,&size_incoming,sizeof(int),0);
					recv(server_data_sock,listBuff,size_incoming,0);
					printf("%s\n",listBuff);
					//close data socket upon file transmission
					close(server_data_sock);

					//receive LIST OK
					char request_buff[4200];
					bzero(request_buff,sizeof(request_buff));
					recv(server_sd,request_buff,sizeof(request_buff),0);
					printf("%s\n",request_buff);
				}

				//RETR

				if(strncmp(buffer, "RETR", 4)==0)
				{

					//get filename from request
					char request_buffer[256];
					strcpy(request_buffer,buffer);
					char s[] = " ";
					char* filename = strtok(request_buffer,s);
					filename = strtok(NULL,s);
					
					//send RETR request to server on control socket
					send(server_sd,buffer,sizeof(buffer),0);
					
					//receive DATA OK response
					char check_data_buff[256];
					recv(server_data_sock,check_data_buff,sizeof(check_data_buff),0);
					printf("%s\n",check_data_buff);
					if (strncmp(check_data_buff,"550",3)==0)
					{
						//close(check_data_buff);
						continue;
					}

					//buffer to receive file
					char recv_file_buff[256];
					//temporary filename
					char temp_fname[256];
					sprintf(temp_fname,"temp%d.dat",channel);

					//check if file writable and create new file
					FILE* file_recv = fopen(temp_fname, "w");
					if (file_recv == NULL) {
						continue;
					}
					fclose(file_recv);

					//file pointer to use for writing
					file_recv = fopen(temp_fname, "a+");

					//keep track of received bytes
					int bytes_received = recv(server_data_sock, recv_file_buff, sizeof(recv_file_buff), 0);
					
					// if nothing received, terminate action
					if (bytes_received <= 0){
						continue;
					}
					//while something is received
					while (bytes_received > 0) {
						//write to file
						fwrite(recv_file_buff, bytes_received, 1, file_recv);
						//stop if last transmission
						if (bytes_received<2048){
							break;
						}
						bytes_received = recv(server_data_sock, recv_file_buff, sizeof(recv_file_buff), 0);
					}
					//close data socket upon file transmission
					close(server_data_sock);
					//close file upon file transmission
					fclose(file_recv);
					//rename file to specified name
					rename(temp_fname,filename);

					//receive retrieve OK
					bzero(request_buffer,sizeof(request_buffer));
					recv(server_sd,request_buffer,sizeof(request_buffer),0);
					printf("%s\n",request_buffer);
			}


				//STOR
				if(strncmp(buffer, "STOR", 4)==0)
				{
				//get filename
				char request_buffer[4200];
				strcpy(request_buffer,buffer);
				char delim[] = " ";
				char* filename = strtok(request_buffer,delim);
				filename = strtok(NULL,delim);

				//send STOR req to server on control socket
				int file_size = fsize(filename);
				if (file_size==-1){
					send(server_sd,"STOR",4,0);
				}
				else{
					send(server_sd,buffer,sizeof(buffer),0);
				}
				
				//recv DATA OK response
				char check_data_buff[4200];
				recv(server_data_sock,check_data_buff,sizeof(check_data_buff),0);
				printf("%s\n",check_data_buff);
				if (strncmp(check_data_buff,"550",3)==0){
					//close(check_data_buff);
					continue;
				}
				
				


				if(file_size!=-1){
					
					//open file for read
					FILE* file_to_send = fopen(filename, "r");
					char *file_data = malloc(file_size);
					//read the entire file
					fread(file_data, 1, file_size, file_to_send);


					//keep track of bytes sent and bytes for the iteration
					int bytes_sent = 0;
					int bites_for_iteration = 0;
					while(bytes_sent < file_size)
					{
						//if remaining file size is greater than packet size, then send packet sized data
						if(file_size - bytes_sent >= 2048){
							bites_for_iteration = 2048;
						}
						//else send remaining data
						else{
							bites_for_iteration = file_size - bytes_sent;
						}
						//send data on the data socket
						send(server_data_sock, file_data + bytes_sent, bites_for_iteration, 0);
						//update sent bytes
						bytes_sent += bites_for_iteration;

					}
					//close data socket after file transmission
					close(server_data_sock);
					//close file after file transmission
					fclose(file_to_send);
				}
				//recv STOR response
				bzero(request_buffer,sizeof(request_buffer));
				recv(server_sd,request_buffer,sizeof(request_buffer),0);
				printf("%s\n",request_buffer);
			}


		}


			if (strncmp(buffer,"PWD",3)==0){
				//send
				send(server_sd,buffer,sizeof(buffer),0);

				//receive
				char request_buffer[256];
				bzero(request_buffer,sizeof(request_buffer));
				recv(server_sd,request_buffer,sizeof(request_buffer),0);
				
				printf("%s\n",request_buffer);
			}

			if (strncmp(buffer,"CWD",3)==0){
				//send
				send(server_sd,buffer,sizeof(buffer),0);
				
				//receive
				char request_buffer[256];
				bzero(request_buffer,sizeof(request_buffer));
				recv(server_sd,request_buffer,sizeof(request_buffer),0);
				
				printf("%s\n",request_buffer);
			}



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
	}

		printf("You are not signed in!");
	

       

	return 0;
}
