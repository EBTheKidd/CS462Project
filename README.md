# CS 462 - Sliding Window Protocols Team Project

<img src="https://img.shields.io/badge/platform-linux-success.svg"> <img src="https://img.shields.io/badge/version-0.6.1-green">

Demo:
	1. Presentations Start April 12th
	2. 1 Hour Demos
	3. First Come First Serve
	4. Paramaters
		a. Packet Size: 8k-64k
		b. Timeout: 5ms-5s
		c. Window: 16 - 256/512          <--- Apparently this is different between client and server
		d. Errors: specific Packet numbers
	
	
	
Testing:
	Stop and Wait:
		1: No Errors, 64k Packet Size, 100ms timeout, 1Gtestfile
			- Transfered Successfully
			- ~1-2 minute transfer time
		2: 5% Random Errors (both), 64k Packet Size, 100ms timeout, 1Gtestfile
			- Transfered Successfully
			- ~5-7 minute transfer time
	Go-Back-N:
		1: No Errors, 64k Packet Size, 100ms timeout, 1Gtestfile, window size of 5
			- Transfered Successfully
			- ~2-3 minute transfer time
		4: 5% Random Errors (both), 64k Packet Size, 100ms timeout, 1Gtestfile, window size of 5
			- Transfered Successfully
			- ~5-7 minute transfer time
		5: Custom Erros (~5 ACK drops, ~5 Packet Drops, ~5 Damaged Packets), 64k Packet Size, 100ms timeout, 1Gtestfile, window size of 5
			- Transfered Successfully
			- ~1-3 minute transfer time


## TODO
- [x] CRC Checksum
- [ ] Required Protocols
    - [x] Stop And Wait
    - [x] Go-Back-N
    - [ ] Selective Repeat
- [ ] Situational Errors
    - [ ] Randomly Generated
        - [x] Stop And Wait
        - [x] Go-Back-N
        - [ ] Selective Repeat
    - [ ] User-Specified
        - [ ] Drop Packets (ex: 2,4,5)
            - [x] Stop And Wait
            - [x] Go-Back-N
            - [ ] Selective Repeat
        - [ ] Damage Packets (ex: 2,4,5)
            - [x] Stop And Wait
            - [x] Go-Back-N
            - [ ] Selective Repeat
        - [ ] Loose Acks (ex: 11)  
            - [x] Stop And Wait
            - [x] Go-Back-N
            - [ ] Selective Repeat
- [x] Output
    - [x] Original Packets Sent
    - [x] Re-Transmitted Packets Sent
    - [x] Total ellapsed Time (ms)
    - [x] Total Throughput (mbps)
    - [x] Effective Throughput
    - [x] Packets in Current Window
    - [x] Acks Sent/Recieved
    - [x] Damaged Packets (bad crc result / failed ack)
    - [x] Last Packet Seq# Recieved 
