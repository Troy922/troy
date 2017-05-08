#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <xdc/std.h>

#include <ti/sdo/ce/Engine.h>

#include <ti/sdo/dmai/Cpu.h>
#include <ti/sdo/dmai/Pause.h>
#include <ti/sdo/dmai/Rendezvous.h>

#include "protocol.h"
#include "process.h"
#include "../ctrl.h"

char MyID = 1;
char Reg[16];
char instQueue[QUEUESIZE][32];		/*指令队列*/
unsigned long queueHead = 0;		/*指令队列头*/
unsigned long queueTail = 0;		/*指令队列尾*/
/******************************************************************************
* @Name:		protocolProcess(dat)
* @Description:	receive a byte,and parse it
* @Param:		dat: one byte from UART(485)
* @Return:		none
* @Date:		2015-04-03 16:45:48
* ***************************************************************************/
unsigned long inPacket = FALSE;		/*数据包接收状态*/
unsigned long cntOf0xFF = 0;		/*连续出现0xFF的个数*/
unsigned long cntOfByte = 0;		/*字节个数（除数据头）*/
unsigned long instLength = 0;		/*指令解析出的长度*/
unsigned char checkSum = 0;			/*校验和*/
Char para_name[21][20]={"coalFlag:","xh0:","yh0:","A1Agl:","LRboundFlag:","BLTFFlag:",
                    "squareSize:","greynum:","greyignore:","Avideolr:","Avideoud:",
                    "Asee:","focus:","Ps:","hud:","radius:","BLavenum:","FAavenum:",
                    "MIavenum:","wdxavenum:","Stab_Th:"};
char instBuffer[32];				/*指令缓冲区(ID LEN INST PARA1 ... PARAN)*/
char *instPointer;					/*指令字节指针*/

void protocolProcess(char dat)
{
	if(dat == 0xFF)
	{
		cntOf0xFF++;
		if(!inPacket) /*数据包接收过程外出现0xFF*/
		{
			switch(cntOf0xFF)
			{
				case 1:		/*新数据包第一个0xFF*/
					break;
				case 2:		/*新数据包第二个0xFF*/
					inPacket = TRUE; 		/*数据包开始*/
					cntOf0xFF = 0;			/*0xFF个数清零*/
					cntOfByte = 0;			/*字节个数清零*/
					break;
				default:
					break;
			}
		}
		else	/*接收数据包过程中出现0xFF*/  /*TODO 未测试*/
		{
			switch(cntOf0xFF)
			{
				case 1:		/*数据包接受过程中第一个0xFF*/
					*instPointer++ = dat;	/*第一个0xFF加入包中*/
					break;
				case 2:		/*数据包接受过程中第二个0xFF*/
					inPacket = TRUE;		/*数据包开始（前面出现错误）*/
					cntOf0xFF = 0;			/*0xFF个数清零*/
					cntOfByte = 0;			/*字节个数清零*/
					break;
				default:	
					break;
			}
		}
	}
	else
	{
		cntOf0xFF = 0;	/*cntOf0xFF清零*/
		if(inPacket)	/*接收过程中的字节加入缓冲区*/
		{
			cntOfByte++;
			if(cntOfByte == 1)		/*ID*/
			{
				if(dat != MyID && dat != BroadcastID)	/*不匹配*/
				{
					inPacket = FALSE;
				}
				else									/*ID匹配*/
				{
					instPointer = instBuffer;
					*instPointer++ = dat;
					checkSum = dat;
				}
			}
			else if(cntOfByte == 2)	/*Length*/
			{
				instLength = dat;
				*instPointer++ = dat;
				checkSum += dat;
			}
			else if(--instLength) 	/*instruction & parameter*/	/*执行 instLength-1 次*/
			{
				*instPointer++ = dat;
				checkSum += dat;
			}
			else					/*checkSum*/
			{
				inPacket = FALSE;
				if((checkSum&0xFF) == 0x00)
				{
					checkSum = 0x01;
				}
				if((checkSum&0xFF) == ((~dat)&0xFF))
				{
					switch(instBuffer[2])	/*instruction type byte*/
					{
                        case CHANGE_REGESTER    :
                        case CHANGE_REGISTERS   :
                        case MODIFY_PARAMETER	:
                            EnQueue();
							if(instBuffer[0] != BroadcastID)
							{
								ack();
							}
							break;
						case CLEAR_DATA			:
							deleteAllData();
							if(instBuffer[0] != BroadcastID)
							{
								ack();
							}
							break;
						case REQUEST_DATA		:
							if(instBuffer[0] != BroadcastID)
							{
								transmitDataPacket();
							}
							break;
						default :
							break;
					}
				}/*else校验出错不返回数据包*/
			}
		}
	}
}
/******************************************************************************
* @Name:		ack()
* @Description:	acknowledge master's parameter modification instruction
* @Param:		none
* @Return:		none
* @Date:		2015-04-03 20:01:22
* ***************************************************************************/
void ack(void)
{
    clear_send(uart_port);
    LED_blink(LED3);
    UART_sendChar(0xFF);	/*Packet Head	*/
	UART_sendChar(0xFF);
	UART_sendChar(0x00);	/*Master's ID	*/
	UART_sendChar(0x02);	/*Length		*/
	UART_sendChar(0x00);	/*ACK WORD		*/
	UART_sendChar(0xFD);	/*Check Sum		*/
    request_send(uart_port);
}
/******************************************************************************
* @Name:		EnQueue()
* @Description:	add an element to the instruction queue's tail
* @Param:		none
* @Return:		none
* @Date:		2015-04-03 16:48:08
* ***************************************************************************/
void EnQueue(void)
{
	unsigned long tail;
	unsigned long i;
	tail = (queueTail+1)%QUEUESIZE;		/*循环队列*/
	if (tail == queueHead)             	/*指令队列满(基本不会出现)*/
	{
		queueHead = (queueHead+1)%QUEUESIZE;	/*丢弃最老的指令*/
	}
	for(i=0;i<32;i++)					/*将缓冲区指令写入指令队列*/
	{
		instQueue[queueTail][i] = instBuffer[i];
	}
	queueTail = tail;					/*更新队列尾*/
}
/******************************************************************************
* @Name:		IsQueueEmpty()
* @Description:	query whether the instruction queue is empty
* @Param:		none
* @Return:		if instruction queue is empty return 1,else return 0
* @Date:		2015-04-03 16:48:54
* ***************************************************************************/
unsigned long IsQueueEmpty(void)
{
	if(queueHead == queueTail)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/******************************************************************************
* @Name:		DeQueue(char *inst)
* @Description:	delete an element from the instruction queue's head
* @Param:		instruction pointer to put at
* @Return:		none
* @Date:		2015-04-03 16:50:50
* ***************************************************************************/
void DeQueue(char *inst)
{
	unsigned long i;
	if(IsQueueEmpty())  /*队列为空?*/
	{
		*inst = 0;				/*指令出错*/
	}
	else
	{
		for(i=0;i<31;i++)
		{
			inst[i] = instQueue[queueHead][i+1];
		}
		queueHead = (queueHead+1)%QUEUESIZE;	/*头指向下一个指令*/
	}
}
/******************************************************************************
* @Name:		dataAdd(char *dat)
* @Description:	add data to data buffer
* @Param:		pointer of data start
* @Return:		none
* @Date:		2015-04-04 15:01:52
* ***************************************************************************/
#define	DTBUF_LEN  16	
unsigned char dataBuffer[DTBUF_LEN][4];
unsigned long dataCount = 0;
void dataAdd(float BL,float Tt,float MI,int wdxflag)
{
	if(dataCount == DTBUF_LEN)
	{
		dataCount = 0;
	}
   /*  dataBuffer[dataCount][0] = (unsigned char)(BL*0.5+0.5); */
	/* dataBuffer[dataCount][1] = (unsigned char)(Tt*2+0.5); */
	/* dataBuffer[dataCount][2] = (unsigned char)(MI + 0.5); */
	/* dataBuffer[dataCount][3] = (unsigned char)wdxflag; */

	dataBuffer[dataCount][0] = (unsigned char)BL;
	dataBuffer[dataCount][1] = (unsigned char)Tt;
	dataBuffer[dataCount][2] = (unsigned char)MI;
	dataBuffer[dataCount][3] = (unsigned char)wdxflag;
	dataCount++;
}
/******************************************************************************
* @Name:		deleteAllData()
* @Description:	delete all data in data buffer
* @Param:		none
* @Return:		none
* @Date:		2015-04-04 15:49:47
* ***************************************************************************/
void deleteAllData(void)
{
	dataCount = 0;
}
/******************************************************************************
* @Name:		transmitDataPacket()
* @Description:	transmit all data in data buffer
* @Param:		none
* @Return:		none
* @Date:		2015-04-04 15:03:15
* ***************************************************************************/
void transmitDataPacket(void)
{
	unsigned long i,cs;
    clear_send(uart_port);
    LED_blink(LED3);
    UART_sendChar(0xFF);					/*Packet Head	*/
	UART_sendChar(0xFF);
	UART_sendChar(0x00);					/*Master's ID	*/
	UART_sendChar((char)(2+(dataCount<<2)));	/*Length		*/
	UART_sendChar(0xFE);					/*DATA WORD		*/
	cs = dataCount<<2;
    for(i=0;i<dataCount;i++)
	{
		UART_sendChar(dataBuffer[i][0]);
		UART_sendChar(dataBuffer[i][1]);
		UART_sendChar(dataBuffer[i][2]);
		UART_sendChar(dataBuffer[i][3]);
		cs += dataBuffer[i][0]+dataBuffer[i][1]+dataBuffer[i][2]+dataBuffer[i][3];
	}
	if((cs&0xFF) == 0x00)
	{
		cs = 0x01;
	}
	UART_sendChar((char)((~cs)&0xFF));	/*Check Sum		*/
    request_send(uart_port);    
}
void Num2Str(int num_int,char *num_str){
    char num_tmp[10],k=0,j=0;
    if(num_int == 0)
       num_str[k++] = '0';
    while(num_int){
        num_tmp[j++] = num_int % 10 + '0';
        num_int = num_int / 10;
    }

    while(k < j){
        num_str[k++]=num_tmp[j - k - 1];
    }

    num_str[k++] = '\n';
    num_str[k] ='\0' ;
}
/******************************************************************************
* @Name:		parseInstruction(unsigned char *inst)
* @Description:	parse instruction at *inst
* @Param:		instruction array at pointer inst
* @Return:		none
* @Date:		2015-04-04 16:39:33
* ***************************************************************************/
void parseInstruction(unsigned char *inst)
{
    char tmp_para[30],tmp_num[5];
    int i;
    int len;
    FILE *para_File,*Reg_File; 
    if((inst[0] == FIXED_LEN) && (inst[1] == MODIFY_PARAMETER))
    {
        para_File = fopen("parameter_config","w");
        coalFlag = inst[2];	
        if(0xFE == inst[3])
        {
            xh0 = 319;
        }
        else
        {
            xh0 = inst[3]*2;
        }
        yh0 = inst[4];
        A1Agl = inst[5];
        LRboundFlag = inst[6];
        BLTFFlag = inst[7];
        squareSize = inst[8];
        greynum = inst[9];
        greyignore = inst[10];
        Avideolr = inst[11];
        Avideoud = inst[12];
        Asee = inst[13];
        focus = inst[14]*0.05;//修改类型
        Ps = inst[15]*0.00001;//修改类型
        hud = inst[16]*5;
        radius = inst[17]*5;
        BLavenum = inst[18];
        FAavenum = inst[19];
        MIavenum = inst[20];
        wdxavenum = inst[21];
        Stab_Th = inst[22]*0.01;//修改类型

       for(i=2;i<23;i++){
           strcpy(tmp_para,para_name[i-2]);
           Num2Str(inst[i],tmp_num);
           strcat(tmp_para,tmp_num);
           fputs(tmp_para,para_File);
       }
    fclose(para_File);
    }
    else if((inst[0] == 18) && (inst[1] == CHANGE_REGISTERS)){

        Reg_File = fopen("Registers","w");
        for(i=0;i<16;i++){
           strcpy(tmp_para,"Reg ");
           Num2Str(i,tmp_num);
           tmp_num[strlen(tmp_num)-1] = '\0';
           strcat(tmp_para,tmp_num);
           len = strlen(tmp_para);
           tmp_para[len] = ':';
           tmp_para[len+1] = '\0';

           Reg[i] = inst[i+2];
           Num2Str(Reg[i],tmp_num);
           strcat(tmp_para,tmp_num);
           fputs(tmp_para,Reg_File);
        }
        fclose(Reg_File);
    }
    else if((inst[0] == 4) && (inst[1] == CHANGE_REGESTER)){
        Reg_File = fopen("Registers","w");
        if(inst[2] <= 16)
            Reg[inst[2]] = inst[3];
        for(i=0;i<16;i++){
           strcpy(tmp_para,"Reg ");
           Num2Str(i,tmp_num);
           tmp_num[strlen(tmp_num)-1] = '\0';
           strcat(tmp_para,tmp_num);
           len = strlen(tmp_para);
           tmp_para[len] = ':';
           tmp_para[len+1] = '\0';

           Num2Str(Reg[i],tmp_num);
           strcat(tmp_para,tmp_num);
           fputs(tmp_para,Reg_File);
        }

        fclose(Reg_File);
    }
}

