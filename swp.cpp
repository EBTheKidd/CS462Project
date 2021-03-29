// Old Includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <netdb.h>
#include <errno.h>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <vector>

// Display stuff
#define RESETTEXT "\x1B[0m" // Set all colors back to normal.
#define FORERED "\x1B[31m" // Red
#define FOREGRN "\x1B[32m" // Green
#define FOREYEL "\x1B[33m" // Yellow
#define FORECYN "\x1B[36m" // Cyan
#define FOREWHT "\x1B[37m" // White

// PHONEIX SERVER IPs:
// Phoneix0: 10.35.195.251
// Phoneix1: 10.34.40.33
// Phoneix2: 10.35.195.250
// Phoneix3: 10.34.195.236

// VALGRIND COMMANDS:
// valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --log-file=leak.txt ./swp -c -d
// valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --log-file=leak.txt ./swp -s -d

using namespace std;

// Server/Client Functions
int client(bool debug); // Client
int server(bool debug); // Server

// Server/Client Variables
bool debug = false, isServer = false, isClient = false;
int packetSize = 512;

// File info functions
int md5(char * fileName); // MD5
int fsize(FILE * fp); // File Size

// Packet Struct
typedef struct PACKET {
    int src_port;
    int dst_port;
    int seq;
	int ttl;
    int checksum;
	bool finalPacket;
    unsigned char *buffer;
	
	void print() {
		try {
			cout << "  |===PACKET===========\n";
			cout << "  |-source:       " << src_port << "\n";
			cout << "  |-dest:         " << dst_port << "\n";
			cout << "  |-seq:          " << seq << "\n";
			cout << "  |-ttl:  " << ttl << "\n";
			cout << "  |-checksum:     " << checksum << "\n";
			cout << "  |-buffer:     '" << buffer << "'\n";
			cout << "  |-final:        " << finalPacket << "\n";
			cout << "  |====================\n";
		} catch (int i){
			
		}
	}	
}PACKET;

// Packet functions
void serialize(PACKET* msgPacket, char *data);
void deserialize(char *data, PACKET* msgPacket);
int compute_crc16(unsigned char *buf);
void copy_packet( PACKET* packet1, PACKET* packet2 );

// Main function, parses arguments to determine server/client
int main(int argc, char * argv[]) {
    int responce = 0, mode = 0;
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
    int sfd = 0, n = 0, b, port, packetNum = 0, count = 0, pMode = 0, timeout = 0, sWindowSize = 0, sRangeLow = 0, sRangeHigh = 0, sErrors = 0;
    char ip[32], fileName[64], dropPackets[1024], looseAcks[1024];
    struct sockaddr_in serv_addr;

    // Initialize socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);

      // Get server IP from user
    do {
		cout << FOREWHT << "IP Address: ";
		scanf("%15s", ip);
		if (strlen(ip) == 0) {
			cout << FORERED << "Invalid IP Input Entered... please try again\n" << RESETTEXT;
		}
    } while (strlen(ip) == 0);
	
    // Get server Port from user
    do {
		cout << "Port: ";
		cin >> port;
		if (port < 1) {
			cout << FORERED << "Invalid Port Input Entered... please try again\n" << RESETTEXT;
		}
    } while (port < 1);
	
    // Get file name from user
    do {
		cout << "File Name: ";
		scanf("%64s", fileName);
		if (strlen(fileName) == 0) {
			cout << FORERED << "Invalid File Name Input Entered... please try again\n" << RESETTEXT;
        }
    } while (strlen(fileName) == 0);
	
    // Open file for sending
    FILE * fp = fopen(fileName, "rb");
    if (fp == NULL) {
        cout << FORERED << "ERROR OPENING FILE...\n" << RESETTEXT;
        perror("File");
        return 2;
    }	
    int fileSize = fsize(fp);
	
    // Get packet size from user
    do {
		cout << "Packet Size (bytes): ";
		cin >> packetSize;
		if (packetSize < 1) {
			cout << FORERED << "Invalid Packet Size Input Entered... please try again\n" << RESETTEXT;
        }
    } while (packetSize < 1);
	
    //packetSize = packetSize * 1000;
    char sendbuffer[packetSize];
	
    // Get protocol from user
    do {
		cout << "Protocol (1=Stop And Wait, 2=Go-Back-N, 3=Selective Repeat): ";
		cin >> pMode;
		if (pMode < 1 || pMode > 3) {
			cout << FORERED << "Invalid Protocol Input Entered... please try again\n" << RESETTEXT;
        }
    } while (pMode < 1 || pMode > 3);
	
    // Get timeout from user
    do {
		cout << "Timeout (ms): ";
		cin >> timeout;
		if (timeout < 1) {
			cout << FORERED << "Invalid Timeout Input Entered... please try again\n" << RESETTEXT;
        }
    } while (timeout < 1);
	
    // Get size of sliding window from user
	do {	
		cout << "Size Of Sliding Window: ";
		cin >> sWindowSize;
		if (sWindowSize < 1) {
			cout << FORERED << "Invalid Sliding Window Input Entered... please try again\n" << RESETTEXT;
        }
    } while (sWindowSize < 1);
	
    // Get sequence range from user
	do {
		cout << "Sequence Range Low: ";
		cin >> sRangeLow;
		cout << "Sequence Range High: ";
		cin >> sRangeHigh;
		if (sRangeHigh < sRangeLow || sRangeLow < 0) {
			cout << FORERED << "Invalid Range Input Entered... please try again\n" << RESETTEXT;
        }
		if (((sRangeHigh - sRangeLow + 1) / 2 ) < sWindowSize ) {
			cout << FORERED << "Sequence Range must be equal to or greater than twice the Window Size\n" << RESETTEXT; 
		}
    } while (sRangeHigh < sRangeLow || sRangeLow < 0 || ((sRangeHigh - sRangeLow + 1) / 2 ) < sWindowSize);
	
    // Get situational errors from user
	do {
		cout << "Situational Errors (1=none, 2=random, 3=custom): ";
		cin >> sErrors;
		if (sErrors == 3) {
			cout << "Please enter comma-separated packet numbers to drop, if none, enter nothing and press enter: ";
			scanf("%1024s", dropPackets);
			cout << "Please enter comma-separated acks to loose, if none, enter nothing and press enter: ";
			scanf("%1024s", looseAcks);
			bool validatedErrors = (strlen(dropPackets) != 0) || (strlen(looseAcks) != 0);
			if (!validatedErrors) {
				cout << FORERED << "Invalid Custom Errors Input Entered... please try again\n" << RESETTEXT;
				return 0;
			} else {	
			// Parse custom situational errors here
			}
		}
		if (sErrors <= 0 || sErrors > 3) {
			cout << FORERED << "Invalid Situationel Errors Input Entered... please try again\n" << RESETTEXT;
        }
    } while (sErrors <= 0 || sErrors > 3);

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
    bool run = true, transferFinished = false;
	auto transferStart = chrono::high_resolution_clock::now(); 
	int originalPacketsSent = 0,retransmittedPacketsSent = 0,totalEllapsedTime = 0,totalThroughput = 0,effectiveThroughput = 0;
	int fileReadSize = packetSize - sizeof(PACKET);
	int framesToSend = (int) (fileSize/fileReadSize);
	
    cout << "\n[File Transfer]\n" << RESETTEXT;
	// Stop and wait
	if (pMode == 1){
		// Stop And Wait
		while (run) {
			
			// memset sendbuffer to make sure its empty (might not be needed)
			memset(sendbuffer, 0, fileReadSize);
			// read/send file by desired packet size
			if (((b = fread(sendbuffer, 1, fileReadSize, fp)) > 0)) {
				// Setup current frame's ack variables
				int expectedAck;
				
				// Initialize char array for packet serialization
				char data[sizeof(PACKET) + b];
					
				// Build Packet
				PACKET* newMsg = new PACKET;
				newMsg->src_port = 0;
				newMsg->dst_port = port;
				newMsg->seq = packetNum;
				expectedAck = newMsg->seq; // set expected ackResponce from server
				newMsg->ttl = 5;
				newMsg->checksum = compute_crc16(reinterpret_cast<unsigned char*>(sendbuffer)); // compute crc16
				newMsg->buffer = reinterpret_cast<unsigned char*>(sendbuffer); // cast char[] to packet buffer
				if (b < fileReadSize) {
					newMsg->finalPacket = true;
					transferFinished = true;
				} else {
					newMsg->finalPacket = false;
				}
				
				// Serialize packet into char array
				serialize(newMsg, data);
				
				// Send Packet
				send(sfd, data, sizeof(data), 0);
				originalPacketsSent++;
				
				// Print 'Sent Packet x' output (x = current seq num)
				if (packetNum < 10 || debug == true) {
					// Print "Sent Packet x" <- x = current packet number
					cout << "Sent Packet #" << packetNum;
					
					// If in debug, print every packet size and packet contents
					if (debug) {
						// Print packet size (in bytes)
						cout << "(" << sizeof(data) << " bytes)\n";
						// Print packet data
						newMsg->print();
					} else {
						cout << "\n";
					}
					
					// If not in debug, print filler message after 10 packets
					if (packetNum == 9 && debug == false) {
						cout << "\nSending Remaining Packets...\n" << FORECYN;
					}
				}
					
				int s = 0, recievedAck;
				bool ackRecieved = false;
				auto start = chrono::high_resolution_clock::now(); 
				// Loop until either timout is reached or ack is recieved
				while (!ackRecieved) { 
					
					// calculate time ellapsed since timer was started
					auto now = chrono::high_resolution_clock::now();
					int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
				
					// Check to see if ack has been recieved
					if (s = recv(sfd, &recievedAck, sizeof(recievedAck), MSG_DONTWAIT ) > 0) {
						if (recievedAck == expectedAck) {
							// Correct Packet Ack recieved, transfer success
							cout << "  |-" << FOREGRN << "ACK RECIEVED" << RESETTEXT << "  (" << ms << " ms)\n";
							ackRecieved = true;
							cout << "  |====================\n";
						} else {
							// Incorrect Packet Ack recieved, transfer failed
							cout << "  |-" << FORERED << "NAK RECIEVED" << RESETTEXT << "  (" << ms << " ms) (Resending Packet After Timeout)\n";
						}
					}
					
					// Check to see if timeout has been reached
					if (timeout < ms && (transferFinished == false)){
						cout << "  |-" << FORERED << "ACK TIMEOUT  " << RESETTEXT << " (Resending Packet)\n";
						// Re-Send Packet, reset timeout timer, and increment retransmittedPacketsSent
						send(sfd, data, sizeof(data), 0);
						start = chrono::high_resolution_clock::now();
						retransmittedPacketsSent++;
					} else if (transferFinished){
						cout << "  |-" << FOREGRN << "FINAL PACKET\n" << RESETTEXT ;
						ackRecieved = true; // not really, but it infinitley hangs unless this is called
						run = false;
					}
				}
				
				// Increase packet counter
				packetNum++;
			}
			
		}
	} 
	
	// Go-Back-N
	if (pMode == 2){
		typedef struct frame {
			PACKET packet;
			bool ack = false;
		} frame;
		vector<frame> all_frames;
		int seq_num = 0;
		int head_seq_num = 0;
		int next_seq_num = 0;
		int ackFrames = 0;
		int timer = 0;
		int s = 0, recievedAck;
		PACKET sendingPacket;
		PACKET resendingPacket;
		// Create vector of all frames of file
		for (int i = 0; i < framesToSend; i++) {
			char readBuffer[fileReadSize];
			if (((b = fread(readBuffer, 1, fileReadSize, fp)) > 0)) {
				char readBufferTrim[b];
				for (int t = 0; t < b; t++) {
					readBufferTrim[t] = readBuffer[t];
				}
				// Build Packet
				PACKET newMsg;
				newMsg.src_port = 0;
				newMsg.dst_port = port;
				newMsg.seq = (seq_num % sRangeHigh);
				newMsg.buffer = reinterpret_cast<unsigned char*>(strdup(reinterpret_cast<const char*>(readBufferTrim)));
				newMsg.checksum = compute_crc16(newMsg.buffer); // compute crc16 from buffer
				frame f;
				f.packet = newMsg;
				all_frames.push_back(f);
			}
			seq_num++;
		}
		do { 
			if ( next_seq_num < (head_seq_num + sWindowSize) && (head_seq_num + sWindowSize) < all_frames.size() ) {
				char* data;						
				copy_packet( &all_frames[next_seq_num].packet, &sendingPacket );
				// Serialize packet into char array
				serialize(&sendingPacket, data);
				// Send Packet
				send(sfd, data, sizeof(data), 0);
				originalPacketsSent++;				
				sendingPacket.print();
				next_seq_num++;
			}	
			// Check to see if an ack has been recieved
			if (s = recv(sfd, &recievedAck, sizeof(recievedAck), MSG_DONTWAIT ) > 0) {
				cout << "Recieved Ack " << recievedAck << "\n";
				if (recievedAck == all_frames[head_seq_num].packet.seq) {
					all_frames[head_seq_num].ack = true;
					head_seq_num++;
					ackFrames++;
					cout << "Incrementing head index of window frame...\n";				
				}
			}
			if ( timer == timeout ) {
				cout << "  |-" << FORERED << "ACK TIMEOUT  " << RESETTEXT << " (Resending Packet)\n";
				timer = 0;
				for( int i = head_seq_num; i < next_seq_num; i++ ) {
					char* data;						
					copy_packet( &all_frames[i].packet, &resendingPacket );
					// Serialize packet into char array
					serialize(&resendingPacket, data);
					// Send Packet
					send(sfd, data, sizeof(data), 0);
					retransmittedPacketsSent++;
					all_frames[i].packet.print();
					resendingPacket.print();
				}					
			}
			timer++;
		} while( ackFrames < framesToSend );
		transferFinished = true;
	} 
	
	// Selective Repeat
	if (pMode == 3){ 
	
		// Create struct to track frames with their ack
		struct windowFrame {
			PACKET* packet;
			bool ack;
			int buffSize;
		};
		// initialize frames[] array which will store file contents in memory
		windowFrame frames[framesToSend];
		// initialize window index (this will represent the low index in frames[])
		int windowFrameIndex = 0;
		
		// Load frames into memory
		for (int i = 0; i < framesToSend; i++){
			char readBuffer[fileReadSize];
			if (((b = fread(readBuffer, 1, fileReadSize, fp)) > 0)) {
				char readBufferTrim[b];
				for (int t = 0; t < b; t++){
					readBufferTrim[t] = readBuffer[t];
				}
				// Build Packet
				PACKET* newMsg = new PACKET;
				newMsg->src_port = 0;
				newMsg->dst_port = port;
				newMsg->seq = packetNum;
				//expectedAck = newMsg->seq; // set expected ackResponce from server
				newMsg->buffer = reinterpret_cast<unsigned char*>(strdup(reinterpret_cast<const char*>(readBufferTrim)));
				newMsg->checksum = compute_crc16(newMsg->buffer); // compute crc16 from buffer
				if (b < fileReadSize) {
					newMsg->finalPacket = true;
					transferFinished = true;
				} else {
					newMsg->finalPacket = false;
				}
				
				frames[i].packet = newMsg;
				frames[i].ack = false;
				frames[i].buffSize = b;
			}
		}
		
		// Send frames in window size
		while (run){
			for (int f = windowFrameIndex; f < framesToSend; f++){
				if (f < (f + sWindowSize)){
					cout << "checking frame "<< f << "\n"; 
					// Only send frames that not been ack'd 
					if (frames[f].ack == false){
						char data[sizeof(PACKET) + frames[f].buffSize];
						
						PACKET* currentPacket = frames[f].packet;
						//currentPacket->buffer = reinterpret_cast<unsigned char*>(strdup(reinterpret_cast<const char*>(frames[f].packet->buffer)));
						
						// Serialize packet into char array
						serialize(currentPacket, data);
						
						// Send Packet
						send(sfd, data, sizeof(data), 0);
						originalPacketsSent++;
						currentPacket->print();
					} else {
						cout << "frame " << f << "recieved an ack... skipping\n";
					}
				}
			}
			
			int s = 0, recievedAck;
			auto start = chrono::high_resolution_clock::now();
			bool lowFrameAckd = false;
			while (false){
				// calculate time ellapsed since timer was started
				auto now = chrono::high_resolution_clock::now();
				int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
				
				// Check to see if an ack has been recieved
				if (s = recv(sfd, &recievedAck, sizeof(recievedAck), MSG_DONTWAIT ) > 0) {
					cout << "Recieved Ack " << recievedAck << "\n";
					if (recievedAck == frames[windowFrameIndex].packet->seq){
						frames[windowFrameIndex].ack = true;
						lowFrameAckd = true;
						windowFrameIndex++;
						cout << "Incrementing windowFrameIndex...\n";
					} else {
						frames[recievedAck].ack = true;
					}
				}
					
				// Check to see if timeout has been reached
				if (timeout < ms && (transferFinished == false)){
					cout << "  |-" << FORERED << "ACK TIMEOUT  " << RESETTEXT << " (Resending Packet)\n";
					// Re-Send Packet, reset timeout timer, and increment retransmittedPacketsSent
					lowFrameAckd = true;
					retransmittedPacketsSent++;
				} else if (transferFinished){
					cout << "  |-" << FOREGRN << "FINAL PACKET\n" << RESETTEXT ;
					lowFrameAckd = true; // not really, but it infinitley hangs unless this is called
					run = false;
				}
			
			}
			
			transferFinished = true;
			run = false;
		}
		
		
	}
    
    fclose(fp); // Close file
    if (transferFinished) {
		auto transferEnd = chrono::high_resolution_clock::now();
		totalEllapsedTime = (int)std::chrono::duration_cast<std::chrono::milliseconds>(transferEnd - transferStart).count();
        cout << FOREGRN << "\nSend Success!\n" << RESETTEXT; // Print 'Send Success!'
		cout << "Number of original packets sent: " << FORECYN << originalPacketsSent << RESETTEXT << "\n";
		cout << "Number of retransmitted packets sent: " << FORECYN << retransmittedPacketsSent << RESETTEXT << "\n";
		cout << "Total elapsed time (ms): " << FORECYN << totalEllapsedTime << RESETTEXT << "\n";
		//cout << "Total throughput (Mbps): " << FORECYN << totalThroughput << RESETTEXT << "\n";
		//cout << "Effective throughput: " << FORECYN << effectiveThroughput << RESETTEXT << "\n";
        md5(fileName); // Print md5
    } else {
        cout << FORERED << "\nSend Failed!\n" << RESETTEXT;
    }
    close(sfd); // Close socket
    return 0;
}

// Server function, recieve file from client
int server(bool debug) {
    // Declare necessary variables
    int fd = 0, confd = 0, b, num, port, packetNum = 0, count = 0, pMode = 0, timeout = 0, sWindowSize = 0, sRangeLow = 0, sRangeHigh = 0, sErrors = 0;
    struct sockaddr_in serv_addr;
    char fileName[64], ip[32];

    // Get IP from user
    cout << FOREWHT << "IP Address: ";
    scanf("%15s", ip);
    if (strlen(ip) == 0) {
        cout << FORERED << "Invalid IP Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get port from user
    cout << "Port: ";
    cin >> port;
    if (port < 1) {
        cout << FORERED << "Invalid Port Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get output file from user
    cout << "Output File Name: ";
    scanf("%64s", fileName);
    if (strlen(fileName) == 0) {
        cout << FORERED << "Invalid Output File Name Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Initialize socket
    fd = socket(AF_INET, SOCK_STREAM, 0);

    // Allocate memory for server address and initial buffer (used to get packet size)
    memset( & serv_addr, '0', sizeof(serv_addr));

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
        if (pMode == 1) {
            cout << "Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: Stop And Wait\n";
        } else if (pMode == 2) {
            cout << "Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: Go-Back-N\n";
        } else if (pMode == 3) {
            cout << "Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: Selective Repeat\n";
        }
        cout << "Timeout (ms): " << timeout << " | Sliding Window Size: " << sWindowSize << " | Sequence Range: [" << sRangeLow << "," << sRangeHigh << "]\n" << FOREWHT;
        cout << "\n[File Transfer]\n";

        // Open new file for writing
        FILE * fp = fopen(fileName, "wb");
        if (fp != NULL) {
            bool run = true;
			bool transferFinished = false;
			int ack;
			
			// set timeout for socket
			int timeoutSeconds = (int)(timeout / 1000); // convert timeout (ms) to seconds with no remainder
			int remainingTimeout = 0;
			if (timeoutSeconds > 0){
				remainingTimeout = timeout - (timeoutSeconds * 1000); // remainingTimeout = ms remaining after subtracting seconds from timeout
			}	
			struct timeval tv;
			tv.tv_sec = timeoutSeconds;
			tv.tv_usec = remainingTimeout * 1000;
			if (setsockopt(confd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
				perror("Error");
			}
			
			
			int originalPacketsRecieved = 0;
			int retransmittedPacketsRecieved = 0;
			
			// Stop and wait
			if (pMode == 1) {
				int previousPacketSeq = -1;
				// Stop And Wait
				while (run){
					char data[packetSize];
					char data2[packetSize];
                    if ((b = recv(confd, data, packetSize, MSG_WAITALL)) > 0) {
						// Calculate amount of bytes to write to file
						int writeBytes = b - sizeof(PACKET);
						
						// Deserialize packet
						PACKET* recievedPacket = new PACKET;
						deserialize(data, recievedPacket);
						
						// Compute CRC16 from duplicated 'temp' packet (using actual packet causes data corruption)
						PACKET* temp = new PACKET;
						memcpy( data2, data, sizeof(data) );
						deserialize(data2, temp);
						int crcNew = compute_crc16(temp->buffer);
						
						// Determine if current packet is final packet
                        if (recievedPacket->finalPacket == true) {
                            transferFinished = true;
                        }
						
						// Set ack responce
						ack = recievedPacket->seq;
						
                        // Print output
                        if (recievedPacket->seq < 10 || debug == true) {
							// Print "Sent Packet x" <- x = current packet number
                            cout << "Recieved Packet #" << recievedPacket->seq;
							
							// If in debug, print every packet size and packet contents
                            if (debug) {
								// Write packet bytes size
                                cout << "(" << b << " bytes)\n";
								// print packet data
								recievedPacket->print();
                            } else {
                                cout << "\n";
                            }
							
							// If not in debug, print filler message after 10 packets
                            if (recievedPacket->seq == 9 && debug == false) {
                                cout << "\nRecieving Remaining Packets...\n" << FORECYN;
                            }
                        }
						
						
						
						// Validate checksum 
						if (recievedPacket->checksum == crcNew){
							cout << "  |-Checksum " << FOREGRN << "OK\n" << RESETTEXT;
							cout << "  |-Sending " << "ACK " << FOREGRN  << ack << "\n" << RESETTEXT;
							cout << "  |====================\n";
							send(confd, & ack, sizeof(ack), 0);
							
							if (previousPacketSeq != ack){
								originalPacketsRecieved++;
								// Write packet buffer to file
								fwrite(recievedPacket->buffer, 1, writeBytes, fp);
								// update previousPacketSeq to current packet seq
								previousPacketSeq = ack;
								if (transferFinished){
									run = false;
								}
							} else {
								retransmittedPacketsRecieved++;
							}
						} else {
							cout << "  |-Checksum " << FORERED << "failed\n" << RESETTEXT;
							cout << "  |-Sending " << "ACK " << FORERED  << ack << "\n" << RESETTEXT;
							cout << "  =========================\n";
							send(confd, & ack, sizeof(ack), 0);
						}
                    } else {
						perror("Timeout");
					}
				}
			}
			// Go-Back-N
			if (pMode == 2) {
				int fileReadSize = packetSize - sizeof(PACKET);
				int framesToReceive = (int) (fileSize/fileReadSize);
				int framesReceived = 0;
				int next_seq_num = 0; //Next expected packet sequence number to be received
				char data[packetSize];
				char data2[packetSize];
				do {	
                    if ((b = recv(confd, data, packetSize, MSG_WAITALL)) > 0) {						
						// Deserialize packet
						PACKET* recievedPacket = new PACKET;
						deserialize(data, recievedPacket);
						// Validate checksum, write to file, send ack if next expected packet
						if ( recievedPacket->seq == (next_seq_num % sRangeHigh) ) { 
							// Compute CRC16 from duplicated 'temp' packet (using actual packet causes data corruption)
							PACKET* temp = new PACKET;
							memcpy( data2, data, sizeof(data) );
							deserialize(data2, temp);
							int crcNew = compute_crc16(temp->buffer);
							if (recievedPacket->checksum == crcNew) {
								ack = recievedPacket->seq;
								cout << "  |-Checksum " << FOREGRN << "OK\n" << RESETTEXT;
								cout << "  |-Sending " << "ACK " << FOREGRN  << ack << "\n" << RESETTEXT;
								cout << "  |====================\n";	
								// Calculate amount of bytes to write to file
								int writeBytes = b - sizeof(PACKET);
								fwrite(recievedPacket->buffer, 1, writeBytes, fp);
								send(confd, & ack, sizeof(ack), 0);
								originalPacketsRecieved++;
								framesReceived++;
								next_seq_num++;
							} else {
								cout << "  |-Checksum " << FORERED << "failed\n" << RESETTEXT;
								cout << "  |-Sending " << "ACK " << FORERED  << ack << "\n" << RESETTEXT;
								cout << "  =========================\n";
								//send(confd, & ack, sizeof(ack), 0);
							}
						}
					}
				} while ( framesReceived < framesToReceive );
				transferFinished = true;
			} 
			
			// Selective Repeat
			if (pMode == 3) {
				
				while (run){
					char data[packetSize];
					char data2[packetSize];
                    if ((b = recv(confd, data, packetSize, MSG_WAITALL)) > 0) {
						// Calculate amount of bytes to write to file
						int writeBytes = b - sizeof(PACKET);
						
						// Deserialize packet
						PACKET* recievedPacket = new PACKET;
						deserialize(data, recievedPacket);
						
						// Compute CRC16 from duplicated 'temp' packet (using actual packet causes data corruption)
						PACKET* temp = new PACKET;
						memcpy( data2, data, sizeof(data) );
						deserialize(data2, temp);
						int crcNew = compute_crc16(temp->buffer);
						
						// Determine if current packet is final packet
                        if (recievedPacket->finalPacket == true) {
                            transferFinished = true;
                        }
						
						// Set ack responce
						ack = recievedPacket->seq;
						
                        // Print output
                        if (recievedPacket->seq < 10 || debug == true) {
							// Print "Sent Packet x" <- x = current packet number
                            cout << "Recieved Packet #" << recievedPacket->seq;
							
							// If in debug, print every packet size and packet contents
                            if (debug) {
								// Write packet bytes size
                                cout << "(" << b << " bytes)\n";
								// print packet data
								recievedPacket->print();
                            } else {
                                cout << "\n";
                            }
							
							// If not in debug, print filler message after 10 packets
                            if (recievedPacket->seq == 9 && debug == false) {
                                cout << "\nRecieving Remaining Packets...\n" << FORECYN;
                            }
                        }
						
						
						
						// Validate checksum 
						if (recievedPacket->checksum == crcNew){
							cout << "  |-Checksum " << FOREGRN << "OK\n" << RESETTEXT;
							cout << "  |-Sending " << "ACK " << FOREGRN  << ack << "\n" << RESETTEXT;
							cout << "  |====================\n";
							fwrite(recievedPacket->buffer, 1, writeBytes, fp);
							//send(confd, & ack, sizeof(ack), 0);
							originalPacketsRecieved++;
							if (transferFinished){
								run = false;
							}
						} else {
							cout << "  |-Checksum " << FORERED << "failed\n" << RESETTEXT;
							cout << "  |-Sending " << "ACK " << FORERED  << ack << "\n" << RESETTEXT;
							cout << "  =========================\n";
							//send(confd, & ack, sizeof(ack), 0);
						}
                    } else {
						perror("Timeout");
						run = false;
					}
				}
			
				transferFinished = true;
			}

            fclose(fp); // Close file
            if (transferFinished) {
                cout << FOREGRN << "\nRecieve Success!\n" << RESETTEXT; // Print 'Recieve Success!'
				cout << "Number of original packets recieved: " << FORECYN << originalPacketsRecieved << RESETTEXT << "\n";
				cout << "Number of retransmitted packets recieved: " << FORECYN << retransmittedPacketsRecieved << RESETTEXT << "\n";
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

// serialize function, used to convert packet struct into char array for sending over sockets
void serialize(PACKET* msgPacket, char *data) {
	// Ints
    int *q = (int*)data;    
    *q = msgPacket->src_port;       q++;    
    *q = msgPacket->dst_port;       q++;    
    *q = msgPacket->seq;            q++; 
	*q = msgPacket->ttl;			q++;
    *q = msgPacket->checksum;       q++;
	// Bools
	bool *b = (bool*)q;
	*b = msgPacket->finalPacket;	b++;
	// Chars
    char *p = (char*)b;
    int i = 0;
    while (i < packetSize)
    {
        *p = msgPacket->buffer[i];
        p++;
        i++;
    }
}

// deserialize function, used to convert char array from socket into packet struct
void deserialize(char *data, PACKET* msgPacket) {
	// Ints
    int *q = (int*)data;    
    msgPacket->src_port = *q;        q++;    
    msgPacket->dst_port = *q;        q++;    
    msgPacket->seq = *q;             q++; 
	msgPacket->ttl = *q;			 q++;
    msgPacket->checksum = *q;        q++;
	// Bools
	bool *b = (bool*)q;
	msgPacket->finalPacket = *b;     b++;
	// Chars
    char *p = (char*)b;
    int i = 0;
	unsigned char buf[packetSize];
    while (i < packetSize)
    {
		buf[i] = *p;
		p++;
		i++;
    }
	msgPacket->buffer = reinterpret_cast<unsigned char*>(malloc(sizeof(unsigned char) * packetSize));
	memcpy( msgPacket->buffer, buf, sizeof(buf) );
}

// compute_crc16 function, used to quickly ensure packet integrity
int compute_crc16(unsigned char *buf){
	unsigned char x;
	unsigned short crc = 0xFFFF;

	int length = sizeof(buf);
	while (length--){
		x = crc >> 8 ^ *buf++;
		x ^= x>>4;
		crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
	}
	return (int) crc;
}

void copy_packet( PACKET* packet1, PACKET* packet2 ) {
	packet2->src_port = packet1->src_port;
	packet2->dst_port = packet1->dst_port;
	packet2->seq = packet1->seq;
	packet2->checksum = packet1->checksum;
	packet2->finalPacket = packet1->finalPacket;
	memcpy( packet2->buffer, packet1->buffer, sizeof(packet1->buffer) );
}