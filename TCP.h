#ifndef TCP
    #define TCP

#include <memory.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

#include <time.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

#define RTT 15
#define MSS 1024
#define THRES 65536
#define MAX_BUF 524288

#define POIS_MEAN 0


#define YELLOW "\033[1;33m"
#define BLUE "\033[1;32;34m"
#define PURPLE "\033[1;32;35m"
#define PURPLE_GND "\033[0;32;45m"
#define BLUE_GND "\033[1;32;44m"


#define ORIGN "\033[m"
#define RED "\033[1;32;31m"
#define GREEN "\033[1;32;32m"

typedef struct timeval timeval;
typedef void * (*ThreadFuncPtr)(void *);
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct tcp_hdr{
    int seq_num;
    int ack_num;
    int checksum;
    char flag;
    // 0x1 = Fin, 0x2 = SYN, 0x10 = ACK
    int rwnd;
    
    int size;

}tcp_hdr;

typedef struct Segment{
    tcp_hdr hdr;
    char data[MSS];
}Segment;

// a tcp conversation core
class CORE{
    private:
        sockaddr_in my_info;
        sockaddr_in your_info;
        int fd;
        char BUFFER[MAX_BUF]; 
        int buf_idx;
        int rwnd;
        float cwnd;
        int ssthres;
        int seq_number; //  myself
        int ack_number; //  myself
        int DupAckCount;


        
    public:
        CORE();
        CORE(char * ip, int port_num);
        CORE(CORE *core);
        
        // Some set and get function
        void set_my_info(char *ip, int port_num);
        void set_your_info(char *ip, int port_num);
        void set_your_info(sockaddr_in info){
            your_info = info;
        }
        sockaddr_in get_your_info(){
            return your_info;
        }
        sockaddr_in get_my_info(){
            return my_info;
        }
        int get_fd(){
            return fd;
        }
        void set_fd(int fd){
            fd = fd;
        }
        char* get_buf(){
            return BUFFER;
        }
        
        int snd_segm(void *, int, int );
        int rcv_segm();

        int cngst_ctrl();
        Segment* packup(void* , int, int);
        void unpack(Segment * );
        
        bool generate_loss();
        unsigned int get_checksum(Segment *);
        void fast_retransmit_recovery(FILE *);

        void clean();
        
        void buf_output(const char *filename);
        void status_msg(char);
        void snd_msg(int, int);
        void rcv_msg(int , tcp_hdr );
    
        void turn_on_rcvfrom(int );

        int lossbyte;
        
        
        char filename[100];
};
CORE::CORE():rwnd(MAX_BUF),cwnd(1), ssthres(THRES), buf_idx(0){
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd == -1){
        perror("Fail to create a socket!");
    }

    srand( time(NULL) );
    seq_number =  rand()%10000 + 1; 
    ack_number = 0;
    
    // initialize
    memset(BUFFER, '\0', MAX_BUF);

    DupAckCount = 0;

}
CORE::CORE(char * ip, int port_num): rwnd(MAX_BUF), cwnd(1), ssthres(THRES), buf_idx(0){
    set_my_info(ip, port_num);

    // IPv4, UDP protocol
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd == -1){
        perror("Fail to create a socket!");
    }

    // combine sockfd with IP and port number
    bind(fd, (sockaddr *) &my_info, sizeof(my_info));

    srand( time(NULL) );
    seq_number =  rand()%10000 + 1; 
    ack_number = 0;
 

    memset(BUFFER, '\0', MAX_BUF);

    DupAckCount = 0;

}
CORE::CORE(CORE *A){
    // IPv4, UDP
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd == -1)
        perror("Fail to create a socket!");

    DupAckCount = A->DupAckCount;

    your_info = A->your_info;
    buf_idx = A->buf_idx;
    memcpy(BUFFER, A->BUFFER, buf_idx);
    rwnd = A->rwnd;
    cwnd = A->cwnd;
    seq_number = A->seq_number;
    ack_number = A->ack_number;

    ssthres = A->ssthres;
}
void CORE::snd_msg(int mode, int dsize){
    cout << "\nSend a Segment with flag( ";
    
    if(!mode)
        cout << "NORMAL ";

    if(mode & 0x1)
        cout << BLUE << "FIN " << ORIGN;

    if(mode & 0x2)
        cout << GREEN << "SYN " << ORIGN;
    
    if(mode & 0x10)
        cout << PURPLE << "ACK " << ORIGN;
    
    
    printf(") to %s : %d, %d byte %s[Seq = %d, ACK = %d]%s-------------\n", inet_ntoa(your_info.sin_addr), your_info.sin_port, dsize, BLUE_GND, seq_number, ack_number, ORIGN);
}

int CORE::snd_segm(void *data, int send_size, int mode)
{
    Segment* segm = NULL;
    int actl_snd_size = 0;

    segm = packup( (char *)data , send_size, mode & (~0x20) ); 

    if( generate_loss() ){       
        segm->hdr.checksum += (rand()%100 + 1) ;
        mode  = mode & (~0x20);
        cout << RED << "\n!!!!!!!!!!!! Loss !!!!!!!!!!!!\n\n" << ORIGN;
    }else{
        segm->hdr.checksum = 0;
    }

    // UDP's sendto
    actl_snd_size = sendto(fd, segm, sizeof(tcp_hdr) + send_size, 0, (sockaddr *)&your_info, sizeof(sockaddr_in) );

    if ( actl_snd_size == -1 ){
        perror("Send message failed.");
        return -1;
    }else
        snd_msg(mode, send_size );

    // ACK or SYN or EOF or FIN
    if(mode & 0x53){
        if( send_size == 0) 
            seq_number = seq_number + 1;
        else
            seq_number = seq_number + send_size;
    }

    
    if(mode & 0x1){
        rcv_segm();
    }

    /*  Normal case, no flag  */
    if( !mode ){
        seq_number += send_size;
    }

    return send_size;
}
void CORE::rcv_msg(int mode, tcp_hdr hdr)
{
    cout << "\n------Receive a segment with flag( ";
    
    if(!mode)
        cout << "NORMAL ";

    if(mode & 0x1)
        cout << BLUE << "FIN " << ORIGN;

    if(mode & 0x2)
        cout << GREEN << "SYN " << ORIGN;
    
    if(mode & 0x10)
        cout << PURPLE << "ACK " << ORIGN;
    
    printf(") to %s : %d, %d byte %s[Seq = %d, ACK = %d]%s\n", inet_ntoa(your_info.sin_addr), your_info.sin_port, hdr.size, BLUE_GND, seq_number, ack_number, ORIGN);
}
int CORE::rcv_segm()
{

    int recv_size;
    Segment *segm = new Segment;

    // fucku very importantl!!
    memset(segm, 0, sizeof(Segment));

    socklen_t addrlen = sizeof(sockaddr_in);
    sockaddr_in addr ;

    // block at here if don't receive sth
    recv_size = recvfrom(fd, segm,  sizeof(Segment), 0, (sockaddr *)&your_info , &addrlen);

     if(recv_size == -1){
        cout << "\nWait time exceed 500ms" << endl;
        return 0;
    }

    int flag = segm->hdr.flag;
    int dsize = recv_size - sizeof( tcp_hdr );
    if( int loss = get_checksum(segm)  ){
        printf( RED"\n\tPacket Broken, Checksum = %x\n", loss);
        cout << ORIGN;
        rcv_msg( flag, segm->hdr);
        printf(ORIGN);
    }else{
        rcv_msg( flag, segm->hdr);
    }  

    /* ACK */
   if ( flag & 0x10){
       int akno = segm->hdr.ack_num, i; 
       if( ack_number == segm->hdr.seq_num ){
            if( dsize == 0){
                ack_number += 1;
            }else{
                ack_number = ack_number + dsize;
            }
            unpack(segm);
            
       }
        
       if ( seq_number != segm->hdr.ack_num){
            cout << RED << "\n\n~~~~~~~~~Duplicated ACK~~~~~~~~~\n\n" << ORIGN;
            DupAckCount ++;
            if ( DupAckCount == 3 ){
                DupAckCount = 0;
                return 0x20;
           }
        }
       
    }
   
    /* SYN or EOF or FIN*/
    if ( flag & 0x43 ){
        ack_number = segm->hdr.seq_num + 1;
    }


    /* EOF */
    if( flag & 0x40){
        buf_output(filename);
    }

    
    if( flag & 0x1){
        snd_segm(NULL, 0, 0x10);
    }

    /* Normal case */
    if( !flag ){  
        if( (ack_number == segm->hdr.seq_num) && !get_checksum(segm)  ){
            unpack(segm );
            ack_number = ack_number +  segm->hdr.size;
        }
    }

    return flag;
}

void CORE::buf_output(const char *filename)
{ 
    FILE *fp;
    fp = fopen(filename,"a");// "a": append  
    fwrite(BUFFER, 1, buf_idx, fp);// write bytes = 1 * buf_idx

    fclose(fp);
    buf_idx = 0;
}
void CORE::set_my_info(char * ip, int port_num){
    memset((void *) &my_info, 0, sizeof(my_info));
    my_info.sin_family = AF_INET;
    my_info.sin_port = port_num;
    my_info.sin_addr.s_addr = inet_addr(ip);
}

void CORE::set_your_info(char * ip, int port_num){
    memset((void *) &your_info, 0, sizeof(your_info));
    your_info.sin_family = AF_INET;
    your_info.sin_port = port_num;
    your_info.sin_addr.s_addr = inet_addr(ip);
}

int CORE::cngst_ctrl(){
    int wnd;
    if(cwnd*MSS < ssthres)
        cwnd *= 2;
    else
        cwnd++;

    if(cwnd < (rwnd/MSS))
        wnd = cwnd;
    else
        wnd = rwnd / MSS;

    return wnd;
}

void CORE::status_msg(char id){
    if( cwnd == 1)
        cout << RED << "\n!!!!!!!!!! Slow Start !!!!!!!!!!" << ORIGN << endl;
    else if( cwnd * MSS == ssthres )
        cout << RED << "\n!!!!!!! Congestion Avoidance !!!!!!!" << ORIGN << endl;
    
    printf("\ncwnd = %d MSS, rwnd = %d byte, threshosld = %d byte\n", (int)cwnd, rwnd, ssthres);
}

Segment* CORE::packup(void * msg, int data_size, int mode)
{
    Segment *segm = new Segment;
    memset(segm, 0, sizeof(Segment) );
    
    segm->hdr.seq_num = seq_number;
    segm->hdr.ack_num = ack_number; 

    segm->hdr.flag = mode;
    segm->hdr.rwnd = MAX_BUF - buf_idx;
    segm->hdr.size = data_size;

    memcpy(segm->data, msg, data_size);

    segm->hdr.checksum = get_checksum(segm);
    return segm;
}
void CORE::unpack(Segment *segm)
{
    memcpy(&BUFFER[buf_idx], segm->data, segm->hdr.size);

    buf_idx += segm->hdr.size ;

    rwnd = segm->hdr.rwnd;
    if ( !rwnd )
        rwnd = MAX_BUF;

    if( buf_idx == MAX_BUF )
        buf_output(filename);
}
void CORE::clean(){
    // restart!!
    buf_idx = 0;
    rwnd = 0;
    cwnd = 1;
    ssthres = THRES;

    srand(time(NULL));
    seq_number = rand() % 10000 + 1; 
    ack_number = 0;

    memset(BUFFER, '\0', MAX_BUF);
}
bool CORE::generate_loss()
{
    poisson_distribution<int> distribution(POIS_MEAN);
    default_random_engine generator;
    int rdn_num = distribution(generator);

    return distribution(generator) > 0;
}
unsigned int CORE::get_checksum(Segment *segm){

    return segm->hdr.checksum;
    
}
void CORE::turn_on_rcvfrom(int msec)
{
    timeval timeout;
    timeout.tv_usec = msec*1000;
	setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
}
void CORE::fast_retransmit_recovery(FILE *fp){
    
    cout << RED << "\n----------------!!!Fast Transmit!!!----------------" << ORIGN << endl;

    fseek(fp, -1 * 3 * MSS,SEEK_CUR);
    seq_number = seq_number - 3*MSS; 

    ssthres = cwnd / 2 * MSS;
    cwnd = 0.5;
}
#endif