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
    char address[50]; // the IP address 
    int is_authenticated; // for authentication for LIST STOR RETR
    int client_fd; 
};

struct User userlist[MAX_USERS] = {
    {} // example user from decription / testing
};

int num_users = 1;

int user(char *username, int client_fd) {

    for (int i = 0; i < num_users; i++) {
        if (strcmp(userlist[i].user, username) == 0) {
            userlist[i].is_authenticated = 0; // set authentication 0
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



// cwd
int cwd(char *path, int client_fd){

	if (chdir(path) == 0){ // change dir
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

	if (getcwd(path, sizeof(path)) == 0){ // get path of curr dir
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

//list: list out current files etc in dir
int list(int client_fd){

			int data_fd;
			//handle_port(data_fd);

		    char data_command[256];

		    printf("hello");
		    send(client_fd, "PORT OK", strlen("PORT OK"), 0);
		    recv(client_fd, data_command, sizeof(data_command), 0);

		 
		    	//handle_port(data_command);
		        DIR *dir = opendir(".");
		        if (dir == NULL) 
		        {
		            char message[256];
		            snprintf(message, sizeof(message), "error opening directory\n");
		            send(client_fd, message, strlen(message), 0);
		            exit(1);
		        }

		        char message[256];
		        struct dirent *filename;

		        while ((filename = readdir(dir)) != NULL) {
		            if (strcmp(filename->d_name, ".") == 0 || strcmp(filename->d_name, "..") == 0) {
		                continue;  // skip . .. directory --> causes error
		            }
		            char printt[256];
		            snprintf(printt, sizeof(printt), "%s\n", filename->d_name);// send names
		            snprintf(message, sizeof(message), "200 PORT command successful.\n150 File status okay; about to open. data connection.\n%s", printt);
		            send(client_fd, message, strlen(message), 0);
		        }
		        
		        closedir(dir);
		        exit(0);

		}




int stor(int client_fd, char *filename)
{
    
    int data_fd;
        //handle_port("");
        int file_transfer = open(filename, O_CREAT | O_WRONLY, 0644); // open file with write acces
        if (file_transfer < 0) 
        {
            char message[256];
            snprintf(message, sizeof(message), "Error with file\n");
            send(client_fd, message, strlen(message), 0);
            exit(1);
        }

        char buffer[256];

        int byter;
        int bytew;

        byter = recv(data_fd, buffer, sizeof(buffer), 0);

        while (byter > 0) 
        {
            bytew = write(file_transfer, buffer, byter); // write data in file
            if (bytew < 0) 
            {
                char message[256];
                snprintf(message, sizeof(message), "Error occurred\n");
                send(client_fd, message, strlen(message), 0);
                break;
            }
            byter = recv(data_fd, buffer, sizeof(buffer), 0);
        }

        close(file_transfer);
        close(data_fd);
        exit(0);
    

    char message[256];
    snprintf(message, sizeof(message), "200 PORT command successful.\n150 File status okay; about to open. data connection.\n");
    send(client_fd, message, strlen(message), 0);
    return 0;

		}




int retr(int client_fd, char *filename)
{
         int data_fd;
        //handle_port("");
        int file_transfer = open(filename, O_RDONLY); // open in read only mode
        if (file_transfer < 0) 
        {
            send(client_fd, "550 No such file or directory", strlen("550 No such file or directory"), 0);
            exit(1);
        }

        char buffer[256];

        int byter;
        int bytew;

        byter = read(file_transfer, buffer, sizeof(buffer)); // read

        while (byter > 0) 
        {
            bytew = send(data_fd, buffer, byter, 0);
            if (bytew < 0) 
            {
                char message[256];
                snprintf(message, sizeof(message), "Error occurred\n");
                send(client_fd, message, strlen(message), 0);
                break;
            }
            byter = read(file_transfer, buffer, sizeof(buffer));
        }

        close(file_transfer);
        close(data_fd);
        exit(0);
    

    char message[256];
    snprintf(message, sizeof(message), "200 PORT command successful.\n150 File status okay; about to open. data connection.\n");
    send(client_fd, message, strlen(message), 0);
    return 0;
}


// for data socket + forking
int handle_port(int client_fd, char* d, char code) {
    
//printf("hiiiii"); test
    int pid = fork(); //  fork
    if (pid == 0) 
    {
		    char buffer[256];
		    recv(client_fd, buffer, sizeof(buffer),0);
		    // Create data socket
		    int data_fd = socket(AF_INET, SOCK_STREAM, 0);

		    if (data_fd < 0) 
		    {
		        printf("Error creating data socket\n");
		        return -1;
		    }

		    int value  = 1;
			setsockopt(data_fd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
			

						
			struct sockaddr_in server_data_addr;
			bzero(&server_data_addr, sizeof(server_data_addr));
			socklen_t server_len = sizeof(server_data_addr);
			server_data_addr.sin_family = AF_INET;
			server_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
			server_data_addr.sin_port = htons(6000);
			
			// check bind
			if (bind(data_fd, (struct sockaddr*) &server_data_addr, sizeof(server_data_addr)) < 0) 
			{
		        perror("bind failed");
				exit(-1);

			}

		  // Parse the PORT command
		// Extract c and h1 separately
		char* h2 = strtok(NULL, ",");
		char* h3 = strtok(NULL, ",");
		char* h4 = strtok(NULL, ",");
		char* p1 = strtok(NULL, ",");
		char* p2 = strtok(NULL, "\r\n");

		char c[5]; //  char array to hold the first 4 chars of d
		strncpy(c, d, 4); //  copy first 4 chars of d in c
		c[4] = '\0'; // add a null character

		char* h1 = strtok(d+4, ","); //  tokenizing from 5th char 

		h2 = strtok(NULL, ",");
		h3 = strtok(NULL, ",");
		h4 = strtok(NULL, ",");
		p1 = strtok(NULL, ",");
		p2 = strtok(NULL, "\r\n");


		printf(" ");

		    struct sockaddr_in client_data_addr;
			bzero(&client_data_addr, sizeof(client_data_addr));
			client_data_addr.sin_family = AF_INET;
			client_data_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
			int port = atoi(p1) * 256 + atoi(p2);
		    client_data_addr.sin_port = htons(port);
			//client_data_addr.sin_port = htons(6000);
			int client_len = sizeof(client_data_addr);
		    //declare client stuff before connect
			//printf("hi its me %d\n",port);
		    //int port = atoi(p1) * 256 + atoi(p2);
		    
		    //data_addr.sin_port = htons(port);

		    // if (bind(data_fd, (struct sockaddr*) &client_data_addr,&client_len) < 0) 
			// {
			// 	printf("brpoooo\n");
		    //     perror("bind failed");
			// 	exit(-1);

			// }

		//parse the details of the client and use that to connect
			// convert p1 and p2 to port
		      //port = (p1 * 256) + p2
			//unsigned int len = sizeof(data_addr);
		    
		    // connect
		    if(connect(data_fd,(struct sockaddr*)&client_data_addr,sizeof(client_data_addr))<0)
			{
				perror("connect");
				exit(-1);
			}

			// send
			if (send(data_fd, "sending from server", strlen("sending from server"),0)<0){
				perror("send");
				exit(-1);
			}

			bzero(buffer,sizeof(buffer));
			recv(client_fd, buffer, sizeof(buffer),0);

			// authentication
			for (int i = 0; i < num_users; i++) {
       			 if (userlist[i].is_authenticated==1) {

					if (code == "L"){       // code L for list 
						list(client_fd);
					}

					else if (code=="S"){  // S for stor
						stor(client_fd, d);
					}

					else if (code=="R"){   // R for retr
						retr(client_fd, d);
					}

					close(data_fd);  // close socket
					exit(0);

				}
				else {
					send(client_fd, "503 Login with USER first.\n", strlen("503 Login with USER first.\n"), 0);
		    		return 0;
				}
			}
		}
		else{
			wait(1);
		}
		        
}

int client_port;
// commands from the client, tokenised reference: https://www.tutorialspoint.com/string-tokenisation-function-in-c

int commands(int client_sd)
{

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
	     
	    printf("%s\n", data_command);
	    // split between command and data using tokens
	    char *command = strtok(data_command, " ");
	    char *data = strtok(NULL, "\n");
	    
	    
	    printf("%s\n", data);
	    printf("%s\n", command);

	    if (command == NULL) { // if command is not given
	        send(client_sd, "500 Syntax error, command unrecognized.\n", strlen("500 Syntax error, command unrecognized.\n"), 0);
	        return 1;
	    }

	    //-----USER(username)-----
	    if (strcmp(command, "USER") == 0) {

	        if (data == NULL) { // check if username is given or not
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

		//-----cwd---------
	    } else if (strcmp(command, "CWD") == 0) {
	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        cwd(data, client_sd);
	    //-----pwd---------
	    } else if (strcmp(command, "PWD") == 0) {
	        if (data != NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        pwd(data,client_sd);
	    } 
        //-----list---------
	    else if (strcmp(command, "LIST") == 0) 
	    {	
	    	//recv(client_sd, clientr, sizeof(clientr), 0);
	    	//send(client_sd, "ack\n", strlen("ack\n"), 0);	
			//list(client_sd);
			printf("hi");
			handle_port(client_sd, data, "L");


	    } 

	    // else if (strcmp(command, "PORT") == 0) {
	    // 	printf("hello");

	    // 	for (int i = 0; i < num_users; i++) {
	    // 		if (strcmp(userlist[i].is_authenticated, 1) == 0) {
			    
		// 	    	send(client_sd, "200 PORT command recieved.\n", strlen("200 PORT command recieved.\n"), 0);
		// 	    	recv(client_sd, data_command, sizeof(data_command), 0);
		// 	    	printf("%s\n",data_command);
		// 	        handle_port(data_command);
		// 	    }
		// 	        else{
		// 	        	send(client_sd, "530 Not logged in.", strlen("530 Not logged in."), 0);}
		// 	   }

	    // } 

        //-----stor---------
	    else if (strcmp(command, "STOR") == 0) {
	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        handle_port(client_sd, data, "S");
	        //stor(client_sd, data);

		//-----retr---------
	    } else if (strcmp(command, "RETR") == 0) {
	        if (data == NULL) {
	            send(client_sd, "501 Syntax error in parameters or arguments.\n", strlen("501 Syntax error in parameters or arguments.\n"), 0);
	            return 1;
	        }
	        handle_port(client_sd, data, "R");
	        //retr(client_sd, data);


        //-----quit---------
	    } else if (strcmp(command, "QUIT\r\n") == 0) {
	            send(client_sd, "221 Service closing control connection.\n", strlen("221 Service closing control connection.\n"), 0);
	            //close(client_sd);
	            // Make sure that once the client quits the data for it gets reset
	            return 1;
	    
	    
	    } else { //wrong commands
	        send(client_sd, "500 Syntax error, command unrecognized.\n", strlen("500 Syntax error, command unrecognized.\n"), 0);
	        return 1;
	    }
	   return 0;
}





int main()
{
	
	FILE *fp = fopen("users.txt", "r"); // for users: username password
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
            userlist[i++] = user; // read from file into userlist
            
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
