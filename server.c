/* 
This code primarily comes from 
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>

// #include "read_usb.h"

char* read_file(char* filename, char* header, int size);
char* create_response(char* request_uri);
char* create_json(char* json_content);
char* format_temp(char* current_temp, char current_unit);
char convert_unit(char current_unit);

char unit = 'C';
int port;
char* usb;


pthread_mutex_t lock;
pthread_mutex_t lock_unit;
pthread_mutex_t lock_port;
pthread_mutex_t lock_usb;

extern char msg[100];
extern void* read_temp(void* filename);
extern int send_data(char* name, int msg);



void* start_server(void* arg)
{
  pthread_mutex_lock(&lock_port);
  int port_number = port;
  pthread_mutex_unlock(&lock_port);

  pthread_mutex_lock(&lock_usb);
  char* usb_file = malloc(sizeof(char)*strlen(usb) + 1);
  strcpy(usb_file, usb);
  pthread_mutex_unlock(&lock_usb);

  // structs to represent the server and client
  struct sockaddr_in server_addr,client_addr;    

  int sock; // socket descriptor

  // 1. socket: creates a socket descriptor that you later use to make other system calls
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
  int temp;
  if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
    perror("Setsockopt");
    exit(1);
  }

  // configure the server
  server_addr.sin_port = htons(port_number); // specify port number
  server_addr.sin_family = AF_INET;         
  server_addr.sin_addr.s_addr = INADDR_ANY; 
  bzero(&(server_addr.sin_zero),8); 

  // 2. bind: use the socket and associate it with the port number
  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("Unable to bind");
    exit(1);
  }

  // 3. listen: indicates that we want to listen to the port to which we bound; second arg is number of allowed connections
  if (listen(sock, 1) == -1) {
    perror("Listen");
    exit(1);
  }
      
  // once you get here, the server is set up and about to start listening
  printf("\nServer configured to listen on port %d\n", port_number);
  fflush(stdout);

  // 4. accept: wait here until we get a connection on that port
  int sin_size = sizeof(struct sockaddr_in);
  
  //while true and keep accepting new connection and then have a quit button in browser
  while (1) {
    int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
    printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    // buffer to read data into
    char request[1024];
    request[0] = '\0';

    // 5. recv: read incoming message (request) into buffer
  	int bytes_received = recv(fd,request,1024,0);
  	// null-terminate the string
  	request[bytes_received] = '\0';
  	// print it to standard out
  	printf("This is the incoming request:\n%s\n", request);

    char request_cpy[1024];
    request_cpy[0] = '\0';
    strcpy(request_cpy, request);

    char* token = strtok(request_cpy, " ");
    char* request_type = malloc(sizeof(char)*(strlen(token)+1));
    strcpy(request_type, token);

    token = strtok(NULL, " ");
    char* request_uri = malloc(sizeof(char)*(strlen(token)+1));
    strcpy(request_uri, token+1);

    char* reply = create_response(request_uri);

  	// 6. send: send the outgoing message (response) over the socket
  	// note that the second argument is a char*, and the third is the number of chars	
  	send(fd, reply, strlen(reply), 0);

    free(reply);
    free(request_uri);
    free(request_type);

  	// 7. close: close the connection
  	close(fd);
  	printf("Server closed connection\n");
  }

  // 8. close: close the socket
  close(sock);
  printf("Server shutting down\n");

  pthread_exit(NULL);
} 

/*helper function to create a response based off of the request_uri received from http request*/
char* create_response(char* request_uri) {
  char* reply;

  pthread_mutex_lock(&lock);
  char* current_temp = malloc(sizeof(char)*strlen(msg) + 1);
  strcpy(current_temp, msg);
  pthread_mutex_unlock(&lock);

  pthread_mutex_lock(&lock_unit);
  char current_unit = unit;
  pthread_mutex_unlock(&lock_unit);

  char* reply_header = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";

  if (strcmp(request_uri, "user.html") == 0){
    reply = read_file(request_uri, reply_header, 5000);
  } else if (strcmp(request_uri, "jquery.js") == 0) {
    reply = read_file(request_uri, reply_header, 150000);
  } else if (strcmp(request_uri, "userinput.js") == 0) {
    reply = read_file(request_uri, reply_header, 5000);
  } else if (strcmp(request_uri, "convert") == 0) {
    current_unit = convert_unit(current_unit);
    if (send_data(usb, current_unit) != 0) {
      printf("hello");
      char* message = "Cannot get reading from sensor";
      reply = create_json(message);
    } else {
      char* new_temp = format_temp(current_temp, current_unit);
      reply = create_json(new_temp);
      free(new_temp);
    }
  } else if (strcmp(request_uri, "standby") == 0) {
    if (send_data(usb, 'S') != 0) {
      char* message = "Cannot get reading from sensor";
      reply = create_json(message);
    } else {
      reply = create_json(current_temp);
    }
  } else if (strcmp(request_uri, "favicon.ico") == 0) {
    reply = malloc(sizeof(char)*(strlen(reply_header)+1));
    strcpy(reply, reply_header);
  } else if (strcmp(request_uri, "gettemp") == 0) {
    char* new_temp = format_temp(current_temp, current_unit);
    reply = create_json(new_temp);
    free(new_temp);
  } else {
    reply = malloc(1);
  }
  free(current_temp);
  return reply;
}

char convert_unit(char current_unit) {
  if (current_unit == 'C'){
    current_unit = 'F';
  }
  else{
    current_unit = 'C';
  }

  pthread_mutex_lock(&lock_unit);
  unit = current_unit;
  pthread_mutex_unlock(&lock_unit);

  return current_unit;
}

char* format_temp(char* current_temp, char current_unit) {
  char* result = malloc(sizeof(char) * 20);

  float temperature = atof(current_temp);

  //Every time a new request comes in the temperature is always in C, need to differentiate units from HTTP request
  if (current_unit == 'F'){
    temperature = temperature * 9 / 5 + 32;
  }

  sprintf(result, "%.2f", temperature);

  if (current_unit == 'C') strcat(result, " degrees C");
  else strcat(result, " degrees F");

  return result;
}

/*helper function read file to a string*/
char* create_json(char* json_content){
  char* reply;
  char* json_header = "HTTP/1.1 200 OK\nContent-Type: application/json\n\n";
  char* json_start = "{\n\t\"display\" : \"";
  char* json_end = "\"\n}";
  reply = malloc(sizeof(char)*(strlen(json_header) + strlen(json_content) + strlen(json_start) + strlen(json_end) + 1));
  strcpy(reply, json_header);
  strcat(reply, json_start);
  strcat(reply, json_content);
  strcat(reply, json_end);

  return reply;
}

/*helper function read file to a string*/
char* read_file(char* filename, char* header, int size){
  char file_content[size];
  char file_line[size];

  //setting both temp strings to null; not clearing for some reason
  file_content[0] = '\0';
  file_line[0] = '\0';

  FILE* f;
  f = fopen(filename, "r");
  while (fgets(file_line, size, f)!= NULL){
    strcat(file_content, file_line);
  }

  char* reply = malloc(sizeof(char)*(strlen(header) + strlen(file_content) + 1));
  strcpy(reply, header);
  strcat(reply, file_content);

  return reply;
}



int main(int argc, char *argv[])
{
  // check the number of arguments

  //a[1] is port number, a[2] is usb port
  if (argc != 3) {
      printf("\nUsage: %s [port_number] [usb_number]\n", argv[0]);
      exit(-1);
  }

  port = atoi(argv[1]);
  if (port <= 1024) {
    printf("\nPlease specify a port number greater than 1024\n");
    exit(-1);
  }

  usb = malloc(sizeof(char)*(strlen(argv[2])+1));
  strcpy(usb, argv[2]);

  if (pthread_mutex_init(&lock, NULL) != 0) return 1;
  if (pthread_mutex_init(&lock_port, NULL) != 0) return 1;
  if (pthread_mutex_init(&lock_usb, NULL) != 0) return 1;
  if (pthread_mutex_init(&lock_unit, NULL) != 0) return 1;

  /* Initialize two threads */
  pthread_t t1;
  pthread_t t2;

  /* Create each thread; return 1 if error */
  if (pthread_create(&t1, NULL, &read_temp, NULL) != 0) return 1;
  if (pthread_create(&t2, NULL, &start_server, NULL) != 0) return 1;
  
  /* Join each thread; return 1 if error */
  if (pthread_join(t1, NULL) != 0) return 1;
  if (pthread_join(t2, NULL) != 0) return 1;

  free(usb);

  return 0;
}

