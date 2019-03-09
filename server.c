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
size_t part_receiver(int , char **);

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
	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if(server_socket<0){
		exit(1);
	}

	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_port = htons(atoi(p));
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/**Designate port and ip address**/

	if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
		exit(1);
	}

	if(listen(server_socket,200)<0){
		exit(1);
	}
	/**Coneect and set  socket**/

	while(1){
		//unsigned int len = sizeof(c_addr);
		//int fd;
		client_addr_size = sizeof (client_addr);
		client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_size);
		if (client_socket <0){
			exit(1);
		}
		int pid = fork();
		if (pid == -1){
			exit(1);
		}
		if (pid == 0){
			//temporary
			while(1){
			//char * rcv_msg = (char *) calloc (MAX_BUF, sizeof (char));
			int rec;
			char *rcv_msg;
			
			
			//rec = recv(client_socket, rcv_msg, 25, 0);
			size_t rcv_length = part_receiver(client_socket, &rcv_msg);

			if (rcv_length == 0){
				free(rcv_msg);
				break;
			}
			if (rec < 0){
				exit(1);
			}
			encrypt(rcv_msg);
			*(unsigned short*) (rcv_msg+2) = (unsigned short) 0; 
			*(unsigned short*) (rcv_msg+2) = checksum2(rcv_msg, rcv_length);
			send(client_socket, rcv_msg, rcv_length,0);

			free(rcv_msg);
			}
			close(client_socket);
			exit(1);
		}
		if (pid > 0){
			close(client_socket);
		}
	}
	exit(0);
}
size_t part_receiver(int c_socket, char **receiver){
	char *temp;
	temp = realloc(NULL, sizeof(char)*8);

	size_t rec_part = 0;
	while(rec_part < 8){
		int rec = recv(c_socket, temp+rec_part, 8-rec_part, 0);
		rec_part += rec;
		if (rec == 0)
			return 0;
	}
	unsigned char op = *(unsigned char *)temp;
	if ((op != 0) && (op!= 1)){
		close(c_socket);
		exit(0);
	}
	size_t new_length = ntohl(*(unsigned int *)(temp+4));
	temp = realloc(temp, sizeof(char)*new_length);

	while(rec_part != new_length){
		int rec = recv(c_socket, temp+rec_part, new_length-rec_part, 0);
		rec_part += rec;
	}
	if (checksum2(temp, new_length) != 0){
		close(c_socket);
		exit(0);
	}
	*receiver = temp;
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
