#include "TCP.h"

class Client
{
    private:

    public:
        CORE core;
        Client(){}
        int connect_to_server(char * ip, int port_num, char*, int);
        void start_to_rcv();
};

int main(){
	printf("------------------<<Client>>------------------\n");

	char ip[20] = "127.0.0.1";
	int port_num = 8700;
	cout << "Which server do you want to connect?" << endl << "Server IP, Port num: ";
	cin >> ip >> port_num;

	/* ask for file */
	int num;
	cout << "File num(max to 5): ";
	cin >> num;
	int total = 0;
	char tmp[50];
	char *filename = new char[500];
	for(int i = 1; i <= num; i++){
		printf("File%d : ", i );
		cin >> tmp;
		strcpy(&filename[total], tmp);
		total = total + strlen(tmp) + 1;
	}

	Client client;
	cout << "\nOutput file name: ";
	cin >> client.core.filename;
	
	// Start to 3-way hand shake
	client.connect_to_server(ip, port_num, filename, total);

	// receive data from server
	client.start_to_rcv();

	return 0;
}
int Client::connect_to_server(char * ip, int port_num, char * filename, int filechcnt)
{
	core.set_your_info(ip, port_num);

	int flag;
	printf("-------Start the three-way handshake-------\n\n");
	while(true){
		/* send SYN segement, connect request */
		core.snd_segm( NULL, 0, 0x2);
		/* receive SYN ack */
		flag = core.rcv_segm();
		/* send ack and data */
		if(flag & 0x12){
			/* send filename at last shake */
			core.snd_segm( filename, filechcnt, 0x10);
			break;
		}else
			perror("Handshake faled!!\n");
	}
	cout<< "\n-------Handshake successfully-------\n";


	return 0;

}
void Client::start_to_rcv(){
	int flag = 0;
	timeval timeout;

	while(true){	
		flag = core.rcv_segm();

		if( !flag ){
			core.turn_on_rcvfrom(500);

			flag = core.rcv_segm();
			
			core.turn_on_rcvfrom(0);

			core.snd_segm(NULL, 0, 0x10);
		}
		if( flag & 0x1 ){
			core.snd_segm(NULL, 0 , 0x1);
			break;
		}
		
		if(flag & 0x40){
			cout << "\n--------- Output file successfully ---------\n";
			cout << "\nOutput filename: ";
			cin >> core.filename;
		}
	}
	cout << "\n--------- Output file successfully ---------\n";
	close( core.get_fd());
}