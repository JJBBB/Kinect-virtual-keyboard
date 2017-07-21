#include"stdafx.h"
#include <opencv2/opencv.hpp>
#include <math.h>


const double Pi = 3.1415926;

class LineFinder
{
private:
	cv::Mat img; // original image
	std::vector<cv::Vec4i> lines;
	double deltaRho;
	double deltaTheta;
	int minVote;

	double minLength; // min length for a line
	double maxGap; // max allowed gap along the line
public:
	// Default accumulator resolution is 1 pixel by 1 degree
	// no gap, no mimimum length
	LineFinder() : deltaRho(1),
		deltaTheta(Pi / 180),
		minVote(10),
		minLength(0.),
		maxGap(0.) {}
	// Set the resolution of the accumulator
	void setAccResolution(double dRho, double dTheta)
	{
		deltaRho = dRho;
		deltaTheta = dTheta;
	}
	// Set the minimum number of votes
	void setMinVote(int minv)
	{
		minVote = minv;
	}
	// Set line length and gap
	void setLineLengthAndGap(double length, double gap)
	{
		minLength = length;
		maxGap = gap;
	}
	// Apply probabilistic Hough Transform
	std::vector<cv::Vec4i> findLines(cv::Mat& binary)
	{
		lines.clear();
		cv::HoughLinesP(binary, lines, deltaRho, deltaTheta, minVote, minLength, maxGap);
		return lines;
	}
	// Draw the detected lines on an image
	void drawDetectedLines(cv::Mat &image, cv::Scalar color = cv::Scalar(255, 255, 255))
	{
		// Draw the lines
		std::vector<cv::Vec4i>::const_iterator it2 = lines.begin();
		while (it2 != lines.end())
		{
			cv::Point pt1((*it2)[0], (*it2)[1]);
			cv::Point pt2((*it2)[2], (*it2)[3]);
			cv::line(image, pt1, pt2, color, 2);
			++it2;
		}
	}
};
