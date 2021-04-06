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
#include <random>
#include <math.h>

#define RESETTEXT "\x1B[0m" // Set all colors back to normal.
#define FORERED "\x1B[31m"  // Red
#define FOREGRN "\x1B[32m"  // Green
#define FOREYEL "\x1B[33m"  // Yellow
#define FORECYN "\x1B[36m"  // Cyan
#define FOREWHT "\x1B[37m"  // White
#define PBSTR "##################################################" // Progress bar filler
#define PBWIDTH 35 // Progress bar width

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
int client(bool debug);
int server(bool debug);

// Server/Client Variables
bool debug = false, isServer = false, isClient = false;
int packetSize = 512, pMode = 0, timeout = 0, sWindowSize = 0, sRangeLow = 0, sRangeHigh = 0, sErrors = 0, sRandomProb = 0, fileSize = 0;

// File info functions
int md5(char * fileName);
int fsize(FILE * fp);

// Packet Struct
typedef struct PACKET {
    int src_port;
    int dst_port;
    int seq;
	int ttl;
    int checksum;
    int buffSize;
	bool finalPacket;
    unsigned char *buffer;
	
	void print() {
		try {
			cout << "  |===PACKET===========\n";
			cout << "  |-source:       " << src_port << "\n";
			cout << "  |-dest:         " << dst_port << "\n";
			cout << "  |-seq:          " << seq << "\n";
			cout << "  |-ttl:          " << ttl << "\n";
			cout << "  |-checksum:     " << checksum << "\n";
			cout << "  |-buffSize:     " << buffSize << "\n";
			//cout << "  |-buffer:       '" << buffer << "'\n";
			cout << "  |-final:        " << finalPacket << "\n";
			cout << "  |====================\n";
		} catch (int i){
			
		}
	}	
} PACKET;

// Packet functions
void serialize(PACKET* msgPacket, char *data);
void deserialize(char *data, PACKET* msgPacket);
int compute_crc16(unsigned char *buf);
void copy_packet( PACKET* packet1, PACKET* packet2 );
void printWindow(int index, int windowSize, int totalFrames);

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
        if (!debug) {
			cout << FOREGRN << "[---------- SERVER ----------]\n";
        } else {
			cout << FOREGRN << "[---------- SERVER [DEBUG] ----------]\n";
		}
        responce = server(debug);
    } else if (isClient) {
        if (!debug) {
			cout << FOREGRN << "[---------- CLIENT ----------]\n";
        } else {
			cout << FOREGRN << "[---------- CLIENT [DEBUG] ----------]\n";
		}
        responce = client(debug);
    } else {
        cout << FORERED << "Invalid parameter(s) entered, please specify if you are running a server ('-s') or a client ('-c') and use ('-d') to run in debug mode\n" << RESETTEXT;
    }
    return 0;
}

// Client function, send file to server
int client(bool debug) {
    // Declare necessary variables/structures
    int sfd = 0, n = 0, b, port, packetNum = 0, count = 0, sDropPacketCount = 0;
	int droppedPackets[1024];
    char ip[32], fileName[64];
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
			// clear cin 
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			// print error and reset input value
			cout << FORERED << "Invalid Port Input Entered... please try again\n" << RESETTEXT;
			port = 0;
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
	
    // Open file for reading/sending
    FILE * fp = fopen(fileName, "rb");
    if (fp == NULL) {
        cout << FORERED << "ERROR OPENING FILE...PLEASE TRY AGAIN...\n" << RESETTEXT;
        perror("File");
        return 2;
    }	
    fileSize = fsize(fp);
	
    // Get packet size from user
    do {
		cout << "Packet Size (bytes): ";
		cin >> packetSize;
		if (packetSize < 1) {
			// clear cin 
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			// print error and reset input value
			cout << FORERED << "Invalid Packet Size Input Entered... please try again\n" << RESETTEXT;
			packetSize = 0;
        }
    } while (packetSize < 1);
    char sendbuffer[packetSize];
	
    // Get protocol from user
    do {
		cout << "Protocol (1=Stop And Wait, 2=Go-Back-N, 3=Selective Repeat): ";
		cin >> pMode;
		if (pMode < 1 || pMode > 3) {
			// clear cin 
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			// print error and reset input value
			cout << FORERED << "Invalid Protocol Input Entered... please try again\n" << RESETTEXT;
			pMode = 0;
        } else if  (pMode != 1){	
			// Get size of sliding window from user
			do {	
				cout << "Size Of Sliding Window: ";
				cin >> sWindowSize;
				if (sWindowSize < 2) {
					// clear cin 
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					// print error and reset input value
					cout << FORERED << "Invalid Sliding Window Input Entered... please try again\n" << RESETTEXT;
					sWindowSize = 0;
				}
			} while (sWindowSize < 2);
		
			// Get sequence range from user
			do {
				cout << "Sequence Range Low: ";
				cin >> sRangeLow;
				cout << "Sequence Range High: ";
				cin >> sRangeHigh;
				if (sRangeHigh < sRangeLow || sRangeLow < 0) {
					// clear cin 
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					// print error and reset input value
					cout << FORERED << "Invalid Range Input Entered... please try again\n" << RESETTEXT;
					sRangeLow = 0;
					sRangeHigh = 0;
				}
				if (((sRangeHigh - sRangeLow + 1) / 2 ) < sWindowSize ) {
					// clear cin 
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					// print error and reset input value
					cout << FORERED << "Sequence Range must be equal to or greater than twice the Window Size\n" << RESETTEXT; 
					sRangeLow = 0;
					sRangeHigh = 0;
				}
			} while (sRangeHigh < sRangeLow || sRangeLow < 0 || ((sRangeHigh - sRangeLow + 1) / 2 ) < sWindowSize);
		}
    } while (pMode < 1 || pMode > 3);
	
	// Get timeout from user
	do {
		cout << "Timeout (ms): ";
		cin >> timeout;
		if (timeout < 1) {
			cout << FORERED << "Invalid Timeout Input Entered... please try again\n" << RESETTEXT;
			timeout = 0;
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}
	} while (timeout < 1);
	
    // Get situational errors from user
	do {
		cout << "Situational Errors (1=none, 2=random, 3=custom): ";
		cin >> sErrors;
		if (sErrors == 2){
			do {
				cout << "Random Probability (%): ";
				cin >> sRandomProb;
				if (sRandomProb <= 1 && sRandomProb > 100) {
					cout << FORERED << "Invalid Random Probability Input Entered... please try again\n" << RESETTEXT;
					sRandomProb = 0;
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
				}
			} while (sRandomProb <= 1 && sRandomProb > 100);
		}
		
		if (sErrors == 3) {
			// Drop Packets
			do {
				// get number of packets to drop
				cout << "Please desired number of packets to drop: ";
				cin >> sDropPacketCount;
				// check input for invalid values
				if (sDropPacketCount <= 0 && sDropPacketCount >= fileSize) {
					cout << FORERED << "Invalid Number Of Packets Entered... please try again\n" << RESETTEXT;
					sDropPacketCount = 0;
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
				}
				// Loop to get all desired packet numbers
				for (int i = 0; i < sDropPacketCount; i++){
					do {
						// Get packet number
						cout << "Enter packet number to drop: ";
						cin >> droppedPackets[i];
						// check input for invalid values
						if (droppedPackets[i] <= 0 && droppedPackets[i] >= fileSize) {
							cout << FORERED << "Invalid Packet Number Entered... please try again\n" << RESETTEXT;
							droppedPackets[i] = 0;
							cin.clear();
							cin.ignore(numeric_limits<streamsize>::max(), '\n');
						}
					} while (droppedPackets[i] <= 0 && droppedPackets[i] >= fileSize);
				}
			} while (sDropPacketCount <= 0 && sDropPacketCount >= fileSize);
		}
		if (sErrors <= 0 || sErrors > 3) {
			cout << FORERED << "Invalid Situationel Errors Input Entered... please try again\n" << RESETTEXT;
			sErrors = 0;
        }
    } while (sErrors <= 0 || sErrors > 3);

    // Set family, port, and ip for socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    // Attempt to connect to server
    cout << FOREYEL << "\nAttempting to connect to " << ip << ":" << port << "\n" << RESETTEXT;
    if ((connect(sfd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) == -1)) {
        cout << FORERED << "CONNECTION FAILED...\n" << RESETTEXT;
        perror("Connect");
        return 1;
    } else {
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
	}

    // Set client variables
    bool run = true, transferFinished = false;
	int fileReadSize = packetSize - sizeof(PACKET);	
	int framesToSend = (int) ( fileSize / fileReadSize );
	if ( ( fileSize % fileReadSize ) > 0 ) {
		framesToSend++;
	}
	cout << "File size: " << fileSize << "\n";
	cout << "Packet size: " << packetSize << "\n";
	cout << "Original Packet Struct Size: " << sizeof(PACKET) << "\n";
	cout << "File read size: " << fileReadSize << "\n";
	cout << "Frames to send: " << framesToSend << "\n";
	int finalPacketSeq = -1;
	
	// Set output variables
	auto transferStart = chrono::high_resolution_clock::now(); 
	int originalPacketsSent = 0,retransmittedPacketsSent = 0;
	unsigned long long int totalBytesSent = 0, totalCorrectBytesSent = 0;

	// Begin File Transfer
    cout << "\n[File Transfer]\n" << RESETTEXT;
	
	// Stop and wait
	if (pMode == 1){
		// Stop And Wait
		while (run) {
			PACKET packetToResend;
			// memset sendbuffer to make sure its empty (might not be needed)
			memset(sendbuffer, 0, fileReadSize);
			// read/send file by desired packet size
			if (((b = fread(sendbuffer, 1, fileReadSize, fp)) > 0)) {
				
				char readBufferTrim[b];
				for (int t = 0; t < b; t++){
					readBufferTrim[t] = sendbuffer[t];
				}
				
				// Initialize char array for packet serialization
				char data[sizeof(PACKET) + b];
					
				// Build Packet
				PACKET* newMsg = new PACKET;
				newMsg->src_port = 0;
				newMsg->dst_port = port;
				newMsg->seq = packetNum;
				newMsg->ttl = 255;
				newMsg->checksum = compute_crc16(reinterpret_cast<unsigned char*>(readBufferTrim));
				newMsg->buffer = reinterpret_cast<unsigned char*>(readBufferTrim); 
				newMsg->buffSize = b;
				if (b < fileReadSize) {
					newMsg->finalPacket = true;
					finalPacketSeq = newMsg->seq;
				} else {
					newMsg->finalPacket = false;
				}
				
				// create backup packet in case it fails
				copy_packet(newMsg, &packetToResend);
				
				// Situational Errors
				bool damagePacket = false;
				if (sErrors == 2 && packetNum != 0 && newMsg->finalPacket == false){
					// Generate Random Number between 0 and 100
					std::random_device rd;
					std::mt19937 rng(rd());
					std::uniform_int_distribution<int> uni(0,100);
					auto random_integer = uni(rng);
					
					// Determine if random number means that we should trigger an error
					damagePacket = sRandomProb > (int)random_integer;
				} else if (sErrors == 3 && packetNum != 0 && newMsg->finalPacket == false){
					// Check if current packet needs to be damaged/dropped
					for (int i = 0; i < sDropPacketCount; i++){
						if(droppedPackets[i] == packetNum){
							damagePacket = true;
							droppedPackets[i] = (droppedPackets[i] * -1);
						}
					}
				}
				if (damagePacket && sErrors != 1){
					cout << "Intentially Damaging Packet " << packetNum << "\n";
					newMsg->checksum += 1;
				} 
				
				// Serialize packet into char array
				serialize(newMsg, data);
				int sentBytes = sizeof(data);
				
				// Send Packet
				send(sfd, data, sizeof(data), 0);
				totalBytesSent += sentBytes;
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
					// Check to see if ack has been recieved
					if (s = recv(sfd, &recievedAck, sizeof(recievedAck), MSG_DONTWAIT ) > 0) {
						if (recievedAck == packetNum) {
							// Correct Packet Ack recieved, transfer success
							cout << "  |-" << FOREGRN << "ACK " << recievedAck << " recieved" << RESETTEXT;
							cout << " (" << recievedAck + 1 << "/" << framesToSend << ")\n";
							ackRecieved = true;
							totalCorrectBytesSent += sentBytes;
							if (recievedAck == finalPacketSeq){
								transferFinished = true;
								cout << "  |-" << FOREGRN << "FINAL PACKET\n" << RESETTEXT ;
							}
							cout << "  |====================\n";
						} 
					}
					
					// calculate time ellapsed since timer was started
					auto now = chrono::high_resolution_clock::now();
					int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
					
					// Check to see if timeout has been reached
					if (timeout < ms && transferFinished == false ){
						cout << "  |-" << FORERED << "ACK timed out for packet "<< packetNum << RESETTEXT << ", resending...";
						//cout << "  |-Buffer: '" << newMsg->buffer << "'\n";
						//cout << "  |-Buffer: '" << packetToResend->buffer << "'\n";
						char data2[sizeof(PACKET) + b];
						serialize(&packetToResend, data2);
						// Resend packet
						send(sfd, data2, sizeof(data2), 0);
						// increment output variables
						totalBytesSent += sentBytes;
						retransmittedPacketsSent++;
						cout << "sent.\n";
						// reset timeout timer
						start = chrono::high_resolution_clock::now(); 
					} else if (transferFinished){
						run = false;
					}
				}
				
				if (newMsg->ttl <= 0){
					run = false;
					break;
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
		int ms;
		int s = 0, recievedAck;
		PACKET sendingPacket;
		PACKET resendingPacket;
		
		// Create vector of all frames of file
		cout << FORECYN << "Loading Frames into memory...\n";
		for (int i = 0; i <= framesToSend; i++) {
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
				newMsg.ttl = 5;
				newMsg.buffer = (unsigned char*)calloc(b, sizeof(char));
				for (int i = 0; i < b; i++){
					newMsg.buffer[i] = readBufferTrim[i];
				}
				newMsg.checksum = compute_crc16(newMsg.buffer); // compute crc16 from buffer
				newMsg.buffSize = b;
				if (i == framesToSend){
					newMsg.finalPacket = true;
				} else {
					newMsg.finalPacket = false;
				}
				frame f;
				f.packet = newMsg;
				all_frames.push_back(f);
				
				// Print progress bar progress
				double percentage = ((double) i / (double) framesToSend);
                int val = (int)(percentage * 100);
                int lpad = (int)(percentage * PBWIDTH);
                int rpad = PBWIDTH - lpad;
                printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
                fflush(stdout);
			}
			seq_num++;
		}
		cout << RESETTEXT << "\n";
		
		auto start = chrono::high_resolution_clock::now();
		do { 
			if ( next_seq_num < ( head_seq_num + sWindowSize ) && next_seq_num < framesToSend && head_seq_num < framesToSend ) {					
				copy_packet( &all_frames[next_seq_num].packet, &sendingPacket );
				char data[sizeof(PACKET) + sendingPacket.buffSize];
				// Serialize packet into char array
				serialize(&sendingPacket, data);
				// Send Packet
				send(sfd, data, sizeof(data), 0);
				totalBytesSent += sizeof(data);
				originalPacketsSent++;				
				sendingPacket.print();
				next_seq_num++;
			}	
			// Check to see if an ack has been recieved
			if (s = recv(sfd, &recievedAck, sizeof(recievedAck), MSG_DONTWAIT ) > 0) {
				cout << "Recieved Ack " << recievedAck << "\n";
				if (recievedAck == ( head_seq_num % sRangeHigh ) ) {
					all_frames[head_seq_num].ack = true;
					totalCorrectBytesSent += (all_frames[head_seq_num].packet.buffSize + sizeof(PACKET));
					head_seq_num++;
					cout << "Incrementing head index of window frame...\n";	
					printWindow(head_seq_num, sWindowSize, framesToSend);
				}
			}
			auto now = chrono::high_resolution_clock::now();
			ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
			if ( timeout < ms && head_seq_num < framesToSend ) {
				cout << "  |-" << FORERED << "ACK TIMEOUT  " << RESETTEXT << " (Resending Packet)\n";
				auto start = chrono::high_resolution_clock::now();
				for( int i = head_seq_num; i < next_seq_num; i++ ) {				
					copy_packet( &all_frames[i].packet, &resendingPacket );
					char data[sizeof(PACKET) + resendingPacket.buffSize];
					// Serialize packet into char array
					serialize(&resendingPacket, data);
					// Send Packet
					send(sfd, data, sizeof(data), 0);
					totalBytesSent += sizeof(data);
					retransmittedPacketsSent++;
					resendingPacket.print();					
				}					
			}
		} while ( head_seq_num < framesToSend );
		transferFinished = true;
	} 
	
	// Selective Repeat
	if (pMode == 3){ 
		// Create struct to track frames with their ack
		struct windowFrame {
			PACKET packet;
			bool ack;
			bool sent;
			int size;
			std::chrono::_V2::system_clock::time_point lifeStart;
		};
		// initialize frames[] array which will store file contents in memory
		windowFrame frames[framesToSend];
		// initialize window index (this will represent the low index in frames[])
		int windowLow = 0;
		
		// Load frames into memory
		cout << FORECYN << "Loading Frames into memory...\n";
		for (int i = 0; i <= framesToSend; i++){
			char readBuffer[fileReadSize];
			if (((b = fread(readBuffer, 1, fileReadSize, fp)) > 0)) {
				char readBufferTrim[b];
				for (int t = 0; t < b; t++){
					readBufferTrim[t] = readBuffer[t];
				}
				// Build Packet
				PACKET newMsg;
				newMsg.src_port = 0;
				newMsg.dst_port = port;
				newMsg.seq = i;
				newMsg.buffer = (unsigned char*)calloc(b, sizeof(char));
				for (int i = 0; i < b; i++){
					newMsg.buffer[i] = readBufferTrim[i];
				}
				newMsg.checksum = compute_crc16(reinterpret_cast<unsigned char*>(readBufferTrim)); // compute crc16 from buffer
				newMsg.buffSize = b;
				newMsg.ttl = 255;
				
				// Determine if this is the final packet
				if (b < fileReadSize) {
					newMsg.finalPacket = true;
					finalPacketSeq = i;
				} else {
					newMsg.finalPacket = false;
				}
				
				
				frames[i].packet = newMsg;
				frames[i].ack = false;
				frames[i].sent = false;
				
				// Print progress bar progress
				double percentage = ((double) i / (double) (framesToSend - 1));
                int val = (int)(percentage * 100);
                int lpad = (int)(percentage * PBWIDTH);
                int rpad = PBWIDTH - lpad;
                printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
                fflush(stdout);
			}
		}
		cout << RESETTEXT << "\n";
		
		bool shouldSendPacket = true;
		// Send frames in window until first frame is ack'd, then increment window
		while (run){
			int windowHigh = (windowLow + sWindowSize);
			if (windowHigh >= framesToSend){
				windowHigh = framesToSend - 1;
			}
			// Loop through frames
			cout << FORECYN << "Loop through all frames, Window: ";
			cout << "(low: " << windowLow << ", high: " << windowHigh << ")\n" << RESETTEXT;
			if (shouldSendPacket){
				for (int f = 0; f < framesToSend; f++){
					// if current frame is within window
					if (f >= windowLow && f <= windowHigh){
						//cout << FORECYN << "Frame " << f << " is in the current window\n" << RESETTEXT;
						
						if (frames[f].sent == false && frames[f].packet.buffSize > 0){
							// If current frame is not ack'd, send the frame to the server
							cout << FORECYN << "Frame " << f << " needs to be sent\n" << RESETTEXT;
							
							// Build packet to be sent
							PACKET currentPacket;
							copy_packet(&frames[f].packet, &currentPacket);
							char data[sizeof(PACKET) + currentPacket.buffSize];
							
							
							
							// Set frame as sent & set packet size
							frames[f].sent = true;
							frames[f].size = sizeof(data);
							frames[f].lifeStart = chrono::high_resolution_clock::now();
							
							// Situational Errors
							bool damagePacket = false;
							if (sErrors == 2 && packetNum != 0 && currentPacket.finalPacket == false){
								// Generate Random Number between 0 and 100
								std::random_device rd;
								std::mt19937 rng(rd());
								std::uniform_int_distribution<int> uni(0,100);
								auto random_integer = uni(rng);
								
								// Determine if random number means that we should trigger an error
								damagePacket = sRandomProb > (int)random_integer;
							} else if (sErrors == 3 && packetNum != 0 && currentPacket.finalPacket == false){
								// Check if current packet needs to be damaged/dropped
								for (int i = 0; i < sDropPacketCount; i++){
									if(droppedPackets[i] == packetNum){
										damagePacket = true;
										droppedPackets[i] = (droppedPackets[i] * -1);
									}
								}
							}
							if (damagePacket && sErrors != 1){
								cout << "Intentially Damaging Packet " << packetNum << "\n";
								currentPacket.checksum += 1;
							}
							
							// Serialize packet into char array
							serialize(&currentPacket, data);
							
							// Determine if packet is being resent
							if (frames[f].packet.ttl < 255){
								retransmittedPacketsSent++;
							} else if (frames[f].packet.ttl == 255) {
								originalPacketsSent++;	
							}
							
							// send packet
							send(sfd, data, sizeof(data), 0);
							totalBytesSent += frames[f].size;
							
							// Print output
							if (currentPacket.seq < 10 || debug == true) {
								// Print "Sent Packet x" <- x = current packet number
								cout << "Sent Packet #" << currentPacket.seq;
								
								// If in debug, print every packet size and packet contents
								if (debug) {
									// Write packet bytes size
									cout << "(" << sizeof(data) << " bytes)\n";
									// print packet data
									//currentPacket.print();
								} else {
									cout << "\n";
								}
								
								// If not in debug, print filler message after 10 packets
								if (currentPacket.seq == 9 && debug == false) {
									cout << "\nSending Remaining Packets...\n";
								}
							}
							
							// if packet being sent is last packet, set shouldSendPacket to false
							if (currentPacket.finalPacket == true){
								cout << "Final Packet Sent...\n";
								shouldSendPacket = false;
							}
						} else if (frames[f].sent == true && frames[f].ack == false ){
							cout << "Frame " << f << " needs to be ACK'd\n";
							// Calculate time passed since packet was sent
							auto now = chrono::high_resolution_clock::now();
							int lifetime = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - frames[f].lifeStart).count();
							
							// if life time of packet is greater than timeout, resend packet
							if (lifetime > timeout){
								frames[f].sent == false;
								cout << "Packet " << f << "'s ACK has timed out after " << lifetime << " ms...\n";
								frames[f].packet.ttl = frames[f].packet.ttl - 1;
								shouldSendPacket = true;
								break;
							} 
						}
					}
				}
			}
			
			
			
			// set variables for recieving acks
			int s = 0, recievedAck;
			auto ackTimeStart = chrono::high_resolution_clock::now();
			bool lowFrameAckd = false;
			
			// Loop until current low window frame is ack'd
			while (lowFrameAckd == false && transferFinished == false ){
				
				// Check to see if an ack has been recieved
				if (s = recv(sfd, &recievedAck, sizeof(recievedAck), MSG_DONTWAIT ) > 0) {
					// Reset timeout timer, since ack was recieved
					ackTimeStart = chrono::high_resolution_clock::now();
					cout << "Recieved Ack " << recievedAck << "\n";
					
					// set frame ack responce
					frames[recievedAck].ack = true;
						
					// Increment totalCorrectBytesSent for effective throughput
					totalCorrectBytesSent += frames[recievedAck].size;
						
					// check if recieved ack is last ack in sequence
					if (recievedAck == finalPacketSeq){
						cout << "Final Packet ACK Recieved...\n";
						transferFinished = true;
						run = false;
					}
						
					// if recieved ack is the current low window frame index, increment windowLow
					if (recievedAck == windowLow){
						windowLow++;
						lowFrameAckd = true;
						printWindow(windowLow, sWindowSize, framesToSend);
						break;
					}
				}
				
				// calculate time ellapsed since started recieving acks
				auto ackTimeNow = chrono::high_resolution_clock::now();
				int ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(ackTimeNow - ackTimeStart).count();
				
				// after 50ms of no ack recieved, skip back to sending packets 
				if (timeout < ms && shouldSendPacket == true){
					cout << FORERED << "ACK WAIT TIMEOUT" << RESETTEXT << "\n";
					ackTimeStart = chrono::high_resolution_clock::now();
					break;
				}
				
			}
		
		}
	}
    

	// Close file and socket
    fclose(fp); 
    close(sfd);
	
	// Finish File Transfer
    if (transferFinished) {
		// Calculate End Data
		auto transferEnd = chrono::high_resolution_clock::now();
		int totalEllapsedTime = (int)std::chrono::duration_cast<std::chrono::milliseconds>(transferEnd - transferStart).count();
		unsigned long long totalBitsSent = totalBytesSent * 8;
		unsigned long long totalCorrectBitsSent = totalCorrectBytesSent * 8;
		unsigned long long totalThroughput = totalBitsSent / totalEllapsedTime;
		unsigned long long effectiveThroughput = totalCorrectBitsSent / totalEllapsedTime;
		// Print End Data
        cout << FOREGRN << "\nSession Successfully Terminated!\n\n" << RESETTEXT;
		cout << "Number of original packets sent: " << FORECYN << originalPacketsSent << RESETTEXT << "\n";
		cout << "Number of retransmitted packets sent: " << FORECYN << retransmittedPacketsSent << RESETTEXT << "\n";
		cout << "Total elapsed time (ms): " << FORECYN << totalEllapsedTime << RESETTEXT << "\n";
		cout << "Total throughput (Mbps): " << FORECYN << ((double)totalThroughput / 1000) << RESETTEXT << "\n";
		cout << "Effective throughput (Mbps): " << FORECYN << ((double)effectiveThroughput / 1000) << RESETTEXT << "\n";
        md5(fileName); // Print md5
    } else {
        cout << FOREGRN << "\nSession Terminated Unsuccessfully...\n" << RESETTEXT;
    }
    return 0;
}

// Server function, recieve file from client
int server(bool debug) {
    // Declare necessary variables
    int fd = 0, confd = 0, b, num, port, packetNum = 0, count = 0, sDropAckCount = 0;
    struct sockaddr_in serv_addr;
    char fileName[64], ip[32];
	int droppedAcks[1024];
	
	// Get server IP from user
    do {
		cout << FOREWHT << "IP Address: ";
		scanf("%15s", ip);
		if (strlen(ip) == 0) {
			cout << FORERED << "Invalid IP Input Entered... please try again\n" << RESETTEXT;
		}
    } while (strlen(ip) == 0);
	
    // Get port from user
	do {
		cout << "Port: ";
		cin >> port;
		if (port < 1) {
			// clear cin 
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
			// print error and reset input value
			cout << FORERED << "Invalid Port Input Entered... please try again\n" << RESETTEXT;
			port = 0;
		}
    } while (port < 1);
	
	// Get output file from user
    do {
		cout << "Output File Name: ";
		scanf("%64s", fileName);
		if (strlen(fileName) == 0) {
			cout << FORERED << "Invalid File Name Input Entered... please try again\n" << RESETTEXT;
        }
    } while (strlen(fileName) == 0);
	
	 // Get situational errors from user
	do {
		cout << "Situational Errors (1=none, 2=random, 3=custom): ";
		cin >> sErrors;
		if (sErrors == 2){
			do {
				cout << "Random Probability (%): ";
				cin >> sRandomProb;
				if (sRandomProb <= 1 && sRandomProb > 100) {
					cout << FORERED << "Invalid Random Probability Input Entered... please try again\n" << RESETTEXT;
					sRandomProb = 0;
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
				}
			} while (sRandomProb <= 1 && sRandomProb > 100);
		}
		
		if (sErrors == 3) {
			// Drop Acks
			do {
				// get number of packets to drop
				cout << "Please desired number of acks to drop: ";
				cin >> sDropAckCount;
				// check input for invalid values
				if (sDropAckCount <= 0 && sDropAckCount >= fileSize) {
					cout << FORERED << "Invalid Number Of Acks Entered... please try again\n" << RESETTEXT;
					sDropAckCount = 0;
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
				}
				// Loop to get all desired packet numbers
				for (int i = 0; i < sDropAckCount; i++){
					do {
						// Get packet number
						cout << "Enter ack number to drop: ";
						cin >> droppedAcks[i];
						// check input for invalid values
						if (droppedAcks[i] <= 0 && droppedAcks[i] >= fileSize) {
							cout << FORERED << "Invalid Ack Number Entered... please try again\n" << RESETTEXT;
							droppedAcks[i] = 0;
							cin.clear();
							cin.ignore(numeric_limits<streamsize>::max(), '\n');
						}
					} while (droppedAcks[i] <= 0 && droppedAcks[i] >= fileSize);
				}
			} while (sDropAckCount <= 0 && sDropAckCount >= fileSize);
		}
		if (sErrors <= 0 || sErrors > 3) {
			cout << FORERED << "Invalid Situational Errors Input Entered... please try again\n" << RESETTEXT;
			sErrors = 0;
        }
    } while (sErrors <= 0 || sErrors > 3);
	
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
    cout << FOREYEL << "\nSocket started on " << ip << ":" << port << "\nWaiting for client connection...\n" << RESETTEXT;
    while (1) {
        // Accept connection and error handle
        confd = accept(fd, (struct sockaddr * ) NULL, NULL);
        if (confd == -1) {
            perror("Accept");
            continue;
        } else {
			cout << FOREGRN << "Connected to client!\n";
			
			// Recieve settings from client

			// Get Packet Size from client
			uint32_t tmp;
			read(confd, & tmp, sizeof(tmp));
			packetSize = ntohl(tmp);
			
			// Get File Size from client
			uint32_t tmp2;
			read(confd, & tmp2, sizeof(tmp2));
			fileSize = ntohl(tmp2);
			
			// Get Protocol from client
			uint32_t tmp3;
			read(confd, & tmp3, sizeof(tmp3));
			pMode = ntohl(tmp3);
			
			// Get timeout from client
			uint32_t tmp4;
			read(confd, & tmp4, sizeof(tmp4));
			timeout = ntohl(tmp4);
			
			// Get Size Of Sliding Window from client
			uint32_t tmp5;
			read(confd, & tmp5, sizeof(tmp5));
			sWindowSize = ntohl(tmp5);
			
			// Get Sequence Range Low from client
			uint32_t tmp6;
			read(confd, & tmp6, sizeof(tmp6));
			sRangeLow = ntohl(tmp6);
			
			// Get Sequence Range High from client
			uint32_t tmp7;
			read(confd, & tmp7, sizeof(tmp7));
			sRangeHigh = ntohl(tmp7);
			
			// Print recieved information from client
			cout << FOREYEL << "\n[Settings Recieved From Client]\n";
			if (pMode == 1) {
				cout << "| Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: Stop And Wait | Timeout (ms): " << timeout << " |\n";
			} else if (pMode == 2) {
				cout << "| Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: Go-Back-N | Timeout (ms): " << timeout << " |\n";
				cout << "| Sliding Window Size: " << sWindowSize << " | Sequence Range: [" << sRangeLow << "," << sRangeHigh << "] " << " |\n";
			} else if (pMode == 3) {
				cout << "| Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: Selective Repeat | Timeout (ms): " << timeout << " |\n";
				cout << "| Sliding Window Size: " << sWindowSize << " | Sequence Range: [" << sRangeLow << "," << sRangeHigh << "] " << " |\n";
			}
			
			// Print situational errors output
			if (sErrors == 2){
				cout << "| Situational Errors: Random | Probability: " << sRandomProb << "% |\n";
			} else if (sErrors == 3){
				cout << "| Situational Errors: Custom | Dropped Acks: ";
				for (int i = 0; i < sDropAckCount; i++){
					cout << droppedAcks[i] << " ";
				}
				cout << "|\n";
			}
			cout << RESETTEXT;
		}
        
		// Declare new recieving buff[] to be length of packet size and allocate memory for new buffer
		char recieveBuffer[packetSize];
		memset(recieveBuffer, '0', sizeof(recieveBuffer));
		
		// Open new file for writing
        FILE * fp = fopen(fileName, "wb");

        // Begin Filel Transfer
        cout << "\n[File Transfer]\n";
        if (fp != NULL) {
            bool run = true;
			bool transferFinished = false;
			int ack;
			int lastPacketSeq = 0;
			
			struct timeval tv;
			tv.tv_sec = 10;
			if (setsockopt(confd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
				perror("Error");
			}
			
			// Set up variables for final output
			int originalPacketsRecieved = 0, retransmittedPacketsRecieved = 0;
			int fileReadSize = packetSize - sizeof(PACKET);
			int framesToReceive= (int) (fileSize/fileReadSize);
			if ( ( fileSize % fileReadSize ) > 0 ) {
				framesToReceive++;
			}
			cout << "Frames to recieve: " << framesToReceive << "\n";
			
			// Stop and wait
			if (pMode == 1) {
				int previousPacketSeq = -1;
				int lastRecievedSeq = -1;
				// Stop And Wait
				while (run){
					char data[packetSize];
					char data2[packetSize];
                    if ((b = recv(confd, data, packetSize, MSG_WAITALL)) > 0) {
						
						// Deserialize packet
						PACKET* recievedPacket = new PACKET;
						deserialize(data, recievedPacket);
						// Calculate amount of bytes to write to file
						int writeBytes = recievedPacket->buffSize;
						
						if (recievedPacket->seq >= 0 && recievedPacket->seq < framesToReceive && b > recievedPacket->buffSize){
							// Compute CRC16 from duplicated 'temp' packet (using actual packet causes data corruption)
							PACKET* temp = new PACKET;
							memcpy( data2, data, sizeof(data) );
							deserialize(data2, temp);
							int crcNew = compute_crc16(temp->buffer);
							
							// Determine if current packet is final packet
							if (recievedPacket->finalPacket == true) {
								transferFinished = true;
								lastPacketSeq = recievedPacket->seq;
							}
							
							// Set ack responce
							ack = recievedPacket->seq;
							
							// Print output
							if (recievedPacket->seq < 10 || debug == true) {
								// Print "Sent Packet x" <- x = current packet number
								cout << "Recieved Packet #" << recievedPacket->seq;
								
								if (lastRecievedSeq == recievedPacket->seq){
									cout << " (again)";
									// If in debug, print packet size
									if (debug) {
										// Write packet bytes size
										cout << " (" << writeBytes + sizeof(PACKET) << " bytes)\n";
									} else {
										cout << "\n";
									}
								} else {
									lastRecievedSeq = recievedPacket->seq;
									// If in debug, print every packet size and packet contents
									if (debug) {
										// Write packet bytes size
										cout << " (" << writeBytes + sizeof(PACKET) << " bytes)\n";
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
							}
							
							// Validate checksum and send ack if correct
							if (recievedPacket->checksum == crcNew){
								cout << "  |-Checksum " << FOREGRN << "OK\n" << RESETTEXT;
								cout << "  |-Sending " << "ACK " << FOREGRN  << ack << RESETTEXT << "..." ;
								
								// Situational Errors
								bool looseAck = false;
								if (sErrors == 2 && ack != 0 && recievedPacket->finalPacket == false){
									// Generate random number in range of 0 to 100
									std::random_device rd;
									std::mt19937 rng(rd());
									std::uniform_int_distribution<int> uni(0,100);
									auto random_integer = uni(rng);
									
									// Use random number to determine if ack should be lost
									looseAck = sRandomProb > (int)random_integer;
								} else if (sErrors == 3 && ack != 0 && recievedPacket->finalPacket == false){
									// Custom situational Errors
									for (int i = 0; i < sDropAckCount; i++){
										if(droppedAcks[i] == ack){
											looseAck = true;
											droppedAcks[i] = (droppedAcks[i] * -1);
										}
									}
								}
								
								if (looseAck){
									// Should loose/not send ACK, this will trigger the client to re-send
									cout << FORERED << "failed.\n" << RESETTEXT;
								} else {
									// Should not loose ack, send ACK
									send(confd, & ack, sizeof(ack), 0);
									cout << FOREGRN << "sent.\n" << RESETTEXT;
								}
								
								if (previousPacketSeq != ack && looseAck == false){
									originalPacketsRecieved++;
									// Write packet buffer to file
									cout << "  |-Writing to file...\n";
									fwrite(recievedPacket->buffer, 1, writeBytes, fp);
									// update previousPacketSeq to current packet seq
									previousPacketSeq = ack;
									if (transferFinished){
										run = false;
									}
								} else {
									retransmittedPacketsRecieved++;
								}
								cout << "  |====================\n";
							} else {
								cout << "  |-Checksum " << FORERED << "failed\n" << RESETTEXT;
								cout << "  | " << recievedPacket->checksum << " vs " << crcNew << "\n";
								retransmittedPacketsRecieved++;
							}
							
						} else {
							cout << "weird packet recieved... (#" << recievedPacket->seq << "), skipping...\n";
							for (int i = 0; i < sizeof(data); i++){
								data[i] = NULL;
							}
						}
						
						
                    } else {
						cout << "timeout reached";
						run = false;
						break;
						//perror("Timeout");
					}
				}
			}
			
			// Go-Back-N
			if (pMode == 2) {
				int next_seq_num = 0; 
				char data[packetSize];
				char data2[packetSize];
				do {	
                    if ((b = recv(confd, data, packetSize, MSG_WAITALL)) > 0) {						
						// Deserialize packet
						PACKET recievedPacket;
						deserialize(data, &recievedPacket);
						
						// Print output
						if (recievedPacket.seq < 10 || debug == true) {
							// Print "Sent Packet x" <- x = current packet number
							cout << "Recieved Packet #" << recievedPacket.seq;
							
							// If in debug, print every packet size and packet contents
							if (debug) {
								// Write packet bytes size
								cout << "(" << b << " bytes)\n";
								// print packet data
								recievedPacket.print();
							} else {
								cout << "\n";
							}
							
							// If not in debug, print filler message after 10 packets
							if (recievedPacket.seq == 9 && debug == false) {
								cout << "\nRecieving Remaining Packets...\n" << FORECYN;
							}
						}
						
						// Validate checksum, write to file, send ack if next expected packet
						if ( recievedPacket.seq == (next_seq_num % sRangeHigh) ) { 
							// Compute CRC16 from duplicated 'temp' packet (using actual packet causes data corruption)
							PACKET temp;
							memcpy( data2, data, sizeof(data) );
							deserialize(data2, &temp);
							int crcNew = compute_crc16(temp.buffer);
							if (recievedPacket.checksum == crcNew) {
								ack = recievedPacket.seq;
								cout << "  |-Checksum " << FOREGRN << "OK\n" << RESETTEXT;
								cout << "  |-Sending " << "ACK " << FOREGRN  << ack << "\n" << RESETTEXT;
								cout << "  |====================\n";
								// Calculate amount of bytes to write to file
								int writeBytes = recievedPacket.buffSize;
								fwrite(recievedPacket.buffer, 1, writeBytes, fp);
								send(confd, &ack, sizeof(ack), 0);
								originalPacketsRecieved++;
								next_seq_num++;
								if ( next_seq_num < framesToReceive ) {
									cout << "Current Window [" << next_seq_num << "]\n";
								}
							} else {
								cout << "  |-Checksum " << FORERED << "failed\n" << RESETTEXT;
								cout << "  =========================\n";
								cout << "Current Window [" << next_seq_num << "]\n";
							}
						}
					}
				} while ( next_seq_num < framesToReceive );
				transferFinished = true;
			} 
			
			// Selective Repeat
			if (pMode == 3) {
				struct windowFrame {
					PACKET packet;
					bool ack;
				};
				// initialize frames[] array which will store file contents in memory
				windowFrame frames[framesToReceive];
				for (int i = 0; i < framesToReceive; i++){
					frames[i].ack = false;
				}
				// initialize window index (this will represent the low index in frames[])
				int windowLow = 0;
				int recievedPacketsInWindow = 0;
				bool recievePackets = true;
				while (run){
					int windowHigh = (windowLow + sWindowSize);
					if (windowHigh >= framesToReceive){
						windowHigh = framesToReceive - 1;
					}
					// recieve packets for window
					char data[packetSize];
					char data2[packetSize];
					cout << FORECYN << "Check if we should be recieving packets\n" << RESETTEXT;
					if (recievedPacketsInWindow < sWindowSize && transferFinished == false && recievePackets == true){
						cout << FORECYN << "Waiting for incomming packets\n" << RESETTEXT;
						if ((b = recv(confd, data, packetSize, MSG_WAITALL)) > 0) {
							// Deserialize packet
							PACKET recievedPacket;
							deserialize(data, &recievedPacket);
							int frameIndex = recievedPacket.seq;
							
							// Add packet to frames[]
							frames[frameIndex].packet = recievedPacket;
							recievedPacketsInWindow++;
							
							// Create copy of packet to calculate checksum
							PACKET copyPacket;
							copy_packet(&recievedPacket, &copyPacket);
							int crcNew = compute_crc16(copyPacket.buffer);
							
							// Set ack value for frame
							if (recievedPacket.checksum == crcNew){
								cout << "  |-Checksum " << FOREGRN << "OK\n" << RESETTEXT;
								// Situational Errors
								bool looseAck = false;
								if (sErrors == 2 && frameIndex != 0 && recievedPacket.finalPacket == false){
									// Generate random number in range of 0 to 100
									std::random_device rd;
									std::mt19937 rng(rd());
									std::uniform_int_distribution<int> uni(0,100);
									auto random_integer = uni(rng);
									
									// Use random number to determine if ack should be lost
									looseAck = sRandomProb >= (int)random_integer;
									cout << "sRandomProb: " << sRandomProb << " random_integer: " << (int)random_integer << " looseAck: " << looseAck << "\n";
								} else if (sErrors == 3 && frameIndex != 0 && recievedPacket.finalPacket == false){
									// Custom situational Errors
									for (int i = 0; i < sDropAckCount; i++){
										if(droppedAcks[i] == frameIndex){
											looseAck = true;
											droppedAcks[i] = (droppedAcks[i] * -1);
											cout << "Packet " << frameIndex << " will loose its ACK\n";
										}
									}
								}
								
								if (recievedPacket.finalPacket == true){
									recievePackets = false;
								}
								
								if (!looseAck){
									// Should not loose ack
									frames[frameIndex].ack = true;
								} else {
									frames[frameIndex].ack = false;
									cout << "Loosing ACK " << frameIndex << "\n";
								}
								
							} else {
								frames[frameIndex].ack = false;
								cout << "  |-Checksum " << FORERED << "failed\n" << RESETTEXT;
							}
							
							
							// Print output
							if (recievedPacket.seq < 10 || debug == true) {
								// Print "Sent Packet x" <- x = current packet number
								cout << "Recieved Packet #" << frameIndex;
								
								// If in debug, print packet data
								if (debug) {
									cout << "(" << b << " bytes)\n";
									//recievedPacket.print();
								} else {
									cout << "\n";
								}
								
								// If not in debug, print filler message after 10 packets
								if (recievedPacket.seq == 9 && debug == false) {
									cout << "\nRecieving Remaining Packets...\n";
								}
							}
						
						} else {
							// Packets expected, but none are recieved
							//cout << FORERED << "PACKETS IN WINDOW:\n";
							for (int i = windowLow; i < windowHigh; i++){
								if (frames[i].ack = false){
									recievePackets = true;
									cout << "Packet " << i << " still needs an ack\n";
									break;
								}
							}
							//cout << RESETTEXT << "\n";
							//perror("Timeout");
						} 
					} else {
						// Loop through frames in window
						cout << FORECYN << "Loop through frames in current window ";
						cout << "(low: " << windowLow << ", high: " << windowHigh << ")\n" << RESETTEXT;
						for (int i = windowLow; i <= windowHigh; i++){
							cout << FORECYN << "Checking packet/frame " << i << "'s checksum result\n" << RESETTEXT;
							
							if (frames[i].ack == true) {
								cout << "Sending " << "ACK " << i << RESETTEXT << "...";
								
								// ACK
								int ack = i;
								send(confd, & ack, sizeof(ack), 0);
								cout << FOREGRN << "sent.\n" << RESETTEXT;
								
								// Check if current packet is low index of window
								if (i == windowLow){
									cout << "Writing packet " << i << " to file\n";
									// Write packet payload to file
									fwrite(frames[i].packet.buffer, 1, frames[i].packet.buffSize, fp);
									
									// Increment windowLow & Decrease recievedPacketsInWindow
									windowLow++;
									recievedPacketsInWindow--;
									
									// Print current window
									printWindow(windowLow, sWindowSize, framesToReceive);
									
									// If packet is final packet, end transmission
									if (frames[i].packet.finalPacket){
										cout << "Final packet recieved\n";
										transferFinished = true;
										lastPacketSeq = i;
									} 
									break;
								}
								
							} else if (frames[i].ack == false) {
								cout << "Sending " << "ACK " << i << "..." << FORERED << "failed.\n" << RESETTEXT;
								recievePackets = true;
								// Print current window
								printWindow(windowLow, sWindowSize, framesToReceive);
								break;
							}
						}
					}
					
					if (transferFinished){
						run = false;
					}
				}
			}

			// Close file
			fclose(fp);
			
			// Print End Data
            if (transferFinished) {
				cout << "\nLast packet seq# recieved: " << FORECYN << lastPacketSeq << RESETTEXT << "\n";
				cout << "Number of original packets recieved: " << FORECYN << originalPacketsRecieved << RESETTEXT << "\n";
				cout << "Number of retransmitted packets recieved: " << FORECYN << retransmittedPacketsRecieved << RESETTEXT << "\n";
				// Print md5
                md5(fileName);
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
    *q = msgPacket->buffSize;       q++;
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
    msgPacket->buffSize = *q;        q++;
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

// Copy contents of packet 1 to packet 2
void copy_packet( PACKET* packet1, PACKET* packet2 ) {
	packet2->src_port = packet1->src_port;
	packet2->dst_port = packet1->dst_port;
	packet2->seq = packet1->seq;
	packet2->ttl = packet1->ttl;
	packet2->checksum = packet1->checksum;
	packet2->buffSize = packet1->buffSize;
	packet2->finalPacket = packet1->finalPacket;
	
	packet2->buffer = (unsigned char*)calloc(packet1->buffSize, sizeof(char));
	for (int i = 0; i < packet1->buffSize; i++){
		packet2->buffer[i] = packet1->buffer[i];
	}
}

// Prints current sliding window
void printWindow(int index, int windowSize, int totalFrames){
	totalFrames--;
	int lowerWindow = index;
	int upperWindow = index + windowSize;
	if (upperWindow > totalFrames){
		upperWindow = totalFrames;
	}
	cout << "Current Window [";
	for (int i = lowerWindow; i <= upperWindow; i++){
		if (i != upperWindow){
			cout << i << ", ";
		} else {
			cout << i;
		}
	}
	cout << "]\n";
}