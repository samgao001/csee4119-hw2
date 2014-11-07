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
#include <vector>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>		
#include <netinet/udp.h>	
#include <arpa/inet.h> 

using namespace std;

/************************** Defines ******************************/
#define BUFFER_SIZE						512
#define TCP_HEADER_LEN					20

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

typedef struct tcp_header_t
{
	uint16_t source_port;
	uint16_t destin_port;
	uint32_t seq_num;
	uint32_t ack_num;
	uint8_t  dataoffset_NSflag; // first 4 bits are data offset and last bit is NS flag
	uint8_t  flags;
	uint16_t window_size;
	uint16_t checksum;
	uint16_t URG;
}tcp_header;

typedef struct tcp_packet_t
{
	tcp_header_t header;
	char* buffer;
}tcp_packet;

/******************* Global Variables ****************************/
vector<char> raw_data;

/******************* Function Prototype **************************/
void error(string str);
void quitHandler(int signal_code);
string get_time_stamp(void);
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
	
	tcp_packet* packet = (tcp_packet*)malloc(TCP_HEADER_LEN + BUFFER_SIZE);
	
	int sender_socket;
	int receiver_socket;
	struct sockaddr_in sender;
	struct sockaddr_in receiver;
	int len = sizeof(struct sockaddr_in);
	
	// setup to capture process terminate signals
	signal(SIGINT, quitHandler);
	signal(SIGTERM, quitHandler);
	signal(SIGKILL, quitHandler);
	signal(SIGQUIT, quitHandler);
	signal(SIGTSTP, quitHandler);
	
	if(argc < 5)
	{
		error("Not enough argument.\nUsage: ./receiver <filename> <listening_port> <sender_ip> <sender_port> <log_filename>");
	}
	
	filename = argv[1];
	listening_port = atoi(argv[2]);
	sender_ip = argv[3];
	sender_port = atoi(argv[4]);
	logfilename = argv[5];
	
	//setup receiver address
	memset(&receiver, 0, sizeof(receiver));
	receiver.sin_family = PF_INET;
	receiver.sin_addr.s_addr = INADDR_ANY;
	receiver.sin_port = htons(listening_port);
	
	//setup sender address
	memset(&sender, 0, sizeof(sender));
	sender.sin_family = PF_INET;
	sender.sin_addr.s_addr = inet_addr(sender_ip.c_str());
	sender.sin_port = htons(sender_port);

	// try to open a UDP socket
	if((receiver_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		error("Failed to open socket.");
	}
	
	// attempt to bind the socket
	if (bind(receiver_socket, (struct sockaddr *)&receiver, sizeof(receiver)) < 0) 
	{
		shutdown(receiver_socket, SHUT_RDWR);
		error("Failed to bind.");
	}
	
	int n = 0;
	n = recvfrom(receiver_socket, &packet, sizeof(&packet), 0, (struct sockaddr*)&receiver, (socklen_t*)&len);
	cout << n << "bytes received.";
	
	shutdown(receiver_socket, SHUT_RDWR);
	shutdown(sender_socket, SHUT_RDWR);
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
/*	write_file - write received data to a file
/**************************************************************/
void write_file(string filename)
{
	ofstream fd;
	fd.open(filename.c_str());
	
	if(fd.is_open())
	{
		for(int i = 0; i < raw_data.size(); i++)
		{
			fd << raw_data.at(i);
		}
		
		fd.flush();
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
