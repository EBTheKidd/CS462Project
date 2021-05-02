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
