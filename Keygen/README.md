     ____       _                  _  __
    |  _ \ _ __(_)_ __ ___   ___  | |/ /___ _   _  __ _  ___ _ __
    | |_) | '__| | '_ ` _ \ / _ \ | ' // _ \ | | |/ _` |/ _ \ '_ \ 
    |  __/| |  | | | | | | |  __/ | . \  __/ |_| | (_| |  __/ | | |
    |_|   |_|  |_|_| |_| |_|\___| |_|\_\___|\__, |\__, |\___|_| |_|
                                            |___/ |___/            

This is the first version of PIU Prime KeyGenerator based on Fiesta 2 Key Generator

*   `haspdump.c`  (Key Generator Injection Library SourceCode)
*   `haspdump.so`  (Compiled Key Generator Injection Library)
*   `primegen`  (PIU Prime 1.00.0 modified exec to work with injection lib)
*   `primegen.txt`  (Modifications to an Original 1.00.0 exec to work with injection lib)
*   `primekeyconvert.py`  (Convert text **prime_keys.txt** to **hasp.bin**)
*   `primekeygen.py`  (Generates the keys using a list file)

For compiling **haspdump** library use:
    gcc -shared ./haspdump.c -ldl -D_GNU_SOURCE -o haspdump.so

For generating the keys, create a file with folders described like that:

    CONFIG
    D
    USER_INTERFACE
    VIDEO
    WAVE
    CONFIG/LANG
    CONFIG/MISSION
    CONFIG/SFX
    CONFIG/LANG/CN
    CONFIG/LANG/EN

Then call python `primekeygen.py dirlistfile`. It will do everything for you. Keep in mind that **.amf** file should be on same folder.
