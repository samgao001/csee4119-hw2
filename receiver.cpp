/*****************************************************************
**	File name: 		receiver.c
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
#include <map>
#include <time.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>		
#include <arpa/inet.h> 
#include <pthread.h> 

using namespace std;

/************************* Typedefs ******************************/
typedef struct log_struct_t
{
	ofstream log_file;
	time_t time_stamp;
	string source;
	string destin;
	int seq_num;
	int ack_num;
	string flags;
}log_data;

/******************* Global Variables ****************************/
vector<byte[]> raw_data = new vector<byte[]>;

/******************* Function Prototype **************************/
void error(string str);
void quitHandler(int signal_code);
void write_file(string filename);
void write_log(log_data* my_log);

/******************* Main program ********************************/
int main(int argc, char* argv[])
{
	string filename;
	int listening_port;
	string sender_ip;
	int sender_port;
	string logfilename;
	
	int listening_socket;
	int outgoing_socket;
	struct sockaddr_in sender;
	
	// setup to capture process terminate signals
	signal(SIGINT, quitHandler);
	signal(SIGTERM, quitHandler);
	signal(SIGKILL, quitHandler);
	signal(SIGQUIT, quitHandler);
	signal(SIGTSTP, quitHandler);
	
	if(argc < 5)
	{
		error("Not enough argument.\nUsage: ./receiver <filename> <listening_port> <sender_ip> <sender_port> <log_filename>");
		exit(EXIT_FAILURE);
	}
	
	filename = argv[1];
	listening_port = atoi(argv[2]);
	sender_ip = argv[3];
	sender_port = atoi(argv[4]);
	 = atoi(argv[5]);
	
	//setup sender
	memset(&sender, 0, sizeof(sender));
	sender.sin_family = PF_INET;
	sender.sin_addr.s_addr = inet_addr(sender_ip.c_str());
	sender.sin_port = htons(sender_port);
	
	// try to open a socket
	listening_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(listening_socket < 0)
	{
		error("Failed to open socket.");
		exit(EXIT_FAILURE);
	}
	
	// attempt to bind the socket
	if (bind(listening_socket, (struct sockaddr *)&sender, sizeof(sender)) < 0) 
	{
		error("Failed to bind.");
		shutdown(listening_socket, SHUT_RDWR);
		exit(EXIT_FAILURE);
	}
	
	// it is expecting total user size. This server do not accept duplicate logins
	listen(listening_socket, user_id.size());
	
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	quitHandler - capture process quit/stop/termination signals
/* 				  and exit gracefully.
/**************************************************************/
void quitHandler(int signal_code)
{
	printf("\n>User terminated receiver process.\n");
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	error - Error msg helper function
/**************************************************************/
void error(string str)
{
	cout << ">ERROR: " << str << endl;
}

/**************************************************************/
/*	write_log - write log
/**************************************************************/
void write_log(log_data* my_log)
{
	
}