#include "TCP.h"
#define ClientNum 4
#define LossPosition 8192

class Server
{
    private:
        CORE * reception; // waiting for connect
        CORE * core[ClientNum];
        pthread_t core_thread[ClientNum];
        int client_counter;
        
    public:
        Server(){}
        Server(char *ip, int port_num);
        int Accept();
        void * Start_transport(void *);
};

int main()
{
    printf("------------------<<Server>>------------------\n");

    char ip[20] = "127.0.0.1";
    Server server(ip, 8700);
    server.Accept();

    return 0;
}

Server::Server(char *ip, int port_num)
{
    reception = new CORE(ip, port_num);
    client_counter = 0;    
}
int Server::Accept()
{
    int flag;

    for(; client_counter < ClientNum;  client_counter++){
        // start to hand shake
        while(1){
            printf("--------Waiting for client request--------\n\n");
            reception->clean();  
            //cout<<"hjkdfshjkldfhjkldfhjkldfhjafkl\n";
            flag = reception->rcv_segm( );
            if(flag & 0x2){ 
                reception->snd_segm( NULL, 0, 0x12);
                flag = reception->rcv_segm( );
                break;
            }
        }
        printf("\n--------Three-way handshake success!--------\n");
        pthread_create( &core_thread[client_counter], NULL, (ThreadFuncPtr)&Server::Start_transport, (void *)this) ;
        sleep(1);
        // cout << "fhjkdzfhjdklfdsahjfdsaklfdshjkfadskl\n";
    }

    
    for(int i = 0;  i < ClientNum;  i++)
        pthread_join(core_thread[i], NULL);
// cout << "fhjkdzfhjdklfdsahjfdsaklfdshjkfadskl\n";
    return 0;
}
void * Server::Start_transport(void *obj)
{
    int fd, id = client_counter;

    core[id] = new CORE(reception);
// cout << "fhjkdzfhjdklfdsahjfdsaklfdshjkfadskl\n";
    FILE *fptr;
    
    char buffer[MAX_BUF], * filename;
    
    filename = core[id]->get_buf();
// cout << "fhjkdzfhjdklfdsahjfdsaklfdshjkfadskl\n";
    while( strlen(filename) ){        
        fptr = fopen(filename, "r");
        if(fptr == NULL){
            cout << RED;
            perror("No such file or directory \" %s \" in the Server \n");
            cout << ORIGN;
            filename = filename + (int)strlen(filename) + 1;
            
            if( !strlen(filename) ){ 
                
                core[id]->snd_segm(NULL, 0, 0x41);
                
                core[id]->rcv_segm();
            }else{
                core[id]->snd_segm(NULL, 0, 0x40);
            }
            continue;// cout << "fhjkdzfhjdklfdsahjfdsaklfdshjkfadskl\n";
        }else{
            printf("\n--- Client ask for file: %s ---\n", filename);
        }

        int wnd = 1, read_size, snd_size;
        int dsize = 0, i, j, loss_flag = 0;

        while(1){
            if(dsize != 0 )
                printf("\n\t[Send a packet at %d bytes]\n", dsize );
            core[id]->status_msg(0); 


        if ( wnd %2 ){
            read_size = fread(buffer, 1, MSS, fptr);
            dsize = dsize + read_size;
            snd_size = core[id]->snd_segm(buffer, read_size, 0);
            if(feof(fptr)){
                    break;
            }
            core[id]->rcv_segm( );               
        }

        for(i = 0; i < wnd/2 ; i++){
            for(j = 0; j < 2; j++){
                read_size = fread(buffer, 1, MSS, fptr);
                dsize = dsize + read_size;
                snd_size = core[id]->snd_segm(buffer, read_size, 0);

                if(feof(fptr))
                    break;
            }
            loss_flag = core[id]->rcv_segm( );

            if(feof(fptr))
                break;            
        }

            wnd = core[id]->cngst_ctrl();  
            
            if(feof(fptr)){// cout << "fhjkdzfhjdklfdsahjfdsaklfdshjkfadskl\n";
                char *tmp = filename;
                
                filename = filename + (int)strlen(filename) + 1;
                if( !strlen(filename) ){ 
                    core[id]->snd_segm(NULL, 0, 0x41);
                    core[id]->rcv_segm();
                }else{
                    core[id]->snd_segm(NULL, 0, 0x40);
                }
                cout << "\n--------- Transport file successfully ---------\n\n";
                break;
            }
        }
        fclose(fptr);// cout << "fhjkdzfhjdklfdsahjfdsaklfdshjkfadskl\n";

    }

    return 0;
}