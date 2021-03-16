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

#define RESETTEXT "\x1B[0m" // Set all colors back to normal.
#define FORERED "\x1B[31m" // Red
#define FOREGRN "\x1B[32m" // Green
#define FOREYEL "\x1B[33m" // Yellow
#define FORECYN "\x1B[36m" // Cyan
#define FOREWHT "\x1B[37m" // White
#define PBSTR "##################################################" // Progress bar filler
#define PBWIDTH 35 // Progress bar width

// SERVER IPs (ignore)
// Phoneix0: 10.35.195.251
// Phoneix1: 10.34.40.33
// Phoneix2: 10.35.195.250
// Phoneix3: 10.34.195.236

using namespace std;

int client(bool debug, bool validate, int pMode); // Client
int server(bool debug, bool validate, int pMode); // Server
int md5(char * fileName); // MD5
int fsize(FILE * fp); // File Size

int main(int argc, char * argv[]) { // Main function, parses arguments to determine server/client
    int responce = 0, mode = 0;
    bool debug = false, isServer = false, isClient = false, validate = false, GBN = false, SR = false;
    system("clear");
    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-s") {
            isServer = true;
        } else if (arg == "-c") {
            isClient = true;
        } else if (arg == "-d") {
            debug = true;
        } else if (arg == "-v") {
            validate = true;
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
			if (validate) {
				cout << "[VALIDATE]";
			}
			if (GBN) {
				cout << "[GBN]";
			}
			if (SR) {
				cout << "[SR]";
			}
			cout << "\n";
			responce = server(debug, validate, mode);
		} else if (isClient) {
			cout << FOREGRN << "[CLIENT]";
			if (debug) {
				cout << "[DEBUG]";
			}
			if (validate) {
				cout << "[VALIDATE]";
			}
			if (GBN) {
				cout << "[GBN]";
			}
			if (SR) {
				cout << "[SR]";
			}
			cout << "\n";
			responce = client(debug, validate, mode);
		} else {
			cout << FORERED << "Invalid parameter(s) entered, please specify if you are running a server ('-s') or a client ('-c') and use ('-d') to run in debug mode\n" << RESETTEXT;
		}
	}
    return 0;
}
int client(bool debug, bool validate, int pMode) { // Client function, send file to server
    int sfd = 0, n = 0, b, port, packetSize, packetNum = 0, count = 0, totalSent = 0; // Declare integers
    char rbuff[1024], ip[32], fileName[64]; // Declare char arrays
    struct sockaddr_in serv_addr; // Declare socket
    memset(rbuff, '0', sizeof(rbuff)); // memset recieving buffer
    sfd = socket(AF_INET, SOCK_STREAM, 0); // Initialize socket
    cout << FOREWHT << "IP Address: "; // Get user input
    scanf("%15s", ip);
    cout << "Port: ";
    cin >> port;
    cout << "File Name: ";
    scanf("%64s", fileName);
    cout << "Packet Size (kb): ";
    cin >> packetSize;
    packetSize = packetSize * 1000;
    char sendbuffer[packetSize];
    string key = "";
    cout << "Encryption Key: ";
    cin >> key;
    char encryptionKey[key.size()];
    strcpy(encryptionKey, key.c_str());
    serv_addr.sin_family = AF_INET; // Set family, port, and ip for socket
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    cout << FOREYEL << "Attempting to connect to " << ip << ":" << port << "\n" << RESETTEXT;
    if ((connect(sfd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)) == -1)) { // Connect to socket & error handle
        cout << FORERED << "CONNECTION FAILED...\n" << RESETTEXT;
        perror("Connect");
        return 1;
    }
    cout << FOREGRN << "Connected to server!\n" << RESETTEXT;
    FILE * fp = fopen(fileName, "rb"); // Open file for sending
    if (fp == NULL) {
        cout << FORERED << "ERROR OPENING FILE...\n" << RESETTEXT;
        perror("File");
        return 2;
    }
    int fileSize = fsize(fp);
    uint32_t tmp = htonl(packetSize); // Send packet size to server
    write(sfd, & tmp, sizeof(tmp));
    uint32_t tmp2 = htonl(fileSize); // Send file size to server
    write(sfd, & tmp2, sizeof(tmp2));

    char packetCode[8];
    int mode = 1;
    bool run = true;
    bool validated = false;
    int validateMode = 1;
    while ((totalSent != fileSize) && run) {
        switch (mode) {
        case 1:
            if (((b = fread(sendbuffer, 1, packetSize / 2, fp)) > 0)) {
                totalSent += b;
                char rawBuffer[b], encryptedBuff[packetSize], first4[4], last4[4];
                for (int i = 0; i < b; i++) { // Encrypt packet with xor, then hex
                    int keyVal = (i % sizeof(encryptionKey));
                    sprintf( & encryptedBuff[i * 2], "%02x", (((unsigned char) sendbuffer[i]) ^ (encryptionKey[keyVal])));
                    if (i < 8) {
                        packetCode[i] = encryptedBuff[i];
                    }
                }

                if (packetNum < 10 || debug == true) { // Print output, if debug is enabled all packets will be printed, otherwise only print first 10 packets
                    cout << "Sent Encrypted Packet #" << (packetNum + 1) << " - [";
                    memcpy(first4, encryptedBuff, 5);
                    first4[4] = 0;
                    cout << FORECYN << first4;
                    cout << "...";
                    memcpy(last4, encryptedBuff + ((b * 2) - 4), 5);
                    last4[4] = 0;
                    cout << last4 << FOREWHT;
                    cout << "] ";
                    if (debug) {
                        cout << "(" << (b * 2) << " bytes (" << totalSent << "/" << fileSize << "))\n";
                    } else {
                        cout << "\n";
                    }
                    if (packetNum == 9 && debug == false) {
                        cout << "\nSending Remaining Packets...\n" << FORECYN;
                    }
                } else if (!validate) { // Print total progress bar after 10 packets if not in debug
                    double percentage = ((double) totalSent / (double) fileSize);
                    int val = (int)(percentage * 100);
                    int lpad = (int)(percentage * PBWIDTH);
                    int rpad = PBWIDTH - lpad;
                    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
                    fflush(stdout);
                }
                packetNum++; // Increase packet counter
                send(sfd, encryptedBuff, b * 2, 0); // Send Packet
                mode = 2;
                validated = false;
            }
            break;
        case 2:
            if (validate) {
                int sendCode = 1;
                int s = 0;
                while (!validated) {
                    switch (validateMode) {
                    case 1: // recieve packet code and validate
                        char recieve[8];
                        cout << FOREYEL << "Validating Incomming Packet Code...\n" << RESETTEXT;
                        while (s == 0) { // Loop until packet code is recieved
                            if (s = recv(sfd, recieve, 8, 0) > 0) {
                                for (int i = 0; i < 8; i++) { // Validate packet code
                                    if (packetCode[i] != recieve[i]) {
                                        sendCode = 0;
                                    }
                                }
                                if (sendCode == 1) {
                                    validateMode = 2;
                                }
                            }
                        }
                        break;
                    case 2: // send responce code
                        if (sendCode == 1) {
                            cout << FOREGRN << "Packet Code Validated!\n" << RESETTEXT;
                            uint32_t un = htonl(sendCode);
                            send(sfd, & un, sizeof(uint32_t), 0);
                            mode = 1;
                            validateMode = 1;
                            validated = true;
                        } else if (sendCode == 0) {
                            cout << FORERED << "Packet Code Invalid! Stopping File Transfer...\nExpected: ";
                            for (int i = 0; i < 8; i++) { // Print expected packet code
                                cout << packetCode[i];
                            }
                            cout << "\nRecieved: ";
                            for (int i = 0; i < 8; i++) { // Print recieved packet code
                                cout << recieve[i];
                                if (packetCode[i] != recieve[i]) {
                                    sendCode = 0;
                                }
                            }
                            cout << "\n" << RESETTEXT;

                            uint32_t un = htonl(sendCode);
                            send(sfd, & un, sizeof(uint32_t), 0); // Send responce code
                            run = false;
                        }
                        break;
                    }
                }
            } else {
                mode = 1;
            }
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
int server(bool debug, bool validate, int pMode) { // Server function, connects to a client, recieves and decrypts packets of file contents, and writes result to file
    int fd = 0, confd = 0, b, num, port, packetSize, packetNum = 0, count = 0, totalRecieved = 0; // Declare integers
    struct sockaddr_in serv_addr; // Declare socket struct
    char fileName[64], initialBuff[32], ip[32]; // Declare char arrays
    cout << FOREWHT << "IP Address: "; // Get user input
    scanf("%15s", ip);
    cout << "Port: ";
    cin >> port;
    cout << "Output File Name: ";
    scanf("%64s", fileName);
    string key = "";
    cout << "Encryption Key: ";
    cin >> key;
    char encryptionKey[key.size()];
    strcpy(encryptionKey, key.c_str());
    fd = socket(AF_INET, SOCK_STREAM, 0); // Initialize socket
    memset( & serv_addr, '0', sizeof(serv_addr)); // memset server address
    memset(initialBuff, '0', sizeof(initialBuff)); // memset initial buff (this is used because we dont know the packet size yet)
    serv_addr.sin_family = AF_INET; // Set family, port, and ip address for socket	
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);
    bind(fd, (struct sockaddr * ) & serv_addr, sizeof(serv_addr)); // Bind socket struct to socket and begin listening
    listen(fd, 10);
    cout << FOREYEL << "Socket started on " << ip << ":" << port << "\n";
    cout << "Waiting for client connection...\n" << RESETTEXT;
    while (1) {
        confd = accept(fd, (struct sockaddr * ) NULL, NULL); // Accept connection and error handle
        if (confd == -1) {
            perror("Accept");
            continue;
        }
        cout << FOREGRN << "Connected to client!\n" << FOREWHT;
        uint32_t tmp; // Get Packet Size
        read(confd, & tmp, sizeof(tmp));
        packetSize = ntohl(tmp);
        char recieveBuffer[packetSize]; // Declare new recieving buff[] to be length of packet size and memset
        memset(recieveBuffer, '0', sizeof(recieveBuffer));
        uint32_t tmp2; // Get File Size
        read(confd, & tmp2, sizeof(tmp2));
        int fileSize = ntohl(tmp2);
        FILE * fp = fopen(fileName, "wb"); // Open new file for writing
        if (fp != NULL) {
            char packetCode[8];
            int mode = 1;
            bool run = true;
            bool validated = false;
            int validateMode = 1;
            while ((totalRecieved != fileSize) && run) {
                switch (mode) {
                case 1:
                    if (((b = recv(confd, recieveBuffer, packetSize, MSG_WAITALL)) > 0)) {
                        totalRecieved += (b / 2);
                        char first4[4], last4[4], fromHex[b * 2], rawBuffer[b], decrypted[b];
                        for (int i = 0; i < b; i++) { // Populate rawBuffer with incoming data & add deliminator
                            rawBuffer[i] = recieveBuffer[i];
                            if (i < 8) {
                                packetCode[i] = rawBuffer[i];
                            }
                        }
                        rawBuffer[sizeof(rawBuffer)] = 0;

                        if (packetNum < 10 || debug == true) { // Print output, if debug is enabled all packets will be printed, otherwise only print first 10 packets
                            cout << "Recieved Packet #" << (packetNum + 1) << " - Encrypted As [";
                            memcpy(first4, rawBuffer, 5);
                            first4[4] = 0;
                            cout << FORECYN << first4;
                            cout << "...";
                            memcpy(last4, rawBuffer + (b - 4), 5);
                            last4[4] = 0;
                            cout << last4 << FOREWHT;
                            cout << "] ";
                            if (debug) {
                                cout << "(" << b << " bytes (" << totalRecieved << "/" << fileSize << "))\n";
                            } else {
                                cout << "\n";
                            }
                            if (packetNum == 9 && debug == false) {
                                cout << "\nRecieving Remaining Packets...\n" << FORECYN;
                            }
                        } else if (!validate) { // Print total progress bar after 10 packets if not in debug
                            double percentage = ((double) totalRecieved / (double) fileSize);
                            int val = (int)(percentage * 100);
                            int lpad = (int)(percentage * PBWIDTH);
                            int rpad = PBWIDTH - lpad;
                            printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
                            fflush(stdout);
                        }
                        int len = strlen(rawBuffer);
                        for (int i = 0, j = 0; j < len; ++i, j += 2) { // Convert hex into decryptable data, stored in fromHex[]
                            int val[1];
                            sscanf(rawBuffer + j, "%02x", val);
                            fromHex[i] = val[0];
                            fromHex[i + 1] = '\0';
                        }
                        for (int i = 0; i < b / 2; ++i) { // Apply XOR on fromHex[] to get decrypted packet
                            int keyVal = (i % sizeof(encryptionKey));
                            sprintf( & decrypted[i], "%c", (fromHex[i]) ^ (encryptionKey[keyVal]));
                        }
                        packetNum++; // Increase packet count
                        fwrite(decrypted, 1, b / 2, fp); // Write packet to file
                        mode = 2;
                        validated = false;
                    }
                    break;
                case 2:
                    if (validate) { // Send packet code to client for validation
                        while (!validated) {
                            switch (validateMode) {
                            case 1: // send packet code to client
                                cout << FOREYEL << "Validating packet code [";
                                for (int i = 0; i < 8; i++) {
                                    cout << packetCode[i];
                                }
                                cout << "]\n" << RESETTEXT;

                                send(confd, packetCode, sizeof(packetCode), 0);
                                validateMode = 2;
                                break;
                            case 2: // recieve responce code from client
                                int s = 0;
                                while (s == 0) {
                                    uint32_t recieve;
                                    if (s = recv(confd, & recieve, sizeof(uint32_t), 0) > 0) {
                                        int responceCode = ntohl(recieve);
                                        if (responceCode == 1) {
                                            cout << FOREGRN << "Packet Validated\n" << RESETTEXT;
                                            mode = 1;
                                            validated = true;
                                            validateMode = 1;
                                        } else if (responceCode == 0) {
                                            cout << FORERED << "Responce Code: " << responceCode << "\n";
                                            cout << "Packet Invalid! Stopping file transfer...\n" << RESETTEXT;
                                            run = false;
                                        }
                                    }
                                }
                                break;
                            }
                        }

                    } else {
                        mode = 1;
                    }
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