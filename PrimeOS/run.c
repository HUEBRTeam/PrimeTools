/**	 ____       _                 ___  ____    _   _ _   _ _____
	|  _ \ _ __(_)_ __ ___   ___ / _ \/ ___|  | | | | | | | ____|
	| |_) | '__| | '_ ` _ \ / _ \ | | \___ \  | |_| | | | |  _|
	|  __/| |  | | | | | | |  __/ |_| |___) | |  _  | |_| | |___
	|_|   |_|  |_|_| |_| |_|\___|\___/|____/  |_| |_|\___/|_____|

 	This is an alternative /etc/init.d/run for PrimeOS.
 	It will mount p, x and run the game

    X -> /dev/ram1 -> /usr/
    For /dev/ram2 we have this possibilites:
        -   If Nvidia 9 series or lower, we need to mount P that has nvidia 173 driver
        -   If Intel, we need to mount I that has Intel DRM Driver
        -   If Nvidia 210 or upper, we need to mount U that has nvidia 304 driver

    Uncomment DROP_TO_SHELL define for dropping into a Shell
/
 */
#define _XOPEN_SOURCE
#define _GNU_SOURCE
#define BLOCK_SIZE 256 * 1024
//#define DROP_TO_SHELL
//#define DEBUG_MODE
//#define BOOT_AFTER_LOAD
//#define CHANGETTY

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/hdreg.h>
#include <stdint.h>
#include <linux/vt.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pciaccess.h>
#include <dlfcn.h>


static int VideoMode = 2;       //  0 -> Nvidia 173, 1-> Intel, 2-> Nvidia 340

static char *env[]  =   {
    "LD_LIBRARY_PATH=/usr/slib:/lib:/usr/lib",
    "PATH=/usr/bin:/bin",
    "__GL_SYNC_TO_VBLANK=1",
    "force_s3tc_enable=true",
    NULL
};
extern unsigned char _binary_bin_old_nvidia_bin_start;
extern unsigned char _binary_bin_old_nvidia_bin_end;
extern unsigned char _binary_bin_old_nvidia_bin_size;

struct pci_device_iterator* (*h_pci_id_match_iterator_create)(const struct pci_id_match *) = NULL;
struct pci_device* (*h_pci_device_next)(struct pci_device_iterator *)  = NULL;
int (*h_pci_system_init) (void)  = NULL;

//
//  Checks for PID on old_nvidia list
//
int CheckOldCard(uint16_t pid)   {
    uint16_t *list = (uint16_t *)&_binary_bin_old_nvidia_bin_start;
    uint16_t *end_list = (uint16_t *)&_binary_bin_old_nvidia_bin_end;
    while(list < end_list)  {
        if(*list == pid)
            return 1;
        list += 1;
    }
    return 0;
}

//
//  Detects if we will use Intel, Nvidia 173 or Nvidia 340 driver
//
void DetectVideoMode()  {
    int gpufound = 0;
    printf("Loading libpciaccess\n");
    //  Need to be full path :(
    void *handle = dlopen("/usr/slib/libpciaccess.so.0",RTLD_LAZY);
    h_pci_id_match_iterator_create = dlsym(handle, "pci_id_match_iterator_create");
    h_pci_device_next = dlsym(handle, "pci_device_next");
    h_pci_system_init = dlsym(handle, "pci_system_init");
    //Sanity Check
    if(h_pci_system_init == NULL || h_pci_id_match_iterator_create == NULL || h_pci_device_next == NULL)    {
        printf("Error loading libpciaccess methods!\n");
        exit(1);
    }
    printf("Loaded.\n");
    int err = h_pci_system_init();
    if(err == 0)    {
        printf("PCI Subsystem initialized!\n");
        printf("Searching for NVIDIA Devices.\n");
        //  Nvidia VID = 0x10DE, 0x300 is the VideoCard Device Type, 0xFFFF0000 is the Mask for Checking Device Type
        struct pci_id_match match = {0x10de, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, 0x30000, 0xffff0000};

        struct pci_device_iterator* iter = h_pci_id_match_iterator_create(&match);
        struct pci_device* device;
        while(1) {
            if(iter == NULL)
                break;
            device = h_pci_device_next(iter);
            if(device == NULL)
                break;
            int oldcard = CheckOldCard(device->device_id);
            if(oldcard) {
                gpufound = 1;
                VideoMode = 0;
                printf("Found Nvidia Oldcard for 173 driver. %x:%x\n",device->vendor_id,device->device_id);
            }else{
                gpufound = 1;
                VideoMode = 2;
                printf("Found Nvidia Newcard for 340 driver. %x:%x\n",device->vendor_id,device->device_id);
            }
        }
        if(!gpufound)   {
            printf("Found Intel Card (or no card lol)\n");
            VideoMode = 1;
        }
    }else{
        printf("Error initializing PCI Subsystem\n");
        VideoMode = 0;
    }
}

void SetupEnvironment() {
    putenv("LD_LIBRARY_PATH=/usr/slib:/lib:/usr/lib");
    putenv("PATH=/usr/bin:/bin");
    putenv("__GL_SYNC_TO_VBLANK=1");
    putenv("force_s3tc_enable=true");
}

void ChangeToTTY2()	{
    #ifdef CHANGETTY
    int fd = open("/dev/tty",O_RDWR);
    if(fd >= 0) {
       if (ioctl(fd,VT_ACTIVATE,2))
          perror("chvt: VT_ACTIVATE");
       if (ioctl(fd,VT_WAITACTIVE,2))
          perror("VT_WAITACTIVE");
       close(fd);
    }else{
       printf("Cannot change to tty2!\n");
    }
    #endif
}

void LoadRAM(const char *file, const char *ramfile) {
    //  Buffer for reading data
    unsigned char buff[BLOCK_SIZE];
    #ifdef DEBUG_MODE
    printf("Loading %s to %s\n",file,ramfile);
    #endif
    FILE *fp = fopen(file,"rb");
    FILE *ram = fopen(ramfile,"wb");
    #ifdef DEBUG_MODE
    printf("FP: %u\nRAM: %u\n",(unsigned int)fp,(unsigned int)ram);
    #endif
    fseek (fp , 0 , SEEK_END);
    int size = ftell (fp);
    #ifdef DEBUG_MODE
    printf("Size: %u\n",size);
    #endif
    rewind (fp);

    int block = 0, read = 0;
    while(read < size)  {
        if(read+BLOCK_SIZE > size)
            block = size - read;
        else
            block = BLOCK_SIZE;

        fread (buff, 1, BLOCK_SIZE, fp);
        fwrite(buff, 1, BLOCK_SIZE, ram);
        read += BLOCK_SIZE;
    }

    printf("%s loaded at %s\n",file,ramfile);

    fclose(fp);
    fclose(ram);
}

void execute(char* argv[]) {
     pid_t  pid;
     int    status;

     if ((pid = fork()) < 0) {     /* fork a child process           */
          printf("*** ERROR: forking child process failed\n");
          exit(1);
     }
     else if (pid == 0) {          /* for the child process:         */
          if (execvpe(argv[0], argv, env) < 0) {     /* execute the command  */
               printf("*** ERROR: exec failed\n");
               exit(1);
          }
     }
     else {                                  /* for the parent:      */
          while (wait(&status) != pid);       /* wait for completion  */
     }
}

void AMixerRoutine()    {
    char *amixer1[] = {"/usr/bin/amixer", "set", "Master", "80%", "unmute", NULL};
    char *amixer2[] = {"/usr/bin/amixer", "set", "PCM"   , "75%", "unmute", NULL};
    char *amixer3[] = {"/usr/bin/amixer", "set", "Front" , "90%", "unmute", NULL};
    printf("amixer set Master 80%% unmute\n");
    execute(amixer1);
    printf("amixer set PCM    75%% unmute\n");
    execute(amixer2);
    printf("amixer set Front  90%% unmute\n");
    execute(amixer3);
}

void ModulesRoutine()   {
    char *atkbd[]  = {"/usr/bin/busybox", "insmod", "/usr/slib/modules/atkbd.ko", NULL};
    char *ehci[]   = {"/usr/bin/busybox", "insmod", "/usr/slib/modules/ehci-hcd.ko", NULL};
    char *ohci[]   = {"/usr/bin/busybox", "insmod", "/usr/slib/modules/ohci-hcd.ko", NULL};
    char *xhci[]   = {"/usr/bin/busybox", "insmod", "/usr/slib/modules/xhci-hcd.ko", NULL};
    char *nvidia[] = {"/usr/bin/busybox", "insmod", "/usr/lib/nvidia.ko", NULL};

    printf("Inserting Keyboard Module.\n");
    execute(atkbd);

    printf("Inserting USB Modules\n");
    execute(ehci);
    execute(ohci);
    execute(xhci);

    switch(VideoMode)   {
        case 0: printf("Inserting nvidia module.\n"); execute(nvidia);  break; //  Nvidia 173
        case 1: break;
        case 2: printf("Inserting nvidia module.\n"); execute(nvidia);  break; //  Nvidia 304
    }
}

void NetworkRoutine()   {
    char *eth0up[]    =   {"/usr/bin/busybox", "ifconfig", "eth0", "up", NULL};
    char *udhcpc[]    =   {"/usr/bin/busybox", "udhcpc", "-b", NULL};

    printf("Bringing up eth0\n");
    execute(eth0up);
    printf("Starting udhcpc\n");
    execute(udhcpc);
}


#ifndef DROP_TO_SHELL
    //char *piuargs[] = {"/piu", "/mnt/hd/game/", NULL};
    //char *piuargs[] = {"/usr/bin/xinit","/prime","/mnt/hd/game/","--","-modulepath","/usr/slib/xorg/modules","-config","/usr/etc/X11/xorg.conf","-configdir","/usr/etc/X11/xorg.conf.d","-br","-quiet","-logverbose","0","-verbose","0","-depth","24","-audit","0","-bs","-tst","-xinerama", NULL};
    char *piuargs[] = {"/usr/bin/busybox","sh","/loadgame", NULL};
#else
    char *piuargs[] = {"/usr/bin/busybox", "sh", NULL};
#endif

char *bootargs[]    = {"/bin/busybox", "reboot", NULL};
char *selfrun[]     = {"/etc/init.d/run", NULL};
char *e2fsck[]      = {"/usr/bin/e2fsck", "/dev/sda2", "-y", NULL};

int main(int argc, char* args[])    {
    ChangeToTTY2();
    printf("Starting FakeRUN\n");
    printf("Setting up environment\n");
    SetupEnvironment();
    #ifdef BOOT_AFTER_LOAD
    execvp(bootargs[0], bootargs);
    while(1);
    #endif
    ChangeToTTY2();
    printf("Mouting /mnt/hd\n");
    int mountres = mount("/dev/sda1", "/mnt/hd",      "ext2", 0, NULL);
    if(mountres != 0)	{
       if(errno != EBUSY)  {
          printf("Error mouting /mnt/hd!\n");
          return 1;
       }else{
          printf("Already mounted. Skipping\n");
       }
    }

    printf("Loading Ramdisks\n");

    LoadRAM("/mnt/hd/x", "/dev/ram1");
    printf("Mounting /usr\n");
    mountres = mount("/dev/ram1", "/usr",      "cramfs", 0, NULL);
    if(mountres != 0 && errno != EBUSY) {
       printf("Failed mouting /dev/ram3 on /usr: %d\n",errno);
       return 1;
    }else if(mountres == 0 && errno == 0) {
        /**
            This is needed because of a bug. If we dont re-run, the mounts isnt showed on the environment.
        **/
        printf("Mounted. Re-running.\n");
        execvpe(selfrun[0], selfrun, env);
    }

    DetectVideoMode();

    switch(VideoMode)   {
        case 0: LoadRAM("/mnt/hd/p", "/dev/ram2");  break; //  Nvidia 173
        case 1: LoadRAM("/mnt/hd/i", "/dev/ram2");  break; //  Intel
        case 2: LoadRAM("/mnt/hd/u", "/dev/ram2");  break; //  Nvidia 304
        default: printf("Unknown Videomode %d!\n",VideoMode); return 1;
    }
    printf("Mounting /usr/lib\n");

    mountres = mount("/dev/ram2", "/usr/lib",  "cramfs", 0, NULL);
    if(mountres != 0 && errno != EBUSY) {
       printf("Failed mouting /dev/ram2 on /usr/lib: %d\n",errno);
       return 1;
    }

    //  Reload /dev/ram2
    //  Again, this should not be needed. Its a bug.
    switch(VideoMode)   {
        case 0: LoadRAM("/mnt/hd/p", "/dev/ram2");  break; //  Nvidia 173
        case 1: LoadRAM("/mnt/hd/i", "/dev/ram2");  break; //  Intel
        case 2: LoadRAM("/mnt/hd/u", "/dev/ram2");  break; //  Nvidia 304
        default: printf("Unknown Videomode %d!\n",VideoMode); return 1;
    }
    printf("Executing e2fsck.\n");
    execute(e2fsck);

    printf("Mouting /SETTINGS\n");
    mountres = mount("/dev/sda2", "/SETTINGS",      "ext2", 0, NULL);
    if(mountres != 0)   {
       if(errno != EBUSY)  {
          printf("Error mouting /SETTINGS: %d\n",errno);
          return 1;
       }else{
          printf("Already mounted. Skipping\n");
       }
    }

    ChangeToTTY2();

    printf("Inserting Modules\n");
    ModulesRoutine();
    printf("Setting Amixer\n");
    AMixerRoutine();
    printf("Network routines\n");
    NetworkRoutine();
    #ifdef DROP_TO_SHELL
    printf("Dropping to shell\n");
    sleep(5);
    #else
    printf("Executing game\n");
    #endif
    //  Call the /loadgame or /usr/bin/busybox sh
    execvpe(piuargs[0], piuargs, env);
    printf("Fail to load %s\n",piuargs[0]);
    return 1;
}
