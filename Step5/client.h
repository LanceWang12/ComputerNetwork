#include "TCP.h"
void* client(void *);

class TCP_Client
{
    private:

    public:
        /* communicate unite */
        TCP_Unit unit;
        TCP_Client(){}
        int to_connect(char * ip, int portno, char*, int);
        void communicate();
};