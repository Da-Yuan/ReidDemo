
// ReidDemoDlg.h : 头文件
//

#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "gdiplustypes.h"
#include "afxwin.h"
#include "D:\Program Files (x86)\opencv\build\include\opencv2\core\mat.hpp"

using namespace cv;


// CReidDemoDlg 对话框
class CReidDemoDlg : public CDialogEx
{
// 构造
public:
	CReidDemoDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REIDDEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedShowvideoa();
	afx_msg void OnBnClickedTest();
	afx_msg void OnBnClickedDetecta();		
	CEdit m_edit_test;
	afx_msg void OnBnClickedBtnshota();
	afx_msg void OnBnClickedTest2();
	afx_msg void OnBnClickedDetectb();
	afx_msg void OnBnClickedBtnvideotesta();
	afx_msg void OnBnClickedBtnvideotestb();

private:

};
