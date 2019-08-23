// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "View.h"
#include "MainFrm.h"

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	return FALSE;
}

void CMainFrame::InitializeTree() {
	m_Images.Create(16, 16, ILC_COLOR32 | ILC_COLOR, 4, 4);
	m_Images.AddIcon(AtlLoadIcon(IDI_PARENTJOB));
	m_Images.AddIcon(AtlLoadIcon(IDI_CHILDJOB));
	m_Images.AddIcon(AtlLoadIcon(IDR_MAINFRAME));
	m_Tree.SetImageList(m_Images, TVSIL_NORMAL);

	m_AllJobsNode = m_Tree.InsertItem(L"Job List", 2, 2, TVI_ROOT, TVI_LAST);

	m_JobMgr.EnumJobObjects();

	for (auto& job : m_JobMgr.GetJobObjects()) {
		if (job->ParentJob == nullptr) {
			AddJobNode(job.get(), TVI_ROOT, 0);
		}
	}
	m_AllJobsNode.Select();
}

void CMainFrame::AddJobNode(JobObjectEntry* job, HTREEITEM parent, int icon) {
	CString text;
	text.Format(L"0x%p", job->Object);
	if (!job->Name.empty())
		text += CString(L" (") + job->Name.c_str() + L")";
	auto node = m_Tree.InsertItem(text, icon, icon, parent, TVI_LAST);
	node.SetData((DWORD_PTR)job->Object);
	for (auto& child : job->ChildJobs)
		AddJobNode(child, node.m_hTreeItem, 1);
	//m_Tree.Expand(node.m_hTreeItem, TVE_EXPAND);
}

LRESULT CMainFrame::OnTreeSelectionChanged(int, LPNMHDR, BOOL&) {
	auto selected = m_Tree.GetSelectedItem();
	if (selected == m_AllJobsNode) {
		m_view.RefreshJobList(m_JobMgr);
	}
	else {
		auto job = m_JobMgr.GetJobByObject((void*)selected.GetData());
		m_view.RefreshJob(job);
	}

	return 0;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	// remove old menu
	SetMenu(nullptr);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, nullptr, TRUE);

	CreateSimpleStatusBar();

	m_hWndClient = m_splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_Tree.Create(m_splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
	m_view.Create(m_splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

	m_splitter.SetSplitterPanes(m_Tree, m_view);
	UpdateLayout();
	m_splitter.SetSplitterPosPct(25);

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_TREEPANE, 1);

	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	InitializeTree();

	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	// TODO: add code to initialize document

	return 0;
}

LRESULT CMainFrame::OnViewRefresh(WORD, WORD, HWND, BOOL&) {
	return LRESULT();
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnViewTreePane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool bShow = (m_splitter.GetSinglePaneMode() != SPLIT_PANE_NONE);
	m_splitter.SetSinglePaneMode(bShow ? SPLIT_PANE_NONE : SPLIT_PANE_RIGHT);
	UISetCheck(ID_VIEW_TREEPANE, bShow);

	return 0;
}

