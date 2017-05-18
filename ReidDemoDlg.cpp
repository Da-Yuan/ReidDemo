// ReidDemoDlg.cpp : ʵ���ļ�
//

/*
Copyright 2017.4 by Da_Yuan

������Ƶ�Ƽ���С
�ֱ��� 360*240 (4:3)
֡�� 30

���±༭��
CString str;
str.Format(_T("%d * %d"), rectA.Width(), rectA.Height());
m_edit_test.SetWindowTextW(str);
SetDlgItemText(IDD_YOURCTRL, str);
*/

#include "stdafx.h"
#include "ReidDemo.h"
#include "ReidDemoDlg.h"
#include "afxdialogex.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>

#include "Saliency.h"

using namespace cv;
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///����
static Point origin;
static Mat srcImageA, srcImageB, srcImageXR;
static CRect rectA, rectB, rectXR, rectHist;
//static Mat dstImageA, dstImageB;
static std::vector<cv::Rect> regionsA;
static std::vector<cv::Rect> regionsB;
static MatND HistXR;
static std::vector<cv::MatND> srcHistB;
static vector<double> compareResult;
// ��Ƶ
static VideoCapture captureA, captureB;
static const std::string offvideoAddressA = "./res/clip1.avi";
static const std::string offvideoAddressB = "./res/clip2.avi";
static VideoCapture vcapA, vcapB;
static const std::string videoStreamAddressA = "http://192.168.1.102:81/videostream.cgi?user=admin&pwd=888888&.mjpg";
static const std::string videoStreamAddressB = "http://192.168.1.142:81/videostream.cgi?user=admin&pwd=888888&.mjpg";
// HOG����
static cv::HOGDescriptor hog
(
	cv::Size(64, 128), cv::Size(16, 16),
	cv::Size(8, 8), cv::Size(8, 8), 9, 1, -1,
	cv::HOGDescriptor::L2Hys, 0.2, true, cv::HOGDescriptor::DEFAULT_NLEVELS
);
// ֱ��ͼ����
static int hbins = 9, sbins = 16;
static int histSize[] = { hbins,sbins };
static int channels[] = { 0,1 };
static float hRange[] = { 0,180 };
static float sRange[] = { 0,256 };
const float* ranges[] = { hRange,sRange };
// ��־λ
bool offvideoFlagA = false;
bool offvideoFlagB = false;
bool OLvideoFalgA = false;
bool OLvideoFalgB = false;

///����
/* @brief �õ�һ��ͼ���ֱ��ͼ����
@param srcImage ����ͼ��
@param srcHist ���ֱ��ͼ
@return void ����ֵΪ��
*/
void getHist(const Mat srcImage, MatND &srcHist) {
	Mat srcHsvImage;

	cvtColor(srcImage, srcHsvImage, CV_RGB2HSV);

	calcHist(&srcHsvImage, 1, channels, Mat(), srcHist, 2, histSize, ranges, true, false);
	normalize(srcHist, srcHist, 0, 1, NORM_MINMAX, -1, Mat());
}

/* @brief �õ�H������ֱ��ͼͼ��
@param src ����ͼ��
@param histimg �����ɫֱ��ͼ
@return void ����ֵΪ��
*/
void getHistImg(const Mat src, Mat &histimg) {
	Mat hue, hist;

	int hsize = 9;//ֱ��ͼbin����
	float hranges[] = { 0,180 };
	const float* phranges = hranges;

	int ch[] = { 0, 0 };
	hue.create(src.size(), src.depth());
	mixChannels(&src, 1, &hue, 1, ch, 1);

	calcHist(&hue, 1, 0, Mat(), hist, 1, &hsize, &phranges);

	normalize(hist, hist, 0, 255, NORM_MINMAX);

	histimg = Scalar::all(0);
	int binW = histimg.cols / hsize;
	Mat buf(1, hsize, CV_8UC3);
	for (int i = 0; i < hsize; i++)
		buf.at<Vec3b>(i) = Vec3b(saturate_cast<uchar>(i*180. / hsize), 255, 255);
	cvtColor(buf, buf, COLOR_HSV2BGR);

	for (int i = 0; i < hsize; i++)
	{
		int val = saturate_cast<int>(hist.at<float>(i)*histimg.rows / 255);
		rectangle(histimg, Point(i*binW, histimg.rows),
			Point((i + 1)*binW, histimg.rows - val),
			Scalar(buf.at<Vec3b>(i)), -1, 8);
	}
}

/* @brief Saliency_CVPR2010
@param src ����ͼ��
@param dst �������ͼ��
@return void ����ֵΪ��
*/
void Sal2010(Mat src, Mat &salImage) {
	Saliency sal;
	vector<unsigned int >imgInput;
	vector<double> imgSal;

	//Mat to vector
	//srcImageXR.convertTo();
	int nr = src.rows; // number of rows  
	int nc = src.cols; // total number of elements per line  
	if (src.isContinuous()) {
		// then no padded pixels  
		nc = nc*nr;
		nr = 1;  // it is now a 1D array  
	}

	for (int j = 0; j<nr; j++) {
		uchar* data = src.ptr<uchar>(j);
		for (int i = 0; i<nc; i++) {
			unsigned int t = 0;
			t += *data++;
			t <<= 8;
			t += *data++;
			t <<= 8;
			t += *data++;
			imgInput.push_back(t);

		}
	}

	sal.GetSaliencyMap(imgInput, src.cols, src.rows, imgSal);

	//vector to Mat
	int index0 = 0;
	for (int h = 0; h < src.rows; h++) {
		double* p = salImage.ptr<double>(h);
		for (int w = 0; w < src.cols; w++) {
			*p++ = imgSal[index0++];
		}
	}

	normalize(salImage, salImage, 0, 1, NORM_MINMAX);
}



/* @brief ��setMouseCallback���á�
*/
static void onMouse(int event, int x, int y, int, void*)
{
	Mat dstXR, histimgXR = Mat::zeros(rectHist.Height(), rectHist.Width(), CV_8UC3);
	switch (event)
	{
	case EVENT_LBUTTONDOWN:
		origin = Point(x, y);
		for (int i = 0; i < regionsA.size(); i++) {
			if (origin.inside(regionsA[i]))
			{
				//���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�  
				srcImageA(regionsA[i]).copyTo(srcImageXR);
				resize(srcImageXR, dstXR, cv::Size(rectXR.Width(), rectXR.Height()));
				imshow("ViewXR", dstXR);
				//ֱ��ͼ
				getHistImg(srcImageXR, histimgXR);
				imshow("ViewHist", histimgXR);
				break;
			}
		}
		//origin = (0,0);
		break;
	}
}

/* @brief ���Ĵ���
*/
void coreFunction(CReidDemoDlg * const  obj) {
	Mat frameA, frameB;

	while (true) {

		//offVideoA
		if (offvideoFlagA) {
			OLvideoFalgA = false;
			captureA >> frameA;
			
			///��Ϊ��֡����������Ƶ�������
			if (frameA.empty()) {
				offvideoFlagA = false;
				continue;
			}
			resize(frameA, frameA, cv::Size(rectA.Width(), rectA.Height()));
			frameA.copyTo(srcImageA);
			// ��ͼ���ϼ����������			
			std::vector<cv::Rect> regionsA_uf;
			hog.detectMultiScale(frameA, regionsA_uf, 0, cv::Size(8, 8), cv::Size(0, 0), 1.4, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50
			
			//����foundѰ��û�б�Ƕ�׵ĳ�����
			regionsA.clear();
			for (int i = 0; i < regionsA_uf.size(); i++) {
				Rect r = regionsA_uf[i];

				int j = 0;
				for (; j < regionsA_uf.size(); j++) {
					//���ʱǶ�׵ľ��Ƴ�ѭ��
					if (j != i && (r & regionsA_uf[j]) == r)
						break;
				}
				if ( regionsA_uf.size() == j ) {
					regionsA.push_back(r);
				}
			}

			// ��ʾ
			for (int i = 0; i < regionsA.size(); i++)
			{
				regionsA[i].x += cvRound(regionsA[i].width*0.15);
				regionsA[i].width = cvRound(regionsA[i].width*0.7);
				regionsA[i].y += cvRound(regionsA[i].height*0.1);
				regionsA[i].height = cvRound(regionsA[i].height*0.8);

				cv::rectangle(frameA, regionsA[i], cv::Scalar(0, 0, 255), 2);
			}
			
			imshow("ViewA", frameA);
			waitKey(1);

		}

		//offVideoB
		if(offvideoFlagB){
			OLvideoFalgB = false;
			captureB >> frameB;

			///��Ϊ��֡����������Ƶ�������
			if (frameB.empty()) {
				offvideoFlagB = false;
				continue;
			}

			//frameB.copyTo(srcImageB);
			resize(frameB, frameB, cv::Size(rectA.Width(), rectA.Height()));
			
			// 3. �ڲ���ͼ���ϼ����������
			std::vector<cv::Rect> regionsB_uf;
			hog.detectMultiScale(frameB, regionsB_uf, 0, cv::Size(8, 8), cv::Size(0, 0), 1.4, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50
			
			//����foundѰ��û�б�Ƕ�׵ĳ�����
			for (int i = 0; i < regionsB_uf.size(); i++) {
				Rect r = regionsB_uf[i];

				int j = 0;
				for (; j < regionsB_uf.size(); j++) {
					//���ʱǶ�׵ľ��Ƴ�ѭ��
					if (j != i && (r & regionsB_uf[j]) == r)
						break;
				}
				if (regionsB_uf.size() == j) {
					regionsB.push_back(r);
				}
			}
			
			
			if (regionsB.size()) {
				srcHistB.resize(regionsB.size());
				compareResult.resize(regionsB.size());
				Mat tempRoi;
				// ��ʾ
				for (int i = 0; i < regionsB.size(); i++)
				{
					regionsB[i].x += cvRound(regionsB[i].width*0.15);
					regionsB[i].width = cvRound(regionsB[i].width*0.7);
					regionsB[i].y += cvRound(regionsB[i].height*0.1);
					regionsB[i].height = cvRound(regionsB[i].height*0.8);

					tempRoi = frameB(regionsB[i]);
					getHist(tempRoi, srcHistB[i]);
					compareResult[i] = compareHist(HistXR, srcHistB[i], 0);
					cv::rectangle(frameB, regionsB[i], cv::Scalar(0, 0, 255), 2);
				}

				std::vector<double>::iterator biggest = std::max_element(std::begin(compareResult), std::end(compareResult));
				size_t MaxIndex = std::distance(std::begin(compareResult), biggest);

				CString str;
				str.Format(_T("%.2lf"), compareResult[MaxIndex]);
				obj->SetDlgItemText(IDC_EDIT_Test, str);
				if(compareResult[MaxIndex]<0.7)
						cv::rectangle(frameB, regionsB[MaxIndex], cv::Scalar(0, 0, 255), 2);
				else
					cv::rectangle(frameB, regionsB[MaxIndex], cv::Scalar(0, 255, 0), 2);
				MaxIndex = 0;
			}
			regionsB.clear();
			
			imshow("ViewB", frameB);
			waitKey(1);
		}

		//OLVideoA
		if (OLvideoFalgA) {
			offvideoFlagA = false;
			vcapA >> frameA;

			if (frameA.empty()) {
				cout << "No frame" << endl;
				break;
			}

			// ����ͼ���ϼ����������
			hog.detectMultiScale(frameA, regionsA, 0, cv::Size(8, 8), cv::Size(32, 32), 1.25, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50

			// ��ʾ
			for (int i = 0; i < regionsA.size(); i++)
			{
				cv::rectangle(frameA, regionsA[i], cv::Scalar(0, 0, 255), 2);
			}
			//resize(frameA, frameA, cv::Size(rectA.Width(), rectA.Height()));
			imshow("ViewA", frameA);

			//if (waitKey(1));

		}

		//OLVideoB
		if (OLvideoFalgB) {
			offvideoFlagB = false;
			vcapB >> frameB;

			if (frameB.empty()) {
				cout << "No frame" << endl;
				break;
			}

			//// ����ͼ���ϼ����������			
			//hog.detectMultiScale(frameB, regionsB, 0, cv::Size(8, 8), cv::Size(32, 32), 1.05, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50

			//// ��ʾ
			//for (int i = 0; i < regionsA.size(); i++)
			//{
			//	cv::rectangle(frameB, regionsA[i], cv::Scalar(0, 0, 255), 2);
			//}
			//resize(frameA, frameA, cv::Size(rectA.Width(), rectA.Height()));
			imshow("ViewB", frameB);

			if (waitKey(1));
		}

		//obj->m_edit_test.SetWindowText(_T("wwww"));
		waitKey(1);
	}

}

/*------------------------------------------------------------------*/

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CReidDemoDlg �Ի���


CReidDemoDlg::CReidDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_REIDDEMO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CReidDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Control(pDX, IDC_EDIT_Test, m_edit_test);
	DDX_Control(pDX, IDC_EDIT_Test, m_edit_test);
}

BEGIN_MESSAGE_MAP(CReidDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_ShowVideoA, &CReidDemoDlg::OnBnClickedShowvideoa)
	ON_BN_CLICKED(IDC_BtnShotA, &CReidDemoDlg::OnBnClickedBtnshota)
	ON_BN_CLICKED(IDC_BtnVideoTestA, &CReidDemoDlg::OnBnClickedBtnvideotesta)
	ON_BN_CLICKED(IDC_BtnVideoTestB, &CReidDemoDlg::OnBnClickedBtnvideotestb)
	ON_BN_CLICKED(IDC_BtnStart, &CReidDemoDlg::OnBnClickedBtnstart)
	ON_BN_CLICKED(IDC_ShowVideoB, &CReidDemoDlg::OnBnClickedShowvideob)
	ON_BN_CLICKED(IDC_BtnShotB, &CReidDemoDlg::OnBnClickedBtnshotb)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BtnSal, &CReidDemoDlg::OnBnClickedBtnsal)
END_MESSAGE_MAP()


// CReidDemoDlg ��Ϣ�������

BOOL CReidDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	Mat temp;
	//picture control
	///PicA
	namedWindow("ViewA", WINDOW_AUTOSIZE);
	HWND hWndA = (HWND)cvGetWindowHandle("ViewA");
	HWND hParentA = ::GetParent(hWndA);
	::SetParent(hWndA, GetDlgItem(IDC_PicA)->m_hWnd);
	::ShowWindow(hParentA, SW_HIDE);
	///PicXR
	namedWindow("ViewXR", WINDOW_AUTOSIZE);
	HWND hWndXR = (HWND)cvGetWindowHandle("ViewXR");
	HWND hParentXR = ::GetParent(hWndXR);
	::SetParent(hWndXR, GetDlgItem(IDC_PicXR)->m_hWnd);
	::ShowWindow(hParentXR, SW_HIDE);
	///picHist
	namedWindow("ViewHist", WINDOW_AUTOSIZE);
	HWND hWndHist = (HWND)cvGetWindowHandle("ViewHist");
	HWND hParentHist = ::GetParent(hWndHist);
	::SetParent(hWndHist, GetDlgItem(IDC_PicHist)->m_hWnd);
	::ShowWindow(hParentHist, SW_HIDE);	
	///PicB
	namedWindow("ViewB", WINDOW_AUTOSIZE);
	HWND hWndB = (HWND)cvGetWindowHandle("ViewB");
	HWND hParentB = ::GetParent(hWndB);
	::SetParent(hWndB, GetDlgItem(IDC_PicB)->m_hWnd);
	::ShowWindow(hParentB, SW_HIDE);
	///resize
	GetDlgItem(IDC_PicA)->GetClientRect(&rectA);
	GetDlgItem(IDC_PicB)->GetClientRect(&rectB);
	GetDlgItem(IDC_PicXR)->GetClientRect(&rectXR);
	GetDlgItem(IDC_PicHist)->GetClientRect(&rectHist);
	//��ʼ��ʾ
	///PicA
	temp = imread("./res/cameraA.jpg");
	resize(temp, temp, cv::Size(rectA.Width(), rectA.Height()));
	imshow("ViewA", temp);
	///PicB
	temp = imread("./res/cameraB.jpg");
	resize(temp, temp, cv::Size(rectB.Width(), rectB.Height()));
	imshow("ViewB", temp);
	///PicXR
	temp = imread("./res/xrinit.jpg");
	resize(temp, temp, cv::Size(rectXR.Width(), rectXR.Height()));
	imshow("ViewXR", temp);
	///picHist
	temp = imread("./res/hist.jpg");
	resize(temp, temp, cv::Size(rectHist.Width(), rectHist.Height()));
	imshow("ViewHist", temp);
	//Hog
	///�����Ѿ�ѵ���õ����˼�������������getDaimlerPeopleDetector()
	hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector()); 

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CReidDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ��������Ƹ�ͼ�ꡣ
// ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó����⽫�ɿ���Զ���ɡ�


void CReidDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//CDialogEx::OnPaint();
		CRect   rect;
		CPaintDC   dc(this);
		GetClientRect(rect);
		dc.FillSolidRect(rect, RGB(255, 255, 255));
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CReidDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

/*-----------------------------�û��ؼ�-----------------------------*/

// ����ť�� ��ȡʵʱ����A
void CReidDemoDlg::OnBnClickedShowvideoa()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OLvideoFalgA = true;
	// open the video stream and make sure it's opened
	if (!vcapA.open(videoStreamAddressA)) {
		AfxMessageBox(_T("����Ƶ��A��������IP��ַ��"));
	}
}

// ����ť�� ��ȡʵʱ����B
void CReidDemoDlg::OnBnClickedShowvideob()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OLvideoFalgB = true;
	// open the video stream and make sure it's opened
	if (!vcapB.open(videoStreamAddressB)) {
		AfxMessageBox(_T("����Ƶ��B��������IP��ַ��"));
	}

}

// ����ť�� ��ͼA
void CReidDemoDlg::OnBnClickedBtnshota()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	offvideoFlagA = false;

}

// ����ť�� ��ͼB
void CReidDemoDlg::OnBnClickedBtnshotb()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}

// ����ť�� ������ƵA
void CReidDemoDlg::OnBnClickedBtnvideotesta()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	setMouseCallback("ViewA", onMouse, 0);
	
	//captureA.open("./res/viptrain.avi");
	captureA.open(offvideoAddressA);
	
	offvideoFlagA = true;
}

// ����ť�� ������ƵB
void CReidDemoDlg::OnBnClickedBtnvideotestb()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	// �õ����˵���ɫֱ��ͼ
	if (srcImageXR.empty()) {
		AfxMessageBox(_T("��ѡ�����ˣ�"));
		return;
	}
	getHist(srcImageXR, HistXR);

	captureB.open(offvideoAddressB);

	offvideoFlagB = true;
}

// ����ť�� ��ʼ
void CReidDemoDlg::OnBnClickedBtnstart()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	setMouseCallback("ViewA", onMouse, 0);

	coreFunction(this);
}


void CReidDemoDlg::OnBnClickedBtnsal()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	Mat salImage(srcImageXR.size(), CV_64FC1);

	Sal2010(srcImageXR, salImage);

	resize(salImage, salImage, cv::Size(rectXR.Width(), rectXR.Height()));
	imshow("ViewXR", salImage);
}
