#include "read_usb.h"

extern char msg[100];
extern float temperature[360];
extern char* usb;
extern int port;
extern int connected;
extern int unit;
extern pthread_mutex_t lock;
extern pthread_mutex_t lock_usb;
extern pthread_mutex_t lock_port;
extern pthread_mutex_t lock_usb;

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

float convert_to_temperature(char* msg) {
  float temperature = atof(msg);
  temperature = (floorf(temperature * 100)) / 100;
  return temperature;
}

void* read_temp(void* arg) {
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
    //printf("Successfully opened %s for reading and writing\n", filename);
    sleep(5);
  }

  configure(fd);

  /*
    Write the rest of the program below, using the read and write system calls.
  */

  char buf;
  int i = 0;

  
  while (1) {
    if (connected == 0) {
      fd = open(filename, O_RDWR | O_NOCTTY);
      if (fd < 0) {
        perror("read_data: Could not open file\n");
      } else {
        connected = 1;
        unit = 'C';
        configure(fd);
        //printf("read_data: Successfully opened %s for reading and writing\n", filename);
      }
    }

    if (connected == 1) {
      int index = 0;
      int bytes_read = read(fd, &buf, 1);

      char temp[100];

      while (buf != '\n' && bytes_read != -1) {
        if (bytes_read > 0) {
          temp[index++] = buf;
        }
        bytes_read = read(fd, &buf, 1);
      }

      if (bytes_read != -1) {
        pthread_mutex_lock(&lock);
        strcpy(msg, temp);
        pthread_mutex_unlock(&lock);

        float temporary = convert_to_temperature(msg);
        temperature[i % 360] = temporary;
        i++;
        
        printf("%s\n", temp);
      } else {
        connected = 0;
      }
    }
  }  

  close(fd);
  pthread_exit(NULL);
}


int send_data(char* name, int msg) {
  char* filename = (char*) name; 

  // try to open the file for reading and writing
  // you may need to change the flags depending on your platform
  int fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
  
  if (fd < 0) {
    perror("send_data: Could not open file\n");
    connected = 0;
    return 1;
  }
  else {
    if (connected != 1) {
      connected = 1;
    }
    //printf("send_data: Successfully opened %s for reading and writing\n", filename);
  }
  configure(fd);

  /*
    Write the rest of the program below, using the read and write system calls.
  */

  int x = write(fd, &msg, 1);

  if (x == -1) {
    //printf("did not send\n");
  }

  close(fd);
  return 0;
}