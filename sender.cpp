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
#include <math.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>		
#include <arpa/inet.h> 
#include <pthread.h> 

using namespace std;

/************************** Defines ******************************/
#define BUFFER_SIZE						512

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
void quitHandler(int exit_code);
void read_file(string filename);
void write_log(log_data* my_log);

/******************* Main program ********************************/
int main(int argc, char* argv[])
{
	string filename;
	string remote_ip;
	int remote_port;
	string logfilename;
	int window_size = 1;
	
	struct sockaddr_in receiver;
	int receiver_socket;
	
	// setup to capture process terminate signals
	signal(SIGINT, quitHandler);
	signal(SIGTERM, quitHandler);
	signal(SIGKILL, quitHandler);
	signal(SIGQUIT, quitHandler);
	signal(SIGTSTP, quitHandler);
    
    // check if we have enough argument passed into the program
    if (argc < 4) {
    	error("Not enough argument.\nUsage: ./receiver <filename> <remote_ip> <remote_port> <log_filename> <window_size>(window_size is optional)");
    	exit(EXIT_FAILURE);
    }
	
	filename = argv[1];
	remote_ip = argv[2];
	remote_port = atoi(argv[3]);
	logfilename = argv[4];
	
	if(argc == 5)
	{
		window_size = atoi(argv[5]);
	}
 
	// setup receiver
    memset(&receiver, 0, sizeof(receiver));
	receiver.sin_addr.s_addr = inet_addr(remote_ip.c_str());
    receiver.sin_family = PF_INET;
    receiver.sin_port = htons(remote_port);
	
	// Try to open a socket and check if it open successfully
    receiver_socket = socket(PF_INET, SOCK_STREAM, 0);
    if(receiver_socket < 0)
	{
		error("Failed to open socket.");
		exit(EXIT_FAILURE);
	}
	
	// Connect to remote server
    if(connect(receiver_socket, (struct sockaddr *)&receiver, sizeof(receiver)) < 0)
    {
        error("Failed to connect to the receiver.");
        exit(EXIT_FAILURE);
    }
    
	shutdown(receiver_socket, SHUT_RDWR);
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	quitHandler - capture process quit/stop/termination signals
/* 				  and exit gracefully.
/**************************************************************/
void quitHandler(int signal_code)
{
	shutdown(client_socket, SHUT_RDWR);
	cout << endl << ">User terminated sender process." << endl;	
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	error - Error msg helper function
/**************************************************************/
void error(string str)
{
	cout << ">ERROR: " << str << endl;
}

void read_file(string filename)
{
	ifstream fd;
	fd.open(filename);
	streampos size;
	byte[] buffer;
	
	if(fd.is_open())
	{
		size = fd.tellg();
		buffer = new byte[size];
		
		fd.seekg(0, ios::beg);
		fd.read(buffer, size);
		fd.close();
		
		int i = 0;
		
		while(i < Math.ceil(size / BUFFER_SIZE))
		{
			memcpy(raw_data.at(i), &(buffer[i*BUFFER_SIZE]), BUFFER_SIZE); 
		}
		
		for(int i = 0; i < raw_data.size(); i++)
		{
			for(int j = 0; j < BUFFER_SIZE; j++)
			{
				cout << raw_data.at(i)[j];
			}
			cout << endl;
		}
		
		delete[] buffer;
	}
	else
	{
		error("File not found.");
	}
}
