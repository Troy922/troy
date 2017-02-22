#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_
#define BroadcastID	0xFE

#define	TRUE	1
#define	FALSE	0

#define	MODIFY_PARAMETER	0x00
#define	REQUEST_DATA		0xFE
#define	CLEAR_DATA			0xFD
#define CHANGE_REGISTERS    0XFC
#define CHANGE_REGESTER     0XFB

#define FIXED_LEN			23

#define QUEUESIZE	8
extern char Reg[16];
#define USE_FAKE_TT         (Reg[0]&(1<<0))      /*0 bit of Register 0 is fake angleofflare bit. */
#define FAKE_COEFFA         (Reg[1])             /*coefficiet for angleofflare*/
#define FAKE_COEFFK         (Reg[2])             /*coefficient for gradient of blackdragon */
#define FAKE_COEFFP         (Reg[3])             /*coefficient for mix intensity*/

extern char MyID;
extern Char para_name[21][20];
void protocolProcess(char dat);
void EnQueue(void);
unsigned long IsQueueEmpty(void);
void DeQueue(char *inst);
void dataAdd(float BL,float Tt,float MI,int wdxflag);
void deleteAllData(void);
void transmitDataPacket(void);
void ack(void);
void parseInstruction(unsigned char *inst);

#endif	/*_PROTOCOL_H_*/
