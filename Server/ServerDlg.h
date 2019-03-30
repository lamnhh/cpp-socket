#pragma once

#include "afxwin.h"
#include <afxsock.h>
#include <vector>
#include <string>
#include "helper.h"
using namespace std;

struct Client {
    SOCKET socket;
    char username[20];
};

struct AuthInfo {
    CString username;
    CString password;
    bool loggedIn;
};

class CServerDlg : public CDialogEx
{
public:
	CServerDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_SERVER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
    
    SOCKET server;
    SOCKADDR_IN serverAddress;
    vector<Client> clientList;
    vector<AuthInfo> authList;

    LRESULT handleEvents(WPARAM wParam, LPARAM lParam);

    void fetchAllUsername(char*);
    void sendToAll(Message msg);

public:
    afx_msg void OnBnClickedOk();
    CListBox logs;
};
