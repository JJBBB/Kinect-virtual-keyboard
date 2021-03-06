// Keyboard.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Kinect.h> 
#include <iostream>
#include <opencv2/opencv.hpp>
//#include "LineFinder.h"
#include <WinUser.h>
#include <vector>
#include <memory>
#include <math.h>
#include <algorithm>
#include "Keyboard.h"

using namespace std;
using namespace cv;

//释放接口需要自己定义  
template<class Interface>

inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL){
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

Mat BGRA2BGR(Mat &bufferMat);
Mat YCrCb(const Mat &bufferMat);
Mat BGRWhite(const Mat &bufferMat);
void BabblSort(vector<Key>&Line1Keys);
bool FindKey(const Point2d &point1, const Point2d &point2, double xielv);
Mat YCrCb1(const Mat &bufferMat);
Mat YCrCbHand(const Mat &bufferMat);

//int width = 1920;
//int height = 1080;

unsigned int bufferSize = 1920 * 1080 * 4 * sizeof(unsigned char);
int width = 960;
int height = 540;
Mat bufferMat(1080, 1920, CV_8UC4);
//Mat dst(height, width, CV_8UC4);
Mat dst1(height, width, CV_8UC1);
Mat dst2(height, width, CV_8UC1);
Mat dst3(height, width, CV_8UC4);
Mat dst5(height / 2, width / 2, CV_8UC1);

//const char *pstrWindowsToolBar = "Threshold";
//static int nThresholdEdge = 50;
//
//void on_trackbar(int nThresholdEdge, void *)
//{
//	//canny边缘检测
//	Canny(dst2, dst2, nThresholdEdge, nThresholdEdge * 3, 3);
//	//dilate(dst, dst1, Mat(1, 1, CV_8U), Point(-1, -1), 2);//进行膨胀操作
//	//resize(dst1, dst1, Size(), 0.5, 0.5);
//	
//}


//向量的叉乘a×b=（x1y2-x2y1）
inline float GetCross(CvPoint& p1, CvPoint& p2, CvPoint& p)
{
	return (p2.x - p1.x) * (p.y - p1.y) - (p.x - p1.x) * (p2.y - p1.y);
}

inline float GetCross1(Point& p1, Point& p2, Point& p)
{
	return (p2.x - p1.x) * (p1.y - p.y) - (p1.x - p.x) * (p2.y - p1.y);
}

//判断点是否在四边形框内
inline bool InTheRectangle(CvPoint &P1, CvPoint &P2, CvPoint &P3, CvPoint &P4, int &x,int &y)
{
	CvPoint p = cvPoint(x, y);
	if (GetCross(P1, P3, p)*GetCross(P4, P2, p) >= 0 && GetCross(P2, P1, p)*GetCross(P3, P4, p) >= 0)
		return true;
	return false;
}
inline bool InTheRectangle1(Point &P1, Point &P2, Point &P3, Point &P4, int &x, int &y)
{
	Point p(x, y);
	if (GetCross1(P1, P3, p)*GetCross1(P4, P2, p) >= 0 && GetCross1(P2, P1, p)*GetCross1(P3, P4, p) >= 0)
		return true;
	return false;
}

//计算两向量夹角的余弦
inline double ArcCos(Point P, Point P1, Point P2)
{
	return acosf(((P1.x - P.x)*(P2.x - P.x) + (P1.y - P.y)*(P2.y - P.y)) / (sqrtf((P1.x - P.x)*(P1.x - P.x) + (P1.y - P.y)*(P1.y - P.y))*sqrtf((P2.x - P.x)*(P2.x - P.x) + (P2.y - P.y)*(P2.y - P.y))));
}

bool SortGetTwo(const Point &v1, const Point &v2);

unsigned int Times = 0;//用于键位正确计数
vector<Key>KeyboardAll;//存储最终正确的键位
vector<vector<Key>>KeyboardCorrect(6);//存储最终正确的键位
vector<CvPoint>Vertex(4);//确定下来的最终4个顶点
bool KeyDetecationOK = false;
vector<vector<bool>>FingerTipISInTheRegion;//判断键位内是否有指尖点
vector<vector<Point>>FingerTipsInTheKey;//如果键位内有指尖点，记录下该指尖点的位置
vector<vector<bool>>KeyIsPushed;//每次键被按下就做标记，同一按键前后两帧都被按下，需要被排除
vector<vector<bool>>KeyIsStatic;
static int Mark = 0;//标记帧数

//简单的判断是否在框内
inline bool InTheRect(Point P)
{
	return(P.x > Vertex[0].x || P.x > Vertex[2].x) && (P.x<Vertex[1].x || P.x<Vertex[3].x) && (P.y>Vertex[0].y || P.y>Vertex[1].y) && (P.y < Vertex[2].y || P.y < Vertex[3].y);
}



//vector<Point>FingerTips;//存储检测到的指尖点
//依据到指尖距离对键位进行排序
//inline bool  cmpMy(const Key &KFirst, const Key &KSecond)
//{
//	int mX1 = FingerTips[FingerTips.size() - 1].x - KFirst.Kernel.x;
//	int mY1 = FingerTips[FingerTips.size() - 1].y - KFirst.Kernel.y;
//	int mX2 = FingerTips[FingerTips.size() - 1].x - KSecond.Kernel.x;
//	int mY2 = FingerTips[FingerTips.size() - 1].y - KSecond.Kernel.y;
//	if (mX1*mX1 + mY1*mY1 < mX2*mX2 + mY2*mY2)
//	{
//		return true;
//	}
//	return false;
//}

//对键位轮廓进行排序获取

int _tmain(int argc, _TCHAR* argv[])
{
	setUseOptimized(true);
	
	//Sensor
	IKinectSensor *pSensor;
	HRESULT hResult = S_OK;
	hResult = GetDefaultKinectSensor(&pSensor);
	if (FAILED(hResult))
	{
		cerr<<"Error : GetDefaultKinectSensor" << endl;
		return -1;
	}
	hResult = pSensor->Open();
	if (FAILED(hResult))
	{
		cerr << "Error ： IKinectSensor::Open()" << endl;
		return -1;
	}

	//Source
	IColorFrameSource *pColorSource;
	hResult = pSensor->get_ColorFrameSource(&pColorSource);
	if (FAILED(hResult))
	{
		cerr << "Error : IKInectSensor::get_ColorFrameSource()" << endl;
		return -1;
	} 

	//Reader
	IColorFrameReader *pColorReader;
	hResult = pColorSource->OpenReader(&pColorReader);
	if (FAILED(hResult))
	{
		cerr << "Error : IColorFrameSource::OpenReader()" << endl;
		return -1;
	}

	/*int width = 1920;
	int height = 1080;
	unsigned int bufferSize = width * height * 4 * sizeof(unsigned char);
	Mat bufferMat(height, width, CV_8UC4);
	Mat colorMat(height / 2, width / 2, CV_8UC4);
	Mat dst(height / 2, width / 2, CV_8UC4);
	Mat dst1(height / 2, width / 2, CV_8UC4);
	const char *pstrWindowsToolBar = "滑动条";*/

	//创建窗口
	//namedWindow("Keyboard", CV_WINDOW_NORMAL);
	//namedWindow("Binary", CV_WINDOW_NORMAL);
	////创建滑动条
	//int nThresholdEdge = 1;
	//cvCreateTrackbar(pstrWindowsToolBar, "Keyboard", &nThresholdEdge, 200, on_trackbar);

	//LineFinder finder;
	//finder.setLineLengthAndGap(300,20);//设置检测线段的最小长度和线段之间的最大间距（参数暂定）
	//finder.setMinVote(100);//设置检测线段的阈值


	/*int Line = 0;
	int clo = 0;
	
	cout << "请输入键方位：" << endl;
	cin >> Line >> clo;*/
	
	
	

	while (1)
	{
		
			//Frame
			IColorFrame *pColorFrame = nullptr;
			hResult = pColorReader->AcquireLatestFrame(&pColorFrame);
			if (SUCCEEDED(hResult))
			{
				hResult = pColorFrame->CopyConvertedFrameDataToArray(bufferSize, reinterpret_cast<BYTE*>(bufferMat.data), ColorImageFormat_Bgra);
				if (SUCCEEDED(hResult))
				{
					//dst = BGRA2BGR(bufferMat);
					//cvtColor(dst, dst, CV_BGR2YCrCb);//将彩图转化为二值图
					//dst4 = YCrCb(dst);

					/*resize(dst4, dst5, Size(), 0.5, 0.5);
					resize(bufferMat, dst1, Size(), 0.5, 0.5);*/


					//////////////////////////////////////////////////////
					Mat dst;
					Mat origin;
					//bufferMat.copyTo(origin);
					resize(bufferMat, origin, Size(), 0.5, 0.5,CV_INTER_AREA);//先缩小图像，降低运算时延
					dst = BGRA2BGR(origin);
					
					
					
					if (!KeyDetecationOK)
					{
						/*Mat Skin = BGRWhite(dst);
						namedWindow("Skin", CV_WINDOW_NORMAL);
						imshow("Skin", Skin);*/
						GaussianBlur(dst, dst, Size(5, 5), 0);//高斯滤波，去除噪声
						cvtColor(dst, dst, CV_BGR2YCrCb);
						Mat dst1 = YCrCb(dst);//蓝色分割，单通道

						//大津法自适应阈值提取，有利有弊，会把手也提取到
						/*Mat Y, Cr, Cb;
						vector<Mat> channels;
						split(dst, channels);
						Y = channels.at(0);
						Cr = channels.at(1);
						Cb = channels.at(2);
						threshold(Cr, dst1, 0, 255, CV_THRESH_OTSU);*/
						///////////////////////////////////////////////////////////////////////////////////
						erode(dst1, dst2, Mat(2, 2, CV_8U), Point(-1, -1), 1);//腐蚀

						namedWindow("BLUE_ONLY", CV_WINDOW_NORMAL);
						imshow("BLUE_ONLY", dst2);
						//namedWindow("CANNY", CV_WINDOW_NORMAL);
						//resize(dst1, dst1, Size(), 0.5, 0.5);

						//resize(dst2, dst2, Size(), 0.5, 0.5);

						//////////////////////CANNY边缘检测部分////////////////////////////////////
						//createTrackbar(pstrWindowsToolBar, "Keyboard", &nThresholdEdge, 200, on_trackbar);//创建滑动条控制Canny阈值
						//on_trackbar(nThresholdEdge, 0);//回调函数用来进行Canny边缘检测
						//dilate(dst2, dst3, Mat(2, 2, CV_8U), Point(-1, -1), 1);//进行膨胀操作
						//imshow("Keyboard", dst3);
						//std::vector<cv::Vec4i> lines = finder.findLines(dst3);//数据量太大做不了直线检测
						//Mat dst4(height, width, CV_8UC3);
						//finder.drawDetectedLines(dst4, cv::Scalar(0, 0, 255));
						//imshow("origin", dst4);
						/////////////////////////////////////////////////////////////////////////////

						///////////////////////先对红色部分去噪，删除小面积的红色噪声部分，并且提取空格键，最后提取所有键的质心////////////////////////////
						Mat resultImage;
						dst2.copyTo(resultImage);
						vector< vector< Point> > contours;//轮廓数组
						findContours(resultImage, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
						for (int i = 0; i < contours.size(); i++)
						{
							if (arcLength(contours[i], true) <= 1000)//只取最大的键盘平面
							{
								contours[i].clear();
							}
						}

						//画出空格键的质心
						//Moments muMax = moments(maxContour, false);
						//Point2d mcMax = Point2d(muMax.m10 / muMax.m00, muMax.m01 / muMax.m00);
						//Key MaxK;
						//MaxK.Kernel = mcMax;
						//MaxK.Area = maxArea;
						//MaxK.Contour = maxContour;
						//AllKey.push_back(MaxK);
						//circle(origin, mcMax, 5, Scalar(0, 255, 0), -1, 8, 0);
						drawContours(resultImage, contours, -1, Scalar(255), CV_FILLED, CV_AA);//复原轮廓内部
						//画出最大轮廓（空格键）
						//maxContours.push_back(maxContour);
						//drawContours(origin, maxContours, -1, Scalar(0, 0, 0), CV_FILLED, CV_AA);//复原轮廓内部

						namedWindow("去噪图", CV_WINDOW_NORMAL);
						imshow("去噪图", resultImage);


						////////////////////////////////////////////////////////////////////////////

						/////////////////////直线逼近得顶点///////////////////////////////////

						uchar* data;//指向src_open像素值的指针
						data = resultImage.data;

						int y1 = 0, x1 = 0;
						int y2 = 0, x2 = 0;
						int y3 = 0, x3 = 0;
						int y4 = 0, x4 = 0;

						CvScalar color;//B G R
						color.val[0] = 0;
						color.val[1] = 0;
						color.val[2] = 0;
						//int record;

						/*for (int i = 1; i < height-1; i++)
						{
						for (int j = 1; j < width-1; j++)
						{
						if ((data[i*width + j] == 255) && (data[(i + 1)*width + (j)] == 0) || (data[(i - 1)*width + (j)] == 0) || (data[(i)*width + (j + 1)] == 0) || (data[(i)*width + (j - 1)] == 0))
						{
						data[i*width + j] = 255;
						}
						else
						{
						data[i*width + j] = 0;
						}
						}
						}*/

						//左上顶点获取
						for (int b = 0; b < width && 255 != data[y1*width + x1]; b++)
						{
							for (x1 = 0; x1 <= b && 255 != data[y1*width + x1]; x1++)
							{
								if (b - x1 >= height)//用与图像对角线相平行的线来进行逼近
									continue;
								else
									y1 = b - x1;
							}
						}

						/*for (int b = 0; b < height; b++)
						{
						for (xx = 0; xx <width; xx++)
						{
						yy = b - xx;
						if (data[yy*width + xx] == 255)
						{
						break;
						}
						}
						}*/

						cout << "左上顶点： " << x1 << "," << y1 << endl;
						//cout << "左下顶点： " << (int)data[yy*width + xx] << endl;
						CvPoint pointD = cvPoint(x1, y1);
						color.val[0] = 0;//B G R
						color.val[1] = 128;
						color.val[2] = 255;
						circle(origin, pointD, 10, color, 5, 8, 0); //yellow

						//右上顶点
						for (int b = width - 1; b >= 0 && 255 != data[y2*width + x2]; b--)
						{
							for (x2 = width - 1; x2 >= b && 255 != data[y2*width + x2]; x2--){

								if (x2 - b >= height)
								{
									continue;
								}
								y2 = x2 - b;
							}

						}
						cout << "右上顶点： " << x2 << "," << y2 << endl;
						CvPoint pointC = cvPoint(x2, y2);
						color.val[0] = 0;//B G R
						color.val[1] = 0;
						color.val[2] = 255;
						circle(origin, pointC, 10, color, 5, 8, 0); //red

						//左下顶点
						for (int b = height - 1; b >= -width + 1 && 255 != data[y3*width + x3]; b--)
						{
							for (x3 = 0; x3 <= height - b && 255 != data[y3*width + x3]; x3++)
							{
								if (x3 + b <= 0 || x3 >= width)
								{
									continue;
								}
								y3 = x3 + b;
							}

						}
						cout << "左下顶点： " << x3 << "," << y3 << endl;
						CvPoint pointA = cvPoint(x3, y3);
						color.val[0] = 255;//B G R
						color.val[1] = 0;
						color.val[2] = 0;
						circle(origin, pointA, 10, color, 5, 8, 0); //blue

						//右下顶点
						for (int b = height + width - 2; b >= 0 && 255 != data[y4*width + x4]; b--)
						{
							for (x4 = width - 1; x4 >= b - height && 255 != data[y4*width + x4]; x4--)
							{
								if (b - x4 < 0 || b - x4 >= height)
								{
									continue;
								}
								y4 = b - x4;
							}

						}
						cout << "右下顶点： " << x4 << "," << y4 << endl;
						CvPoint pointB = cvPoint(x4, y4);
						color.val[0] = 0;//B G R
						color.val[1] = 255;
						color.val[2] = 0;
						circle(origin, pointB, 10, color, 5, 8, 0); //green

						////////////////////////////////////////////////////////////
						/*CvPoint pointDD = cvPoint(x1-20, y1-20);
						CvPoint pointCC = cvPoint(x2+20, y2-20);
						CvPoint pointAA = cvPoint(x3-20, y3+20);
						CvPoint pointBB = cvPoint(x4+20, y4+20);*/
						CvPoint pointDD = cvPoint(x1 + 5, y1 + 5);
						CvPoint pointCC = cvPoint(x2 - 5, y2 + 5);
						CvPoint pointAA = cvPoint(x3 + 10, y3 - 10);
						CvPoint pointBB = cvPoint(x4 - 10, y4 - 10);
						Mat Binary(height, width, CV_8UC1);
						dst2.copyTo(Binary);
						for (int i = 0; i < origin.rows; i++)
						{
							uchar *ptr = origin.ptr<uchar>(i);  //第i行的指针                    
							uchar *ptrB = Binary.ptr<uchar>(i);
							for (int j = 0; j < origin.cols; j++)
							{
								if (!InTheRectangle(pointDD, pointCC, pointAA, pointBB, j, i))
								{
									ptr[4 * j] = 0;  //内部数据是4个字节，0-1-2是BGR，第4个现在未使用   
									ptr[4 * j + 1] = 0;
									ptr[4 * j + 2] = 0;
									ptrB[j] = 0;
								}
							}
						}
						Mat element = getStructuringElement(MORPH_RECT, Size(9, 9));
						erode(Binary, Binary, element);//腐蚀
						element = getStructuringElement(MORPH_RECT, Size(9, 9));
						dilate(Binary, Binary, element);//膨胀要的尺寸小一些
						/*element = getStructuringElement(MORPH_RECT, Size(15, 15));
						erode(Binary, Binary, element);
						dilate(Binary, Binary, element); */
						/*erode(Binary, Binary, element);
						element = getStructuringElement(MORPH_ELLIPSE, Size(17, 17));
						dilate(Binary, Binary, element);*/
						//morphologyEx(Binary, Binary, MORPH_OPEN, element);//形态学开运算，相当于先腐蚀后膨胀，能够平滑
						namedWindow("Binary", CV_WINDOW_NORMAL);
						imshow("Binary", Binary);
						/////////////////////////////////////////////////////////////////
						vector< vector< Point> > contours1;//轮廓数组
						findContours(Binary, contours1, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
						////计算每个轮廓的轮廓距
						vector<Moments> mu(contours1.size());
						for (int i = 0; i < contours1.size(); i++)
						{
							mu[i] = moments(contours1[i], false);
						}
						//计算轮廓的质心     
						vector<Point2d> mc(contours1.size());
						for (int i = 0; i < contours1.size(); i++)
						{
							mc[i] = Point2d(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
						}

						//double maxArea = 0;
						vector< vector< Point> > maxContours;//提取出的最大轮廓组，因为drawContours函数必须要输入vector< vector< Point> >
						//vector<Point>maxContour;//提取出最大轮廓
						vector<Key>AllKey;
						int Max = 0;
						for (int i = 0; i < contours1.size(); i++)
						{
							double area = contourArea(contours1[i]);
							//if (area>maxArea)
							//{
							///*maxArea = area;
							//maxContour = contours1[i];*/
							//	Max = i;
							//}
							/*if (contourArea(contours1[i])>10000 || arcLength(contours1[i], true) > 400)*/
							if (contourArea(contours1[i])<50 || contourArea(contours1[i])>5000)
								contours1[i].clear();
							else
							{
								/*if (area > maxArea)
								{
									maxArea = area;
									maxContour = contours1[i];
								}*/
								Key K;
								K.Kernel = mc[i];
								K.Area = area;
								K.Contour = contours1[i];
								AllKey.push_back(K);
							}

							//m 
						}
						cout << "AllKey: " << AllKey.size() << endl;
						
						
						
						//画出空格键的质心
						/*Moments muMax = moments(maxContour, false);
						Point2d mcMax = Point2d(muMax.m10 / muMax.m00, muMax.m01 / muMax.m00);
						Key MaxK;
						MaxK.Kernel = mcMax;
						MaxK.Area = maxArea;
						MaxK.Contour = maxContour;
						AllKey.push_back(MaxK);
						circle(origin, mcMax, 5, Scalar(0, 255, 0), -1, 8, 0);*/
						/////////////////////////////////////////////////////////////////////




						/////////////////////////键位定位///////////////////////////////////

						try{
							if (AllKey.size() != 79)
								throw string("请重新放置键盘纸！！");
							const double KeyWidth = (x2 - x1) / 14;
							const double KeyHeight = y3 - y1;//先不算开机键等
							const double xielv = (double)(y2 - y1) / (double)(x2 - x1);
							/*const double angle = atan2((x2 - x1), (y2 - y1));
							const double XX = KeyHeight * cos(angle);
							const double YY = KeyHeight * sin(angle);*/
							const double b = y1 - xielv*x1;

							//建立用于存储每行键位的容器
							vector<Key>Line1Keys;
							vector<Key>Line2Keys;
							vector<Key>Line3Keys;
							vector<Key>Line4Keys;
							vector<Key>Line5Keys;
							vector<Key>Line6Keys;

							for (int i = 0; i < AllKey.size(); i++)
							{
								if (AllKey[i].Kernel.y < xielv*AllKey[i].Kernel.x + b + 0.2 * KeyHeight)
								{
									circle(origin, AllKey[i].Kernel, 5, Scalar(255, 255, 255), -1, 8, 0);
									Line6Keys.push_back(AllKey[i]);
								}
								else if (AllKey[i].Kernel.y < xielv*AllKey[i].Kernel.x + b + 0.35 * KeyHeight && AllKey[i].Kernel.y > xielv*AllKey[i].Kernel.x + b + 0.23 * KeyHeight)
								{
									circle(origin, AllKey[i].Kernel, 5, Scalar(0, 255, 255), -1, 8, 0);
									Line5Keys.push_back(AllKey[i]);
								}
								else if (AllKey[i].Kernel.y < xielv*AllKey[i].Kernel.x + b + 0.52 * KeyHeight && AllKey[i].Kernel.y > xielv*AllKey[i].Kernel.x + b + 0.38 * KeyHeight)
								{
									circle(origin, AllKey[i].Kernel, 5, Scalar(255, 0, 255), -1, 8, 0);
									Line4Keys.push_back(AllKey[i]);
								}
								else if (AllKey[i].Kernel.y < xielv*AllKey[i].Kernel.x + b + 0.65 * KeyHeight && AllKey[i].Kernel.y > xielv*AllKey[i].Kernel.x + b + 0.53 * KeyHeight)
								{
									circle(origin, AllKey[i].Kernel, 5, Scalar(255, 255, 0), -1, 8, 0);
									Line3Keys.push_back(AllKey[i]);
								}
								else if (AllKey[i].Kernel.y < xielv*AllKey[i].Kernel.x + b + 0.82 * KeyHeight && AllKey[i].Kernel.y > xielv*AllKey[i].Kernel.x + b + 0.68 * KeyHeight)
								{
									circle(origin, AllKey[i].Kernel, 5, Scalar(255, 0, 0), -1, 8, 0);
									Line2Keys.push_back(AllKey[i]);
								}
								else if (AllKey[i].Kernel.y < xielv*AllKey[i].Kernel.x + b + KeyHeight && AllKey[i].Kernel.y > xielv*AllKey[i].Kernel.x + b + 0.85 * KeyHeight)
								{
									circle(origin, AllKey[i].Kernel, 5, Scalar(0, 0, 0), -1, 8, 0);
									Line1Keys.push_back(AllKey[i]);
								}
							}

///////////////////////////////////////////这边好像有问题，有时候会溢出//////////////////////////////
							//imshow("origin", origin);
							if (Line1Keys.size() != 14 || Line2Keys.size() != 14 || (!(Line3Keys.size() == 13 && Line4Keys.size() == 14) && !(Line3Keys.size() == 14 && Line4Keys.size() == 13)) || Line5Keys.size() != 13 || Line6Keys.size() != 11)
								throw string("请重新放置键盘纸！！");

							BabblSort(Line1Keys);
							BabblSort(Line2Keys);
							BabblSort(Line3Keys);
							BabblSort(Line4Keys);
							BabblSort(Line5Keys);
							BabblSort(Line6Keys);

							//try{
							
							for (int i = 0; i < Line1Keys.size(); i++)
							{
								Line1Keys[i].Name = KeyName1[i];
								Line1Keys[i].WinIndex = KeyIndex1[i];
							}
							for (int i = 0; i < Line2Keys.size(); i++)
							{
								Line2Keys[i].Name = KeyName2[i];
								Line2Keys[i].WinIndex = KeyIndex2[i];
							}
							for (int i = 0; i < Line3Keys.size(); i++)
							{
								Line3Keys[i].Name = KeyName3[i];
								Line3Keys[i].WinIndex = KeyIndex3[i];
							}
							for (int i = 0; i < Line4Keys.size(); i++)
							{
								Line4Keys[i].Name = KeyName4[i];
								Line4Keys[i].WinIndex = KeyIndex4[i];
							}
							for (int i = 0; i < Line5Keys.size(); i++)
							{
								Line5Keys[i].Name = KeyName5[i];
								Line5Keys[i].WinIndex = KeyIndex5[i];
							}
							for (int i = 0; i < Line6Keys.size(); i++)
							{
								Line6Keys[i].Name = KeyName6[i];
								Line6Keys[i].WinIndex = KeyIndex6[i];
							}

							vector<vector<Key>>Keyboard;
							Keyboard.push_back(Line1Keys);
							Keyboard.push_back(Line2Keys);
							Keyboard.push_back(Line3Keys);
							Keyboard.push_back(Line4Keys);
							Keyboard.push_back(Line5Keys);
							Keyboard.push_back(Line6Keys);


							cout << "键位提取成功！！\n";

							//将固定好的键位信息保存
							if (Times >= 1000)
							{
								KeyboardCorrect = Keyboard;
								for (int i = 0; i < KeyboardCorrect.size(); i++)
								{
									for (int j = 0; j < KeyboardCorrect[i].size(); j++)
									{
										KeyboardAll.push_back(KeyboardCorrect[i][j]);
									}
								}

								//找出每个键位的四个顶点
								for (int i = 0; i < KeyboardAll.size(); i++)
								{
									long a = 9999999;
									long b = 9999999;
									long c = 9999999;
									long d = 9999999;
									for (int j = 0; j < KeyboardAll[i].Contour.size(); j++)
									{
										//circle(k, AllKey[i].Contour[j], 1, Scalar(255), -1, 8, 0);
										////与左上角键盘顶点坐标差值
										//int mX1 = AllKey[i].Contour[j].x - pointD.x;
										//int mY1 = AllKey[i].Contour[j].y - pointD.y;
										////右上
										//int mX2 = AllKey[i].Contour[j].x - pointC.x;
										//int mY2 = AllKey[i].Contour[j].y - pointC.y;
										////左下
										//int mX3 = AllKey[i].Contour[j].x - pointA.x;
										//int mY3 = AllKey[i].Contour[j].y - pointA.y;
										////右下
										//int mX4 = AllKey[i].Contour[j].x - pointB.x;
										//int mY4 = AllKey[i].Contour[j].y - pointB.y;
										//与左上角键盘顶点坐标差值
										int mX1 = KeyboardAll[i].Contour[j].x;
										int mY1 = KeyboardAll[i].Contour[j].y;
										//右上
										int mX2 = KeyboardAll[i].Contour[j].x - 960;
										int mY2 = KeyboardAll[i].Contour[j].y;
										//左下
										int mX3 = KeyboardAll[i].Contour[j].x;
										int mY3 = KeyboardAll[i].Contour[j].y - 540;
										//右下
										int mX4 = KeyboardAll[i].Contour[j].x - 960;
										int mY4 = KeyboardAll[i].Contour[j].y - 540;

										long dis1 = mX1*mX1 + mY1*mY1;
										long dis2 = mX2*mX2 + mY2*mY2;
										long dis3 = mX3*mX3 + mY3*mY3;
										long dis4 = mX4*mX4 + mY4*mY4;

										if (a>dis1)
										{
											a = dis1;
											KeyboardAll[i].Vertex[0] = KeyboardAll[i].Contour[j];
										}
										if (b>dis2)
										{
											b = dis2;
											KeyboardAll[i].Vertex[1] = KeyboardAll[i].Contour[j];
										}
										/*if (c>dis3)
										{
										c = dis3;
										AllKey[i].Vertex[2] = AllKey[i].Contour[j];
										}*/
										/*if (d>dis4)
										{
										d = dis4;
										AllKey[i].Vertex[3] = AllKey[i].Contour[j];
										}*/
										KeyboardAll[i].Vertex[2].x = KeyboardAll[i].Kernel.x * 2 - KeyboardAll[i].Vertex[1].x;
										KeyboardAll[i].Vertex[2].y = KeyboardAll[i].Kernel.y * 2 - KeyboardAll[i].Vertex[1].y;
										KeyboardAll[i].Vertex[3].x = KeyboardAll[i].Kernel.x * 2 - KeyboardAll[i].Vertex[0].x;
										KeyboardAll[i].Vertex[3].y = KeyboardAll[i].Kernel.y * 2 - KeyboardAll[i].Vertex[0].y;
									}
									/*for (int z = 0; z < 4; z++)
									{
									circle(origin, AllKey[i].Vertex[z], 2, Scalar(255,255,255), -1, 8, 0);
									}*/

								}

								Vertex.push_back(pointD);
								Vertex.push_back(pointC);
								Vertex.push_back(pointA);
								Vertex.push_back(pointB);

								for (int i = 0; i < 6; i++)
								{
									for (int j = 0; j < KeyboardCorrect[i].size(); j++)
									{
										cout << i << "," << j << " " << KeyboardCorrect[i][j].Name << " " << KeyboardCorrect[i][j].Kernel.x << "," << KeyboardCorrect[i][j].Kernel.y << endl;
									}
								}
								KeyDetecationOK = true;
							}

							++Times;

						}

						catch (string &aval)
						{
							SafeRelease(pColorFrame);
							if (waitKey(30) == VK_ESCAPE)
							{
								break;
							}
							cout << aval << endl;
							Times = 0;
							KeyboardAll.clear();
							KeyboardCorrect.clear();
							Vertex.clear();
							continue;
						}
						namedWindow("origin", CV_WINDOW_NORMAL);
						imshow("origin", origin);
					}
					//键盘定位完成后，进入手部提取部分
					else
					{
						//FingerTips.clear();
						
						destroyWindow("BLUE_ONLY");
						destroyWindow("去噪图");
						destroyWindow("Binary");
						destroyWindow("origin");
						//Mat res;
						//bilateralFilter(dst, res, 5, 5 * 2, 5 / 2);//高斯滤波，去除噪声
						cvtColor(dst, dst, CV_BGR2YCrCb);
						Mat hand = YCrCbHand(dst);
						
						
						//先膨胀后腐蚀，闭运算，用于消除小型黑洞
						Mat element = getStructuringElement(MORPH_ELLIPSE, Size(13, 13));
						dilate(hand, hand, element);
						element = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
						erode(hand, hand, element);
						/*element = getStructuringElement(MORPH_ELLIPSE, Size(13, 13));
						erode(hand, hand, element); */
						
						
						


						/*for (int i = 0; i < hand.rows; i++)
						{
							uchar *ptrH = hand.ptr<uchar>(i);
							for (int j = 0; j < hand.cols; j++)
							{
								if ((i<Vertex[0].y&&i<Vertex[1].y)||(i>Vertex[2].y&&i>Vertex[3].y)||(j<Vertex[0].x&&j<Vertex[2].x)||(j>Vertex[1].x&&j>Vertex[3].x))
								{
									ptrH[j] = 0;
								}
							}
						}*/

						/*for (int i = 0; i < origin.rows; i++)//直接用向量定位太费时了
						{
							uchar *ptrH = hand.ptr<uchar>(i);
							for (int j = 0; j < origin.cols; j++)
							{
								if (!InTheRectangle(Vertex[0], Vertex[1], Vertex[2], Vertex[3], j, i))
								{
									ptrH[j] = 0;
								}
							}
						}*/

						//用轮廓提取来代替形态学
						//Mat element = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
						//erode(hand, hand, element);//腐蚀
						//element = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
						//dilate(hand, hand, element);//膨胀要的尺寸大一些
						
						vector< vector< Point> > contours;//肤色轮廓数组
						findContours(hand, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);//
						//cout << "contoursHand.size(): " << contoursHand.size() << endl;
						//drawContours(origin, contoursHand, -1, Scalar(0, 0, 0), 3);
						for (int i = 0; i < contours.size(); i++)
						{
							if (arcLength(contours[i], true) <= 500)//先去除过小的的点
							{
								contours[i].clear();
							}
						}
						drawContours(hand, contours, -1, Scalar(255), CV_FILLED, 8);//复原轮廓内部，8为默认联通线型
						namedWindow("Hand", CV_WINDOW_NORMAL);
						imshow("Hand", hand);
						
						vector< vector< Point> > contoursHand;//拟指尖轮廓数组

						findContours(hand, contoursHand, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);//只提取最外层轮廓

						Mat Tip(height, width, CV_8UC1, Scalar(0, 0, 0));
						//Mat HandInRegion(height, width, CV_8UC1, Scalar(0, 0, 0));

						//for (int i = 0; i < contoursHand.size(); i++)
						//{
						//	for (int j = 0; j < contoursHand[i].size(); j++)
						//	{
						//		if (InTheRectangle(CvPoint(Vertex[0].x, Vertex[0].y - 100), CvPoint(Vertex[1].x, Vertex[1].y - 100), Vertex[2], Vertex[3], contoursHand[i][j].x, contoursHand[i][j].y))
						//			circle(HandInRegion, contoursHand[i][j], 1, Scalar(255,255,255), -1, 8, 0);
						//	}
						//}
						//vector< vector< Point> > HAND;
						//findContours(HandInRegion, HAND, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
						////找手的中心
						//vector<Moments> mu(HAND.size());
						//for (int i = 0; i < HAND.size(); i++)
						//{
						//	mu[i] = moments(HAND[i], false);
						//}
						////计算轮廓的质心     
						//vector<Point2d> mc(HAND.size());
						//for (int i = 0; i < HAND.size(); i++)
						//{
						//	mc[i] = Point2d(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
						//	circle(origin, mc[i], 3, Scalar(0, 0, 0), -1, 8, 0);
						//}

						///////////////////////////////////
						//namedWindow("HandInRegion", CV_WINDOW_NORMAL);
						//imshow("HandInRegion", HandInRegion);

						for (int i = 0; i < contoursHand.size(); i++)
						{
							for (int j = 20; (j < contoursHand[i].size() - 20) && contoursHand[i].size() > 20; j++)
							{
								double Angle = ArcCos(contoursHand[i][j], contoursHand[i][j - 20], contoursHand[i][j + 20]);
								if (Angle >= PI / 3 && Angle <= PI / 6 * 5)
								{
									if (GetCross1(contoursHand[i][j], contoursHand[i][j - 20], contoursHand[i][j + 20]) < 0)//依据（contoursHand[i][j]，contoursHand[i][j + 40]）X（contoursHand[i][j - 40]，contoursHand[i][j]）的符号来去除手指连接凹陷处的干扰
									{
										//只提取有纵坐标比其他相连点大的//只提取在键盘位置上的
										if ((contoursHand[i][j].y >= contoursHand[i][j - 20].y) && (contoursHand[i][j].y >= contoursHand[i][j + 20].y))
										{
											if (InTheRectangle(Vertex[0], Vertex[1], Vertex[2], Vertex[3], contoursHand[i][j].x, contoursHand[i][j].y))
											{
												//circle(origin, contoursHand[i][j], 3, Scalar(0, 255, 0), -1, 8, 0);
												circle(Tip, contoursHand[i][j], 1, Scalar(255), -1, 8, 0);//在黑白图像中画出指尖点轮廓
											}
										}
									}
								}
							}
						}

						vector< vector< Point> > contoursHandTip;//指尖轮廓数组
						
						/*Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
						dilate(Tip, Tip, element);*/
						findContours(Tip, contoursHandTip, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);//只提取最外层轮廓
						drawContours(Tip, contoursHandTip, -1, Scalar(255), CV_FILLED, 8);
						
						
						////对轮廓左右位置进行排序
						//for (int i = 0; i < contoursHandTip.size() - 1; i++)
						//{
						//	for (int j = 0; j < contoursHandTip.size() - 1 - i; j++)
						//	{
						//		if (mc[j].x > mc[j + 1].x)
						//		{
						//			vector<Point>temp = contoursHandTip[j];
						//			contoursHandTip[j] = contoursHandTip[j + 1];
						//			contoursHandTip[j + 1] = temp;
						//		}
						//	}
						//}

						//对指尖轮廓两两比较删除重复轮廓
						//for (int i = 0; i < contoursHandTip.size(); i++)
						//{
						//	for (int j = i + 1; j < contoursHandTip.size(); j++)
						//	{
						//		if (abs(mc[i].x - mc[j].x) < 35)
						//		{
						//			//if (contoursHandTip[i].size() >= contoursHandTip[j].size())//通过比较两个重复轮廓的点含量来删除
						//			if (mc[i].y >= mc[j].y)//通过比较重复轮廓的最低点坐标来删除
						//				contoursHandTip[j].clear();
						//			else
						//				contoursHandTip[i].clear();
						//		}
						//	}
						//}


						//放在外面的话，比较出的是唯一一个最大点
						//先拿一个指尖进行实验
						/*int maxY(0);
						Point FingerTip(0, 0);*/
						bool pushFinger = false;
						//vector<Point>FingerTip(20);

						vector<Point>FingerTips;//存储检测到的指尖点
						//寻找每个指尖轮廓的最下点作为指尖点（但指尖轮廓因为包含个数可能不准）
						for (int i = 0; i < contoursHandTip.size(); i++)
						{
							int maxY(0);
							Point FingerTip(0, 0); 

							for (int j = 0; j < contoursHandTip[i].size(); j++)
							{
								if (maxY < contoursHandTip[i][j].y)
								{
									maxY = contoursHandTip[i][j].y;
									FingerTip = contoursHandTip[i][j];
								}
							}
							//circle(origin, FingerTip, 3, Scalar(0, 0, 255), -1, 8, 0);
							FingerTips.push_back(FingerTip);//每一帧只要在键盘范围内有指尖点，就会存入FingerTips
						}

						if (FingerTips.size() != 0)
						{
							pushFinger = true;
						}

						/////////////////去除临近干扰点///////////////////
						int k = 0;
						vector<bool>Mark(FingerTips.size());
						vector<Point>Last;
						vector<Point>Last1;
						vector<Point>temp;
						Mark.insert(Mark.end(), FingerTips.size(), false);
						for (int i = 0; i < FingerTips.size(); i++)
						{
							for (int j = i + 1; j < FingerTips.size(); j++)
							{
								if (abs(FingerTips[i].x - FingerTips[j].x) < 15)
								{
									Mark[i] = true;
									Mark[j] = true;
									//选两个点的中点
									Last.push_back(Point((FingerTips[i].x + FingerTips[j].x) / 2, (FingerTips[i].y + FingerTips[j].y) / 2));
								}
							}
						}
						for (int i = 0; i < FingerTips.size(); i++)
						{
							if (Mark[i] == false)
							{
								Last.push_back(FingerTips[i]);
							}
						}

						//sort(Last.begin(), Last.end(), SortGetTwo);//指尖点排序
						
						//找左右手的最远端作为两个操纵指尖
						for (int i = 0; i < Last.size(); i++)//找键盘左侧的最远指尖点
						{
							if (Last[i].x < 0.6*Vertex[0].x + 0.4*Vertex[1].x)
							{
								//circle(origin, Last[i], 3, Scalar(0, 0, 255), -1, 8, 0);
								temp.push_back(Last[i]);
								//break;
							}
						}
						
						if (temp.size() != 0)
						{
							sort(temp.begin(), temp.end(), SortGetTwo);//指尖点排序
							Last1.push_back(temp[0]);
							circle(origin, temp[0], 3, Scalar(0, 0, 255), -1, 8, 0);
							temp.clear();
						}
							
						
						for (int i = 0; i < Last.size(); i++)//找键盘右侧的最远指尖点
						{
							if (Last[i].x >= 0.6*Vertex[0].x + 0.4*Vertex[1].x)
							{
								//circle(origin, Last[i], 3, Scalar(0, 0, 255), -1, 8, 0);
								temp.push_back(Last[i]);
								//break;
							}
						}

						if (temp.size() != 0)
						{
							sort(temp.begin(), temp.end(), SortGetTwo);//指尖点排序
							Last1.push_back(temp[0]);
							circle(origin, temp[0], 3, Scalar(0, 0, 255), -1, 8, 0);
						}

						/*if (Last.size() >= 2)
						{
							for (int i = 0; i < 2; i++)
							{
								Last1.push_back(Last[i]);
								circle(origin, Last[i], 3, Scalar(0, 0, 255), -1, 8, 0);
							}
						}*/
						
						/////////////////////////////////////////////////////

						//if (contoursHandTip.size() != 0)
						//{
						//	FingerTips.push_back(FingerTip);//每一帧只要在键盘范围内有指尖点，就会存入FingerTips
						//	pushFinger = true;//该帧有指尖点存入，进行标记
						//}
						
						
						

						////////////////////对所有键位进行遍历，如果有指尖点在键内，进行击键判断////////////////////
						//当前帧键位内指尖点的存在情况
						vector<bool>FI(79);
						vector<Point>FTIK(79);//FingerTipsInTheKey的元素，每一帧键位内指尖点的坐标信息
						//if (pushFinger)
						for (int i = 0; i < KeyboardAll.size(); i++)
						{	
							if (Last1.size() != 0)
							{
								for (int j = 0; j < Last1.size(); j++)
								{
									for (int x = Last1[j].x - 5; x < Last1[j].x + 6; x++)
									{
										for (int y = Last1[j].y - 5; y < Last1[j].y + 6; y++)
										{
											int mx = x - Last1[j].x;
											int my = y - Last1[j].y;
											if (mx*mx + my*my <= 20)//扩大指尖点范围
											{
												//四边形准则
												if (InTheRectangle1(KeyboardAll[i].Vertex[0], KeyboardAll[i].Vertex[1], KeyboardAll[i].Vertex[2], KeyboardAll[i].Vertex[3], x, y))
												{
													FI[i] = true;
													FTIK[i] = Last1[j];//如果键位内有指尖点，记录下指尖点坐标
												}
												else if (FI[i] != true)//原来没有的时候之前FI[i]==true的情况也被覆盖///////////////////////////////
												{
													FI[i] = false;
													FTIK[i] = Point(0, 0);//如果没有就存个非法值
												}
											}
										}
									}
								}
							}
							else
							{
								FI[i] = false;
								FTIK[i] = Point(0, 0);
							}
						}

						FingerTipsInTheKey.push_back(FTIK);
						FingerTipISInTheRegion.push_back(FI);//按帧塞入

						if (FingerTipISInTheRegion.size() >= 4)//上来先将最旧的一帧所有键位内指尖点信息删除
						{
							FingerTipISInTheRegion.erase(FingerTipISInTheRegion.begin());
						}
						if (FingerTipsInTheKey.size() >= 4)
						{
							FingerTipsInTheKey.erase(FingerTipsInTheKey.begin());
							
							//消除指尖点抖动
							for (int i = 0; i < 79; i++)
							{
								int mX2 = FingerTipsInTheKey[FingerTipsInTheKey.size() - 1][i].x - FingerTipsInTheKey[FingerTipsInTheKey.size() - 2][i].x;
								int mY2 = FingerTipsInTheKey[FingerTipsInTheKey.size() - 1][i].y - FingerTipsInTheKey[FingerTipsInTheKey.size() - 2][i].y;
								if (mX2*mX2 + mY2*mY2 <= 60)
								{
									FingerTipsInTheKey[FingerTipsInTheKey.size() - 1][i] = FingerTipsInTheKey[FingerTipsInTheKey.size() - 2][i];
								}
							}
							
						}
						///////////////////////////////////////////////////////////
						
						//
						//标记这一帧内每个键内指尖点是否抬起
						bool FingerMove[79] = { false };
						vector<bool>FingerStatic(79, false);
						if (pushFinger)//这一帧检测到了指尖点
						{
							for (int i = 0; i < FingerTipsInTheKey.size(); i++)
							{
								for (int j = 0; j < FingerTipsInTheKey[i].size(); j++)
								{
									if (FI[j] == true)//如果这一帧键位内有指尖点
									{
										//circle(origin, FingerTipsInTheKey[FingerTipsInTheKey.size() - 1][j], 5, Scalar(255, 255, 0), -1, 8, 0);
										int x = FingerTipsInTheKey[FingerTipsInTheKey.size() - 1][j].x;//这一帧的指尖坐标
										int y = FingerTipsInTheKey[FingerTipsInTheKey.size() - 1][j].y;
										int xx = FingerTipsInTheKey[FingerTipsInTheKey.size() - 2][j].x;
										int yy = FingerTipsInTheKey[FingerTipsInTheKey.size() - 2][j].y;
										/*int xxx = FingerTipsInTheKey[FingerTipsInTheKey.size() - 3][j].x;
										int yyy = FingerTipsInTheKey[FingerTipsInTheKey.size() - 3][j].y;*/
										/*int xxx = FingerTips[FingerTips.size() - 3].x;
										int yyy = FingerTips[FingerTips.size() - 3].y;*/
										int mX = x - xx;
										int mY = y - yy;
										/*int mX1 = xx - xxx;
										int mY1 = yy - yyy;*/
										/*int mX1 = FingerTips[FingerTips.size() - 2].x - FingerTips[FingerTips.size() - 3].x;
										int mY1 = FingerTips[FingerTips.size() - 2].y - FingerTips[FingerTips.size() - 3].y;*/
										if (mY < -7)//连续两帧要有上移标记为抬起
										{
 											FingerMove[j] = true;//抬起指尖动作标记
										}
										else if (mX == 0 && mY == 0)
										{
											FingerStatic[j] = true;
										}
											
									}
									
								}
							}
							
						}

						KeyIsStatic.push_back(FingerStatic);

						int FingerMoveNum = 0;//不能同时抬起多个手指
						for (int i = 0; i < 79; i++)
						{
							if (FingerMove[i] == true)
							{
								FingerMoveNum++;
							}
						}

						vector<bool>KIP(79, false);
						if (KeyIsStatic.size() >= 4)
						{
							KeyIsStatic.erase(KeyIsStatic.begin());
							//判断是否被键入
							for (int i = 0; i < 79; i++)
							{
								if (FingerMove[i])//如果此帧键内指尖点抬起了
								{
									//如果这一帧指尖点抬起的情况下，前两帧该键位内都存在指尖点的话，继续
									if (KeyIsStatic[KeyIsStatic.size() - 2][i] && KeyIsStatic[KeyIsStatic.size() - 3][i])
									{
										KIP[i] = true;
									}

									/*if (FingerTipISInTheRegion[FingerTipISInTheRegion.size() - 2][i] == true && FingerTipISInTheRegion[FingerTipISInTheRegion.size() - 3][i] == true)
									{
									keybd_event(KeyboardAll[i].WinIndex, 0, 0, 0);
									keybd_event(KeyboardAll[i].WinIndex, 0, KEYEVENTF_KEYUP, 0);
									circle(origin, KeyboardAll[i].Kernel, 5, Scalar(255, 255, 0), -1, 8, 0);
									KeyIsPushed[i] = true;
									}*/
								}
							}
						}

						KeyIsPushed.push_back(KIP);

						if (FingerMoveNum == 1)
						if (KeyIsPushed.size() >= 4)
						{
							KeyIsPushed.erase(KeyIsPushed.begin());
							for (int i = 0; i < 79; i++)
							{
								if (KeyIsPushed[KeyIsPushed.size() - 1][i] == true && KeyIsPushed[KeyIsPushed.size() - 2][i] == false && KeyIsPushed[KeyIsPushed.size() - 3][i] == false)//这一帧有抬起动作而前两帧没有
								{
									keybd_event(KeyboardAll[i].WinIndex, 0, 0, 0);
									keybd_event(KeyboardAll[i].WinIndex, 0, KEYEVENTF_KEYUP, 0);
									circle(origin, KeyboardAll[i].Kernel, 5, Scalar(255, 255, 0), -1, 8, 0);
									break;
								}
							}
						}
						//if (contoursHandTip.size() != 0)
						//{
						//	FingerTips.push_back(FingerTip);//每一帧只要在键盘范围内有指尖点，就会存入FingerTips
						//	pushFinger = true;
						//}
						////消除指尖点抖动
						//if (FingerTips.size() >= 2)
						//{
						//	int mX2 = FingerTips[FingerTips.size() - 1].x - FingerTips[FingerTips.size() - 2].x;
						//	int mY2 = FingerTips[FingerTips.size() - 1].y - FingerTips[FingerTips.size() - 2].y;
						//	if (mX2*mX2 + mY2*mY2 <= 20)
						//	{
						//		FingerTips[FingerTips.size() - 1] = FingerTips[FingerTips.size() - 2];
						//	}
						//}

						//circle(origin, FingerTip, 5, Scalar(0, 0, 255), -1, 8, 0);
						//
						//string name = "NULL";
						//double Juli = 10000.0;
						////判断指尖点是否静止了一帧
						//if (FingerTips.size() >= 5 && pushFinger)
						//{
						//	FingerTips.erase(FingerTips.begin());
						//	int x = FingerTips[FingerTips.size() - 1].x;//这一帧的指尖坐标
						//	int y = FingerTips[FingerTips.size() - 1].y;
						//	int xx = FingerTips[FingerTips.size() - 2].x;
						//	int yy = FingerTips[FingerTips.size() - 2].y;
						//	int mX = FingerTips[FingerTips.size() - 2].x - FingerTips[FingerTips.size() - 3].x;
						//	int mY = FingerTips[FingerTips.size() - 2].y - FingerTips[FingerTips.size() - 3].y;
						//	int mX1 = FingerTips[FingerTips.size() - 3].x - FingerTips[FingerTips.size() - 4].x;
						//	int mY1 = FingerTips[FingerTips.size() - 3].y - FingerTips[FingerTips.size() - 4].y;
						//	int mX2= FingerTips[FingerTips.size() - 1].x - FingerTips[FingerTips.size() - 2].x;
						//	int mY2 = FingerTips[FingerTips.size() - 1].y - FingerTips[FingerTips.size() - 2].y;
						//	
						//	//if (mX1*mX1 + mY1*mY1 <= 20 && mX*mX + mY*mY <= 20 && y < y - mY2 - 2)//前后两帧指尖点满足空间距离小于阈值则认为静止
						//	if (mX1*mX1 + mY1*mY1 <= 25 && mX*mX + mY*mY <= 25 && mY2 < -1 && mX2*mX2 + mY2*mY2 > 25)
						//	{
						//		sort(KeyboardAll.begin(), KeyboardAll.end(), cmpMy);//对所有按键按照到指尖的距离进行排序
						//		
						//		vector<Key>NearestKeys;//4个最临近的键位
						//		vector<Key>CandidateKeys;
						//		bool IsIn[4] = { false, false, false, false };
						//		int IntheKeyNum[4] = { 0, 0, 0, 0 };
						//		double SpaceRatio[4] = { 0.0, 0.0, 0.0, 0.0 };
						//		for (int i = 0; i < 4; i++)
						//		{
						//			NearestKeys.push_back(KeyboardAll[i]);
						//		}
						//		for (int i = xx - 5; i < xx + 6; i++)
						//		{
						//			for (int j = yy - 5; j < yy + 6; j++)
						//			{
						//				if ((i - xx)*(i - xx) + (j - yy)*(j - yy) <= 25)
						//				{
						//					for (int k = 0; k < 4; k++)
						//					{
						//						if (InTheRectangle1(NearestKeys[k].Vertex[0], NearestKeys[k].Vertex[1], NearestKeys[k].Vertex[2], NearestKeys[k].Vertex[3], i, j))
						//						{
						//							IsIn[k] = true;
						//							++IntheKeyNum[k];
						//						}
						//					}
						//					
						//				}
						//			}
						//		}
						//		for (int i = 0; i < 4; i++)
						//		{
						//			if (IsIn[i] == true)
						//			{
						//				CandidateKeys.push_back(NearestKeys[i]);
						//			}
						//		}
						//		
						//		double Space = 1.0;
						//		Key Last1;
						//		for (int i = 0; i < CandidateKeys.size(); i++)
						//		{
						//			if (CandidateKeys[i].Name == "SPACE")
						//			{
						//				SpaceRatio[i] = IntheKeyNum[i] * 5 / CandidateKeys[i].Area;
						//			}
						//			else if (CandidateKeys[i].Name == "RIGHTSHIFT")
						//			{
						//				SpaceRatio[i] = IntheKeyNum[i] * 2 / CandidateKeys[i].Area;
						//			}
						//			else
						//			{
						//				SpaceRatio[i] = IntheKeyNum[i] / CandidateKeys[i].Area;
						//			}
						//			if (Space > SpaceRatio[i])
						//			{
						//				Space = SpaceRatio[i];
						//				Last1 = CandidateKeys[i];
						//			}
						//			/*circle(origin, CandidateKeys[i].Kernel, 5, Scalar(255,255,0), -1, 8, 0);
						//			cout << CandidateKeys[i].Name << endl;*/
						//		}
						//		//win按键事件
						//		if (Last1.Name != "NULL"&&Last1.WinIndex != 0)
						//		{
						//			keybd_event(Last1.WinIndex, 0, 0, 0);
						//			keybd_event(Last1.WinIndex, 0, KEYEVENTF_KEYUP, 0);
						//		}
						//		
						//		circle(origin, Last1.Kernel, 5, Scalar(255, 255, 0), -1, 8, 0);
						//		cout << Last1.Name << endl;
						//		/*if (Last1.Name == "A")
						//		{
						//			keybd_event(65, 0, 0, 0);
						//			keybd_event(65, 0, KEYEVENTF_KEYUP, 0);
						//		}*/
						//			

						//	}
						//}

						
						//去除临近干扰点
						//int k = 0;
						//vector<bool>Mark(FingerTips.size());
						//vector<Point>Last1;
						//Mark.insert(Mark.end(), FingerTips.size(), false);
						//for (int i = 0; i < FingerTips.size(); i++)
						//{
						//	for (int j = i + 1; j < FingerTips.size(); j++)
						//	{
						//		if (abs(FingerTips[i].x - FingerTips[j].x) < 10)
						//		{
						//			Mark[i] = true;
						//			Mark[j] = true;
						//			//选两个点的中点
						//			Last1.push_back(Point((FingerTips[i].x + FingerTips[j].x) / 2, (FingerTips[i].y + FingerTips[j].y) / 2));
						//		}
						//	}
						//}
						//for (int i = 0; i < FingerTips.size(); i++)
						//{
						//	if (Mark[i] == false)
						//	{
						//		Last1.push_back(FingerTips[i]);
						//	}
						//}
						////指尖点从左往右排序
						//for (int i = 0; i < Last1.size(); i++)
						//{
						//	for (int j = i + 1; j < Last1.size(); j++)
						//	if (Last1[i].x>Last1[j].x)
						//		swap(Last1[i], Last1[j]);
						//}

						//for (int i = 0; i < Last1.size(); i++)
						//{
						//	circle(origin, Last1[i], 5, Scalar(0, 0, 255), -1, 8, 0);
						//}

							
					

						
						
						
						
						namedWindow("Tip", CV_WINDOW_NORMAL);
						imshow("Tip", Tip);
						
						//只取键盘部分
						//for (int i = 0; i < origin.rows; i++)
						//{
						//	uchar *ptr = origin.ptr<uchar>(i);  //第i行的指针                    
						//	for (int j = 0; j < origin.cols; j++)
						//	{
						//		if ((i<Vertex[0].y&&i<Vertex[1].y) || (i>Vertex[2].y&&i>Vertex[3].y) || (j<Vertex[0].x&&j<Vertex[2].x) || (j>Vertex[1].x&&j>Vertex[3].x))
						//		{
						//			ptr[4 * j] = 0;  //内部数据是4个字节，0-1-2是BGR，第4个现在未使用   
						//			ptr[4 * j + 1] = 0;
						//			ptr[4 * j + 2] = 0;
						//		}
						//	}
						//}
						namedWindow("HandOrigin", CV_WINDOW_NORMAL);
						imshow("HandOrigin", origin);
					}


					
					
					
					//
					////maxContours.push_back(Keyboard[Line - 1][clo - 1].Contour);
					//drawContours(origin, maxContours, -1, Scalar(255, 255, 255), CV_FILLED, CV_AA);



					//createTrackbar(pstrWindowsToolBar, "Keyboard", &nThresholdEdge, 200, on_trackbar);//创建滑动条控制Canny阈值
					//on_trackbar(nThresholdEdge,0);//回调函数用来进行Canny边缘检测
					//dilate(dst5, dst5, Mat(2, 2, CV_8U), Point(-1, -1), 1);//进行膨胀操作
					//std::vector<cv::Vec4i> lines = finder.findLines(dst5);//数据量太大做不了直线检测
					//finder.drawDetectedLines(dst1, cv::Scalar(0, 0, 255));

					//imshow("Keyboard", dst);
				}
			}

			SafeRelease(pColorFrame);
			/*
			imshow("Keyboard", dst1);
			imshow("Binary", dst5);*/


			if (waitKey(30) == VK_ESCAPE)
			{
				break;
			}


	}

	return 0;
}

//Kinect原彩色图像是4通道，转为3通道
Mat BGRA2BGR(Mat &bufferMat)
{
	Mat result(height, width, CV_8UC3);

	//将原本Kinect输出的BGRA格式转化为RGB格式
	for (int i = 0; i<bufferMat.rows; i++)
	{
		uchar *ptr = result.ptr<uchar>(i);  //第i行的指针                    
		//每个字节代表一个颜色信息，直接使用uchar 
		uchar *pBuffer = bufferMat.ptr<uchar>(i);
		for (int j = 0; j<bufferMat.cols; j++)
		{
			ptr[3 * j] = pBuffer[4 * j];  //内部数据是4个字节，0-1-2是BGR，第4个现在未使用   
			ptr[3 * j + 1] = pBuffer[4 * j + 1];
			ptr[3 * j + 2] = pBuffer[4 * j + 2];
		}
	}
	return result;
}

//YCrCb下的蓝色分割
Mat YCrCb(const Mat &bufferMat)
{
	Mat result(height, width, CV_8UC1);
	Mat Y, Cr, Cb;
	vector<Mat> channels;
	split(bufferMat, channels);
	Y = channels.at(0);
	Cr = channels.at(1);
	Cb = channels.at(2);

	

	for (int j = 0; j < Y.rows; j++)
	{
		uchar* currentY = Y.ptr< uchar>(j);
		uchar* currentCr = Cr.ptr< uchar>(j);
		uchar* currentCb = Cb.ptr< uchar>(j);
		uchar* current = result.ptr< uchar>(j);
		for (int i = 0; i < Y.cols; i++)
		{
			if ((currentCb[i] >= 140))
			//if ((currentY[i] >= 0) && (currentY[i] <= 255) && (currentCr[i] > 127) && (currentCr[i] < 129) && (currentCb[i] > 127) && (currentCb[i] < 129))//肤色140≤Cr≤200，100≤Cb≤150
			{
				current[i] = 255;
				
			}
			else
				current[i] = 0;
			//提取笔
			//if (currentY[i] >= 200 && currentCr[i] <= 128)
			//{
			//	current[i] = 128;
			//	//K = false;
			//}
		}
	}
	return result;
}


Mat BGRWhite(const Mat &bufferMat)
{
	Mat result(height, width, CV_8UC1);
	Mat Y, Cr, Cb;
	vector<Mat> channels;
	split(bufferMat, channels);
	Y = channels.at(0);
	Cr = channels.at(1);
	Cb = channels.at(2);

	//bool K = true;

	for (int j = 0; j < Y.rows; j++)
	{
		uchar* currentY = Y.ptr< uchar>(j);
		uchar* currentCr = Cr.ptr< uchar>(j);
		uchar* currentCb = Cb.ptr< uchar>(j);
		uchar* current = result.ptr< uchar>(j);
		for (int i = 0; i < Y.cols; i++)
		{
			//if ((currentY[i] >= 0) && (currentY[i] <= 255) && (currentCr[i] > 127) && (currentCr[i] < 129) && (currentCb[i] > 127) && (currentCb[i] < 129))//肤色140≤Cr≤200，100≤Cb≤150
			if (currentY[i] >= 130 && currentCr[i] <= 128)
			{
				current[i] = 128;
				//K = false;
			}
			else
				current[i] = 0;
		}
	}
	return result;
}

//键位排序，先根据键的纵坐标，再根据横坐标
bool FindKey(const Point2d &point1, const Point2d &point2, double xielv)
{
	double x1 = point1.x;
	double x2 = point2.x;
	double y1 = point1.y;
	double y2 = point2.y;

	if (x1 != x2)
	{
		if (abs((y2 - y1) / (x2 - x1) - xielv) < 0.08)
			return true;
		return false;
	}
	return false;
}

//按照X轴坐标从小到大排列一行键位		
void BabblSort(vector<Key>&Line1Keys)
{
	for (int i = 0; i < Line1Keys.size() - 1; i++)
	{
		for (int j = 0; j < Line1Keys.size() - 1 - i; j++)
		{
			if (Line1Keys[j].Kernel.x > Line1Keys[j + 1].Kernel.x)
			{
				Key temp;
				temp = Line1Keys[j];
				Line1Keys[j] = Line1Keys[j + 1];
				Line1Keys[j + 1] = temp;
			}
		}
	}
}

Mat YCrCb1(const Mat &bufferMat)
{
	Mat result(height, width, CV_8UC1);
	Mat Y, Cr, Cb;
	vector<Mat> channels;
	split(bufferMat, channels);
	Y = channels.at(0);
	Cr = channels.at(1);
	Cb = channels.at(2);
	/*namedWindow("Cr", CV_WINDOW_NORMAL);
	imshow("Cr", Cr);*/
	//threshold(Cr, Cr, 0, 255, CV_THRESH_OTSU);//OTSU最大类间方差的阈值提取
	for (int j = 0; j < Y.rows; j++)
	{
		uchar* currentY = Y.ptr< uchar>(j);
		uchar* currentCr = Cr.ptr< uchar>(j);
		uchar* currentCb = Cb.ptr< uchar>(j);
		//uchar* current = result.ptr< uchar>(j);
		for (int i = 0; i < Y.cols; i++)
		{
			//if ((currentCr[i] > 160) && (currentCb[i] > 100) && (currentCb[i] < 150))
			//if ((currentCr[i] > 133) && (currentCr[i] < 173) && (currentCb[i] == 0))//肤色140≤Cr≤200，100≤Cb≤150
			//if ((currentCb[i] >= 77) && (currentCb[i] <= 127) && (currentCr[i] == 255))
			if ((currentCb[i]) <= 150)
			{
				currentY[i] = 0;
				currentCr[i] = 128;
				currentCb[i] = 128;
			}
		}
	}
	threshold(Cb, Cb, 0, 255, CV_THRESH_OTSU);//OTSU最大类间方差的阈值提取
	/*namedWindow("dst1", CV_WINDOW_NORMAL);
	imshow("dst1", Cb);*/


	
	for (int j = 0; j < Y.rows; j++)
	{
		uchar* currentY = Y.ptr< uchar>(j);
		uchar* currentCr = Cr.ptr< uchar>(j);
		uchar* currentCb = Cb.ptr< uchar>(j);
		uchar* current = result.ptr< uchar>(j);
		for (int i = 0; i < Y.cols; i++)
		{
			//if ((currentCr[i] > 160) && (currentCb[i] > 100) && (currentCb[i] < 150))
			//if ((currentCr[i] > 133) && (currentCr[i] < 173) && (currentCb[i] == 0))//肤色140≤Cr≤200，100≤Cb≤150
			//if ((currentCb[i] >= 77) && (currentCb[i] <= 127) && (currentCr[i] == 255))
			if ((currentCb[i] == 255))
			{
				current[i] = 255;
				//K = false;
			}
			else
				current[i] = 0;
			//提取笔
			//if (currentY[i] >= 200 && currentCr[i] <= 128)
			//{
			//	current[i] = 128;
			//	//K = false;
			//}
		}
	}
	return result;
}
Mat YCrCbHand(const Mat &bufferMat)//用不用OTSU提取都没什么关系，因为键盘和手的色差本来就很大
{
	Mat result(height, width, CV_8UC1);
	Mat Y, Cr, Cb;
	vector<Mat> channels;
	split(bufferMat, channels);
	Y = channels.at(0);
	Cr = channels.at(1);
	Cb = channels.at(2);

	//for (int j = 0; j < Y.rows; j++)
	//{
	//	uchar* currentY = Y.ptr< uchar>(j);
	//	uchar* currentCr = Cr.ptr< uchar>(j);
	//	uchar* currentCb = Cb.ptr< uchar>(j);
	//	//uchar* current = result.ptr< uchar>(j);
	//	for (int i = 0; i < Y.cols; i++)
	//	{
	//		//if ((currentCr[i] > 160) && (currentCb[i] > 100) && (currentCb[i] < 150))
	//		//if ((currentCr[i] > 133) && (currentCr[i] < 173) && (currentCb[i] == 0))//肤色140≤Cr≤200，100≤Cb≤150
	//		//if ((currentCb[i] >= 77) && (currentCb[i] <= 127) && (currentCr[i] == 255))
	//		if (currentCb[i] < 77 || currentCb[i] > 127)//先用传统的肤色Cb值进行预分割
	//		{
	//			currentY[i] = 0;
	//			currentCr[i] = 128;
	//			currentCb[i] = 128;
	//		}
	//	}
	//}

	//OTSU最大类间方差的阈值提取
	//threshold(Cr, Cr, 0, 255, CV_THRESH_OTSU);

	for (int j = 0; j < Y.rows; j++)
	{
		uchar* currentY = Y.ptr< uchar>(j);
		uchar* currentCr = Cr.ptr< uchar>(j);
		uchar* currentCb = Cb.ptr< uchar>(j);
		uchar* current = result.ptr< uchar>(j);
		for (int i = 0; i < Y.cols; i++)
		{
			if ((currentCr[i] >= 133) && (currentCr[i] <= 173) && (currentCb[i] >= 77 && currentCb[i] <= 127))
				//if ((currentY[i] >= 0) && (currentY[i] <= 255) && (currentCr[i] > 127) && (currentCr[i] < 129) && (currentCb[i] > 127) && (currentCb[i] < 129))//肤色140≤Cr≤200，100≤Cb≤150
			{
				current[i] = 255;
			}
			else
				current[i] = 0;
			//提取笔
			//if (currentY[i] >= 200 && currentCr[i] <= 128)
			//{
			//	current[i] = 128;
			//	//K = false;
			//}
		}
	}
	//OTSU最大类间方差的阈值提取
	//threshold(Cr, Cr, 0, 255, CV_THRESH_OTSU);
	//
	//for (int j = 0; j < Y.rows; j++)
	//{
	//	uchar* currentCr = Cr.ptr< uchar>(j);
	//	uchar* current = result.ptr< uchar>(j);
	//	for (int i = 0; i < Y.cols; i++)
	//	{
	//		if (currentCr[i] != 255)//放小点范围
	//		{
	//			current[i] = 0;
	//		}
	//	}
	//}

	return result;
}

bool SortGetTwo(const Point &v1, const Point &v2)//根据指尖点纵坐标降序排列
{
	return v1.y > v2.y;
}