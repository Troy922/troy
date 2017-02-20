#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <xdc/std.h>
extern void parameterInit(void);
//��ز�������
extern Void parameterUpdate(Void);

//��ͼ�����鸳ֵ
extern Void GetImgtoArray(Int32* img);

//��ȡͼƬ��ָ������ĻҶ�ֵ����(�����õ�ʱopencv��)
extern Int Pixel(Float x,Float y);

//������ʵ���Ȼ��㺯��,atan,cos,sin
extern Float RealLength(Int BlackLength,Float Tt);

//���ұ任,sqrt
extern Void Tpixel1stlr(Float* x, Float* y);

//���±任,sqrt
extern Void Tpixel2ndud(Float* x, Float* y);

//�����������,abs,sqrt
extern Int BlackDragon(Void);

//��ɢ�Ǵ������,atan,sqrt
extern Float AngleOfFlare(Int BLength);

//���ǿ��,sqrt,pow
extern Float MixIntensity(Int BLength);

//�ȶ����б�
extern Int stability(Float BL);

//�������������DCS(4-20ma�ź�)
extern Void outputToDCS(Float BL,Float Tt,Float MI);

//��ƽ������
extern Void avepro(Float* BL,Float* Tt,Float* MI);

//�ĳ�����λ�����Ͳ����ĸ�ʽ
extern Void BAMS_change(Float BL,Float Tt,Float MI,Int wdxflag,Char *dat);

extern Int xh0,yh0;
extern Float A1Agl;
extern Int BLTFFlag;
extern Int coalFlag;
//���ǿ�ȵĲ���
extern Int squareSize;
extern Int greynum;
extern Int greyignore;
extern Int LRboundFlag;
//�Ƕ�����Ĳ���
extern Float Avideolr;
extern Float Avideoud;
extern Float Asee;
extern Float focus;
extern Float Ps;
extern Float hud;
extern Float radius;


extern Int BLavenum;
extern Int FAavenum;
extern Int MIavenum;
extern Int wdxavenum;
extern Int Stab_Th;

#endif /*_PROCESS_H_*/
