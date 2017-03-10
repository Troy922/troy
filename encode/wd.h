#ifndef _WG_
#define _WG_
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/types.h>
#include<linux/watchdog.h>
extern int watchdog;
extern void InitWatchdog();
extern void Feeddog();

#endif
