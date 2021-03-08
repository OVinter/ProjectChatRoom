#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <ctype.h>

#define LENGTH 2048

volatile sig_atomic_t flag = 0;
int sockfd = 0;
int filefd = 0;
char name[32], password[32];

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

/* print "> " in console */
void str_overwrite_stdout() {
  printf("%s", "> ");
  fflush(stdout);
}

/* remove \n and \0 */
void str_trim_lf(char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

/* function to set flag = 1 when press ctr+c or type exit */ 
void catch_ctrl_c_and_exit(int sig) {
  flag = 1;
}

/* mess one to many */
void send_broadcast_message() {
  char message[LENGTH] = {};
  char buffer[LENGTH + 64] = {};
  msg mess;
  mess.type = BROADCAST;
  //bzero(buffer, LENGTH + 32);
  printf("*Write a broadcast message*\n");
  printf("For exit type: exit\n");
  while(1) {
    str_overwrite_stdout();
    fgets(message, LENGTH, stdin);
    str_trim_lf(message, LENGTH);
    strcpy(mess.data, message);
    if (strcmp(mess.data, "exit") == 0) {
      break;
    } else {
      mess.len_mess = strlen(mess.data);
      sprintf(buffer, "%s: %s\n", name, mess.data);
      send(sockfd, &mess , sizeof(msg), 0);
    }
    // put n zeros in mess and buff
    bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
}

/* mess one to one */
void send_message_to_user() {
  char message[LENGTH] = {};
  char buffer[LENGTH + 64] = {};
  char user[32] = {};
  msg mess;
  mess.type = TOUSER;
  fflush(stdin);
  printf("*Write the name of an available user*\n");
  str_overwrite_stdout();
  fgets(user, 32, stdin);
  str_trim_lf(user, 32);
  strcpy(mess.user, user);
  printf("%s\n", mess.user);
  bzero(user, 32);
  fflush(stdin);
  printf("*Write a message to %s*\n", user);
  printf("For exit type: exit\n");
  while(1) {
    str_overwrite_stdout();
    fgets(message, LENGTH, stdin);
    str_trim_lf(message, LENGTH);
    strcpy(mess.data, message);
    if (strcmp(message, "exit") == 0) {
      break;
    } else {
      mess.len_mess = strlen(mess.data);
      sprintf(buffer, "%s: %s\n", name, message);
      send(sockfd, &mess, sizeof(msg), 0);
    }
    bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
}


void send_msg_handler() {
  while(1) {
    char option = '\0';
    printf("Select an option:\n");
    printf("1. Send a broadcast message\n");
    printf("2. Send a message to certain user\n");
    printf("3. Exit\n");
    //str_overwrite_stdout();
    option = fgetc(stdin);
    switch(option) {
      case '1': {
        send_broadcast_message();
        break;
      }
      case '2': {
        send_message_to_user();
        break;
      }
      case '3': {
        catch_ctrl_c_and_exit(2);
        break;
      }
      default: {
        printf("Please try a correct option\n");
        break;
      }
    }
  } 
}

void recv_msg_handler() {
  msg mess;
  while (1) {
    int receive = recv(sockfd, (void*)&mess, sizeof(msg), 0);
    if (receive > 0) {
      printf("%s\n", mess.data);
      str_overwrite_stdout();
    } else if (receive == 0) {
      break;
    }
    memset((void*)&mess, 0, sizeof(msg));
  }
}

int main(int argc, char **argv) {
  if(argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);
  char buffer_out[LENGTH];
  
  if((filefd = open("date.txt", O_RDWR)) < 0) {
    printf("ERROR: opening file\n");
    return EXIT_FAILURE;
  }

  signal(SIGINT, catch_ctrl_c_and_exit);

  printf("Please enter your name: ");
  fgets(name, 32, stdin);
  fflush(stdin);
  str_trim_lf(name, strlen(name));
    
  if (strlen(name) > 32 || strlen(name) < 2) {
    printf("Name must be less than 30 and more than 2 characters.\n");
    return EXIT_FAILURE;
  }

  printf("Please enter your password: ");
  fgets(password, 32, stdin);
  fflush(stdin);
  str_trim_lf(password, strlen(password));
    
  if(strlen(password) > 32) {
    printf("Password must be less than 32\n");
    return EXIT_FAILURE;
  }

  for(int i = 0; i <= strlen(password); i++) {
    if(password[i] != '\0')
        password[i] = password[i] + 1;
  }

  int name_found=0, pass_found=0;
  while(read(filefd,buffer_out,32) > 0) {
    if(strstr(buffer_out,name) != NULL) {
      name_found=1;
      if (read(filefd,buffer_out,32) > 0)
        if (strstr(buffer_out,password) != NULL)
          pass_found=1;
    }
  }
  if (name_found == 1 && pass_found == 0) {
    printf("Wrong Password! \n");
    return EXIT_FAILURE;
  }
  bzero(buffer_out, LENGTH);
  close(filefd);

  struct sockaddr_in server_address;

  /* socket settings */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr(ip);
  server_address.sin_port = htons(port);


  /* sonnect to Server */
  int err = connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
  if (err == -1) {
    printf("ERROR: connect\n");
    return EXIT_FAILURE;
  }

  // send name
  send(sockfd, name, 32, 0);

  // send password
  send(sockfd, password, 32, 0);

  printf("=== WELCOME TO THE CHATROOM ===\n");

  // create a thread 
  pthread_t send_msg_thread;
  if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0) {
    printf("ERROR: pthread\n");
    return EXIT_FAILURE;
  }

  // create a thread
  pthread_t recv_msg_thread;
  if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0) {
    printf("ERROR: pthread\n");
    return EXIT_FAILURE;
  }

  while (1) {
    if(flag) {
      printf("\nBye\n");
      break;
    }
  }

  close(sockfd);

  return EXIT_SUCCESS;
}