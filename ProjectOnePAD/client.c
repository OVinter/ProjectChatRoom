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

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
int filefd = 0;
char name[32], password[32];

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

void str_overwrite_stdout() {
    printf("%s", "> ");
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

void catch_ctrl_c_and_exit(int sig) {
  flag = 1;
}
// 1 -> send(server) -> client -> send_message from server -> send to all users
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
      sprintf(buffer, "%s: %s\n", name, mess.data);
      send(sockfd, &mess , sizeof(msg), 0);
    }

    bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
  //catch_ctrl_c_and_exit(2);
}

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
      sprintf(buffer, "%s: %s\n", name, message);
      send(sockfd, &mess, sizeof(msg), 0);
    }

    bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
}

void print_all_users() {
  msg mess;
  mess.type = GETUSERS;
  str_overwrite_stdout();
  strcpy(mess.data, "Dasdas");
  strcpy(mess.user, "Das");
  send(sockfd, &mess, sizeof(msg), 0);
}

void send_msg_handler() {
  

  while(1) {
    char option = '\0';
    printf("Select an option:\n");
    printf("1. Send a broadcast message\n");
    printf("2. Send a message to certain user\n");
    printf("3. Print all users\n");
    printf("4. Exit\n");
    //str_overwrite_stdout();
    option = fgetc(stdin);
    switch(option) {
      case '1': {
        // char buffer[LENGTH + 64] = {};
        // sprintf(buffer, "%s\n", "dasdsasda");
        // printf("%s\n", buffer);
        // send(sockfd, buffer, strlen(buffer), 0);
        send_broadcast_message();
        break;
      }
      case '2': {
        send_message_to_user();
        break;
      }
      case '3': {
        print_all_users();
        fflush(stdin);
        //printf("aa\n");
        break;
      }
      case '4': {
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

// void recv_msg_handler() {
//   char message[LENGTH] = {};
//   while (1) {
//     int receive = recv(sockfd, message, LENGTH, 0);
//     if (receive > 0) {
//       printf("%s", message);
//       str_overwrite_stdout();
//     } else if (receive == 0) {
//       break;
//     } else {
//       // -1
//     }
//     memset(message, 0, sizeof(message));
//   }
// }

int main(int argc, char **argv){
  if(argc != 2){
    printf("Usage: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);
  char buff_out[LENGTH];
  
  if((filefd = open("date.txt", O_RDWR)) < 0) {
      printf("ERROR: opening file\n");
      return EXIT_FAILURE;
    }

  signal(SIGINT, catch_ctrl_c_and_exit);

  printf("Please enter your name: ");
    fgets(name, 32, stdin);
    fflush(stdin);
    str_trim_lf(name, strlen(name));
    
    if (strlen(name) > 32 || strlen(name) < 2){
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
      if(password[i] != '\0') 
        password[i] = password[i] + 1;
    }

    int name_found=0,pass_found=0;
    while(read(filefd,buff_out,32) > 0)
  {
    if(strstr(buff_out,name) != NULL)
      {
      name_found=1;
      if (read(filefd,buff_out,32) > 0)
        if (strstr(buff_out,password) != NULL)
          pass_found=1;
      }
  }
  if (name_found == 1 && pass_found == 0)
  {
    printf("Wrong Password! \n");
    return EXIT_FAILURE;
  }
  bzero(buff_out, LENGTH);
  close(filefd);

  struct sockaddr_in server_addr;

  /* Socket settings */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);


  // Connect to Server
    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1) {
    printf("ERROR: connect\n");
    return EXIT_FAILURE;
  }

  // Send name
  send(sockfd, name, 32, 0);

  // Send password
  send(sockfd, password, 32, 0);

  printf("=== WELCOME TO THE CHATROOM ===\n");

  pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
    printf("ERROR: pthread\n");
    return EXIT_FAILURE;
  }

  pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
    printf("ERROR: pthread\n");
    return EXIT_FAILURE;
  }

  while (1){
    if(flag){
      printf("\nBye\n");
      break;
      }
  }

  close(sockfd);

  return EXIT_SUCCESS;
}