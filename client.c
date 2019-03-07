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

#define MAX_BUF 1000000

//char BUF[BUFSIZ];
char *inputString(FILE*, size_t);
unsigned short checksum(const char *, unsigned);
unsigned short checksum2(const char *, unsigned);
void build_header(char *, unsigned int, unsigned char, unsigned char);
void op_chunk(int, char *, size_t, unsigned char, unsigned char);
void receiver(int, char *, size_t);
size_t part_receiver(int , char *);

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
	while(length >= MAX_BUF - 8){
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

	/*
	unsigned int rec = recv(c_socket, rec_buf, strlen(buf)+8, 0);
	printf("rec[%d] vs strlen[%d]\n", rec, strlen(buf)+8);
	printf("rec is %d\n", rec);
	printf(rec_buf+8);

	free(m);
	close(c_socket);
	*/
}
void op_chunk(int c_socket, char *start, size_t length, unsigned char op, unsigned char shift){
	char *msg = (char *) calloc (length+9, sizeof(char));
	memcpy (msg+8, start, length);
	build_header(msg, length+8, op, shift);

	send(c_socket, msg, length+8, 0);

	free(msg);
	char *rcv_msg = (char *) calloc (length, sizeof(char));
	
	receiver(c_socket, rcv_msg, length);
	/*
	int rec = recv(c_socket, rcv_msg, length+8, 0);
	*/
	/*
	if (checksum(rcv_msg, length+8) != 0){
		printf("[length+8:%u] [rec:%d]\n", length+8, strlen(rcv_msg+8));
		printf("[chcksm: %x]\n",checksum(rcv_msg, length+8));
		printf("Something went wrong\n");
		for (int j = 0; j<4 ; j++){
			printf("%x\n", rcv_msg[j]);
		}
		printf("rec length [%u][%d]\n", rcv_msg+4, rcv_msg+4);
		exit(0);
	}
	*/
	for (int i = 0; i<length ; i++){
		//printf("seg?\n");
		printf("%c", rcv_msg[i]);

	}
	//printf(msg+8);
	free(rcv_msg);
}
void receiver(int c_socket, char *receiver, size_t length){
	//initially wait for at least 8 bytes
	size_t rec_total = 0;
	while(rec_total != length){
		
		//char p_receiver[MAX_BUF];
		char * p_receiver = calloc(MAX_BUF, sizeof(char));
		size_t p_length = part_receiver(c_socket, p_receiver);
		
		//printf("Now @ receiver, p_length[%u]\n", p_length);
		memcpy(receiver+rec_total, p_receiver+8, p_length -8);
		rec_total += p_length - 8;
		free(p_receiver);
		//int rec = recv(c_socket, receiver+rec_total, length-rec_total, 0);
		//rec_total += rec;
	}
	//printf("rec_total[%u] length[%u]\n", rec_total, length);
}
size_t part_receiver(int c_socket, char *receiver){
	//first see 8 bytes
	size_t rec_part = 0;
	while(rec_part < 8){
		int rec = recv(c_socket, receiver+rec_part, 8-rec_part, 0);
		rec_part += rec;
	}
	size_t new_length = ntohl(*(unsigned int *)(receiver+4));
	//printf("at part_receiver, rec_part[%u]\n", rec_part);

	while(rec_part != new_length){
		int rec = recv(c_socket, receiver+rec_part, new_length-rec_part, 0);
		rec_part += rec;
		//printf("[%d]\n", rec_part);
	}
	if (checksum2(receiver, new_length) != 0){
		//printf("[length+8:%u] [rec:%d]\n", length+8, strlen(rcv_msg+8));
	
		printf("what [%u], [%u]\n", rec_part, new_length);
		printf("[chcksm: %x]\n",checksum2(receiver, new_length));
		printf("Something went wrong\n");
		exit(0);
	}
	return new_length;
}
unsigned short checksum(const char *buf, unsigned size){
	unsigned sum = 0;
	int i;
	
	for (i=0; i< size; i+=2)
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
void build_header (char* h, unsigned int length, unsigned char op, unsigned char shift){
	/*
	*(unsigned char*)(h) = 'x';
	*(unsigned char*)(h+1) = 'x';
	*(unsigned int*)(h+4) = (unsigned int) 'zzzz';
	*(unsigned short*)(h+2) = checksum(h, length);
	*/
	*(unsigned char*)(h) = op;
	*(unsigned char*)(h+1) = shift;
	*(unsigned int*)(h+4) = htonl((unsigned int)length);
	*(unsigned short*)(h+2) = checksum2(h, length);
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
unsigned short checksum2(const char *buf, unsigned size)
{
	unsigned long long sum = 0;
	const unsigned long long *b = (unsigned long long *) buf;

	unsigned t1, t2;
	unsigned short t3, t4;

	/* Main loop - 8 bytes at a time */
	while (size >= sizeof(unsigned long long))
	{
		unsigned long long s = *b++;
		sum += s;
		if (sum < s) sum++;
		size -= 8;
	}

	/* Handle tail less than 8-bytes long */
	buf = (const char *) b;
	if (size & 4)
	{
		unsigned s = *(unsigned *)buf;
		sum += s;
		if (sum < s) sum++;
		buf += 4;
	}

	if (size & 2)
	{
		unsigned short s = *(unsigned short *) buf;
		sum += s;
		if (sum < s) sum++;
		buf += 2;
	}

	if (size)
	{
		unsigned char s = *(unsigned char *) buf;
		sum += s;
		if (sum < s) sum++;
	}

	/* Fold down to 16 bits */
	t1 = sum;
	t2 = sum >> 32;
	t1 += t2;
	if (t1 < t2) t1++;
	t3 = t1;
	t4 = t1 >> 16;
	t3 += t4;
	if (t3 < t4) t3++;

	return ~t3;
}
