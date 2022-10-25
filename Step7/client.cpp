
#include "client.h"

int main()
{

	printf("=====================Client=====================\n");

	/* enter IP address and port number */
	char ip[20] = "127.0.0.1";
	int portno = 8700;
	printf("Enter Server IP : ");
	scanf("%s", ip);
	printf("Enter Server Port Number : ");
	scanf("%d", &portno);

	/* ask for file */
	int num;
	printf("How many file do you want : ");
	scanf("%d", &num);
	//char	filename[10] [50];
	int total = 0;
	char tmp[50];
	char *filename = new char[500];
	for(int i = 1; i <= num; i++){
		printf("Filename%d : ", i );
		scanf("%s", tmp);
		strcpy(&filename[total], tmp);
		total = total + strlen(tmp) + 1;
	}


	/* start connecting */
	TCP_Client client;
	/* first filename */
	printf("\nExpect first output filename : ");
	scanf("%s", client.unit.filename);	

	/* Do 3 way hand shake */
	client.to_connect(ip, portno, filename, total);
	/* start getting file */
	client.communicate();

	return 0;
}
int TCP_Client::to_connect(char * ip, int portno, char * filename, int filechcnt)
{
	/* set server information */
	unit.set_other_info(ip, portno);

	int flag;

	/* try connecting, 3way head shake */
	printf("=====Start the three-way handshake=====\n\n");
	while(1){
		/* send SYN segement, connect request */
		unit.send_data( NULL, 0, 0x2);
		/* receive SYN ack */
		flag = unit.receive_data();
		/* send ack and data */
		if(flag & 0x12){
			/* send filename at last shake */
			unit.send_data( filename, filechcnt, 0x10);
			break;
		}else{
		perror("Connect Failed...Retry\n");
		}
	}
	printf("\n=====Three-way handshake success=====\n");


	return 0;

}
void TCP_Client::communicate()
{
	int flag = 0;
	FILE *fp;
	timeval timeout;

	/* start receiving message */
	while( 1 ){	
		/* won't go down if havn't receive thing */
		flag = unit.receive_data();
		/* normal data */
		if( !flag ){
			/* wait up to 500 ms */
			//unit.nonblocking(500);
			//flag = unit.receive_data();
			/* back to blocking mode */
			//unit.nonblocking(0);
			
			/* send normal ACK */
			unit.send_data(NULL, 0, 0x10);
		}
		/* Recevie Fin segment (already ACK) */
		if( flag & 0x1 ){
			/* send FIN, receive ACK */
			unit.send_data(NULL, 0 , 0x1);
			break;
		}
		/* EOF */
		if(flag & 0x40){
			if ( fp != NULL ) {
				fclose(fp);
				printf("\n<<<<<<< \"%s\" output success >>>>>>>\n", unit.filename);
			} else {
				printf("\n<<<<<<< \"%s\" output failed >>>>>>>\n", unit.filename);
			}
			printf("\nExpect output filename : ");

			scanf("%s", unit.filename);	
		}
	}
	fp = fopen( unit.filename, "rb" );
	if ( fp != NULL ) {
		fclose(fp);
		printf("\n<<<<<<< \"%s\" output success >>>>>>>\n", unit.filename);
	} else {
		printf("\n<<<<<<< \"%s\" output failed >>>>>>>\n", unit.filename);
	}

	close( unit.get_sockfd());
 
}