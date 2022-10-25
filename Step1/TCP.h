#ifndef TCP
    #define TCP

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <random>
#include <chrono>


#define RTT 15
#define MSS 1024
#define Threshold 65536
#define RcvBuffer 524288

#define PoissonMean 0

#define SackBlockMax 3

#define RED "\033[0;32;31m"
#define YELLOW "\033[1;33m"
#define GREEN "\033[0;32;32m"
#define NONE "\033[m"

typedef void * (*ThreadFuncPtr)(void *);
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct timeval timeval;

typedef struct tcp_header{
 // int src_portno;
// int dist_portno;
    int seqno;
    int ackno;
    int checksum;
    char flag;
    /* 0x1 = Fin, 0x2 = SYN, 0x4 = RST, 0x8 = PSH, 
        0x10 = ACK, 0x20 = URG, 0x40 = EOF*/ 
    int rwnd;
    
    int data_length; /*testing */
    /* record start position, zero means not been use (not safe, -1 will be safer ) */
    int sack[SackBlockMax];

}tcp_header;

typedef struct tcp_segment{
    tcp_header header;
    //void *data;
    char data[MSS];

}tcp_segment;

class TCP_Unit
{
    private:
        sockaddr_in itself_info;  /* normally for server to bind */
        sockaddr_in other_info;   /* normally for client to connect or for server to accept */
        int sockfd;     /* socket file descripter */
        char buf[RcvBuffer]; /* buffer, 512k bytes */
        int bufptr;
        int rwnd;
        float cwnd;
        int ssthreshold;
        int seq_number;
        int ack_number;
        int biggest_ack_number;
        int dupACKcount;
        //int sack_start[SackBlockMax];
        //int other_sack_start[SackBlockMax];
    public:
        /* for no bind */
        TCP_Unit();
        /* for bind */
        TCP_Unit(char * ip, int portno);
        TCP_Unit(TCP_Unit *unit);
        /* every unit can send and receive */
        int send_data(void *, int, int );
        int receive_data();

        int congestion_ctrl();
        tcp_segment* packup(void* , int, int);
        void unpack(tcp_segment *);
        /* poisson distribution, mean = 10^(-6) */
        bool generate_loss();
        unsigned int do_checksum(tcp_segment *);
        void fast_retransmit_recovery(FILE *);
        void sack_handle( int seqno );
        void sack_info_record(int seqno);
        void sack_send(FILE *fptr);

        /* just for get or set private member */
        void set_itself_info(char *ip, int portno);
        void set_other_info(char *ip, int portno);
        void set_other_info(sockaddr_in info){ other_info = info;}
        sockaddr_in get_other_info(){return other_info;}
        sockaddr_in get_itself_info(){return itself_info;}
        int get_sockfd(){return sockfd;}
        void set_sockfd(int fd){sockfd = fd;}
        char* get_buf(){return buf;}
        void dress_up();

        /* just for printing out the status */
        void buf2file(const char *filename);
        void print_stutas(char);
        void print_send_msg(int, int);
        void print_receive_msg(int , tcp_header );
    
        void nonblocking(int );
        
        /* output filename */
        char filename[50];
};

#endif








