/*
 * ctrl.c
 *
 * This module handles the Remote Control commands and Processes Key Actions
 * for DM365 platform.
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <xdc/std.h>

#include <ti/sdo/ce/Engine.h>

#include <ti/sdo/dmai/Cpu.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Rendezvous.h>

#include "ctrl.h"
#include "demo.h"
#include "ui.h"
#include "protocol.h"
#include "TLV5616.h"
#include "process.h"

/* How often to poll for new keyboard commands */
#define CONTROLLATENCY 600000

/* Keyboard command prompt */
#define COMMAND_PROMPT       "Command [ 'help' for usage ] > "

/* Maximum length of a keyboard command */
#define MAX_CMD_LENGTH       80

#define delay()	usleep(100)
/* Structure containing the state of the OSD */
typedef struct OsdData {
    Int           time;
    ULong         firstTime;
    ULong         prevTime;
    Int           samplingFrequency;
    Int           imageWidth;
    Int           imageHeight;
} OsdData;

/* Initial values of osd data structure */
#define OSD_DATA_INIT        { -1, 0, 0, 0, 0, 0 }

pthread_mutex_t mutex_dcs = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_video = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_capture = PTHREAD_COND_INITIALIZER; 
int status_thread =THREAD_RUN;
/******************************************************************************
 * drawDynamicData
 ******************************************************************************/
static Void drawDynamicData(Engine_Handle hEngine, Cpu_Handle hCpu,
                            UI_Handle hUI, OsdData *op)
{
    Char                  tmpString[20];
    struct timeval        tv;
    struct tm            *timePassed;
    time_t                spentTime;
    ULong                 newTime;
    ULong                 deltaTime;
    Int                   armLoad;
    Int                   fps;
    Int                   videoKbps;
    Float                 fpsf;
    Float                 videoKbpsf;
    Int                   imageWidth;
    Int                   imageHeight;


    op->time = -1;

    if (gettimeofday(&tv, NULL) == -1) {
        ERR("Failed to get os time\n");
        return;
    }

    newTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (!op->firstTime) {
        op->firstTime = newTime;
        op->prevTime = newTime;
        return;
    }

    /* Only update user interface every second */
    deltaTime = newTime - op->prevTime;
    if (deltaTime <= 1000) {
        return;
    }

    op->prevTime = newTime;

    spentTime = (newTime - op->firstTime) / 1000;
    if (spentTime <= 0) {
        return;
    }

    op->time     = spentTime;

    /* Calculate the frames per second */
    fpsf         = gblGetAndResetFrames() * 1000.0 / deltaTime;
    fps          = fpsf + 0.5;

    /* Calculate the video bit rate */
    videoKbpsf   = gblGetAndResetVideoBytesProcessed() * 8.0 / deltaTime;
    videoKbps    = videoKbpsf + 0.5;


    /* Get the local ARM cpu load */
    if (Cpu_getLoad(hCpu, &armLoad) < 0) {
        armLoad = 0;
        ERR("Failed to get ARM CPU load\n");
    }

    timePassed = localtime(&spentTime);
    if (timePassed == NULL) {
        return;
    }

    /* Update the UI */
    sprintf(tmpString, "%.2d:%.2d:%.2d\n", timePassed->tm_hour,
                                         timePassed->tm_min,
                                         timePassed->tm_sec);

    UI_updateValue(hUI, UI_Value_Time, tmpString);


    imageWidth = gblGetImageWidth();
    imageHeight = gblGetImageHeight();

    if (imageWidth != op->imageWidth ||
        imageHeight != op->imageHeight) {

        sprintf(tmpString, "%dx%d\n", imageWidth, imageHeight);

        UI_updateValue(hUI, UI_Value_ImageResolution, tmpString);

        op->imageWidth = imageWidth;
        op->imageHeight = imageHeight;
    } 

    sprintf(tmpString, "%d%%\n", armLoad);
    UI_updateValue(hUI, UI_Value_ArmLoad, tmpString);

    sprintf(tmpString, "%d fps\n", fps);
    UI_updateValue(hUI, UI_Value_Fps, tmpString);

    sprintf(tmpString, "%d kbps\n", videoKbps);
    UI_updateValue(hUI, UI_Value_VideoKbps, tmpString);

    UI_update(hUI);
}
void LED_blink(char num){
    LED_state_set(num,LED_ON);
    usleep(1000);
    LED_state_set(num,LED_OFF);
    usleep(10);
}
 
void LED_state_set(char num,char status){
    if(status == LED_OFF)
        ioctl(gpio,GPIO_WRITE,num | mask);
    else
        ioctl(gpio,GPIO_WRITE,num & (~mask));
}

void LED_init(){
   Int i;
   for(i=0;i<8;i++){
       ioctl(gpio,GPIO_OUTPUT,LED[i]);
       LED_state_set(LED[i],LED_DEFAULT_STATE);
   }
}
void GPIO_state_set(char num,char status){
    switch(status){
        case 1:
            ioctl(gpio,GPIO_WRITE,num | mask);
            break;
        case 0:
            ioctl(gpio,GPIO_WRITE,num & (~mask));
            break;
        default:
            ERR("Wrong GPIO state");
            break;
    }
}
void thread_pause(){
    if(status_thread == THREAD_RUN){
        pthread_mutex_lock(&mutex_dcs);
        status_thread = THREAD_STOP;
        /* printf("thread stop!\n"); */
        pthread_mutex_unlock(&mutex_dcs);
    }
    else{
        ERR("pthread pause already!\n");
    }
}

void thread_resume(){
    if(status_thread == THREAD_STOP){
        pthread_mutex_lock(&mutex_dcs);
        status_thread = THREAD_RUN;
        pthread_cond_signal(&cond_video);
        pthread_cond_signal(&cond_capture);
        /* printf("pthread run!\n"); */
        pthread_mutex_unlock(&mutex_dcs);
    }
    else{
        ERR("pthread run already!\n");
    }
}
/******************************************************************************
 * ctrlThrFxn
 ******************************************************************************/
Void *ctrlThrFxn(Void *arg)
{
    CtrlEnv                *envp                = (CtrlEnv *) arg;
    Void                   *status              = THREAD_SUCCESS;
    OsdData                 osdData             = OSD_DATA_INIT;
    Cpu_Attrs               cpuAttrs            = Cpu_Attrs_DEFAULT;
    Engine_Handle           hEngine             = NULL;
    Cpu_Handle              hCpu                = NULL;
    Char                  uart_buffer;
    Char                  instruction[32];
    Char                    LED_Freq = 5 , LF_ON = 0 ,LF_OFF = 0;
        /* Open the codec engine */
    hEngine = Engine_open(envp->engineName, NULL, NULL);

    if (hEngine == NULL) {
        ERR("Failed to open codec engine %s\n", envp->engineName);
        cleanup(THREAD_FAILURE);
    }

    /* Create the Cpu object to obtain ARM cpu load */
    hCpu = Cpu_create(&cpuAttrs);

    if (hCpu == NULL) {
        ERR("Failed to create Cpu Object\n");
    }

    /* Signal that initialization is done and wait for other threads */
    Rendezvous_meet(envp->hRendezvousInit);

    if (envp->keyboard) {
        printf(COMMAND_PROMPT);
        fflush(stdout);
    }

    while (!gblGetQuit()) {
        /* [> Update the dynamic data, either on the OSD or on the console <] */
        drawDynamicData(hEngine, hCpu, envp->hUI, &osdData);
        /* Feeddog(); */
        if(LF_ON ++ >= LED_Freq/2){
            LED_state_set(LED4,LED_ON);
            LF_ON = 0;
        }
        /*analog output*/
        /*it is necessary to suspend other threads to make sure that */
        /*the outputing process is not break, for the need of tlv5616's timing*/
        thread_pause();
        outputToDCS(gblGetBL(),gblGetTt(),gblGetMI());
        thread_resume();
        if (LF_OFF ++ >= LED_Freq){
            LED_state_set(LED4,LED_OFF);
            LF_OFF = 0;
        }
        /*receive and enqueue instructions.*/
        request_send(uart_port);
        while(read(uart_port,&uart_buffer,1)){
                LED_blink(LED2);
                protocolProcess(uart_buffer);
        }
        while(!IsQueueEmpty()){
            DeQueue(instruction);
            parseInstruction((unsigned char*)instruction);
            parameterUpdate();
        }
    }

cleanup:
    /* Make sure the other threads aren't waiting for us */
    Rendezvous_force(envp->hRendezvousInit);
    Pause_off(envp->hPauseProcess);

    /* Meet up with other threads before cleaning up */
    Rendezvous_meet(envp->hRendezvousCleanup);

    /* Clean up the thread before exiting */
    if (hCpu) {
        Cpu_delete(hCpu);
    }

    if (hEngine) {
        Engine_close(hEngine);
    }

    return status;
}

