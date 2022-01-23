/////////////////////////////////////////////////////////
// 程序名称：2.5D 赛车
// 编译环境：Visual Studio 2022，EasyX_20211109
// 作　　者：极品史莱姆 (799052200@qq.com)
// 修 改 者: huidong <huidong_mail@163.com>
// 修改版本: Ver 2.0
// 发布日期：2019.02.04
// 最后修改：2022.01.23
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

// 直线方程
struct Line
{
	double k = 0;	// 系数，为 0 表示横线（常函数）

	// 直线不为竖线时，此值表示竖直偏移量
	// 直线为竖线时，此值表示水平偏移量
	double b = 0;

	bool isVertical = false;	// 是否为竖线
};

// 是否开启原版视角
bool isUseOriginal = false;

// 是否开启 2.5D 的透视效果
bool isPerspectiveEffect = true;

double MaxSpeed = 30;		// 最大速度（单位混乱）
int FinSand = 5;			// 在沙上的摩擦力
int FinRoad = 1;			// 在路上的摩擦力
int FinIce = -2;			// 在冰上的摩擦力
double SpeedAdd = PI / 2;	// 加速度
int Rota_base = 64;			// 基础转速（转动速度的 -1 次方）
int Rota = 64;				// 实际转速（加速可提高转速）
int NeedR = 2;				// 目标圈数

// 窗口大小
int WIDTH = 1280;
int HEIGHT = 960;

// 最大窗口大小（即初始窗口大小）
int nMaxW = 1280;
int nMaxH = 960;

// 窗体缩放比（用于低分辨率模式）
double dWidthZoom = 1;
double dHeightZoom = 1;

double EndLineForward = 0;	// 终点角度
POINT pEndLinePoints[2] = { {0,0},{0,0} };	// 终点的两端点位置
Line pEndLine;	// 终点线方程

bool inIce;
bool inRoad;
bool inSand;
bool inWall;
bool inEndline;

IMAGE Racing;				// 赛场地图
IMAGE Mask;					// 碰撞图
IMAGE car1;
IMAGE car2;

// 每个等级的时间目标（秒）
int nLevelTime[3] = { 100, 70, 50 };

IMAGE imgWinFlag;	// 获胜旗帜
IMAGE imgStar[2];	// 五角星（0 未点亮，1 点亮）

int nMapW;	// 地图宽
int nMapH;	// 地图高

IMAGE Player1;
int Px = 150;
int Py = 150;
double PForward = 0;		// 方向
double Pspeed = 0;			// 速度（单位混乱）
int PRota = 64;				// 转速（转向速度）
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
int CRota = 64;
int Cpass = 0;
bool Cwrong = false;
bool CHadPass = false;
bool CWaitOut = false;
bool Cover = false;
clock_t Ctime = 0;
clock_t Ctime2 = 0;

bool TwoPlayer = false;

bool isres = true;
bool chexit = false;

bool MeumMod = false;

clock_t Start = 0;
clock_t Now = 0;
clock_t MeumUsed = 0;
clock_t Processing = 0;	// 程序每次开始绘制、处理消息的时刻

// 点是否位于矩形内
bool isInRect(int x, int y, RECT rct)
{
	if (rct.left > rct.right)	std::swap(rct.left, rct.right);
	if (rct.top > rct.bottom)	std::swap(rct.top, rct.bottom);
	return x >= rct.left && x <= rct.right && y >= rct.top && y <= rct.bottom;
}

struct button				// 简易的按钮实现
{
	int ID;

	int x;
	int y;
	int width;
	int height;
	wstring str;

	COLORREF fillcolor;
	COLORREF linecolor;
	COLORREF textcolor;
	LOGFONT textstyle;
	UINT uFormat;

	button(int gID, int gx, int gy, int gw, int gh, wstring gs)
	{
		fillcolor = getfillcolor();
		linecolor = getlinecolor();
		textcolor = gettextcolor();
		gettextstyle(&textstyle);
		uFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
		ID = gID;
		x = gx;
		y = gy;
		width = gw;
		height = gh;
		str = gs;
	}
};

struct page
{
	vector<button> botlist;

	bool MouseTouch(int left, int top, int right, int bottom, MOUSEMSG m)	// 鼠标区域判定
	{
		return isInRect(m.x, m.y, { left,top,right,bottom });
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

			fillrectangle(botlist[i].x, botlist[i].y, botlist[i].x + botlist[i].width, botlist[i].y + botlist[i].height);

			RECT rct = { botlist[i].x, botlist[i].y, botlist[i].x + botlist[i].width, botlist[i].y + botlist[i].height };
			drawtext(botlist[i].str.c_str(), &rct, botlist[i].uFormat);
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
					if (MouseTouch(botlist[i].x, botlist[i].y, botlist[i].x + botlist[i].width, botlist[i].y + botlist[i].height, m))
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
int PointTsm(int x, int y, int width, int high);	// 坐标与数值的转换
void Draw();
void End();
void PutImgWithout(IMAGE& obj, int px, int py, COLORREF withouter, DWORD* pbWnd, int wX, int wY, DWORD bitsub);	// 放置图片，除了
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
bool CanRota(bool player);//是否可以旋转
