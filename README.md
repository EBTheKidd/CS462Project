# CS 462 - Sliding Window Protocols Team Project

<img src="https://img.shields.io/badge/platform-linux-success.svg"> <img src="https://img.shields.io/badge/version-0.6.1-green">

## Prototype Screenshot
![](demo.PNG)

## TODO
- [x] CRC Checksum
- [ ] Required Protocols
    - [x] Stop And Wait
    - [ ] Go-Back-N
    - [ ] Selective Repeat
- [ ] Situational Errors
    - [ ] Randomly Generated
    - [ ] User-Specified
        - [ ] Drop Packets (ex: 2,4,5)
        - [ ] Loose Acks (ex: 11)
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
