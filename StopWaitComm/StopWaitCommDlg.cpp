// StopWaitCommDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "StopWaitComm.h"
#include "StopWaitCommDlg.h"
#include ".\stopwaitcommdlg.h"
#include "afxmt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CStopWaitCommDlg 对话框
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

//生成多项式G！！！
int G[] = {1,1,0,1};
int lenG = 4;
int R[];
int lenR = lenG - 1;
int RR[];
int lenRR = lenG - 1;

CFile		SendFile,RecvFile;	//发送和接收文件

HANDLE		m_hCommPort;		//保存打开的串行口设备句柄
char		RecvBuf[BUFFER_SIZE], SendBuf[BUFFER_SIZE]; //reveiver缓存 sender缓存！！！
UINT		SendLen,RecvPTR; //发送长度 接收指针！！！

UINT		CommState;	//保存通信状态：IdleState、SendState和RecvState三个宏定义！！！
BYTE		CRC;		//保存校验和
BYTE		Sequence;	//在等停协议中仅使用0和1
bool		ACKFlag;	//接收到ACK：true ！！！
bool		LastPacket;	//是否最后一包
UINT		STXFlag;	//0：没收到STX；1：收到STX但未收到序列号；2：收到STX而且收到序列号 ！！！
UINT_PTR	m_nTimer;	//定时器
UINT		SendCount;	//重发计数器
time_t		TimeStart,TimeFinish;
ULONG		TextLen;	//发送文件的总长度

CString		CommPort;//需要打开的串行口
UINT		PacketLen,Speed,DelayTime;//数据包大小，串行口的速率，为模拟长线路而增加的线路延迟

CStopWaitCommDlg* pDlg;

UINT ReadChar(PVOID hWnd);	//读取字符线程
void FormPacket();	//打包
void SendMsg(char *DisplayStr); //向串行口发送信息
bool SetupCommPort();	//设置串行口



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


// CStopWaitCommDlg 消息处理程序

BOOL CStopWaitCommDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
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
			sprintf(str,"StopWaitComm 串行口=%s，速率=%i，包长度=%i，线路延迟+%i",CommPort,Speed,PacketLen,DelayTime);
			pDlg->SetWindowText(str);
		}
		else
			m_Send.EnableWindow(false);
	}

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CStopWaitCommDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CStopWaitCommDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CStopWaitCommDlg::OnBnClickedOk() //发送按钮点击事件！！！
{
	// TODO: 在此添加控件通知处理程序代码

	//获取需要发送的文件名
	CString FileName;
	CFileDialog GetFileName(TRUE,NULL,NULL,OFN_HIDEREADONLY,"文本文件(*.txt)|*.txt||",NULL,0);
	if (GetFileName.DoModal()!=IDOK) return;
	FileName=GetFileName.GetPathName();
	if ( !SendFile.Open(FileName,CFile::modeRead|CFile::typeBinary,NULL))	//打开文件
	{
		MessageBox("发送文件打开错误！");
		return;
	}
	//进入发送状态！！！
	m_Send.EnableWindow(false);
	m_Setup.EnableWindow(false);

	m_ListLog.InsertString(-1,"------进入发送状态？？------");
	time(&TimeStart);	//开始计时
	CommState=SENDSTATE;ACKFlag=false;Sequence=0;LastPacket=false;SendCount=0;TextLen=0;
	SendBuf[0]=ENQ;SendLen=1;	//通知接收方开始发送 ENQ询问！！！
	SendMsg("发送？？：ENQ");
	m_nTimer=SetTimer(1,TIMEOUT,NULL);	//设置定时器
}

UINT ReadChar(PVOID hWnd)
{
	CEvent		RecvEvent(0,TRUE,0,0);//发送和接收事件
	OVERLAPPED	RecvOV;
	char		DisplayStr[256],tempStr[50];
	ULONG		ReadLen;
	double		DiffTime,Ratio;

	int len, i, j;
	int st;
	int ed;//end是最后移位所在下标
	int lenMM = 0;
	int MM[100000];
	int lenQQ = 4;
	int QQ[10];
	int fflag = 1;
	int bbit[10];

	memset(&RecvOV,0,sizeof(RecvOV));	//初始化Overlapped变量
	RecvOV.hEvent=RecvEvent;

	while (true)		//只要线程运行，就监视端口是否收到数据！！！
	{
		if (CommState==RECVSTATE)	m_nTimer=pDlg->SetTimer(1,3*TIMEOUT,NULL);	//设置定时器

		ReadFile(m_hCommPort,&RecvBuf[RecvPTR],1,&ReadLen,&RecvOV); //读字符 读到RecvBuf里！！！
		GetOverlappedResult(m_hCommPort,&RecvOV,&ReadLen,TRUE);		//等待读状态完成
		RecvEvent.ResetEvent();		//复位事件变量

		if (ReadLen>0)//非空
		{
			switch (CommState)//3中通信状态！！！
			{
			case IDLESTATE://空闲状态！！！
				if (RecvBuf[RecvPTR]==ENQ)	//收到ENQ，进入接收状态！！！
				{
					pDlg->m_ListLog.InsertString(-1,"------进入接收状态？？------");
					pDlg->m_ListLog.InsertString(-1,"接收：ENQ");
					if ( !RecvFile.Open("receive.txt",CFile::modeCreate|CFile::modeWrite|CFile::typeBinary,NULL))	//打开文件
					{
                            AfxMessageBox("接收文件打开错误！");
							pDlg->m_ListLog.InsertString(-1,"------返回空闲状态------");
							break;
					}
					pDlg->m_Send.EnableWindow(false);
					pDlg->m_Setup.EnableWindow(false);
					CommState=RECVSTATE;Sequence=0;RecvPTR=0;STXFlag=0;TextLen=0;	//进入接收状态
					time(&TimeStart);
					SendBuf[0]=ACK; SendBuf[1]=Sequence; SendLen=2;	//发送确认信息 组帧ACK！！！
					SendMsg("发送？？：ACK 0");
				}
				break;
			case SENDSTATE://发送状态！！！
				if (ACKFlag==true)//收到ACK-》发下一个包！！！
				{
					if (RecvBuf[RecvPTR]==Sequence)//已发包序号与ACK序号匹配！！！
					{
						sprintf(DisplayStr,"接收：ACK %i",Sequence);pDlg->m_ListLog.InsertString(-1,DisplayStr);
						SendCount=0;
						pDlg->KillTimer(m_nTimer);//关掉计时器！！！
						if (LastPacket==true)//是否最后一包！！！
						{
							CommState=IDLESTATE;//回到空闲状态！！！
							SendFile.Close();	//最后一包，关闭文件！！！

							SendBuf[0]=EOT;SendBuf[1]=EOT;SendLen=2;	//发送EOTEOT 通知接收方开始发送！！！
							SendMsg("发送？？：EOT");
							pDlg->m_ListLog.InsertString(-1,"------返回空闲状态------");
						
							time(&TimeFinish);	//显示统计信息！！！
							DiffTime=difftime(TimeFinish,TimeStart);
							if ((ULONG (DiffTime))==0) sprintf(tempStr,"xxxxxx");	//该程序只能精确到秒
							else
							{
								Ratio=(((TextLen*9.0)/Speed)/DiffTime)*100;
								sprintf(tempStr,"%f%%",Ratio);
							}
							pDlg->m_ListLog.InsertString(-1,"统计信息（注意本程序的传输时间只能精确到秒）：");
							sprintf(DisplayStr,"数据速率：%i字节/秒；数据包长：%i字节；线路延迟：+%i微秒；",Speed,PacketLen,DelayTime);
							pDlg->m_ListLog.InsertString(-1,DisplayStr);
							sprintf(DisplayStr,"数据长度：%li字节；传输耗时：%li秒；传输效率：%s。",TextLen,(ULONG) DiffTime,tempStr);
							pDlg->m_ListLog.InsertString(-1,DisplayStr);
							pDlg->m_Send.EnableWindow(true);
							pDlg->m_Setup.EnableWindow(true);
						}
						else//包没发完 发送下一包！！！
						{
							FormPacket();//组帧！！！
							SendMsg("发送：信息......");
							Sequence = (Sequence==0) ? 1:0;	//变换Sequence！！！
							m_nTimer=pDlg->SetTimer(1,TIMEOUT,0);	//设置定时器！！！
						}
					}
					ACKFlag=false;//复位ACKFLAG!!!
				}
				else//没收到ACK！！！
					if (RecvBuf[RecvPTR]==ACK) ACKFlag=true;
				break;
			case RECVSTATE://接收状态！！！
				switch (STXFlag)//STXFLAG有3种情况！！！
				{
				case 0://没收到STX！！！
					switch (RecvBuf[RecvPTR])
					{
					case EOT: //收到EOT 传输结束！！！
						CommState=IDLESTATE;RecvPTR=0;	//接收完成 连接断开 回到空闲状态！！！
						pDlg->m_ListLog.InsertString(-1,"接收：完成");
						RecvFile.Close ();//关闭记录接受信息的文件！！！
						pDlg->m_ListLog.InsertString(-1,"------返回空闲状态------");
						pDlg->KillTimer(m_nTimer);

						time(&TimeFinish);	//显示统计信息！！！
						DiffTime=difftime(TimeFinish,TimeStart);
						if ((ULONG (DiffTime))==0) sprintf(tempStr,"xxxxxx");	//该程序只能精确到秒
						else
						{
							Ratio=(((TextLen*9.0)/Speed)/DiffTime)*100;
							sprintf(tempStr,"%f%%",Ratio);
						}
						pDlg->m_ListLog.InsertString(-1,"统计信息（注意本程序的传输时间只能精确到秒）：");
						sprintf(DisplayStr,"数据速率：%i字节/秒；数据包长：%i字节；线路延迟：+%i微秒；",Speed,PacketLen,DelayTime);
						pDlg->m_ListLog.InsertString(-1,DisplayStr);
						sprintf(DisplayStr,"数据长度：%li字节；传输耗时：%li秒；传输效率：%s。",TextLen,(ULONG) DiffTime,tempStr);
						pDlg->m_ListLog.InsertString(-1,DisplayStr);
						pDlg->m_Send.EnableWindow(true);
						pDlg->m_Setup.EnableWindow(true);
						break;
					case ENQ: //ENQ 询问 传输开始！！！
						pDlg->m_ListLog.InsertString(-1,"接收：ENQ");
						SendBuf[0]=ACK; SendBuf[1]=Sequence; SendLen=2;	//回传ACK！！！
						sprintf(DisplayStr,"发送~~：ACK %i",Sequence);
						SendMsg(DisplayStr);
						break;
					case STX:
						STXFlag=1;//收到STX了
						break;
					}
					break;
				case 1://收到STX但没收到序号！！！
					if (RecvBuf[RecvPTR]==Sequence)//读到了序号！！！
					{
						STXFlag=2;
						CRC=0;
					}
					else STXFlag=0;
					break;
				case 2://收到STX&序号！！！
					switch (RecvBuf[RecvPTR])
					{
					case ETX://帧结束！！！
						len = RecvPTR;
						st = 0;
						ed = RecvPTR - 1;//end是最后移位所在下标
						for (i = st; i <= ed; i++)
						{
							lenMM++;
							MM[lenMM] = RecvBuf[i];
						}

						QQ[1] = MM[1]; QQ[2] = MM[2]; QQ[3] = MM[3]; QQ[4] = MM[4];
						fflag = 1;
						for (i = 1; i <= lenMM - 3; i++)
						{
							QQ[4] = MM[i + 3];
							bbit[1] = QQ[1] ^ (G[1] * fflag);
							bbit[2] = QQ[2] ^ (G[2] * fflag);
							bbit[3] = QQ[3] ^ (G[3] * fflag);
							bbit[4] = QQ[4] ^ (G[4] * fflag);

							fflag = bbit[2];
							QQ[1] = bbit[2];
							QQ[2] = bbit[3];
							QQ[3] = bbit[4];
							if (i + 3 == lenMM)
								for (j = 1; j <= 3; j++)
									RR[j] = bbit[j + 1];
						}
						CRC = 1;
						for (j = 1; j <= 3; j++)
							if (RR[j] == 1)
								CRC = 1;

						if (/*CRC==0*/0)//校验正确！！！
						{
							pDlg->m_ListLog.InsertString(-1,"接收：信息......");
							RecvFile.Write(RecvBuf,RecvPTR-1);	//将接收信息存入文件
							TextLen=TextLen+RecvPTR-1;
							Sequence=(Sequence==0)?1:0;//更新Sequence！！！
							SendBuf[0]=ACK; SendBuf[1]=Sequence; SendLen=2;//回传ACK！！！
							sprintf(DisplayStr,"发送？？：ACK %i",Sequence);
							SendMsg(DisplayStr);
						}
						RecvPTR=0;STXFlag=0;	//开始为新的一包做准备！！！
						break;
					//TODO: 此处是receiver的checksum核算程序！！！
					default://帧没结束！！！
						if (RecvPTR < BUFFER_SIZE)
							//CRC=CRC^RecvBuf[RecvPTR++];//奇偶校验！！！
							CRC = 0;
						else 
						{
							RecvPTR=0;
							STXFlag=0;
						}
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
void FormPacket()	//打包 checksum的构造程序！！！
{
	/*int	len,i;

	SendBuf[0]=STX;SendBuf[1]=Sequence; //STX为正文开头
	len=SendFile.Read(&SendBuf[2],PacketLen);	//读取数据

	CRC=0;	//计算校验和
	for (i=0;i<len;i++)
		CRC=CRC^SendBuf[i+2];

	SendBuf[2+len]=CRC;
	SendBuf[2+len+1]=ETX; //ETX为正文结束

	SendLen=2+1+len+1;
	LastPacket=(len==PacketLen)?false:true;
	TextLen=TextLen+len;*/

	int len, i,j;
	SendBuf[0] = STX; SendBuf[1] = Sequence;//帧头
	len = SendFile.Read(&SendBuf[2], PacketLen);//从发送文件中读取数据 len带0下标位长度
	int st = 2;
	int end = len - 1;//end是最后移位所在下标
	int lenM = 0;
	int M[100000];
	for (i = st;i <= end;i++)
	{
		lenM++;
		M[lenM] = SendBuf[i];
	}
	for (i = lenM; i <= lenM + lenR; i++)
		M[i] = 0;
	lenM += lenR;
	
	int lenQ = 4;
	int Q[10];
	Q[1] = M[1]; Q[2] = M[2]; Q[3] = M[3]; Q[4] = M[4];
	int flag = 1;
	for (i = 1; i <= lenM-3; i++)
	{
		int bit[10];
		Q[4] = M[i+3];
		bit[1] = Q[1] ^ (G[1] * flag);
		bit[2] = Q[2] ^ (G[2] * flag);
		bit[3] = Q[3] ^ (G[3] * flag);
		bit[4] = Q[4] ^ (G[4] * flag);

		flag = bit[2];
		Q[1] = bit[2];
		Q[2] = bit[3];
		Q[3] = bit[4];
		if (i + 3 == lenM)
			for (j = 1; j <= 3; j++)
			{
				R[j] = bit[j + 1];
				M[i + j] = R[j];
			}
	}

	for (i = 1; i <= lenR; i++)
		SendBuf[end+i] = M[lenM-lenR+i];
	SendBuf[end+lenR+1] = ETX; //ETX为正文结束

	SendLen = end + lenR + 1;
	LastPacket = (len == PacketLen) ? false : true;
	TextLen = TextLen + len; 
}
void SendMsg(char *DisplayStr)	//向串行口发送信息
{
	CEvent		SendEvent(0,TRUE,0,0);	//发送和接收事件
	OVERLAPPED	SendOV;
	ULONG		SentLen;

	memset(&SendOV,0,sizeof(SendOV));	//初始化Overlapped变量
	SendOV.hEvent=SendEvent;

	Sleep(DelayTime);		//模拟长线路延迟

	WriteFile(m_hCommPort,SendBuf,SendLen,&SentLen,&SendOV);
	GetOverlappedResult(m_hCommPort,&SendOV,&SentLen,TRUE);		//等待发送完成
	SendEvent.ResetEvent();
	pDlg->m_ListLog.InsertString(-1,DisplayStr);	//显示信息
}

void CStopWaitCommDlg::OnTimer(UINT nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	switch (CommState)
	{
	case SENDSTATE:
		if (SendCount<3)
		{
			SendCount++;
			KillTimer(m_nTimer);SendMsg("超时重传......");m_nTimer=SetTimer(1,TIMEOUT,NULL);
		}
		else
		{
			pDlg->KillTimer(m_nTimer);
			m_ListLog.InsertString(-1,"发送超时......");
			CommState=IDLESTATE;
			SendFile.Close();	//最后一包，关闭文件
			SendBuf[0]=EOT;SendBuf[1]=EOT;SendLen=2;	//通知接收方开始发送
			SendMsg("发送：EOT");
			m_ListLog.InsertString(-1,"------返回空闲状态------");
			m_Send.EnableWindow(true);
			m_Setup.EnableWindow(true);
		}
		break;
	case RECVSTATE:
		m_ListLog.InsertString(-1,"接收超时......");
		CommState=IDLESTATE;RecvPTR=0;	//接收完成,连接断开
		pDlg->m_ListLog.InsertString(-1,"接收：完成");
		RecvFile.Close ();
		m_ListLog.InsertString(-1,"------返回空闲状态------");
		KillTimer(m_nTimer);
		m_Send.EnableWindow(true);
		m_Setup.EnableWindow(true);
		break;
	}
	CDialog::OnTimer(nIDEvent);
}

bool SetupCommPort()	//设置串行口
{

	DCB	CommDCB;	//串行口的设备控制块

	if (m_hCommPort==INVALID_HANDLE_VALUE)
		//CloseHandle(m_hCommPort);	//关闭已打开的串行口
	{
		m_hCommPort=CreateFile (CommPort,		//打开串行口
			GENERIC_READ|GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,
			NULL);
		if(m_hCommPort==INVALID_HANDLE_VALUE)
			{
				AfxMessageBox("系统无法打开此串口设备（CreateFile）!");
				return false;
			}

		AfxBeginThread(ReadChar,NULL,THREAD_PRIORITY_NORMAL);
	}
	
	if(!GetCommState(m_hCommPort,&CommDCB)) //得到原来的串口参数
	{
		AfxMessageBox("无法得到串口状态（GetCommState）!");
		CloseHandle(m_hCommPort);
		return false;
	}
  
	CommDCB.BaudRate =Speed;	//设置新的串口参数
	CommDCB.ByteSize =8; 
	CommDCB.Parity = NOPARITY; 
    CommDCB.StopBits = ONESTOPBIT ;
	CommDCB.fBinary = TRUE ;
	CommDCB.fParity = FALSE; 

   	if(!SetCommState(m_hCommPort, &CommDCB))
	{
		AfxMessageBox("无法设置串口状态（SetupCommState）!");
		CloseHandle(m_hCommPort);
		return false;
	} 

	if (!SetupComm(m_hCommPort,BUFFER_SIZE,BUFFER_SIZE))		//设置缓冲区
	{
		AfxMessageBox("设置缓冲区错误（SetupComm）！");
		CloseHandle(m_hCommPort);
		return false;
	}
	if (!PurgeComm(m_hCommPort, PURGE_RXCLEAR|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_TXABORT)) //清除所有接收和发送缓冲区中的数据
	{
		AfxMessageBox("清空缓冲区错误（PurgeComm）！");
		CloseHandle(m_hCommPort);
		return false;
	}
	RecvPTR=0;
	return true;
}

void CStopWaitCommDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CloseHandle(m_hCommPort);
	CDialog::OnClose();
}

void CStopWaitCommDlg::OnBnClickedSetup()
{
	// TODO: 在此添加控件通知处理程序代码
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
			sprintf(str,"StopWaitComm 串行口=%s，速率=%i，包长度=%i，线路延迟+%i",CommPort,Speed,PacketLen,DelayTime);
			pDlg->SetWindowText(str);
		}
		else
			m_Send.EnableWindow(false);
	}
}
