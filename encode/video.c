/*
 * video.c
 *
 * This source file has the implementations for the video thread
 * functions implemented for 'DVSDK encode demo' on DM365 platform
 * TODO: transplant algrithm into this thread.

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
 */

#include <xdc/std.h>

#include <ti/sdo/ce/Engine.h>

#include <ti/sdo/dmai/Fifo.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/BufTab.h>
#include <ti/sdo/dmai/VideoStd.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/Rendezvous.h>
#include <ti/sdo/dmai/ce/Venc1.h>

#include "video.h"
#include "../demo.h"

#ifndef YUV_420SP
#define YUV_420SP 256
#endif 

/* Number of buffers in the pipe to the capture thread */
/* Note: It needs to match capture.c pipe size */
#define VIDEO_PIPE_SIZE           4 
Void drawline(Buffer_Handle hBuf)
{
        Int8  *yPtr     = Buffer_getUserPtr(hBuf);
        Int32  ySize    = Buffer_getSize(hBuf) * 2 / 3;
        Int8  *cbcrPtr  = yPtr + ySize;
        Int    bpp = ColorSpace_getBpp(ColorSpace_YUV420PSEMI);
        Int    y;
        Int    position1,position2;
        BufferGfx_Dimensions dim;

        BufferGfx_getDimensions(hBuf, &dim);
        position1 = dim.height / 2 * dim.lineLength;
        position2 = dim.height/2 * ( dim.lineLength / 2);
        yPtr += dim.y * dim.lineLength + dim.x * bpp / 8 + position1;
        for (y = 0; y < 20; y++) {
            memset(yPtr, 0x0, dim.width * bpp / 8);
            yPtr += dim.lineLength;
        }

        cbcrPtr += dim.y * dim.lineLength / 2 + dim.x * bpp / 8 + position2;
        for (y = 0; y < 20 / 2; y++) {
            memset(cbcrPtr, 0x40, dim.width * bpp / 8);
            cbcrPtr += dim.lineLength;
        }
}


/******************************************************************************
 * videoThrFxn
 ******************************************************************************/
Void *videoThrFxn(Void *arg)
{
    VideoEnv               *envp                = (VideoEnv *) arg;
    Void                   *status              = THREAD_SUCCESS;
    BufferGfx_Attrs         gfxAttrs            = BufferGfx_Attrs_DEFAULT;
    BufTab_Handle           hBufTab             = NULL;
    Int                     frameCnt            = 0;
    Buffer_Handle           hCapBuf, hDstBuf;
    Int                     fifoRet;
    Int                     bufIdx;
    ColorSpace_Type         colorSpace = ColorSpace_YUV420PSEMI;
    BufferGfx_Dimensions    dim;
    Int32                   bufSize;


   
    /* Signal that the codec is created and output buffer size available */
    Rendezvous_meet(envp->hRendezvousWriter);

    gfxAttrs.colorSpace = colorSpace;
    gfxAttrs.dim.width  = envp->imageWidth;
    gfxAttrs.dim.height = envp->imageHeight;
    gfxAttrs.dim.lineLength = 
        Dmai_roundUp(BufferGfx_calcLineLength(gfxAttrs.dim.width,
                               gfxAttrs.colorSpace), 32);
    
    /* 
     *Calculate size of codec buffers based on rounded LineLength 
     *This will allow to share the buffers with Capture thread. If the
     *size of the buffers was obtained from the codec through 
     *Venc1_getInBufSize() there would be a mismatch with the size of
     * Capture thread buffers when the width is not 32 byte aligned.
     */
    if (colorSpace ==  ColorSpace_YUV420PSEMI) {
        bufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 3 / 2;
        envp->outBufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 3 / 2; 
    } 
    else {
        bufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 2;
        envp->outBufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 2; 
    } 

    /* Create a table of buffers with size based on rounded LineLength */
    hBufTab = BufTab_create(VIDEO_PIPE_SIZE, bufSize,
                            BufferGfx_getBufferAttrs(&gfxAttrs));

    if (hBufTab == NULL) {
        ERR("Failed to allocate contiguous buffers\n");
        cleanup(THREAD_FAILURE);
    }

    /* Send buffers to the capture thread to be ready for main loop */
    for (bufIdx = 0; bufIdx < VIDEO_PIPE_SIZE; bufIdx++) {
        if (Fifo_put(envp->hCaptureInFifo,
                     BufTab_getBuf(hBufTab, bufIdx)) < 0) {
            ERR("Failed to send buffer to display thread\n");
            cleanup(THREAD_FAILURE);
        }
    }
    /* Signal that initialization is done and wait for other threads */
    Rendezvous_meet(envp->hRendezvousInit);
    
    while (!gblGetQuit()) {
        /* Pause processing? */
        Pause_test(envp->hPauseProcess);

        /* Get a buffer to encode from the capture thread */
        fifoRet = Fifo_get(envp->hCaptureOutFifo, &hCapBuf);

        if (fifoRet < 0) {
            ERR("Failed to get buffer from video thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Did the capture thread flush the fifo? */
        if (fifoRet == Dmai_EFLUSH) {
            cleanup(THREAD_SUCCESS);
        }
        /* drawline(hCapBuf); */
        /* Get a buffer to encode to from the writer thread */
        fifoRet = Fifo_get(envp->hWriterOutFifo, &hDstBuf);

        if (fifoRet < 0) {
            ERR("Failed to get buffer from video thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Did the writer thread flush the fifo? */
        if (fifoRet == Dmai_EFLUSH) {
            cleanup(THREAD_SUCCESS);
        }

/*         [> Make sure the whole buffer is used for input <] */
        BufferGfx_resetDimensions(hCapBuf);

        /* [> Ensure that the video buffer has dimensions accepted by codec <] */
        BufferGfx_getDimensions(hCapBuf, &dim);
        dim.height = Dmai_roundUp(dim.height, CODECHEIGHTALIGN);
        BufferGfx_setDimensions(hCapBuf, &dim);


        /* [> Reset the dimensions to what they were originally <] */
        BufferGfx_resetDimensions(hCapBuf);

        /* drawline(hCapBuf); */
        //TODO: add algorithm at this place

        /* Send encoded buffer to writer thread for filesystem output */
        if (Fifo_put(envp->hWriterInFifo, hCapBuf) < 0) {
            ERR("Failed to send buffer to display thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Return buffer to capture thread */
        if (Fifo_put(envp->hCaptureInFifo, hCapBuf) < 0) {
            ERR("Failed to send buffer to display thread\n");
            cleanup(THREAD_FAILURE);
        }

        /* Increment statistics for the user interface */
        gblIncVideoBytesProcessed(Buffer_getNumBytesUsed(hDstBuf));

        frameCnt++;
    }

cleanup:
    /* Make sure the other threads aren't waiting for us */
    Rendezvous_force(envp->hRendezvousInit);
    Rendezvous_force(envp->hRendezvousWriter);
    Pause_off(envp->hPauseProcess);
    Fifo_flush(envp->hWriterInFifo);
    Fifo_flush(envp->hCaptureInFifo);

    /* Make sure the other threads aren't waiting for init to complete */
    Rendezvous_meet(envp->hRendezvousCleanup);

    /* Clean up the thread before exiting */
    if (hBufTab) {
        BufTab_delete(hBufTab);
    }



    return status;
}

