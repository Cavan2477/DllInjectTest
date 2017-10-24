
// DllInjectDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "DllInject.h"
#include "DllInjectDlg.h"
#include "afxdialogex.h"
#include <tlhelp32.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDllInjectDlg 对话框



CDllInjectDlg::CDllInjectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDllInjectDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDllInjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDllInjectDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_INJECT, &CDllInjectDlg::OnBnClickedBtnInject)
END_MESSAGE_MAP()


// CDllInjectDlg 消息处理程序

BOOL CDllInjectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDllInjectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDllInjectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CDllInjectDlg::OnBnClickedBtnInject()
{
	char szDllName[MAX_PATH] = {0};
	char szProcessName[MAXBYTE] = {0};

	DWORD dwPid = 0;

	GetDlgItemText(IDC_EDIT_DLL, szDllName, MAX_PATH);
	GetDlgItemText(IDC_EDIT_PROCESS, szProcessName, MAXBYTE);

	// 由进程名称获得PID
	dwPid = GetProcId(szProcessName);

	// 注入szDllName到dwPid
	InjectDll(dwPid, szDllName);
}

DWORD CDllInjectDlg::GetProcId(char* pszProcessName)
{
	// E:\MyWork\Dev\Test\MFC\DllInject\Debug\TestInjectDll.dll
	// superdic.exe
	if (NULL == pszProcessName)
		return -1;

	PROCESSENTRY32 pe32 = {0};
	HANDLE hSnap,hProcess;

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnap == NULL)
		return -1;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	
	if (Process32First(hSnap, &pe32))
	{
		do
		{
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, (DWORD)pe32.th32ProcessID);

			if (hProcess == INVALID_HANDLE_VALUE)
				continue;

			if (_stricmp(pe32.szExeFile, pszProcessName) == 0)
				return pe32.th32ProcessID;

			TRACE("processName=%s\r\n", pe32.szExeFile);
		} while (Process32Next(hSnap, &pe32) != FALSE);
	}
	
	return 0;
}

void CDllInjectDlg::InjectDll(DWORD dwPid, char* pszDllName)
{
	if (dwPid < 0 || NULL == pszDllName || lstrlen(pszDllName) == 0)
		return;

	char* pszFunName = "LoadLibraryA";

	// 1.打开目标进程
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);

	if (hProcess == NULL)
		return;

	// 2.计算欲注入DLL文件完整路径的长度
	int nDllLen = lstrlen(pszDllName) + sizeof(char);

	// 3.在目标进程中申请一块长度为nDllLen大小的内存空间
	PVOID pDllAddr = VirtualAllocEx(hProcess, NULL, nDllLen, MEM_COMMIT, PAGE_READWRITE);

	if (pDllAddr == NULL)
	{
		CloseHandle(hProcess);
		return;
	}

	DWORD dwWriteNum = 0;		// 实际写入长度

	// 4.将欲注入DLL文件的完整路径写入在目标进程中申请的空间内
	if (!WriteProcessMemory(hProcess, pDllAddr, pszDllName, nDllLen, &dwWriteNum))
		return;

	// 5.获得LoadLibraryA()函数的地址
	FARPROC pFunAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), pszFunName);

	// 6.创建远程线程
	HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunAddr, pDllAddr, 0, NULL);

	WaitForSingleObject(hRemoteThread, INFINITE);

	CloseHandle(hRemoteThread);
	CloseHandle(hProcess);
}