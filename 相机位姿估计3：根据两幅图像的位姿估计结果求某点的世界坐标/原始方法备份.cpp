// SlovePNPByOpenCV.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <opencv2\opencv.hpp>
#include <math.h>
#include <iostream>
#include <fstream>
#include "SolvePNPBy4Points.h"

#include "GetDistanceOf2linesIn3D.h"
using namespace std;

//���ռ����Z����ת
//������� x yΪ�ռ��ԭʼx y����
//thetazΪ�ռ����Z����ת���ٶȣ��Ƕ��Ʒ�Χ��-180��180
//outx outyΪ��ת��Ľ������
void codeRotateByZ(double x, double y, double thetaz, double& outx, double& outy)
{
	double x1 = x;//����������һ�Σ���֤&x == &outx���������Ҳ�ܼ�����ȷ
	double y1 = y;
	double rz = thetaz * CV_PI / 180;
	outx = cos(rz) * x1 - sin(rz) * y1;
	outy = sin(rz) * x1 + cos(rz) * y1;
}

//���ռ����Y����ת
//������� x zΪ�ռ��ԭʼx z����
//thetayΪ�ռ����Y����ת���ٶȣ��Ƕ��Ʒ�Χ��-180��180
//outx outzΪ��ת��Ľ������
void codeRotateByY(double x, double z, double thetay, double& outx, double& outz)
{
	double x1 = x;
	double z1 = z;
	double ry = thetay * CV_PI / 180;
	outx = cos(ry) * x1 + sin(ry) * z1;
	outz = cos(ry) * z1 - sin(ry) * x1;
}

//���ռ����X����ת
//������� y zΪ�ռ��ԭʼy z����
//thetaxΪ�ռ����X����ת���ٶȣ��Ƕ��ƣ���Χ��-180��180
//outy outzΪ��ת��Ľ������
void codeRotateByX(double y, double z, double thetax, double& outy, double& outz)
{
	double y1 = y;//����������һ�Σ���֤&y == &y���������Ҳ�ܼ�����ȷ
	double z1 = z;
	double rx = thetax * CV_PI / 180;
	outy = cos(rx) * y1 - sin(rx) * z1;
	outz = cos(rx) * z1 + sin(rx) * y1;
}


//��������������ת������ϵ
//�������old_x��old_y��old_zΪ��תǰ�ռ�������
//vx��vy��vzΪ��ת������
//thetaΪ��ת�ǶȽǶ��ƣ���Χ��-180��180
//����ֵΪ��ת�������
cv::Point3f RotateByVector(double old_x, double old_y, double old_z, double vx, double vy, double vz, double theta)
{
	double r = theta * CV_PI / 180;
	double c = cos(r);
	double s = sin(r);
	double new_x = (vx*vx*(1 - c) + c) * old_x + (vx*vy*(1 - c) - vz*s) * old_y + (vx*vz*(1 - c) + vy*s) * old_z;
	double new_y = (vy*vx*(1 - c) + vz*s) * old_x + (vy*vy*(1 - c) + c) * old_y + (vy*vz*(1 - c) - vx*s) * old_z;
	double new_z = (vx*vz*(1 - c) - vy*s) * old_x + (vy*vz*(1 - c) + vx*s) * old_y + (vz*vz*(1 - c) + c) * old_z;
	return cv::Point3f(new_x, new_y, new_z);
}
void test();

vector<cv::Point2f> Points2D;//���ڶ�λ���ĸ�������
vector<cv::Point2f> Points2DNeedToFind;
cv::Mat imgDisp;

//��ͼ������ĵ�(pix)ת�����������ϵ(mm)��
//��������ڲ���
//double FΪ��ͷ����
cv::Point3f imageFrame2CameraFrame(cv::Point2f p, double fx, double fy, double u0, double v0, double F)
{
	double zc = F;
	double xc = (p.x - u0)*F / fx;
	double yc = (p.y - v0)*F / fy;
	return cv::Point3f(xc, yc, zc);
}

void on_mouse(int event, int x, int y, int flag, void *param)
{
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		//ֻ��¼�ĸ����ٵ㡣
		if (Points2D.size() < 4)
		{
			Points2D.push_back(cv::Point2f(x, y));
			cout << "add 1 point :" << Points2D.size() << "/4" << endl;
			cv::circle(imgDisp,  cv::Point2f(x, y), 25, cv::Scalar(255, 0, 0), 5);
			cv::imshow("CamPos", imgDisp);
		}
		else if (Points2DNeedToFind.size() == 0)
		{
			Points2DNeedToFind.push_back(cv::Point2f(x, y));
			cv::circle(imgDisp, cv::Point2f(x, y), 5, cv::Scalar(0, 255, 255), -1);
			cv::imshow("CamPos", imgDisp);
		}
	}
}

double Dot(cv::Point3f a, cv::Point3f b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
cv::Point3f Cross(cv::Point3f a, cv::Point3f b){ return cv::Point3f(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
double Length(cv::Point3f a) { return sqrt(Dot(a, a)); }

int main(int argc, _TCHAR* argv[])
{
	double camD[9] = {
		6800.7, 0, 3065.8,
		0, 6798.1, 1667.6,
		0, 0, 1 };

	double fx = camD[0];
	double fy = camD[4];
	double u0 = camD[2];
	double v0 = camD[5];


	SolvePNPBy4Points sp4p;
	//��ʼ���������
	sp4p.SetCameraMatrix(fx, fy, u0, v0);
	//�������
	sp4p.SetDistortionCoefficients(-0.189314, 0.444657, -0.00116176, 0.00164877, -2.57547);




	sp4p.Points3D.push_back(cv::Point3f(0, 0, 0));    //��ά����ĵ�λ�Ǻ���
	sp4p.Points3D.push_back(cv::Point3f(0, 200, 0));
	sp4p.Points3D.push_back(cv::Point3f(150, 0, 0));
	sp4p.Points3D.push_back(cv::Point3f(150, 200, 0));

	cv::namedWindow("CamPos", 0);
	cvSetMouseCallback("CamPos", on_mouse, NULL);//��������¼���ͨ���������������㡣

	cv::Mat img1 = cv::imread("1.jpg");
	cv::Mat img2 = cv::imread("2.jpg");
	cv::Point3d vec1;//��������ϵ���������ϵ��λ��
	cv::Point3d vec2;
	cv::Point2f point2find1_IF;//������ͼ������ϵ����
	cv::Point2f point2find2_IF ;//������ͼ������ϵ����
	cv::Point3f point2find1_CF;
	cv::Point3f point2find2_CF;


	/********��һ��ͼ********/
	sp4p.Points2D.push_back(cv::Point2f(2985, 1688));
	sp4p.Points2D.push_back(cv::Point2f(5077, 1684));
	sp4p.Points2D.push_back(cv::Point2f(2998, 2793));
	sp4p.Points2D.push_back(cv::Point2f(5546, 2754));
	Points2DNeedToFind.push_back(cv::Point2f(4149, 671));

	if (sp4p.Solve() != 0)
		return -1;
	vec1.x = sp4p.Position_OcInW.x;
	vec1.y = sp4p.Position_OcInW.y;
	vec1.z = sp4p.Position_OcInW.z;
	point2find1_IF = cv::Point2f(Points2DNeedToFind[0]);
	Points2D.clear();
	Points2DNeedToFind.clear();
	sp4p.Points2D.clear();


	//�����һ��ͼ��ֱ�߷���
	point2find1_CF = imageFrame2CameraFrame(point2find1_IF, fx, fy, u0, v0, 350);//�����
	double x1 = point2find1_CF.x;
	double y1 = point2find1_CF.y;
	double z1 = point2find1_CF.z;
	//�������η�����ת
	codeRotateByZ(x1, y1, sp4p.Theta_W2C.z, x1, y1);
	codeRotateByY(x1, z1, sp4p.Theta_W2C.y, x1, z1);
	codeRotateByX(y1, z1, sp4p.Theta_W2C.x, y1, z1);

	//����ȷ��һ��ֱ��
	cv::Point3f a1(sp4p.Position_OcInW.x, sp4p.Position_OcInW.y, sp4p.Position_OcInW.z);
	cv::Point3f a2(sp4p.Position_OcInW.x + x1, sp4p.Position_OcInW.y + y1, sp4p.Position_OcInW.z + z1);
	cv::Point3f l1vector(x1, y1, z1);//ֱ��1�ķ�������



	/********��2��ͼ********/
	//imgDisp = img2;
	//cv::imshow("CamPos", imgDisp);
	//while (Points2D.size() < 4 || Points2DNeedToFind.size() < 1)
	//{
	//	if (cv::waitKey(5) == 'r')
	//		Points2D.clear();
	//}
	//ofstream fout2("2.txt");
	//fout2 << Points2D << endl;
	//fout2 << Points2DNeedToFind << endl;
	//fout2.close();

	//for (int i = 0; i < Points2D.size(); i++)
	//{
	//	if (Points2D.size() >= 4)//�������㳬���ĸ�ʱ
	//	{
	//		sp4p.Points2D.push_back(Points2D[i]);
	//	}
	//}


	sp4p.Points2D.push_back(cv::Point2f(3062, 3073));
	sp4p.Points2D.push_back(cv::Point2f(3809, 3089));
	sp4p.Points2D.push_back(cv::Point2f(3035, 3208));
	sp4p.Points2D.push_back(cv::Point2f(3838, 3217));
	Points2DNeedToFind.push_back(cv::Point2f(3439, 2691));


	if (sp4p.Solve() != 0)
		return -1;
	vec2.x = sp4p.Position_OcInW.x;
	vec2.y = sp4p.Position_OcInW.y;
	vec2.z = sp4p.Position_OcInW.z;
	point2find2_IF = cv::Point2f(Points2DNeedToFind[0]);
	Points2D.clear();
	Points2DNeedToFind.clear();

	/**********************/

	//����ڶ���ͼ��ֱ�߷���
	point2find2_CF = imageFrame2CameraFrame(point2find2_IF, fx, fy, u0, v0, 350);//�����
	double x2 = point2find2_CF.x;
	double y2 = point2find2_CF.y;
	double z2 = point2find2_CF.z;
	//�������η�����ת
	codeRotateByZ(x2, y2, sp4p.Theta_W2C.z, x2, y2);
	codeRotateByY(x2, z2, sp4p.Theta_W2C.y, x2, z2);
	codeRotateByX(y2, z2, sp4p.Theta_W2C.x, y2, z2);

	//����ȷ��һ��ֱ��
	cv::Point3f b1(sp4p.Position_OcInW.x, sp4p.Position_OcInW.y, sp4p.Position_OcInW.z);
	cv::Point3f b2(sp4p.Position_OcInW.x + x2, sp4p.Position_OcInW.y + y2, sp4p.Position_OcInW.z + z2);
	cv::Point3f l2vector(x2, y2, z2);//ֱ��2�ķ�������


	if (1 == 1)
	{
		/*************************����ֱ�������**************************/
		//http://blog.sina.com.cn/s/blog_a401a1ea0101ij9z.html
		//������˻�ù���������
		//a=(X1,Y1,Z1),b=(X2,Y2,Z2),
		//a��b=��Y1Z2-Y2Z1,Z1X2-Z2X1,X1Y2-X2Y1��
		double X1 = l1vector.x, Y1 = l1vector.y, Z1 = l1vector.z;
		double X2 = l2vector.x, Y2 = l2vector.y, Z2 = l2vector.z;
		cv::Point3f N(Y1*Z2 - Y2*Z1, Z1*X2 - Z2*X1, X1*Y2 - X2*Y1);//������N

		//����̾���
		/*����ֱ���Ϸֱ�ѡȡ��A, B(����)���õ�����AB��
			������AB������N�����ͶӰ��Ϊ������ֱ�߼�ľ����ˣ�������̾���������*/

		cv::Point3f AB(b1.x - a1.x, b1.y - a1.y, b1.z - a1.z);

		/*a��b�ϵ�ͶӰ�� | a | cos<a, b >= a*b / | b |
			�磺
			a = (1, 2, 3)
			b = (2, 1, 4)
			a��b�ϵ�ͶӰΪ��
			a*b = 2 + 2 + 12 = 16
			| b |= ��(2 ^ 2 + 1 ^ 2 + 4 ^ 2) = ��21
			a��b�ϵ�ͶӰΪ��
			16 / ��21         */

		double axb = Dot(AB, N);
		double N_abs = sqrt(Dot(N, N));
		double dist = axb / N_abs;//�������ֱ�ߵľ���

		//�轻��ΪC, D�����빫����N�ĶԳ�ʽ�У�����ΪC, D����ֱ�����һ��ʼ��ֱ�߷��̣����Եõ�����C����D�����������ȷ��̣��ֱ������ͺ��ˣ�
		//C.x =  


		//�������ԣ�http://blog.csdn.net/pi9nc/article/details/11820545
		cv::Point3f d1 = a2 - a1, d2 = b2 - b1;
		cv::Point3f e = b1 - a1;
		cv::Point3f cross_e_d2 = Cross(e, d2);
		cv::Point3f cross_e_d1 = Cross(e, d1);
		cv::Point3f cross_d1_d2 = Cross(d1, d2);

		double t1, t2;
		t1 = Dot(cross_e_d2, cross_d1_d2);
		t2 = Dot(cross_e_d1, cross_d1_d2);
		double dd = Length((Cross(d1, d2)));
		t1 /= dd*dd;
		t2 /= dd*dd;
		//�õ������λ��
		cv::Point3f ans1, ans2;
		ans1 = (a1 + (a2 - a1)*t1);
		ans2 = (b1 + (b2 - b1)*t2);
		printf("%.6f %.6f %.6f\n", ans1.x, ans1.y, ans1.z);
		printf("%.6f %.6f %.6f\n", ans2.x, ans2.y, ans2.z);
		printf("middle: %.6f %.6f %.6f\n", (ans1.x + ans2.x) / 2, (ans1.y + ans2.y) / 2, (ans1.z + ans2.z) / 2);




		GetDistanceOf2linesIn3D g;//��ʼ��
		g.SetLineA(a1.x, a1.y, a1.z, a2.x, a2.y, a2.z);//����ֱ��A�ϵ�����������
		g.SetLineB(b1.x, b1.y, b1.z, b2.x, b2.y, b2.z);//����ֱ��B�ϵ�����������
		g.GetDistance();//�������
		double d = g.distance;//��þ���
		double x = g.PonA_x;//���AB�������������A���ϵĵ��x����
		double y = g.PonA_y;//���AB�������������A���ϵĵ��y����
		double z = g.PonA_z;//���AB�������������A���ϵĵ��z����
	}
	test();
	return 0;
}

void test()
{

}