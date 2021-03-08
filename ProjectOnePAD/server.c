#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int client_count = 0;
static int uid = 10;
int filefd = 0;

/* clientent structure */
typedef struct {
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	char password[16];
} client_type;

client_type *clients[MAX_CLIENTS];

typedef enum state {
  BROADCAST,
  TOUSER,
  GETUSERS
} type_message;

/* mess structure */
typedef struct {
	char data[50];
	type_message type;
	char user[32];
	int len_mess;
} msg;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf (char* arr, int length) {
  	int i;
  	for (i = 0; i < length; i++) {
    	if (arr[i] == '\n') {
    	  arr[i] = '\0';
    	  break;
    	}
    }
}

void print_client_address(struct sockaddr_in addr) {
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* add clients to queue */
void queue_add_client(client_type *cl) {
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(!clients[i]) {
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* remove clients to queue */
void queue_remove_client(int uid) {
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i) {
		if(clients[i]) {
			if(clients[i]->uid == uid) {
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* send message to all clients except sender */
void send_message_broadcast(char *s, int uid) {
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i) {
		if(clients[i]) { 
			if(clients[i]->uid != uid) {
				if(write(clients[i]->sockfd, s, strlen(s)) < 0) {
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* send message to a certain user */
void send_message_to_user(char *s, char *uid) {
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i<MAX_CLIENTS; ++i) {
		if(clients[i]) {
			if(strcmp(clients[i]->name, uid) == 0) {
				if(write(clients[i]->sockfd, s, strlen(s)) < 0) {
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* handle all communication with the clientent */
void *handle_client(void *arg) {
	char buffer_out[BUFFER_SZ];
	char name[32];
	char password[32];
	int leave_flag = 0;

	client_count++;
	client_type *client = (client_type *)arg;

	// name
	if(recv(client->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1) {
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else {
		strcpy(client->name, name);

		if(write(filefd, name, 32) < 0) {
			printf("ERROR: saving name to file\n");
			exit(-1);
		}

		sprintf(buffer_out, "%s has joined\n", client->name);
		printf("%s", buffer_out);
		send_message_broadcast(buffer_out, client->uid);
	}

	// password
	if(recv(client->sockfd, password, 32, 0) <= 0 || strlen(password) >= 32-1) {
		printf("Didn't enter the password.\n");
		leave_flag = 1;
	} else {
		strcpy(client->password, password);
		if(write(filefd, password, 32) < 0) {
			printf("ERROR: saving password to file\n");
			exit(-2);
		}
	}
	bzero(buffer_out, BUFFER_SZ);

	msg mess;

	while(1){
		if (leave_flag) {
			break;
		}
		int receive = recv(client->sockfd, (void *)&mess, sizeof(msg), 0);
		if (receive > 0){
			if(sizeof(mess) > 0) {
				if(mess.type == BROADCAST) {
					strcpy(buffer_out, mess.data);
					printf("%d\n", mess.len_mess);
					if(strlen(mess.data) == mess.len_mess) {
						send_message_broadcast(buffer_out, client->uid);
						str_trim_lf(buffer_out, strlen(buffer_out));
						printf("%s -> %s\n", buffer_out, client->name);
					}
				} else if(mess.type == TOUSER) {
					if(strlen(mess.data) == mess.len_mess) {
						printf("%s\n", mess.user);
						strcpy(buffer_out, mess.data);
						send_message_to_user(buffer_out, mess.user);
						str_trim_lf(buffer_out, strlen(buffer_out));
						printf("%s -> %s\n", buffer_out, client->name);
					}
				}
			}
		} else if (receive == 0 || strcmp(mess.data, "exit") == 0) {
			sprintf(mess.data, "%s has left\n", client->name);
			printf("%s", buffer_out);
			send_message_broadcast(mess.data, client->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
		bzero(buffer_out, BUFFER_SZ);
	}

  /* delete clientent from queue and yield thread */
  	close(client->sockfd);
  	queue_remove_client(client->uid);
  	free(client);
  	client_count--;
  	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv) {
	if(argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
  	struct sockaddr_in serv_addr;
  	struct sockaddr_in client_addr;
  	pthread_t tid;

  	if((filefd = open("date.txt", O_RDWR)) < 0) {
  		printf("ERROR: opening file\n");
  		return EXIT_FAILURE;
  	}

  /* socket settings */
  	listenfd = socket(AF_INET, SOCK_STREAM, 0);
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_addr.s_addr = inet_addr(ip);
  	serv_addr.sin_port = htons(port);

  /* ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (char*)&option, sizeof(option)) < 0) {
		perror("ERROR: setsockopt failed");
    	return EXIT_FAILURE;
	}

	/* bind */
  	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    	perror("ERROR: Socket binding failed");
    	return EXIT_FAILURE;
  	}

  /* listen */
  	if (listen(listenfd, 10) < 0) {
    	perror("ERROR: Socket listening failed");
    	return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");

	while(1) {
		socklen_t clientlen = sizeof(client_addr);
		connfd = accept(listenfd, (struct sockaddr*)&client_addr, &clientlen);

		/* check if max clients is reached */
		if((client_count + 1) == MAX_CLIENTS) {
			printf("Max clients reached. Rejected: ");
			print_client_address(client_addr);
			printf(":%d\n", client_addr.sin_port);
			close(connfd);
			continue;
		}

		/* clientent settings */
		client_type *client = (client_type *)malloc(sizeof(client_type));
		client->address = client_addr;
		client->sockfd = connfd;
		client->uid = uid++;

		/* add clientent to the queue and fork thread */
		queue_add_client(client);
		pthread_create(&tid, NULL, &handle_client, (void*)client);

		/* reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}