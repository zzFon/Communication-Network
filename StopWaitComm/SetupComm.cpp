// SetupComm.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "StopWaitComm.h"
#include "SetupComm.h"
#include ".\setupcomm.h"

extern CString CommPort;
extern UINT	Speed,PacketLen,DelayTime;
extern HANDLE m_hCommPort;		//����򿪵Ĵ��п��豸���


// CSetupComm �Ի���

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

	DDX_CBString(pDX,IDC_COMMPORT,m_sCommPort);		//������ݽ���
	DDX_Text(pDX,IDC_SPEED,m_iSpeed);
	DDX_Text(pDX,IDC_PACKETLEN,m_iPacketLen);
	DDX_Text(pDX,IDC_DELAYTIME,m_iDelayTime);
}


BEGIN_MESSAGE_MAP(CSetupComm, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CSetupComm ��Ϣ�������

BOOL CSetupComm::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	m_CommPort.AddString("COM1");	//��ʼ�����п�ѡ���
	m_CommPort.AddString("COM2");
	if (CommPort=="COM1")
	    m_CommPort.SetCurSel(0);
	else
		m_CommPort.SetCurSel(1);
	if (m_hCommPort!=INVALID_HANDLE_VALUE)
		m_CommPort.EnableWindow(false);
	
	m_Speed.AddString("300");	//��ʼ��������ѡ���
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

	m_PacketLen.AddString("100");	//��ʼ�����ݰ�����ѡ���
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

	m_DelayTime.AddString("0");		//��ʼ����·�ӳ�ѡ���
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
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CSetupComm::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	OnOK();
}
