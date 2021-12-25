/////////////////////////////////////////////////////////
// 程序名称：2.5D 赛车
// 编译环境：Visual Studio 2022，EasyX_20211109
// 作　　者：极品史莱姆 (799052200@qq.com)
// 修 改 者: huidong <huidong_mail@163.com>
// 发布日期：2019-2-4
// 最后修改：2021.12.25
//
#include <graphics.h>      // 引用图形库头文件
#include <ctime>
#include <sstream>
#include <fstream>
#include <vector>
#include <conio.h>
// 播放 MP3 所需
#include <mmsystem.h>
#pragma comment(lib,"Winmm.lib")

using namespace std;

#define	CMD_UP			1
#define	CMD_DOWN		2
#define	CMD_LEFT		4
#define	CMD_RIGHT		8
#define	CMD_SHIFT		512
#define	sCMD_UP			16
#define	sCMD_DOWN		32
#define	sCMD_LEFT		64
#define	sCMD_RIGHT		128
#define	CMD_QUIT		256
#define PI				3.1415926

double MaxSpeed = 30;		// 最大速度
int FinSand = 5;			// 在沙上的摩擦力
int FinRoad = 1;			// 在路上的摩擦力
int FinIce = -2;			// 在冰上的摩擦力
double SpeedAdd = PI / 2;	// 加速度
int Rota = /*64*/80;		// 转动速度的 -1 次方
int NeedR = 5;				// 目标圈数

int WIDE = 1280;
int HEIGHT = 960;
double EndLineForward = 0;	// 终点角度

bool inIce;
bool inRoad;
bool inSand;
bool inWall;
bool inEndline;

IMAGE Racing;				// 赛场地图
IMAGE Toucher;				// 碰撞图
IMAGE car1;
IMAGE car2;

IMAGE Player1;
int Px = 150;
int Py = 150;
double PForward = 0;		// 方向
double Pspeed = 0;			// 速度
int Ppass = 0;				// 通过几次终点
bool Pwrong = false;		// 是否逆行
bool PHadPass = false;		// 是否通过终点
bool PWaitOut = false;		// 是否等待通过终点
bool Pover = false;			// 是否结束
clock_t Ptime = 0;
clock_t Ptime2 = 0;

IMAGE Player2;
int Cx = 170;
int Cy = 170;
double CForward = 0;
double Cspeed = 0;
int Cpass = 0;
bool Cwrong = false;
bool CHadPass = false;
bool CWaitOut = false;
bool Cover = false;
clock_t Ctime = 0;
clock_t Ctime2 = 0;

bool TwoPlayer = true;

bool isres = true;
bool chexit = false;

bool MeumMod = false;

clock_t Start = 0;
clock_t Now = 0;
clock_t MeumUsed = 0;

struct bottom				// 简易的按钮实现
{
	int ID;

	int x;
	int y;
	int wide;
	int heigh;
	RECT a;
	wstring str;

	COLORREF fillcolor;
	COLORREF linecolor;
	COLORREF textcolor;
	LOGFONT textstyle;
	UINT uFormat;

	bottom(int gID, int gx, int gy, int gw, int gh, wstring gs)
	{
		fillcolor = getfillcolor();
		linecolor = getlinecolor();
		textcolor = gettextcolor();
		gettextstyle(&textstyle);
		uFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
		ID = gID;
		x = gx;
		y = gy;
		wide = gw;
		heigh = gh;
		str = gs;
		a = { x, y, x + wide, y + heigh };
	}
};

struct page
{
	vector<bottom> botlist;

	bool MouseTouch(int left, int top, int right, int bottom, MOUSEMSG m)	// 鼠标区域判定
	{
		for (int i1 = left; i1 < right; i1++)
		{
			for (int i2 = top; i2 < bottom; i2++)
			{
				if (m.x == i1 && m.y == i2)
				{
					return true;
				}
			}
		}
		return false;
	}

	int ShownPage()														// 显示并等待按键被响应，返回相应的ID值
	{
		COLORREF fillcolor = getfillcolor();
		COLORREF linecolor = getlinecolor();
		COLORREF textcolor = gettextcolor();
		LOGFONT textstyle;
		gettextstyle(&textstyle);

		MOUSEMSG m;
		setbkmode(TRANSPARENT);

		for (unsigned int i = 0; i < botlist.size(); i++)
		{
			setfillcolor(botlist[i].fillcolor);
			setlinecolor(botlist[i].linecolor);
			settextcolor(botlist[i].textcolor);
			settextstyle(&botlist[i].textstyle);

			fillrectangle(botlist[i].x, botlist[i].y, botlist[i].x + botlist[i].wide, botlist[i].y + botlist[i].heigh);

			drawtext(botlist[i].str.c_str(), &botlist[i].a, botlist[i].uFormat);
		}
		FlushBatchDraw();

		while (true)
		{
			FlushMouseMsgBuffer();
			m = GetMouseMsg();
			if (m.mkLButton)
			{
				for (unsigned int i = 0; i < botlist.size(); i++)
				{
					if (MouseTouch(botlist[i].x, botlist[i].y, botlist[i].x + botlist[i].wide, botlist[i].y + botlist[i].heigh, m))
					{
						return botlist[i].ID;
					}
				}
			}
		}

		setfillcolor(fillcolor);
		setlinecolor(linecolor);
		settextcolor(textcolor);
		settextstyle(&textstyle);
	}
};

struct intro	// 地图的介绍信息
{
	wstring filename;
	wstring title;
	wstring intr;
	wstring inipath;
};
vector<intro> IntroList;

class timer		// 计时器
{
private:
	bool is_start = false;
	clock_t start;
public:
	bool WaitFor(clock_t s)
	{
		if (is_start)
		{
			if ((start + s) <= clock())
			{
				is_start = false;
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			start = clock();
			is_start = true;
			return false;
		}
	}
};

void init();
void gaming();
int GetCommand();
void DispatchCommand(int _cmd);
void OnLeft(bool player);		// false 玩家 1，true 玩家 2
void OnRight(bool player);
void OnUp(bool player);
void OnDown(bool player);
void OnShift(bool player);
void MoveCheck(bool player);	// 碰撞判定
int PointTsm(int x, int y, int wide, int high);	// 坐标与数值的转换
void Draw();
void End();
void PutImgWithout(IMAGE &obj, int px, int py, COLORREF withouter, DWORD* pbWnd, int wX, int wY, DWORD bitsub);	// 放置图片，除了
void SetBirth();	// 第一次读取
void StartWord();
void Loading();		// 加载地图
int ChooseMap();	// 选择地图
void LoadIntro(string File);
BOOL SearchFilesByWildcard(string wildcardPath);	// 搜索文件，参考自https://blog.csdn.net/faithzzf/article/details/54290084
IMAGE zoomImage(IMAGE* pImg, int newWidth, int newHeight);	// 图片缩放
void showhelp();	// 显示帮助文件
void clean();		// 清空缓冲区
void restart();		// 用于重新开始游戏
bool CanRota (bool player);//是否可以旋转
