#include "tinymath.h"
#include "protocol.h"
#include <math.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/BufTab.h>
//-----------------------------------------------------------------------------------
//需要给出的外部参数
Int xh0 = 180,yh0 = 0;//火焰边界中心点，(假设了初始值)!需要外部给变量!
Float A1Agl = 90;//表示火焰方向的角度!需要外部给变量!
Int BLTFFlag = 0;//黑龙相对长度0，绝对长度1标志
Int coalFlag = 1;//投煤标志1投，0没投
Int ChangeFlag = 0;//上位机修改数据标志1改，0不改
//Int imgFlag = 1;//图片准备标志1准备好了，0未准备好
Int SendFlag = 0;//数据发送给上位机0不发送，1发送
//混合强度的参数
Int squareSize=10;//每行每列分的块数！需要外部给变量！
Int greynum = 11;//灰度级数目!需要外部给变量!
Int greyignore = 2;//干扰级数目!需要外部给变量!
Int LRboundFlag = 1;//火焰中心的左边为0，右边为1，表示用左边求还是右边求!需要外部给变量!
Float NMA_k;
//角度问题的参数
Float Avideolr=15;//摄像头与！竖直！方向的夹角!!!!（左右斜和中间的夹角）!需要外部给变量!
Float Avideoud=85;//摄像头与！竖直！方向的夹角!!!!（上下斜和竖直的夹角）!需要外部给变量!
Float Asee=45;//视野张角!需要外部给变量!
Float focus=1;//焦距!需要外部给变量!
Float Ps=0.001;//像元尺寸！！！不是这个窗口获得，只是借用!需要外部给变量!
Float hud=100;//摄像头高度（包含半径的）!需要外部给变量!
Float radius=50;//喷口半径（数字是瞎写的）!需要外部给变量!
Float hreal;//hud-radius //摄像头距实际喷口距离（排除喷口半径的）!需要外部给变量!
const Float pai=3.14159265357;//π
//------------------------------------------------------------------------------------
//-----------------------------------------------------------------
//角度变换中需要用到的变量，可以有参数直接求得，不用重复计算
Float TAvud;//夹角正弦值，角度变换有用
Float TAs;//视角正弦值	，角度变换有用
Float TAvlr;//夹角正弦值，角度变换有用
Float TAvmslr;//夹角-视角的正弦值，角度变换有用
Float hlr;//摄像头高度,用在！左右变换！中
//-------------------------------------------------------------------
//---------------------------------------------------------------------
//这一部分可以放到初始化来做，黑龙强度，扩散角，混合系数都有这一部分

Float AAgl;//将角度制改成弧度制
Float k_dir;//火焰方向的斜率
Float b_dir;//火焰方向的常数y=k_dir*x+b_dir
Float c_dir;//火焰垂直方向常数y=-x/k_dir+c，用来求黑龙长度！！！k不能为0
Float d_dir;//火焰垂直方向常数x=-k_dir*y+d，用来求黑龙长度！！！k不能为无穷
//------------------------------------------------------------------------------
//求平均程序
Float BLarray[64]={0},FAarray[64]={0},MIarray[64]={0};//储存计算需要的数组和变量
Float BLave_backup = 0;
Float FAave_backup = 0;
Float MIave_backup = 0;
Int BLavenum = 20;//多少个数平均,黑龙
Int BLarray_LP=0;
Int FAavenum = 20;//多少个数平均，扩散角
Int FAarray_LP=0;
Int MIavenum = 20;//多少个数平均，混合强度
Int MIarray_LP=0;
//---------------------------------------
//-----------------------------------------------------------
//稳定性判别,变量需要放在循环的外面定义
Float wendinglength[64]={0};
Int wdxavenum = 20;
Float nextlength = 0;
Float priorlength = 0;
Int Stab_Th = 0.1;//xg
//定义为全局变量
//-----------------------------------------------------------
void parameterInit(void)
{
    unsigned long i;
    parameterUpdate();
    for(i=0;i<64;i++){
        BLarray[i]=0;
        FAarray[i]=0;
        MIarray[i]=0;
        wendinglength[i]=0;
    }

}
/******************************************************************************
* @Name:		parameterUpdate(Void)
* @Description:	update corresponding parameters
* @Param:		none
* @Return:		none
* @Date:		2015-04-05 17:22:13
* ***************************************************************************/
Void parameterUpdate(Void)
{
	TAvud=chatan(Avideoud);//夹角正弦值，角度变换有用
	TAs=chatan(Asee);//视角正弦值	，角度变换有用
	TAvlr=chatan(Avideolr);//夹角正弦值，角度变换有用
	TAvmslr=chatan(Avideolr-Asee);//夹角-视角的正弦值，角度变换有用
	hlr= focus*sqrt(1+TAvlr*TAvlr);//摄像头高度,用在！左右变换！中
	//-------------------------------------------------------------------
	//---------------------------------------------------------------------
	//这一部分可以放到初始化来做，黑龙强度，扩散角，混合系数都有这一部分
	hreal = hud-radius;
	AAgl = A1Agl/180*pai;//将角度制改成弧度制
	k_dir = chatan(A1Agl);//火焰方向的斜率
	b_dir = yh0-k_dir*xh0;//火焰方向的常数y=kx+b_dir
	c_dir = yh0+xh0/k_dir;//火焰垂直方向常数y=-x/k_dir+c，用来求黑龙长度！！！k不能为0
	d_dir = k_dir*yh0+xh0;//火焰垂直方向常数x=-k_dir*y+d，用来求黑龙长度！！！k不能为无穷
	//----------------------------------------------------------------------------
	NMA_k = -400.0*(greynum-greyignore)*(greynum-greyignore)/(greynum-greyignore-1);
}
Int8 *picArray;

Void GetImgtoArray(Buffer_Handle hBuf){
    picArray = Buffer_getUserPtr(hBuf);
}

Int Pixel(Float x,Float y)
{
	Int i = y+0.5;
	Int j = x+0.5;
    Int Grey = picArray[0xFFFFF&(i*320)+j];
    /* Int Grey = picArray[i][j]; */
	return Grey;
}

#define Pixel0(x,y)	(picArray[0xFFFFF&(320*x)+y])

//黑龙真实长度换算函数,atan,cos,sin
Float RealLength(Int BlackLength,Float Tt)//BlackLength为一次变换后的坐标，不用像元尺寸，Tt为扩散角角度
{

	Float TB=Avideoud+fast_atan(BlackLength*Ps/focus-TAs);//角度B
	Float RealL=hreal*chacos(Tt)*chasin(TB)/chasin(90+Tt-TB);//真实黑龙长度求取
	return RealL;
}

//左右变换,sqrt
Void Tpixel1stlr(Float* x, Float* y)
{
	Int u = *x+0.5;
	Int v = *y+0.5;
	*x = hlr*(TAvlr+u*Ps/focus-TAs)/(1+TAvlr*(TAs-u*Ps/focus))-hlr*TAvmslr;//坐标变换后x的坐标
	*y = v*Ps*sqrt((hlr*hlr+(*x+hlr*TAvmslr)*(*x+hlr*TAvmslr))/(focus*focus+(focus*TAs-u*Ps)*(focus*TAs-u*Ps)));//坐标变换后y的坐标
}

//上下变换,sqrt
Void Tpixel2ndud(Float* x, Float* y)
{
	Float u = *x;
	Float v = *y;
	*y = hud*(TAvud+v/focus-TAs)/(1+TAvud*(TAs-v/focus));//坐标变换后y的坐标
	*x = u*sqrt((hud*hud+(*y)*(*y))/(focus*focus+(focus*TAs-v)*(focus*TAs-v)));//坐标变换后x的坐标
}
/******************************************************************************
* @Name:		BlackDragon(Void)
* @Description:	get black dragon length
* @Param:		none
* @Return:		relative black dragon length
* @Date:		2015-04-05 17:20:22
* ***************************************************************************/
Int BlackDragon(Void)//黑龙处理程序,abs,sqrt
{
	//------------------------------------------
	Int Lsum = 0;//黑龙长度加和储存
	Int xx=0,yy=0;//黑龙点
	Int t,Gmax,y1,x1;
	Float x,y,length;
	Int det,j;
	//-------------------------------------------------------------

	Int blacklength = 0;
	if(AAgl>pai*0.5)//方向大于90
	{
		if(319 == xh0 || 0 == yh0)//中心点横坐标为319或者纵坐标为0时
		{

			t=0;
			Gmax=0;
			y1=yh0;
			x1=xh0;
			x=xh0;
			y=yh0;
			length=0;
			for(det=0;det<13;det++)//做10条直线，求长度边界
			{
				xx = xh0;
				yy = yh0;
				Gmax=0;
				if(AAgl>0.75*pai)//方向角度大于135
				{
					for(j=0;j<319;j++)//寻找每条直线的边界
					{
						if(x-2<0 || y-2*k_dir>239)//超出图像尺寸跳出
							break;
						if(x>300 || y<20)//加入阈值
							t = 0;
						else
							t = Pixel(x-2,y-2*k_dir)-Pixel(x,y);//两相邻点的灰度差
						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1;//将点移到下一点
						y = y-k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//用点到线距离求黑龙长度
				}
				else//方向角度再90-135,xh0=319,yh0=0
				{
					for(j=0;j<239;j++)//寻找每条直线的边界
					{
						if(x+2/k_dir<0 || y+2>239)//超出图像尺寸跳出
							break;
						if(y<20 || x>300)//加入阈值
							t = 0;
						else
							t = Pixel(x+2/k_dir,y+2)-Pixel(x,y);//两相邻点的灰度差
						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1/k_dir;//将点移到下一点
						y = y+1;
					}
					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//用点到线距离求黑龙长度
				}
				Lsum = Lsum+length;

				if(0 == LRboundFlag)//左边
				{
					if(y1 == 0)
					{
						x1 = x1-3;
						if(x1<0)
							break;
					}
					else if(319 == x1)
					{
						y1 = y1-3;
						if(y1<0)
						{
							y1 = 0;
							x1 = 318;
						}
					}
				}
				else//右边
				{
					if(319 == x1)
					{
						y1 = y1+3;
						if(y1>239)
							break;
					}
					else if(0 == y1)
					{
						x1 = x1+3;
						if(x1>319)
						{
							x1 = 319;
							y1 = 1;
						}
					}
				}
				x = x1;
				y = y1;
			}
			blacklength = Lsum/13;
		}
		//------------------------------------------------------------------------------------
		else if(0 == xh0 || 239 == yh0)
		{
			t=0;
			Gmax=0;
			y1=yh0;
			x1=xh0;
			x=xh0;
			y=yh0;
			length=0;
			for(det=0;det<13;det++)//做10条直线，求长度边界
			{
				Gmax=0;
				xx = xh0;
				yy = yh0;
				if(AAgl>0.75*pai)//方向角度大于135
				{
					for(j=0;j<319;j++)//寻找每条直线的边界
					{
						if(x+2>319|| y+2*k_dir<0)//超出图像尺寸跳出
							break;
						if(x<20 || y>220)//加入阈值
							t = 0;
						else
							t = Pixel(x+2,y+2*k_dir)-Pixel(x,y);//两相邻点的灰度差
						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1;//将点移到下一点
						y = y+k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//用点到线距离求黑龙长度
				}
				else//方向角度再90-135,xh0=0,yh0=239
				{
					for(j=0;j<239;j++)//寻找每条直线的边界
					{
						if(x-2/k_dir>319 || y-2<0)//超出图像尺寸跳出
							break;
						if(y>220 || x<20)//加入阈值
							t = 0;
						else
							t = Pixel(x-2/k_dir,y-2)-Pixel(x,y);//两相邻点的灰度差
						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1/k_dir;//将点移到下一点
						y = y-1;
					}
					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//用点到线距离求黑龙长度
				}
				Lsum = Lsum+length;
				//计算黑龙左边还是右边
				if(1 == LRboundFlag)//右边
				{
					if(y1 == 239)
					{
						x1 = x1+3;
						if(x1>319)
							break;
					}
					else if(x1 == 0)
					{
						y1 = y1+3;
						if(y1>239)
						{
							x1 = 1;
							y1 = 239;
						}
					}
				}
				else//左边
				{
					if(x1 == 0)
					{
						y1 = y1-3;
						if(y1<0)
							break;
					}
					else if(y1 == 239)
					{
						x1 = x1-3;
						if(x1<0)
						{
							x1 = 0;
							y1 = 238;
						}
					}
				}
				x = x1;
				y = y1;
			}
			blacklength = Lsum/13;
		}
	}
	//------------------------------------------------------------------------------------
	else//方向小于等于90
	{
		if(0 == xh0 || 0 == yh0)
		{
			t=0;
			Gmax=0;
			y1=yh0;
			x1=xh0;
			x=xh0;
			y=yh0;
			length=0;
			for(det=0;det<13;det++)//做10条直线，求长度边界
			{
				xx = xh0;
				yy = yh0;
				Gmax=0;
				if(AAgl>pai*0.25)//角度再45-90之间，xh0=0,yh0=0
				{
					for(j=0;j<239;j++)//寻找每条直线的边界
					{
						if(x+2/k_dir>319 || y+2>239)//超出图像尺寸跳出
							break;
						if(x<20 || y<20)//加入阈值
							t = 0;
						else
							t = Pixel(x+2/k_dir,y+2)-Pixel(x,y);//两相邻点的灰度差
						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1/k_dir;//将点移到下一点
						y = y+1;
					}

					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//用点到线距离求黑龙长度
				}

				else//角度在0-45之间，xh0=0,yh0=0
				{
					for(j=0;j<319;j++)//寻找每条直线的边界
					{
						if(x+2>319 || y+2*k_dir>239)//超出图像尺寸跳出
							break;
						if(x<20 || y<20)//加入阈值
							t = 0;
						else
							t = Pixel(x+2,y+2*k_dir)-Pixel(x,y);//两相邻点的灰度差

						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1;//将点移到下一点
						y = y+k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//用点到线距离求黑龙长度
				}
				Lsum = Lsum+length;

				if(0 == LRboundFlag)//左边
				{
					if(x1 == 0)//在（0，y）线上
					{
						y1 = y1+3;
						if(y1>239)
							break;
					}
					else if(y1 == 0)//在(x,0)线上
					{
						x1 = x1-3;
						if(x1<0)
						{
							x1 = 0;
							y1 = 1;
						}
					}
				}
				else//右边
				{
					if(y1 == 0)
					{
						x1 = x1+3;
						if(x1>319)
							break;
					}
					else if(x1 == 0)
					{
						y1 = y1-3;
						if(y1<0)
						{
							x1 = 1;
							y1 = 0;
						}
					}
				}
				x = x1;
				y = y1;
			}
			blacklength = Lsum/13;
		}

		//----------------------------------------------------------------------------
		else if(319 == xh0 || 239 ==yh0)
		{
			t=0;
			Gmax=0;
			y1=yh0;
			x1=xh0;
			x=xh0;
			y=yh0;
			length=0;
			for(det=0;det<13;det++)//做10条直线，求长度边界
			{
				Gmax=0;
				xx = xh0;
				yy = yh0;
				if(AAgl>pai*0.25)//角度再45-90之间，xh0=319,yh0=239
				{
					for(j=0;j<239;j++)//寻找每条直线的边界
					{
						if(x-2/k_dir<0 || y-2<0)//超出图像尺寸跳出
							break;
						if(y>220 || x>300)//加入阈值
							t = 0;
						else
							t = Pixel(x-2/k_dir,y-2)-Pixel(x,y);//两相邻点的灰度差
						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1/k_dir;//将点移到下一点
						y = y-1;
					}
					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//用点到线距离求黑龙长度
				}
				else//角度在0-45之间，xh0=319,yh0=239
				{
					for(j=0;j<319;j++)//寻找每条直线的边界
					{
						if(x-2<0 || y-2*k_dir<0)//超出图像尺寸跳出
							break;
						if(x>300 || y>220)//加入阈值
							t = 0;
						else
							t = Pixel(x-2,y-2*k_dir)-Pixel(x,y);//两相邻点的灰度差
						if(t>Gmax)//灰度差最大，记录该点
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1;//将点移到下一点
						y = y-k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//用点到线距离求黑龙长度
				}
				Lsum = Lsum+length;
				//计算黑龙左边还是右边
				if(1 == LRboundFlag)//右边
				{
					if(x1 == 319)
					{
						y1 = y1-3;
						if(y1<0)
							break;
					}
					else if(y1 == 239)
					{
						x1 = x1+3;
						if(x1>319)
						{
							x1 = 319;
							y1 = 238;
						}
					}
				}
				else//左边
				{
					if(y1 == 239)
					{
						x1 = x1-3;
						if(x1<0)
							break;
					}
					else if(x1 == 319)
					{
						y1 = y1+3;
						if(y1>239)
						{
							y1 = 239;
							x1 = 318;
						}
					}
				}
				x = x1;
				y = y1;
			}
			blacklength = Lsum/13;
		}
	}
	//---------------------------------------
	return blacklength;
}

/******************************************************************************
* @Name:		AngleOfFlare(BL)
* @Description:	get angle of the flare
* @Param:		black dragon length
* @Return:		Tt
* @Date:		2015-04-05 17:19:19
* ***************************************************************************/
//扩散角处理程序,atan,sqrt
Float AngleOfFlare(Int BLength)
{
	Int i=0,j=0;
	Int det;
	//-------------------------------------------------------------------------------

	Int xylabel;
	Float xincd=0,yincd=0;//x,y每次查找增量
	Int xarray[10];//10个初始点的横坐标
	Int yarray[10];//初始点纵坐标
	Float AOFsum=0;//扩散角
	Float gapx,gapy;//每个初始点之间的差距

	Float xx,yy;
	Int Gmax,t;
	Float x,y;
	Int xp,yp;

	if(AAgl<pai*0.25 || AAgl>=pai*0.75)//简单写一个0-45角度的扩散角
	{
		if(AAgl>=pai*0.75)//135-180
		{
			if(319 == xh0 || 0 == yh0)//,xh0=319,yh0=0
				xylabel = -1;
			else//xh0=0,yh0=239
				xylabel = 1;
		}

		else
		{
			if(0 == xh0 || 0 == yh0)//,xh0=0,yh0=0
				xylabel = 1;
			else//xh0=319,yh0=239
				xylabel = -1;
		}

		xarray[0] = xh0+0.2*BLength/sqrt(1+k_dir*k_dir)*xylabel;//首个初始点的横坐标
		gapx = (xarray[0]-xh0)*2/9;//每个初始点之间的差距
		for(i=1;i<10;i++)//每个初始点的值
			xarray[i] = xarray[i-1]+gapx;
		for(i=0;i<10;i++)
			yarray[i] = k_dir*xarray[i]+b_dir;
		if(AAgl<pai*0.25)
		{
			xincd = -k_dir;//xy的增量
			yincd = 1;
		}
		else
		{
			xincd = k_dir;//xy的增量
			yincd = -1;
		}

	}


	else if(AAgl<pai*0.75)//简单写一个90-135角度的扩散角
	{
		if(AAgl>pai*0.5)//90-135
		{
			if(319 == xh0 || 0 == yh0)//,xh0=319,yh0=0
				xylabel = 1;
			else//xh0=0,yh0=239
				xylabel = -1;
		}

		else//45-90
		{
			if(0 == xh0 || 0 == yh0)//,xh0=0,yh0=0
				xylabel = 1;
			else//xh0=319,yh0=239
				xylabel = -1;
		}

		yarray[0] = yh0+0.2*BLength/sqrt(1+1/(k_dir*k_dir))*xylabel;//首个初始点的横坐标
		gapy = (yarray[0]-yh0)*2/9;//每个初始点之间的差距
		for(i=1;i<10;i++)//每个初始点的值
			yarray[i] = yarray[i-1]+gapy;
		for(i=0;i<10;i++)
			xarray[i] = yarray[i]/k_dir-b_dir/k_dir;
		xincd = -1;//xy的增量
		yincd = 1/k_dir;
	}


	if(0 == LRboundFlag )//左边
	{

		for(det=0;det<10;det++)
		{
			xx=0;
			yy=0;
			Gmax=0;
			t=0;
			x=xarray[det];
			y=yarray[det];
			for(j=0;j<319;j++)
			{
				if(x+xincd<0 || y+yincd<0 || x+xincd>319 || y+yincd>239)
					break;
				t = Pixel(x+xincd,y+yincd)-Pixel(x,y);//两相邻点的灰度差
				if(t>Gmax)//灰度差最大，记录该点
				{
					Gmax = t;
					xx = x;
					yy = y;
				}
				x = x+xincd;//将点移到下一点
				y = y+yincd;
			}

			xp = xx+0.5;
			yp = yy+0.5;

			xx =  sqrt((xx-xarray[det])*(xx-xarray[det])+(yy-yarray[det])*(yy-yarray[det]));//相对于起始点的横坐标
			yy =  sqrt ((Float)((yh0-yarray[det])*(yh0-yarray[det])+(xh0-xarray[det])*(xh0-xarray[det])));//相对于起始点的纵坐标
			Tpixel1stlr(&xx, &yy);
			Tpixel2ndud(&xx, &yy);
			AOFsum+= fast_atan(xx/yy);
		}
		AOFsum=AOFsum/10;
	}
	else
	{
		for(det=0;det<10;det++)
		{
			xx=0;
			yy=0;
			x=xarray[det];
			y=yarray[det];
			Gmax=0;
			t=0;
			for(j=0;j<319;j++)
			{
				if(x-xincd<0 || y-yincd<0 || x-xincd>319 || y-yincd>239)
					break;
				t = Pixel(x-xincd,y-yincd)-Pixel(x,y);//两相邻点的灰度差
				if(t>Gmax)//灰度差最大，记录该点
				{
					Gmax = t;
					xx = x;
					yy = y;
				}
				x = x-xincd;//将点移到下一点
				y = y-yincd;
			}

			xp = xx+0.5;
			yp = yy+0.5;

			xx =  sqrt((xx-xarray[det])*(xx-xarray[det])+(yy-yarray[det])*(yy-yarray[det]));//相对于起始点的横坐标
			yy =  sqrt((Float)((yh0-yarray[det])*(yh0-yarray[det])+(xh0-xarray[det])*(xh0-xarray[det])));//相对于起始点的纵坐标
			Tpixel1stlr(&xx, &yy);
			Tpixel2ndud(&xx, &yy);
			AOFsum+= fast_atan(xx/yy);
		}
		AOFsum=AOFsum/10;
	}

	return AOFsum;
}

/******************************************************************************
* @Name:		MixIntensity(BL)
* @Description:	get mix intensity
* @Param:		relative black dragon length
* @Return:		MI
* @Date:		2015-04-05 17:16:23
* ***************************************************************************/
Float MixIntensity(Int BLength) //混合强度,sqrt,pow
{
	Float wbk = 0;//返回的混合系数
	Int count = 0;//计数，做了多少次小块运算
	Int potall=0;//所有点数
	Int greypot[20];
	Int outFlag;
	Float Allcol;
	Float wbkindi;
	Float avacol;
	Float middlevalue2;
	Int xylabel=0;
	Int x=0,y=0;
	Float d1=0;
	Int xst=0,yst=0,xincd=1,yincd=1;
	Float xh0yh0;//表示中心点在分割线哪边的系数
	Int F1=1,F2=1,F3=1,F4=1;//在分割线哪边的系数
	Int potindi = 256/greynum;
	Int greydegree[20];
	Int Density_N;
	Int Block_SZ;
	Int t,i,j,yi,xj;
	//----------------------------------------------------------------------
	//试写混合强度
	if(BLength<250)
	{
		Density_N=4;
		Block_SZ=2*squareSize;
	}
	else
	{
		Density_N=2;
		Block_SZ=squareSize;
	}
	if(k_dir>1 || k_dir<-1)//45-90,90-135
	{
		if(k_dir<0)//90-135
		{
			if(319 == xh0 || 0 == yh0)//,xh0=319,yh0=0
				xylabel = 1;
			else//xh0=0,yh0=239
				xylabel = -1;
		}

		else//45-90
		{
			if(0 == xh0 || 0 == yh0)//,xh0=0,yh0=0
				xylabel = 1;
			else//xh0=319,yh0=239
				xylabel = -1;
		}
		y = yh0+BLength*xylabel/sqrt(1+1/(k_dir*k_dir));//首点的纵坐标
		x = y/k_dir-b_dir/k_dir;//首个初始点的横坐标
		d1 = x/k_dir+y;//分割线的系数y=-x/k_dir+d1
		xh0yh0 = xh0/k_dir+yh0-d1;//表示中心点在分割线哪边的系数
		F1 = -d1;//0,0
		F2 = 239-d1;//0,239
		F3 = 319/k_dir+239-d1;//319,239
		F4 = 319/k_dir-d1;//319,0
	}
	else//0-45,135-180
	{
		if(k_dir<0)//135-180
		{
			if(319 == xh0 || 0 == yh0)//,xh0=319,yh0=0
				xylabel = -1;
			else//xh0=0,yh0=239
				xylabel = 1;
		}
		else
		{
			if(0 == xh0 || 0 == yh0)//,xh0=0,yh0=0
				xylabel = 1;
			else//xh0=319,yh0=239
				xylabel = -1;
		}
		x = xh0+BLength*xylabel/sqrt(1+k_dir*k_dir);//首个初始点的横坐标
		y = k_dir*x+b_dir;//首点的纵坐标
		d1 = x+k_dir*y;//分割线的系数y=-x/k_dir+d1/k_dir
		xh0yh0 = xh0+k_dir*yh0-d1;//表示中心点在分割线哪边的系数
		F1 = -d1;//0,0
		F2 = 239*k_dir-d1;//0,239
		F3 = 319+239*k_dir-d1;//319,239
		F4 = 319-d1;//319,0

	}
	if(xh0yh0*F1<0)//中心点与0,0在线两边
	{
		xst = 0;//x起始点
		yst = 0;//y起始点
		xincd=1;//x增量
		yincd=1;//y增量
	}
	else if(xh0yh0*F2<0)//中心点与0,239在线两边
	{
		xst = 0;
		yst = 239;
		xincd=1;//x增量
		yincd=-1;//y增量
	}
	else if(xh0yh0*F3<0)//中心点与319,239在线两边
	{
		xst = 319;
		yst = 239;
		xincd=-1;//x增量
		yincd=-1;//y增量
	}
	else if(xh0yh0*F4<0)//中心点与319,0在线两边
	{
		xst = 319;
		yst = 0;
		xincd=-1;//x增量
		yincd=1;//y增量
	}

	for(t=0;t<greynum;t++)
		greydegree[t] = 255-potindi*(t+1);

	for(yi=yst;yi<240 && yi>-1;yi=yi+yincd*Block_SZ )
	{
		if((yincd>0 && yi+Block_SZ>240) ||(yincd<0 && yi-Block_SZ+1<0))//减小超出了范围，或增大超出了范围
			break;
		for(xj=xst;xj<320 && xj>-1;xj=xj+xincd*Block_SZ)
		{
			outFlag=0;
			if((xincd>0 && xj+Block_SZ>320) ||(xincd<0 && xj-Block_SZ+1<0))//减小超出了范围，或增大超出了范围
				break;
			if(k_dir>1 || k_dir<-1)//45-90,90-135
			{
				if(xh0yh0*(xj/k_dir+yi-d1)>=0)//跑到了分割线的另一侧
					//break;
					outFlag=1;
			}
			else
			{
				if(xh0yh0*(xj+k_dir*yi-d1)>=0)//跑到了分割线的另一侧
					//break;
					outFlag=1;
			}
			if(0 == outFlag)
			{

				Allcol=0;
				wbkindi=0;
				for(i=0;i<greynum;i++)
					greypot[i] = 0;//每个区域黑点，白点计数和灰度值变量
				for(i=yi;i<yi+Block_SZ && i>yi-Block_SZ;i=i+Density_N*yincd)//对整个区域做循环，判断每点特性
				{
					for(j=xj;j<xj+Block_SZ && j>xj-Block_SZ;j=j+Density_N*xincd)
					{
						//画出二值图，判断每点是黑是白是灰
						for(t=0;t<greynum;t++)
						{
							if(Pixel0(j,i)>=greydegree[t])
							{
								greypot[t]++;
								break;
							}
						}
					}
				}
				for(i=0;i<greynum;i++)
					Allcol+= greypot[i];

				if(Allcol<1)
					wbkindi = 0;
				else
				{
					avacol = 0.5;
					for(i=greyignore+1;i<greynum;i++)//i=2
					{
						Float middlevalue1 = greypot[greynum-i]/Allcol;
						wbkindi+= avacol*middlevalue1*middlevalue1;
						avacol = avacol/2;
					}
					middlevalue2 = greypot[0]/Allcol-1;
					wbkindi+= avacol*middlevalue2*middlevalue2;
					wbkindi = wbkindi/(greynum-greyignore);
				}
				wbk+= wbkindi;
				count++;
			}
		}
	}
	if(0 == count)
	{
		wbk = 0.5;
	}
	else
	{
		wbk = wbk/count;
	}

	wbk = NMA_k*wbk+100;	/*线性变换 变换到0~100*/

	return wbk;
}

/******************************************************************************
* @Name:		stability()
* @Description:	return the stability of the flare
* @Param:		none
* @Return:		none
* @Date:		2015-04-05 17:10:54
* ***************************************************************************/
Int stability(Float BL)
{
	//稳定性判别
	Int j;
	nextlength = BL;//稳定性判别

	for(j=0;j<wdxavenum;j++)
	{
		wendinglength[j] = wendinglength[j+1];
	}
	wendinglength[wdxavenum-1] = my_abs(nextlength-priorlength)/nextlength;//两幅图黑龙相对差
	priorlength = nextlength;
	nextlength = 0;
	for(j=0;j<wdxavenum;j++)
	{
		nextlength = nextlength+wendinglength[j];
	}
	if(nextlength>Stab_Th*wdxavenum || priorlength<30)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/******************************************************************************
* @Name:		avepro(Float* LLLL,Float* Tt,Float* hunhe)
* @Description:	get corresponding move average values of BL TT and MI
* @Param:		original data from a field of image
* @Return:		none
* @Date:		2015-04-05 11:03:19
* ***************************************************************************/
Void avepro(Float* BL,Float* Tt,Float* MI)
{
	BLarray_LP = (BLarray_LP+1)&0x3F;
	BLarray[BLarray_LP] = *BL;
	BLave_backup += (BLarray[BLarray_LP] - BLarray[(BLarray_LP-BLavenum)&0x3F])/BLavenum;
	*BL = BLave_backup;

	FAarray_LP = (FAarray_LP+1)&0x3F;
	FAarray[FAarray_LP] = *Tt;
	FAave_backup += (FAarray[FAarray_LP] - FAarray[(FAarray_LP-FAavenum)&0x3F])/FAavenum;
	*Tt = FAave_backup;

	MIarray_LP = (MIarray_LP+1)&0x3F;
	MIarray[MIarray_LP] = *MI;
	MIave_backup += (MIarray[MIarray_LP] - MIarray[(MIarray_LP-MIavenum)&0x3F])/MIavenum;
	*MI = MIave_backup;
}
