#pragma once
#include "afxwin.h"


// CSetupComm 对话框

class CSetupComm : public CDialog
{
	DECLARE_DYNAMIC(CSetupComm)

public:
	CSetupComm(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSetupComm();

// 对话框数据
	enum { IDD = IDD_SETUPCOMM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CComboBox m_CommPort;
	CComboBox m_Speed;
	CComboBox m_PacketLen;
	CComboBox m_DelayTime;
	unsigned int m_iSpeed;
	afx_msg void OnBnClickedOk();
	unsigned int m_iPacketLen;
	unsigned int m_iDelayTime;
	CString m_sCommPort;
};
