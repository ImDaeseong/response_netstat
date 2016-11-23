#include "stdafx.h"
#include "response_netstat.h"
#include "response_netstatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(Cresponse_netstatApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

Cresponse_netstatApp::Cresponse_netstatApp()
{
}

Cresponse_netstatApp theApp;

BOOL Cresponse_netstatApp::InitInstance()
{
	HANDLE hMutex = ::CreateMutex(NULL, FALSE, _T("ResponsenetstatInstance"));
	if(GetLastError() == ERROR_ALREADY_EXISTS)
		return FALSE;

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	AfxEnableControlContainer();

	Cresponse_netstatDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	return FALSE;
}
