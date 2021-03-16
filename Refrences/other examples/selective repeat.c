#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose
   Unidirectional data transfer protocols (from A to B).
   Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********************      SELECTIVE REPEAT PROTOCOL     *************************/

/* called from layer 5, passed the data to be sent to other side */


#include <stdio.h>
#include <stdlib.h>
#include<string.h>


struct pkt send_pkt[1010];
struct pkt rcv_pkt[1010];
int pkt_no;
int nxt_seq_no;
static int s_base;
int r_base;
float diff;
int buffer_msg;
int buffer_send;
int seq_no;
int flag;
int N;
float RTT;

struct pkt_timer
{
	int seqnum;
	int acknum;
	float start_time;
	float rtt;
};
struct pkt_timer timeout[1000];

int checksum_call(int seq_no, int ack_no, char* payload)
{
    int sum=0;
    sum=seq_no+ack_no;
    int i;
    for(i=0;i<20;i++) {
        sum+=payload[i];
    }
    //printf("\nSum = %d\n",sum);
    return sum;
}


void A_output(message)
  struct msg message;
{
	printf("In A_output\n");
	if(nxt_seq_no<s_base+N)
	{
		int i;
		for ( i=0; i<20 ; i++ )
			send_pkt[pkt_no].payload[i] = message.data[i];
		send_pkt[pkt_no].seqnum=seq_no;
		send_pkt[pkt_no].acknum=0;
		send_pkt[pkt_no].checksum=checksum_call(send_pkt[pkt_no].seqnum,send_pkt[pkt_no].acknum,send_pkt[pkt_no].payload);
		tolayer3(0, send_pkt[pkt_no]);
		timeout[pkt_no].seqnum=seq_no;
		timeout[pkt_no].acknum=0;
		timeout[pkt_no].start_time = get_sim_time();
		timeout[pkt_no].rtt=get_sim_time()+RTT;
		printf("rtt time %f", timeout[pkt_no].rtt);
		if(s_base==nxt_seq_no)
		{
			starttimer(0, RTT);
		}
		seq_no++;
		nxt_seq_no++;
		pkt_no++;
		return;
	}
	else
	{
		int i;
		for ( i=0; i<20 ; i++ )
			send_pkt[pkt_no].payload[i] = message.data[i];
		send_pkt[pkt_no].seqnum=seq_no;
		send_pkt[pkt_no].acknum=0;
		send_pkt[pkt_no].checksum=checksum_call(send_pkt[pkt_no].seqnum,send_pkt[pkt_no].acknum,send_pkt[pkt_no].payload);
		buffer_msg++;
		seq_no++;
		pkt_no++;
		return;
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	printf("In A_input\n");
	int ack_checksum=0;
	ack_checksum=checksum_call(packet.seqnum,packet.acknum,packet.payload);
	if(packet.acknum==-1)
	{
		return;
	}
	if(ack_checksum==packet.checksum)
	{
		send_pkt[packet.acknum].acknum=1;
		timeout[packet.acknum].acknum=1;
		if(s_base==packet.acknum)
		{
			int i;
			i=packet.acknum;
			while(send_pkt[i].acknum==1)
			{
				printf("incrementing s_base");
				i++;
				s_base++;
			}
		}

		if(s_base==nxt_seq_no)
		{
			stoptimer(0);
		}
		buffer_send=nxt_seq_no;
		while(buffer_msg!=0 && nxt_seq_no<s_base+N)
		{

			tolayer3(0, send_pkt[buffer_send]);
			timeout[buffer_send].seqnum=seq_no;
			timeout[buffer_send].acknum=0;
			timeout[pkt_no].start_time = get_sim_time();
			timeout[buffer_send].rtt=get_sim_time()+RTT;
			if(s_base==nxt_seq_no)
			{
				starttimer(0, RTT);
			}
			nxt_seq_no++;
			buffer_send++;
			buffer_msg--;
		}

	}
	else if(ack_checksum!=packet.checksum)
	{
		printf("Corrupted ack\n");
	}
	return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("In A_timerinterrupt\n");
	int num,i;
	num=nxt_seq_no-s_base;
	int pkt=s_base;
	float currenttime;
	currenttime=get_sim_time();
	starttimer(0,diff);
	printf("%f vsvsdnvoidsvdsovndsvsdonvbsdvsoivni", diff);

	for(i=0;i<num;i++)
	{
		if(send_pkt[pkt].acknum==0 && (currenttime > timeout[pkt].rtt))
		{
			tolayer3(0, send_pkt[pkt]);
			timeout[pkt].seqnum=pkt;
			timeout[pkt].acknum=0;
			timeout[pkt_no].start_time = get_sim_time();
			timeout[pkt].rtt=get_sim_time()+RTT;
                        break;
		}
		pkt++;
	}
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	N=getwinsize();
	RTT = 32;
	pkt_no = 0;
	nxt_seq_no = 0;
	s_base = 0;
	diff = 2.0;
	buffer_msg = 0;
	seq_no = 0;
	buffer_send =0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/

void B_input(packet)
  struct pkt packet;
{
	printf("In B_input \n");
	int pkt_checksum=0;
	pkt_checksum=checksum_call(packet.seqnum,packet.acknum,packet.payload);
	int seq=packet.seqnum;
	if(pkt_checksum==packet.checksum && r_base==packet.seqnum)
	{
		rcv_pkt[r_base]=packet;
		int i;
		for ( i=0; i<20 ; i++ )
		{
			rcv_pkt[packet.seqnum].payload[i] = packet.payload[i];
		}
	       	if(flag)
		{
			tolayer5(1, rcv_pkt[r_base].payload);
			r_base++;
			flag=0;
			while(seq=rcv_pkt[r_base].seqnum)
			{
				tolayer5(1,rcv_pkt[r_base].payload);
				r_base++;
			}
		}
		while(seq>0)
                {
			printf("delivering data to upper layer\n");
			tolayer5(1, rcv_pkt[r_base].payload);
                        r_base++;
			seq=rcv_pkt[r_base].seqnum;
                }
		struct pkt ackpkt=packet;
		ackpkt.acknum=packet.seqnum;
		ackpkt.checksum=checksum_call(ackpkt.seqnum, ackpkt.acknum, ackpkt.payload);
		tolayer3(1, ackpkt);
	}
	else if(pkt_checksum==packet.checksum && (packet.seqnum<r_base+N))
	{
		rcv_pkt[packet.seqnum]=packet;
		  int i;
		  for ( i=0; i<20 ; i++ )
		  {
		  rcv_pkt[packet.seqnum].payload[i] = packet.payload[i];
		  }
		struct pkt ackpkt=packet;
		ackpkt.acknum=packet.seqnum;
		ackpkt.checksum=checksum_call(ackpkt.seqnum, ackpkt.acknum, ackpkt.payload);
		tolayer3(1, ackpkt);
	}
	else
	{
		printf("Corrupted packet or undesired packet\n");

	}
	return;
}


void B_init()
{
r_base=0;
flag=1;
}
