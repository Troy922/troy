#include <time.h>
#include <stdlib.h>
#include "TLV5616.h"
#include "../ctrl.h"
#include "../demo.h"

#define a_little_delay()	usleep(1)
/******************************************************************************
* @Name:		DA_init(void)
* @Description:	initialize serial bus's line status
* @Param:		none
* @Return:		none
* @Date:		2015-03-25 17:12:00
* ***************************************************************************/

void DA_init(void)
{
    ioctl(gpio,GPIO_OUTPUT,TLV_FS);
    GPIO_state_set(TLV_FS,1);
    ioctl(gpio,GPIO_OUTPUT,TLV_CS0);
    GPIO_state_set(TLV_CS0,1);
    ioctl(gpio,GPIO_OUTPUT,TLV_CS1);
    GPIO_state_set(TLV_CS1,1);
    ioctl(gpio,GPIO_OUTPUT,TLV_CS2);
    GPIO_state_set(TLV_CS2,1);
    ioctl(gpio,GPIO_OUTPUT,TLV_SCK);
    GPIO_state_set(TLV_SCK,1);
    ioctl(gpio,GPIO_OUTPUT,TLV_DI);
}
/******************************************************************************
* @Name:		DA_write(cs,val)
* @Description:	output 4-20mA current to corresponding channel
* @Param:		cs(channel,specific at board_config.h file),val(4~20)
* @Return:		none
* @Date:		2015-03-25 17:08:21
* ***************************************************************************/
void DA_write(unsigned int cs,float val)
{
	unsigned int dat;
	unsigned int i;
    dat = 0;
    dat =(unsigned int) ((val-4)*2071/16);
	if(cs == 0)
	{
        GPIO_state_set(TLV_CS0,0);
	}
	else if(cs == 1)
	{
        GPIO_state_set(TLV_CS1,0);
	}
	else if(cs == 2)
	{
        GPIO_state_set(TLV_CS2,0);
	}
    GPIO_state_set(TLV_FS,0);
	a_little_delay();

	for(i=0;i<16;i++)
	{
        GPIO_state_set(TLV_SCK,1);
		if(dat & 0x8000)
		{
            GPIO_state_set(TLV_DI,1);
		}
        else
            GPIO_state_set(TLV_DI,0);
        GPIO_state_set(TLV_SCK,0);
		a_little_delay();
		dat = dat << 1;
	}

    GPIO_state_set(TLV_FS,1);
    GPIO_state_set(TLV_SCK,1);

	if(cs == 0)
	{
        GPIO_state_set(TLV_CS0,1);
	}
	else if(cs == 1)
	{
        GPIO_state_set(TLV_CS1,1);
	}
	else if(cs == 2)
	{
        GPIO_state_set(TLV_CS2,1);
	}
}



