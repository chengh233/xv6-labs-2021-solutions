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

  int p_val = 0;
  int n_val = 0;
  int p_read = 0;
  int p_write = 0;

  int p[2];

  pipe(p);

  if(fork() > 0){
    close(p[0]);
    int i;
    for(i = 2;i < 35;i++){
      write(p[1],(int*)&i,sizeof(int));
      sleep(1);
    }
    close(p[1]);
    wait((int*)0);
    exit(0);
  } else {
    close(p[1]);
    p_read = p[0];
    while (read(p_read,(int*)&n_val,sizeof(int))) {
      if(p_val == 0){
        printf("prime %d\n",n_val);
        p_val = n_val;
      } else {
        if(n_val % p_val != 0){
          if (!p_write) {
            int p_tmp[2];
            pipe(p_tmp);
            if (fork() > 0) {
              close(p_tmp[0]);
              p_write = p_tmp[1];
              write(p_write,(int*)&n_val,sizeof(int));
            } else {
              close(p_tmp[1]);
              p_read = p_tmp[0];
              p_val = 0;
            }
          } else {
            write(p_write,(int*)&n_val,sizeof(int));
          }
        }
      }
    }
    close(p_read);
    if(!p_write) {
      close(p_write);
      wait((int*)0);
    }
    exit(0);
  }
}
