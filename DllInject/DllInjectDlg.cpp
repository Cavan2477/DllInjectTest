
// DllInjectDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "DllInject.h"
#include "DllInjectDlg.h"
#include "afxdialogex.h"
#include <tlhelp32.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDllInjectDlg �Ի���



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


// CDllInjectDlg ��Ϣ�������

BOOL CDllInjectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CDllInjectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
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
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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

	// �ɽ������ƻ��PID
	dwPid = GetProcId(szProcessName);

	// ע��szDllName��dwPid
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

	// 1.��Ŀ�����
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);

	if (hProcess == NULL)
		return;

	// 2.������ע��DLL�ļ�����·���ĳ���
	int nDllLen = lstrlen(pszDllName) + sizeof(char);

	// 3.��Ŀ�����������һ�鳤��ΪnDllLen��С���ڴ�ռ�
	PVOID pDllAddr = VirtualAllocEx(hProcess, NULL, nDllLen, MEM_COMMIT, PAGE_READWRITE);

	if (pDllAddr == NULL)
	{
		CloseHandle(hProcess);
		return;
	}

	DWORD dwWriteNum = 0;		// ʵ��д�볤��

	// 4.����ע��DLL�ļ�������·��д����Ŀ�����������Ŀռ���
	if (!WriteProcessMemory(hProcess, pDllAddr, pszDllName, nDllLen, &dwWriteNum))
		return;

	// 5.���LoadLibraryA()�����ĵ�ַ
	FARPROC pFunAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), pszFunName);

	// 6.����Զ���߳�
	HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pFunAddr, pDllAddr, 0, NULL);

	WaitForSingleObject(hRemoteThread, INFINITE);

	CloseHandle(hRemoteThread);
	CloseHandle(hProcess);
}