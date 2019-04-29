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
#include "read_usb.h"

void* start_server(void* arg);
char* read_file(char* filename, char* header, int size);
char* create_response(char* request_uri);
char* create_json(char* json_content);
char* format_temp(float current_temp, char current_unit);
char convert_unit(char current_unit);
char* format_content(float current_temp, float current_max, float current_min, float current_avg, char current_unit);
void calc();
void* handle_request(void* fd_arg);
void* read_quit(void* arg);
void handleThreshold(char* request_uri, char* usb_port);