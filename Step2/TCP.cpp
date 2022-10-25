#include "TCP.h"

TCP_Unit::TCP_Unit():rwnd(RcvBuffer),cwnd(1), ssthreshold(Threshold), bufptr(0)
{
    /* create socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1){
        perror("Fail to create a socket...Return");
    }

    srand( time(NULL) );
    seq_number =  rand()%10000 + 1; 
    ack_number = 0;
    biggest_ack_number = 0;
    
    /* set buffer to end of string */
    memset(buf, '\0', RcvBuffer);

    /* initialize ack count*/
    dupACKcount = 0;

}
TCP_Unit::TCP_Unit(char * ip, int portno):rwnd(RcvBuffer),cwnd(1), ssthreshold(Threshold), bufptr(0)
{
    /* set itself info */
    set_itself_info(ip, portno);
    /* create socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1){
        perror("Fail to create a socket...Return");
    }

    /* bind socket file description with spcefiy IP and port number */
    bind(sockfd, (sockaddr *) &itself_info, sizeof(itself_info));

    /* 1 ~ 10000 random number */
    srand( time(NULL) );
    seq_number =  rand()%10000 + 1; 
    ack_number = 0;
    biggest_ack_number = 0;

    /* set buffer to end of string */
    memset(buf, '\0', RcvBuffer);
    /* initialize ack */
    dupACKcount = 0;
}
TCP_Unit::TCP_Unit(TCP_Unit *unit)
{
    /* create socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1){
        perror("Fail to create a socket...Return");
    }


    other_info = unit->other_info;
    bufptr = unit->bufptr;
    /* filename information */
    memcpy(buf, unit->buf, bufptr);
    rwnd = unit->rwnd;
    cwnd = unit->cwnd;
    ssthreshold = unit->ssthreshold;
    seq_number = unit->seq_number;
    ack_number = unit->ack_number;
    biggest_ack_number = unit->biggest_ack_number;
    /* initialize ack */
    dupACKcount = unit -> dupACKcount;
}
void TCP_Unit::print_send_msg(int mode, int dsize)
{
    printf( GREEN "\nSend a Packet ( ");
    /* FIN */
    if( mode & 0x1 ){
        printf("FIN ");
    }
    if( mode & 0x2 ){
        printf("SYN ");
    }
    if( mode & 0x4 ){
        printf("RST ");
    }
    if( mode & 0x8 ){
        printf("PSH ");
    }
    if( mode & 0x10 ){
        printf("ACK ");
    }
    if( mode & 0x20 ){
        printf("URG ");
    }
    if ( mode & 0x40 ){
        printf("EOF ");
    }
    printf(") to %s : %d, %d byte ( Seq = %d, ACK = %d )\n" NONE,\
     inet_ntoa(other_info.sin_addr), other_info.sin_port, dsize, seq_number, ack_number);
}
int TCP_Unit::send_data(void *data, int send_size, int mode)
{
    /*  mode means flag */


    tcp_segment* segm = NULL;
    int actual_send_size = 0;

    /* whole file, consume indicate error flag */
    segm = packup( (char *)data , send_size, mode & (~0x80) ); 

    /* packet loss or not */
    if( generate_loss() ){       
        /* 0x80 indicate loss at LossPosition, so we mess up the checksum by random number */
        segm->header.checksum += rand() ;
        /* consume flag */ 
        printf( RED "\n***lose***" NONE);
    }
    actual_send_size = sendto(sockfd, segm, sizeof(tcp_header) + send_size, 0, (sockaddr *)&other_info, sizeof(sockaddr_in) );

    if ( actual_send_size == -1 ){
        /* need some handle */
        perror("Send message failed.");
        return -1;
    }else{
        /* print the send message */
        print_send_msg(mode, send_size );
    }

    /* ACK or SYN or EOF or FIN */
    if(mode & 0x53){
        /*  if no data in segment , acknumber still need plus one */
        if( send_size == 0) 
            seq_number = seq_number + 1;
        else
            seq_number = seq_number + send_size;
    }

    /* Fin */
    if(mode & 0x1){
        /* wait for ack*/
        receive_data();
    }

    /*  Normal case, no flag  */
    if( !mode ){
        /* send normal data, need updata our sequence number */
        seq_number += send_size;
    }

    return send_size;
}
void TCP_Unit::print_receive_msg(int mode, tcp_header header)
{
    printf( YELLOW "\n\tReceive a Packet ( ");
    /* FIN */
    if( mode & 0x1 ){
        printf("FIN ");
    }
    if( mode & 0x2 ){
        printf("SYN ");
    }
    if( mode & 0x4 ){
        printf("RST ");
    }
    if( mode & 0x8 ){
        printf("PSH ");
    }
    if( mode & 0x10 ){
        printf("ACK ");
    }
    if( mode & 0x20 ){
        printf("URG ");
    }
    if ( mode & 0x40 ){
        printf("EOF ");
    }

    printf(") from %s : %d,  %d byte ( Seq = %d, ACK = %d )\n" NONE,\
     inet_ntoa(other_info.sin_addr), other_info.sin_port, header.data_length , header.seqno, header.ackno);
}
int TCP_Unit::receive_data()
{

    int recv_size;

    /* segm is true segment, tmp is just temporary receive the data that exceed 64K*/
    tcp_segment *segm = new tcp_segment;
    /* need clear or it will mess up */
    memset(segm, 0, sizeof(tcp_segment));

    /* some information variable, not important */
    socklen_t addrlen = sizeof(sockaddr_in);
    sockaddr_in addr ;


    /* get segment in to tmp */
    recv_size = recvfrom(sockfd, segm,  sizeof(tcp_segment), 0, (sockaddr *)&other_info , &addrlen);

     if(recv_size == -1){
        printf("\nWait time exceed 500ms\n");
        return 0;
    }

    int flag = segm->header.flag;
    /* data size */
    int dsize = recv_size - sizeof( tcp_header );
    /* check sum, the value should be 0xFFFFFFFF convert --->>>  0 */
    if( int loss = do_checksum(segm)  ){
        printf( "\033[31m\n\tPacket Broken, Checksum = %x", loss);
        print_receive_msg( flag, segm->header);
        printf("\033[0m" );
    }else{
        print_receive_msg( flag, segm->header);
    }  

    /* ACK */
   if ( flag & 0x10){
       /* receive ack number is expect number */
       if( ack_number == segm->header.seqno ){
           /* the ack segment has no data, but ack number plus one */
            if( dsize == 0){
                ack_number += 1;
            }else{          /*the ack has data means its contain require filename */
                ack_number = ack_number + dsize;
            }
            unpack(segm);
       }

    }
    /* SYN or EOF or FIN*/
    if ( flag & 0x43 ){
        ack_number = segm->header.seqno + 1;
    }


    /* EOF */
    if( flag & 0x40){
        /* clean up */
        buf2file(filename);
    }

    /* FIN */
    if( flag & 0x1){
        /*  send ack */
        send_data(NULL, 0, 0x10);
    }

    /* Normal case */
    if( !flag ){  
        /* receive data is  the expect data, and check is zero */
        if( ack_number == segm->header.seqno  ){
            //printf("?");
            /* get data */
            unpack(segm);
            /* update ack_number */
            ack_number = ack_number +  segm->header.data_length ;
        }
    }

    return flag;
}

void TCP_Unit::buf2file(const char *filename)
{
    FILE *fptr;
    fptr = fopen( filename,"a" );    
    fwrite(buf, 1, bufptr, fptr);

    fclose(fptr);
    /* clean up */
    bufptr = 0;
}
void TCP_Unit::set_itself_info(char * ip, int portno)
{
    memset((void *) &itself_info, 0, sizeof(itself_info));

    /* Defined other(server) information */
    itself_info.sin_family = AF_INET;  /* Protocol Family */
    itself_info.sin_port = portno;
    itself_info.sin_addr.s_addr = inet_addr(ip);
}
void TCP_Unit::set_other_info(char * ip, int portno)
{
    memset((void *) &other_info, 0, sizeof(other_info));

    /* Defined other(server) information */
    other_info.sin_family = AF_INET;  /* Protocol Family */
    other_info.sin_port = portno;
    other_info.sin_addr.s_addr = inet_addr(ip);
}
int TCP_Unit::congestion_ctrl()
{
    int wnd;


    if(cwnd*MSS < ssthreshold)
        cwnd *= 2;
    else
        cwnd++;

    wnd = cwnd < (rwnd/MSS) ? cwnd : (rwnd/MSS);

    return wnd;
}
void TCP_Unit::print_stutas(char id)
{
    if( cwnd == 1){
        printf("\n********* Slow Start ************\n");
    }else if( cwnd*MSS == ssthreshold ){
        printf("\n********* Congestion Avoidance ************\n");
    }

    printf("\ncwnd = %d MSS, rwnd = %d byte, threshold = %d byte\n", (int)cwnd, rwnd, ssthreshold);
}
tcp_segment* TCP_Unit::packup(void * msg, int data_size, int mode)
{
    tcp_segment *segm = new tcp_segment;
    memset(segm, 0, sizeof(tcp_segment) );
    
    segm->header.seqno = seq_number;
    segm->header.ackno = ack_number; 

    segm->header.flag = mode;
    /* meanful for ack segment */
    segm->header.rwnd = RcvBuffer - bufptr;
    segm->header.data_length = data_size;

    /* copy content in memory, (dist mem, srcmem, byte) */
    memcpy(segm->data, msg, data_size);

    segm->header.checksum = do_checksum(segm);

    return segm;
}
void TCP_Unit::unpack(tcp_segment *segm )
{
    /* copy data into local buffer */
    memcpy(&buf[bufptr], segm->data, segm->header.data_length);
    /* move the pointer because the space have had data */
    bufptr += segm->header.data_length ;

    /* record sender rwnd */
    rwnd = segm->header.rwnd;
    /* kind of trick */
    if ( !rwnd )
        rwnd = RcvBuffer;

    /* if rwnd is 0, means buffer is full, clean up buffer, write data to file */
    if( bufptr == RcvBuffer ){
        buf2file(filename);
    }
}
void TCP_Unit::dress_up(){


    bufptr = 0;
    rwnd = 0;
    cwnd = 1;
    ssthreshold = Threshold;

    /* 1 ~ 10000 random number */
    srand( time(NULL) );
    seq_number =  rand()%10000 + 1; 
    ack_number = 0;

    memset(buf, '\0', RcvBuffer);
}
bool TCP_Unit::generate_loss()
{
    std::poisson_distribution<int> p(PoissonMean);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator (seed);

    return p( generator ) > 0;
}
unsigned int TCP_Unit::do_checksum(tcp_segment *segm)
{
    /* filled zero */
    unsigned int op, *data, sum;
    int dsize32;
    char carry;

    /* just for convientent */
    data = (unsigned int *) segm;

    /* how many 32 bits unit */
    dsize32 = (int)sizeof(tcp_header) / 4;

    /* first 32 bits */
    sum = *data;
    for( int i = 0; i < dsize32; i++){
        /* next 32 bits */
        data++;
        /* first oprand */
        op = *data;

        /* get carry, 0 or 1 */
        carry =  ( (op) >>31 ) ;
        /* make op1 top bits zero */
        op = (op) & 0x7FFFFFFF;
        /* plus ( won't over flow ) */
        sum = op + sum;
        /* top bits one and carry is one*/
        if( ( sum >>  31 ) & carry ){
            /* first bit to 0 ( carry ) */
            sum = (sum) & 0x7FFFFFFF;
            /* carry plus to last bit */
            sum = sum + 1;
        }else{  /* put back carry */
            sum = sum + (carry<<31);
        }
    }

    return ~sum;
}
void TCP_Unit::nonblocking(int msec)
{
    timeval timeout;
    timeout.tv_usec = msec*1000;
	setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
}
void TCP_Unit::fast_retransmit_recovery(FILE *fptr)
{
    printf("\n***********Fast Retransmit************\n");
    /* the best should be the current client sequent number - ack number */
    /* but here we can sure that the data will be 3 MSS before, so  just -3 MSS*/
    fseek(fptr, -1 * 3 * MSS,SEEK_CUR);
    seq_number = seq_number - 3*MSS; 
    /* ssthreshold is current cwnd half */
    ssthreshold = ( cwnd / 2 ) * MSS;
    /* slow start */
    cwnd = 0.5;
    
}