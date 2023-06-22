#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

static char target_file[DIRSIZ];

void
find(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  char *p_name;
  switch(st.type){
  case T_FILE:

    // Find first character after last slash.
    for(p_name=path+strlen(path); p_name >= path && *p_name != '/'; p_name--)
      ;
    p_name++;
    if(strcmp(target_file,p_name) == 0) {
        printf("%s\n", path);
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      if(strcmp(".",de.name) == 0 || 
         strcmp("..",de.name) == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 3){
    strcpy(target_file, argv[1]);
    find(".");
    exit(0);
  }
  strcpy(target_file, argv[2]);
  find(argv[1]);
  exit(0);
}
