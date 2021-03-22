// Old Includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <errno.h>
#include <sstream>

// New Includes
#include<stdio.h>	//For standard things
#include<stdlib.h>	//malloc
#include<string.h>	//memset
#include<netinet/ip_icmp.h>	//Provides declarations for icmp header
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/tcp.h>	//Provides declarations for tcp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include<sys/socket.h>
#include<arpa/inet.h>

// Display stuff
#define RESETTEXT "\x1B[0m" // Set all colors back to normal.
#define FORERED "\x1B[31m" // Red
#define FOREGRN "\x1B[32m" // Green
#define FOREYEL "\x1B[33m" // Yellow
#define FORECYN "\x1B[36m" // Cyan
#define FOREWHT "\x1B[37m" // White

// PHONEIX SERVER IPs (ignore)
// Phoneix0: 10.35.195.251
// Phoneix1: 10.34.40.33
// Phoneix2: 10.35.195.250
// Phoneix3: 10.34.195.236

using namespace std;

// Old Function declarations
int client(bool debug); // Client
int server(bool debug); // Server
int md5(char * fileName); // MD5
int fsize(FILE * fp); // File Size

// New function declarations
void ProcessPacket(unsigned char* , int);
void print_ip_header(unsigned char* , int);
void print_tcp_packet(unsigned char* , int);
void print_udp_packet(unsigned char * , int);
void print_icmp_packet(unsigned char* , int);
void PrintData (unsigned char* , int);

// New Variables declrations
int sock_raw;
FILE *logfile;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j;
struct sockaddr_in source,dest;


// Main function, parses arguments to determine server/client
int main(int argc, char * argv[]) { 
    int responce = 0, mode = 0;
    bool debug = false, isServer = false, isClient = false;
    system("clear");
    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-s") {
            isServer = true;
        } else if (arg == "-c") {
            isClient = true;
        } else if (arg == "-d") {
            debug = true;
        }
    }
	
	if (isServer) {
		cout << FOREGRN << "[----------SERVER----------]";
		if (debug) {
			cout << "[DEBUG MODE]";
		}
		cout << "\n";
		responce = server(debug);
	} else if (isClient) {
		cout << FOREGRN << "[----------CLIENT----------]";
		if (debug) {
			cout << "[DEBUG MODE]";
		}
		cout << "\n";
		responce = client(debug);
	} else {
		cout << FORERED << "Invalid parameter(s) entered, please specify if you are running a server ('-s') or a client ('-c') and use ('-d') to run in debug mode\n" << RESETTEXT;
	}
    return 0;
}

// Client function, send file to server
int client(bool debug) { 
	// Declare necessary variables/structures
    int sfd = 0, n = 0, b, port, packetSize, packetNum = 0, count = 0, totalSent = 0, pMode = 0, timeout = 0, sWindowSize = 0, sRangeLow = 0, sRangeHigh = 0, sErrors = 0;
    char ip[32], fileName[64], dropPackets[1024], looseAcks[1024];
    struct sockaddr_in serv_addr;
	
	// Initialize socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
	
	// Get server IP from user
    cout << FOREWHT << "IP Address: ";
    scanf("%15s", ip);
	if (strlen(ip) == 0){
		cout << FORERED << "Invalid IP Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get server Port from user
    cout << "Port: ";
    cin >> port;
	if (port < 1){
		cout << FORERED << "Invalid Port Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get file name from user
    cout << "File Name: ";
    scanf("%64s", fileName);
	if (strlen(fileName) == 0){
		cout << FORERED << "Invalid File Name Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	// Open file for sending
    FILE * fp = fopen(fileName, "rb"); 
    if (fp == NULL) {
        cout << FORERED << "ERROR OPENING FILE...\n" << RESETTEXT;
        perror("File");
        return 2;
    }
    int fileSize = fsize(fp);
	
	// Get packet size from user
    cout << "Packet Size (kb): ";
    cin >> packetSize;
	if (packetSize < 1){
		cout << FORERED << "Invalid Packet Size Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
    packetSize = packetSize * 1000;
    char sendbuffer[packetSize];
	
	// Get protocol from user
    cout << "Protocol (1=GBN 2=SR): ";
    cin >> pMode;
	if (pMode < 1 || pMode > 2){
		cout << FORERED << "Invalid Protocol Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get timeout from user
    cout << "Timeout (ms): ";
    cin >> timeout;
	if (timeout < 1){
		cout << FORERED << "Invalid Timeout Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get size of sliding window from user
    cout << "Size Of Sliding Window: ";
    cin >> sWindowSize;
	if (sWindowSize < 1){
		cout << FORERED << "Invalid Sliding Window Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get sequence range from user
    cout << "Sequence Range Low: ";
    cin >> sRangeLow;
    cout << "Sequence Range High: ";
    cin >> sRangeHigh;
	if (sRangeHigh < sRangeLow || sRangeLow < 0){
		cout << FORERED << "Invalid Range Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get situational errors from user
    cout << "Situational Errors (1=none, 2=random, 3=custom): ";
    cin >> sErrors;
	if (sErrors == 3){
			cout << "Please enter comma-separated packet numbers to drop, if none, enter nothing and press enter: ";
			scanf("%1024s", dropPackets);
			
			cout << "Please enter comma-separated acks to loose, if none, enter nothing and press enter: ";
			scanf("%1024s", looseAcks);
			bool validatedErrors = (strlen(dropPackets) != 0) || (strlen(looseAcks) != 0);
			if (!validatedErrors){
				cout << FORERED << "Invalid Custom Errors Input Entered... please try again\n" << RESETTEXT;
				return 0;
			} else {
				// Parse custom situational errors here
			}
	} else if (sErrors == 0 || sErrors > 3){
		cout << FORERED << "Invalid Situationel Errors Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Set family, port, and ip for socket
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
	
	// Connect to server
    cout << FOREYEL << "\nAttempting to connect to " << ip << ":" << port << "\n" << RESETTEXT;
    if ((connect(sfd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) == -1)) {
        cout << FORERED << "CONNECTION FAILED...\n" << RESETTEXT;
        perror("Connect");
        return 1;
    }
    cout << FOREGRN << "Connected to server!\n" << RESETTEXT;
	
	
	// Send Settings to server
	cout << FOREYEL << "Sending Settings to Server...\n" << RESETTEXT;
	// Send packet size to server
    uint32_t tmp = htonl(packetSize); 
    write(sfd, & tmp, sizeof(tmp));
	// Send file size to server
    uint32_t tmp2 = htonl(fileSize); 
    write(sfd, & tmp2, sizeof(tmp2));
	// Send protcol mode to server
    uint32_t tmp3 = htonl(pMode); 
    write(sfd, & tmp3, sizeof(tmp3));
	// Send timeout to server
    uint32_t tmp4 = htonl(timeout); 
    write(sfd, & tmp4, sizeof(tmp4));
	// Send Size Of Sliding Window to server
    uint32_t tmp5 = htonl(sWindowSize); 
    write(sfd, & tmp5, sizeof(tmp5));
	// Send Sequence Range Low to server
    uint32_t tmp6 = htonl(sRangeLow); 
    write(sfd, & tmp6, sizeof(tmp6));
	// Send Sequence Range High to server
    uint32_t tmp7 = htonl(sRangeHigh); 
    write(sfd, & tmp7, sizeof(tmp7));
	cout << FOREGRN << "Settings Sent!\n" << RESETTEXT;
	
	// Set client variables
    int mode = 1;
    bool run = true;
	// Loop until total bytes sent is equal to file size bytes
	cout << "\n[File Transfer]\n" << RESETTEXT;
    while ((totalSent != fileSize) && run) {
        switch (mode) {
        case 1:
			// read/send file by desired packet size
            if (((b = fread(sendbuffer, 1, packetSize, fp)) > 0)) {
                totalSent += b;
				// Print output, if debug is enabled all packets will be printed, otherwise only print first 10 packets and then progress bar for remaining packets
                if (packetNum < 10 || debug == true) { 
                    cout << "Sent Packet #" << (packetNum + 1);
                    if (debug) {
                        cout << "(" << (b * 2) << " bytes (" << totalSent << "/" << fileSize << "))\n";
                    } else {
                        cout << "\n";
                    }
                    if (packetNum == 9 && debug == false) {
                        cout << "\nSending Remaining Packets...\n" << FORECYN;
                    }
                }
				 // Send Packet
                send(sfd, sendbuffer, b, 0);
				// Increase packet counter
                packetNum++;
				// Switch to validation/protocol mode
                mode = 2;
            }
            break;
        case 2:
			if (pMode == 1){
				// Place GBN client functionality here
			} else if (pMode == 2){
				// Place SR client functionality here
			}
			mode = 1; // after validation, set mode back to 1 for normal client functions
			break;
        case 0:
            cout << "Exiting...\n" << RESETTEXT;
            break;
        }
    }
    fclose(fp); // Close file
    if (run) {
        cout << FOREGRN << "\nSend Success!\n" << RESETTEXT; // Print 'Send Success!'
        md5(fileName); // Print md5
    } else {
        cout << FORERED << "\nSend Failed!\n" << RESETTEXT;
    }
    close(sfd); // Close socket
    return 0;
}

// Server function, connects to a client, recieves packets of file contents, and writes result to file
int server(bool debug) { 
	logfile=fopen("log.txt","w");
	// Declare necessary variables
    int fd = 0, confd = 0, b, num, port, packetSize, packetNum = 0, count = 0, totalRecieved = 0, pMode = 0, timeout = 0, sWindowSize = 0, sRangeLow = 0, sRangeHigh = 0, sErrors = 0;
    struct sockaddr_in serv_addr;
    char fileName[64], initialBuff[32], ip[32];
	
	// Get IP from user
    cout << FOREWHT << "IP Address: "; 
    scanf("%15s", ip);
	if (strlen(ip) == 0){
		cout << FORERED << "Invalid IP Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get port from user
    cout << "Port: ";
    cin >> port;
	if (port < 1){
		cout << FORERED << "Invalid Port Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Get output file from user
    cout << "Output File Name: ";
    scanf("%64s", fileName);
	if (strlen(fileName) == 0){
		cout << FORERED << "Invalid Output File Name Input Entered... please try again\n" << RESETTEXT;
		return 0;
	}
	
	// Initialize socket
    fd = socket(AF_INET, SOCK_STREAM, 0); 
	
	// Allocate memory for server address and initial buffer (used to get packet size)
    memset( & serv_addr, '0', sizeof(serv_addr));
    memset(initialBuff, '0', sizeof(initialBuff));
	
	 // Set family, port, and ip address for socket	
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
	
	 // Bind socket struct to socket and begin listening for clients
    bind(fd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr));
    listen(fd, 10);
    cout << FOREYEL << "\nSocket started on " << ip << ":" << port << "\n";
    cout << "Waiting for client connection...\n" << RESETTEXT;
    while (1) {
		// Accept connection and error handle
        confd = accept(fd, (struct sockaddr * ) NULL, NULL); 
        if (confd == -1) {
            perror("Accept");
            continue;
        }
        cout << FOREGRN << "Connected to client!\n";
		
		// Get Packet Size from client
        uint32_t tmp; 
        read(confd, & tmp, sizeof(tmp));
        packetSize = ntohl(tmp);
		// Declare new recieving buff[] to be length of packet size and allocate memory for new buffer
        char recieveBuffer[packetSize]; 
        memset(recieveBuffer, '0', sizeof(recieveBuffer));
		// Get File Size from client
        uint32_t tmp2; 
        read(confd, & tmp2, sizeof(tmp2));
        int fileSize = ntohl(tmp2);
		// Get Protocol from client
        uint32_t tmp3; 
        read(confd, & tmp3, sizeof(tmp3));
        int pMode = ntohl(tmp3);
		// Get timeout from client
        uint32_t tmp4; 
        read(confd, & tmp4, sizeof(tmp4));
        int timeout = ntohl(tmp4);
		// Get Size Of Sliding Window from client
        uint32_t tmp5; 
        read(confd, & tmp5, sizeof(tmp5));
        int sWindowSize = ntohl(tmp5);
		// Get Sequence Range Low from client
        uint32_t tmp6; 
        read(confd, & tmp6, sizeof(tmp6));
        int sRangeLow = ntohl(tmp6);
		// Get Sequence Range High from client
        uint32_t tmp7; 
        read(confd, & tmp7, sizeof(tmp7));
        int sRangeHigh = ntohl(tmp7);
		
		
		

		// Print recieved information from client
		cout << FOREYEL << "\n[Settings Recieved From Client]\n";
		if (pMode == 1){
			cout << "Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: GBN\n";
		} else if (pMode == 2){
			cout << "Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: SR\n";
		}
		cout << "Timeout (ms): " << timeout << " | Sliding Window Size: " << sWindowSize << " | Sequence Range: [" << sRangeLow << "," << sRangeHigh << "]\n" << FOREWHT;
		cout << "\n[File Transfer]\n";
		
		// Open new file for writing
        FILE * fp = fopen(fileName, "wb"); 
        if (fp != NULL) {
            int mode = 1;
            bool run = true;
			// Loop until total recieved bytes equals desired file size (recieved from client)
            while ((totalRecieved != fileSize) && run) {
                switch (mode) {
                case 1:
                    if (((b = recv(confd, recieveBuffer, packetSize, MSG_WAITALL)) > 0)) {
                        totalRecieved += b;
						// Print output, if debug is enabled all packets will be printed, otherwise only print first 10 packets and then progress bar for remaining packets
						if (packetNum < 10 || debug == true) { 
							
                            cout << "Recieved Packet #" << (packetNum + 1);
                            if (debug) {
                                cout << "(" << b << " bytes (" << totalRecieved << "/" << fileSize << "))\n";
                            } else {
                                cout << "\n";
                            }
							
							unsigned char* processBuffer = reinterpret_cast<unsigned char *>( recieveBuffer );
							// Process Packet
							ProcessPacket(processBuffer , b);
							
                            if (packetNum == 9 && debug == false) {
                                cout << "\nRecieving Remaining Packets...\n" << FORECYN;
                            }
                        } 
						// Write packet to file
                        fwrite(recieveBuffer, 1, b, fp); 
						// Increase packet count
                        packetNum++; 
						// Switch to validation/protocol mode
                        mode = 2;
                    }
                    break;
                case 2:
					if (pMode == 1){
						// Place GBN server functionality here
					} else if (pMode == 2){
						// Place SR server functionality here
					}
					mode = 1; // after validation, set mode back to 1 for normal server functions
					break;
                }

            }

            fclose(fp); // Close file
            if (run) {
                cout << FOREGRN << "\nRecieve Success!\n" << RESETTEXT; // Print 'Recieve Success!'
                md5(fileName); // Print md5
            } else {
                cout << FORERED << "\nRecieve Failed!\n" << RESETTEXT;
            }
            return 0;
        } else {
            cout << FORERED << "ERROR OPENING FILE FOR WRITING...\n" << RESETTEXT;
            perror("File");
        }
        close(confd); // Close socket
    }
    return 0;
}

// MD5 function, uses system call to output md5sum of file
int md5(char * fileName) { 
    string str(fileName);
    cout << FORECYN << "\nMD5 result for '" << str << "'\n";
    string command = "md5sum " + str;
    system(command.c_str());
    cout << "\n" << RESETTEXT;
    return 0;
}

// fsize function, returns input value's file size 
int fsize(FILE * fp) {
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return sz;
}

// Process packet buffer to extract IP and TCP headers
void ProcessPacket(unsigned char* buffer, int size) {
	//Get the IP Header part of this packet
	struct iphdr *iph = (struct iphdr*)buffer;
	++total;
	// Parse TCP packet header
	try {
		print_tcp_packet(buffer , size);
	} catch (int error){
		cout << "\nError Getting TCP Headder: " << error;
	}
	// printf("TCP : %d   UDP : %d   ICMP : %d   IGMP : %d   Others : %d   Total : %d\r",tcp,udp,icmp,igmp,others,total);
}

// Print IP Header information. Writes header to file as well as outputs necessary info to console
void print_ip_header(unsigned char* Buffer, int Size) {
	unsigned short iphdrlen;
		
	struct iphdr *iph = (struct iphdr *)Buffer;
	iphdrlen =iph->ihl*4;
	
	memset(&source, 0, sizeof(source));
	source.sin_addr.s_addr = iph->saddr;
	
	memset(&dest, 0, sizeof(dest));
	dest.sin_addr.s_addr = iph->daddr;
	
	fprintf(logfile,"\n");
	fprintf(logfile,"IP Header\n");
	fprintf(logfile,"   |-IP Version        : %d\n",(unsigned int)iph->version);
	fprintf(logfile,"   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
	fprintf(logfile,"   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
	fprintf(logfile,"   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
	fprintf(logfile,"   |-Identification    : %d\n",ntohs(iph->id));
	//fprintf(logfile,"   |-Reserved ZERO Field   : %d\n",(unsigned int)iphdr->ip_reserved_zero);
	//fprintf(logfile,"   |-Dont Fragment Field   : %d\n",(unsigned int)iphdr->ip_dont_fragment);
	//fprintf(logfile,"   |-More Fragment Field   : %d\n",(unsigned int)iphdr->ip_more_fragment);
	fprintf(logfile,"   |-TTL      : %d\n",(unsigned int)iph->ttl);
	fprintf(logfile,"   |-Protocol : %d\n",(unsigned int)iph->protocol);
	fprintf(logfile,"   |-Checksum : %d\n",ntohs(iph->check));
	cout << " |-IP Checksum: " << ntohs(iph->check) << "\n";
	fprintf(logfile,"   |-Source IP        : %s\n",inet_ntoa(source.sin_addr));
	fprintf(logfile,"   |-Destination IP   : %s\n",inet_ntoa(dest.sin_addr));
}

// Print TCP Header information. Writes header to file as well as outputs necessary info to console
void print_tcp_packet(unsigned char* Buffer, int Size) {
	unsigned short iphdrlen;
	
	struct iphdr *iph = (struct iphdr *)Buffer;
	iphdrlen = iph->ihl*4;
	
	struct tcphdr *tcph=(struct tcphdr*)(Buffer + iphdrlen);
			
	fprintf(logfile,"\n\n***********************TCP Packet*************************\n");	
		
	print_ip_header(Buffer,Size);
		
	fprintf(logfile,"\n");
	fprintf(logfile,"TCP Header\n");
	fprintf(logfile,"   |-Source Port      : %u\n",ntohs(tcph->source));
	fprintf(logfile,"   |-Destination Port : %u\n",ntohs(tcph->dest));
	fprintf(logfile,"   |-Sequence Number    : %u\n",ntohl(tcph->seq));
	fprintf(logfile,"   |-Acknowledge Number : %u\n",ntohl(tcph->ack_seq));
	cout << " |-Ack #: " << ntohl(tcph->ack_seq) << "\n";
	fprintf(logfile,"   |-Header Length      : %d DWORDS or %d BYTES\n" ,(unsigned int)tcph->doff,(unsigned int)tcph->doff*4);
	//fprintf(logfile,"   |-CWR Flag : %d\n",(unsigned int)tcph->cwr);
	//fprintf(logfile,"   |-ECN Flag : %d\n",(unsigned int)tcph->ece);
	fprintf(logfile,"   |-Urgent Flag          : %d\n",(unsigned int)tcph->urg);
	fprintf(logfile,"   |-Acknowledgement Flag : %d\n",(unsigned int)tcph->ack);
	fprintf(logfile,"   |-Push Flag            : %d\n",(unsigned int)tcph->psh);
	fprintf(logfile,"   |-Reset Flag           : %d\n",(unsigned int)tcph->rst);
	fprintf(logfile,"   |-Synchronise Flag     : %d\n",(unsigned int)tcph->syn);
	fprintf(logfile,"   |-Finish Flag          : %d\n",(unsigned int)tcph->fin);
	fprintf(logfile,"   |-Window         : %d\n",ntohs(tcph->window));
	cout << " |-Window: " << ntohs(tcph->window) << "\n";
	fprintf(logfile,"   |-Checksum       : %d\n",ntohs(tcph->check));
	cout << " |-TCP Checksum: " << ntohs(tcph->check) << "\n\n";
	fprintf(logfile,"   |-Urgent Pointer : %d\n",tcph->urg_ptr);
	fprintf(logfile,"\n");
	fprintf(logfile,"                        DATA Dump                         ");
	fprintf(logfile,"\n");
		
	fprintf(logfile,"IP Header\n");
	PrintData(Buffer,iphdrlen);
		
	fprintf(logfile,"TCP Header\n");
	PrintData(Buffer+iphdrlen,tcph->doff*4);
		
	fprintf(logfile,"Data Payload\n");	
	PrintData(Buffer + iphdrlen + tcph->doff*4 , (Size - tcph->doff*4-iph->ihl*4) );
						
	fprintf(logfile,"\n###########################################################");
}

// Prints raw data of the packet
void PrintData (unsigned char* data , int Size) {
	
	for(i=0 ; i < Size ; i++)
	{
		if( i!=0 && i%16==0)   //if one line of hex printing is complete...
		{
			fprintf(logfile,"         ");
			for(j=i-16 ; j<i ; j++)
			{
				if(data[j]>=32 && data[j]<=128)
					fprintf(logfile,"%c",(unsigned char)data[j]); //if its a number or alphabet
				
				else fprintf(logfile,"."); //otherwise print a dot
			}
			fprintf(logfile,"\n");
		} 
		
		if(i%16==0) fprintf(logfile,"   ");
			fprintf(logfile," %02X",(unsigned int)data[i]);
				
		if( i==Size-1)  //print the last spaces
		{
			for(j=0;j<15-i%16;j++) fprintf(logfile,"   "); //extra spaces
			
			fprintf(logfile,"         ");
			
			for(j=i-i%16 ; j<=i ; j++)
			{
				if(data[j]>=32 && data[j]<=128) fprintf(logfile,"%c",(unsigned char)data[j]);
				else fprintf(logfile,".");
			}
			fprintf(logfile,"\n");
		}
	}
}