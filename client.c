#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <dirent.h>

char BUF[BUFSIZ];

unsigned short checksum(const char *, unsigned);
void build_header(char *, int);

int main(int argc, char **argv){
	int c_socket;
	struct sockaddr_in c_addr;
	int len;
	int n;
	char rcvBuf[BUFSIZ];

	int arg =0;
	int arg_pt = 0;

	char buf[BUFSIZ];
	char buff[BUFSIZ];



	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_addr.s_addr = inet_addr(argv[1]);
	c_addr.sin_family = AF_INET;
	//c_addr.sin_port = htons(atoi(argv[4]));
	c_addr.sin_port = htons(5001);

	printf("Let's");
	if((c_socket = socket(PF_INET, SOCK_STREAM, 0))<0){
		printf("Can't create\n");
		return -1;
	}
	printf(" make a new");
	if(connect(c_socket, (struct sockaddr *) &c_addr, sizeof(c_addr)) == -1) {
		printf("CAN'T CONNECT\n");
		close(c_socket);
		return -1;
	}
	printf(" connection!\n");
	memset(buf, 0 , BUFSIZ);

	printf("20140496> ");

	fgets(buf,256,stdin);


	//char * msg;
	char * msg = (char *) calloc(strlen(buf)+8, sizeof(char));

	printf("strlen of buf %d\n", strlen(buf));
	memcpy(msg+8, buf, strlen(buf));
	build_header(msg,strlen(buf));
	printf("strlen of buf %d\n", strlen(msg));
	printf(msg+8);
	
	send(c_socket,msg,strlen(buf)+8, 0);
	
	unsigned int rec = recv(c_socket, buff, strlen(buf)+8, 0);
	printf("rec is %d\n", rec);
	printf(buff+8);

	close(c_socket);
}

unsigned short checksum(const char *buf, unsigned size){
	unsigned sum = 0;
	int i;
	
	for (i=0; i< size-1; i+=2)
	{
		unsigned short word16 = *(unsigned short *) &buf[i];
		sum += word16;
	}
	
	if (size & 1)
	{
		unsigned short word16 = (unsigned char) buf[i];
		sum+= word16;
	}

	while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

	return ~sum;
}
//void build_header (char* h, unsigned char op, unsigned char shift, unsigned int length){
void build_header (char* h, int len){
	/*
	*(unsigned char*)(h) = 'x';
	*(unsigned char*)(h+1) = 'x';
	*(unsigned int*)(h+4) = (unsigned int) 'zzzz';
	*(unsigned short*)(h+2) = checksum(h, length);
	*/
	int length = len +8;
	*(unsigned char*)(h) = 0x01;
	*(unsigned char*)(h+1) = 0x04;
	*(int*)(h+4) = htonl(length);
	*(unsigned short*)(h+2) = checksum(h, length);
}
