#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

void configure(int fd);
int read_ard(char* filename);
float convert_to_temperature(char* msg);
void* read_temp(void* arg);
int send_data(char* name, int msg);