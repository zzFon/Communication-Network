// StopWaitCommDlg.h : ͷ�ļ�
//

#pragma once
#include "afx.h"
#include "afxwin.h"
#include "setupcomm.h"


// CStopWaitCommDlg �Ի���
class CStopWaitCommDlg : public CDialog
{
// ����
public:
	CStopWaitCommDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_STOPWAITCOMM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CListBox m_ListLog;
	afx_msg void OnTimer(UINT nIDEvent);
	CButton m_Send;
	afx_msg void OnClose();
	// ���ô��пڶԻ���
	CSetupComm m_SetupComm;
	afx_msg void OnBnClickedSetup();
	CButton m_Setup;
};
