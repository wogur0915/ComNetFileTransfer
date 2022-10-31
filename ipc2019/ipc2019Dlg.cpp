﻿
// ipc2019Dlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ipc2019.h"
#include "ipc2019Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cipc2019Dlg 대화 상자



Cipc2019Dlg::Cipc2019Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IPC2019_DIALOG, pParent)
	, CBaseLayer("ChatDlg")
	, m_bSendReady(FALSE)

	, m_unSrcAddr("")
	, m_unDstAddr("")
	, m_stMessage(_T(""))
	, _selectedFilePath(_T(""))
{
	//대화상자 멤버 변수 초기화
	//  m_unDstAddr = 0;
	//  unSrcAddr = 0;
	//  m_stMessage = _T("");
	//대화 상자 멤버 초기화 완료

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//Protocol Layer Setting
	m_LayerMgr.AddLayer(new CChatAppLayer("ChatApp"));
	m_LayerMgr.AddLayer(new CEthernetLayer("Ethernet"));
	m_LayerMgr.AddLayer(new CNILayer("Link"));
	m_LayerMgr.AddLayer(this);

	// 레이어를 연결한다. (레이어 생성)
	m_LayerMgr.ConnectLayers("Link ( *Ethernet ( *ChatApp ( *ChatDlg ) ) )");

	m_ChatApp = (CChatAppLayer*)m_LayerMgr.GetLayer("ChatApp");
	//Protocol Layer Setting
}

void Cipc2019Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SRC, m_unSrcAddr);
	DDX_Text(pDX, IDC_EDIT_DST, m_unDstAddr);
	DDX_Text(pDX, IDC_EDIT_MSG, m_stMessage);
	DDX_Control(pDX, IDC_LIST_CHAT, m_ListChat);
	DDX_Control(pDX, IDC_COMBO1, deviceComboBox);
	DDX_Text(pDX, IDC_FILEPATHBOX, _selectedFilePath);
	DDX_Control(pDX, IDC_PROGRESS1, _progressBar);
}

BEGIN_MESSAGE_MAP(Cipc2019Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_ADDR, &Cipc2019Dlg::OnBnClickedButtonAddr)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &Cipc2019Dlg::OnBnClickedButtonSend)
	
	ON_BN_CLICKED(IDC_CHECK_TOALL, &Cipc2019Dlg::OnBnClickedCheckToall)
	ON_CBN_SELCHANGE(IDC_COMBO1, &Cipc2019Dlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_FILESELECTBUTTON, &Cipc2019Dlg::OnBnClickedFileselectbutton)
	ON_BN_CLICKED(IDC_FILESENDBUTTON, &Cipc2019Dlg::OnBnClickedFilesendbutton)
END_MESSAGE_MAP()


// Cipc2019Dlg 메시지 처리기

BOOL Cipc2019Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	// Fill the device list
	m_LayerMgr.GetLayer("Link");

	CNILayer* linkLayer = (CNILayer*)m_LayerMgr.GetLayer("Link");
	auto vector = linkLayer->GetDevicesList();
	for (size_t i = 0; i < vector->size(); i++)
	{
		auto& device = (*vector)[i];

		deviceComboBox.InsertString(i, device.description);
		deviceComboBox.SetItemDataPtr(i, device.name);
	}
	SetDlgState(IPC_INITIALIZING);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void Cipc2019Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void Cipc2019Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR Cipc2019Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}




void Cipc2019Dlg::OnBnClickedButtonSend()
{
	UpdateData(TRUE);

	if (!m_stMessage.IsEmpty())
	{
		SendData();
		m_stMessage = "";

		(CEdit*)GetDlgItem(IDC_EDIT3)->SetFocus();
	}

	UpdateData(FALSE);
}

void Cipc2019Dlg::SendData()
{
	CString MsgHeader = _T("[You] ");

	m_ListChat.AddString(MsgHeader + m_stMessage);

	int nlength = m_stMessage.GetLength();
	unsigned char* ppayload = new unsigned char[nlength + 1];
	memcpy(ppayload, (unsigned char*)(LPCTSTR)m_stMessage, nlength);
	ppayload[nlength] = '\0';

	m_ChatApp->Send(ppayload, nlength + 1);
}

BOOL Cipc2019Dlg::Receive(unsigned char* ppayload)
{
	m_ListChat.AddString((LPCTSTR)ppayload);
	return TRUE;
}

BOOL Cipc2019Dlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			if (::GetDlgCtrlID(::GetFocus()) == IDC_EDIT3)
				OnBnClickedButtonSend();					
			return FALSE;
		case VK_ESCAPE: return FALSE;
		}
		break;
	}

	return CDialog::PreTranslateMessage(pMsg);
}


void Cipc2019Dlg::SetDlgState(int state)
{
	UpdateData(TRUE);

	CButton* pChkButton = (CButton*)GetDlgItem(IDC_CHECK1);

	CButton* pSendButton = (CButton*)GetDlgItem(bt_send);
	CButton* pSetAddrButton = (CButton*)GetDlgItem(bt_setting);
	CEdit* pMsgEdit = (CEdit*)GetDlgItem(IDC_EDIT3);
	CEdit* pSrcEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
	CEdit* pDstEdit = (CEdit*)GetDlgItem(IDC_EDIT2);

	CString dstAddr;
	switch (state)
	{
	case IPC_INITIALIZING:
		pSendButton->EnableWindow(FALSE);
		pMsgEdit->EnableWindow(FALSE);
		m_ListChat.EnableWindow(FALSE);
		break;
	case IPC_READYTOSEND:
		pSendButton->EnableWindow(TRUE);
		pMsgEdit->EnableWindow(TRUE);
		m_ListChat.EnableWindow(TRUE);
		break;
	case IPC_WAITFORACK:	break;
	case IPC_ERROR:		break;
	case IPC_UNICASTMODE:
		m_unDstAddr = _T("");
		pDstEdit->EnableWindow(TRUE);
		break;
	case IPC_BROADCASTMODE:
		m_unDstAddr = _T("ff:ff:ff:ff:ff:ff");
		pDstEdit->EnableWindow(FALSE);
		break;
	case IPC_ADDR_SET:
		pSetAddrButton->SetWindowText(_T("재설정(&R)"));
		pSrcEdit->EnableWindow(FALSE);
		pDstEdit->EnableWindow(FALSE);
		//pChkButton->EnableWindow(FALSE);
		break;
	case IPC_ADDR_RESET:
		pSetAddrButton->SetWindowText(_T("설정(&O)"));
		pSrcEdit->EnableWindow(TRUE);
		if (!pChkButton->GetCheck())
			pDstEdit->EnableWindow(TRUE);
		pChkButton->EnableWindow(TRUE);
		break;
	}

	UpdateData(FALSE);
}


void Cipc2019Dlg::EndofProcess()
{
	m_LayerMgr.DeAllocLayer();
}

void Cipc2019Dlg::OnBnClickedButtonAddr()
{
	UpdateData(TRUE);

	if (!m_unDstAddr ||
		!m_unSrcAddr)
	{
		AfxMessageBox(_T("주소를 설정 오류발생",
			"경고"),
			MB_OK | MB_ICONSTOP);

		return;
	}

	if (m_bSendReady) {
		SetDlgState(IPC_ADDR_RESET);
		SetDlgState(IPC_INITIALIZING);
	}
	else {
		auto ethernet = (CEthernetLayer*)m_LayerMgr.GetLayer("Ethernet");

		// Parse the mac address string into bytes, then set the source / destination address in the ethernet layer
		CNILayer::PhysicalAddress srcAddress, dstAddress;
		 
		// Scanf requires 32-bit destinations, so copy it into this intermediate storage
		unsigned int intermediate[6];
		sscanf_s( (const char*)m_unSrcAddr, "%02x:%02x:%02x:%02x:%02x:%02x", intermediate, intermediate + 1, intermediate + 2, intermediate + 3, intermediate + 4, intermediate + 5);
		srcAddress.a = intermediate[0]; srcAddress.b = intermediate[1]; srcAddress.c = intermediate[2]; srcAddress.d = intermediate[3]; srcAddress.e = intermediate[4]; srcAddress.f = intermediate[5];
		sscanf_s( (const char*)m_unDstAddr, "%02x:%02x:%02x:%02x:%02x:%02x", intermediate, intermediate + 1, intermediate + 2, intermediate + 3, intermediate + 4, intermediate + 5);
		dstAddress.a = intermediate[0]; dstAddress.b = intermediate[1]; dstAddress.c = intermediate[2]; dstAddress.d = intermediate[3]; dstAddress.e = intermediate[4]; dstAddress.f = intermediate[5];
		
		ethernet->SetSourceAddress((unsigned char*)&srcAddress);
		ethernet->SetDestinAddress((unsigned char*)&dstAddress);

		auto networkInterface = (CNILayer*)m_LayerMgr.GetLayer("Link");
		auto currentSelection = deviceComboBox.GetCurSel();
		char* deviceId = (char*)deviceComboBox.GetItemDataPtr(currentSelection);
		networkInterface->StartReceive(deviceId);
		SetDlgState(IPC_ADDR_SET);
		SetDlgState(IPC_READYTOSEND);
	}

	m_bSendReady = !m_bSendReady;
}



void Cipc2019Dlg::OnBnClickedCheckToall()
{
	CButton* pChkButton = (CButton*)GetDlgItem(IDC_CHECK_TOALL);

	if (pChkButton->GetCheck()) {
		SetDlgState(IPC_BROADCASTMODE);
	}
	else {
		SetDlgState(IPC_UNICASTMODE);
	}
}


void Cipc2019Dlg::OnCbnSelchangeCombo1()
{
	// TODO: Add your control notification handler code here

	int selectedIndex = deviceComboBox.GetCurSel();
	if (selectedIndex == 0xffffffff)
	{
		return;
	}
	char* deviceName = (char*)deviceComboBox.GetItemDataPtr(selectedIndex);
	CNILayer* linkLayer = (CNILayer*)m_LayerMgr.GetLayer("Link");
	CNILayer::PhysicalAddress address{};
	if (linkLayer->GetMacAddress(deviceName, &address))
	{
		CString format;
		format.Format(_T("%02x:%02x:%02x:%02x:%02x:%02x"), (int)address.a, (int)address.b, (int)address.c, (int)address.d, (int)address.e, (int)address.f);
		m_unSrcAddr = format;
		UpdateData(FALSE);
	}

}


void Cipc2019Dlg::OnBnClickedFileselectbutton()
{
	CFileDialog dialog(TRUE);

	if (dialog.DoModal() == IDOK)
	{
		CString path = dialog.GetPathName();
		_selectedFilePath = path;
		UpdateData(FALSE);
	}
}


void Cipc2019Dlg::OnBnClickedFilesendbutton()
{
	// TODO: Add your control notification handler code here
}
