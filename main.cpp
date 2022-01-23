//////////////////////////////////////////////
//
//	2.5D 赛车
//
//	改编自 极品史莱姆 (799052200@qq.com)
//	详见 define.h
//

#include "NewDrawer.h"

void LoadIntro(wstring File)
{
	intro a;
	a.inipath = L"map/" + File + L"/set.ini";
	wchar_t tmp[30];								//缓冲区
	a.filename = File;

	GetPrivateProfileString(L"Intro", L"title", L"", tmp, 30, a.inipath.c_str());
	a.title = tmp;
	GetPrivateProfileString(L"Intro", L"intr", L"", tmp, 30, a.inipath.c_str());
	a.intr = tmp;

	IntroList.push_back(a);
}

BOOL SearchFilesByWildcard(wstring wildcardPath)	// 查找文件
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA pNextInfo;

	hFile = FindFirstFile(wildcardPath.c_str(), &pNextInfo);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}

	WCHAR infPath[MAX_PATH] = { 0 };
	if (pNextInfo.cFileName[0] != '.')
	{
		LoadIntro(pNextInfo.cFileName);
	}

	while (FindNextFile(hFile, &pNextInfo))
	{
		if (pNextInfo.cFileName[0] == '.')
		{
			continue;
		}

		LoadIntro(pNextInfo.cFileName);
	}

	return TRUE;
}

MOUSEMSG m;
bool MouseTouch(int left, int top, int right, int button)
{
	for (int i1 = left; i1 < right; i1++)
	{
		for (int i2 = top; i2 < button; i2++)
		{
			if (m.x == i1 && m.y == i2)
			{
				return true;
			}
		}
	}
	return false;
}

// 选择地图，返回地图序号，-1 表示取消选择
int ChooseMap()
{
	// 是否已经加载过地图
	static bool isLoad = false;
	if (IntroList.size() == 0)
	{
		isLoad = false;
	}

	RECT r;
	cleardevice();
	wchar_t tmp[30];				//缓冲区
	wstring tw;						//对标题的处理

	if (isLoad || SearchFilesByWildcard(L"map/*"))
	{
		isLoad = true;

		setbkmode(TRANSPARENT);
		setlinecolor(LIGHTBLUE);
		settextcolor(BLACK);

		for (int i = 0; i < (int)IntroList.size(); i++)
		{
			IMAGE t1;				//存放临时的图片用于缩放

			fillrectangle(10, 60 + i * 120, 1260, 170 + i * 120);

			GetPrivateProfileString(L"File", L"Titlepic", L"", tmp, 30, IntroList[i].inipath.c_str());//读取或生成缩略图
			tw = tmp;
			if (tw == L"defpic")
			{
				GetPrivateProfileString(L"File", L"Racing", L"", tmp, 30, IntroList[i].inipath.c_str());
				loadimage(&t1, (L"map\\" + IntroList[i].filename + L"\\" + tmp).c_str());
				IMAGE img = zoomImage(&t1, 100, 100);
				putimage(10 + 5, 60 + i * 120 + 5, &img);
			}
			else
			{
				loadimage(&t1, (L"map\\" + IntroList[i].filename + L"\\" + tmp).c_str());
				IMAGE img = zoomImage(&t1, 100, 100);
				putimage(10 + 5, 60 + i * 120 + 5, &img);
			}



			r = { 120, 60 + i * 120 + 5, 1260, 60 + i * 120 + 25 };
			drawtext((L"标题  " + IntroList[i].title).c_str(), &r, DT_WORDBREAK | DT_LEFT);
			r = { 120, 60 + i * 120 + 30, 1260, 60 + i * 120 + 80 };
			drawtext((L"介绍  " + IntroList[i].intr).c_str(), &r, DT_WORDBREAK | DT_LEFT | DT_WORD_ELLIPSIS);
			r = { 120, 60 + i * 120 + 85, 1260, 60 + i * 120 + 105 };
			drawtext((L"文件夹  " + IntroList[i].filename).c_str(), &r, DT_WORDBREAK | DT_LEFT);
		}

		settextcolor(WHITE);
		r = { 0, 0, 1280, 60 };
		drawtext(L"选择地图", &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		while (true)
		{
			FlushMouseMsgBuffer();
			while (true)
			{
				if (MouseHit())
				{
					m = GetMouseMsg();
					break;
				}

				if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
				{
					return -1;
				}
			}

			if (m.mkLButton)
			{
				for (unsigned int i = 0; i < IntroList.size(); i++)
				{
					if (MouseTouch(10, 60 + i * 120, 1260, 170 + i * 120))
					{
						cleardevice();
						return i;
					}
				}
			}
		}
	}
	else
	{
		MessageBox(NULL, L"找不到地图文件", L"错误", MB_OK | MB_ICONWARNING);
		exit(0);
		return 0;
	}
}

// 返回一个二维点在一维数组中的下标
// 若坐标越界则将缩放到边界上最近一点
int PointTsm(int x, int y, int width, int height)
{
	if (x < 0)		x = 0;
	if (x >= width)	x = width;
	if (y < 0)		y = 0;
	if (y >= height)y = height;

	return width * y + x;
}

// 根据两点计算直线方程
Line GetLineOfPoints(POINT p1, POINT p2)
{
	Line l;
	if (p1.x == p2.x)
	{
		l.isVertical = true;
		l.b = p1.x;
	}
	else
	{
		l.k = (double)(p1.y - p2.y) / (p1.x - p2.x);
		l.b = (double)p1.y - l.k * p1.x;
	}

	return l;
}

// 获取两直线交点
// 返回 -1 表示无交点（两条竖线不重叠）
// 返回 0 表示有一个交点
// 返回 1 表示有无穷个交点（两条竖线重叠）
int GetLinesIntersection(Line l1, Line l2, POINT* p)
{
	// 若存在至少一条竖线的解
	Line pl[2] = { l1,l2 };
	for (int i = 0; i < 2; i++)
	{
		if (pl[i].isVertical)
		{
			if (pl[!i].isVertical)
			{
				if (pl[i].b == pl[!i].b)
				{
					*p = { (LONG)pl[i].b,0 };
					return 1;
				}
				else
				{
					return -1;
				}
			}
			else
			{
				*p = { (LONG)pl[i].b,(LONG)(pl[!i].k * pl[i].b + pl[!i].b) };
				return 0;
			}
		}
	}

	// 两条斜线的解
	double x = (l2.b - l1.b) / (l1.k - l2.k);
	double y = l1.k * x + l1.b;
	*p = { (LONG)x,(LONG)y };
	return 0;
}

// 过线检测
// 由于可能车速太快直接穿越终点线导致没有检测到过线
// 使用此函数专门检验是否过线
// p1, p2: 连续两次获取的玩家坐标
// 返回是否穿过终点线
bool CheckPass(POINT p1, POINT p2)
{
	Line l = GetLineOfPoints(p1, p2);
	POINT p = { 0,0 };
	if (GetLinesIntersection(l, pEndLine, &p) == 0)
	{
		if (isInRect(p.x, p.y, { p1.x,p1.y,p2.x,p2.y }) &&
			isInRect(p.x, p.y, { pEndLinePoints[0].x,pEndLinePoints[0].y,pEndLinePoints[1].x,pEndLinePoints[1].y }))
		{
			return true;
		}
	}

	return false;
}

// 玩家移动检测及消息处理
// player: false 表示玩家 1，true 表示玩家 2
void MoveCheck(bool player)
{
	// 上一帧的小车位置
	static POINT pPrevious[2] = { {-1,-1},{-1,-1} };

	bool PinLine = false;
	bool CinLine = false;

	int rtmp = 1;

	inIce = false;
	inRoad = false;
	inSand = false;
	inWall = false;
	inEndline = false;
	DWORD* pbTch = GetImageBuffer(&Mask);
	DWORD* pCar;
	double x;
	double y;
	int w;
	int h;
	double SpeedChange = 0;
	COLORREF c;

	// 程序绘图、处理消息耗时（秒）
	float fProcessingTime = 0;
	if (Processing != 0)
	{
		fProcessingTime = (clock() - Processing) / (float)CLOCKS_PER_SEC;
	}
	float fMultiple = 1 + fProcessingTime;

	if (!player)
	{
		x = Pspeed * cos(PForward) * fMultiple + Px;
		y = Pspeed * sin(PForward) * fMultiple + Py;
		DWORD* pbImg = GetImageBuffer(&Player1);
		pCar = pbImg;
		w = Player1.getwidth();
		h = Player1.getheight();
	}
	else
	{
		x = Cspeed * cos(CForward) * fMultiple + Cx;
		y = Cspeed * sin(CForward) * fMultiple + Cy;
		DWORD* pbImg = GetImageBuffer(&Player2);
		pCar = pbImg;
		w = Player2.getwidth();
		h = Player2.getheight();
	}

	int i;
	int j;

	// 获取所在的地面类型
	for (i = 0; i < w; i++)
	{
		for (j = 0; j < h; j++)
		{
			if ((pCar[PointTsm(i, j, w, h)] & 0x00FFFFFF) == WHITE)
			{
				continue;
			}

			c = BGR(pbTch[PointTsm(i + (int)x, j + (int)y, nMapW, nMapH)] & 0x00FFFFFF);
			switch (c)
			{
			case BLACK:				inRoad = true;	break;
			case RGB(255, 255, 0):	inSand = true;	break;
			case RGB(0, 0, 255):	inIce = true;	break;
			case RGB(255, 0, 0):	inWall = true;	break;
			case RGB(0, 255, 0):	inEndline = true; inRoad = true;	break;
			}
		}
	}

	// 单独检验过线
	if (!player && pPrevious[0].x > 0)
	{
		if (CheckPass(pPrevious[0], { Px, Py }))
		{
			inEndline = true;
			inRoad = true;
		}
	}
	if (player && pPrevious[1].x > 0)
	{
		if (CheckPass(pPrevious[1], { Cx, Cy }))
		{
			inEndline = true;
			inRoad = true;
		}
	}

	//对所在地面类型进行操作
	if (inSand)
	{
		SpeedChange -= FinSand;
	}
	if (inRoad)
	{
		SpeedChange -= FinRoad;
	}
	if (inIce)
	{
		SpeedChange -= FinIce;
	}
	if (inEndline)
	{
		if (!player)
		{
			PinLine = true;
			if (cos(PForward - EndLineForward) * Pspeed < 0)	// 逆行
			{
				inWall = true;
				Pwrong = true;
				Ptime2 = Now;
			}
			else
			{
				if (!PHadPass)	PHadPass = true;
			}
		}
		else
		{
			CinLine = true;
			if (cos(CForward - EndLineForward) * Cspeed < 0)	// 逆行
			{
				inWall = true;
				Cwrong = true;
				Ctime2 = Now;
			}
			else
			{
				if (!CHadPass)CHadPass = true;
			}
		}
	}
	if (!player)
	{
		if (inWall)
		{
			Pspeed = 0;
			return;
		}
		if (-SpeedChange < abs(Pspeed))
		{
			SpeedChange += abs(Pspeed);
			if (Pspeed > 0)
			{
				Pspeed = SpeedChange;
			}
			else
			{
				Pspeed = -SpeedChange;
			}
		}
		else Pspeed = 0;
		if (inWall)
		{
			/*while (c == BGR(0x0000FF))
			{
				Px += (int)round(cos(PForward));
				Py += (int)round(sin(PForward));
				for (int i = 0; i < w; i++)
				{
					for (int j = 0; j < h; j++)
					{
						c = pbTch[PointTsm((int)ceil(i + x), (int)ceil(j + y), nMapW, nMapH)] & 0x00FFFFFF;
					}
				}
			}*/
		}
		else
		{
			Px = (int)ceil(x);
			Py = (int)ceil(y);
		}
	}
	else
	{
		if (inWall)
		{
			Cspeed = 0;
			return;
		}
		if (-SpeedChange < abs(Cspeed))
		{
			SpeedChange += abs(Cspeed);
			if (Cspeed > 0)
			{
				Cspeed = SpeedChange;
			}
			else
			{
				Cspeed = -SpeedChange;
			}
		}
		else Cspeed = 0;
		if (inWall)
		{
			/*while (c == BGR(0x0000FF))
			{
				Cx += (int)round(cos(CForward));
				Cy += (int)round(sin(CForward));
				for (int i = 0; i < w; i++)
				{
					for (int j = 0; j < h; j++)
					{
						c = pbTch[PointTsm((int)ceil(i + x), (int)ceil(j + y), Racing.getwidth(), Racing.getheight())] & 0x00FFFFFF;
					}
				}
			}*/
		}
		else
		{
			Cx = (int)ceil(x);
			Cy = (int)ceil(y);
		}
	}
	if (PinLine && PHadPass)
	{
		PWaitOut = true;
	}
	if (CinLine && CHadPass)
	{
		CWaitOut = true;
	}
	if (PWaitOut && !PinLine)
	{
		Ppass++;
		PWaitOut = false;
	}
	if (CWaitOut && !CinLine)
	{
		Cpass++;
		CWaitOut = false;
	}


	// 记录上一次的坐标
	if (!player)
	{
		pPrevious[0] = { Px,Py };
	}
	else
	{
		pPrevious[1] = { Cx,Cy };
	}
}

int GetCommand()
{
	int c = 0;
	if (!Pover)
	{
		if (GetAsyncKeyState(VK_LEFT) & 0x8000)		c |= CMD_LEFT;
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000)	c |= CMD_RIGHT;
		if (GetAsyncKeyState(VK_UP) & 0x8000)
			c |= CMD_UP;
		if ((GetAsyncKeyState(VK_DOWN) & 0x8000))	c |= CMD_DOWN;
		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000))	c |= CMD_SHIFT;
	}

	if (TwoPlayer)
	{
		if (!Cover)
		{
			if (GetAsyncKeyState('A') & 0x8000)		c |= sCMD_LEFT;
			if (GetAsyncKeyState('D') & 0x8000)		c |= sCMD_RIGHT;
			if (GetAsyncKeyState('W') & 0x8000)		c |= sCMD_UP;
			if (GetAsyncKeyState('S') & 0x8000)		c |= sCMD_DOWN;
		}
	}

	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)	c |= CMD_QUIT;
	return c;
}

void DispatchCommand(int _cmd)
{
	if (_cmd & CMD_UP)			OnUp(false);
	if (_cmd & CMD_DOWN)		OnDown(false);
	if (_cmd & CMD_LEFT)		OnLeft(false);
	if (_cmd & CMD_RIGHT)		OnRight(false);
	if (_cmd & CMD_SHIFT)		OnShift(false);
	if (TwoPlayer)
	{
		if (_cmd & sCMD_UP)			OnUp(true);
		if (_cmd & sCMD_DOWN)		OnDown(true);
		if (_cmd & sCMD_LEFT)		OnLeft(true);
		if (_cmd & sCMD_RIGHT)		OnRight(true);
	}
	if (_cmd & CMD_QUIT)
	{
		MeumMod = true;
	}
}

void InitGame()
{
	Loading();

	BeginBatchDraw();
	if (chexit)return;

	setbkmode(TRANSPARENT);
	settextcolor(WHITE);

	Player1 = car1;
	Player2 = car2;

	SetBirth();
}

// 自动根据速度更新转向速度
void AutoRotaSetting()
{
	int max = 15;
	int p = Rota_base - (int)(Pspeed / 3);
	int c = Rota_base - (int)(Cspeed / 3);
	if (p >= max)	PRota = p;
	else			PRota = max;
	if (c >= max)	CRota = c;
	else			CRota = max;
}

void gaming()
{
	//游戏菜单
	settextcolor(BLACK);
	button gc(1, WIDTH / 2 - 50, HEIGHT / 2 - 100, 100, 50, L"继续游戏");
	button gh(2, WIDTH / 2 - 50, HEIGHT / 2, 100, 50, L"回到主菜单");
	button ge(3, WIDTH / 2 - 50, HEIGHT / 2 + 100, 100, 50, L"退出游戏");

	page gm;

	gm.botlist.push_back(gc);
	gm.botlist.push_back(gh);
	gm.botlist.push_back(ge);

	int gid = 0;

	mciSendString(L"play mymusic from 0", NULL, 0, NULL);	// 从头播放音乐并重复
	mciSendString(L"play mymusic repeat", NULL, 0, NULL);

	StartWord();
	Start = clock();// 开始计时
	timer tCmd;		// 获取命令计时
	timer tWinWait;	// 胜利后计时
	bool bStartWaitWin = false;	// 是否进入胜利后的短暂等待时间
	while (true)
	{
		Now = clock();
		if (tCmd.WaitFor(25))						// 如果过了0.025秒
		{
			DispatchCommand(GetCommand());			// 获取操作
			if (Pspeed != 0)
			{
				MoveCheck(false);
			}
			if (TwoPlayer && Cspeed != 0)
			{
				MoveCheck(true);
			}
		}
		if (MeumMod)								// 显示暂停界面
		{
			MeumUsed = clock();
			gid = gm.ShownPage();
			if (gid == 1)
			{
				MeumMod = false;
			}
			if (gid == 2)
			{
				End();
				isres = true;
				return;
			}
			if (gid == 3)
			{
				isres = false;
				return;
			}
			Start += clock() - MeumUsed;
		}

		Processing = clock();

		AutoRotaSetting();
		Draw();

		// 胜利判定
		if ((TwoPlayer && Cover && Pover) || (!TwoPlayer && Pover))
		{
			bStartWaitWin = true;
		}
		if (bStartWaitWin && tWinWait.WaitFor(3000))
		{
			End();
			WinScene();
			isres = true;
			break;
		}
	}
}

void End()
{
	mciSendString(L"stop mymusic", NULL, 0, NULL);	// 停止并关闭音乐
	mciSendString(L"close mymusic", NULL, 0, NULL);
	EndBatchDraw();
}

void OnLeft(bool player)
{
	if (!player)
	{
		PForward -= PI / PRota;
	}
	else
	{
		CForward -= PI / CRota;
	}
	if (!CanRota(player))
	{
		if (!player)
		{
			if (Pspeed >= 0)
			{
				Px += (int)round(cos(PForward - PI / 2));
				Py += (int)round(sin(PForward - PI / 2));
			}
			else
			{
				Px -= (int)round(cos(PForward - PI / 2));
				Py -= (int)round(sin(PForward - PI / 2));
			}
		}
		else
		{
			if (Pspeed >= 0)
			{
				Cx += (int)round(cos(CForward - PI / 2));
				Cy += (int)round(sin(CForward - PI / 2));
			}
			else
			{
				Cx -= (int)round(cos(CForward - PI / 2));
				Cy -= (int)round(sin(CForward - PI / 2));
			}
		}
	}
}

void OnRight(bool player)
{
	if (!player)
	{
		PForward += PI / PRota;
	}
	else
	{
		CForward += PI / CRota;
	}
	if (!CanRota(player))
	{
		if (!player)
		{
			if (Pspeed >= 0)
			{
				Px += (int)round(cos(PForward + PI / 2));
				Py += (int)round(sin(PForward + PI / 2));
			}
			else
			{
				Px -= (int)round(cos(PForward + PI / 2));
				Py -= (int)round(sin(PForward + PI / 2));
			}
		}
		else
		{
			if (Pspeed >= 0)
			{
				Cx += (int)round(cos(CForward + PI / 2));
				Cy += (int)round(sin(CForward + PI / 2));
			}
			else
			{
				Cx -= (int)round(cos(CForward + PI / 2));
				Cy -= (int)round(sin(CForward + PI / 2));
			}
		}
	}
}

void OnUp(bool player)
{
	if (!player)
	{
		Pspeed += SpeedAdd;
		if (Pspeed > MaxSpeed)
		{
			Pspeed = MaxSpeed;
		}
	}
	else
	{
		Cspeed += SpeedAdd;
		if (Cspeed > MaxSpeed)
		{
			Cspeed = MaxSpeed;
		}
	}
}

void OnDown(bool player)
{
	if (!player)
	{
		Pspeed -= SpeedAdd;
		if (Pspeed < -SpeedAdd)
		{
			Pspeed = -SpeedAdd;
		}
	}
	else
	{
		Cspeed -= SpeedAdd;
		if (Cspeed < -SpeedAdd)
		{
			Cspeed = -SpeedAdd;
		}
	}
}

// 新增漂移。不支持双人
void OnShift(bool player)
{
	if (!player)
	{
		// 漂移不能减速到 0
		if (Pspeed - SpeedAdd / 2 > 0)
			Pspeed -= SpeedAdd / 2;
	}
}

void SetBirth()
{
	int Ax = 0;
	int Ay = 0;
	int Bx = 0;
	int By = 0;
	bool findA = false;
	DWORD* pbTch = GetImageBuffer(&Mask);
	//通过两个终点线端点获取终点线的向量
	//通过atan2获取终点线与正方向的夹角
	for (int i1 = 0; i1 < nMapW; i1++)
	{
		for (int i2 = 0; i2 < nMapH; i2++)
		{
			DWORD c = pbTch[PointTsm(i1, i2, nMapW, nMapH)] & 0x00FFFFFF;
			if (c == 0xFF00FF)
			{
				Px = i1;
				Py = i2;
			}
			if (c == 0xFFFFFF)
			{
				Cx = i1;
				Cy = i2;
			}
			if (c == 0xAAAAAA)
			{
				if (!findA)
				{
					Ax = i1;
					Ay = i2;
					findA = true;
				}
				else
				{
					Bx = i1;
					By = i2;
				}
			}
		}
	}
	EndLineForward = atan2(Ax - Bx, Ay - By);
	PForward = EndLineForward;
	CForward = EndLineForward;

	pEndLinePoints[0] = { Ax, Ay };
	pEndLinePoints[1] = { Bx, By };

	pEndLine = GetLineOfPoints(pEndLinePoints[0], pEndLinePoints[1]);
}

// 设置低分辨率
void LowResolution()
{
	static int n = 0;

	if (n == 0)
	{
		WIDTH = 800;
		HEIGHT = 600;
	}
	else
	{
		int w = (int)(WIDTH / 1.3);
		int h = (int)(HEIGHT / 1.3);
		if (w > 300 && h > 300)
		{
			WIDTH = w;
			HEIGHT = h;
		}
	}

	dWidthZoom = WIDTH / 1280.0;
	dHeightZoom = HEIGHT / 960.0;
	closegraph();
	initgraph(WIDTH, HEIGHT);

	n++;
}

// 菜单界面
void Loading()
{
	mciSendString(_T("open res\\bk.mp3 alias bk"), NULL, 0, NULL);
	mciSendString(_T("play bk repeat"), NULL, 0, NULL);

begin:

	cleardevice();
	settextstyle(16, 0, L"system");
	settextcolor(BLACK);

	//主菜单按钮与界面
	button bs(1, 540, 580 - 400, 200, 100, L"单人模式");
	button bd(2, 540, 580 - 200, 200, 100, L"双人模式");
	button bh(3, 540, 580, 200, 100, L"游戏说明");
	button be(4, 540, 580 + 200, 200, 100, L"退出游戏");

	button btnLowResolution(5, 160, 20, 160, 40, L"低分辨率模式");
	button btnOriginal(6, 160, 70, 160, 40, L"开启原版视角（2D）");
	button btnPerspective(7, 160, 120, 160, 40, L"关闭 2.5D 透视效果");

	page sm;

	sm.botlist.push_back(bs);
	sm.botlist.push_back(bh);
	sm.botlist.push_back(be);
	sm.botlist.push_back(bd);
	sm.botlist.push_back(btnLowResolution);
	sm.botlist.push_back(btnOriginal);
	sm.botlist.push_back(btnPerspective);

#define MULTIPLY(x,y) x = (int)(x * y)

	// 适配低分辨率
	for (int i = 0; i < 7; i++)
	{
		MULTIPLY(sm.botlist[i].x, dWidthZoom);
		MULTIPLY(sm.botlist[i].y, dHeightZoom);
		MULTIPLY(sm.botlist[i].width, dWidthZoom);
		MULTIPLY(sm.botlist[i].height, dHeightZoom);
	}

	settextcolor(RGB(255, 130, 40));
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 86;
	_tcscpy_s(f.lfFaceName, _T("黑体"));
	f.lfItalic = true;

	settextstyle(&f);
	RECT r = { 0, 0, (int)(1280 * dWidthZoom), (int)(200 * dHeightZoom) };
	drawtext(L"2.5D 双人赛车", &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	settextstyle(16, 0, L"system", 0, 0, 0, false, false, false);
	settextcolor(LIGHTBLUE);
	outtextxy(50, 200, L"双人赛车   huidong 修改版 Ver 2.0");
	outtextxy(50, 240, L"原作者：极品史莱姆");
	outtextxy(50, 300, L"玩家一：方向键操作，并且可以使用 shift 键短刹车");
	outtextxy(50, 340, L"玩家二：WSAD 操作");

	int gid = 0;
	while (true)
	{
		if (isUseOriginal)	sm.botlist[btnOriginal.ID - 1].str = L"关闭原版视角（2D）";
		else				sm.botlist[btnOriginal.ID - 1].str = L"开启原版视角（2D）";

		if (isUseOriginal)				sm.botlist[btnPerspective.ID - 1].str = L"--原版视角下无透视--";
		else if (isPerspectiveEffect)	sm.botlist[btnPerspective.ID - 1].str = L"关闭 2.5D 透视效果";
		else							sm.botlist[btnPerspective.ID - 1].str = L"开启 2.5D 透视效果";

		gid = sm.ShownPage();

		switch (gid)
		{
		case 1:	TwoPlayer = false;	goto btn_end;
		case 2:	TwoPlayer = true;	goto btn_end;
		case 3:	showhelp();			goto begin;
		case 4:	chexit = true;		return;
		case 5: LowResolution();	goto begin;
		case 6: isUseOriginal = !isUseOriginal;	break;
		case 7:	if (!isUseOriginal) isPerspectiveEffect = !isPerspectiveEffect;	break;
		}
	}

btn_end:

	int num = ChooseMap();
	if (num == -1)
	{
		goto begin;
	}

	wstring rootpath = L"map\\" + IntroList[num].filename + L"\\";
	wchar_t tmp[30];
	IMAGE imgTmp[4];

	GetPrivateProfileString(L"File", L"Car1", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&imgTmp[0], (rootpath + tmp).c_str());
	car1 = imgTmp[0];
	GetPrivateProfileString(L"File", L"Car2", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&imgTmp[1], (rootpath + tmp).c_str());
	car2 = imgTmp[1];
	GetPrivateProfileString(L"File", L"Racing", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&imgTmp[2], (rootpath + tmp).c_str());
	Racing = imgTmp[2];
	GetPrivateProfileString(L"File", L"Toucher", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&imgTmp[3], (rootpath + tmp).c_str());
	Mask = imgTmp[3];
	GetPrivateProfileString(L"File", L"Music", NULL, tmp, 30, IntroList[num].inipath.c_str());
	mciSendString((L"open " + rootpath + tmp + L" alias mymusic").c_str(), NULL, 0, NULL);	// 打开MP3文件
	//通过文件获取值
	MaxSpeed = (double)GetPrivateProfileInt(L"Set", L"MaxSpeed", (int)MaxSpeed, IntroList[num].inipath.c_str());
	FinSand = GetPrivateProfileInt(L"Set", L"FinSand", FinSand, IntroList[num].inipath.c_str());
	FinRoad = GetPrivateProfileInt(L"Set", L"FinRoad", FinRoad, IntroList[num].inipath.c_str());
	FinIce = GetPrivateProfileInt(L"Set", L"FinIce", FinIce, IntroList[num].inipath.c_str());
	Rota_base = GetPrivateProfileInt(L"Set", L"Rota", Rota_base, IntroList[num].inipath.c_str());
	NeedR = GetPrivateProfileInt(L"Set", L"NeedR", NeedR, IntroList[num].inipath.c_str());

	for (int i = 0; i < 3; i++)
	{
		wstring wch;
		wch += L'C' - i;
		nLevelTime[i] = GetPrivateProfileInt(L"Level", wch.c_str(), nLevelTime[i], IntroList[num].inipath.c_str());
	}

	nMapW = Racing.getwidth();
	nMapH = Racing.getheight();

	mciSendString(_T("stop bk"), NULL, 0, NULL);
	mciSendString(_T("close bk"), NULL, 0, NULL);
}

void showhelp()
{
	cleardevice();
	settextcolor(WHITE);
	settextstyle(24, 0, L"宋体");
	outtextxy(10, 10, L"游戏说明");

	wstring word;
	int Record = 3;

	wifstream HelpFile(L"Help.txt");

	HelpFile.imbue(std::locale(""));

	settextstyle(16, 0, L"宋体");
	if (HelpFile.is_open())
	{
		while (!HelpFile.eof())
		{
			getline(HelpFile, word, L'\n');//读入整行
			outtextxy(10, 5 + (int)(1.2 * Record * 16), word.c_str());
			Record++;
		}
	}
	else
	{
		MessageBox(NULL, L"帮助文件丢失", L"错误", MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
		closegraph();
		exit(1);
	}

	Record += 2;
	settextstyle(24, 0, L"宋体");
	outtextxy(10, (int)(1.2 * Record * 16), L"按 ESC 键返回主菜单");

	settextstyle(16, 0, L"宋体");

	clean();
	_getch();

	cleardevice();
	HelpFile.close();

	settextcolor(BLACK);
	return;
}

// 清空按键缓冲区
void clean()
{
	int k;
	while (1) {
		if (_kbhit()) {
			k = _getch();
			if (0 == k || 0xE0 == k) k = _getch();
		}
		else break;
	}
}

void restart()
{
	IntroList.clear();
	Px = 150;
	Py = 150;
	PForward = 0;		// 方向
	Pspeed = 0;				// 速度
	Ppass = 0;				// 通过几次终点
	Pwrong = false;		// 是否逆行
	PHadPass = false;		// 是否通过终点
	PWaitOut = false;		// 是否等待通过终点
	Pover = false;			// 是否结束
	Ptime = 0;
	Ptime2 = 0;

	Cx = 170;
	Cy = 170;
	CForward = 0;
	Cspeed = 0;
	Cpass = 0;
	Cwrong = false;
	CHadPass = false;
	CWaitOut = false;
	Cover = false;
	Ctime = 0;
	Ctime2 = 0;

	MeumMod = false;

	Start = 0;
	Now = 0;
	MeumUsed = 0;
	Processing = 0;
}

bool CanRota(bool player)
{
	DWORD* pbTch = GetImageBuffer(&Mask);
	int x, y, w, h;
	int SpeedChange = 0;
	IMAGE tmpRotatedCar;	// 存储旋转后的车子
	if (!player)
	{
		x = Px;
		y = Py;
		rotateimage(&tmpRotatedCar, &car1, -PForward, WHITE, true, false);
	}
	else
	{
		x = Cx;
		y = Cy;
		rotateimage(&tmpRotatedCar, &car2, -CForward, WHITE, true, false);
	}
	DWORD* pCar = GetImageBuffer(&tmpRotatedCar);
	w = tmpRotatedCar.getwidth();
	h = tmpRotatedCar.getheight();

	// 获取所在的地面类型
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++)
		{
			if ((pCar[PointTsm(i, j, w, h)] & 0x00FFFFFF) == WHITE)
			{
				continue;
			}

			if ((pbTch[PointTsm(i + x, j + y, nMapW, nMapH)] & 0x00FFFFFF) == BGR(0x0000FF))
			{
				return false;
			}
		}
	}
	return true;
}

// 载入只需要加载一次的资源
void InitRes()
{
	loadimage(&imgWinFlag, L"res/flag.jpg");
	loadimage(&imgStar[0], L"res/star1.bmp");
	loadimage(&imgStar[1], L"res/star2.bmp");
}

int main()
{
	initgraph(WIDTH, HEIGHT);

	InitRes();

	while (isres)
	{
		InitGame();
		if (chexit)
		{
			break;
		}
		gaming();
		restart();
	}

	closegraph();
}