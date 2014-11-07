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
#include <vector>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>	
#include <netinet/udp.h>	
#include <arpa/inet.h> 

using namespace std;

/************************** Defines ******************************/
#define BUFFER_SIZE						512
#define UDP_HEADER_LEN					8
#define TCP_HEADER_LEN					20
#define VAR_WS_SUPPORT					false

/************************* Typedefs ******************************/
typedef struct log_struct_t
{
	string logfilename;
	string time_stamp;
	string source;
	string destin;
	int seq_num;
	int ack_num;
	string flags;
}log_data;

/******************* Global Variables ****************************/
char* raw_data;

/******************* Function Prototype **************************/
void error(string str);
void quitHandler(int exit_code);
string get_time_stamp(void);
void read_file(string filename);
void write_log(log_data* my_log);

/******************* Main program ********************************/
int main(int argc, char* argv[])
{
	string filename;
	string remote_ip;
	int remote_port;
	int ack_port_num;
	string logfilename;
	int window_size = 1;
	
	struct sockaddr_in receiver;
	int len = sizeof(struct sockaddr_in);
	int receiver_socket;
	
	// setup to capture process terminate signals
	signal(SIGINT, quitHandler);
	signal(SIGTERM, quitHandler);
	signal(SIGKILL, quitHandler);
	signal(SIGQUIT, quitHandler);
	signal(SIGTSTP, quitHandler);
    
    // check if we have enough argument passed into the program
    if (argc < 5) {
    	error("Not enough argument.\nUsage: ./receiver <filename> <remote_ip> <remote_port> <ack_port_num> <log_filename> <window_size>(window_size is optional)");
    }
	
	filename = argv[1];
	remote_ip = argv[2];
	remote_port = atoi(argv[3]);
	ack_port_num = atoi(argv[4]);
	logfilename = argv[5];
	
	if(argc == 6 && VAR_WS_SUPPORT)
	{
		window_size = atoi(argv[6]);
	}
	
	// port the file into byte buffer
	read_file(filename);
 
	// setup receiver
    memset(&receiver, 0, sizeof(receiver));
	receiver.sin_addr.s_addr = inet_addr(remote_ip.c_str());
    receiver.sin_family = PF_INET;
    receiver.sin_port = htons(remote_port);
	
    if((receiver_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		error("Failed to open socket.");
	}
	
	int n = 0;
	n = sendto(receiver_socket, raw_data, BUFFER_SIZE, 0, (struct sockaddr *)&receiver, len);
	cout << n << " bytes sent." << endl;
	
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	quitHandler - capture process quit/stop/termination signals
/* 				  and exit gracefully.
/**************************************************************/
void quitHandler(int signal_code)
{
	cout << endl << ">User terminated sender process." << endl;	
	exit(EXIT_SUCCESS);
}

/**************************************************************/
/*	error - Error msg helper function
/**************************************************************/
void error(string str)
{
	cout << ">ERROR: " << str << endl;
	exit(EXIT_FAILURE);
}

/**************************************************************/
/*	get_time_stamp - get a time stamp in a nice formmat
/**************************************************************/
string get_time_stamp(void)
{
	char buff[20];
	time_t current_time;
	time(&current_time);
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&current_time));
	return buff;
}

/**************************************************************/
/*	read_file - read transmitting file data to a buffer
/**************************************************************/
void read_file(string filename)
{
	ifstream fd;
	fd.open(filename.c_str());
	streampos size;
	
	if(fd.is_open())
	{
		fd.seekg(0, ios::end);
		size = fd.tellg();
		raw_data = new char[size];
		
		fd.seekg(0, ios::beg);
		fd.read(raw_data, size);
		fd.close();
	}
	else
	{
		error("File not found.");
	}
}

/**************************************************************/
/*	write_log - write log
/**************************************************************/
void write_log(log_data* my_log)
{
	ofstream log;
	log.open(my_log->logfilename.c_str());
	
	if(log.is_open())
	{
		log << my_log->time_stamp << ", " << my_log->source << ", " << my_log->destin << ", ";
		log << my_log->seq_num << ", " << my_log->ack_num << ", " << my_log->flags << endl;
		
		log.flush();
		log.close();
	}
	else
	{
		error("File not found.");
	}
}
