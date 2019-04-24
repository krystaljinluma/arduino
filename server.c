#include "server.h"

/* 
This code primarily comes from 
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */

/*globals*/
extern char unit;
extern int port;
extern char* usb;
extern float max;
extern float min;
extern float avg;
extern int connected;
extern int quit;

extern pthread_mutex_t lock;
extern pthread_mutex_t lock_usb;
extern pthread_mutex_t lock_port;
extern pthread_mutex_t lock_arr;
extern pthread_mutex_t lock_unit;
extern pthread_mutex_t lock_quit;
extern pthread_mutex_t lock_connected;

extern char msg[100];
extern float temperature[360];

void* handle_request(void* fd_arg) {
  
  int fd = *(int*) fd_arg;

  // buffer to read data into
  char request[1024];
  request[0] = '\0';

  // 5. recv: read incoming message (request) into buffer
  int bytes_received = recv(fd,request,1024,0);

  // null-terminate the string
  request[bytes_received] = '\0';
  // print it to standard out
  //printf("This is the incoming request:\n%s\n", request);

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
  //printf("Server closed connection\n");
  pthread_exit(&fd);
}

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
  //printf("\nServer configured to listen on port %d\n", port_number);
  fflush(stdout);

  // 4. accept: wait here until we get a connection on that port
  int sin_size = sizeof(struct sockaddr_in);
  
  //while true and keep accepting new connection and then have a quit button in browser
  while (1) {
    int* fd = malloc(sizeof(int));
    *fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);

    //printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

    // start thread to pass in fd; pthread
    pthread_t t;
    
    /* Create each thread; exit thread, if error */
    pthread_create(&t, NULL, &handle_request, fd);

    pthread_detach(t);

    free(fd);


    pthread_mutex_lock(&lock_quit);
    int current_quit = quit;
    pthread_mutex_unlock(&lock_quit);

    if (current_quit == 1) {
      printf("quit start_server\n");
      break;
    }
  }

  // 8. close: close the socket
  close(sock);
  //printf("Server shutting down\n");

  pthread_exit(NULL);
} 


/*helper function to create a response based off of the request_uri received from http request*/
char* create_response(char* request_uri) {
  char* reply;

  pthread_mutex_lock(&lock);
  char* current_temp = malloc(sizeof(char)*strlen(msg) + 1);
  strcpy(current_temp, msg);
  pthread_mutex_unlock(&lock);

  calc();
  float temp = atof(current_temp);
  float current_max = max; 
  float current_min = min;
  float current_avg = avg;

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
    send_data(usb, current_unit);
    char* new_temp = format_content(temp, current_max, current_min, current_avg, current_unit);
    reply = create_json(new_temp);
    free(new_temp);
  } else if (strcmp(request_uri, "standby") == 0) {
    send_data(usb, 'S');
    reply = create_json("{\n\t\"display\" : \"standby\"\n}");
  } else if (strcmp(request_uri, "favicon.ico") == 0) {
    reply = malloc(sizeof(char)*(strlen(reply_header)+1));
    strcpy(reply, reply_header);
  } else if (strcmp(request_uri, "gettemp") == 0) {
    char* new_temp = format_content(temp, current_max, current_min, current_avg, current_unit);
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

void calc() {
  float temp_arr[360];
  int i = 0;
  pthread_mutex_lock(&lock_arr);
  while (i < 360){
    temp_arr[i] = temperature[i];
    i++;
  }
  pthread_mutex_unlock(&lock_arr);
  i = 0;
  max = -1000.0; min = 1000.0; avg = 0; 
  int count = 0;
  float total = 0;
  while (i < 360){
    if (temp_arr[i] != 0){
      count++;
      if (temp_arr[i] > max)
        max = temp_arr[i];
      if (temp_arr[i] < min)
        min = temp_arr[i];
      total += temp_arr[i];
    }
    i++;
  }
  avg = total / count;
}

char* format_temp(float temperature, char current_unit) {
  char* result = malloc(sizeof(char) * 25);

  if (current_unit == 'F'){
    temperature = temperature * 9 / 5 + 32;
  }

  sprintf(result, "%.2f", temperature);

  if (current_unit == 'C') strcat(result, " degrees C");
  else strcat(result, " degrees F");

  return result;
}

char* format_content(float current_temp, float current_max, float current_min, float current_avg, char current_unit) {
  char* result = malloc(sizeof(char) * 150);
  char* string_temp = format_temp(current_temp, current_unit);
  char* string_max = format_temp(current_max, current_unit);
  char* string_min = format_temp(current_min, current_unit);
  char* string_avg = format_temp(current_avg, current_unit);

  strcat(result, "{\n\t\"display\" : \"");

  pthread_mutex_lock(&lock_connected);
  int current_connected = connected;
  pthread_mutex_unlock(&lock_connected);

  if (current_connected == 1) {
    strcat(result, string_temp);
    strcat(result, "\",\n\t\"high\" : \"");
    strcat(result, string_max);
    strcat(result, "\",\n\t\"low\" : \"");
    strcat(result, string_min);
    strcat(result, "\",\n\t\"avg\" : \"");
    strcat(result, string_avg);
    strcat(result, "\",\n\t\"connected\" : \"1");
    strcat(result, "\"\n}");
  } else {
    strcat(result, "disconnected\",\n\t\"connected\" : \"0\"\n}");
  }


  free(string_temp);
  free(string_max);
  free(string_min);
  free(string_avg);
  return result;
}


/*helper function read file to a string*/
char* create_json(char* json_content){
  char* reply;
  char* json_header = "HTTP/1.1 200 OK\nContent-Type: application/json\n\n";
  reply = malloc(sizeof(char)*(strlen(json_header) + strlen(json_content)+ 1));
  strcpy(reply, json_header);
  strcat(reply, json_content);
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

void* read_quit(void* arg) {
  while(1) {
    char input[100];
    if (fgets(input, 100*sizeof(char), stdin)) {
      if (strcmp(input, "q\n") == 0) {
        pthread_mutex_lock(&lock_quit);
        quit = 1;
        pthread_mutex_unlock(&lock_quit);
        printf("exit read_quit\n");
        pthread_exit(NULL);
      }
    }
  }
}



