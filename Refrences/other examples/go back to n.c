#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
 **********************************************************************/

#include <stdio.h>
#include <string.h>


#define A 0
#define B 1


char buffer[2000][20];

int seqnum, acknum, ackrecv, base, nextseqnum, expectedseqnum;
int window_size;
int i=0;
int j=0;
struct pkt packet1,prev_packet;


/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
struct msg message;
{

	int checksum = 0;
	memcpy(buffer+i, message.data, 20);
	i++;
	//printf("Message received from layer 5 of A: %s\n",message.data);
	if(nextseqnum<base+window_size)
	{
		memcpy(packet1.payload, buffer+j, 20);
		j++;
		packet1.seqnum = nextseqnum;
		packet1.acknum = acknum;
		for(int k=0; k<20; k++)
		{
			checksum += packet1.payload[k];
		}
		checksum += seqnum;
		checksum += acknum;
		packet1.checksum = checksum;
		tolayer3(A, packet1);
		//printf("Message sent to B: %s\n", packet1.payload);
		if(base == nextseqnum)
		{
		starttimer(A,20);
		}
		nextseqnum++;
		acknum++;
		seqnum++;
	}

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
struct pkt packet;
{
	int checksum = 0;
	for(int k=0; k<20; k++)
			{
				checksum += packet.payload[k];
			}
	checksum+= packet.seqnum;
	checksum+= packet.acknum;
	if(packet.checksum == checksum)
	{
		base = packet.acknum+1;
		//printf("Ack_Rcv for packet:%s\n",packet.payload);
		if(base == nextseqnum){
			stoptimer(A);}
		else{
			stoptimer(A);
			starttimer(A,20);
			}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{

	int i;
	int checksum = 0;
	struct pkt packet;
	starttimer(A,20);
	for(i= base;i<nextseqnum; i++)
	{
		memcpy(packet.payload, buffer+i,20);
		packet.seqnum =i;
		packet.acknum =i;
		for(int k=0; k<20; k++)
				{
					checksum += packet.payload[k];
				}
		checksum += i;
		checksum += i;
		packet.checksum = checksum;
		tolayer3(A, packet);
		//printf("Packet sent again to B is: %s\n", packet.payload);
	}

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	seqnum = 0;
	acknum = 0;
	//ackrecv= 1;
	base = 0;
	nextseqnum = 0;
	window_size = getwinsize();
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
struct pkt packet;
{
	int checksum = 0;
	for(int k=0; k<20; k++){
		checksum += packet.payload[k];
	}
	checksum += packet.seqnum;
	checksum += packet.acknum;
	if(packet.checksum == checksum){
		if(packet.seqnum == expectedseqnum){
			tolayer5(B, packet.payload);
			//printf("Packet sent to application layer of B is: %s\n", packet.payload);
			prev_packet = packet;
			tolayer3(B, packet);
			//printf("Acknowledgment sent to A for packet: %s\n", packet.payload);
			expectedseqnum++;
		}
	}
	else if (packet.seqnum != 0) {
	tolayer3(B, prev_packet);
	//printf("Acknowledgment sent to A for prev_packet: %s\n", prev_packet.payload);
	}
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	expectedseqnum = 0;
	//prevseqnum = 1;
	//seqnum, acknum= 0;
}
