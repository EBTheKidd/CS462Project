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

int prevseqnum = 1;
int a = 0;

int seqnum, acknum, ackrecv;
int i=0;
int j=0;
struct pkt packet1;

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
struct msg message;
{
	int checksum = 0;
	memcpy(buffer+i, message.data, 20);
	//printf("Message received from application layer: %s\n",message.data);
	i++;

		if (a == 0){
		memcpy(packet1.payload, buffer+j, 20);
		//printf("Message sent to B: %s\n", packet1.payload);
		if(seqnum == 0)
		{
			seqnum = 1;
		}
		else
		{
			seqnum = 0;
		}
		if(acknum == 0)
		{
			acknum = 1;
		}
		else
		{
			acknum = 0;
		}
		packet1.seqnum = seqnum;
		packet1.acknum = acknum;
		for(int k=0; k<20; k++){
			checksum += packet1.payload[k];
		}
		checksum += seqnum;
		checksum += acknum;

		packet1.checksum = checksum;
		j++;
		tolayer3(A, packet1);
		starttimer (A, 25);
		a = 1;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
struct pkt packet;
{
	int checksum = 0;
	for(int k=0; k<20; k++){
		checksum += packet.payload[k];
	}
	checksum += packet.seqnum;
	checksum += packet.acknum;
	if(packet.checksum == checksum){
			a = 0;
			stoptimer(A);
			//printf("Ack_Rcv %s\n",packet.payload);
			}

}


/* called when A's timer goes off */
void A_timerinterrupt()
{
	//printf("Timer interrupted for packet:%s\n", packet1.payload);
	tolayer3(A, packet1);
	starttimer(A,25);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	seqnum=1;
	acknum =1;
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
		if(packet.seqnum != prevseqnum){
			tolayer5(B, packet.payload);
			//printf("Message sent to application layer\n");
			prevseqnum = packet.seqnum;
		}

		//printf("Packet received: %s\n", packet.payload);
		tolayer3(B, packet);
		//printf("Acknowledgment sent to A\n");
	}
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{

}

