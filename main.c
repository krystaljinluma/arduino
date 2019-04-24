#include "read_usb.h"
#include "server.h"

/*Global Variables*/ 
char unit = 'C';
int port;
char* usb;
float max;
float min;
float avg;
int connected = 1;
int quit = 0;;
char msg[100];
float temperature[360];

/*Locks*/
pthread_mutex_t lock;
pthread_mutex_t lock_unit;
pthread_mutex_t lock_port;
pthread_mutex_t lock_usb;
pthread_mutex_t lock_arr;
pthread_mutex_t lock_quit;
pthread_mutex_t lock_connected;


/*Main Method*/
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
  if (pthread_mutex_init(&lock_arr, NULL) != 0) return 1;
  if (pthread_mutex_init(&lock_quit, NULL) != 0) return 1;
  if (pthread_mutex_init(&lock_connected, NULL) != 0) return 1;

  /* Initialize two threads */
  pthread_t t1;
  pthread_t t2;
  pthread_t t3;

  /* Create each thread; return 1 if error */
  if (pthread_create(&t1, NULL, &read_temp, NULL) != 0) return 1;
  if (pthread_create(&t2, NULL, &start_server, NULL) != 0) return 1;
  if (pthread_create(&t3, NULL, &read_quit, NULL) != 0) return 1;

  /* Join each thread; return 1 if error */
  if (pthread_join(t1, NULL) != 0) return 1;
  if (pthread_join(t2, NULL) != 0) return 1;
  if (pthread_join(t3, NULL) != 0) return 1;

  printf("exit main\n");
  free(usb);

  return 0;
}

