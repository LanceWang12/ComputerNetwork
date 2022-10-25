#include "server.h"


int main()
{
    // struct in_addr server_addr = {.s_addr = }
    // struct sockaddr_in clientsocket = {.sin_family = AF_INET, .sin_port = 1200, .sin_addr};

    printf("=====================Server=====================\n");

    char ip[20] = "127.0.0.1";
    int portno;

    printf("Input Server IP : ");
    scanf("%s", ip);
    printf("Input Server Port Number : ");
    scanf("%d", &portno);
    TCP_Server server( ip, portno );

    server.accept_connect();

    while(1);


    return 0;
}

TCP_Server::TCP_Server(char *ip, int portno)
{
    /* reception socket */
    reception = new TCP_Unit(ip, portno);

    /* record current amout client */
    client_cnt = 0;    
}
int TCP_Server::accept_connect()
{
    int flag;

    for(;  client_cnt < ClientNum;  client_cnt++){
        /* 3 hand shake */
        while(1){
            printf("=====Waiting for request=====\n\n");
            /* clean up before meet next client */
            reception->dress_up();  
            flag = reception->receive_data( );
            if(flag & 0x2){ /* receive syn segment */
                reception->send_data( NULL, 0, 0x12);
                /* receive final ack */
                flag = reception->receive_data( );
                break;
            }
        }
        printf("\n=====Three-way handshake success=====\n");
        /* 3 hand shake end */
        
        /* can't create after accept, or the fd will be only for this main thread use */
        pthread_create( &unit_thread[client_cnt], NULL, (ThreadFuncPtr)&TCP_Server::communicate, (void *)this) ;
        /* avoid race condition of client_cnt in thread, can use mutex, but hard*/
        sleep(1);
    }

    
    /*  wait for client finish */
    for(int i = 0;  i < ClientNum;  i++)
        pthread_join(unit_thread[i], NULL);

    return 0;
}
void * TCP_Server::communicate(void *obj)
{
    int fd, id = client_cnt;

    unit[id] = new TCP_Unit(reception);

    FILE *fptr;
    
    /* local buffer, local filename */
    char buffer[RcvBuffer], * filename;
    
    /* get the filename */
    filename = unit[id]->get_buf();

    /* Check whether more require file or more*/
    while( strlen(filename) ){        
        /* try to open the file */
        fptr = fopen(filename, "r");
        /* file not exist */
        if(fptr == NULL){
            printf("\n<<< No such file or directory \" %s \" in the Server >>>\n\n", filename);
            /* try to get next file */
            filename = filename + (int)strlen(filename) + 1;
            /* no more file */
            if( !strlen(filename) ){ 
                /* send EOF FIN, get ACK */
                unit[id]->send_data(NULL, 0, 0x41);
                /* receive FIN, get ACK */
                unit[id]->receive_data();
            }else{      /* have other file */
                /* send EOF */
                unit[id]->send_data(NULL, 0, 0x40);
            }
            continue;
        }else{
            printf("\n<<< Client ask for \"%s\" >>>\n", filename);
        }

        /* wnd = min( cwnd, rwnd/MSS ) */
        int wnd = 1, read_size, snd_size;
        int dsize = 0, i, j;
        int timer_start;
        int timer_end;
        /* sending a file */
        while(1){
            if(dsize != 0 )
                printf("\n<<<<<<<<<<<<<<<<<<< Send a packet at %d bytes >>>>>>>>>>>>>>>>>\n", dsize );
            /* current cwnd, rwnd information */
            unit[id]->print_stutas(0); 

            /* send to time, receive one time */
            for(i = 0; i < wnd; i++){
                //for( j = 0; j < 2; j++){
                    read_size = fread(buffer, 1, MSS, fptr);
                    dsize = dsize + read_size;
                    
                    /* send normal data, 0 or 0x80 */
                    //timer_start = clock();
                    snd_size = unit[id]->send_data(buffer, read_size, 0);
                    
                unit[id]->receive_data( );
                //timer_end = clock();
                //printf("\nRTT = %d\n", timer_end - timer_start );
                /* will trigger fast retransmit */
                if(feof(fptr))
                    break;
            }

            wnd = unit[id]->congestion_ctrl();  
            /* send fin segment, won't bring any data */
            if(feof(fptr)){
                char *tmp = filename;
                /* get next filename */
                filename = filename + (int)strlen(filename) + 1;
                if( !strlen(filename) ){ 
                    /* send EOF FIN, get ACK */
                    unit[id]->send_data(NULL, 0, 0x41);
                    /* receive FIN, get ACK */
                    unit[id]->receive_data();
                }else{
                    /* send EOF */
                    unit[id]->send_data(NULL, 0, 0x40);
                }
                printf("\n<<<<<< Send \"%s\" success >>>>>>\n", tmp);
                break;
            }
        }
        fclose(fptr);

    }

    return 0;
}