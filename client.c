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
#define BUFSIZE 25

//char BUF[BUFSIZ];
char *inputString(FILE*, size_t);
unsigned short checksum(const char *, unsigned);
unsigned short checksum2(const char *, unsigned);
void build_header(char *, unsigned int, unsigned char, unsigned char);
void op_chunk(int, char *, size_t, unsigned char, unsigned char);
size_t part_receiver(int , char *);

int main(int argc, char **argv){
	int c_socket;
	struct sockaddr_in c_addr;
	int len;
	int n;

	char h[BUFSIZE];
	char p[BUFSIZE];
	char o[BUFSIZE];
	char s[BUFSIZE];
	unsigned char op;
	unsigned char shift;

	int choice = -1;
	while ((choice = getopt(argc, argv, "h:p:o:s:")) != -1){
		switch(choice){
			case 'h':
				memcpy(h, optarg, strlen(optarg));
				break;
			case 'p':
				memcpy(p, optarg, strlen(optarg));
				break;
			case 'o':
				memcpy(o, optarg, strlen(optarg));
				break;
			case 's':
				memcpy(s, optarg, strlen(optarg));
				break;
			case '?':
				exit(0);
		}
	}

	memset(&c_addr, 0, sizeof(c_addr));
	c_addr.sin_addr.s_addr = inet_addr(h);
	c_addr.sin_family = AF_INET;
	c_addr.sin_port = htons(atoi(p));

	op = atoi(o);
	shift = atoi(s);

	if((c_socket = socket(PF_INET, SOCK_STREAM, 0))<0){
		return -1;
	}
	if(connect(c_socket, (struct sockaddr *) &c_addr, sizeof(c_addr)) == -1) {
		close(c_socket);
		return -1;
	}

	char *buf;
	buf = inputString(stdin, 12);

	size_t length = strlen(buf);
	while(length >= MAX_BUF - 8){
		op_chunk (c_socket, buf, MAX_BUF - 8, op, shift);
		buf = buf + MAX_BUF -8;
		length = length - (MAX_BUF - 8);
	}
	op_chunk(c_socket, buf, length, op, shift);

	return 0;
}
void op_chunk(int c_socket, char *start, size_t length, unsigned char op, unsigned char shift){
	char *msg = (char *) calloc (length+8, sizeof(char));
	memcpy (msg+8, start, length);
	build_header(msg, length+8, op, shift);

	send(c_socket, msg, length+8, 0);

	free(msg);
	char *rcv_msg = (char *) calloc (length+8, sizeof(char));
	
	size_t rcv_length = part_receiver(c_socket, rcv_msg);
	for (int i = 0; i<length ; i++){
		printf("%c", rcv_msg[i+8]);
	}
	free(rcv_msg);
}
size_t part_receiver(int c_socket, char *receiver){
	size_t rec_part = 0;
	while(rec_part < 8){
		int rec = recv(c_socket, receiver+rec_part, 8-rec_part, 0);
		rec_part += rec;
		if (rec == 0){
			close(c_socket);
			return 0;
		}
	}
	unsigned char op = *(unsigned char *) receiver;
	if (op != 0 && op != 1){
		exit(0);
	}
	size_t new_length = ntohl(*(unsigned int *)(receiver+4));

	while(rec_part != new_length){
		int rec = recv(c_socket, receiver+rec_part, new_length-rec_part, 0);
		rec_part += rec;
		if (rec == 0){
			close(c_socket);
			return 0;
		}
	}
	if (checksum2(receiver, new_length) != 0){
		close(c_socket);
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
void build_header (char* h, unsigned int length, unsigned char op, unsigned char shift){
	if (op!=1 && op!= 0){
		exit(0);
	}
	*(unsigned char*)(h) = op;
	*(unsigned char*)(h+1) = shift;
	*(unsigned int*)(h+4) = htonl((unsigned int)length);
	*(unsigned short*)(h+2) = checksum2(h, length);
}

char *inputString(FILE* fp, size_t size){
    char *str;
    int ch;
    size_t len = 0;
    str = realloc(NULL, sizeof(char)*size);
    if(!str)return str;
    while(EOF!=(ch=fgetc(fp))/* && ch != '\n'*/ ){
        str[len++]=ch;
        if(len==size){
            str = realloc(str, sizeof(char)*(size+=10));
            if(!str)return str;
        }
    }
    //str[len++]='\0';

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
