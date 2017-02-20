#include "tinymath.h"
#include "protocol.h"
#include <math.h>
#include <ti/sdo/dmai/BufferGfx.h>
#include <ti/sdo/dmai/BufTab.h>
//-----------------------------------------------------------------------------------
//��Ҫ�������ⲿ����
Int xh0 = 180,yh0 = 0;//����߽����ĵ㣬(�����˳�ʼֵ)!��Ҫ�ⲿ������!
Float A1Agl = 90;//��ʾ���淽��ĽǶ�!��Ҫ�ⲿ������!
Int BLTFFlag = 0;//������Գ���0�����Գ���1��־
Int coalFlag = 1;//Ͷú��־1Ͷ��0ûͶ
Int ChangeFlag = 0;//��λ���޸����ݱ�־1�ģ�0����
//Int imgFlag = 1;//ͼƬ׼����־1׼�����ˣ�0δ׼����
Int SendFlag = 0;//���ݷ��͸���λ��0�����ͣ�1����
//���ǿ�ȵĲ���
Int squareSize=10;//ÿ��ÿ�зֵĿ�������Ҫ�ⲿ��������
Int greynum = 11;//�Ҷȼ���Ŀ!��Ҫ�ⲿ������!
Int greyignore = 2;//���ż���Ŀ!��Ҫ�ⲿ������!
Int LRboundFlag = 1;//�������ĵ����Ϊ0���ұ�Ϊ1����ʾ����������ұ���!��Ҫ�ⲿ������!
Float NMA_k;
//�Ƕ�����Ĳ���
Float Avideolr=15;//����ͷ�룡��ֱ������ļн�!!!!������б���м�ļнǣ�!��Ҫ�ⲿ������!
Float Avideoud=85;//����ͷ�룡��ֱ������ļн�!!!!������б����ֱ�ļнǣ�!��Ҫ�ⲿ������!
Float Asee=45;//��Ұ�Ž�!��Ҫ�ⲿ������!
Float focus=1;//����!��Ҫ�ⲿ������!
Float Ps=0.001;//��Ԫ�ߴ磡��������������ڻ�ã�ֻ�ǽ���!��Ҫ�ⲿ������!
Float hud=100;//����ͷ�߶ȣ������뾶�ģ�!��Ҫ�ⲿ������!
Float radius=50;//��ڰ뾶��������Ϲд�ģ�!��Ҫ�ⲿ������!
Float hreal;//hud-radius //����ͷ��ʵ����ھ��루�ų���ڰ뾶�ģ�!��Ҫ�ⲿ������!
const Float pai=3.14159265357;//��
//------------------------------------------------------------------------------------
//-----------------------------------------------------------------
//�Ƕȱ任����Ҫ�õ��ı����������в���ֱ����ã������ظ�����
Float TAvud;//�н�����ֵ���Ƕȱ任����
Float TAs;//�ӽ�����ֵ	���Ƕȱ任����
Float TAvlr;//�н�����ֵ���Ƕȱ任����
Float TAvmslr;//�н�-�ӽǵ�����ֵ���Ƕȱ任����
Float hlr;//����ͷ�߶�,���ڣ����ұ任����
//-------------------------------------------------------------------
//---------------------------------------------------------------------
//��һ���ֿ��Էŵ���ʼ������������ǿ�ȣ���ɢ�ǣ����ϵ��������һ����

Float AAgl;//���Ƕ��Ƹĳɻ�����
Float k_dir;//���淽���б��
Float b_dir;//���淽��ĳ���y=k_dir*x+b_dir
Float c_dir;//���洹ֱ������y=-x/k_dir+c��������������ȣ�����k����Ϊ0
Float d_dir;//���洹ֱ������x=-k_dir*y+d��������������ȣ�����k����Ϊ����
//------------------------------------------------------------------------------
//��ƽ������
Float BLarray[64]={0},FAarray[64]={0},MIarray[64]={0};//���������Ҫ������ͱ���
Float BLave_backup = 0;
Float FAave_backup = 0;
Float MIave_backup = 0;
Int BLavenum = 20;//���ٸ���ƽ��,����
Int BLarray_LP=0;
Int FAavenum = 20;//���ٸ���ƽ������ɢ��
Int FAarray_LP=0;
Int MIavenum = 20;//���ٸ���ƽ�������ǿ��
Int MIarray_LP=0;
//---------------------------------------
//-----------------------------------------------------------
//�ȶ����б�,������Ҫ����ѭ�������涨��
Float wendinglength[64]={0};
Int wdxavenum = 20;
Float nextlength = 0;
Float priorlength = 0;
Int Stab_Th = 0.1;//xg
//����Ϊȫ�ֱ���
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
	TAvud=chatan(Avideoud);//�н�����ֵ���Ƕȱ任����
	TAs=chatan(Asee);//�ӽ�����ֵ	���Ƕȱ任����
	TAvlr=chatan(Avideolr);//�н�����ֵ���Ƕȱ任����
	TAvmslr=chatan(Avideolr-Asee);//�н�-�ӽǵ�����ֵ���Ƕȱ任����
	hlr= focus*sqrt(1+TAvlr*TAvlr);//����ͷ�߶�,���ڣ����ұ任����
	//-------------------------------------------------------------------
	//---------------------------------------------------------------------
	//��һ���ֿ��Էŵ���ʼ������������ǿ�ȣ���ɢ�ǣ����ϵ��������һ����
	hreal = hud-radius;
	AAgl = A1Agl/180*pai;//���Ƕ��Ƹĳɻ�����
	k_dir = chatan(A1Agl);//���淽���б��
	b_dir = yh0-k_dir*xh0;//���淽��ĳ���y=kx+b_dir
	c_dir = yh0+xh0/k_dir;//���洹ֱ������y=-x/k_dir+c��������������ȣ�����k����Ϊ0
	d_dir = k_dir*yh0+xh0;//���洹ֱ������x=-k_dir*y+d��������������ȣ�����k����Ϊ����
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

//������ʵ���Ȼ��㺯��,atan,cos,sin
Float RealLength(Int BlackLength,Float Tt)//BlackLengthΪһ�α任������꣬������Ԫ�ߴ磬TtΪ��ɢ�ǽǶ�
{

	Float TB=Avideoud+fast_atan(BlackLength*Ps/focus-TAs);//�Ƕ�B
	Float RealL=hreal*chacos(Tt)*chasin(TB)/chasin(90+Tt-TB);//��ʵ����������ȡ
	return RealL;
}

//���ұ任,sqrt
Void Tpixel1stlr(Float* x, Float* y)
{
	Int u = *x+0.5;
	Int v = *y+0.5;
	*x = hlr*(TAvlr+u*Ps/focus-TAs)/(1+TAvlr*(TAs-u*Ps/focus))-hlr*TAvmslr;//����任��x������
	*y = v*Ps*sqrt((hlr*hlr+(*x+hlr*TAvmslr)*(*x+hlr*TAvmslr))/(focus*focus+(focus*TAs-u*Ps)*(focus*TAs-u*Ps)));//����任��y������
}

//���±任,sqrt
Void Tpixel2ndud(Float* x, Float* y)
{
	Float u = *x;
	Float v = *y;
	*y = hud*(TAvud+v/focus-TAs)/(1+TAvud*(TAs-v/focus));//����任��y������
	*x = u*sqrt((hud*hud+(*y)*(*y))/(focus*focus+(focus*TAs-v)*(focus*TAs-v)));//����任��x������
}
/******************************************************************************
* @Name:		BlackDragon(Void)
* @Description:	get black dragon length
* @Param:		none
* @Return:		relative black dragon length
* @Date:		2015-04-05 17:20:22
* ***************************************************************************/
Int BlackDragon(Void)//�����������,abs,sqrt
{
	//------------------------------------------
	Int Lsum = 0;//�������ȼӺʹ���
	Int xx=0,yy=0;//������
	Int t,Gmax,y1,x1;
	Float x,y,length;
	Int det,j;
	//-------------------------------------------------------------

	Int blacklength = 0;
	if(AAgl>pai*0.5)//�������90
	{
		if(319 == xh0 || 0 == yh0)//���ĵ������Ϊ319����������Ϊ0ʱ
		{

			t=0;
			Gmax=0;
			y1=yh0;
			x1=xh0;
			x=xh0;
			y=yh0;
			length=0;
			for(det=0;det<13;det++)//��10��ֱ�ߣ��󳤶ȱ߽�
			{
				xx = xh0;
				yy = yh0;
				Gmax=0;
				if(AAgl>0.75*pai)//����Ƕȴ���135
				{
					for(j=0;j<319;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x-2<0 || y-2*k_dir>239)//����ͼ��ߴ�����
							break;
						if(x>300 || y<20)//������ֵ
							t = 0;
						else
							t = Pixel(x-2,y-2*k_dir)-Pixel(x,y);//�����ڵ�ĻҶȲ�
						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1;//�����Ƶ���һ��
						y = y-k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//�õ㵽�߾������������
				}
				else//����Ƕ���90-135,xh0=319,yh0=0
				{
					for(j=0;j<239;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x+2/k_dir<0 || y+2>239)//����ͼ��ߴ�����
							break;
						if(y<20 || x>300)//������ֵ
							t = 0;
						else
							t = Pixel(x+2/k_dir,y+2)-Pixel(x,y);//�����ڵ�ĻҶȲ�
						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1/k_dir;//�����Ƶ���һ��
						y = y+1;
					}
					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//�õ㵽�߾������������
				}
				Lsum = Lsum+length;

				if(0 == LRboundFlag)//���
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
				else//�ұ�
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
			for(det=0;det<13;det++)//��10��ֱ�ߣ��󳤶ȱ߽�
			{
				Gmax=0;
				xx = xh0;
				yy = yh0;
				if(AAgl>0.75*pai)//����Ƕȴ���135
				{
					for(j=0;j<319;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x+2>319|| y+2*k_dir<0)//����ͼ��ߴ�����
							break;
						if(x<20 || y>220)//������ֵ
							t = 0;
						else
							t = Pixel(x+2,y+2*k_dir)-Pixel(x,y);//�����ڵ�ĻҶȲ�
						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1;//�����Ƶ���һ��
						y = y+k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//�õ㵽�߾������������
				}
				else//����Ƕ���90-135,xh0=0,yh0=239
				{
					for(j=0;j<239;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x-2/k_dir>319 || y-2<0)//����ͼ��ߴ�����
							break;
						if(y>220 || x<20)//������ֵ
							t = 0;
						else
							t = Pixel(x-2/k_dir,y-2)-Pixel(x,y);//�����ڵ�ĻҶȲ�
						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1/k_dir;//�����Ƶ���һ��
						y = y-1;
					}
					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//�õ㵽�߾������������
				}
				Lsum = Lsum+length;
				//���������߻����ұ�
				if(1 == LRboundFlag)//�ұ�
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
				else//���
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
	else//����С�ڵ���90
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
			for(det=0;det<13;det++)//��10��ֱ�ߣ��󳤶ȱ߽�
			{
				xx = xh0;
				yy = yh0;
				Gmax=0;
				if(AAgl>pai*0.25)//�Ƕ���45-90֮�䣬xh0=0,yh0=0
				{
					for(j=0;j<239;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x+2/k_dir>319 || y+2>239)//����ͼ��ߴ�����
							break;
						if(x<20 || y<20)//������ֵ
							t = 0;
						else
							t = Pixel(x+2/k_dir,y+2)-Pixel(x,y);//�����ڵ�ĻҶȲ�
						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1/k_dir;//�����Ƶ���һ��
						y = y+1;
					}

					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//�õ㵽�߾������������
				}

				else//�Ƕ���0-45֮�䣬xh0=0,yh0=0
				{
					for(j=0;j<319;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x+2>319 || y+2*k_dir>239)//����ͼ��ߴ�����
							break;
						if(x<20 || y<20)//������ֵ
							t = 0;
						else
							t = Pixel(x+2,y+2*k_dir)-Pixel(x,y);//�����ڵ�ĻҶȲ�

						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x+1;//�����Ƶ���һ��
						y = y+k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//�õ㵽�߾������������
				}
				Lsum = Lsum+length;

				if(0 == LRboundFlag)//���
				{
					if(x1 == 0)//�ڣ�0��y������
					{
						y1 = y1+3;
						if(y1>239)
							break;
					}
					else if(y1 == 0)//��(x,0)����
					{
						x1 = x1-3;
						if(x1<0)
						{
							x1 = 0;
							y1 = 1;
						}
					}
				}
				else//�ұ�
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
			for(det=0;det<13;det++)//��10��ֱ�ߣ��󳤶ȱ߽�
			{
				Gmax=0;
				xx = xh0;
				yy = yh0;
				if(AAgl>pai*0.25)//�Ƕ���45-90֮�䣬xh0=319,yh0=239
				{
					for(j=0;j<239;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x-2/k_dir<0 || y-2<0)//����ͼ��ߴ�����
							break;
						if(y>220 || x>300)//������ֵ
							t = 0;
						else
							t = Pixel(x-2/k_dir,y-2)-Pixel(x,y);//�����ڵ�ĻҶȲ�
						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1/k_dir;//�����Ƶ���һ��
						y = y-1;
					}
					length = my_abs(xx/k_dir+yy-c_dir)/sqrt(1/(k_dir*k_dir)+1);//�õ㵽�߾������������
				}
				else//�Ƕ���0-45֮�䣬xh0=319,yh0=239
				{
					for(j=0;j<319;j++)//Ѱ��ÿ��ֱ�ߵı߽�
					{
						if(x-2<0 || y-2*k_dir<0)//����ͼ��ߴ�����
							break;
						if(x>300 || y>220)//������ֵ
							t = 0;
						else
							t = Pixel(x-2,y-2*k_dir)-Pixel(x,y);//�����ڵ�ĻҶȲ�
						if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
						{
							Gmax = t;
							xx = x;
							yy = y;
						}
						x = x-1;//�����Ƶ���һ��
						y = y-k_dir;
					}
					length = my_abs(xx+k_dir*yy-d_dir)/sqrt(k_dir*k_dir+1);//�õ㵽�߾������������
				}
				Lsum = Lsum+length;
				//���������߻����ұ�
				if(1 == LRboundFlag)//�ұ�
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
				else//���
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
//��ɢ�Ǵ������,atan,sqrt
Float AngleOfFlare(Int BLength)
{
	Int i=0,j=0;
	Int det;
	//-------------------------------------------------------------------------------

	Int xylabel;
	Float xincd=0,yincd=0;//x,yÿ�β�������
	Int xarray[10];//10����ʼ��ĺ�����
	Int yarray[10];//��ʼ��������
	Float AOFsum=0;//��ɢ��
	Float gapx,gapy;//ÿ����ʼ��֮��Ĳ��

	Float xx,yy;
	Int Gmax,t;
	Float x,y;
	Int xp,yp;

	if(AAgl<pai*0.25 || AAgl>=pai*0.75)//��дһ��0-45�Ƕȵ���ɢ��
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

		xarray[0] = xh0+0.2*BLength/sqrt(1+k_dir*k_dir)*xylabel;//�׸���ʼ��ĺ�����
		gapx = (xarray[0]-xh0)*2/9;//ÿ����ʼ��֮��Ĳ��
		for(i=1;i<10;i++)//ÿ����ʼ���ֵ
			xarray[i] = xarray[i-1]+gapx;
		for(i=0;i<10;i++)
			yarray[i] = k_dir*xarray[i]+b_dir;
		if(AAgl<pai*0.25)
		{
			xincd = -k_dir;//xy������
			yincd = 1;
		}
		else
		{
			xincd = k_dir;//xy������
			yincd = -1;
		}

	}


	else if(AAgl<pai*0.75)//��дһ��90-135�Ƕȵ���ɢ��
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

		yarray[0] = yh0+0.2*BLength/sqrt(1+1/(k_dir*k_dir))*xylabel;//�׸���ʼ��ĺ�����
		gapy = (yarray[0]-yh0)*2/9;//ÿ����ʼ��֮��Ĳ��
		for(i=1;i<10;i++)//ÿ����ʼ���ֵ
			yarray[i] = yarray[i-1]+gapy;
		for(i=0;i<10;i++)
			xarray[i] = yarray[i]/k_dir-b_dir/k_dir;
		xincd = -1;//xy������
		yincd = 1/k_dir;
	}


	if(0 == LRboundFlag )//���
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
				t = Pixel(x+xincd,y+yincd)-Pixel(x,y);//�����ڵ�ĻҶȲ�
				if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
				{
					Gmax = t;
					xx = x;
					yy = y;
				}
				x = x+xincd;//�����Ƶ���һ��
				y = y+yincd;
			}

			xp = xx+0.5;
			yp = yy+0.5;

			xx =  sqrt((xx-xarray[det])*(xx-xarray[det])+(yy-yarray[det])*(yy-yarray[det]));//�������ʼ��ĺ�����
			yy =  sqrt ((Float)((yh0-yarray[det])*(yh0-yarray[det])+(xh0-xarray[det])*(xh0-xarray[det])));//�������ʼ���������
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
				t = Pixel(x-xincd,y-yincd)-Pixel(x,y);//�����ڵ�ĻҶȲ�
				if(t>Gmax)//�ҶȲ���󣬼�¼�õ�
				{
					Gmax = t;
					xx = x;
					yy = y;
				}
				x = x-xincd;//�����Ƶ���һ��
				y = y-yincd;
			}

			xp = xx+0.5;
			yp = yy+0.5;

			xx =  sqrt((xx-xarray[det])*(xx-xarray[det])+(yy-yarray[det])*(yy-yarray[det]));//�������ʼ��ĺ�����
			yy =  sqrt((Float)((yh0-yarray[det])*(yh0-yarray[det])+(xh0-xarray[det])*(xh0-xarray[det])));//�������ʼ���������
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
Float MixIntensity(Int BLength) //���ǿ��,sqrt,pow
{
	Float wbk = 0;//���صĻ��ϵ��
	Int count = 0;//���������˶��ٴ�С������
	Int potall=0;//���е���
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
	Float xh0yh0;//��ʾ���ĵ��ڷָ����ıߵ�ϵ��
	Int F1=1,F2=1,F3=1,F4=1;//�ڷָ����ıߵ�ϵ��
	Int potindi = 256/greynum;
	Int greydegree[20];
	Int Density_N;
	Int Block_SZ;
	Int t,i,j,yi,xj;
	//----------------------------------------------------------------------
	//��д���ǿ��
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
		y = yh0+BLength*xylabel/sqrt(1+1/(k_dir*k_dir));//�׵��������
		x = y/k_dir-b_dir/k_dir;//�׸���ʼ��ĺ�����
		d1 = x/k_dir+y;//�ָ��ߵ�ϵ��y=-x/k_dir+d1
		xh0yh0 = xh0/k_dir+yh0-d1;//��ʾ���ĵ��ڷָ����ıߵ�ϵ��
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
		x = xh0+BLength*xylabel/sqrt(1+k_dir*k_dir);//�׸���ʼ��ĺ�����
		y = k_dir*x+b_dir;//�׵��������
		d1 = x+k_dir*y;//�ָ��ߵ�ϵ��y=-x/k_dir+d1/k_dir
		xh0yh0 = xh0+k_dir*yh0-d1;//��ʾ���ĵ��ڷָ����ıߵ�ϵ��
		F1 = -d1;//0,0
		F2 = 239*k_dir-d1;//0,239
		F3 = 319+239*k_dir-d1;//319,239
		F4 = 319-d1;//319,0

	}
	if(xh0yh0*F1<0)//���ĵ���0,0��������
	{
		xst = 0;//x��ʼ��
		yst = 0;//y��ʼ��
		xincd=1;//x����
		yincd=1;//y����
	}
	else if(xh0yh0*F2<0)//���ĵ���0,239��������
	{
		xst = 0;
		yst = 239;
		xincd=1;//x����
		yincd=-1;//y����
	}
	else if(xh0yh0*F3<0)//���ĵ���319,239��������
	{
		xst = 319;
		yst = 239;
		xincd=-1;//x����
		yincd=-1;//y����
	}
	else if(xh0yh0*F4<0)//���ĵ���319,0��������
	{
		xst = 319;
		yst = 0;
		xincd=-1;//x����
		yincd=1;//y����
	}

	for(t=0;t<greynum;t++)
		greydegree[t] = 255-potindi*(t+1);

	for(yi=yst;yi<240 && yi>-1;yi=yi+yincd*Block_SZ )
	{
		if((yincd>0 && yi+Block_SZ>240) ||(yincd<0 && yi-Block_SZ+1<0))//��С�����˷�Χ�������󳬳��˷�Χ
			break;
		for(xj=xst;xj<320 && xj>-1;xj=xj+xincd*Block_SZ)
		{
			outFlag=0;
			if((xincd>0 && xj+Block_SZ>320) ||(xincd<0 && xj-Block_SZ+1<0))//��С�����˷�Χ�������󳬳��˷�Χ
				break;
			if(k_dir>1 || k_dir<-1)//45-90,90-135
			{
				if(xh0yh0*(xj/k_dir+yi-d1)>=0)//�ܵ��˷ָ��ߵ���һ��
					//break;
					outFlag=1;
			}
			else
			{
				if(xh0yh0*(xj+k_dir*yi-d1)>=0)//�ܵ��˷ָ��ߵ���һ��
					//break;
					outFlag=1;
			}
			if(0 == outFlag)
			{

				Allcol=0;
				wbkindi=0;
				for(i=0;i<greynum;i++)
					greypot[i] = 0;//ÿ������ڵ㣬�׵�����ͻҶ�ֵ����
				for(i=yi;i<yi+Block_SZ && i>yi-Block_SZ;i=i+Density_N*yincd)//������������ѭ�����ж�ÿ������
				{
					for(j=xj;j<xj+Block_SZ && j>xj-Block_SZ;j=j+Density_N*xincd)
					{
						//������ֵͼ���ж�ÿ���Ǻ��ǰ��ǻ�
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

	wbk = NMA_k*wbk+100;	/*���Ա任 �任��0~100*/

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
	//�ȶ����б�
	Int j;
	nextlength = BL;//�ȶ����б�

	for(j=0;j<wdxavenum;j++)
	{
		wendinglength[j] = wendinglength[j+1];
	}
	wendinglength[wdxavenum-1] = my_abs(nextlength-priorlength)/nextlength;//����ͼ������Բ�
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
