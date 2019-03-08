#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/uio.h>
#include <fcntl.h>

#define MAX_BUF 1000000 
#define BUFSIZE 25
void receiver(int, char *, size_t);
size_t part_receiver(int , char *);

//void encrypt (char *, unsigned, unsigned char, unsigned char);
void encrypt (char *);
void cipher (char *, unsigned char, unsigned char);
unsigned short checksum2(const char *, unsigned);

int main(int argc, char *argv[]){
	int server_socket;
	int client_socket;

	int client_addr_size;

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	char buff[MAX_BUF];/**Data for sending and receving**/

	char p[BUFSIZE];
	int choice = -1;
	while ((choice = getopt(argc, argv, "p:")) != -1){
		switch(choice){
			case 'p':
				memcpy(p, optarg, strlen(optarg));
				break;
			case '?':
				exit(0);
		}
	}

	printf("=Server Start\n");
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(server_socket<0){
		printf("%s \n", "fail to make socket");
		exit(1);
	}

	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_port = htons(atoi(p));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/**Designate port and ip address**/

	if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
		printf("%s \n", "bind error");
		exit(1);
	}

	if(listen(server_socket,100)<0){
		printf("%s \n","listen error");
		exit(1);
	}
	/**Coneect and set  socket**/

	while(1){
		//unsigned int len = sizeof(c_addr);
		//int fd;
		client_addr_size = sizeof (client_addr);
		client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_size);
		if (client_socket <0){
			printf("can't cbind\n");
			exit(1);
		}
		int pid = fork();
		if (pid == -1){
			exit(1);
		}
		if (pid == 0){
			//temporary
			while(1){
			char * rcv_msg = (char *) calloc (MAX_BUF, sizeof (char));
			int rec;
			
			
			//rec = recv(client_socket, rcv_msg, 25, 0);
			size_t rcv_length = part_receiver(client_socket, rcv_msg);

			if (rcv_length == 0)
				break;
			if (rec < 0){
				printf("rec error\n");
				exit(1);
			}
			encrypt(rcv_msg);
			*(unsigned short*) (rcv_msg+2) = (unsigned short) 0; 
			*(unsigned short*) (rcv_msg+2) = checksum2(rcv_msg, rcv_length);
			send(client_socket, rcv_msg, rcv_length,0);

			/*
			rec = recv(client_socket, buffer, BUFSIZ, 0);
			if (rec < 0){
				printf("rec error\n");
				exit(1);
			}
			printf("Received Message is like \n");
			printf(buffer);
			*/
			}
			close(client_socket);
			exit(1);


		}
		if (pid > 0){
			close(client_socket);
		}
	}
	exit(0);
	while(1){
		client_addr_size = sizeof(client_addr);
		client_socket = accept(server_socket,(struct sockaddr*)&client_addr,&client_addr_size);/**Wait until client socket is connected**/
		printf("=================OK Server\n");

		if(client_socket<0){
			printf("%s \n","cleint connection error");
			exit(1);
		}

		//read(client_socket, buff,BUFSIZ);/**Receive the signal**/
		printf("client saids... \n");
		printf(buff+8);
		close(client_socket);

	}
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
		if (rec == 0)
			return 0;
		//printf("this core? %d\n", rec_part);
	}
	unsigned char op = *(unsigned char *)receiver;
	if ((op != 0) && (op!= 1)){
		exit(0);
	}
	size_t new_length = ntohl(*(unsigned int *)(receiver+4));
	//printf("at part_receiver, rec_part[%u]\n", rec_part);

	while(rec_part != new_length){
		//printf("this core??\n");
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

void encrypt(char *msg){
	unsigned char op = *(unsigned char *) msg;
	unsigned char shift = *(unsigned char *) (msg+1);
	int length = ntohl(*(unsigned int *) (msg+4));

	for (unsigned i = 8; i < length; i++){
		if ((msg[i]>=65) && (msg[i]<=90)){
			msg[i] = msg[i] +32;
		}
		if ((msg[i]>=97) && (msg[i]<=122)){
			cipher(msg+i, op, shift);
		}
	}
}

void cipher(char *letter, unsigned char op, unsigned char shift){
	int temp = (int) *letter;
	int shifter;

	if (op == 1){
		shifter = -1 * shift;
	}else if (op == 0){
		shifter = shift;
	}else{
		exit(0);
	}

	temp -= 97;

	temp += shifter;

	temp = temp % 26;
	if (temp < 0){
		temp += 26;
	}

	*letter = (char) temp+97;

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
