// SetupComm.cpp : 实现文件
//

#include "stdafx.h"
#include "StopWaitComm.h"
#include "SetupComm.h"
#include ".\setupcomm.h"

extern CString CommPort;
extern UINT	Speed,PacketLen,DelayTime;
extern HANDLE m_hCommPort;		//保存打开的串行口设备句柄


// CSetupComm 对话框

IMPLEMENT_DYNAMIC(CSetupComm, CDialog)
CSetupComm::CSetupComm(CWnd* pParent /*=NULL*/)
	: CDialog(CSetupComm::IDD, pParent)
	, m_iSpeed(0)
	, m_iPacketLen(0)
	, m_iDelayTime(0)
	, m_sCommPort(_T(""))
{
}

CSetupComm::~CSetupComm()
{
}

void CSetupComm::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMMPORT, m_CommPort);
	DDX_Control(pDX, IDC_SPEED, m_Speed);
	DDX_Control(pDX, IDC_PACKETLEN, m_PacketLen);
	DDX_Control(pDX, IDC_DELAYTIME, m_DelayTime);

	DDX_CBString(pDX,IDC_COMMPORT,m_sCommPort);		//添加数据交换
	DDX_Text(pDX,IDC_SPEED,m_iSpeed);
	DDX_Text(pDX,IDC_PACKETLEN,m_iPacketLen);
	DDX_Text(pDX,IDC_DELAYTIME,m_iDelayTime);
}


BEGIN_MESSAGE_MAP(CSetupComm, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CSetupComm 消息处理程序

BOOL CSetupComm::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_CommPort.AddString("COM1");	//初始化串行口选择框
	m_CommPort.AddString("COM2");
	if (CommPort=="COM1")
	    m_CommPort.SetCurSel(0);
	else
		m_CommPort.SetCurSel(1);
	if (m_hCommPort!=INVALID_HANDLE_VALUE)
		m_CommPort.EnableWindow(false);
	
	m_Speed.AddString("300");	//初始化波特率选择框
	m_Speed.AddString("9600");
	m_Speed.AddString("56000");
	m_Speed.AddString("115200");
	switch (Speed)
	{
	case 300:
        m_Speed.SetCurSel(0);break;
	case 9600:
        m_Speed.SetCurSel(1);break;
	case 56000:
        m_Speed.SetCurSel(2);break;
	case 115200:
        m_Speed.SetCurSel(3);break;
	}

	m_PacketLen.AddString("100");	//初始化数据包长度选择框
	m_PacketLen.AddString("1500");
	m_PacketLen.AddString("3000");
	switch (PacketLen)
	{
	case 100:
        m_PacketLen.SetCurSel(0);break;
	case 1500:
		m_PacketLen.SetCurSel(1);break;
	case 3000:
		m_PacketLen.SetCurSel(2);break;
	}

	m_DelayTime.AddString("0");		//初始化线路延迟选择框
	m_DelayTime.AddString("100");
	m_DelayTime.AddString("1000");
	switch (DelayTime)
	{
	case 0:
        m_DelayTime.SetCurSel(0);break;
	case 100:
		m_DelayTime.SetCurSel(1);break;
	case 1000:
		m_DelayTime.SetCurSel(2);break;
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CSetupComm::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	OnOK();
}
