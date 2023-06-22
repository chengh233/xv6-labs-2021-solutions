#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

  if(argc > 1){
    fprintf(2, "Usage: pingpong\n");
    exit(1);
  }

  int p1[2];
  int p2[2];
  
  pipe(p1);
  pipe(p2);

  char rx_byte, tx_byte;

  if(fork() == 0){
    close(p1[1]);
    close(p2[0]);
    tx_byte = 'b';

    read(p1[0], (char*)&rx_byte, 1); 
    fprintf(1, "%d: received ping\n", getpid());
    close(p1[0]);

    write(p2[1], (char*)&tx_byte, 1);
    close(p2[1]);
    exit(0);
  } else {
    close(p1[0]);
    close(p2[1]);
    tx_byte = 'a';

    write(p1[1], (char*)&tx_byte, 1);
    close(p1[1]);

    read(p2[0], (char*)&rx_byte, 1);
    close(p2[0]);
    fprintf(1, "%d: received pong\n", getpid());
    wait((int*) 0);
    exit(0);
  }
}
