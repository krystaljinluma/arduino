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

char msg[100];
extern pthread_mutex_t lock;
extern pthread_mutex_t lock_usb;
extern pthread_mutex_t lock_port;
extern char* usb;
extern int port;
/*
This code configures the file descriptor for use as a serial port.
*/
void configure(int fd) {
  struct termios pts;
  tcgetattr(fd, &pts);
  cfsetospeed(&pts, 9600);
  cfsetispeed(&pts, 9600);

  /*
  // You may need to un-comment these lines, depending on your platform.
  pts.c_cflag &= ~PARENB;
  pts.c_cflag &= ~CSTOPB;
  pts.c_cflag &= ~CSIZE;
  pts.c_cflag |= CS8;
  pts.c_cflag &= ~CRTSCTS;
  pts.c_cflag |= CLOCAL | CREAD;
  pts.c_iflag |= IGNPAR | IGNCR;
  pts.c_iflag &= ~(IXON | IXOFF | IXANY);
  pts.c_lflag |= ICANON;
  pts.c_oflag &= ~OPOST;
  */

  tcsetattr(fd, TCSANOW, &pts);

}

void* read_temp(void* arg) {
  printf("thread temp created\n");

  pthread_mutex_lock(&lock_usb);
  char* filename = malloc(sizeof(char)*strlen(usb) + 1);
  strcpy(filename, usb);
  pthread_mutex_unlock(&lock_usb);

  // try to open the file for reading and writing
  // you may need to change the flags depending on your platform
  int fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
  
  if (fd < 0) {
    perror("Could not open file\n");
    exit(1);
  }
  else {
    printf("Successfully opened %s for reading and writing\n", filename);
  }

  configure(fd);

  /*
    Write the rest of the program below, using the read and write system calls.
  */

  char buf;

  while (1) {
    int index = 0;
    int bytes_read = read(fd, &buf, 1);

    char temp[100];

    while (buf != '\n') {
      if (bytes_read > 0) {
        temp[index++] = buf;
      }
      bytes_read = read(fd, &buf, 1);
    }

    pthread_mutex_lock(&lock);
    strcpy(msg, temp);
    pthread_mutex_unlock(&lock);
    

  }

  close(fd);
  pthread_exit(NULL);
}

int send_data(char* name, int msg) {
  printf("in send data\n");
  char* filename = (char*) name; 

  // try to open the file for reading and writing
  // you may need to change the flags depending on your platform
  int fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
  
  if (fd < 0) {
    perror("Could not open file\n");
    return 1;
    //exit(1);

  }
  else {
    printf("Successfully opened %s for reading and writing\n", filename);
  }
  configure(fd);

  /*
    Write the rest of the program below, using the read and write system calls.
  */

  int x = write(fd, &msg, 1);

  if (x == -1) {
    printf("did not send\n");
  }

  close(fd);
  return 0;
}