// StopWaitComm.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error �ڰ������� PCH �Ĵ��ļ�֮ǰ������stdafx.h��
#endif

#include "resource.h"		// ������


// CStopWaitCommApp:
// �йش����ʵ�֣������ StopWaitComm.cpp
//

class CStopWaitCommApp : public CWinApp
{
public:
	CStopWaitCommApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CStopWaitCommApp theApp;
