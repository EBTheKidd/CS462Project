# CS 462 - Sliding Window Protocols Team Project By Ethan and Kong

<img src="https://img.shields.io/badge/platform-linux-success.svg"> <img src="https://img.shields.io/badge/version-0.6.1-green">

Linux sliding window protocol simulation program developed in c++
Featured Protocols: Stop and Wait, Go-Back-N, and Selective Repeat

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
	Go-Back-N:
		1. ~1.2mb test file
			a. No Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 2901 ms transfer time
			b. 5% Random Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 3220 ms transfer time
			c. 3 Custom Errors (each), 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 3209 ms transfer time
		2. 1G test file
			a. No Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 24532 ms transfer time
			b. 1% Random Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 80564 ms transfer time
			c. 5-10 Custom Errors (each), 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 38547 ms transfer time
			d. No Errors, 64000 byte packet size, 1000 ms timeout, window size of 256
				- Success, 26030 ms transfer time
			e. 1% Random Errors, 64000 byte packet size, 1000 ms timeout, window size of 256
				- Random ACKS are received after a while, transfer never finishes
			f. 3 Custom Errors (each), 64000 byte packet size, 1000 ms timeout, window size of 256
				- Success, 31015 ms transfer time
	Selective Repeat:
		1. ~1.2mb test file
			a. No Errors, 64000 byte packet size, 1000 ms timeout, window size of 5
				- Success, 10217 ms transfer time
			b. 5% Random Errors, 64000 byte packet size, 500 ms timeout, window size of 5
				- Success, 16609 ms transfer time
			c. 3 Custom Errors (each), 64000 byte packet size, 1000 ms timeout, window size of 5
				- Success, 11454 ms transfer time
		2. 1G test file
			a. No Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 33360 ms transfer time
			b. 1% Random Errors, 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 179558 ms transfer time
			c. 5-10 Custom Errors (each), 64000 byte packet size, 100 ms timeout, window size of 5
				- Success, 75139 ms transfer time
			d. No Errors, 64000 byte packet size, 500 ms timeout, window size of 256
				- Success, transfer finishes for server with correct md5sum but client just stops
			e. 1% Random Errors, 64000 byte packet size, 500 ms timeout, window size of 256
				- Process gets 'Killed' after a while, transfer never finishes
			f. 3 Custom Errors (each), 64000 byte packet size, 1000 ms timeout, window size of 512
				- Success, transfer finishes for server with correct md5sum but client just stops

## Known Bugs (will probably not be fixed)
  - Window Sequence Display is not correct sometimes, this is likely caused by a missing modulo in the display function
