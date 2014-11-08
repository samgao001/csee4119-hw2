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
#define TCP_HEADER_LEN					20
#define PACKET_SIZE						BUFFER_SIZE + TCP_HEADER_LEN
#define VAR_WS_SUPPORT					false

#define NS_bm							(1 << 0)
#define CWR_bm							(1 << 7)
#define ECE_bm							(1 << 6)
#define URG_bm							(1 << 5)							
#define ACK_bm							(1 << 4)
#define PSH_bm							(1 << 3)
#define RST_bm							(1 << 2)
#define SYN_bm							(1 << 1)
#define FIN_bm							(1 << 0)

/************************* Typedefs ******************************/
typedef struct log_struct_t
{
	string logfilename;
	string time_stamp;
	int source;
	int destin;
	int seq_num;
	int ack_num;
	uint8_t flags;
}log_data;

typedef struct tcp_packet_t
{
	// Start of TCP header
	uint16_t source_port;
	uint16_t destin_port;
	uint32_t seq_num;
	uint32_t ack_num;
	uint8_t  dataoffset_NSflag; // first 4 bits are data offset and last bit is NS flag
	uint8_t  flags;
	uint16_t window_size;
	uint16_t checksum;
	uint16_t URG;
	//end of TCP header
	char buffer[BUFFER_SIZE];
}tcp_packet;

/******************* Global Variables ****************************/
char* raw_data;
int file_size;

/******************* Function Prototype **************************/
void error(string str);
void quitHandler(int exit_code);
string get_time_stamp(void);
void read_file(string filename);
bool write_log(log_data* my_log);

/******************* Main program ********************************/
int main(int argc, char* argv[])
{
	string filename;
	string remote_ip;
	int remote_port;
	int ack_port_num;
	string logfilename;
	int window_size = 1;
	
	tcp_packet* packet = (tcp_packet*)malloc(PACKET_SIZE);
	tcp_packet* ack_packet = (tcp_packet*)malloc(PACKET_SIZE);
	
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
    
    // check if we have enough argument passed into the program
    if (argc < 5) 
    {
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
    
    //setup sender address
	memset(&sender, 0, sizeof(sender));
	sender.sin_family = PF_INET;
	sender.sin_addr.s_addr = INADDR_ANY;
	sender.sin_port = htons(ack_port_num);
	
    if((receiver_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		shutdown(receiver_socket, SHUT_RDWR);
		error("Failed to open UDP socket.");
	}
	
	// try to open a TCP socket
	if((sender_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		shutdown(receiver_socket, SHUT_RDWR);
		shutdown(sender_socket, SHUT_RDWR);
		error("Failed to open TCP socket.");
	}
	
	// attempt to bind the TCP socket
	if (bind(sender_socket, (struct sockaddr *)&sender, sizeof(sender)) < 0) 
	{
		shutdown(receiver_socket, SHUT_RDWR);
		shutdown(sender_socket, SHUT_RDWR);
		error("Failed to bind TCP socket.");
	}
	
	packet->source_port = ack_port_num;
	packet->destin_port = remote_port;
	packet->seq_num = 0;
	packet->ack_num = 0;
	packet->dataoffset_NSflag = 0;
	packet->flags = 0;
	packet->window_size = window_size;
	packet->checksum = 0;
	packet->URG = 0;
	
	int n;
	bool TCP_link = false;
	while((packet->seq_num * BUFFER_SIZE) <= file_size && file_size != 0)
	{
		int b_size = BUFFER_SIZE;
		
		if(file_size < (packet->seq_num + 1) * BUFFER_SIZE)
		{
			b_size = file_size - packet->seq_num * BUFFER_SIZE;
		}
		
		memcpy(packet->buffer, (raw_data + packet->seq_num * BUFFER_SIZE), b_size);
		b_size = b_size + TCP_HEADER_LEN;

		n = sendto(receiver_socket, packet, b_size, 0, (struct sockaddr *)&receiver, len);
		
		if(!TCP_link)
		{
			// only one to one TCP connection
			listen(sender_socket, 1);
			sender_socket = accept(sender_socket, (struct sockaddr *)&sender, (socklen_t*)&len);
			
			if(sender_socket < 0)
			{
				shutdown(receiver_socket, SHUT_RDWR);
				shutdown(sender_socket, SHUT_RDWR);
				error("Failed to establish TCP link");
			}
			TCP_link = true;
		}
		
		n = recv(sender_socket, ack_packet, TCP_HEADER_LEN, 0);
	
		// if correct ack packet is received, send next packet, else resend current packet
		if((ack_packet->flags & ACK_bm) && packet->seq_num == ack_packet->ack_num)
		{
			packet->seq_num++;
			packet->ack_num = ack_packet->ack_num;
		}
		
	}
	
	packet->flags |= FIN_bm;
	n = sendto(receiver_socket, packet, TCP_HEADER_LEN, 0, (struct sockaddr *)&receiver, len);

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
		file_size = size;
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
bool log_opened_previously = false;
bool write_log(log_data* my_log)
{
	if(!log_opened_previously)
	{
		fstream fd;
		fd.open(my_log->logfilename.c_str(), fstream::in | fstream::out | fstream::app);
		if(fd.is_open())
		{
			fd.seekg(0, ios::end);
			streampos size = fd.tellg();
			if(size == 0)
			{
				fd << "Time Stamp       , Source, Destin, Seq Num, ACK Num, Flags" << endl;
			}
			fd.close();
		}
		log_opened_previously = true;
	}
	
	ofstream log;
	log.open(my_log->logfilename.c_str(), fstream::app);
	
	if(log.is_open())
	{
		log << my_log->time_stamp << ", " << my_log->source << "  , " << my_log->destin << "  ,   ";
		log << my_log->seq_num << "   ,   " << my_log->ack_num;
		log << "   ,  0x" << hex << (int)my_log->flags << endl;
		log.flush();
		log.close();
	}
	else
	{
		cout << ">ERROR: Unable to create file." << endl;
		return false;
	}
	return true;
}
