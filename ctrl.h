/*
 * ctrl.h
 *
 * This module handles the definitions and declarations for 
 * Remote Control commands and Processes Key Actions for DM365 platform.
 *
 * Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

#ifndef _CTRL_H
#define _CTRL_H

#include <xdc/std.h>

#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Rendezvous.h>

#include "ui.h"
#include "wd.h"

/* Defining infinite time */
#define FOREVER         -1
/*GPIO*/
#define GPIO_INPUT          0
#define GPIO_OUTPUT         1
#define GPIO_READ           2
#define GPIO_WRITE          3

/* Environment passed when creating the thread */
typedef struct CtrlEnv {
    UI_Handle               hUI;
    Rendezvous_Handle       hRendezvousInit;
    Rendezvous_Handle       hRendezvousCleanup;
    Pause_Handle            hPauseProcess;
    Int                     keyboard;
    Int                     time;
    Char                   *engineName;
    Bool                    osd;
} CtrlEnv;

extern Int uart_port;
extern Int gpio_num[32];
extern Int gpio_total;
extern Bool gpio_flag;
extern Int gpio;
extern Int mask;
/* Thread function prototype */
extern Void *ctrlThrFxn(Void *arg);

extern Void request_send(Int fd);   //dm368 is ready to receive bits.
extern Void clear_send(int fd);   //dm368 is ready to send bits.
extern void close_port(int fd);
extern int set_parity(int fd, int databits, int stopbits, int parity);
extern void set_speed(int fd, int speed);
extern int check_port_open(const char *dev,unsigned baud);
extern int open_gpio(const char *dev);
extern void UART_sendChar(char uart_port);
#endif /* _CTRL_H */
