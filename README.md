# CS 462 - Sliding Window Protocols Team Project

<img src="https://img.shields.io/badge/platform-linux-success.svg"> <img src="https://img.shields.io/badge/version-0.6.1-green">

## Project Demo:
- Presentations Start April 12th
- 1 Hour Demos
- First Come First Serve
- Paramaters
  - Packet Size: 8k - 64k
  - Timeout: 5ms - 5s
  - Window: 16 - 512 
  - Situational Errors: Custom ACK drops, Custom Packet Drops, Custom Damaged Packets, maybe random errors
	
## Testing:
- ## Stop and Wait:
  - No Errors, 64k Packet Size, 100ms timeout, 1Gtestfile
    - Transfered Successfully
    - ~1-2 minute transfer time
  - 5% Random Errors (both), 64k Packet Size, 100ms timeout, 1Gtestfile
    - Transfered Successfully
    - ~5-7 minute transfer time
- ## Go-Back-N
  - No Errors, 64k Packet Size, 100ms timeout, 1Gtestfile, window size of 5
    - Transfered Successfully
    - ~2-3 minute transfer time
  - No Errors, 64k Packet Size, 1000ms timeout, 1Gtestfile, window size of 512
    - Transfered Successfully
    - ~1-2 minute transfer time
  - 5% Random Errors (both), 64k Packet Size, 100ms timeout, 1Gtestfile, window size of 5
    - Transfered Successfully
    - ~5-7 minute transfer time
  - Custom Erros (~5 ACK drops, ~5 Packet Drops, ~5 Damaged Packets), 64k Packet Size, 100ms timeout, 1Gtestfile, window size of 5
    - Transfered Successfully
    - ~1-3 minute transfer time

## Testing:
	Stop and Wait:
		1. ~1.2mb test file
			a. No Errors, 64000 byte packet size, 1000 ms timeout
				- Success, 1095 ms transfer time
			b. 20% Random Errors, 64000 byte packet size, 1000 ms timeout
				- Success, 8075 ms transfer time
			c. 3 Custom Errors (each), 64000 byte packet size, 1000 ms timeout
				- Success, 7072 ms transfer time
		2. 1G test file
			a. No Errors, 64000 byte packet size, 100 ms timeout
				- Success, 46370 ms transfer time
			b. 5% Random Errors, 64000 byte packet size, 100 ms timeout
				- Success, 303244 ms transfer time
			c. 15 Custom Errors (each), 64000 byte packet size, 100 ms timeout
				- Success, 49344 ms transfer time
	- ## Go-Back-N:
		- 1. ~1.2mb test file
			- a. No Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 2901 ms transfer time
			- b. 5% Random Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 3220 ms transfer time
			- c. 3 Custom Errors (each), 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 3209 ms transfer time
		- 2. 1G test file
			- a. No Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 24532 ms transfer time
			- b. 1% Random Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 80564 ms transfer time
			- c. 5-10 Custom Errors (each), 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 38547 ms transfer time
			- d. No Errors, 64000 byte packet size, 1000 ms timeout, window size of 256
				- Success, 26030 ms transfer time
			- e. 1% Random Errors, 64000 byte packet size, 1000 ms timeout, window size of 256
				- Random ACKS are received after a while, transfer never finishes
			- f. 3 Custom Errors (each), 64000 byte packet size, 1000 ms timeout, window size of 256
				- Success, 31015 ms transfer time
	- ## Selective Repeat:
		- 1. ~1.2mb test file
			- a. No Errors, 64000 byte packet size, 1000 ms timeout, window size of 5
				- Success, 10217 ms transfer time
			- b. 5% Random Errors, 64000 byte packet size, 500 ms timeout, window size of 5
				- Success, 16609 ms transfer time
			- c. 3 Custom Errors (each), 64000 byte packet size, 1000 ms timeout, window size of 5
				- Success, 11454 ms transfer time
		- 2. 1G test file
			- a. No Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 33360 ms transfer time
			- b. 1% Random Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 179558 ms transfer time
			- c. 5-10 Custom Errors (each), 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 75139 ms transfer time
			- d. No Errors, 64000 byte packet size, 500 ms timeout, window size of 256
				- Success, transfer finishes for server with correct md5sum but client just stops
			- e. 1% Random Errors, 64000 byte packet size, 500 ms timeout, window size of 256
				- Process gets 'Killed' after a while, transfer never finishes
			- f. 3 Custom Errors (each), 64000 byte packet size, 1000 ms timeout, window size of 512
				- Success, transfer finishes for server with correct md5sum but client just stops
