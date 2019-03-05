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

#define MAX_BUF 1000

//char BUF[BUFSIZ];
char *inputString(FILE*, size_t);
unsigned short checksum(const char *, unsigned);
void build_header(char *, int, unsigned char, unsigned char);
void op_chunk(int, char *, size_t, unsigned char, unsigned char);

int main(int argc, char **argv){
	// Assume command line format is set as follow
	// $ ./client -h [IP] -p [PORT] -o [op] -s [shift]
	int c_socket;
	struct sockaddr_in c_addr;
	int len;
	int n;

	unsigned char op;
	unsigned char shift;
	//char buf[MAX_BUF];
	char rec_buf[MAX_BUF];

	char *m;

	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_addr.s_addr = inet_addr(argv[2]);
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(atoi(argv[4]));
	//c_addr.sin_port = htons(5000);

	op = atoi(argv[6]);
	shift = atoi(argv[8]);

	if((c_socket = socket(PF_INET, SOCK_STREAM, 0))<0){
		printf("Can't create\n");
		return -1;
	}
	if(connect(c_socket, (struct sockaddr *) &c_addr, sizeof(c_addr)) == -1) {
		printf("CAN'T CONNECT\n");
		close(c_socket);
		return -1;
	}
	//memset(buf, 0 , BUFSIZ);


	char *buf;
	buf = inputString(stdin, 12);
	//fgets(buf,256,stdin);

	size_t length = strlen(buf);
	while(length > MAX_BUF - 8){
		//char *chunk = (char *) calloc(MAX_BUF, sizeof(char));
		op_chunk (c_socket, buf, MAX_BUF - 8, op, shift);

		buf = buf + MAX_BUF -8;
		length = length - (MAX_BUF - 8);
		//memcpy (chunk
	}

	op_chunk(c_socket, buf, length, op, shift);
	//printf(buf);

	exit(0);

	char * msg = (char *) calloc(strlen(buf)+8, sizeof(char));

	printf("strlen of buf %d\n", strlen(buf));
	memcpy(msg+8, buf, strlen(buf));
	build_header(msg,strlen(buf), op, shift);
	printf("strlen of buf %d\n", strlen(msg));
	printf(msg+8);
	
	send(c_socket,msg,strlen(buf)+8, 0);
	/*
	int checks;
	printf("send it two times....\n");
	checks = send(c_socket,msg, 10, 0);
	printf("First shot.. %d\n", checks);
	sleep(1);
	checks = send(c_socket,msg+10 , strlen(buf)-2, 0);
	printf("Second shot.. %d\n", checks);
	*/
	unsigned int rec = recv(c_socket, rec_buf, strlen(buf)+8, 0);
	printf("rec[%d] vs strlen[%d]\n", rec, strlen(buf)+8);
	printf("rec is %d\n", rec);
	printf(rec_buf+8);

	free(m);
	close(c_socket);
}
void op_chunk(int c_socket, char *start, size_t length, unsigned char op, unsigned char shift){
	char *msg = (char *) calloc (length+8, sizeof(char));
	memcpy (msg+8, start, length);
	build_header(msg, length+8, op, shift);

	send(c_socket, msg, length+8, 0);

	free(msg);
	char *rcv_msg = (char *) calloc (length+8, sizeof(char));
	int rec = recv(c_socket, rcv_msg, length+8, 0);
	if (checksum(rcv_msg, length+8) != 0){
		printf("[length+8:%u] [rec:%d]\n", length+8, rec);
		printf("[chcksm: %x]\n",checksum(rcv_msg, length+8));
		printf("Something went wrong\n");
	}

	for (int i = 0; i<length ; i++){
		printf("%c", rcv_msg[8+i]);
	}
	//printf(msg+8);
	free(rcv_msg);
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
void build_header (char* h, int length, unsigned char op, unsigned char shift){
	/*
	*(unsigned char*)(h) = 'x';
	*(unsigned char*)(h+1) = 'x';
	*(unsigned int*)(h+4) = (unsigned int) 'zzzz';
	*(unsigned short*)(h+2) = checksum(h, length);
	*/
	*(unsigned char*)(h) = op;
	*(unsigned char*)(h+1) = shift;
	*(int*)(h+4) = htonl(length);
	*(unsigned short*)(h+2) = checksum(h, length);
}

char *inputString(FILE* fp, size_t size){
//The size is extended by the input with the value of the provisional
    char *str;
    int ch;
    size_t len = 0;
    str = realloc(NULL, sizeof(char)*size);//size is start size
    if(!str)return str;
    while(EOF!=(ch=fgetc(fp))/* && ch != '\n'*/ ){
        str[len++]=ch;
        if(len==size){
            str = realloc(str, sizeof(char)*(size+=10));
            if(!str)return str;
        }
    }
    str[len++]='\0';

    return realloc(str, sizeof(char)*len);
}
