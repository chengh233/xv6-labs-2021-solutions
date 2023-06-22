#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#include "kernel/fs.h"

static char* args[MAXARG];

static void
runCommand (const int argc)
{
  char input[100];
  int idx = 0;;
  char ch;
  while(read(0, (char*)&ch, sizeof(char))) {
      if(ch != '\n') {
          input[idx++] = ch;
          continue;
      } else {
          input[idx] = '\0';
          if(fork() > 0) {
              args[argc] = input; 
              args[argc+1] = 0;
              exec(args[0], args);
              exit(0);
          } else {
              wait((int*)0);
          }
          idx = 0;
      }
  }
}

int
main(int argc, char *argv[])
{
  int i = 0;
  for(i; i < argc-1; i++){
     args[i] = argv[i+1]; 
  }
  runCommand(argc-1);
  exit(0);
}
