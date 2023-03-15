//============================================================================
// Name         : Chat Server using Select()
// Description  : This Program will receive messages from several clients using
//                select() system class
//============================================================================
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/select.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h> 
#include <stdarg.h>


// dirent struct reference https://www.ibm.com/docs/en/zos/2.2.0?topic=functions-readdir-read-entry-from-directory

// port_no save it in client_port()
// send ack
// stor/recv
//fork
// clientaddr == client_port
// transfersocket==socket()

#define MAX_USERS 10
struct User {
    char user[50];
    char password[50];
    char address[50]; // this could be the IP address 
    int is_authenticated;
    int client_fd; 
};

struct User userlist[MAX_USERS] = {
    {} // example user from decription / testing
};

int num_users = 1;

int user(char *username, int client_fd) {

    for (int i = 0; i < num_users; i++) {
        if (strcmp(userlist[i].user, username) == 0) {
            userlist[i].is_authenticated = 0; // reset authentication status
            userlist[i].client_fd = client_fd; // save client
            send(client_fd, "331 Username OK, need password.\n", strlen("331 Username OK, need password.\n"), 0);
            return 0;
            
        }
    }
    send(client_fd, "530 Not logged in.\n", strlen("530 Not logged in.\n"), 0);
    return 0;
}

int pass(char *password, int client_fd) {
    for (int i = 0; i < num_users; i++) {
        if (userlist[i].is_authenticated==0 && userlist[i].client_fd == client_fd) {
            if (strcmp(userlist[i].password, password) == 0) {
                userlist[i].is_authenticated = 1; // authenticated now!
                if (client_fd > 0) {
                send(client_fd, "230 User logged in, proceed.\n", strlen("230 User logged in, proceed.\n"), 0);
                }
                return 0;
            } else {
                send(client_fd, "530 Not logged in.\n", strlen("530 Not logged in.\n"), 0);
                return 0;
            }
        }
    }

    send(client_fd, "503 Login with USER first.\n", strlen("503 Login with USER first.\n"), 0);
    return 0;
}




int cwd(char *path, int client_fd){

	if (chdir(path) == 0){
		char message[256];
		snprintf(message, sizeof(message), "200 directory changed to %s\n", path);
		send(client_fd, message, strlen(message), 0);
		return 0;
	}
	else{
		char message[256];
		snprintf(message, sizeof(message), "550 No such file or directory\n");
		send(client_fd, message, strlen(message), 0);
		return 0;
	}
}

int pwd(char *path, int client_fd){

	if (getcwd(path, sizeof(path)) == 0){
		char message[256];
		snprintf(message, sizeof(message), "550 No such file or directory\n");
		send(client_fd, message, strlen(message), 0);
	}
	else{
		char message[256];
		snprintf(message, sizeof(message), "257 \"%s\"\n", getcwd(path, sizeof(path)));
		send(client_fd, message, strlen(message), 0);
		return 1;
	}
	return 0;
}

int list(int client_fd){

	DIR *dir = opendir(".");
	if (dir == NULL){
		char message[256];
		snprintf(message, sizeof(message), "error opening directory\n");
		send(client_fd, message, strlen(message), 0);
	}

	char message[256];  
    struct dirent* filename;
  
    while ((filename = readdir(dir)) != NULL) {
        if (strcmp(filename->d_name, ".") == 0 || strcmp(filename->d_name, "..") == 0) {
            continue;  // skip . .. directory --> causes error
        }
        char printt[256];
        snprintf(printt, sizeof(printt), "%s\n", filename->d_name);
        snprintf(message, sizeof(message), "200 PORT command successful.\n150 File status okay; about to open. data connection.\n%s", printt);
        send(client_fd, message, strlen(message), 0);
    }

    // close directory
    closedir(dir);
    return 0;
}


int stor(int client_fd, char *filename){
	int file_transfer = open(filename, O_CREAT | O_WRONLY, 0644);
    if (file_transfer < 0) {
        char message[256];
        snprintf(message, sizeof(message), "Error with file\n");
        send(client_fd, message, strlen(message), 0);
        return 1;
    }

    char buffer[256];

    int byter;
    int bytew;

    byter = recv(client_fd, buffer, sizeof(buffer), 0);

    while (byter > 0) {
        bytew = write(file_transfer, buffer, byter);
        if (bytew < 0) {
            char message[256];
            snprintf(message, sizeof(message), "Error occurred\n");
            send(client_fd, message, strlen(message), 0);
            break;
        }
    }

    close(file_transfer); 
    return 0;

}


int retr(int client_fd, char *filename){

    int file_transfer = open(filename, O_RDONLY);
    if (file_transfer < 0) {
        send(client_fd, "550 No such file or directory", strlen("550 No such file or directory"), 0);
        return 1;
    }

    char buffer[256];

    int byter;
    int byte_download;

    byter = read(file_transfer, buffer, sizeof(buffer));

    while (byter > 0) {
        byte_download = send(client_fd, buffer, byter, 0);
        if (byte_download < 0) {
            send(client_fd, "451 Requested action aborted: local error in processing", strlen("451 Requested action aborted: local error in processing"), 0);
            break;
        }
        byter = read(file_transfer, buffer, sizeof(buffer));
    }

    close(file_transfer); 
    send(client_fd, "226 Closing data connection. Requested file action successful", strlen("226 Closing data connection. Requested file action successful"), 0);
    return 0;
}

int client_port;
// commands from the client, tokenised reference: https://www.tutorialspoint.com/string-tokenisation-function-in-c

int commands(int client_sd){

	char data_command[1024];
    char clientr[256];
    //while(1){
        memset(data_command, 0, sizeof(data_command));
       	recv(client_sd, data_command, sizeof(data_command), 0);
        //printf("%s\n", clientr);

		if (clientr < 0) {
	        perror("Error");
	        return 0;
	    }
	     
	    // split between command and data using tokens
	    char *command = strtok(data_command, " ");
	    char *data = strtok(NULL, "\n");
	    printf("%s\n", command);

	    if (command == NULL) {
	        send(client_sd, "500 Syntax error, command unrecognized.\n", strlen("500 Syntax error, command unrecognized.\n"), 0);
	        return 1;
	    }

	    //-----USER(username)-----
	    if (strcmp(command, "USER") == 0) {

	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        user(data, client_sd);
	  
	    } 

	    //-----PASS(password)-----
	    else if (strcmp(command, "PASS") == 0) {
	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        pass(data, client_sd);


	    } else if (strcmp(command, "CWD") == 0) {
	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        cwd(data, client_sd);
	    } else if (strcmp(command, "PWD") == 0) {
	        if (data != NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        pwd(data,client_sd);
	    } 

	    else if (strcmp(command, "LIST\r\n") == 0) {
	        list(client_sd);
	    } 

	    else if (strcmp(command, "STOR") == 0) {
	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        stor(client_sd, data);
	    } else if (strcmp(command, "RETR") == 0) {
	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        retr(client_sd, data);

	    } else if (strcmp(command, "QUIT\r\n") == 0) {
	            send(client_sd, "221 Service closing control connection.\n", strlen("221 Service closing control connection.\n"), 0);
	            //close(client_sd);
	            return 1;
	    

		} else if (strcmp(command, "QUIT\r\n") == 0) {
	            char* h1= strtok(data, ",");
	            char* h2= strtok(NULL, ",");
	            char* h3= strtok(NULL, ",");
	            char* h4= strtok(NULL, ",");
	            char* p1= strtok(NULL, ",");
	            char* p2= strtok(NULL, "\n"); // last one

	            send(client_sd, "200 PORT command successful.\n", strlen("200 PORT command successful.\n"), 0);
	        return 1;


	    
	    } else {
	        send(client_sd, "500 Syntax error, command unrecognized.\n", strlen("500 Syntax error, command unrecognized.\n"), 0);
	        return 1;
	    }
	   return 0;
	}



int handle_port(int client_fd) {
    // Create a socket 

    int data_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (data_fd < 0) {
        printf("Error creating data socket\n");
        return -1;
    }

    // Set up the data connection endpoint
    struct sockaddr_in data_addr;
    bzero(&data_addr, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    data_addr.sin_port = htons(7000);

    
    if (bind(data_fd, (struct sockaddr*) &data_addr, sizeof(data_addr)) < 0) {
        perror("bind failed");
		exit(-1);

    // l

    if (listen(client_fd, 5) < 0) {
    	perror("listen failed");
		close(data_fd);
		exit(-1);
	}
        
    }

}

int main()
{
	
	FILE *fp = fopen("users.txt", "r");
    if (fp == NULL) {
        perror("Error opening file");
        exit(1);
    }

    int i = 0; 
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *username = strtok(line, " ");
        char *password = strtok(NULL, "\n");
        if (username != NULL && password != NULL) {
            struct User user = {0};
            strcpy(user.user, username);
            strcpy(user.password, password);
            user.is_authenticated = 0;
            user.client_fd = -1;
            strcpy(user.address, "");
            userlist[i++] = user;
            
        }
    }

    fclose(fp);



	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	//printf("Server fd = %d \n",server_sd);
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
	server_addr.sin_port = htons(7000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //INADDR_ANY, INADDR_LOOP

	//bind
	if(bind(server_sd, (struct sockaddr*)&server_addr,sizeof(server_addr))<0)
	{
		perror("bind failed");
		exit(-1);
	}
	//listen
	if(listen(server_sd,5)<0)
	{
		perror("listen failed");
		close(server_sd);
		exit(-1);
	}
	
	fd_set full_fdset;
	fd_set read_fdset;
	FD_ZERO(&full_fdset);

	int max_fd = server_sd;

	FD_SET(server_sd,&full_fdset);

	printf("Server is listening...\n");
	while(1)
	{
		//printf("max fd = %d \n",max_fd);
		read_fdset = full_fdset;

		if(select(max_fd+1,&read_fdset,NULL,NULL,NULL)<0)
		{
			perror("select");
			exit (-1);
		}

		for(int fd = 3 ; fd<=max_fd; fd++)
		{
			if(FD_ISSET(fd,&read_fdset))
			{
				if(fd==server_sd)
				{
					int client_sd = accept(server_sd,0,0);
					//send(client_sd, "220 Service ready for new user.\n", strlen("220 Service ready for new user.\n"), 0);
					printf("Client Connected fd = %d \n",client_sd);
					FD_SET(client_sd,&full_fdset);
					
					if(client_sd>max_fd)	
						max_fd = client_sd;

					//commands(client_sd);
				}
				else
				{
					commands(fd);
					// char buffer[256];
					// bzero(buffer,sizeof(buffer));
					// int bytes = recv(fd,buffer,sizeof(buffer),0);
					// if(bytes==0)   //client has closed the connection
					// {
					// 	printf("connection closed from client side \n");
					// 	close(fd);
					// 	FD_CLR(fd,&full_fdset);
					// 	if(fd==max_fd)
					// 	{
					// 		for(int i=max_fd; i>=3; i--)
					// 			if(FD_ISSET(i,&full_fdset))
					// 			{
					// 				max_fd =  i;
					// 				break;
					// 			}
					// 	}
					// }
					

				}
			}

			else{
	
			//commands(fd);
		}


		}

	}

	//close
	close(server_sd);
	return 0;
}
