#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "stdafx.h"
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <WinUser.h>

using namespace cv;
using namespace std;

const double PI = 3.1415926;

enum vertex{ A = 0, B, C, D };//每个键4个顶点的标号

struct Key
{
	Point2d Kernel = cvPoint(0, 0);//键的质心坐标
	//CvPoint Vertex[4] = { cvPoint(0, 0), cvPoint(0, 0), cvPoint(0, 0), cvPoint(0, 0) };//顶点初始化
	double Area = 0;//每个键的面积
	vector<Point>Contour;//每个键的轮廓
	string Name = "NULL";
	CvPoint XiangDui = cvPoint(0, 0);
	Point Vertex[4];//每个键的左上，右上，左下，右下顶点
	unsigned char WinIndex = 0;
};

const vector<string> KeyName1 = { "ESC", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "POWER" };
const vector<unsigned char>KeyIndex1 = { 27, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 0 };
const vector<string> KeyName2 = { "~", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "BACKSPACE" };
const vector<unsigned char>KeyIndex2 = { 192, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 109, 107, 8 };
const vector<string> KeyName3 = { "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "ENTER" };
const vector<unsigned char>KeyIndex3 = { 9, 81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 216, 221, 13 };
const vector<string> KeyName4 = { "CAPSLK", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "|", "ENTER" };
const vector<unsigned char>KeyIndex4 = { 20, 65, 83, 68, 70, 71, 72, 74, 75, 76, 186, 222, 220, 13 };
const vector<string> KeyName5 = { "LEFTSHIFT", "`", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/", "RIGHTSHIFT" };
const vector<unsigned char>KeyIndex5 = { 16, 192, 90, 88, 67, 86, 66, 78, 77, 188, 190, 191, 16 };
const vector<string> KeyName6 = { "FN", "CTRL", "LEFTALT", "LEFTCMD", "SPACE", "RIGHTCMD", "RIGHTALT", "LEFT", "DOWN", "UP", "RIGHT" };
const vector<unsigned char>KeyIndex6 = { 0, 17, 18, 36, 32, 36, 18, 37, 40, 38, 39 };



#endif
