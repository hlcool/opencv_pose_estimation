#include "stdafx.h"

#include <opencv2\opencv.hpp>
#include "camera\MindVisionCAM\MindVisionCAM.h"
#include "..\include\PNPSolver.h"
using namespace std;


//������չʾһ��ʵʱ���λ�˹��Ƶ����̣����е�ԭ����ǰ�����Ѿ�˵���ˣ����á����λ�˹���1_1��OpenCV��solvePnP���η�װ�����ܲ��ԡ��й�������
//ʹ�ó�������Ӽ򵥡�����������HSV�ռ䣬���ٺ�ɫ�������㣬�����ٵ������������ڽ�PNP���⣬�õ����λ�ˣ���������������������������ת�ǣ���
//���ʹ��labview�е���άͼƬ�ؼ���������ϵͳ����3D�ؽ���

//@author:VShawn(singlex@foxmail.com)
//˵�����µ�ַ��http://www.cnblogs.com/singlex/p/pose_estimation_2.html




vector<cv::Point2f> lastCenters;//��¼��һ�����������������

//���ڵ�������¼�
//���һ�����һ������������
void on_mouse(int event, int x, int y, int flag, void *param)
{
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		//ֻ��¼�ĸ����ٵ㡣
		if (lastCenters.size() < 4)
		{
			lastCenters.push_back(cv::Point2f(x, y));
			cout << "add 1 point :" << lastCenters.size() << "/4" << endl;
		}
	}
}

//����������
//������㸽�����Һ�ɫ����������ģ���Ϊ�������µ�λ��
//����Ϊ��1��ǰͼ��2��������������һ�ֵ�λ��
//���ر��θ��ٽ��
cv::Point2f tracking(cv::Mat image, const cv::Point2f lastcenter)
{
	//cv::GaussianBlur(image, image, cv::Size(11, 11), 0);
	/***********��ʼ��ROI**********/
	const int r = 100;//���뾶
	const int r2 = r * 2;

	int startx = lastcenter.x - r;
	int starty = lastcenter.y - r;
	if (startx < 0)
		startx = 0;
	if (starty < 0)
		starty = 0;

	int width = r2;
	int height = r2;
	if (startx + width >= image.size().width)
		startx = image.size().width - 1 - width;
	if (starty + height >= image.size().height)
		starty = image.size().height - 1 - height;

	cv::Mat roi = image(cv::Rect(startx, starty, width, height));
	cv::Mat roiHSV;
	cv::cvtColor(roi, roiHSV, CV_BGR2HSV);//��BGRͼ��תΪHSVͼ��

	vector<cv::Mat> hsv;
	cv::split(roiHSV, hsv);//��hsv����ͨ������
	cv::Mat h = hsv[0];
	cv::Mat s = hsv[1];
	cv::Mat v = hsv[2];


	cv::Mat roiBinary = cv::Mat::zeros(roi.size(), CV_8U);//��ֵͼ��255�ĵط���ʾ�ж�Ϊ��ɫ

	/*************�ж���ɫ****************/
	const double ts = 0.5 * 255;//s��ֵ��С�ڸ�ֵ���ж�
	const double tv = 0.1 * 255;//v��ֵ��С�ڸ�ֵ���ж�
	const double th = 0 * 180 / 360;//h����
	const double thadd = 30 * 180 / 360;//h��Χ��th��thadd�ڵĲű������Ǻ�ɫ

	//ͨ���ض���ֵ����HSVͼ����ж�ֵ��
	for (int i = 0; i < roi.size().height; i++)
	{
		uchar *ptrh = h.ptr<uchar>(i);
		uchar *ptrs = s.ptr<uchar>(i);
		uchar *ptrv = v.ptr<uchar>(i);
		uchar *ptrbin = roiBinary.ptr<uchar>(i);

		for (int j = 0; j < roi.size().width; j++)
		{
			if (ptrs[j] < ts || ptrv[j] < tv)
				continue;
			if (th + thadd > 180)
				if (ptrh[j] < th - thadd && ptrh[j] > th + thadd - 180)
					continue;
			if (th - thadd < 0)
				if (ptrh[j] < th - thadd + 180 && ptrh[j] > th + thadd)
					continue;

			ptrbin[j] = 255;//�ҳ���ɫ�����ص㣬�ڶ�Ӧλ�ñ��255
		}
	}

	/*****************�Զ�ֵ��ͼ�������ͨ�� ����****************/
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(roiBinary.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	//���ܻ��ж����ͨ�򣬷ֱ��������ǵ�����
	std::vector<cv::Point2f> gravityCenters;//���ĵ㼯
	for (int i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() < 10)//��ͨ��̫С
			continue;

		int xsum = 0;
		int ysum = 0;
		for (int j = 0; j < contours[i].size(); j++)
		{
			xsum += contours[i][j].x;
			ysum += contours[i][j].y;
		}
		double gpx = xsum / contours[i].size();
		double gpy = ysum / contours[i].size();
		gravityCenters.push_back(cv::Point2f(gpx + startx, gpy + starty));
	}

	/*********************�������ŵ�******************/
	//�ҵ����ĸ���һ��λ����ӽ����Ǹ�
	cv::Point ret = lastcenter;
	double dist = 1000000000;
	double distX = 1000000000;
	double distY = 1000000000;
	for (int i = 0; i < gravityCenters.size(); i++)
	{
		if (distX > abs(lastcenter.x - gravityCenters[i].x) && distY > abs(lastcenter.y - gravityCenters[i].y))
		{
			double newdist = sqrt((lastcenter.x - gravityCenters[i].x)*(lastcenter.x - gravityCenters[i].x) + (lastcenter.y - gravityCenters[i].y)*(lastcenter.y - gravityCenters[i].y));
			if (dist > newdist)
			{
				distX = abs(lastcenter.x - gravityCenters[i].x);
				distY = abs(lastcenter.y - gravityCenters[i].y);
				dist = newdist;
				ret = gravityCenters[i];
			}
		}
	}
	return ret;
}
int main()
{
	//��ʼ��������˴��滻Ϊ��������������
	MindVisionCAM mvcam;
	mvcam.ExposureTimeMS = 60;//�����ع�ʱ��
	mvcam.AnalogGain = 4;//����ģ������
	if (mvcam.Init())
	{
		//�ɹ���ʼ�������������ͼ
		mvcam.StartCapture();

		//����һ������������ʾ
		cv::namedWindow("CamPos", 0);
		cvSetMouseCallback("CamPos", on_mouse, NULL);//��������¼���ͨ����������׷�ٵ㡣

		//����ڲ���
		double fx = 1196.98;
		double fy = 1194.61;
		double u0 = 634.075;
		double v0 = 504.842;
		//��ͷ�������
		double k1 = -0.475732;
		double k2 = 0.405008;
		double p1 = 0.00196334;
		double p2 = -0.00201087;
		double k3 = -0.337634;

		//��ʼ��λ�˹�����
		PNPSolver p4psolver;

		//��ʼ���������
		p4psolver.SetCameraMatrix(fx, fy, u0, v0);
		//���û������
		p4psolver.SetDistortionCoefficients(k1, k2, p1, p2, k3);
		
		//�������������������ӽ�ȥ
		p4psolver.Points3D.push_back(cv::Point3f(0, 0, 0));		//P1��ά����ĵ�λ�Ǻ���
		p4psolver.Points3D.push_back(cv::Point3f(0, 200, 0));	//P2
		p4psolver.Points3D.push_back(cv::Point3f(150, 0, 0));	//P3
		p4psolver.Points3D.push_back(cv::Point3f(150, 200, 0));	//P4
		//p4psolver.Points3D.push_back(cv::Point3f(0, 100, 105));	//P5

		std::cout << "������Ļ�ϵ��ȷ�������㣬������ĵ�ѡ˳��Ӧ��p4psolver.Points3D�Ĵ洢˳��һ�¡�" << endl;
		while (cv::waitKey(1) != 27)//����ecs���˳�����
		{
			if (cv::waitKey(1) == 'r')//����r����������������
			{
				lastCenters.clear();
				std::cout << "������������㣬����������Ļ�ϵ��ȷ�������㡣" << endl;
			}

			//��ȡ�����ǰ֡ͼ���޸�Ϊ����������
			cv::Mat img = mvcam.Grub();
			cv::Mat paintBoard = cv::Mat::zeros(img.size(), CV_8UC3);//�½�һ��Mat�����ڴ洢���ƵĶ���


			//׷�ٲ�����������λ��
			if (lastCenters.size() > 0)
			{
				//ͨ��HSV��ɫ׷��������
				for (int i = 0; i < lastCenters.size(); i++)
				{
					lastCenters[i] = tracking(img, lastCenters[i]);//����׷�ٵ�����������
					cv::circle(paintBoard, lastCenters[i], 8, cv::Scalar(0, 255, 0), 3);//�����ٵ��ĵ������paintBoard��
				}
			}


			//�����������㹻ʱ�������λ��
			if (lastCenters.size() >= 4)
			{
				//���Ƚ�λ�˹������ڵ�����������������¼��0��
				p4psolver.Points2D.clear();
				//Ȼ���¸��ٵ��������������������
				for (int i = 0; i < lastCenters.size(); i++)
				{
					p4psolver.Points2D.push_back(lastCenters[i]);
				}
				//��λ��
				p4psolver.Solve(PNPSolver::METHOD::CV_P3P);


				//������ͶӰ��ͼ�񣬼���ͶӰ���Ƿ���ȷ
				vector<cv::Point3f> r;
				r.push_back(cv::Point3f(0, 100, 105));//��ͶӰ���¼�����
				vector<cv::Point2f>	ps = p4psolver.WordFrame2ImageFrame(r);
				//����ͶӰ�����paintBoard��
				for (int i = 0; i < ps.size(); i++)
				{
					cv::circle(paintBoard, ps[i], 5, cv::Scalar(0, 255, 255), -1);
				}

				//���λ����Ϣ��txt
				ofstream fout1("D:\\pnp_theta.txt");
				fout1 << p4psolver.Theta_W2C.x << endl << p4psolver.Theta_W2C.y << endl << p4psolver.Theta_W2C.z << endl;
				fout1.close();
				ofstream fout2("D:\\pnp_t.txt");
				fout2 << p4psolver.Position_OcInW.x << endl << p4psolver.Position_OcInW.y << endl << p4psolver.Position_OcInW.z << endl;
				fout2.close();
			}

			//����ǰ֡ͼ��+paintBoardͼ����ʾ����
			cv::imshow("CamPos", (img - paintBoard) + paintBoard + paintBoard);
		}
		//�˳������ͷ����
		mvcam.Release();
	}
	else
		std::cout << "�����ʼ��ʧ�ܣ�" << endl;
	return 0;
}