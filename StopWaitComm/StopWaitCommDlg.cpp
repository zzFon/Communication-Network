// StopWaitCommDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "StopWaitComm.h"
#include "StopWaitCommDlg.h"
#include ".\stopwaitcommdlg.h"
#include "afxmt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CStopWaitCommDlg �Ի���
#define ENQ	0x05
#define	ACK	0x06
#define	STX	0x02
#define	ETX	0x03
#define	EOT	0x04

#define	IDLESTATE	0
#define	SENDSTATE	1
#define	RECVSTATE	2

#define BUFFER_SIZE	3100
#define	TIMEOUT		5000

CFile		SendFile,RecvFile;	//���ͺͽ����ļ�

HANDLE		m_hCommPort;		//����򿪵Ĵ��п��豸���
char		RecvBuf[BUFFER_SIZE], SendBuf[BUFFER_SIZE];
UINT		SendLen,RecvPTR;

UINT		CommState;	//����ͨ��״̬��IdleState��SendState��RecvState
BYTE		CRC;		//����У���
BYTE		Sequence;	//�ڵ�ͣЭ���н�ʹ��0��1
bool		ACKFlag;	//���յ�ACK��true
bool		LastPacket;	//�Ƿ����һ��
UINT		STXFlag;	//0��û�յ�STX��1���յ�STX��δ�յ����кţ�2���յ�STX�����յ����к�
UINT_PTR	m_nTimer;	//��ʱ��
UINT		SendCount;	//�ط�������
time_t		TimeStart,TimeFinish;
ULONG		TextLen;	//�����ļ����ܳ���

CString		CommPort;//��Ҫ�򿪵Ĵ��п�
UINT		PacketLen,Speed,DelayTime;//���ݰ���С�����пڵ����ʣ�Ϊģ�ⳤ��·�����ӵ���·�ӳ�

CStopWaitCommDlg* pDlg;

UINT ReadChar(PVOID hWnd);	//��ȡ�ַ��߳�
void FormPacket();	//���
void SendMsg(char *DisplayStr); //���пڷ�����Ϣ
bool SetupCommPort();	//���ô��п�



CStopWaitCommDlg::CStopWaitCommDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStopWaitCommDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CStopWaitCommDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOG, m_ListLog);
	DDX_Control(pDX, IDOK, m_Send);
	DDX_Control(pDX, IDC_SETUP, m_Setup);
}

BEGIN_MESSAGE_MAP(CStopWaitCommDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_SETUP, OnBnClickedSetup)
END_MESSAGE_MAP()


// CStopWaitCommDlg ��Ϣ�������

BOOL CStopWaitCommDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	CStopWaitCommApp* pApp=(CStopWaitCommApp*)AfxGetApp();
	pDlg=(CStopWaitCommDlg*)pApp->m_pMainWnd;

	m_Send.EnableWindow(false);

	m_hCommPort=INVALID_HANDLE_VALUE;
	CommPort="COM1";Speed=9600;PacketLen=1500;DelayTime=0;

	if (m_SetupComm.DoModal()==IDOK)
	{
        CommPort=m_SetupComm.m_sCommPort;
		Speed=m_SetupComm.m_iSpeed;
		PacketLen=m_SetupComm.m_iPacketLen;
		DelayTime=m_SetupComm.m_iDelayTime;
		if (SetupCommPort())
		{
			m_Send.EnableWindow(true);

			char str[100];
			sprintf(str,"StopWaitComm ���п�=%s������=%i��������=%i����·�ӳ�+%i",CommPort,Speed,PacketLen,DelayTime);
			pDlg->SetWindowText(str);
		}
		else
			m_Send.EnableWindow(false);
	}

	return TRUE;  // ���������˿ؼ��Ľ��㣬���򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CStopWaitCommDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
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
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
HCURSOR CStopWaitCommDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CStopWaitCommDlg::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	//��ȡ��Ҫ���͵��ļ���
	CString FileName;
	CFileDialog GetFileName(TRUE,NULL,NULL,OFN_HIDEREADONLY,"�ı��ļ�(*.txt)|*.txt||",NULL,0);
	if (GetFileName.DoModal()!=IDOK) return;
	FileName=GetFileName.GetPathName();
	if ( !SendFile.Open(FileName,CFile::modeRead|CFile::typeBinary,NULL))	//���ļ�
	{
		MessageBox("�����ļ��򿪴���");
		return;
	}
	//���뷢��״̬
	m_Send.EnableWindow(false);
	m_Setup.EnableWindow(false);

	m_ListLog.InsertString(-1,"------���뷢��״̬------");
	time(&TimeStart);	//��ʼ��ʱ
	CommState=SENDSTATE;ACKFlag=false;Sequence=0;LastPacket=false;SendCount=0;TextLen=0;
	SendBuf[0]=ENQ;SendLen=1;	//֪ͨ���շ���ʼ����
	SendMsg("���ͣ�ENQ");
	m_nTimer=SetTimer(1,TIMEOUT,NULL);	//���ö�ʱ��
}

UINT ReadChar(PVOID hWnd)
{
	CEvent		RecvEvent(0,TRUE,0,0);//���ͺͽ����¼�
	OVERLAPPED	RecvOV;
	char		DisplayStr[256],tempStr[50];
	ULONG		ReadLen;
	double		DiffTime,Ratio;

	memset(&RecvOV,0,sizeof(RecvOV));	//��ʼ��Overlapped����
	RecvOV.hEvent=RecvEvent;

	while (true)		//ֻҪ�߳����У��ͼ��Ӷ˿��Ƿ��յ�����
	{
		if (CommState==RECVSTATE)	m_nTimer=pDlg->SetTimer(1,3*TIMEOUT,NULL);	//���ö�ʱ��

		ReadFile(m_hCommPort,&RecvBuf[RecvPTR],1,&ReadLen,&RecvOV); //���ַ�
		GetOverlappedResult(m_hCommPort,&RecvOV,&ReadLen,TRUE);		//�ȴ���״̬���
		RecvEvent.ResetEvent();		//��λ�¼�����

		if (ReadLen>0)
		{
			switch (CommState)
			{
			case IDLESTATE:
				if (RecvBuf[RecvPTR]==ENQ)	//�յ�ENQ���������״̬
				{
					pDlg->m_ListLog.InsertString(-1,"------�������״̬------");
					pDlg->m_ListLog.InsertString(-1,"���գ�ENQ");
					if ( !RecvFile.Open("receive.txt",CFile::modeCreate|CFile::modeWrite|CFile::typeBinary,NULL))	//���ļ�
					{
                            AfxMessageBox("�����ļ��򿪴���");
							pDlg->m_ListLog.InsertString(-1,"------���ؿ���״̬------");
							break;
					}
					pDlg->m_Send.EnableWindow(false);
					pDlg->m_Setup.EnableWindow(false);
					CommState=RECVSTATE;Sequence=0;RecvPTR=0;STXFlag=0;TextLen=0;	//�������״̬
					time(&TimeStart);
					SendBuf[0]=ACK; SendBuf[1]=Sequence; SendLen=2;	//����ȷ����Ϣ
					SendMsg("���ͣ�ACK 0");
				}
				break;
			case SENDSTATE:
				if (ACKFlag==true)
				{
					if (RecvBuf[RecvPTR]==Sequence)
					{
						sprintf(DisplayStr,"���գ�ACK %i",Sequence);pDlg->m_ListLog.InsertString(-1,DisplayStr);
						SendCount=0;
						pDlg->KillTimer(m_nTimer);
						if (LastPacket==true)	//�Ƿ����һ��
						{
							CommState=IDLESTATE;
							SendFile.Close();	//���һ�����ر��ļ�

							SendBuf[0]=EOT;SendBuf[1]=EOT;SendLen=2;	//֪ͨ���շ���ʼ����
							SendMsg("���ͣ�EOT");
							pDlg->m_ListLog.InsertString(-1,"------���ؿ���״̬------");
						
							time(&TimeFinish);	//��ʾͳ����Ϣ
							DiffTime=difftime(TimeFinish,TimeStart);
							if ((ULONG (DiffTime))==0) sprintf(tempStr,"xxxxxx");	//�ó���ֻ�ܾ�ȷ����
							else
							{
								Ratio=(((TextLen*9.0)/Speed)/DiffTime)*100;
								sprintf(tempStr,"%f%%",Ratio);
							}
							pDlg->m_ListLog.InsertString(-1,"ͳ����Ϣ��ע�Ȿ����Ĵ���ʱ��ֻ�ܾ�ȷ���룩��");
							sprintf(DisplayStr,"�������ʣ�%i�ֽ�/�룻���ݰ�����%i�ֽڣ���·�ӳ٣�+%i΢�룻",Speed,PacketLen,DelayTime);
							pDlg->m_ListLog.InsertString(-1,DisplayStr);
							sprintf(DisplayStr,"���ݳ��ȣ�%li�ֽڣ������ʱ��%li�룻����Ч�ʣ�%s��",TextLen,(ULONG) DiffTime,tempStr);
							pDlg->m_ListLog.InsertString(-1,DisplayStr);
							pDlg->m_Send.EnableWindow(true);
							pDlg->m_Setup.EnableWindow(true);
						}
						else
						{
							FormPacket();
							SendMsg("���ͣ���Ϣ......");
							Sequence = (Sequence==0) ? 1:0;	//�任Sequence.
							m_nTimer=pDlg->SetTimer(1,TIMEOUT,0);	//���ö�ʱ��
						}
					}
					ACKFlag=false;
				}
				else
					if (RecvBuf[RecvPTR]==ACK) ACKFlag=true;
				break;
			case RECVSTATE:
				switch (STXFlag)
				{
				case 0:
					switch (RecvBuf[RecvPTR])
					{
					case EOT:
						CommState=IDLESTATE;RecvPTR=0;	//�������,���ӶϿ�
						pDlg->m_ListLog.InsertString(-1,"���գ����");
						RecvFile.Close ();
						pDlg->m_ListLog.InsertString(-1,"------���ؿ���״̬------");
						pDlg->KillTimer(m_nTimer);

						time(&TimeFinish);	//��ʾͳ����Ϣ
						DiffTime=difftime(TimeFinish,TimeStart);
						if ((ULONG (DiffTime))==0) sprintf(tempStr,"xxxxxx");	//�ó���ֻ�ܾ�ȷ����
						else
						{
							Ratio=(((TextLen*9.0)/Speed)/DiffTime)*100;
							sprintf(tempStr,"%f%%",Ratio);
						}
						pDlg->m_ListLog.InsertString(-1,"ͳ����Ϣ��ע�Ȿ����Ĵ���ʱ��ֻ�ܾ�ȷ���룩��");
						sprintf(DisplayStr,"�������ʣ�%i�ֽ�/�룻���ݰ�����%i�ֽڣ���·�ӳ٣�+%i΢�룻",Speed,PacketLen,DelayTime);
						pDlg->m_ListLog.InsertString(-1,DisplayStr);
						sprintf(DisplayStr,"���ݳ��ȣ�%li�ֽڣ������ʱ��%li�룻����Ч�ʣ�%s��",TextLen,(ULONG) DiffTime,tempStr);
						pDlg->m_ListLog.InsertString(-1,DisplayStr);
						pDlg->m_Send.EnableWindow(true);
						pDlg->m_Setup.EnableWindow(true);
						break;
					case ENQ:
						pDlg->m_ListLog.InsertString(-1,"���գ�ENQ");
						SendBuf[0]=ACK; SendBuf[1]=Sequence; SendLen=2;	//����ȷ����Ϣ
						sprintf(DisplayStr,"���ͣ�ACK %i",Sequence);
						SendMsg(DisplayStr);
						break;
					case STX:
						STXFlag=1;
						break;
					}
					break;
				case 1:
					if (RecvBuf[RecvPTR]==Sequence)
					{
						STXFlag=2;
						CRC=0;
					}
					else STXFlag=0;
					break;
				case 2:
					switch (RecvBuf[RecvPTR])
					{
					case ETX:
						if (CRC==0)
						{
							pDlg->m_ListLog.InsertString(-1,"���գ���Ϣ......");
							RecvFile.Write(RecvBuf,RecvPTR-1);	//��������Ϣ�����ļ�
							TextLen=TextLen+RecvPTR-1;
							Sequence=(Sequence==0)?1:0;
							SendBuf[0]=ACK; SendBuf[1]=Sequence; SendLen=2;
							sprintf(DisplayStr,"���ͣ�ACK %i",Sequence);
							SendMsg(DisplayStr);
						}
						RecvPTR=0;STXFlag=0;	//��ʼ�µ�һ��
						break;
					default:
						if (RecvPTR < BUFFER_SIZE)	CRC=CRC^RecvBuf[RecvPTR++];
						else {RecvPTR=0;STXFlag=0;}
						break;
					}
					break;
				}
				break;
			} 
		}
	}
	return 0;
}
void FormPacket()	//���
{
	int	len,i;	

	SendBuf[0]=STX;SendBuf[1]=Sequence;
	len=SendFile.Read(&SendBuf[2],PacketLen);	//��ȡ����
	
	CRC=0;	//����У���
	for (i=0;i<len;i++)
		CRC=CRC^SendBuf[i+2];
	
	SendBuf[2+len]=CRC;
	SendBuf[2+len+1]=ETX;

	SendLen=2+1+len+1;
	LastPacket=(len==PacketLen)?false:true;
	TextLen=TextLen+len;
}
void SendMsg(char *DisplayStr)	//���пڷ�����Ϣ
{
	CEvent		SendEvent(0,TRUE,0,0);	//���ͺͽ����¼�
	OVERLAPPED	SendOV;
	ULONG		SentLen;

	memset(&SendOV,0,sizeof(SendOV));	//��ʼ��Overlapped����
	SendOV.hEvent=SendEvent;

	Sleep(DelayTime);		//ģ�ⳤ��·�ӳ�

	WriteFile(m_hCommPort,SendBuf,SendLen,&SentLen,&SendOV);
	GetOverlappedResult(m_hCommPort,&SendOV,&SentLen,TRUE);		//�ȴ��������
	SendEvent.ResetEvent();
	pDlg->m_ListLog.InsertString(-1,DisplayStr);	//��ʾ��Ϣ
}

void CStopWaitCommDlg::OnTimer(UINT nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	switch (CommState)
	{
	case SENDSTATE:
		if (SendCount<3)
		{
			SendCount++;
			KillTimer(m_nTimer);SendMsg("��ʱ�ش�......");m_nTimer=SetTimer(1,TIMEOUT,NULL);
		}
		else
		{
			pDlg->KillTimer(m_nTimer);
			m_ListLog.InsertString(-1,"���ͳ�ʱ......");
			CommState=IDLESTATE;
			SendFile.Close();	//���һ�����ر��ļ�
			SendBuf[0]=EOT;SendBuf[1]=EOT;SendLen=2;	//֪ͨ���շ���ʼ����
			SendMsg("���ͣ�EOT");
			m_ListLog.InsertString(-1,"------���ؿ���״̬------");
			m_Send.EnableWindow(true);
			m_Setup.EnableWindow(true);
		}
		break;
	case RECVSTATE:
		m_ListLog.InsertString(-1,"���ճ�ʱ......");
		CommState=IDLESTATE;RecvPTR=0;	//�������,���ӶϿ�
		pDlg->m_ListLog.InsertString(-1,"���գ����");
		RecvFile.Close ();
		m_ListLog.InsertString(-1,"------���ؿ���״̬------");
		KillTimer(m_nTimer);
		m_Send.EnableWindow(true);
		m_Setup.EnableWindow(true);
		break;
	}
	CDialog::OnTimer(nIDEvent);
}

bool SetupCommPort()	//���ô��п�
{

	DCB	CommDCB;	//���пڵ��豸���ƿ�

	if (m_hCommPort==INVALID_HANDLE_VALUE)
		//CloseHandle(m_hCommPort);	//�ر��Ѵ򿪵Ĵ��п�
	{
		m_hCommPort=CreateFile (CommPort,		//�򿪴��п�
			GENERIC_READ|GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
			NULL);
		if(m_hCommPort==INVALID_HANDLE_VALUE)
			{
				AfxMessageBox("ϵͳ�޷��򿪴˴����豸��CreateFile��!");
				return false;
			}

		AfxBeginThread(ReadChar,NULL,THREAD_PRIORITY_NORMAL);
	}
	
	if(!GetCommState(m_hCommPort,&CommDCB)) //�õ�ԭ���Ĵ��ڲ���
	{
		AfxMessageBox("�޷��õ�����״̬��GetCommState��!");
		CloseHandle(m_hCommPort);
		return false;
	}
  
	CommDCB.BaudRate =Speed;	//�����µĴ��ڲ���
	CommDCB.ByteSize =8; 
	CommDCB.Parity = NOPARITY; 
    CommDCB.StopBits = ONESTOPBIT ;
	CommDCB.fBinary = TRUE ;
	CommDCB.fParity = FALSE; 

   	if(!SetCommState(m_hCommPort, &CommDCB))
	{
		AfxMessageBox("�޷����ô���״̬��SetupCommState��!");
		CloseHandle(m_hCommPort);
		return false;
	} 

	if (!SetupComm(m_hCommPort,BUFFER_SIZE,BUFFER_SIZE))		//���û�����
	{
		AfxMessageBox("���û���������SetupComm����");
		CloseHandle(m_hCommPort);
		return false;
	}
	if (!PurgeComm(m_hCommPort, PURGE_RXCLEAR|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_TXABORT)) //������н��պͷ��ͻ������е�����
	{
		AfxMessageBox("��ջ���������PurgeComm����");
		CloseHandle(m_hCommPort);
		return false;
	}
	RecvPTR=0;
	return true;
}

void CStopWaitCommDlg::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CloseHandle(m_hCommPort);
	CDialog::OnClose();
}

void CStopWaitCommDlg::OnBnClickedSetup()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	char str[100];

	if (m_SetupComm.DoModal()==IDOK)
	{
        CommPort=m_SetupComm.m_sCommPort;
		Speed=m_SetupComm.m_iSpeed;
		PacketLen=m_SetupComm.m_iPacketLen;
		DelayTime=m_SetupComm.m_iDelayTime;

		if (SetupCommPort())
		{
			m_Send.EnableWindow(true);
			sprintf(str,"StopWaitComm ���п�=%s������=%i��������=%i����·�ӳ�+%i",CommPort,Speed,PacketLen,DelayTime);
			pDlg->SetWindowText(str);
		}
		else
			m_Send.EnableWindow(false);
	}
}
