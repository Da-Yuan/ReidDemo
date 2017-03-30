
// ReidDemoDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "ReidDemo.h"
#include "ReidDemoDlg.h"
#include "afxdialogex.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//����
static Point origin;
static Mat srcImageA, srcImageB;
static Mat dstImageA, dstImageB;
static std::vector<cv::Rect> regionsA;
static std::vector<cv::Rect> regionsB;
static Mat srcImageXR;
static CRect rectXR;//����PicXR��ͼ���resize
static MatND HistXR;
static std::vector<cv::MatND> srcHistB;
/// 1. ����HOG����
cv::HOGDescriptor hog
(
	cv::Size(64, 128), cv::Size(16, 16),
	cv::Size(8, 8), cv::Size(8, 8), 9, 1, -1,
	cv::HOGDescriptor::L2Hys, 0.2, true, cv::HOGDescriptor::DEFAULT_NLEVELS
);

//����
void getHist(const Mat srcImage, MatND &srcHist) {
	Mat srcHsvImage;

	cvtColor(srcImage, srcHsvImage, CV_RGB2HSV);

	//ֱ��ͼ����
	int hbins = 30, sbins = 16;
	int histSize[] = { hbins,sbins };
	int channels[] = { 0,1 };
	float hRange[] = { 0,180 };
	float sRange[] = { 0,255 };
	const float* ranges[] = { hRange,sRange };

	calcHist(&srcHsvImage, 1, channels, Mat(), srcHist, 2, histSize, ranges, true, false);
	normalize(srcHist, srcHist, 0, 1, NORM_MINMAX, -1, Mat());
}


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
	ON_BN_CLICKED(IDC_Test, &CReidDemoDlg::OnBnClickedTest)
	ON_BN_CLICKED(IDC_DetectA, &CReidDemoDlg::OnBnClickedDetecta)
	ON_WM_LBUTTONDOWN()
	ON_WM_NCPAINT()
	ON_BN_CLICKED(IDC_BtnShotA, &CReidDemoDlg::OnBnClickedBtnshota)
	ON_BN_CLICKED(IDC_Test2, &CReidDemoDlg::OnBnClickedTest2)
	ON_BN_CLICKED(IDC_DetectB, &CReidDemoDlg::OnBnClickedDetectb)
	ON_BN_CLICKED(IDC_BtnVideoTestA, &CReidDemoDlg::OnBnClickedBtnvideotesta)
	ON_BN_CLICKED(IDC_BtnVideoTestB, &CReidDemoDlg::OnBnClickedBtnvideotestb)
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
	///PicB
	namedWindow("ViewB", WINDOW_AUTOSIZE);
	HWND hWndB = (HWND)cvGetWindowHandle("ViewB");
	HWND hParentB = ::GetParent(hWndB);
	::SetParent(hWndB, GetDlgItem(IDC_PicB)->m_hWnd);
	::ShowWindow(hParentB, SW_HIDE);

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



void CReidDemoDlg::OnBnClickedShowvideoa()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	cv::VideoCapture vcap;

	const std::string videoStreamAddress = "http://192.168.1.120:81/videostream.cgi?user=admin&pwd=888888&.mjpg";
	/*Address e.g. "http://IP:port/videostream.cgi?user=admin&pwd=******&.mjpg" */

	//open the video stream and make sure it's opened
	if (!vcap.open(videoStreamAddress)) {
		std::cout << "Error opening video stream or file" << std::endl;		
	}

	// 2. ����SVM������
	hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector()); // getDaimlerPeopleDetector() �����Ѿ�ѵ���õ����˼�������

	cv::Mat image;

	while (1) {
		vcap >> image;

		if (image.empty()) {
			cout << "No frame" << endl;
			break;
		}

		// 3. �ڲ���ͼ���ϼ����������			
		hog.detectMultiScale(image, regionsA, 0, cv::Size(8, 8), cv::Size(32, 32), 1.05, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50

		// ��ʾ
		for (int i = 0; i < regionsA.size(); i++)
		{
			cv::rectangle(image, regionsA[i], cv::Scalar(0, 0, 255), 2);
		}
		//resize(image, image, Size(), 2, 2);
		cv::imshow("ViewA", image);

		if (waitKey(1));
	}

	cv::waitKey(0);
}


void CReidDemoDlg::OnBnClickedTest()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//����ͼƬ
	srcImageA = imread("./res/01/0102.jpg");

	Mat imagedst;
	//���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�  
	CRect rect;
	GetDlgItem(IDC_PicA)->GetClientRect(&rect);
	//Rect dst(rect.left, rect.top, rect.right, rect.bottom);
	//resize(srcImage, dstImage, cv::Size(rect.Width(), rect.Height()));
	resize(srcImageA, imagedst, cv::Size(rect.Width(), rect.Height()));
	imagedst.copyTo(dstImageA);

	imshow("ViewA", imagedst);
}



// User draws box around object to track. This triggers CAMShift to start tracking
static void onMouse(int event, int x, int y, int, void*)
{
	//Point point;
	//Rect rect;
	//point.inside(rect);

	CString str;
	Mat dstXR;
	Mat srcImageARoi;
	switch (event)
	{
	case EVENT_LBUTTONDOWN:
		origin = Point(x, y);
		//str.Format("%d", origin.x);
		//AfxMessageBox("��ǰx����Ϊ��"+str);
		for (int i = 0; i < regionsA.size(); i++) {
			if (origin.inside(regionsA[i]))
			{
				//str.Format("%d", i);
				//AfxMessageBox("ѡ�������˱��Ϊ��" + str);
				//srcImage = imread("./res/small.jpg");
				
				//���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�  
				srcImageARoi = dstImageA(regionsA[i]);
				resize(srcImageARoi, dstXR, cv::Size(rectXR.Width(), rectXR.Height()));
				imshow("ViewXR", dstXR);
				dstXR.copyTo(srcImageXR);
				break;
			}
			origin.inside(regionsA[i]);
		}
		break;
	case EVENT_LBUTTONUP:
		break;
	}
}

void CReidDemoDlg::OnBnClickedDetecta()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	Mat detectImage;
	dstImageA.copyTo(detectImage);
	
	// 2. ����SVM������
	hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector()); // getDaimlerPeopleDetector() �����Ѿ�ѵ���õ����˼�������

	// 3. �ڲ���ͼ���ϼ����������	
	hog.detectMultiScale(detectImage, regionsA, 0, cv::Size(8, 8), cv::Size(32, 32), 1.45, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50


	// ��ʾ
	for (size_t i = 0; i < regionsA.size(); i++)
	{
		regionsA[i].x += cvRound(regionsA[i].width*0.1);
		regionsA[i].width = cvRound(regionsA[i].width*0.8);
		regionsA[i].y += cvRound(regionsA[i].height*0.07);
		regionsA[i].height = cvRound(regionsA[i].height*0.8);

		cv::rectangle(detectImage, regionsA[i], cv::Scalar(0, 0, 255), 2);
	}

	imshow("ViewA", detectImage);

	setMouseCallback("ViewA", onMouse, 0);
	GetDlgItem(IDC_PicXR)->GetClientRect(&rectXR);

}

//void CReidDemoDlg::OnLButtonDown(UINT nFlags, CPoint point)
//{
//	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
//	CDialogEx::OnLButtonDown(nFlags, point);
//}


//void CReidDemoDlg::OnNcPaint()
//{
//	// TODO: �ڴ˴������Ϣ����������
//	// ��Ϊ��ͼ��Ϣ���� CDialogEx::OnNcPaint()
//
//	
//
//}


void CReidDemoDlg::OnBnClickedBtnshota()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString str;
	str.Format("%d", testnum);
	SetDlgItemText(IDC_EDIT_Test, str);
}


void CReidDemoDlg::OnBnClickedTest2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//����ͼƬ
	srcImageB = imread("./res/01/0104.jpg");

	Mat imagedst;
	//���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�  
	CRect rect;
	GetDlgItem(IDC_PicB)->GetClientRect(&rect);
	//Rect dst(rect.left, rect.top, rect.right, rect.bottom);
	//resize(srcImage, dstImage, cv::Size(rect.Width(), rect.Height()));
	resize(srcImageB, imagedst, cv::Size(rect.Width(), rect.Height()));
	imagedst.copyTo(dstImageB);

	imshow("ViewB", imagedst);
}


void CReidDemoDlg::OnBnClickedDetectb()
{
	getHist(srcImageXR, HistXR);

	// TODO: �ڴ���ӿؼ�֪ͨ����������
	Mat detectImage;
	dstImageB.copyTo(detectImage);

	// 2. ����SVM������
	hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector()); // getDaimlerPeopleDetector() �����Ѿ�ѵ���õ����˼�������

	// 3. �ڲ���ͼ���ϼ����������	
	hog.detectMultiScale(detectImage, regionsB, 0, cv::Size(8, 8), cv::Size(32, 32), 1.45, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50

	srcHistB.resize(regionsB.size());
	Mat tempRoi;
	// ��ʾ
	for (int i = 0; i < regionsB.size(); i++)
	{
		regionsB[i].x += cvRound(regionsB[i].width*0.1);
		regionsB[i].width = cvRound(regionsB[i].width*0.8);
		regionsB[i].y += cvRound(regionsB[i].height*0.07);
		regionsB[i].height = cvRound(regionsB[i].height*0.8);

		tempRoi = detectImage(regionsB[i]);
		getHist(tempRoi, srcHistB[i]);

		//cv::rectangle(detectImage, r, cv::Scalar(0, 0, 255), 2);
	}
	vector<double> compareResult;
	compareResult.resize(regionsB.size());
	CString str;
	for (int i = 0; i < regionsB.size(); i++) {
		compareResult[i] = compareHist(HistXR, srcHistB[i], 0);
		//str.Format("%.2f", compareResult[i]);
		//AfxMessageBox("ѡ�������˱��Ϊ��" + str);
	}
	std::vector<double>::iterator biggest = std::max_element(std::begin(compareResult), std::end(compareResult));
	int MaxIndex = std::distance(std::begin(compareResult), biggest);

	str.Format("%.2f", compareResult[MaxIndex]);
	SetDlgItemText(IDC_EDIT_Test, str);

	cv::rectangle(detectImage, regionsB[MaxIndex], cv::Scalar(0, 0, 255), 2);

	imshow("ViewB", detectImage);
}


void CReidDemoDlg::OnBnClickedBtnvideotesta()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	VideoCapture capture;
	Mat frame;

	setMouseCallback("ViewA", onMouse, 0);
	GetDlgItem(IDC_PicXR)->GetClientRect(&rectXR);

	// 2. ����SVM������
	hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector()); // getDaimlerPeopleDetector() �����Ѿ�ѵ���õ����˼�������


	capture.open("./res/viptrain.avi");
	while (true)
	{		
		capture >> frame;
		frame.copyTo(dstImageA);
		if (frame.empty()) break;

		// 3. �ڲ���ͼ���ϼ����������			
		hog.detectMultiScale(frame, regionsA, 0, cv::Size(8, 8), cv::Size(32, 32), 1.15, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50
		// ��ʾ
		for (int i = 0; i < regionsA.size(); i++)
		{
			regionsA[i].x += cvRound(regionsA[i].width*0.1);
			regionsA[i].width = cvRound(regionsA[i].width*0.8);
			regionsA[i].y += cvRound(regionsA[i].height*0.07);
			regionsA[i].height = cvRound(regionsA[i].height*0.8);

			cv::rectangle(frame, regionsA[i], cv::Scalar(0, 0, 255), 2);
		}

		imshow("ViewA", frame);
		waitKey(1);
	}
	capture.release();
}


void CReidDemoDlg::OnBnClickedBtnvideotestb()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	// �õ����˵���ɫֱ��ͼ
	getHist(srcImageXR, HistXR);

	VideoCapture capture;
	Mat frame;
	Mat detectImage;
	vector<double> compareResult;


	// 2. ����SVM������
	hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector()); // getDaimlerPeopleDetector() �����Ѿ�ѵ���õ����˼�������
	
	capture.open("./res/viptrain.avi");
	while (true)
	{
		capture >> frame;

		///��Ϊ��֡����������Ƶ�������
		if (frame.empty()) break;

		frame.copyTo(detectImage);

		// 3. �ڲ���ͼ���ϼ����������	
		hog.detectMultiScale(detectImage, regionsB, 0, cv::Size(8, 8), cv::Size(32, 32), 1.15, 1, 0);//detectMultiScale:����������scaleͨ��ȡֵ1.01-1.50
		if(regionsB.size()){
			srcHistB.resize(regionsB.size());
			compareResult.resize(regionsB.size());
			Mat tempRoi;
			// ��ʾ
			for (int i = 0; i < regionsB.size(); i++)
			{
				regionsB[i].x += cvRound(regionsB[i].width*0.1);
				regionsB[i].width = cvRound(regionsB[i].width*0.8);
				regionsB[i].y += cvRound(regionsB[i].height*0.07);
				regionsB[i].height = cvRound(regionsB[i].height*0.8);

				tempRoi = detectImage(regionsB[i]);
				getHist(tempRoi, srcHistB[i]);
				compareResult[i] = compareHist(HistXR, srcHistB[i], 0);
				//cv::rectangle(detectImage, r, cv::Scalar(0, 0, 255), 2);
			}
			
			std::vector<double>::iterator biggest = std::max_element(std::begin(compareResult), std::end(compareResult));
			int MaxIndex = std::distance(std::begin(compareResult), biggest);

			cv::rectangle(detectImage, regionsB[MaxIndex], cv::Scalar(0, 0, 255), 2);
		}
		imshow("ViewB", detectImage);

		waitKey(1);
	}
	capture.release();
}
