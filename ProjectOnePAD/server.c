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

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
int filefd = 0;

/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	char password[16];
} client_t;

client_t *clients[MAX_CLIENTS];

typedef enum state {
  BROADCAST,
  TOUSER,
  GETUSERS
} type_message;

typedef struct {
	char data[50];
	type_message type;
	char user[32];
} msg;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf (char* arr, int length) {

  	int i;
  	for (i = 0; i < length; i++) { // trim \n
    	if (arr[i] == '\n') {
    	  arr[i] = '\0';
    	  break;
    	}
    }
}

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}
/* Send message to a certain user */
void send_message_to_user(char *s, char *uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(strcmp(clients[i]->name, uid) == 0){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

int check_if_is_user(char *s) {
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(strcmp(clients[i]->name, s) == 0)
			return 1;
	}
	return 0;
}

void send_message_self(const char *s, int connfd){
    msg mess;
    strcpy(mess.data, s);
    for(int i = 1; i < MAX_CLIENTS; i++) {
    	if(clients[i]->uid == connfd) {
		    if (write(clients[i]->sockfd, s, strlen(s)) < 0) {
		        perror("Write to descriptor faileddd");
		        return;
		    }
		}
	}
}

void get_users(int uid) {
	
	char names[64];
	msg mess;
	mess.type = GETUSERS;
	// for(int i = 0; i < MAX_CLIENTS; i++) {
	// 	if(clients[i]) {
	// 		sprintf(names, "[%d] %s\r\n", clients[i]->uid, clients[i]->name);
	// 	}
	// }
	/*char *list = mess.data;
	char s[32];
	pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++){
          if (clients[i])
            {
                // list = stpcpy (list, clients[i]->name);
                // list = stpcpy (list, "\n");
                sprintf(s, "%s\n", clients[i]->name);
                printf("%s\n", s);
                send_message_self(s, uid);

            }
    }*/

  mess.type = GETUSERS;
  char *list = mess.data;

  int i;
  for(i = 0; i < MAX_CLIENTS; i++)
  {
    if(clients[i]->sockfd!= 0)
    {
      list = stpcpy(list, clients[i]->name);
      list = stpcpy(list, "\n");
    }
  }

  if(send(clients[uid]->sockfd, &mess, sizeof(msg), 0) < 0)
  {
      perror("Send failed");
      exit(1);
  }

 //    printf("dasadsads\n");
	// strcpy(mess.data, names);
	// printf("%s\n", mess.data);

	//if((send(clients[uid]->sockfd, &mess, sizeof(msg), 0)) < 0) {
	// for(int i = 0; i < MAX_CLIENTS; i++) {
	// 	if(clients[i]->uid == uid) {
	// 		if(write(uid, list, strlen(list)) < 0) {
	// 			perror("ERROR: write to descriptor failed");
	// 			exit(-1);
	// 		}
	// 	}
	// }
	pthread_mutex_unlock(&clients_mutex);
}

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ];
	char name[32];
	char password[32];
	int leave_flag = 0;
	char user_name[32];

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Name
	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else{
		strcpy(cli->name, name);

		if(write(filefd, name, 32) < 0) {
			printf("ERROR: saving name to file\n");
			exit(-1);
		}

		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid);
	}

	// Password
	if(recv(cli->sockfd, password, 32, 0) <= 0 || strlen(password) >= 32-1) {
		printf("Didn't enter the password.\n");
		leave_flag = 1;
	} else{
		strcpy(cli->password, password);
		if(write(filefd, password, 32) < 0) {
			printf("ERROR: saving password to file\n");
			exit(-2);
		}
	}

	bzero(buff_out, BUFFER_SZ);
	
	// int receive = recv(cli->sockfd, &mess, sizeof(msg), 0);

	// switch(mess.type) {
	// 	case BROADCAST: {

	// 		break;
	// 	}
	// }
	// int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
	// printf("%s\n", buff_out);
	msg mess;

	while(1){
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, (void *)&mess, sizeof(msg), 0);
		//printf("%d\n", mess.type);
		// int r = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		// printf("%s\n", buff_out);

		//printf("%s\n", mess.data);
		if (receive > 0){
			if(sizeof(mess) > 0) {
				
				if(mess.type == BROADCAST){
					strcpy(buff_out, mess.data);
					send_message(buff_out, cli->uid);
					str_trim_lf(buff_out, strlen(buff_out));
					printf("%s -> %s\n", buff_out, cli->name);
				} else if(mess.type == TOUSER){
					printf("%s\n", mess.user);
					strcpy(buff_out, mess.data);
					send_message_to_user(buff_out, mess.user);
					str_trim_lf(buff_out, strlen(buff_out));
					printf("%s -> %s\n", buff_out, cli->name);
				} else if(mess.type == GETUSERS) {
					get_users(cli->uid);
				}
			}
		} else if (receive == 0 || strcmp(mess.data, "exit") == 0){
			sprintf(mess.data, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(mess.data, cli->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
		
		bzero(buff_out, BUFFER_SZ);
	}

  /* Delete client from queue and yield thread */
  	close(cli->sockfd);
  	queue_remove(cli->uid);
  	free(cli);
  	cli_count--;
  	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
  	struct sockaddr_in serv_addr;
  	struct sockaddr_in cli_addr;
  	pthread_t tid;

  	if((filefd = open("date.txt", O_RDWR)) < 0) {
  		printf("ERROR: opening file\n");
  		return EXIT_FAILURE;
  	}

  /* Socket settings */
  	listenfd = socket(AF_INET, SOCK_STREAM, 0);
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_addr.s_addr = inet_addr(ip);
  	serv_addr.sin_port = htons(port);

  /* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (char*)&option, sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    	return EXIT_FAILURE;
	}

	/* Bind */
  	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    	perror("ERROR: Socket binding failed");
    	return EXIT_FAILURE;
  }

  /* Listen */
  	if (listen(listenfd, 10) < 0) {
    	perror("ERROR: Socket listening failed");
    	return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if((cli_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}