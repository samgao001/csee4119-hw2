/*****************************************************************
**	File name: 		sender.c
**	Author:			Xusheng Gao (xg2193)
**  Description:	
**
*****************************************************************/

/*********************** Includes *******************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>    		
#include <stdbool.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <unistd.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>		
#include <arpa/inet.h> 
#include <pthread.h> 

using namespace std;

/******************* User Defines ********************************/
#define BUFFER_SIZE				512

/******************* Global Variables ****************************/
int client_socket;
int port_number;

/******************* Function Prototype **************************/
void error(string str);
void quitHandler(int exit_code);

/******************* Main program ********************************/
int main(int argc, char* argv[])
{
	struct sockaddr_in server_addr;
	string addr;
    string server_msg = "";
    char msg[BUFFER_SIZE];
    bool logout = false;
    
    // check if we have enough argument passed into the program
    if (argc < 3) {
    	error("Did not specify address and port number.\n>Usage: ./Client <IP Address> <Port #>");
    	exit(EXIT_FAILURE);
    }
    
    // setup to capture process terminate signals
	signal(SIGINT, quitHandler);
	signal(SIGTERM, quitHandler);
	signal(SIGKILL, quitHandler);
	signal(SIGQUIT, quitHandler);
	signal(SIGTSTP, quitHandler);
    
    // setup for the socket
    memset(&server_addr, 0, sizeof(server_addr));
    addr = argv[1];
	port_number = atoi(argv[2]);
	
	// Try to open a socket and check if it open successfully
    client_socket = socket(PF_INET, SOCK_STREAM, 0);
    if(client_socket < 0)
	{
		error("Failed to open socket.");
		exit(EXIT_FAILURE);
	}
	
	// setup ip address and port number for the communication
	server_addr.sin_addr.s_addr = inet_addr(addr.c_str());
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(port_number);
	
	// Connect to remote server
    if(connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        error("Failed to connect to the server.");
        exit(EXIT_FAILURE);
    }
    
    // Display message to indication connection established
    cout << ">Connected to " << inet_ntoa(server_addr.sin_addr) << endl;
    
    bool login = false;
    
    // Log-in processing 
    while(!logout)
    {
    	if(!login)
    	{
    		// clear buffer
			memset(msg, 0, BUFFER_SIZE);
		    if(recv(client_socket, msg, BUFFER_SIZE, 0) > 0)
		    {
		    	server_msg = string(msg);
		    	// Check if client received a welcome message from the server
		    	// Welcome message indicates successful log in
		    	if(server_msg.find("Welcome") != string::npos)
		    	{
		    		// Display the welcome message and set the login flag true
		    		cout << server_msg << endl;
		    		login = true;
		    		
		    		// Receive the basic user input prompt from the server
		    		// Could be nicer with GUI design
		    		memset(msg, 0, BUFFER_SIZE);
					if(recv(client_socket, msg, BUFFER_SIZE, 0) > 0)
					{	
						string rec_msg = string(msg);
						cout << rec_msg;
					}
		    		
		    		// Create a thread to handle all the incoming traffic
		    		// This is done so to avoid blocking from the outgoing message scan
		    		pthread_t rec_msg_tr;
					if(pthread_create(&(rec_msg_tr), NULL, rec_msg_handler, &client_socket) < 0)
					{
						error("Could not create thread.");
						logout = true;
					}
		    	}
		    	// log-in failed. 
		    	else if(server_msg.find("failed") != string::npos)
		    	{
		    		cout << server_msg << endl;
		    	}
		    	// Duplicate log-in
		    	else if(server_msg.find("already logged in") != string::npos)
		    	{
		    		cout << server_msg << endl;
		    	}
		    	// user is blocked
		    	else if(server_msg.find("blocked") != string::npos)
		    	{
		    		cout << server_msg << endl;
		    	}
		    	// user input for username and password
		    	else
		    	{
		    		string msg1;
		    		cout << server_msg;
					getline(cin, msg1);
					strcpy(msg, msg1.c_str());
					
					if(server_msg.find("Username") != string::npos)
					{
						username = msg1;
					}
					
					// send a "nop" instead of empty message to avoid
					// corruption. Server side has "nop" handle
					if(strlen(msg) == 0)
					{
						sprintf(msg, "nop");
					}
					
					// input "exit" to safely exit client. 
					// This means exit cannot be username.
					// Use of GUI will eliminate this issue.
					if(msg1.compare("exit") == 0)
					{
						cout << "User decided to exit." << endl;
						logout = true;
					}
					// Seding msg, exit if error
					else if(send(client_socket, msg, strlen(msg), 0) < 0)
					{
						error("Connection lost, please reconnect.");
						logout = true;
					}
		    	}
		    }
		    // exit if error in receiving msg
		    else
		    {
		    	error("Connection lost, please reconnect.");
		   		logout = true;
			}
    	}
    	else
    	{
    		// Scanning for user input and send to server.
			string msg1;
			getline(cin, msg1);
			memset(msg, 0, BUFFER_SIZE);
			strcpy(msg, msg1.c_str());

			if(strlen(msg) == 0)
			{
				sprintf(msg, "nop");
			}

			if(send(client_socket, msg, strlen(msg), 0) < 0)
			{
				error("Connection lost, please reconnect.");
				logout = true;
			}
			
			// logout command, assumed client termination
			if(msg1.compare("logout") == 0)
			{
				logout = true;
			}			
    	}
    }
	
	cout << ">Exited chat room." << endl;
	shutdown(client_socket, SHUT_RDWR);
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	quitHandler - capture process quit/stop/termination signals
/* 				  and exit gracefully.
/**************************************************************/
void quitHandler(int signal_code)
{
	shutdown(client_socket, SHUT_RDWR);
	cout << endl << ">User terminated client process." << endl;	
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	rec_msg_handler - Handler for incoming messages.
/*					  reads and display all the incoming message
/**************************************************************/
void* rec_msg_handler(void* socket_id)
{
	int rec_socket = *(int*)socket_id;
	char msg[BUFFER_SIZE];
	int connected = true;
	
	while(connected)
	{			
		memset(msg, 0, BUFFER_SIZE);
		if(recv(client_socket, msg, BUFFER_SIZE, 0) > 0)
		{	
			cout << string(msg);
			cout.flush();
		}
		else
		{
			connected = false;
		}			
	}
	
	cout << ">Exited chat room." << endl;
	shutdown(rec_socket, SHUT_RDWR);
	exit(EXIT_FAILURE);
}

/**************************************************************/
/*	error - Error msg helper function
/**************************************************************/
void error(string str)
{
	cout << ">ERROR: " << str << endl;
}
