 ____       _                 ___  ____
|  _ \ _ __(_)_ __ ___   ___ / _ \/ ___|
| |_) | '__| | '_ ` _ \ / _ \ | | \___ \
|  __/| |  | | | | | | |  __/ |_| |___) |
|_|   |_|  |_|_| |_| |_|\___|\___/|____/


# Bootloader Stuff

*   `bootloader_noserial.exe`    (Modified Bootloader for No HDD Serial)
    *   --- Change **0x6B5** from **0x72** to **0x73** **(Not used now. Here only for record)**
    *   --- Change **0x6E6** from **0x72** to **0x73** **(Not used now. Here only for record)**
    *   NOP'ed **3** bytes from **0x6DE**
    *   NOP'ed **3** bytes from **0x6AD**
    *   --- Replace **0x2D3** with **0x33 0xc0 0x90** **(Not used now. Here only for record)**
*   `bootloader.exe`    (Original Bootloader at image offset 0x400 16384 bytes size)
*   `mbr_noserial.exe`   (Modified MBR for No HDD Serial)
    *   NOP'ed **11** bytes from **0x68**
*   `mbr.exe`  (Original MBR at image offset 0x0 512 bytes size)
*   `run.c`  (Fake /etc/init.d/run for unencrypted **p** **i** **u** **n** **x** files)
*   `old_nvidia.txt` (Old Nvidia Cards on PrimeOS that will load 173 version of the driver)
*   `old_nvidia.bin` (Old Nvidia Cards on PrimeOS that will load 173 version of the driver. Binary file with every Short Int (2 bytes LE) as PID)