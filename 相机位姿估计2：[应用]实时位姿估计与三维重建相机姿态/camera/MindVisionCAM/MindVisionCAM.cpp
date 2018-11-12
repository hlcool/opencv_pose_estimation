#include "MindVisionCAM.h"

//Ϊ500W��ҵ�����װ���࣬����ǰȷ��MindVisionCAMSDK.lib����
MindVisionCAM::MindVisionCAM()
{

}


MindVisionCAM::~MindVisionCAM()
{
	Release();
}

bool MindVisionCAM::Init()
{
	//������豸��Ϣ
	tSdkCameraDevInfo sCameraList[10];
	INT iCameraNums = 10;//����CameraEnumerateDeviceǰ��������iCameraNums = 10����ʾ���ֻ��ȡ10���豸�������Ҫö�ٸ�����豸�������sCameraList����Ĵ�С��iCameraNums��ֵ
	CameraSdkStatus status;

	if (CameraEnumerateDevice(sCameraList, &iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		printf("No camera was found!");
		return FALSE;
	}

	//��ʾ���У�����ֻ����������һ���������ˣ�ֻ��ʼ����һ�������(-1,-1)��ʾ�����ϴ��˳�ǰ����Ĳ���������ǵ�һ��ʹ�ø�����������Ĭ�ϲ���.
	//In this demo ,we just init the first camera.
	if ((status = CameraInit(&sCameraList[0], -1, -1, &m_hCamera)) != CAMERA_STATUS_SUCCESS)
	{
		char msg[128];
		sprintf_s(msg, "Failed to init the camera! Error code is %d", status);
		printf(msg);
		return FALSE;
	}


	//Get properties description for this camera.
	tSdkCameraCapbility sCameraInfo;
	CameraGetCapability(m_hCamera, &sCameraInfo);// ������������


	CameraSetAeState(m_hCamera, FALSE);//��������ع��ģʽ���Զ������ֶ���bState��TRUE��ʹ���Զ��ع⣻FALSE��ֹͣ�Զ��ع⡣
	CameraSetExposureTime(m_hCamera, 1000 * ExposureTimeMS);//�ع�ʱ��10ms = 1000΢��*10
	CameraSetAnalogGain(m_hCamera, 10 * AnalogGain);//����ģ������16=1.6  ��ֵ���� CameraGetCapability  ��õ�������Խṹ����sExposeDesc.fAnalogGainStep ���͵õ�ʵ�ʵ�ͼ���źŷŴ�����
	if (ColorType == CV_8U)
	{
		CameraSetMonochrome(m_hCamera, TRUE);//���úڰ�ͼ��
	}
	CameraSetFrameSpeed(m_hCamera, sCameraInfo.iFrameSpeedDesc - 1);//�趨������ͼ���֡�ʡ�iFrameSpeedSel��ѡ���֡��ģʽ�����ţ���Χ�� 0 ��CameraGetCapability ��õ���Ϣ�ṹ����	iFrameSpeedDesc - 1


	//����ռ�
	m_pFrameBuffer = (BYTE *)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax*sCameraInfo.sResolutionRange.iWidthMax * 3, 16);


	if (sCameraInfo.sIspCapacity.bMonoSensor)//ISP ����������BOOL bMonoSensor; //��ʾ���ͺ�����Ƿ�Ϊ�ڰ����,����Ǻڰ����������ɫ��صĹ��ܶ��޷�����
	{
		CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_MONO8);
	}

	//������������������ô��ڡ�
	CameraCreateSettingPage(m_hCamera, NULL, "cam", NULL, NULL, 0);//"֪ͨSDK�ڲ��������������ҳ��";

	CameraSetMirror(m_hCamera, 1,TRUE);

	//�����ʼ�����
	CameraShowSettingPage(m_hCamera, TRUE);//TRUE��ʾ������ý��档FALSE�����ء�

	//����ROI
	//tSdkImageResolution sRoiResolution;
	//memset(&sRoiResolution, 0, sizeof(sRoiResolution));
	//sRoiResolution.iIndex = 0xff;
	//sRoiResolution.iWidth = 784;
	//sRoiResolution.iWidthFOV = sRoiResolution.iWidth * 3;
	//sRoiResolution.iHeight = 100;
	//sRoiResolution.iHeightFOV = sRoiResolution.iHeight * 3;
	//sRoiResolution.iWidthZoomHd = 0;
	//sRoiResolution.iHeightZoomHd = 0;
	//sRoiResolution.iHOffsetFOV = 128;
	//sRoiResolution.iVOffsetFOV = 470;
	//sRoiResolution.iWidthZoomSw = 0;
	//sRoiResolution.iHeightZoomSw = 0;
	//sRoiResolution.uBinAverageMode = 0;
	//sRoiResolution.uBinSumMode = 0;
	//sRoiResolution.uResampleMask = 0;
	//sRoiResolution.uSkipMode = 2;
	//CameraSetImageResolution(m_hCamera, &sRoiResolution);//����Ԥ���ķֱ��ʡ�

	HasInited = true;
	return true;
}

void MindVisionCAM::GetFrame(cv::Mat& img)
{
	tSdkFrameHead 	sFrameInfo;
	CameraHandle    hCamera = (CameraHandle)m_hCamera;
	BYTE*			pbyBuffer;
	CameraSdkStatus status;

	if (CameraGetImageBuffer(hCamera, &sFrameInfo, &pbyBuffer, 1000) == CAMERA_STATUS_SUCCESS)
	{
		//����õ�ԭʼ����ת����RGB��ʽ�����ݣ�ͬʱ����ISPģ�飬��ͼ����н��룬������������ɫУ���ȴ���
		//�ҹ�˾�󲿷��ͺŵ������ԭʼ���ݶ���Bayer��ʽ��
		status = CameraImageProcess(hCamera, pbyBuffer, m_pFrameBuffer, &sFrameInfo);//����ģʽ

		if (status == CAMERA_STATUS_SUCCESS)
		{
			////ʹ��IplImage
			////����SDK��װ�õ���ʾ�ӿ�����ʾͼ��,��Ҳ���Խ�m_pFrameBuffer�е�RGB����ͨ��������ʽ��ʾ������directX,OpengGL,�ȷ�ʽ��
			//CameraImageOverlay(hCamera, m_pFrameBuffer, &sFrameInfo);
			//IplImage *iplImage = NULL;
			//if (iplImage)
			//{
			//	cvReleaseImageHeader(&iplImage);
			//}
			//iplImage = cvCreateImageHeader(cvSize(sFrameInfo.iWidth, sFrameInfo.iHeight), IPL_DEPTH_8U, sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3);
			//cvSetData(iplImage, m_pFrameBuffer, sFrameInfo.iWidth*(sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3));

			//cv::Mat img(iplImage);
			//cv::imshow("123", img);

			//if (iplImage)
			//{
			//	cvReleaseImageHeader(&iplImage);
			//}

			//ֱ����MAT
			cv::Mat OriginalImage;
			if (ColorType == CV_8U)
				OriginalImage = cv::Mat(sFrameInfo.iHeight, sFrameInfo.iWidth, CV_8U, m_pFrameBuffer);
			else
				OriginalImage = cv::Mat(sFrameInfo.iHeight, sFrameInfo.iWidth, CV_8UC3, m_pFrameBuffer);
			//ȥ�ױ�
			img = OriginalImage(cv::Rect(2, 2, sFrameInfo.iWidth - 4, sFrameInfo.iHeight - 4)).clone();
		}

		//�ڳɹ�����CameraGetImageBuffer�󣬱������CameraReleaseImageBuffer���ͷŻ�õ�buffer��
		//�����ٴε���CameraGetImageBufferʱ�����򽫱�����ֱ�������߳��е���CameraReleaseImageBuffer���ͷ���buffer
		CameraReleaseImageBuffer(hCamera, pbyBuffer);
	}
}
