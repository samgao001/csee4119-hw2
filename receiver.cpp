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
#include <iomanip>

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
vector<char> raw_data;

/******************* Function Prototype **************************/
void error(string str);
void quitHandler(int signal_code);
uint16_t get_checksum(tcp_packet* packet, int buff_len);
string get_time_stamp(void);
bool write_file(string filename);
bool write_log(log_data* my_log);

/******************* Main program ********************************/
int main(int argc, char* argv[])
{
	string filename;
	int listening_port;
	string sender_ip;
	int sender_port;
	string logfilename;
	
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
		shutdown(receiver_socket, SHUT_RDWR);
		error("Failed to open UDP socket.");
	}
	
	int iSetOption = 1;
	setsockopt(receiver_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
	
	// attempt to bind the socket
	if (bind(receiver_socket, (struct sockaddr *)&receiver, sizeof(receiver)) < 0) 
	{
		shutdown(receiver_socket, SHUT_RDWR);
		error("Failed to bind UDP socket.");
	}
	
	// try to open a TCP socket
	if((sender_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		shutdown(receiver_socket, SHUT_RDWR);
		shutdown(sender_socket, SHUT_RDWR);
		error("Failed to open TCP socket.");
	}
	
	ack_packet->source_port = listening_port;
	ack_packet->destin_port = sender_port;
	ack_packet->seq_num = 0;
	ack_packet->ack_num = 0;
	ack_packet->dataoffset_NSflag = 0;
	ack_packet->flags = 0;
	ack_packet->window_size = 1;
	ack_packet->checksum = 0;
	ack_packet->URG = 0;
	
	int n = 0;
	bool TCP_link = false;
	uint16_t cs = 0;

	log_data mylog;
	mylog.logfilename = logfilename;
	
	do
	{
		n = recvfrom(receiver_socket, packet, sizeof(tcp_packet), 0, (struct sockaddr*)&receiver, (socklen_t*)&len);
		
		cs = get_checksum(packet, n - TCP_HEADER_LEN);
		
		mylog.time_stamp = get_time_stamp();
		mylog.source = sender_port;
		mylog.destin = listening_port;
		mylog.seq_num = packet->seq_num;
		mylog.ack_num = packet->ack_num;
		mylog.flags = packet->flags;
		
		if(!write_log(&mylog))
		{
			error("Failed to write log.");
		}
		
		if(!TCP_link)
		{
			// Connect to remote ip
			if(connect(sender_socket, (struct sockaddr *)&sender, sizeof(sender)) < 0)
			{
				shutdown(receiver_socket, SHUT_RDWR);
				shutdown(sender_socket, SHUT_RDWR);
				error("Failed establish TCP link.");
			}
			TCP_link = true;
		}
		
		if(cs == packet->checksum)
		{
			// if the previous packet is out of sequence, update the raw_data buffer.
			if(packet->seq_num * BUFFER_SIZE >= raw_data.size())
			{
				for(int i = 0; i < n - TCP_HEADER_LEN; i++)
				{
					raw_data.push_back(packet->buffer[i]);
				}
			}
			else
			{
				for(int i = 0; i < n - TCP_HEADER_LEN; i++)
				{
					raw_data.at(packet->seq_num * BUFFER_SIZE + i) = packet->buffer[i];
				}
			}
		
			ack_packet->seq_num = packet->seq_num;
			ack_packet->ack_num = packet->seq_num;
			ack_packet->flags |= ACK_bm;
			ack_packet->checksum = get_checksum(ack_packet, 0);
			n = send(sender_socket, ack_packet, TCP_HEADER_LEN, 0);
		
			mylog.time_stamp = get_time_stamp();
			mylog.source = listening_port;
			mylog.destin = sender_port;
			mylog.seq_num = ack_packet->seq_num;
			mylog.ack_num = ack_packet->ack_num;
			mylog.flags = ack_packet->flags;
		
			if(!write_log(&mylog))
			{
				error("Failed to write log.");
			}
		}
		
	}while((packet->flags & FIN_bm) == 0x00);
	
	ack_packet->flags = FIN_bm;
	n = send(sender_socket, ack_packet, TCP_HEADER_LEN, 0);
	recv(sender_socket, ack_packet, TCP_HEADER_LEN, 0);
	
	if(!write_file(filename))
	{
		cout << "Failed to create file." << endl;
	}
	
	cout << "Delivery completed successfully." << endl;

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
/*	get_checksum - get the checksum of the packet
/**************************************************************/
uint16_t get_checksum(tcp_packet* packet, int buff_len)
{
	uint16_t checksum = 0x00;
	
	checksum = packet->source_port + packet->destin_port;
	checksum = checksum + (packet->seq_num >> 16) & 0xFFFFFFFF;
	checksum = checksum + packet->seq_num & 0xFFFFFFFF;
	checksum = checksum + (packet->ack_num >> 16) & 0xFFFFFFFF;
	checksum = checksum + packet->ack_num & 0xFFFFFFFF;
	checksum = checksum + packet->dataoffset_NSflag << 8 | packet->flags;
	checksum = checksum + packet-> window_size;
	checksum = checksum + packet->URG;
	
	for(int i = 0; i < buff_len; i+=2)
	{
		checksum = checksum + ((packet->buffer[i] << 8) | packet->buffer[i+1]);
	}
	
	if(buff_len % 2 == 1)
	{
		checksum = checksum + packet->buffer[buff_len - 1];
	}
	
	return checksum;
}

/**************************************************************/
/*	get_time_stamp - get a time stamp in a nice formmat
/**************************************************************/
string get_time_stamp(void)
{
	char buff[20];
	time_t current_time;
	time(&current_time);
	strftime(buff, 20, "%D %T", localtime(&current_time));
	return buff;
}

/**************************************************************/
/*	write_file - write received data to a file
/**************************************************************/
bool write_file(string filename)
{
	ofstream fd;
	fd.open(filename.c_str(), fstream::out);
	
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
		cout << ">ERROR: Unable to create file." << endl;
		return false;
	}
	return true;
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
