#include<iostream>
#include<string>
#include<math.h>
#include<time.h>
#include<sstream>
#include<climits>
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
using namespace std;
using namespace cv;
#define totalbrick 16

void skin_color(Mat& color, Mat_<uchar>& mask,
	int R_thr = 95, int G_thr = 40, int B_thr = 20,
	int Max_min_diff_thr = 15, int R_G_diff_thr = 15) {

	// color¬°BGR¼v¹³
	for (int y = 0; y < color.rows; ++y) {
		Vec3b* b = (Vec3b*)color.ptr(y);
		Vec3b* eb = b + color.cols;
		uchar*p = mask.ptr(y);
		memset(p, 0, mask.cols);
		for (; b != eb; ++b, ++p) {
			Vec3b& px = *b;
			if (px[2]>R_thr&& px[0]>B_thr && px[1]>G_thr &&
				px[2]>px[1] && px[2]>px[0] && (px[2] - px[1])>R_G_diff_thr &&
				(px[2] - (px[0] <= px[1] ? px[0] : px[1]))>Max_min_diff_thr) {
				*p = 1;
			}
		}
	}
	return;
}

void show_pic(Mat& frame, pair<Mat, Mat>& pic, int x, int y) {
	Mat canvas = Mat(frame, Rect(x, y, pic.first.cols, pic.first.rows));
	cvAnd(&((CvMat)canvas), &((CvMat)pic.second), &((CvMat)canvas));
	cvOr(&((CvMat)canvas), &((CvMat)pic.first), &((CvMat)canvas));
}

int main(void) {
	VideoCapture gcapture;
	gcapture.open(0);

	VideoWriter videoout;

	if (!gcapture.isOpened()) {
		cerr << "Failed to open a video device or video file!\n" << endl;
		return 1;
	}

	srand(time(NULL));

	// The frame size of the output video
	Size size = Size((int)gcapture.get(CV_CAP_PROP_FRAME_WIDTH) * 2, (int)gcapture.get(CV_CAP_PROP_FRAME_HEIGHT));

	pair<Mat, Mat> ball;
	pair<Mat, Mat> bb;
	pair<Mat, Mat>brick[totalbrick];
	ball.first = imread("ball.png", 1);
	ball.second = imread("ball_mask.png", 1);
	bb.first = imread("bb.png", 1);
	bb.second = imread("bb_mask.png", 1);
	brick[0].first = imread("brick.png", 1);
	brick[0].second = imread("brick_mask.png", 1);

	Mat canvas(gcapture.get(CV_CAP_PROP_FRAME_HEIGHT), 2 * gcapture.get(CV_CAP_PROP_FRAME_WIDTH), CV_8UC3);
	Mat cmask = Mat(canvas, Rect(0, 0, gcapture.get(CV_CAP_PROP_FRAME_WIDTH), gcapture.get(CV_CAP_PROP_FRAME_HEIGHT)));
	Mat frame = Mat(canvas, Rect(gcapture.get(CV_CAP_PROP_FRAME_WIDTH), 0, gcapture.get(CV_CAP_PROP_FRAME_WIDTH), gcapture.get(CV_CAP_PROP_FRAME_HEIGHT)));
	Mat_<uchar> mask(gcapture.get(CV_CAP_PROP_FRAME_HEIGHT), gcapture.get(CV_CAP_PROP_FRAME_WIDTH));
	Mat curframe;
	Mat cframe;
	vector<int> hskin(gcapture.get(CV_CAP_PROP_FRAME_WIDTH));

	Point ballPos;
	Point brickPos[totalbrick];
	Point2f speed;

	ballPos.x = 320;
	ballPos.y = 320;

	int roi_x0 = 0;
	int roi_x1 = gcapture.get(CV_CAP_PROP_FRAME_WIDTH) - ball.first.cols;
	int roi_y0 = 0;
	int roi_y1 = gcapture.get(CV_CAP_PROP_FRAME_HEIGHT) - ball.first.rows;
	int bbPos = 0;

	bool b[16] = { true };

	for (int i = 0; i < totalbrick; i++) {
		brick[0].first.copyTo(brick[i].first);
		brick[0].second.copyTo(brick[i].second);

		brickPos[i].x = 80 + i % 4 * (brick[i].first.cols + 10);
		brickPos[i].y = 80 + i / 4 * (brick[i].first.rows + 10);
		
		b[i] = true;
	}
	// fire a ball
	float angle = rand() % 360 / 180. * CV_PI;
	float s = 15; // initial ball speed

	speed.x = s * cos(angle);
	speed.y = s * sin(angle);

	int countdown = 20;

	while (countdown) {
		gcapture >> curframe;
		if (curframe.empty())
			return 1;

		curframe.copyTo(cframe);
		flip(cframe, cframe, 1);   // flip the image horizontally
		cframe.copyTo(frame);

		skin_color(cframe, mask); // detect pixels of skin color

		int maxid = 0;
		int maxv = INT_MIN;
		for (int i = 0; i < mask.cols; ++i) {
			hskin[i] = sum(Mat(mask, Rect(i, 0, 1, mask.rows)))[0];
			int temp;
			if (hskin[i] > maxv) {
				maxid = i;
				maxv = hskin[i];
			}
		}

		int newbbPos = maxid - bb.first.cols / 2;
		if (newbbPos < 0)
			newbbPos = 0;
		else if (newbbPos + bb.first.cols >= mask.cols)
			newbbPos = mask.cols - bb.first.cols - 1;

		bbPos = newbbPos;

		show_pic(frame, bb, bbPos, frame.rows - bb.first.rows - 1);

		if (ballPos.y > roi_y1) {
			putText(frame, string("Game Over"), Point(60, 240), FONT_HERSHEY_SIMPLEX, 3, Scalar(255, 0, 0), 10);
			countdown--;
		}

		if (ballPos.y < roi_y0)
			speed.y = -speed.y;

		if (ballPos.x < roi_x0 || ballPos.x > roi_x1)
			speed.x = -speed.x;

		for (int i = 0; i < totalbrick; i++) {
			if (b[i]) {
				if (ballPos.y + ball.first.rows > brickPos[i].y && ballPos.y < brickPos[i].y + brick[i].first.rows &&
					ballPos.x > brickPos[i].x && ballPos.x < brickPos[i].x + brick[0].first.cols) {
					speed.y = -speed.y;
					b[i] = false;
				}
				else if (ballPos.x + ball.first.cols > brickPos[i].x && ballPos.x < brickPos[i].x + brick[i].first.cols &&
					ballPos.y > brickPos[i].y && ballPos.y < brickPos[i].y + brick[i].first.rows) {
					speed.x = -speed.x;
					b[i] = false;
				}
				show_pic(frame, brick[i], brickPos[i].x, brickPos[i].y);
			}
		}
		

		if (ballPos.y + ball.first.rows > frame.rows - bb.first.rows &&
			ballPos.x >= bbPos && ballPos.x < bbPos + bb.first.cols)
			speed.y = -speed.y;

		if (ballPos.x >= roi_x0 && ballPos.x < roi_x1 && ballPos.y >= roi_y0 && ballPos.y < roi_y1) {
			show_pic(frame, ball, ballPos.x, ballPos.y);
		}

		ballPos.x += speed.x;
		ballPos.y += speed.y;

		mask *= 255;
		cvtColor(mask, cmask, CV_GRAY2BGR);

		for (int i = 1; i < hskin.size(); ++i) {
			line(cmask, Point(i - 1, cmask.rows - hskin[i - 1]), Point(i, cmask.rows - hskin[i]), Scalar(0, 0, 255));
		}

		imshow("HW2", canvas);

		if (!videoout.isOpened()) {
			cout << "Creating file" << endl;
			Size size = Size((int)canvas.cols, (int)canvas.rows);

			videoout.open("HW2.avi", VideoWriter::fourcc('X', 'V', 'I', 'D'), 24, size, true);

			if (!videoout.isOpened()) {
				cout << "Failed to open result video" << endl;
				gcapture.release();
				return 1;
			}
		}
		videoout << canvas;
		waitKey(1);
	}
	return 0;
}