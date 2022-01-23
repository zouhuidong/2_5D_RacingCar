/////////////////////////////////////////
//
//	NewDrawer.h
//	
//	游戏中绘制的相关函数
//


#pragma once

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
	if (newWidth < 0 || newHeight < 0) {
		newWidth = pImg->getwidth();
		newHeight = pImg->getheight();
	}

	// 当参数只有一个时按比例自动缩放
	if (newHeight == 0) {
		// 此处需要注意先*再/。不然当目标图片小于原图时会出错
		newHeight = newWidth * pImg->getheight() / pImg->getwidth();
	}

	// 获取需要进行缩放的图片
	IMAGE newImg(newWidth, newHeight);

	// 分别对原图像和目标图像获取指针
	DWORD* oldDr = GetImageBuffer(pImg);
	DWORD* newDr = GetImageBuffer(&newImg);

	// 赋值 使用双线性插值算法
	for (int i = 0; i < newHeight - 1; i++) {
		for (int j = 0; j < newWidth - 1; j++) {
			int t = i * newWidth + j;
			int xt = j * pImg->getwidth() / newWidth;
			int yt = i * pImg->getheight() / newHeight;
			newDr[i * newWidth + j] = oldDr[xt + yt * pImg->getwidth()];
			// 实现逐行加载图片
			byte r = (GetRValue(oldDr[xt + yt * pImg->getwidth()]) +
				GetRValue(oldDr[xt + yt * pImg->getwidth() + 1]) +
				GetRValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) +
				GetRValue(oldDr[xt + (yt + 1) * pImg->getwidth() + 1])) / 4;
			byte g = (GetGValue(oldDr[xt + yt * pImg->getwidth()]) +
				GetGValue(oldDr[xt + yt * pImg->getwidth()] + 1) +
				GetGValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) +
				GetGValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) + 1) / 4;
			byte b = (GetBValue(oldDr[xt + yt * pImg->getwidth()]) +
				GetBValue(oldDr[xt + yt * pImg->getwidth()] + 1) +
				GetBValue(oldDr[xt + (yt + 1) * pImg->getwidth()]) +
				GetBValue(oldDr[xt + (yt + 1) * pImg->getwidth() + 1])) / 4;
			newDr[i * newWidth + j] = RGB(r, g, b);
		}
	}

	return newImg;
}

void PutImgWithout(
	IMAGE& obj,
	int px,
	int py,
	COLORREF withouter = WHITE,
	DWORD* pbWnd = GetImageBuffer(GetWorkingImage()),
	int wX = getwidth(),
	int wY = getheight(),
	DWORD bitsub = 0x00FFFFFF
)
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
	// 画面太小，不予绘制
	if (WIDTH < 500 || HEIGHT < 500)
	{
		return;
	}

	IMAGE img = Racing;
	SetWorkingImage(&img);

	// 此时 Player1、2 已经旋转过，无需再旋转
	PutImgWithout(Player1, Px, Py);
	if (TwoPlayer)
		PutImgWithout(Player2, Cx, Cy);

	SetWorkingImage();

	int w = 200, h = 200;
	int x = WIDTH - w;
	//ImageToSize(w, h, &img);
	img = zoomImage(&img, w, h);
	putimage(x, 0, &img);
	rectangle(x, 0, x + w, h);
}

// 2D 坐标旋转
POINT Rotate2D(int x, int y, double radian)
{
	if (radian == 0)	return { x,y };
	return { (int)(x * cos(radian) - y * sin(radian)),
		(int)(x * sin(radian) + y * cos(radian)) };
}

// 获取小车在旋转后的地图中的位置
// NOTE: 旧方法，耗时 60ms 左右，不推荐使用。
POINT GetRotatedCarPosition(int width, int height, int x, int y, double radian)
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
	for (int i = 0; i < nw * nh; i++)
	{
		if (buf[i] == WHITE)
		{
			return { i - (i / nw) * nw,i / nw };
		}
	}

	// 寻找失败
	return { -1,-1 };
}

// 获取小车在旋转后的地图中的位置
// NOTE: 新方法，耗时 <= 1ms，极荐。
POINT GetRotatedCarPosition2(int width, int height, int x, int y, double radian)
{
	int left = 0, button = 0;
	POINT points[3] = { {width,0},{0,height},{width,height} };

	// 由于 IMAGE 对象的坐标系 y 轴向下，通用坐标系 y 轴向上，
	// 故旋转时需要以相反角度旋转才等效于在 IMAGE 对象的坐标系中正常旋转
	for (int j = 0; j < 3; j++)
	{
		points[j] = Rotate2D(points[j].x, points[j].y, -radian);
	}
	POINT p = Rotate2D(x, y, -radian);

	// 记录最左和最下的端点
	for (int j = 0; j < 3; j++)
	{
		if (points[j].x < points[left].x)
		{
			left = j;
		}
		if (points[j].y < points[button].y)
		{
			button = j;
		}
	}

	// 若图像旋转后端点坐标越界，将其补回正常位置
	if (points[left].x < 0)		p.x += -points[left].x;
	if (points[button].y < 0)	p.y += -points[button].y;

	return p;
}

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
	//POINT pCar = GetRotatedCarPosition(nMapWdith, nMapHeight, pOldCar.x, pOldCar.y, radian);
	POINT pCar = GetRotatedCarPosition2(nMapWdith, nMapHeight, pOldCar.x, pOldCar.y, radian);

	// 玩家视野
	IMAGE imgView(pFarViewWidth, pViewLength);

	// 从车子位置向前方获取图像
	// 启用透视效果
	if (isPerspectiveEffect)
	{
		DWORD* bufImgRotatedMap = GetImageBuffer(&imgMapRotated);
		for (int i = 0; i < pViewLength; i++)
		{
			// 当前这一条视野线的宽度
			int nThisWidth = (int)(pNearViewWidth + dViewZoomRatio * i);

			// 获取当前视野线图像【方法 1】
			//IMAGE imgThisLine;
			//SetWorkingImage(&imgMapRotated);
			//getimage(&imgThisLine, pCar.x - nThisWidth / 2, pCar.y - (i - pRearViewLength), nThisWidth, 1);

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
	}

	// 关闭透视（流畅但效果差）
	else
	{
		SetWorkingImage(&imgMapRotated);
		getimage(&imgView, pCar.x - pFarViewWidth / 2, pCar.y - pViewLength, pFarViewWidth, pViewLength + pRearViewLength);
	}

	SetWorkingImage();

	return imgView;
}


// 伪 3D 绘制玩家视角
void Draw3D_PlayerView()
{
	int nViewLength = 400;		// 视野长度
	int nNearViewWidth = 100;	// 近视野宽度
	int nFarViewWidth = 250;	// 远视野宽度

	// 后视野长度（用于看见车身）将根据车子长度动态调整

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
	PutImgWithout(Player1, Px - car1.getwidth() / 2, Py - car1.getheight() / 2);
	if (TwoPlayer)
	{
		rotateimage(&Player2, &car2, -CForward, WHITE, true, false);
		PutImgWithout(Player2, Cx - car2.getwidth() / 2, Cy - car2.getheight() / 2);
	}

	// 获取玩家视野
	IMAGE imgRealView[2];
	imgRealView[0] = GetPlayerView(nViewLength, nNearViewWidth, nFarViewWidth, car1.getheight() * 2, imgMap, PForward + PI / 2, { Px,Py });
	if (TwoPlayer)
		imgRealView[1] = GetPlayerView(nViewLength, nNearViewWidth, nFarViewWidth, car2.getheight() * 2, imgMap, CForward + PI / 2, { Cx,Cy });

	// 实际在屏幕上显示的图像需要拉伸
	if (!TwoPlayer)
	{
		ImageToSize(WIDTH, HEIGHT, &imgRealView[0]);

		// 此方案太慢
		//imgRealView[0] = zoomImage(&imgRealView[0],WIDTH, HEIGHT);

		putimage(0, 0, &imgRealView[0]);
	}
	else
	{
		ImageToSize(WIDTH / 2, HEIGHT, &imgRealView[0]);
		ImageToSize(WIDTH / 2, HEIGHT, &imgRealView[1]);
		putimage(WIDTH / 2, 0, &imgRealView[0]);	// 玩家 1 操作方向键，所以放右边
		putimage(0, 0, &imgRealView[1]);		// 玩家 2 操作 WSAD，放左边

		setlinestyle(PS_SOLID, 2);
		line(WIDTH / 2, 0, WIDTH / 2, HEIGHT);
		setlinestyle(PS_SOLID, 1);
	}
}

// 获取游戏中的速度对应的 km/h 单位制速度
double GetKMSpeed(double speed)
{
	return speed *= 6;
}

wstring GetKMSpeedText(double speed)
{
	wstring str = to_wstring(GetKMSpeed(speed));
	str[str.length() - 5] = L'\0';
	return str.c_str() + (wstring)L" km/h";
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
	settextstyle(50, 0, L"system");

	// 右：玩家一
	settextcolor(GetSpeedColor(Pspeed, MaxSpeed));
	wstring str = GetKMSpeedText(Pspeed);
	int w = textwidth(str.c_str());
	int h = textheight(str.c_str());
	outtextxy(WIDTH - w, HEIGHT - h, str.c_str());

	// 左：玩家二
	if (TwoPlayer)
	{
		settextcolor(GetSpeedColor(Cspeed, MaxSpeed));
		wstring str2 = GetKMSpeedText(Cspeed);
		int h2 = textheight(str2.c_str());
		outtextxy(0, HEIGHT - h2, str2.c_str());
	}

	settextstyle(16, 0, L"宋体");
	settextcolor(WHITE);
}

// 绘制普通的 2D 赛道视角
void Draw2DView()
{
	IMAGE img(Racing.getwidth(), Racing.getheight());
	SetWorkingImage(&img);

	putimage(0, 0, &Racing);
	rotateimage(&Player1, &car1, -PForward, WHITE, true, false);
	PutImgWithout(Player1, Px, Py);

	if (TwoPlayer)
	{
		rotateimage(&Player2, &car2, -CForward, WHITE, true, false);
		PutImgWithout(Player2, Cx, Cy);
	}

	SetWorkingImage();

	if (WIDTH != Racing.getwidth() || HEIGHT != Racing.getheight())
	{
		img = zoomImage(&img, WIDTH, HEIGHT);
		//ImageToSize(WIDTH, HEIGHT, &img);
	}

	putimage(0, 0, &img);
}

// 获胜界面
void WinScene()
{
	IMAGE imgEmpty(nMaxW, 100);
	IMAGE imgFlag = imgWinFlag;
	IMAGE imgScores[2] = { imgEmpty ,imgEmpty };	// 评级
	IMAGE imgPlayers[2] = { imgEmpty ,imgEmpty };	// 玩家信息
	bool isSwap = false;	// 是否交换了玩家 1、2 的顺序

	if (dWidthZoom != 1)
	{
		ImageToSize(WIDTH, WIDTH * imgFlag.getheight() / imgFlag.getwidth(), &imgFlag);
	}

	// 评级
	SetWorkingImage(&imgScores[0]);
	settextstyle(42, 0, L"宋体");
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	outtextxy(20, 20, L"评级");
	int nStarY = 10;	// 输出星星的相对 Y 坐标
	for (int i = 3; i > 0; i--)
	{
		putimage(nMaxW - i * imgStar[0].getwidth(), nStarY, &imgStar[0]);
	}
	if (TwoPlayer)
	{
		imgScores[1] = imgScores[0];
	}

	// 玩家信息
	for (int i = 0; i < TwoPlayer + 1; i++)
	{
		SetWorkingImage(&imgPlayers[i]);
		settextstyle(42, 0, L"宋体");
		settextcolor(WHITE);
		setbkmode(TRANSPARENT);

		IMAGE img;
		int time;	// 耗时，单位毫秒
		LPCTSTR strPlayer;

		if (i == 0)
		{
			img = Player1;
			time = Ptime;
			strPlayer = L"玩家 1";
		}
		else
		{
			img = Player2;
			time = Ctime;
			strPlayer = L"玩家 2";
		}

		int dstH = 50;
		int dstW = dstH * img.getwidth() / img.getheight();
		//img = zoomImage(&img, dstW, dstH);
		ImageToSize(dstW, dstH, &img);
		PutImgWithout(img, 50, 30);

		// 用时
		int m = time / 1000 / 60;
		int s = (time - m * 1000 * 60) / 1000;
		wstring strM = to_wstring(m);
		wstring strS = to_wstring(s);
		if (strM.size() == 1)	strM = L"0" + strM;
		if (strS.size() == 1)	strS = L"0" + strS;
		wstring str = strM + L"′";
		str += strS + L"″";
		LPCTSTR text = str.c_str();

		outtextxy(200, 20, strPlayer);
		outtextxy(nMaxW - 350, 20, text);
	}

	// 缩放
	for (int i = 0; i < 2 && dWidthZoom != 1; i++)
	{
		imgScores[i] = zoomImage(&imgScores[i], WIDTH);
		imgPlayers[i] = zoomImage(&imgPlayers[i], WIDTH);

		//ImageToSize(WIDTH, WIDTH * imgScores[i].getheight() / imgScores[i].getwidth(), &imgScores[i]);
		//ImageToSize(WIDTH, WIDTH * imgPlayers[i].getheight() / imgPlayers[i].getwidth(), &imgPlayers[i]);
	}
	imgEmpty = zoomImage(&imgEmpty, WIDTH);


	//////// 逐级展示出的图像

	// 胜利背景乐
	mciSendString(_T("open res\\win.mp3 alias win"), NULL, 0, NULL);
	mciSendString(_T("play win repeat"), NULL, 0, NULL);

	SetWorkingImage();

	const int nStageNum = 6;

	// 图像阶梯
	IMAGE* pStages[nStageNum] = { &imgFlag,&imgScores[0],&imgPlayers[0],&imgScores[1],&imgPlayers[1],&imgEmpty };
	int nOutY[nStageNum] = { (int)(100 * dHeightZoom) };	// 每张图像的输出 Y 坐标
	if (TwoPlayer && Ctime < Ptime)
	{
		isSwap = true;
		IMAGE* ps[nStageNum] = { &imgFlag,&imgScores[1],&imgPlayers[1],&imgScores[0],&imgPlayers[0],&imgEmpty };
		for (int i = 0; i < nStageNum; i++)	pStages[i] = ps[i];
	}
	for (int i = 1; i < nStageNum; i++)
	{
		nOutY[i] = nOutY[i - 1] + pStages[i - 1]->getheight();
	}

	clock_t tStart = 0;
	int nX[nStageNum] = { 0 };	// 每张图像的位置
	for (int i = 0; i < nStageNum; i++)	nX[i] = -WIDTH;
	int nNum = 0;	// 现在开始运动的图像数量
	while (true)
	{
		// 每隔一段时间开始运动一张图像
		if ((clock() - tStart) / (double)CLOCKS_PER_SEC >= 0.06)
		{
			if (nNum < nStageNum)
			{
				nNum++;
			}

			// 若没有玩家二，那么剩下的图片直接跟着玩家一的最后一张图片动起来
			/*if (nNum == 3 && !TwoPlayer)
			{
				nNum = nStageNum;
			}*/

			tStart = clock();
		}

		// 移动当前可以运动的图像
		for (int i = 0; i < nNum; i++)
		{
			// 线性加速运动
			nX[i] += (int)(100 * dWidthZoom - nX[i] * 0.3);
			putimage((int)((nX[i] > 0 ? 0 : nX[i]) * dWidthZoom), nOutY[i], pStages[i]);
			FlushBatchDraw();
			if (nX[i] > 0 && i == nStageNum - 1)	// 全部图像运动完成
			{
				goto move_end;
			}
		}

		Sleep(50);
	}

move_end:

	Sleep(200);

	//////// 点亮星级

	mciSendString(_T("open res\\prize.mp3 alias prize"), NULL, 0, NULL);

	// 缩放后的星星
	IMAGE imgStarZoom = dWidthZoom == 1 ? imgStar[1] : zoomImage(&imgStar[1], (int)(imgStar[1].getwidth() * dWidthZoom));
	for (int i = 0; i < TwoPlayer + 1; i++)
	{
		int p = isSwap ? !i : i;	// 当前玩家编号

		int layer;	// 记录评分图层是第几层
		if (p == 0) if (!isSwap) layer = 1; else layer = 3;
		else		if (!isSwap) layer = 3; else layer = 1;

		int time;	// 玩家耗时（秒）
		if (p == 0)	time = Ptime;
		else		time = Ctime;
		time /= CLOCKS_PER_SEC;

		// 颁奖
		for (int j = 0; j < 3; j++)
		{
			if (time <= nLevelTime[j])
			{
				putimage(WIDTH - imgStarZoom.getwidth() * (j + 1), nOutY[layer] + (int)(nStarY * dHeightZoom), &imgStarZoom);
				FlushBatchDraw();
				Sleep(50);
				mciSendString(_T("play prize from 0"), NULL, 0, NULL);
				Sleep(300);
			}
		}

		mciSendString(_T("stop prize"), NULL, 0, NULL);
	}

	mciSendString(_T("close prize"), NULL, 0, NULL);

	settextstyle(30, 0, L"宋体");
	settextcolor(BLACK);
	button btnOK(0, (int)(WIDTH - 250 * dWidthZoom), nOutY[nStageNum - 1] + 10, (int)(200 * dWidthZoom), (int)(80 * dHeightZoom), L"Back");
	page mypage;
	mypage.botlist.push_back(btnOK);
	mypage.ShownPage();

	mciSendString(_T("stop win"), NULL, 0, NULL);
	mciSendString(_T("close win"), NULL, 0, NULL);
}

// 绘制游戏中场景
void Draw()
{
	cleardevice();
	settextcolor(WHITE);

	// 绘制赛道
	if (isUseOriginal)
	{
		Draw2DView();
		Sleep(10);
	}
	else
	{
		Draw3D_PlayerView();
		DrawSmallMap();
		DrawSpeed();
	}

	if (!isUseOriginal)
	{
		settextstyle(72, 0, L"宋体");
	}

	setbkmode(OPAQUE);

	if (Pwrong)
	{
		if (isUseOriginal)
		{
			outtextxy((int)((Px - 20) * dWidthZoom), (int)((Py - 20) * dHeightZoom), _T("请勿逆行"));
		}
		else
		{
			if (TwoPlayer)
			{
				outtextxy((int)(700 * dWidthZoom), 300, _T("请勿逆行"));
			}
			else
			{
				outtextxy(300, 300, _T("请勿逆行"));
			}
		}

		if ((Now - Ptime2) > 100)
		{
			Pwrong = false;
		}
	}
	if (TwoPlayer && Cwrong)
	{
		if (isUseOriginal)
		{
			outtextxy((int)((Cx - 20) * dWidthZoom), (int)((Cy - 20) * dHeightZoom), _T("请勿逆行"));
		}
		else
		{
			outtextxy((int)(300 * dWidthZoom), 300, _T("请勿逆行"));
		}

		if ((Now - Ctime2) > 100)
		{
			Cwrong = false;
		}
	}

	settextstyle(20, 0, L"system");
	setbkmode(TRANSPARENT);

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
	if (TwoPlayer && Cover && Pover)
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

// 即将开始比赛的场景
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

			if (isUseOriginal)
			{
				Draw2DView();
			}
			else
			{
				Draw3D_PlayerView();
				DrawSmallMap();
			}

			RECT r = { 0, 0, WIDTH, HEIGHT };
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

		if (isUseOriginal)
		{
			Draw2DView();
		}
		else
		{
			Draw3D_PlayerView();
			DrawSmallMap();
		}

		Now = clock();
		RECT r = { 0, 0, WIDTH, HEIGHT };
		drawtext(_T("START！"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		FlushBatchDraw();
	}
	settextstyle(&font);
	settextcolor(WHITE);
}
