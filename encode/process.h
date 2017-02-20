#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <xdc/std.h>
extern void parameterInit(void);
//相关参数更新
extern Void parameterUpdate(Void);

//给图像数组赋值
extern Void GetImgtoArray(Int32* img);

//读取图片中指定坐标的灰度值函数(现在用的时opencv库)
extern Int Pixel(Float x,Float y);

//黑龙真实长度换算函数,atan,cos,sin
extern Float RealLength(Int BlackLength,Float Tt);

//左右变换,sqrt
extern Void Tpixel1stlr(Float* x, Float* y);

//上下变换,sqrt
extern Void Tpixel2ndud(Float* x, Float* y);

//黑龙处理程序,abs,sqrt
extern Int BlackDragon(Void);

//扩散角处理程序,atan,sqrt
extern Float AngleOfFlare(Int BLength);

//混合强度,sqrt,pow
extern Float MixIntensity(Int BLength);

//稳定性判别
extern Int stability(Float BL);

//输出三个特征到DCS(4-20ma信号)
extern Void outputToDCS(Float BL,Float Tt,Float MI);

//求平均函数
extern Void avepro(Float* BL,Float* Tt,Float* MI);

//改成向上位机发送参数的格式
extern Void BAMS_change(Float BL,Float Tt,Float MI,Int wdxflag,Char *dat);

extern Int xh0,yh0;
extern Float A1Agl;
extern Int BLTFFlag;
extern Int coalFlag;
//混合强度的参数
extern Int squareSize;
extern Int greynum;
extern Int greyignore;
extern Int LRboundFlag;
//角度问题的参数
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
