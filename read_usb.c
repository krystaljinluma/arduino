#include "read_usb.h"

extern char msg[100];
extern float temperature[3600];
extern char* usb;
extern int port;
extern int connected;
extern int unit;
extern int quit;

extern pthread_mutex_t lock;
extern pthread_mutex_t lock_usb;
extern pthread_mutex_t lock_port;
extern pthread_mutex_t lock_usb;
extern pthread_mutex_t lock_quit;
extern pthread_mutex_t lock_unit;
extern pthread_mutex_t lock_connected;

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

/* Read_temp continously reads in termperatures from ardunio and stores the temperature in globals */
void* read_temp(void* arg) {
  pthread_mutex_lock(&lock_usb);
  char* filename = malloc(sizeof(char)*strlen(usb) + 1);
  strcpy(filename, usb);
  pthread_mutex_unlock(&lock_usb);

  int fd;
  char buf;
  int i = 0;
  
  while (1) {
    /* Check if 'q'has been entered. Exit thread if entered */
    pthread_mutex_lock(&lock_quit);
    int current_quit = quit;
    pthread_mutex_unlock(&lock_quit);
    if (current_quit == 1) {
      printf("Exit read_usb()\n");
      break;
    }

    /* Check if arduino has beeen disconnected */
    pthread_mutex_lock(&lock_connected);
    int current_connected = connected;
    pthread_mutex_unlock(&lock_connected);

    /* If ardunio has been disconnected, try to open file again*/
    if (current_connected == 0) {
      fd = open(filename, O_RDWR | O_NOCTTY);
      if (fd < 0) {
        //ardunio still disconnected, do nothing
        //perror("read_data: Could not open file\n");
      } else {
        current_connected = 1;
        pthread_mutex_lock(&lock_connected);
        connected = 1;
        pthread_mutex_unlock(&lock_connected);
        //reset globals
        pthread_mutex_lock(&lock_unit);
        unit = 'C';
        pthread_mutex_unlock(&lock_unit);
        configure(fd);
      }
    }

    //if arduino connected, proceed with reading from arduino
    if (current_connected == 1) {
      int index = 0;
      int bytes_read = read(fd, &buf, 1);

      char temp[100];

      //read in next byte until end of line or error
      while (buf != '\n' && bytes_read != -1) {
        if (bytes_read > 0) {
          temp[index++] = buf;
        }
        bytes_read = read(fd, &buf, 1);
      }

      //if successfully read, update temp values
      if (bytes_read != -1) {
        pthread_mutex_lock(&lock);
        strcpy(msg, temp);
        pthread_mutex_unlock(&lock);

        float temporary = convert_to_temperature(msg);
        temperature[i % 3600] = temporary;
        i++;

      // if not successfully read, indicate disconnected
      } else {
        pthread_mutex_lock(&lock_connected);
        connected = 0;
        pthread_mutex_unlock(&lock_connected);
      }
    }
  }  

  close(fd);
  pthread_exit(NULL);
}

/* Helper function to send data to ardunio to convey updates from browser */
int send_data(char* name, int msg) {
  /* Open file descriptor to send data. */
  int fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    //perror("send_data: Could not open file\n");
    return 1;
  }
  configure(fd);

  /* Write msg to send to Ardunio */
  write(fd, &msg, 1);

  close(fd);
  return 0;
}


/* Helper function to send temperature thresholds to change color of ardunio light
   Different from send_data function to send three bytes ('T' indicator, high temp, low temp)
   */
int send_threshold(char* name, int msg, int hot, int cold) {
  /* Open file descriptor */
  int fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd < 0) {
    //perror("send_data: Could not open file\n");
    return 1;
  }
  configure(fd);

  /* Write msg, hot temp, cold temp in succession to arduino */
  write(fd, &msg, 1);
  write(fd, &hot, 1);
  write(fd, &cold, 1);

  close(fd);
  return 0;
}

/* Helper function to convert temperatures read innto a float */
float convert_to_temperature(char* msg) {
  float temperature = atof(msg);
  temperature = (floorf(temperature * 100)) / 100;
  return temperature;
}