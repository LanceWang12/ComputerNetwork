#include "TCP.h"

#define ClientNum 1

void* server(void *);

class TCP_Server
{
    private:
        TCP_Unit * reception;    /* receptionist socket, do nothing */
        TCP_Unit * unit[ClientNum];
        pthread_t unit_thread[ClientNum];
        int client_cnt;
        //int request_file_cnt;
    public:
        /* communicate unite */

        TCP_Server(){}
        TCP_Server(char *ip, int portno);
        int accept_connect();
        void * communicate(void *);
};
