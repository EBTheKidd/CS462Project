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

// Display stuff
#define RESETTEXT "\x1B[0m" // Set all colors back to normal.
#define FORERED "\x1B[31m" // Red
#define FOREGRN "\x1B[32m" // Green
#define FOREYEL "\x1B[33m" // Yellow
#define FORECYN "\x1B[36m" // Cyan
#define FOREWHT "\x1B[37m" // White
#define PBSTR "##################################################" // Progress bar filler
#define PBWIDTH 35 // Progress bar width

// PHONEIX SERVER IPs (ignore)
// Phoneix0: 10.35.195.251
// Phoneix1: 10.34.40.33
// Phoneix2: 10.35.195.250
// Phoneix3: 10.34.195.236

using namespace std;

int client(bool debug, int pMode); // Client
int server(bool debug, int pMode); // Server
int md5(char * fileName); // MD5
int fsize(FILE * fp); // File Size

int main(int argc, char * argv[]) { // Main function, parses arguments to determine server/client
    int responce = 0, mode = 0;
    bool debug = false, isServer = false, isClient = false, GBN = false, SR = false;
    system("clear");
    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-s") {
            isServer = true;
        } else if (arg == "-c") {
            isClient = true;
        } else if (arg == "-d") {
            debug = true;
        } else if (arg == "-gbn") {
            GBN = true;
			mode = 1;
        } else if (arg == "-sr") {
            SR = true;
			mode = 2;
        }
    }
	
	if (mode == 0){
		cout << FORERED << "Please specify protocol type ('-gbn') for Go-Back-N or ('-sr') for Selective Repeat\n" << RESETTEXT;
		return 0;
	} else {
		if (isServer) {
			cout << FOREGRN << "[SERVER]";
			if (debug) {
				cout << "[DEBUG]";
			}
			if (GBN) {
				cout << "[GBN]";
			}
			if (SR) {
				cout << "[SR]";
			}
			cout << "\n";
			responce = server(debug, mode);
		} else if (isClient) {
			cout << FOREGRN << "[CLIENT]";
			if (debug) {
				cout << "[DEBUG]";
			}
			if (GBN) {
				cout << "[GBN]";
			}
			if (SR) {
				cout << "[SR]";
			}
			cout << "\n";
			responce = client(debug, mode);
		} else {
			cout << FORERED << "Invalid parameter(s) entered, please specify if you are running a server ('-s') or a client ('-c') and use ('-d') to run in debug mode\n" << RESETTEXT;
		}
	}
    return 0;
}
int client(bool debug, int pMode) { // Client function, send file to server
	// Declare necessary variables/structures
    int sfd = 0, n = 0, b, port, packetSize, packetNum = 0, count = 0, totalSent = 0;
    char ip[32], fileName[64];
    struct sockaddr_in serv_addr;
	// Initialize socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
	// Get input from user
    cout << FOREWHT << "IP Address: ";
    scanf("%15s", ip);
    cout << "Port: ";
    cin >> port;
    cout << "File Name: ";
    scanf("%64s", fileName);
    cout << "Packet Size (kb): ";
    cin >> packetSize;
    packetSize = packetSize * 1000;
    char sendbuffer[packetSize];
	// Set family, port, and ip for socket
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
	// Connect to server
    cout << FOREYEL << "Attempting to connect to " << ip << ":" << port << "\n" << RESETTEXT;
    if ((connect(sfd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) == -1)) {
        cout << FORERED << "CONNECTION FAILED...\n" << RESETTEXT;
        perror("Connect");
        return 1;
    }
    cout << FOREGRN << "Connected to server!\n" << RESETTEXT;
	// Open file for sending
    FILE * fp = fopen(fileName, "rb"); 
    if (fp == NULL) {
        cout << FORERED << "ERROR OPENING FILE...\n" << RESETTEXT;
        perror("File");
        return 2;
    }
    int fileSize = fsize(fp);
	// Send packet size to server
    uint32_t tmp = htonl(packetSize); 
    write(sfd, & tmp, sizeof(tmp));
	// Send file size to server
    uint32_t tmp2 = htonl(fileSize); 
    write(sfd, & tmp2, sizeof(tmp2));
	// Set client variables
    int mode = 1;
    bool run = true;
	// Loop until total bytes sent is equal to file size bytes
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
                } else if (!debug) {
					// Progress bar
                    double percentage = ((double) totalSent / (double) fileSize);
                    int val = (int)(percentage * 100);
                    int lpad = (int)(percentage * PBWIDTH);
                    int rpad = PBWIDTH - lpad;
                    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
                    fflush(stdout);
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
        cout << FOREGRN << "\n\nSend Success!\n" << RESETTEXT; // Print 'Send Success!'
        md5(fileName); // Print md5
    } else {
        cout << FORERED << "\n\nSend Failed!\n" << RESETTEXT;
    }
    close(sfd); // Close socket
    return 0;
}
int server(bool debug, int pMode) { // Server function, connects to a client, recieves packets of file contents, and writes result to file
	// Declare necessary variables
    int fd = 0, confd = 0, b, num, port, packetSize, packetNum = 0, count = 0, totalRecieved = 0;
    struct sockaddr_in serv_addr;
    char fileName[64], initialBuff[32], ip[32];
	// Get user input
    cout << FOREWHT << "IP Address: "; 
    scanf("%15s", ip);
    cout << "Port: ";
    cin >> port;
    cout << "Output File Name: ";
    scanf("%64s", fileName);
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
    cout << FOREYEL << "Socket started on " << ip << ":" << port << "\n";
    cout << "Waiting for client connection...\n" << RESETTEXT;
    while (1) {
		// Accept connection and error handle
        confd = accept(fd, (struct sockaddr * ) NULL, NULL); 
        if (confd == -1) {
            perror("Accept");
            continue;
        }
        cout << FOREGRN << "Connected to client!\n" << FOREWHT;
		// Get Packet Size from client
        uint32_t tmp; 
        read(confd, & tmp, sizeof(tmp));
        packetSize = ntohl(tmp);
		// Declare new recieving buff[] to be length of packet size and allocate memory for new buffer
        char recieveBuffer[packetSize]; 
        memset(recieveBuffer, '0', sizeof(recieveBuffer));
        uint32_t tmp2; // Get File Size
        read(confd, & tmp2, sizeof(tmp2));
        int fileSize = ntohl(tmp2);
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
                            if (packetNum == 9 && debug == false) {
                                cout << "\nRecieving Remaining Packets...\n" << FORECYN;
                            }
                        } else if (!debug) { 
							// Progress bar
                            double percentage = ((double) totalRecieved / (double) fileSize);
                            int val = (int)(percentage * 100);
                            int lpad = (int)(percentage * PBWIDTH);
                            int rpad = PBWIDTH - lpad;
                            printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
                            fflush(stdout);
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
                cout << FOREGRN << "\n\nRecieve Success!\n" << RESETTEXT; // Print 'Recieve Success!'
                md5(fileName); // Print md5
            } else {
                cout << FORERED << "\n\nRecieve Failed!\n" << RESETTEXT;
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
int md5(char * fileName) { // MD5 function, uses system call to output md5sum of file
    string str(fileName);
    cout << FORECYN << "\nMD5 result for '" << str << "'\n";
    string command = "md5sum " + str;
    system(command.c_str());
    cout << "\n" << RESETTEXT;
    return 0;
}
int fsize(FILE * fp) { // fsize function, returns input value's file size 
    int prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, prev, SEEK_SET);
    return sz;
}