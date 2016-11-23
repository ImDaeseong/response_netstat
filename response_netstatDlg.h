#pragma once

struct NetstateInfo
{
	CString PID;
	CString port;
};

class Cresponse_netstatDlg : public CDialog
{
public:
	Cresponse_netstatDlg(CWnd* pParent = NULL);	

	void ReadPortList();

	enum { IDD = IDD_RESPONSE_NETSTAT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
	DECLARE_MESSAGE_MAP()

private:
	int m_nCheckPortCount;	
	CListCtrl m_ListCtrl;

	std::vector<NetstateInfo> m_NetstateInfo;

	BOOL RunNetstate(CString &strInfo);
	BOOL ParsingPort(CString strInput);
    void AddPortList(CString PID, CString port);
	CString SeparatePort(CString strInput);
	BOOL GetSplitSpace(CString strInput, CStringArray& strResultArr);
	void InsertListView(CString strTitle, CString strPort, CString strHwnd);	
};
