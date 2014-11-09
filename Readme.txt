/*******************************************************************
**	File name: 		Readme.txt
**	Author:			Xusheng Gao (xg2193)
**
*******************************************************************/

>>>>>>>>>>>>>>>>>>>>>>> Program Description <<<<<<<<<<<<<<<<<<<<<<<<<

Simple one to one file transfer program. It is simplified version of 
TCP-like transfer layer protocol. It handles packet loss, corrupted 
packets, out of order packets (which will not happen in current
implementation, currently it is doing STOP-and-WAIT). The sender will
take different window size settings, but will only default to 1.

NOTE: check KNOWN ISSUE section for known bug that I was not able to 
resolve.

>>>>>>>>>>>>>>>>>>>>> Development Environment <<<<<<<<<<<<<<<<<<<<<<<

Both sender and receiver is develop under 32bit ubuntu 12.04LTS in a virtual
machine. It is compile and build with g++ 4.6.3.

Testing is done with a 64bit ubuntu 14.04LTS in a VM.

>>>>>>>>>>>>>>>>>>>>>>>>>>>>> HOW TO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

Place makefile and all source files (receiver.cpp and sender.cpp) in the same 
directory. Open a terminal and cd to the source file directory. Then
invoke "make rebuild" to compile the source code. 

After doing so, move newudpl and receiver executable binary to a 
computer "receiver" and execute them as follow.

Start the emulator by running the following invoke command:
./newudpl -vv -o <own_IP>:<port#> -i <own_ip>:* -p <proxy_in>:<proxy_out> -B <bit error> -L <loss rate> -O <out of order>

example: 
./newudpl -vv -o 121.12.21.1:4119 -i 121.12.21.1:* -p 4000:6000 -B 50 -L 50 -O 50

Then start invoke the receiver as follow:
./receiver <filename> <listening_port> <sender_ip> <sender_port> <logfile name>

Example:
./receiver file.pdf 4119 121.12.1.21 4118 receiverlog.txt

Now move the sender executable binary to a different computer "sender"
and follow the institution to invoke the sender.

Invoke the sender with following:
./sender <filename> <remote_ip> <remote_port> <ack_port> <logfile name> <window size> 

Example:
./sender test.pdf 74.12.21.11 4000 4118 senderlog.txt 1 

>>>>>>>>>>>>>>>>>>>>>>>>>> KNOWN ISSUE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
*When trying to second second file over the same configuration. 
TCP link may not establish correctly, try again after few seconds. 

*newudpl only supports 64bit machine. I was not able to execute it
under 32 bit machine