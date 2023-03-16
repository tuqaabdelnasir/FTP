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


//Functon to find file size
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
	setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); 
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(7000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//connect
    if(connect(server_sd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect");
        exit(-1);
    }
	
	char buffer[1024];
	printf("220 Service ready for new user.\n");


	while(1)
	{	
		bzero(buffer,sizeof(buffer));
		printf("ftp> ");
		fgets(buffer,sizeof(buffer),stdin);
       	buffer[strcspn(buffer, "\n")] = 0;  

       	//USER command
       	if(strncmp(buffer,"USER",4)==0)
        {
        	//send
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recieve
			recv(server_sd,buffer,sizeof(buffer),0);
			
			//print
			printf("%s\n", buffer);	
			
        }


        //PASS command
		else if (strncmp(buffer,"PASS",4)==0)
		{

			//send
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recieve
			recv(server_sd,buffer,sizeof(buffer),0);
			//print
			printf("%s\n", buffer);	
			
		}
       	

		//!LIST command
		else if(strcmp(buffer,"!LIST")==0)
	    {
	    	system("ls");
	    }


	    //!PWD command
	    else if(strcmp(buffer,"!PWD")==0)
	    {
	    	system("pwd");

	    }

		//!CWD command
	    else if(strncmp(buffer,"!CWD",4)==0)
	   {
	        	char temp_buffer[256];
				strcpy(temp_buffer,buffer);
				char s[] = " ";
				char* name = strtok(temp_buffer,s);
				name = strtok(NULL,s);

				//report if directory name isn't provided
				if (name==NULL){
					printf("no directory name provided\n");
					continue;
				}
				chdir(name);
				
	    }

	    //if STOR RETR LIST are used
	    else if(strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0 || strncmp(buffer, "LIST", 4)==0)
		{		
				
				//printf("hllo");
				//if statement incase STOR RETR doesn't have an argument
				if (strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0)
				{
					char nf_buff[4200];
					strcpy(nf_buff,buffer);
					char delim[] = " ";
					char* no_filename = strtok(nf_buff,delim);
					no_filename = strtok(NULL,delim);

					if (no_filename==NULL)
					{
						printf("Please provide a filename\n");
						continue;
					}
				}

				//fork here
				int state;
				int p_id=fork();
				if (p_id==0)
				{

					char* port;
					char port_req[256];
					char port_ack[256];
					//send PORT command
					//send(server_sd,"PORT",4,0);
					int channel=1;
					bzero(port_ack,sizeof(port_ack));
					//recv ack that it is recieved
					recv(server_sd,port_ack,sizeof(port_ack),0);
					//printf("%s\n", port_ack);
					
					//create the data socket
					int client_data_sock = socket(AF_INET,SOCK_STREAM,0);
					if (client_data_sock<0)
					{
						perror("data socket error: ");
						continue;
					}
					setsockopt(client_data_sock,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value));

					//client details
					struct sockaddr_in curr_addr;
				    bzero(&curr_addr,sizeof(curr_addr));
				    socklen_t len = sizeof(curr_addr);
				    getsockname(server_sd,(struct sockaddr*)&curr_addr,&len);
				    int client_port = (int)ntohs(curr_addr.sin_port)+(channel++);
				    char* client_ip = inet_ntoa(curr_addr.sin_addr);


				

				    //bind to data socket
				    if (bind(client_data_sock,(struct sockaddr *)&curr_addr, sizeof(curr_addr)) < 0)
					{	
					 	
	                	perror("bind error:");
	                	continue;
	                }

	                //listen on socket
	                if (listen(client_data_sock, 5) < 0)
		            {
		                perror("listen");
		                close(client_data_sock);
						continue;
		            }

		            if (send(server_sd, "port ok\n", strlen("port ok\n"),0)<0){
		            	perror("send");
		            	exit(-1);
		            }

				    //printf("%s\n",buffer);
				    //change dots to commas
				    int i;
					int length = strlen(client_ip);
					for (i = 0; i < length; i++) 
					{
					    if (client_ip[i] == '.') 
					    {
					        client_ip[i] = ',';
					        
					    }
					}
				    // convert port to p1 and p2
				    int p1 = client_port/256;
				    int p2 = client_port%256;
				    //concetenate it into client ip
				    sprintf(port_req,"%s,%d,%d",client_ip,p1,p2);
				    //send it to the server
				    //send(server_sd,port_req,sizeof(port_req),0);

					//SERVER DETAiLS
					struct sockaddr_in data_addr;
					bzero(&data_addr,sizeof(data_addr));
					data_addr.sin_family = AF_INET;
					data_addr.sin_port = htons(6000);
					data_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP
					socklen_t s_length=sizeof(data_addr);
					

					//saccept here with the server address
					int server_data_sock = accept(client_data_sock, (struct sockaddr *)&data_addr, &s_length);
					//int check=recv(server_sd,port_ack,sizeof(port_ack),0);




					//if command is LIST
					printf("%s\n",buffer);
					printf("hi!");
					if (strncmp(buffer,"LIST",4)==0)
					{	
						printf("ack?\n");
						
						//send LIST on control socket
						recv(server_sd,buffer,sizeof(buffer),0);
						printf("is this\n");
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

						
					//if command is RETR
					if(strncmp(buffer, "RETR", 4)==0)
					{

						//filename
						char request_buffer[256];
						strcpy(request_buffer,buffer);
						char s[] = " ";
						char* filename = strtok(request_buffer,s);
						filename = strtok(NULL,s);
						
						//send
						send(server_sd,buffer,sizeof(buffer),0);
						
						//receive
						char check_data_buff[256];
						recv(server_data_sock,check_data_buff,sizeof(check_data_buff),0);
						printf("%s\n",check_data_buff);
						if (strncmp(check_data_buff,"550",3)==0)
						{
							close(check_data_buff);
							continue;
						}

						//array to receive file
						char recv_file_buff[256];

						//temporary filename
						char temp_fname[256];
						sprintf(temp_fname,"temp%d.dat",channel);

						//writable check
						FILE* file_recv = fopen(temp_fname, "w");
						if (file_recv == NULL) 
						{
							continue;
						}
						fclose(file_recv);

						
						file_recv = fopen(temp_fname, "a+");

						//tracking received bytes
						int bytes_received = recv(server_data_sock, recv_file_buff, sizeof(recv_file_buff), 0);
						if (bytes_received <= 0)
						{
							continue;
						}
						while (bytes_received > 0) 
						{
							//writes to the file
							fwrite(recv_file_buff, bytes_received, 1, file_recv);

							if (bytes_received<2048)
							{
								break;
							}
							bytes_received = recv(server_data_sock, recv_file_buff, sizeof(recv_file_buff), 0);
						}
						//closing of data socket
						close(server_data_sock);
						fclose(file_recv);
						rename(temp_fname,filename);

						bzero(request_buffer,sizeof(request_buffer));
						recv(server_sd,request_buffer,sizeof(request_buffer),0);
						printf("%s\n",request_buffer);
					}


					//if command is STOR
					if(strncmp(buffer, "STOR", 4)==0)
					{


						char request_buffer[4200];
						strcpy(request_buffer,buffer);
						char delim[] = " ";
						char* filename = strtok(request_buffer,delim);
						filename = strtok(NULL,delim);

						//send 
						int file_size = fsize(filename);
						if (file_size==-1)
						{
							send(server_sd,"STOR",4,0);
						}
						else
						{
							send(server_sd,buffer,sizeof(buffer),0);
						}
						
						//recv
						char check_data_buff[4200];
						recv(server_data_sock,check_data_buff,sizeof(check_data_buff),0);
						printf("%s\n",check_data_buff);
						if (strncmp(check_data_buff,"550",3)==0)
						{
							close(check_data_buff);
							continue;
						}


						if(file_size!=-1)
						{
							
							
							FILE* file_to_send = fopen(filename, "r");
							char *file_data = malloc(file_size);
							fread(file_data, 1, file_size, file_to_send);


							//tracking of bytes
							int bytes_sent = 0;
							int bites_for_iteration = 0;
							while(bytes_sent < file_size)
							{
								
								if(file_size - bytes_sent >= 2048)
								{
									bites_for_iteration = 2048;
								}
								
								else
								{
									bites_for_iteration = file_size - bytes_sent;
								}
								//send data
								send(server_data_sock, file_data + bytes_sent, bites_for_iteration, 0);
								//update bytes sent
								bytes_sent += bites_for_iteration;

							}
							close(server_data_sock);
							fclose(file_to_send);
						}
						bzero(request_buffer,sizeof(request_buffer));
						recv(server_sd,request_buffer,sizeof(request_buffer),0);
						printf("%s\n",request_buffer);

					}
				
				else{
					wait(&state);
					bzero(port_ack, sizeof(port_ack));
					int receive = recv(server_sd, port_ack, sizeof(port_ack), 0);

				}
				}


			}

			//if command is PWD
			else if (strncmp(buffer,"PWD",3)==0)
			{
				//send
				send(server_sd,buffer,sizeof(buffer),0);

				//receive
				char request_buffer[256];
				bzero(request_buffer,sizeof(request_buffer));
				recv(server_sd,request_buffer,sizeof(request_buffer),0);
				
				printf("%s\n",request_buffer);
			}

			//if command is CWD
			else if (strncmp(buffer,"CWD",3)==0)
			{
				//send
				send(server_sd,buffer,sizeof(buffer),0);
				
				//receive
				char request_buffer[256];
				bzero(request_buffer,sizeof(request_buffer));
				recv(server_sd,request_buffer,sizeof(request_buffer),0);
				
				printf("%s\n",request_buffer);
			}

			//if command is QUIT
	       else if (strcmp(buffer,"QUIT")==0)
	        {

	        	send(server_sd,buffer,strlen(buffer),0);
	        	close(server_sd);
	            break;
	        }


		}

	

       

	return 0;
}
