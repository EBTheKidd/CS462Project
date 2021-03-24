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

//#define BUFSIZE 512
#define PACKETSIZE sizeof(MSG)

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
typedef struct MSG {
    int src_port;
    int dst_port;
    int seq;
    int ack;
    int window_size;
    int checksum;
	bool finalPacket;
    unsigned char *buffer;
	
	
	void print() {
		try {
			cout << "  ========PACKET=======\n";
			cout << "  |-source: " << src_port << "\n";
			cout << "  |-dest: " << dst_port << "\n";
			cout << "  |-seq: " << seq << "\n";
			cout << "  |-ack: " << ack << "\n";
			cout << "  |-window size: " << window_size << "\n";
			cout << "  |-checksum: " << checksum << "\n";
			cout << "  |-buffer: '" << buffer << "'\n";
			cout << "  |-final: " << finalPacket << "\n";
			cout << "  =====================\n";
		} catch (int i){
			
		}
	}

	
}MSG;

// Packet functions
void serialize(MSG* msgPacket, char *data);
void deserialize(char *data, MSG* msgPacket);
void compute_crc16(unsigned char *buf);


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
    cout << FOREWHT << "IP Address: ";
    scanf("%15s", ip);
    if (strlen(ip) == 0) {
        cout << FORERED << "Invalid IP Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get server Port from user
    cout << "Port: ";
    cin >> port;
    if (port < 1) {
        cout << FORERED << "Invalid Port Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get file name from user
    cout << "File Name: ";
    scanf("%64s", fileName);
    if (strlen(fileName) == 0) {
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
    cout << "Packet Size (bytes): ";
    cin >> packetSize;
    if (packetSize < 1) {
        cout << FORERED << "Invalid Packet Size Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    //packetSize = packetSize * 1000;
    char sendbuffer[packetSize];
    // Get protocol from user
    cout << "Protocol (1=GBN 2=SR): ";
    cin >> pMode;
    if (pMode < 1 || pMode > 2) {
        cout << FORERED << "Invalid Protocol Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get timeout from user
    cout << "Timeout (ms): ";
    cin >> timeout;
    if (timeout < 1) {
        cout << FORERED << "Invalid Timeout Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get size of sliding window from user
    cout << "Size Of Sliding Window: ";
    cin >> sWindowSize;
    if (sWindowSize < 1) {
        cout << FORERED << "Invalid Sliding Window Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get sequence range from user
    cout << "Sequence Range Low: ";
    cin >> sRangeLow;
    cout << "Sequence Range High: ";
    cin >> sRangeHigh;
    if (sRangeHigh < sRangeLow || sRangeLow < 0) {
        cout << FORERED << "Invalid Range Input Entered... please try again\n" << RESETTEXT;
        return 0;
    }
    // Get situational errors from user
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
    } else if (sErrors == 0 || sErrors > 3) {
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
    bool run = true, transferFinished = false;
    // Loop until total bytes sent is equal to file size bytes
    cout << "\n[File Transfer]\n" << RESETTEXT;
    while (run) {
        switch (mode) {
        case 1: 
		{
			int fileReadSize = packetSize - sizeof(MSG); // amount of file to read, with accounting for size of struct
            memset(sendbuffer, 0, fileReadSize); // memset sendbuffer to make sure its empty (might not be needed)
            // read/send file by desired packet size
            if (((b = fread(sendbuffer, 1, fileReadSize, fp)) > 0)) {
				// Initialize char array for packet serialization
				char data[sizeof(MSG) + b];
				
                // Build Packet
				MSG* newMsg = new MSG;
				newMsg->src_port = 1234;
				newMsg->dst_port = port;
				newMsg->seq = packetNum;
				newMsg->ack = 5;
				newMsg->window_size = 10;
				newMsg->checksum = 555;
				newMsg->buffer = reinterpret_cast<unsigned char*>(sendbuffer);
				if (b < fileReadSize) {
                    newMsg->finalPacket = true;
					transferFinished = true;
					run = false;
                } else {
                    newMsg->finalPacket = false;
                }
				
				// Compute CRC16
				compute_crc16(reinterpret_cast<unsigned char*>(sendbuffer));
				
                // Print output
                if (packetNum < 10 || debug == true) {
					// Print "Sent Packet x" <- x = current packet number
                    cout << "Sent Packet #" << (packetNum + 1);
					
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
				
				// Serialize packet into char array
				serialize(newMsg, data);
				
                // Send Packet
                send(sfd, data, sizeof(data), 0);
                // Increase packet counter
                packetNum++;
                // Switch to validation/protocol mode
                mode = 2;
            }
		}
            break;
        case 2:
		{
            if (pMode == 1) {
                // Place GBN client functionality here
            } else if (pMode == 2) {
                // Place SR client functionality here
            }
            mode = 1; // after validation, set mode back to 1 for normal client functions
		}
            break;
        case 0:
		{
            cout << "Exiting...\n" << RESETTEXT;
		}
            break;
        }
    }
    fclose(fp); // Close file
    if (transferFinished) {
        cout << FOREGRN << "\nSend Success!\n" << RESETTEXT; // Print 'Send Success!'
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
    bool transferFinished = false;

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
            cout << "Packet Size (bytes): " << packetSize << " | File Size: " << fileSize << " | Protocol: GBN\n";
        } else if (pMode == 2) {
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
            while (run) {
                switch (mode) {
                case 1:
				{
					char data[packetSize];
					char data2[packetSize];
                    if (((b = recv(confd, data, packetSize, MSG_WAITALL)) > 0)) {
						// Calculate amount of bytes to write to file
						int writeBytes = b - sizeof(MSG);
						
						// Deserialize packet
						MSG* temp = new MSG;
						deserialize(data, temp);
						
						// Compute CRC16 
						MSG* temp2 = new MSG;
						memcpy( data2, data, sizeof(data) );
						deserialize(data2, temp2);
						compute_crc16(temp2->buffer);
						
						// Determine if current packet is final packet
                        if (temp->finalPacket == true) {
                            transferFinished = true;
                            run = false;
                        }
						
                        // Print output
                        if (packetNum < 10 || debug == true) {
							// Print "Sent Packet x" <- x = current packet number
                            cout << "Recieved Packet #" << (packetNum + 1);
							
							// If in debug, print every packet size and packet contents
                            if (debug) {
								// Write packet bytes size
                                cout << "(" << b << " bytes)\n";
								// print packet data
								temp->print();
                            } else {
                                cout << "\n";
                            }
							
							// If not in debug, print filler message after 10 packets
                            if (packetNum == 9 && debug == false) {
                                cout << "\nRecieving Remaining Packets...\n" << FORECYN;
                            }
                        }
						
                        // Write packet buffer to file
                        fwrite(temp->buffer, 1, writeBytes, fp);
                        // Increase packet count
                        packetNum++;
                        // Switch to validation/protocol mode
                        mode = 2;
                    }
				}
                    break;
                case 2:
				{
                    if (pMode == 1) {
                        // Place GBN server functionality here
                    } else if (pMode == 2) {
                        // Place SR server functionality here
                    }
                    mode = 1; // after validation, set mode back to 1 for normal server functions
				}
                    break;
                }

            }

            fclose(fp); // Close file
            if (transferFinished) {
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

// serialize function, used to convert packet struct into char array for sending over sockets
void serialize(MSG* msgPacket, char *data) {
	// Ints
    int *q = (int*)data;    
    *q = msgPacket->src_port;       q++;    
    *q = msgPacket->dst_port;       q++;    
    *q = msgPacket->seq;            q++; 
    *q = msgPacket->ack;            q++; 
    *q = msgPacket->window_size;    q++;
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
void deserialize(char *data, MSG* msgPacket) {
	// Ints
    int *q = (int*)data;    
    msgPacket->src_port = *q;        q++;    
    msgPacket->dst_port = *q;        q++;    
    msgPacket->seq = *q;             q++; 
    msgPacket->ack = *q;             q++; 
    msgPacket->window_size = *q;     q++; 
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
void compute_crc16(unsigned char *buf){
	unsigned char x;
	unsigned short crc = 0xFFFF;

	int length = sizeof(buf);
	while (length--){
		x = crc >> 8 ^ *buf++;
		x ^= x>>4;
		crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
	}
	cout << "Computed Checksum: " << crc << "\n";
}