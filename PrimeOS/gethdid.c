#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/hdreg.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(int argc, char **argv) {
  uint8_t key[128];
  uint8_t idbuf[512];

  int fd;
  FILE *fe;
  int retval;

  fd = open(argv[1], O_RDWR | O_LARGEFILE);
  if (fd == -1) {
    perror("open");
    printf("USAGE - ./gethdid /dev/sdX\n");
    return EXIT_FAILURE;
  }

  retval = ioctl(fd, HDIO_GET_IDENTITY, idbuf);

  if (retval != 0) {
    perror("ioctl");
    fprintf(stderr,"\nCant Get Info. Make Sure you're running this on PATA or SATA\n");
    exit(EXIT_FAILURE);
  }
  fe = fopen("./HDDRAW","wr+");
  fwrite(idbuf, 512,1, fe);
  fclose(fe);

  fe = fopen("./HDDKEY","wr+");
  memcpy(key, idbuf + 20, 20);
  memcpy(key + 20, idbuf + 46, 8);
  memcpy(key + 28, idbuf + 54, 36);
  fwrite(key, 64, 1, fe);
  fclose(fe);
  printf("HDDKEY File Created...\n");

  return EXIT_SUCCESS;
}

