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
extern pthread_mutex_t lock_vals;

extern char msg[100];
extern float temperature[3600];

/* Start server function starts connection to browsers. Opens up a fd, listens, and accepts requests from browser */
void* start_server(void* arg)
{
  pthread_mutex_lock(&lock_port);
  int port_number = port;
  pthread_mutex_unlock(&lock_port);

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

    /* Detach thread when done handling request */
    pthread_detach(t);

    free(fd);

    /* Check if 'q' to quit has been selected */
    pthread_mutex_lock(&lock_quit);
    int current_quit = quit;
    pthread_mutex_unlock(&lock_quit);

    /* Exit thread if 'q' has been entered */
    if (current_quit == 1) {
      printf("Exit start_server()\n");
      break;
    }

  }

  // 8. close: close the socket
  close(sock);

  //printf("Server shutting down\n");
  pthread_exit(NULL);
} 

/* Helper function to help handle incoming requests from browsers. Depending on the request_uri, method determines
 what to send back to browser. Spawned from threads in start_server to handle multiple incoming requests. */
void* handle_request(void* fd_arg) {
  
  int fd = *(int*) fd_arg;

  // buffer to read data into
  char request[1024];
  request[0] = '\0';

  // 5. recv: read incoming message (request) into buffer
  int bytes_received = recv(fd,request,1024,0);

  // null-terminate the string
  request[bytes_received] = '\0';

  // get request type
  char* token = strtok(request, " ");
  char* request_type = malloc(sizeof(char)*(strlen(token)+1));
  strcpy(request_type, token);

  // get request uri
  token = strtok(NULL, " ");
  char* request_uri = malloc(sizeof(char)*(strlen(token)+1));
  strcpy(request_uri, token+1);

  // create http response with request uri
  char* reply = create_response(request_uri);

  // 6. send: send the outgoing message (response) over the socket
  send(fd, reply, strlen(reply), 0);

  // freee malloc'ed values
  free(reply);
  free(request_uri);
  free(request_type);

  // 7. close: close the connection
  close(fd);

  //printf("Server closed connection\n");
  pthread_exit(&fd);
}

/*helper function to create a response based off of the request_uri received from http request*/
char* create_response(char* request_uri) {
  char* reply;

  /* Save globals locally */ 
  pthread_mutex_lock(&lock_usb);
  char* usb_file = malloc(sizeof(char)*strlen(usb) + 1);
  strcpy(usb_file, usb);
  pthread_mutex_unlock(&lock_usb);

  pthread_mutex_lock(&lock);
  char* current_temp = malloc(sizeof(char)*strlen(msg) + 1);
  strcpy(current_temp, msg);
  pthread_mutex_unlock(&lock);

  float temp = atof(current_temp);
  calc();
  pthread_mutex_lock(&lock_vals);
  float current_max = max; 
  float current_min = min;
  float current_avg = avg;
  pthread_mutex_unlock(&lock_vals);

  pthread_mutex_lock(&lock_unit);
  char current_unit = unit;
  pthread_mutex_unlock(&lock_unit);

  /* Set header */
  char* reply_header = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";

  /* Determine next actions based on uri */
  if (strcmp(request_uri, "user.html") == 0){
    reply = read_file(request_uri, reply_header, 5000);
  } else if (strcmp(request_uri, "jquery.js") == 0) {
    reply = read_file(request_uri, reply_header, 150000);
  } else if (strcmp(request_uri, "userinput.js") == 0) {
    reply = read_file(request_uri, reply_header, 5000);
  } else if (strcmp(request_uri, "convert") == 0) {
    current_unit = convert_unit(current_unit);
    send_data(usb_file, current_unit);
    char* new_temp = format_content(temp, current_max, current_min, current_avg, current_unit);
    reply = create_json(new_temp);
    free(new_temp);
  } else if (strcmp(request_uri, "standby") == 0) {
    send_data(usb_file, 'S');
    reply = create_json("{\n\t\"display\" : \"standby\"\n}");
  } else if (strcmp(request_uri, "favicon.ico") == 0) {
    reply = malloc(sizeof(char)*(strlen(reply_header)+1));
    strcpy(reply, reply_header);
  } else if (strcmp(request_uri, "gettemp") == 0) {
    char* new_temp = format_content(temp, current_max, current_min, current_avg, current_unit);
    reply = create_json(new_temp);
    free(new_temp);
  } else if (strstr(request_uri, "threshold") != NULL) {  //checkstring
    reply = malloc(1);
    handleThreshold(request_uri, usb_file);
  } else if (strcmp(request_uri, "sendmsg") == 0) {
    reply = malloc(1);
    send_data(usb_file, 'M');
  } else {
    reply = malloc(1);
  }

  /* Free heap */
  free(current_temp);
  free(usb_file);

  return reply;
}

/* Helper function to hanlde changes to temp thresholds. Sends the temperature thresholds set by user
in browser and sends to ardunio */
void handleThreshold(char* request_uri, char* usb_port) {
  // parse query string to send arduino hot and cold thresholds
  char* token = strtok(request_uri, "?");
  token = strtok(NULL, "=");
  token = strtok(NULL, "&");
  int hot = atoi(token);
  token = strtok(NULL, "=");
  token = strtok(NULL, "&");
  int cold = atoi(token);

  // send bytes to arduino
  send_threshold(usb_port, 'T', hot, cold);
}

/* Helper function to convert between units based off browser information */
char convert_unit(char current_unit) {
  /* Determine unit to convert to */ 
  if (current_unit == 'C'){
    current_unit = 'F';
  }
  else{
    current_unit = 'C';
  }

  /* Update global variable */
  pthread_mutex_lock(&lock_unit);
  unit = current_unit;
  pthread_mutex_unlock(&lock_unit);

  return current_unit;
}

/* Helper function to calculate max, min, and avg temperatures */
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

/* Helper function to format temperatures to send to browser. If units are in F, convert temp to F*/
char* format_temp(float temperature, char current_unit) {
  char* result = malloc(sizeof(char) * 25);

  /* If current unit is F, convert temp to F*/
  if (current_unit == 'F'){
    temperature = temperature * 9 / 5 + 32;
  }

  /* Print temp as string with 2 decinal places */
  sprintf(result, "%.2f", temperature);

  /* Append proper unit measure */ 
  if (current_unit == 'C') strcat(result, " degrees C");
  else strcat(result, " degrees F");

  return result;
}

/* Helper function to format data fields of JSON to send to browser */
char* format_content(float current_temp, float current_max, float current_min, float current_avg, char current_unit) {
  char* result = malloc(sizeof(char) * 8000);
  result[0] = '\0';

  /* Get formatted temp values */
  char* string_temp = format_temp(current_temp, current_unit);
  char* string_max = format_temp(current_max, current_unit);
  char* string_min = format_temp(current_min, current_unit);
  char* string_avg = format_temp(current_avg, current_unit);
  
  /* Set up visualization formatting */
  float temp_arr[360];
  int i = 0;
  pthread_mutex_lock(&lock_arr);
  while (i < 360){
    temp_arr[i] = temperature[i];
    i++;
  }
  pthread_mutex_unlock(&lock_arr);
  char* visualization = malloc(sizeof(char) * 7200);
  visualization[0] = '\0';
  char* prefix = "{ y: ";
  char* t = malloc(sizeof(char) * 8);
  t[0] = '\0';

  strcat(visualization, "[ ");


  for (int i = 0; i < 360; i++){

    if (temp_arr[i] <= 40 && temp_arr[i] >= 10){
      if (i != 0) {
        strcat(visualization, ", ");

      }
      strcat(visualization, prefix);
      sprintf(t, "%.2f", temp_arr[i]);
        strcat(visualization, t);
        strcat(visualization, " }");
    }

  }
    strcat(visualization, "] ");

  /* Include display value */
  strcat(result, "{\n\t\"display\" : \"");

  /* Get connected status */
  pthread_mutex_lock(&lock_connected);
  int current_connected = connected;
  pthread_mutex_unlock(&lock_connected);

  /* Include all data in JSON if connected */
  if (current_connected == 1) {
    strcat(result, string_temp);
    strcat(result, "\",\n\t\"high\" : \"");
    strcat(result, string_max);
    strcat(result, "\",\n\t\"low\" : \"");
    strcat(result, string_min);
    strcat(result, "\",\n\t\"avg\" : \"");
    strcat(result, string_avg);
    strcat(result, "\",\n\t\"connected\" : \"1");
    strcat(result, "\",\n\t\"visualization\" : \""); 
    strcat(result, visualization);
    strcat(result, "\"\n}");
  } else {
    /* Only include connected status if disconnected */
    strcat(result, "disconnected\",\n\t\"connected\" : \"0\"\n}");
  }

  /* Free heap */
  free(visualization);
  free(t);
  free(string_temp);
  free(string_max);
  free(string_min);
  free(string_avg);

  return result;
}

/* Helper function to create a JSON object to send back to brower*/
char* create_json(char* json_content){
  char* reply;
  char* json_header = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
  reply = malloc(sizeof(char)*(strlen(json_header) + strlen(json_content)+ 1));
  strcpy(reply, json_header);
  strcat(reply, json_content);
  return reply;
}

/*helper function read file to a string to send back to browser */
char* read_file(char* filename, char* header, int size){
  char file_content[size];
  char file_line[size];

  /*Ensure temp variables are empty*/
  file_content[0] = '\0';
  file_line[0] = '\0';

  /* Read in file to content string */
  FILE* f;
  f = fopen(filename, "r");
  while (fgets(file_line, size, f)!= NULL){
    strcat(file_content, file_line);
  }

  /* Concat header and file content */
  char* reply = malloc(sizeof(char)*(strlen(header) + strlen(file_content) + 1));
  strcpy(reply, header);
  strcat(reply, file_content);

  return reply;
}

/* Helper function to read in q from user in terminal to terminate program. */
void* read_quit(void* arg) {
  while(1) {
    char input[100];
    /* If user inputs 'q' in terminal, update global quit */
    if (fgets(input, 100*sizeof(char), stdin)) {
      if (strcmp(input, "q\n") == 0) {
        pthread_mutex_lock(&lock_quit);
        quit = 1;
        pthread_mutex_unlock(&lock_quit);
        printf("Exit read_quit()\n");
        pthread_exit(NULL);
      }
    }
  }
}



