//////////////////////////////////////////////
//
//	2.5D 赛车
//
//	改编自 极品史莱姆 (799052200@qq.com)
//	详见 define.h
//
//	修改内容：
//	1. 视角改为 2.5D（伪 3D）
//	2. 新增 SHIFT 键操作
//	3. 修改了速度等部分参数
//	4. 显示小地图
//	5. 显示速度
//



#include "define.h"

/*
 *	参考自http://tieba.baidu.com/p/5218523817?pid=109267542552&cid=0#109267542552
 *	函数名:zoomImage(IMAGE* pImg,int width，int height)
 *	参数说明:pImg是原图指针，width1和height1是目标图片的尺寸。
 *	函数功能:将图片进行缩放，返回目标图片 可以自定义长与宽，也可以只给长自动计算宽
 *	返回目标图片
*/
IMAGE zoomImage(IMAGE* pImg, int newWidth, int newHeight = 0)
{
	// 防止越界
	if (newWidth<0 || newHeight<0){
		newWidth = pImg->getwidth();
		newHeight = pImg->getheight();
	}

	// 当参数只有一个时按比例自动缩放
	if (newHeight == 0){
		// 此处需要注意先*再/。不然当目标图片小于原图时会出错
		newHeight = newWidth * pImg->getheight() / pImg->getwidth();
	}

	// 获取需要进行缩放的图片
	IMAGE newImg(newWidth, newHeight);

	// 分别对原图像和目标图像获取指针
	DWORD* oldDr = GetImageBuffer(pImg);
	DWORD* newDr = GetImageBuffer(&newImg);

	// 赋值 使用双线性插值算法
	for (int i = 0; i<newHeight - 1; i++){
		for (int j = 0; j<newWidth - 1; j++){
			int t = i*newWidth + j;
			int xt = j*pImg->getwidth() / newWidth;
			int yt = i*pImg->getheight() / newHeight;
			newDr[i*newWidth + j] = oldDr[xt + yt*pImg->getwidth()];
			// 实现逐行加载图片
			byte r = (GetRValue(oldDr[xt + yt*pImg->getwidth()]) +
				GetRValue(oldDr[xt + yt*pImg->getwidth() + 1]) +
				GetRValue(oldDr[xt + (yt + 1)*pImg->getwidth()]) +
				GetRValue(oldDr[xt + (yt + 1)*pImg->getwidth() + 1])) / 4;
			byte g = (GetGValue(oldDr[xt + yt*pImg->getwidth()]) +
				GetGValue(oldDr[xt + yt*pImg->getwidth()] + 1) +
				GetGValue(oldDr[xt + (yt + 1)*pImg->getwidth()]) +
				GetGValue(oldDr[xt + (yt + 1)*pImg->getwidth()]) + 1) / 4;
			byte b = (GetBValue(oldDr[xt + yt*pImg->getwidth()]) +
				GetBValue(oldDr[xt + yt*pImg->getwidth()] + 1) +
				GetBValue(oldDr[xt + (yt + 1)*pImg->getwidth()]) +
				GetBValue(oldDr[xt + (yt + 1)*pImg->getwidth() + 1])) / 4;
			newDr[i*newWidth + j] = RGB(r, g, b);
		}
	}

	return newImg;
}

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
bool MouseTouch(int left, int top, int right, int bottom)
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

int ChooseMap()
{
	RECT r;
	cleardevice();
	wchar_t tmp[30];				//缓冲区
	wstring tw;						//对标题的处理

	if (SearchFilesByWildcard(L"map/*"))
	{

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
			m = GetMouseMsg();
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

int PointTsm(int x, int y, int wide, int high)
{
	if (x < 0)return x = 0;
	if (x >= wide)return x = wide;
	if (y < 0)return y = 0;
	if (y >= high)return y = high;

	return wide*y + x;
}

void MoveCheck(bool player)
{
	bool PinLine = false;
	bool CinLine = false;

	int rtmp = 1;

	inIce = false;
	inRoad = false;
	inSand = false;
	inWall = false;
	inEndline = false;
	DWORD* pbTch = GetImageBuffer(&Toucher);
	double Mx;
	double My;
	int iX;
	int iY;
	double SpeedChange = 0;
	DWORD c;
	if (!player)
	{
		Mx = Pspeed*cos(PForward) + Px;
		My = Pspeed*sin(PForward) + Py;
		DWORD* pbImg = GetImageBuffer(&Player1);
		iX = Player1.getwidth();
		iY = Player1.getheight();
	}
	else
	{
		Mx = Cspeed*cos(CForward) + Cx;
		My = Cspeed*sin(CForward) + Cy;
		DWORD* pbImg = GetImageBuffer(&Player2);
		iX = Player2.getwidth();
		iY = Player2.getheight();
	}
	//获取所在的地面类型
	for (int i1 = 0; i1 < iX; i1++)
	{
		for (int i2 = 0; i2 < iY; i2++)
		{
			c = pbTch[PointTsm((int)ceil(i1 + Mx), (int)ceil(i2 + My), WIDE, HEIGHT)] & 0x00FFFFFF;
			if (c == BGR(BLACK))
			{
				inRoad = true;
			}
			else if (c == BGR(0x00FFFF))
			{
				inSand = true;
			}
			else if (c == BGR(0xFF0000))
			{
				inIce = true;
			}
			else if (c == BGR(0x0000FF))
			{
				inWall = true;
			}
			else if (c == BGR(0x00FF00) || c == BGR(0xAAAAAA))
			{
				inEndline = true;
				inRoad = true;
			}
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
			if (cos(PForward)*Pspeed<0)
			{
				inWall = true;
				Pwrong = true;
				Ptime2 = Now;
			}
			else
			{
				if (!PHadPass)PHadPass = true;
			}
		}
		else
		{
			CinLine = true;
			if (cos(CForward)*Cspeed<0)
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
		if (-SpeedChange<abs(Pspeed))
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
			while (c == BGR(0x0000FF))
			{
				Px += (int)round(cos(PForward));
				Py += (int)round(sin(PForward));
				for (int i1 = 0; i1 < iX; i1++)
				{
					for (int i2 = 0; i2 < iY; i2++)
					{
						c = pbTch[PointTsm((int)ceil(i1 + Mx), (int)ceil(i2 + My), WIDE, HEIGHT)] & 0x00FFFFFF;
					}
				}
			}
		}
		else
		{
			Px = (int)ceil(Mx);
			Py = (int)ceil(My);
		}
	}
	else
	{
		if (inWall)
		{
			Cspeed = 0;
			return;
		}
		if (-SpeedChange<abs(Cspeed))
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
			while (c == BGR(0x0000FF))
			{
				Cx += (int)round(cos(CForward));
				Cy += (int)round(sin(CForward));
				for (int i1 = 0; i1 < iX; i1++)
				{
					for (int i2 = 0; i2 < iY; i2++)
					{
						c = pbTch[PointTsm((int)ceil(i1 + Mx), (int)ceil(i2 + My), WIDE, HEIGHT)] & 0x00FFFFFF;
					}
				}
			}
		}
		else
		{
			Cx = (int)ceil(Mx);
			Cy = (int)ceil(My);
		}
	}
	if (PinLine&&PHadPass)
	{
		PWaitOut = true;
	}
	if (CinLine&&CHadPass)
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

}

void PutImgWithout(IMAGE &obj, int px, int py, COLORREF withouter = WHITE, DWORD* pbWnd = GetImageBuffer(GetWorkingImage()), int wX = getwidth(), int wY = getheight(), DWORD bitsub = 0x00FFFFFF)
{
	DWORD* pbImg = GetImageBuffer(&obj);
	int iX = obj.getwidth();
	int iY = obj.getheight();
	for (int i1 = 0; i1 < iX; i1++)
	{
		for (int i2 = 0; i2 < iY; i2++)
		{
			if (PointTsm(i1 + px, i2 + py, wX, wY) == -1)continue;						// 检测是否越界
			if ((pbImg[PointTsm(i1, i2, iX, iY)] & bitsub) == BGR(withouter))continue;	// 检测是否要排除该颜色


			pbWnd[PointTsm(i1 + px, i2 + py, wX, wY)] = pbImg[PointTsm(i1, i2, iX, iY)]; // 操作显存
		}
	}
}

int GetCommand()
{
	int c = 0;
	if (!Pover)
	{
		if (GetAsyncKeyState(VK_LEFT) & 0x8000)		c |= CMD_LEFT;
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000)	c |= CMD_RIGHT;
		if (GetAsyncKeyState(VK_UP) & 0x8000)		c |= CMD_UP;
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
	if (_cmd&CMD_QUIT)
	{
		MeumMod = true;
	}
}

void init()
{
	Loading();

	BeginBatchDraw();
	if(chexit)return;

	setbkmode(TRANSPARENT);
	settextcolor(WHITE);

	Player1 = car1;
	Player2 = car2;

	SetBirth();
}

void gaming()
{
	//游戏菜单
	settextcolor(BLACK);
	bottom gc(1, WIDE / 2 - 50, HEIGHT / 2 - 100, 100, 50, L"继续游戏");
	bottom gh(2, WIDE / 2 - 50, HEIGHT / 2, 100, 50, L"回到主菜单");
	bottom ge(3, WIDE / 2 - 50, HEIGHT / 2 + 100, 100, 50, L"退出游戏");

	page gm;

	gm.botlist.push_back(gc);
	gm.botlist.push_back(gh);
	gm.botlist.push_back(ge);

	int gid = 0;

	mciSendString(L"play mymusic from 0", NULL, 0, NULL);	// 从头播放音乐并重复
	mciSendString(L"play mymusic repeat", NULL, 0, NULL);

	StartWord();
	Start = clock();//开始计时
	timer a;
	while (true)
	{
		Now = clock();
		if (a.WaitFor(25))							// 如果过了0.025秒
		{
			DispatchCommand(GetCommand());			// 获取操作
			if (Pspeed != 0)
			{
				MoveCheck(false);
			}
			if(TwoPlayer) if (Cspeed != 0)
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
		Draw();
	}
}

void End()
{
	mciSendString(L"stop mymusic", NULL, 0, NULL);	// 停止并关闭音乐
	mciSendString(L"close mymusic", NULL, 0, NULL);
	EndBatchDraw();
}


//
// NOTE: 由于 zoomImage 函数拉伸高度为 1px 的图像时出现问题，
// 所以加入了这个新的图像拉伸函数
//
// 图片拉伸
// width, height 拉伸后的图片大小
// img 原图像
void ImageToSize(int width, int height, IMAGE* img)
{
	IMAGE* pOldImage = GetWorkingImage();
	SetWorkingImage(img);

	IMAGE temp_image(width, height);

	StretchBlt(
		GetImageHDC(&temp_image), 0, 0, width, height,
		GetImageHDC(img), 0, 0,
		getwidth(), getheight(),
		SRCCOPY
	);

	Resize(img, width, height);
	putimage(0, 0, &temp_image);

	SetWorkingImage(pOldImage);
}

// 小地图（右上角）
void DrawSmallMap()
{
	IMAGE img = Racing;
	SetWorkingImage(&img);

	// 此时 Player1、2 已经旋转过，无需再旋转
	PutImgWithout(Player1, Px, Py);
	if (TwoPlayer)
		PutImgWithout(Player2, Cx, Cy);
	
	SetWorkingImage();

	int w = 200, h = 200;
	int x = WIDE - w;
	//ImageToSize(w, h, &img);
	img = zoomImage(&img, w, h);
	putimage(x, 0, &img);
	rectangle(x, 0, x+w, h);
}

//POINT Rotate2D(int x, int y, double radian)
//{
//	if (radian == 0)	return { x,y };
//	return { (int)(x * cos(radian) - y * sin(radian)),
//		(int)(x * sin(radian) + y * cos(radian)) };
//}

// 获取小车在旋转后的地图中的位置
POINT GetRotatedCarPosition(int width, int height, int x, int y,double radian)
{
	IMAGE p(width, height), p2;
	int nw, nh;
	SetWorkingImage(&p);
	fillrectangle(x, y, x + 2, y + 2);
	rotateimage(&p2, &p, radian, BLACK, true, false);
	nw = p2.getwidth();
	nh = p2.getheight();
	SetWorkingImage();
	DWORD* buf = GetImageBuffer(&p2);
	/*POINT pr;*/
	for (int i = 0; i < nw * nh; i++)
	{
		if (buf[i] == WHITE)
		{
			return { i - (i / nw) * nw,i / nw };
		}
	}

	/*SetWorkingImage(&p2);
	for (int j = 0; j < nh; j++)
	{
		for (int j = 0; j < nw; j++)
		{
			if (getpixel(j, j) == WHITE)
			{
				return { j,j };
			}
		}
	}
	SetWorkingImage();*/

	// 寻找失败
	return { -1,-1 };
}

//POINT GetRotatedCarPosition2(int width, int height, int x, int y, double radian)
//{
//	int half_w = width / 2, half_h = height / 2;
//	int left = 0, top = 0;
//	POINT points[4] = { {-half_w,half_h},{half_w,half_h},{-half_w,-half_h},{half_w,-half_h} };
//	for (int j = 0; j < 4; j++)
//	{
//		// 绘图窗口坐标系的 y 坐标需要反转
//		points[j] = Rotate2D(points[j].x, /*-*/points[j].y, radian);
//		//points[j].y = -points[j].y;
//	}
//	POINT p = Rotate2D(x, y, radian);
//
//	for (int j = 0; j < 4; j++)
//	{
//		if (points[j].x < points[left].x)
//		{
//			left = j;
//		}
//		else if (points[j].y < points[top].y)
//		{
//			top = j;
//		}
//	}
//
//	// 移动
//	p.x += -half_w - points[left].x;
//	p.y += points[top].y - half_h;
//	
//	//p.y = -p.y;
//
//	return p;
//}

// 获取玩家视野
IMAGE GetPlayerView(
	int pViewLength,
	int pNearViewWidth,
	int pFarViewWidth,
	int pRearViewLength,	// 后视野长度
	IMAGE imgMap,			// 含有车子的地图
	double radian,			// 车子角度
	POINT pOldCar			// 车子坐标
)
{
	// 视野缩放比
	double dViewZoomRatio = (pFarViewWidth - pNearViewWidth) / (double)pViewLength;
	IMAGE imgMapRotated;	// 以车为正方向的旋转后的地图
	
	int nMapWdith = imgMap.getwidth();
	int	nMapHeight = imgMap.getheight();

	// 将地图旋转为以车方向为正方向
	rotateimage(&imgMapRotated, &imgMap, radian, BLACK, true, false);

	int nRotatedMapWidth = imgMapRotated.getwidth();
	int nRotatedMapHeight = imgMapRotated.getheight();

	// 地图旋转后，车坐标也要旋转
	POINT pCar = GetRotatedCarPosition(nMapWdith, nMapHeight, pOldCar.x, pOldCar.y, radian);

	// 玩家视野
	IMAGE imgView(pFarViewWidth, pViewLength);

	// 从车子位置向前方获取图像
	DWORD* bufImgRotatedMap = GetImageBuffer(&imgMapRotated);
	for (int i = 0; i < pViewLength; i++)
	{
		// 当前这一条视野线的宽度
		int nThisWidth = (int)(pNearViewWidth + dViewZoomRatio * i);

		// 获取当前视野线图像【方法 1】
		/*IMAGE imgThisLine;
		SetWorkingImage(&imgMapRotated);
		getimage(&imgThisLine, pCar.x - nThisWidth / 2, pCar.y - (i - pRearViewLength), nThisWidth, 1);*/

		// 获取当前视野线图像【方法 2】（稍微快一点）
		IMAGE imgThisLine(nThisWidth, 1);
		DWORD* bufThisLine = GetImageBuffer(&imgThisLine);
		for (int j = 0; j < nThisWidth; j++)
		{
			int lines_x = (pCar.y - (i - pRearViewLength)) * nRotatedMapWidth;
			int x = (pCar.x - nThisWidth / 2) + j;

			// 越界，填黑
			if (x < 0 || x > nRotatedMapWidth || lines_x + x > nRotatedMapWidth * nRotatedMapHeight || lines_x + x < 0)
			{
				bufThisLine[j] = BLACK;
			}
			else
			{
				bufThisLine[j] = bufImgRotatedMap[lines_x + x];
			}
		}

		// 将当前视野线拉升至最长视野宽度
		IMAGE imgThisZoomLine = imgThisLine;
		ImageToSize(pFarViewWidth, 1, &imgThisZoomLine);

		// 将当前线画入视野
		SetWorkingImage(&imgView);
		putimage(0, pViewLength - i, &imgThisZoomLine);
	}

	SetWorkingImage();

	return imgView;
}

// 【新增函数 by huidong】
// 伪 3D 绘制玩家视角
void Draw3D_PlayerView()
{
	int nViewLength = 300;		// 视野长度
	int nNearViewWidth = 100;	// 近视野宽度
	int nFarViewWidth = 200;	// 远视野宽度
	int nRearViewLength = 30;	// 后视野长度（用于看见车身）

	// 双人模式下为减小消耗，缩窄视野
	if (TwoPlayer)
	{
		nViewLength /= 2;
		nNearViewWidth /= 2;
		nFarViewWidth /= 2;
	}

	IMAGE imgMap = Racing;	// 地图（有车）

	// 将车放入地图
	SetWorkingImage(&imgMap);
	rotateimage(&Player1, &car1, -PForward, WHITE, true, false);
	PutImgWithout(Player1, Px, Py);
	if (TwoPlayer)
	{
		rotateimage(&Player2, &car2, -CForward, WHITE, true, false);
		PutImgWithout(Player2, Cx, Cy);
	}

	// 获取玩家视野
	IMAGE imgRealView[2];
	imgRealView[0] = GetPlayerView(nViewLength, nNearViewWidth, nFarViewWidth, nRearViewLength, imgMap, PForward + PI / 2, { Px,Py });
	if(TwoPlayer)
		imgRealView[1] = GetPlayerView(nViewLength, nNearViewWidth, nFarViewWidth, nRearViewLength, imgMap, CForward + PI / 2, { Cx,Cy });

	// 实际在屏幕上显示的图像需要拉伸
	if (!TwoPlayer)
	{
		ImageToSize(WIDE, HEIGHT, &imgRealView[0]);
		
		// 此方案太慢
		//imgRealView[0] = zoomImage(&imgRealView[0],WIDE, HEIGHT);

		putimage(0, 0, &imgRealView[0]);
	}
	else
	{
		ImageToSize(WIDE / 2, HEIGHT, &imgRealView[0]);
		ImageToSize(WIDE / 2, HEIGHT, &imgRealView[1]);
		putimage(WIDE / 2, 0, &imgRealView[0]);	// 玩家 1 操作方向键，所以放右边
		putimage(0, 0, &imgRealView[1]);		// 玩家 2 操作 WSAD，放左边
		
		setlinestyle(PS_SOLID, 2);
		line(WIDE / 2, 0, WIDE / 2, HEIGHT);
		setlinestyle(PS_SOLID, 1);
	}
}

// 获取游戏中的速度对应的 km/h 单位制速度
double GetKMSpeed(double speed)
{
	return speed *= 10;
}

// 获取速度对应的颜色
COLORREF GetSpeedColor(double speed, double max)
{
	double stage = max / 3;

	if (speed >= stage * 2)
		return RED;
	else if (speed >= stage * 1)
		return RGB(250, 110, 0);	// 橙色
	else
		return WHITE;
}

// 绘制速度
void DrawSpeed()
{
	settextstyle(30, 0, L"system");

	if (!TwoPlayer)
	{
		settextcolor(GetSpeedColor(Pspeed, MaxSpeed));
		wstring str = to_wstring(GetKMSpeed(Pspeed)) + L" km/h";
		outtextxy(10, 90, str.c_str());
	}
	else
	{
		// 左：玩家二
		wstring strPlayer2 = to_wstring(GetKMSpeed(Cspeed)) + L" km/h";
		settextcolor(GetSpeedColor(Cspeed, MaxSpeed));
		outtextxy(10, 90, strPlayer2.c_str());

		// 右：玩家一
		wstring strPlayer1 = to_wstring(GetKMSpeed(Pspeed)) + L" km/h";
		settextcolor(GetSpeedColor(Pspeed, MaxSpeed));
		outtextxy(WIDE - textwidth(strPlayer1.c_str()), 200, strPlayer1.c_str());
	}

	settextstyle(16, 0, L"宋体");
	settextcolor(WHITE);
}

// 绘制普通的 2D 赛道
void Draw2DView()
{
	putimage(0, 0, &Racing);
	rotateimage(&Player1, &car1, -PForward, WHITE, true, false);
	PutImgWithout(Player1, Px, Py);
	
	if (TwoPlayer)
	{
		rotateimage(&Player2, &car2, -CForward, WHITE, true, false);
		PutImgWithout(Player2, Cx, Cy);
	}
}

void Draw()
{
	cleardevice();
	settextcolor(WHITE);

	// 绘制赛道

	//Draw2DView();

	Draw3D_PlayerView();
	DrawSmallMap();
	DrawSpeed();

	if (Pwrong)
	{
		outtextxy(Px - 20, Py - 20, _T("请勿逆行"));
		if ((Now - Ptime2) > 100)
		{
			Pwrong = false;
		}
	}
	if (TwoPlayer) if (Cwrong)
	{
		outtextxy(Cx - 20, Cy - 20, _T("请勿逆行"));
		if ((Now - Ctime2) > 100)
		{
			Cwrong = false;
		}
	}
	//绘制分数
	if (!Pover)
	{
		outtextxy(10, 10, (L"玩家1  " + to_wstring(Ppass) + L" / " + to_wstring(NeedR)).c_str());
		outtextxy(10, 30, (L"玩家1用时 " + to_wstring((Now - Start) / 1000) + L"." + to_wstring((Now - Start) % 1000) + L"s").c_str());
		if (Ppass == NeedR)
		{
			Pover = true;
			Ptime = Now - Start;
		}
	}
	else
	{
		outtextxy(10, 10, L"玩家1已完成!");
		outtextxy(10, 30, (L"玩家1用时 " + to_wstring(Ptime / 1000) + L"." + to_wstring(Ptime % 1000) + L"s").c_str());
	}
	if (TwoPlayer) if (!Cover)
	{
		outtextxy(10, 50, (L"玩家2  " + to_wstring(Cpass) + L" / " + to_wstring(NeedR)).c_str());
		outtextxy(10, 70, (L"玩家2用时 " + to_wstring((Now - Start) / 1000) + L"." + to_wstring((Now - Start) % 1000) + L"s").c_str());
		if (Cpass == NeedR)
		{
			Cover = true;
			Ctime = Now - Start;
		}
	}
	else
	{
		outtextxy(10, 50, L"玩家2已完成!");
		outtextxy(10, 70, (L"玩家2用时 " + to_wstring(Ctime / 1000) + L"." + to_wstring(Ctime % 1000) + L"s").c_str());
	}
	if ((TwoPlayer) && (Cover&&Pover))
	{
		if (Ctime > Ptime)outtextxy(10, 90, _T("玩家1获胜!"));
		else outtextxy(10, 90, _T("玩家2获胜!"));
		outtextxy(10, 110, _T("游戏结束，按ESC回到主菜单"));
	}
	else if (Pover)
	{
		outtextxy(10, 50, _T("游戏结束，按ESC回到主菜单"));
	}
	FlushBatchDraw();
}

void OnLeft(bool player)
{
	if (!player)
	{
		PForward -= PI / Rota;
	}
	else
	{
		CForward -= PI / Rota;
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
		PForward += PI / Rota;
	}
	else
	{
		CForward += PI / Rota;
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
	DWORD* pbTch = GetImageBuffer(&Toucher);
	//通过两个终点线端点获取终点线的向量
	//通过atan2获取终点线与正方向的夹角
	for (int i1 = 0; i1 < WIDE; i1++)
	{
		for (int i2 = 0; i2 < HEIGHT; i2++)
		{
			DWORD c = pbTch[PointTsm(i1, i2, WIDE, HEIGHT)] & 0x00FFFFFF;
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
}

void StartWord()
{
	int tmp;
	COLORREF fontcol = 0x0;
	LOGFONT font;
	LOGFONT cha;
	gettextstyle(&font);
	cha = font;
	cha.lfHeight = 500;
	cha.lfWeight = FW_BOLD;
	settextstyle(&cha);
	settextcolor(fontcol);
	//实现字体逐渐变白
	for (int i = 3; i > 0; i--)
	{
		Start = Now = clock();
		while ((Now - Start) < 1000)
		{
			cleardevice();
			
			/*putimage(0, 0, &Racing);
			PutImgWithout(car1, Px, Py);
			if (TwoPlayer) PutImgWithout(car2, Cx, Cy);*/

			Draw3D_PlayerView();
			DrawSmallMap();

			RECT r = { 0, 0, WIDE, HEIGHT };
			drawtext(to_wstring(i).c_str(), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			Now = clock();
			tmp = 255 * (Now - Start) / 1000;
			fontcol = RGB(tmp, tmp, tmp);
			settextcolor(fontcol);
			FlushBatchDraw();
		}
	}
	Start = Now = clock();
	while ((Now - Start) < 500)
	{
		cleardevice();

		/*putimage(0, 0, &Racing);
		PutImgWithout(car1, Px, Py);
		if (TwoPlayer) PutImgWithout(car2, Cx, Cy);*/

		Draw3D_PlayerView();
		DrawSmallMap();

		Now = clock();
		RECT r = { 0, 0, WIDE, HEIGHT };
		drawtext(_T("START！"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		FlushBatchDraw();
	}
	settextstyle(&font);
	settextcolor(WHITE);
}

void Loading()
{
	cleardevice();

	settextcolor(BLACK);

	//主菜单按钮与界面
	bottom bs(1, 540, 580 - 400, 200, 100, L"单人模式");
	bottom bd(4, 540, 580 - 200, 200, 100, L"双人模式");
	bottom bh(2, 540, 580, 200, 100, L"游戏说明");
	bottom be(3, 540, 580 + 200, 200, 100, L"退出游戏");

	page sm;

	sm.botlist.push_back(bs);
	sm.botlist.push_back(bh);
	sm.botlist.push_back(be);
	sm.botlist.push_back(bd);

	settextcolor(WHITE);
	RECT r = { 0, 0, 1280, 200 };
	drawtext(L"双人赛车", &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	outtextxy(50, 200, L"2.5D 赛车~  huidong 修改版");
	outtextxy(50, 240, L"原作者：极品史莱姆");
	outtextxy(50, 300, L"玩家一：方向键操作，并且可以使用 shift 键短刹车");
	outtextxy(50, 340, L"玩家二：WSAD 操作");

	int gid = 0;
	while (true)
	{
		gid = sm.ShownPage();

		if (gid == 2)
		{
			showhelp();
		}
		if (gid == 3)
		{
			chexit = true;
			return;
		}
		if (gid == 1)
		{
			TwoPlayer = false;
			break;
		}
		if (gid == 4)
		{
			TwoPlayer = true;
			break;
		}
	}

	int num = ChooseMap();
	wstring rootpath = L"map\\" + IntroList[num].filename + L"\\";

	wchar_t tmp[30];

	GetPrivateProfileString(L"File", L"Toucher", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&Toucher, (rootpath + tmp).c_str());
	HEIGHT = Toucher.getheight();
	WIDE = Toucher.getwidth();

	GetPrivateProfileString(L"File", L"Car1", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&car1, (rootpath + tmp).c_str());
	GetPrivateProfileString(L"File", L"Car2", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&car2, (rootpath + tmp).c_str());
	GetPrivateProfileString(L"File", L"Racing", NULL, tmp, 30, IntroList[num].inipath.c_str());
	loadimage(&Racing, (rootpath + tmp).c_str());
	GetPrivateProfileString(L"File", L"Music", NULL, tmp, 30, IntroList[num].inipath.c_str());
	mciSendString((L"open " + rootpath + tmp + L" alias mymusic").c_str(), NULL, 0, NULL);	// 打开MP3文件
	//通过文件获取值
	MaxSpeed = (double)GetPrivateProfileInt(L"Set", L"MaxSpeed", (int)MaxSpeed, IntroList[num].inipath.c_str());
	FinSand = GetPrivateProfileInt(L"Set", L"FinSand", FinSand, IntroList[num].inipath.c_str());
	FinRoad = GetPrivateProfileInt(L"Set", L"FinRoad", FinRoad, IntroList[num].inipath.c_str());
	FinIce = GetPrivateProfileInt(L"Set", L"FinIce", FinIce, IntroList[num].inipath.c_str());
	Rota = GetPrivateProfileInt(L"Set", L"Rota", Rota, IntroList[num].inipath.c_str());
	NeedR = GetPrivateProfileInt(L"Set", L"NeedR", NeedR, IntroList[num].inipath.c_str());
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
		MessageBox(NULL, L"帮助文件丢失", L"错误", MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
		closegraph();
		exit(1);
	}

	Record += 2;
	settextstyle(24, 0, L"宋体");
	outtextxy(10, (int)(1.2 * Record*16), L"按 ESC 键返回主菜单");

	clean();
	_getch();

	cleardevice();
	HelpFile.close();

	settextcolor(BLACK);
	return;
}

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
}

bool CanRota(bool player)
{
	DWORD* pbTch = GetImageBuffer(&Toucher);
	double Mx;
	double My;
	int iX;
	int iY;
	int SpeedChange = 0;
	DWORD c;
	IMAGE tmp;
	if (!player)
	{
		Mx = Px;
		My = Py;
		rotateimage(&tmp, &car1, -PForward, WHITE, true, false);
	}
	else
	{
		Mx = Cx;
		My = Cy;
		rotateimage(&tmp, &car2, -CForward, WHITE, true, false);
	}
	DWORD* pbImg = GetImageBuffer(&tmp);
	iX = tmp.getwidth();
	iY = tmp.getheight();
	//获取所在的地面类型
	for (int i1 = 0; i1 < iX; i1++)
	{
		for (int i2 = 0; i2 < iY; i2++)
		{
			c = pbTch[PointTsm((int)ceil(i1 + Mx), (int)ceil(i2 + My), WIDE, HEIGHT)] & 0x00FFFFFF;
			if (c == BGR(0x0000FF))
			{
				return false;
			}
		}
	}
	return true;
}

int main()
{
	initgraph(WIDE, HEIGHT);							// 创建绘图窗口

	while (isres)
	{
		init();
		if (chexit)
		{
			break;
		}
		gaming();
		restart();
	}
	
	closegraph();
}