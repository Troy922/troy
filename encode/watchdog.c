#include "wd.h" 

int timeout = 100;
int watchdog;
void InitWatchdog(){
    watchdog = open("/dev/watchdog",O_WRONLY);
    if(watchdog == -1){
        fprintf(stderr,"Watchdog device not enabled.\n");
        fflush(stderr);
        exit(-1);
    }
    
    ioctl(watchdog,WDIOC_SETTIMEOUT,&timeout);
    ioctl(watchdog, WDIOC_SETOPTIONS,WDIOS_ENABLECARD);
}

void Feeddog(){
    int dymmy;
    ioctl(watchdog, WDIOC_KEEPALIVE, &dymmy);
}


