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
char instQueue[QUEUESIZE][32];		/*ָ�����*/
unsigned long queueHead = 0;		/*ָ�����ͷ*/
unsigned long queueTail = 0;		/*ָ�����β*/
/******************************************************************************
* @Name:		protocolProcess(dat)
* @Description:	receive a byte,and parse it
* @Param:		dat: one byte from UART(485)
* @Return:		none
* @Date:		2015-04-03 16:45:48
* ***************************************************************************/
unsigned long inPacket = FALSE;		/*���ݰ�����״̬*/
unsigned long cntOf0xFF = 0;		/*��������0xFF�ĸ���*/
unsigned long cntOfByte = 0;		/*�ֽڸ�����������ͷ��*/
unsigned long instLength = 0;		/*ָ��������ĳ���*/
unsigned char checkSum = 0;			/*У���*/
Char para_name[21][20]={"coalFlag:","xh0:","yh0:","A1Agl:","LRboundFlag:","BLTFFlag:",
                    "squareSize:","greynum:","greyignore:","Avideolr:","Avideoud:",
                    "Asee:","focus:","Ps:","hud:","radius:","BLavenum:","FAavenum:",
                    "MIavenum:","wdxavenum:","Stab_Th:"};
char instBuffer[32];				/*ָ�����(ID LEN INST PARA1 ... PARAN)*/
char *instPointer;					/*ָ���ֽ�ָ��*/

void protocolProcess(char dat)
{
	if(dat == 0xFF)
	{
		cntOf0xFF++;
		if(!inPacket) /*���ݰ����չ��������0xFF*/
		{
			switch(cntOf0xFF)
			{
				case 1:		/*�����ݰ���һ��0xFF*/
					break;
				case 2:		/*�����ݰ��ڶ���0xFF*/
					inPacket = TRUE; 		/*���ݰ���ʼ*/
					cntOf0xFF = 0;			/*0xFF��������*/
					cntOfByte = 0;			/*�ֽڸ�������*/
					break;
				default:
					break;
			}
		}
		else	/*�������ݰ������г���0xFF*/  /*TODO δ����*/
		{
			switch(cntOf0xFF)
			{
				case 1:		/*���ݰ����ܹ����е�һ��0xFF*/
					*instPointer++ = dat;	/*��һ��0xFF�������*/
					break;
				case 2:		/*���ݰ����ܹ����еڶ���0xFF*/
					inPacket = TRUE;		/*���ݰ���ʼ��ǰ����ִ���*/
					cntOf0xFF = 0;			/*0xFF��������*/
					cntOfByte = 0;			/*�ֽڸ�������*/
					break;
				default:	
					break;
			}
		}
	}
	else
	{
		cntOf0xFF = 0;	/*cntOf0xFF����*/
		if(inPacket)	/*���չ����е��ֽڼ��뻺����*/
		{
			cntOfByte++;
			if(cntOfByte == 1)		/*ID*/
			{
				if(dat != MyID && dat != BroadcastID)	/*��ƥ��*/
				{
					inPacket = FALSE;
				}
				else									/*IDƥ��*/
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
			else if(--instLength) 	/*instruction & parameter*/	/*ִ�� instLength-1 ��*/
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
				}/*elseУ������������ݰ�*/
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
	tail = (queueTail+1)%QUEUESIZE;		/*ѭ������*/
	if (tail == queueHead)             	/*ָ�������(�����������)*/
	{
		queueHead = (queueHead+1)%QUEUESIZE;	/*�������ϵ�ָ��*/
	}
	for(i=0;i<32;i++)					/*��������ָ��д��ָ�����*/
	{
		instQueue[queueTail][i] = instBuffer[i];
	}
	queueTail = tail;					/*���¶���β*/
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
	if(IsQueueEmpty())  /*����Ϊ��?*/
	{
		*inst = 0;				/*ָ�����*/
	}
	else
	{
		for(i=0;i<31;i++)
		{
			inst[i] = instQueue[queueHead][i+1];
		}
		queueHead = (queueHead+1)%QUEUESIZE;	/*ͷָ����һ��ָ��*/
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
        focus = inst[14]*0.05;//�޸�����
        Ps = inst[15]*0.00001;//�޸�����
        hud = inst[16]*5;
        radius = inst[17]*5;
        BLavenum = inst[18];
        FAavenum = inst[19];
        MIavenum = inst[20];
        wdxavenum = inst[21];
        Stab_Th = inst[22]*0.01;//�޸�����

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

