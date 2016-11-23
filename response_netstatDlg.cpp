#include "stdafx.h"
#include "response_netstat.h"
#include "response_netstatDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef struct _FIND_WINDOW_PARAM
{
	LPCTSTR pszClassName;
	LPCTSTR pszWindowName;
	DWORD dwProcessID;
	HWND hwnd;

} FIND_WINDOW_PARAM, *LPFIND_WINDOW_PARAM;

BOOL CALLBACK FindWindowFromPidProc(HWND hwnd, LPARAM lParam)
{
	BOOL bRes = TRUE;
	DWORD dwProcessID = 0;
	LPFIND_WINDOW_PARAM findParam = (LPFIND_WINDOW_PARAM)lParam;
	GetWindowThreadProcessId(hwnd, &dwProcessID);

	if(findParam->dwProcessID == dwProcessID)
	{
		findParam->hwnd = ::GetWindow(hwnd, GW_OWNER);
		if(findParam->hwnd == NULL)
			findParam->hwnd = hwnd;
	
		bRes = FALSE;
	}

	return bRes;
}

HWND FindWindowFromProcessId(DWORD dwProcessID)
{
	HWND hwnd = NULL;
	FIND_WINDOW_PARAM param;
	param.pszClassName = NULL;
	param.pszWindowName = NULL;
	param.dwProcessID = dwProcessID;
	param.hwnd = NULL;

	::EnumWindows(FindWindowFromPidProc, (LPARAM)&param);

	return param.hwnd;
}

CString GetExecuteFileName(DWORD processID)
{
	CString strFileName;

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap)
	{
		if (Process32First(hProcessSnap, &pe32))
		{
			do 
			{				
				if(pe32.th32ProcessID == processID)
				{
					strFileName.Format("%s", pe32.szExeFile);
					break;
				}

			} while (Process32Next(hProcessSnap, &pe32));
		}
	}
	CloseHandle(hProcessSnap);

	return strFileName;
}

Cresponse_netstatDlg::Cresponse_netstatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cresponse_netstatDlg::IDD, pParent)
{
	m_nCheckPortCount = 0;
}

void Cresponse_netstatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_ListCtrl);
}

BEGIN_MESSAGE_MAP(Cresponse_netstatDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL Cresponse_netstatDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW, 0);
			
	m_ListCtrl.DeleteAllItems(); 
	m_ListCtrl.InsertColumn(0, _T("타이틀"), LVCFMT_CENTER, 300, -1); 
    m_ListCtrl.InsertColumn(1, _T("포트"), LVCFMT_LEFT, 130, -1); 
	m_ListCtrl.InsertColumn(2, _T("파일명"), LVCFMT_LEFT, 200, -1); 
	
	ReadPortList();	

	SetTimer(1, 1000, NULL);

	return TRUE;  
}

void Cresponse_netstatDlg::OnPaint()
{
	CPaintDC dc(this); 
}

void Cresponse_netstatDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == 1)
	{
		m_nCheckPortCount++;

		if(m_nCheckPortCount == 60)
		{
			ReadPortList();	
			m_nCheckPortCount = 0;
		}	
	}		

	CDialog::OnTimer(nIDEvent);
}

void Cresponse_netstatDlg::ReadPortList()
{
	m_ListCtrl.DeleteAllItems(); 
		
	CString strInput;
	if( RunNetstate(strInput) )
	{
		if( ParsingPort(strInput) )
		{
			for (int i = 0; i < m_NetstateInfo.size(); i++)
			{
				DWORD dwProcessID = (DWORD)_ttoi((LPCTSTR)m_NetstateInfo[i].PID);
				HWND hwnd = FindWindowFromProcessId(dwProcessID);	
				if(hwnd)
				{
					char wndName[MAX_PATH + 1] = {0, };
					::GetWindowText(hwnd, wndName, MAX_PATH + 1);
										
					char className[MAX_PATH + 1] = {0, };
					::GetClassName(hwnd, className, sizeof(className));

					CString strTitle;
					strTitle.Format("%s", wndName);

					CString strClass;
					strClass.Format("%s", className);

					if(strTitle == "") strTitle = strClass;

					CString strExe = GetExecuteFileName(dwProcessID);

					InsertListView(strTitle, m_NetstateInfo[i].port, strExe);	
				}			
			}
		}
	}
	m_NetstateInfo.clear();
}

BOOL Cresponse_netstatDlg::RunNetstate(CString &strInfo)
{
	HANDLE hSTDINWrite, hSTDINRead;    
	HANDLE hSTDOUTWrite, hSTDOUTRead;  
	
	SECURITY_ATTRIBUTES sa;   
    
	sa.bInheritHandle = TRUE;   
	sa.lpSecurityDescriptor = NULL;   
	sa.nLength = sizeof(sa);   
	
	if( !CreatePipe(&hSTDOUTRead, &hSTDOUTWrite, &sa, 0) )   
		return FALSE;   
	
	if( !CreatePipe(&hSTDINRead, &hSTDINWrite, &sa, 0) )   
		return FALSE;
	
	PROCESS_INFORMATION  pi; 
	ZeroMemory(&pi, sizeof(pi));
	STARTUPINFO  si; 
	GetStartupInfo(&si);
	
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;      
	si.hStdInput = hSTDINRead;     
	si.hStdOutput = hSTDOUTWrite;   
	si.hStdError = hSTDOUTWrite;   
	
	char szCmd[256] = {0};
	::strcpy(szCmd, "netstat.exe -nao");
	
	if( !::CreateProcess(NULL, szCmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi) )   
		return FALSE;

	::CloseHandle(hSTDOUTWrite);   
	::CloseHandle(hSTDINRead);
	
	char szData[2048] = {0};
	DWORD dwBytes;
	int i = 0;
	CString strTmp;
	
	while(::ReadFile(hSTDOUTRead, szData, sizeof(szData), &dwBytes, NULL))
	{    
		strTmp.Format("%s",szData);
		strInfo += strTmp;		
	}	
	
	DWORD uExitCode=0;
	::TerminateProcess(pi.hProcess,uExitCode);	
	::WaitForSingleObject(pi.hProcess, INFINITE);  
	
	::CloseHandle(hSTDOUTRead);   
	::CloseHandle(hSTDINWrite);   
	::CloseHandle(pi.hProcess);   
	::CloseHandle(pi.hThread); 

	return TRUE;   
}

BOOL Cresponse_netstatDlg::ParsingPort(CString strInput)
{
	BOOL bFind = FALSE;
	
	int nCount = strInput.Find(_T("\n"));
	int nLength = 0;	
	CString strPrefix, strNext;
	strNext = strInput;
	CString strTmp;
	while(nCount!=-1)
	{
		strPrefix = strNext.Left(nCount);

		if(strstr(strPrefix, "ESTABLISHED"))
		{
			CStringArray strLine;
			GetSplitSpace(strPrefix, strLine);

			//CString strFindPort1 = SeparatePort(strLine.GetAt(1));//로컬주소
			//CString strFindPort2 = SeparatePort(strLine.GetAt(2));//외부주소

			AddPortList(strLine.GetAt(4), strLine.GetAt(1));			
			strLine.RemoveAll();
		}
		
		nLength = strNext.GetLength();
		strTmp = strNext.Right(nLength - nCount - 1);
		strNext = strTmp;
		nCount = strNext.Find(_T("\n"));
		bFind = TRUE;
	}
	
	if(strNext != "")
	{
		if(strstr(strPrefix, "ESTABLISHED"))
		{
			CStringArray strLine;
			GetSplitSpace(strPrefix, strLine);
			
			//CString strFindPort1 = SeparatePort(strLine.GetAt(1));//로컬주소
			//CString strFindPort2 = SeparatePort(strLine.GetAt(2));//외부주소

			AddPortList(strLine.GetAt(4), strLine.GetAt(1));			
			strLine.RemoveAll();
		}
	}
	return bFind;
}

void Cresponse_netstatDlg::AddPortList(CString PID, CString port)
{
	NetstateInfo pInfo;
	pInfo.PID = PID;
	pInfo.port = port;
	m_NetstateInfo.push_back(pInfo);
}

CString Cresponse_netstatDlg::SeparatePort(CString strInput)
{
	CString strValue = "";
	int nCount = strInput.Find(_T(":"));
	int nLength = strInput.GetLength();
	strValue = strInput.Right(nLength - nCount - 1);
	strValue.Replace("\"","");
	strValue.TrimLeft();
	strValue.TrimRight();
	return strValue;
}

BOOL Cresponse_netstatDlg::GetSplitSpace(CString strInput, CStringArray& strResultArr)
{
	BOOL bFind = FALSE;

	strResultArr.RemoveAll();
	int nCount = strInput.Find(_T(" "));
	int nLength = 0;	
	CString strPrefix, strNext;
	strNext = strInput;
	CString strTmp;
	while(nCount!=-1)
	{
		strPrefix = strNext.Left(nCount);

		if(strPrefix != "")
			strResultArr.Add(strPrefix);

		nLength = strNext.GetLength();
		strTmp = strNext.Right(nLength - nCount - 1);
		strNext = strTmp;
		nCount = strNext.Find(_T(" "));
		bFind = TRUE;
	}
	if(strNext != "")
		strResultArr.Add(strNext);
	return bFind;
}

void Cresponse_netstatDlg::InsertListView(CString strTitle, CString strPort, CString strHwnd)
{
	int nIndex = m_ListCtrl.InsertItem(0, _T(strTitle));
	m_ListCtrl.SetItemText(nIndex, 1, _T(strPort));
	m_ListCtrl.SetItemText(nIndex, 2, _T(strHwnd));
}

