#define __USE_GNU
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>

// Compile With cc -shared ./haspdump.c -ldl -D_GNU_SOURCE -o haspdump.so

void *p = NULL;
static void* (*real_malloc)(size_t) = NULL;
static void * hasp_table;
int key_index = 0;
int num_keys = 0;

int *curptr;
int fsz;
int *lastptr;
void load_table() {
  // Load our requests into the table.
  FILE *fp;

  fp = fopen("request.bin","rb");
  if(!fp){
    printf("request.bin not found!\n");
    exit(1);
  }

  fseek(fp,0,SEEK_END);
  fsz = ftell(fp);
  fseek(fp,0,SEEK_SET);
  hasp_table = (unsigned char*)real_malloc(fsz);
  fread(hasp_table,fsz,1,fp);
  fclose(fp);
  num_keys = fsz / 64;
  FILE *fd;
  fd = fopen("response.bin","wb");
  fclose(fd);
}

static void __mtrace_init(void) {
  real_malloc = dlsym(RTLD_NEXT, "malloc");
  if (NULL == real_malloc) {
    fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    return;
  }
  load_table();
}

void *malloc(size_t size){
  if(real_malloc==NULL) {
    __mtrace_init();
  }
  void *p = NULL;
  p = real_malloc(size);

  if(size == 0x57) { // Magic Number
    lastptr = curptr;
    curptr = real_malloc(64);
    if (key_index == num_keys) {
      // Last key, so GTFO
      printf("\rKEY %d/%d",key_index,num_keys);
      FILE * fd;
      fd = fopen("response.bin","ab+");
      fseek(fd,0,SEEK_END);
      fwrite(lastptr,64,1,fd);
      fclose(fd);
      exit(0); // This will crash prime, but fuck it.
    } else {
      //  First Key
      if(lastptr != 0x00){
        FILE * fd;
        printf("\rKEY %d/%d",key_index,num_keys);
        fd = fopen("response.bin","ab+");
        fseek(fd,0,SEEK_END);
        fwrite(lastptr,64,1,fd);
        fclose(fd);
      }

      //  Load the next request key into the buffer.
      //  If last, don't load anything into the buffer.
      if(key_index != num_keys){
        memcpy(curptr, hasp_table + (key_index * 64), 64);
      }
      key_index ++;
    }
    p = curptr;
  }

  return p;
}