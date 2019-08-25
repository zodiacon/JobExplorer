// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "View.h"
#include "MainFrm.h"
#include "ClipboardHelper.h"
#include "SecurityHelper.h"
#include "ProcessHelper.h"

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	if (m_pFind && m_pFind->IsDialogMessageW(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	UIUpdateToolBar();
	return FALSE;
}

void CMainFrame::SelectJob(void * address) {
	m_Tree.SelectItem(m_JobsMap.find(address)->second);
}

void CMainFrame::InitializeTree() {
	m_Images.Create(16, 16, ILC_COLOR32 | ILC_COLOR, 4, 4);
	UINT ids[] = { IDI_PARENTJOB, IDI_CHILDJOB, IDR_MAINFRAME, IDI_PROCESS, IDI_PROCESSES };
	for(auto id : ids)
		m_Images.AddIcon(AtlLoadIcon(id));

	m_Tree.SetImageList(m_Images, TVSIL_NORMAL);
	
	RefreshTree();
}

void CMainFrame::RefreshTree() {
	m_Tree.LockWindowUpdate();
	CWaitCursor wait;
	m_Tree.DeleteAllItems();
	m_AllJobsNode = m_Tree.InsertItem(L"Job List", 2, 2, TVI_ROOT, TVI_LAST);
	//m_ProcessesNode = m_Tree.InsertItem(L"Processes", 4, 4, TVI_ROOT, TVI_LAST);

	m_JobsMap.clear();

	m_JobMgr.EnumJobObjects();
	m_JobsMap.reserve(m_JobMgr.GetJobObjects().size());

	for (auto& job : m_JobMgr.GetJobObjects()) {
		if (job->ParentJob == nullptr) {
			AddJobNode(job.get(), TVI_ROOT, 0);
		}
	}
	m_AllJobsNode.Expand(TVE_EXPAND);
	m_AllJobsNode.Select();
	m_Tree.LockWindowUpdate(FALSE);
}

void CMainFrame::ExpandAll(bool expand) {
	m_Tree.LockWindowUpdate();
	for(auto item = m_Tree.GetRootItem(); item.m_hTreeItem; item = item.GetNextSibling())
		item.Expand(expand ? TVE_EXPAND : TVE_COLLAPSE);
	
	m_Tree.EnsureVisible(m_Tree.GetSelectedItem());
	m_Tree.LockWindowUpdate(FALSE);
}

void CMainFrame::AddJobNode(JobObjectEntry* job, HTREEITEM parent, int icon) {
	CString text;
	text.Format(L"0x%p", job->Object);
	if (!job->Name.IsEmpty())
		text += CString(L" (") + job->Name + L")";
	auto node = m_Tree.InsertItem(text, icon, icon, parent, TVI_LAST);
	node.SetData((DWORD_PTR)job->Object);
	m_JobsMap.insert({ job->Object, node.m_hTreeItem });
	for (auto& child : job->ChildJobs)
		AddJobNode(child, node.m_hTreeItem, 1);
	for (auto pid : job->Processes) {
		CString name = ProcessHelper::GetProcessName((DWORD)pid);
		text.Format(L"%s (%d)", name, pid);
		m_Tree.InsertItem(text, 3, 3, node, TVI_LAST);
	}
	//m_Tree.Expand(node.m_hTreeItem, TVE_EXPAND);
}

LRESULT CMainFrame::OnEditFind(WORD, WORD, HWND, BOOL&) {
	if (m_pFind == nullptr || m_pFind->IsTerminating()) {
		m_pFind = new CFindReplaceDialog;
		m_pFind->Create(TRUE, nullptr);
		m_pFind->ShowWindow(SW_SHOWDEFAULT);
	}
	return 0;
}

LRESULT CMainFrame::OnFind(UINT, WPARAM, LPARAM, BOOL& bHandled) {
	ATLASSERT(m_pFind);
	if (m_pFind->IsTerminating()) {
		m_pFind = nullptr;
		return 0;
	}
	auto isDown = m_pFind->SearchDown();

	CString text(m_pFind->GetFindString());
	text.MakeUpper();

	auto item = m_Tree.GetSelectedItem();
	for (;;) {
		item = isDown ? item.GetNextVisible() : item.GetPrevVisible();
		if (item == nullptr)
			break;
		std::shared_ptr<JobObjectEntry> job;
		while (job == nullptr && item != nullptr) {
			job = m_JobMgr.GetJobByObject((void*)item.GetData());
			if (job != nullptr)
				break;
			item = isDown ? item.GetNextVisible() : item.GetPrevVisible();
		}

		if (job) {
			bool found = false;
			CString name(job->Name);
			name.MakeUpper();
			if (name.Find(text) >= 0) {
				found = true;
				break;
			}
			if (!found) {
				for (auto& p : job->Processes) {
					auto pname = ProcessHelper::GetProcessName((DWORD)p);
					pname.MakeUpper();
					if (pname.Find(text) >= 0) {
						found = true;
						break;
					}
				}
			}
			if (found) {
				item.Select();
				m_FindPos = item;
				return 0;
			}
		}
	}
	MessageBox(L"Job/process not found", L"Job Explorer");
	m_FindPos = nullptr;
	return 0;
}

LRESULT CMainFrame::OnTreeSelectionChanged(int, LPNMHDR, BOOL&) {
	auto selected = m_Tree.GetSelectedItem();
	if (selected == m_AllJobsNode) {
		m_view.RefreshJobList(m_JobMgr);
	}
	else {
		auto job = m_JobMgr.GetJobByObject((void*)selected.GetData());
		if(job)
			m_view.RefreshJob(job);
	}

	return 0;
}

LRESULT CMainFrame::OnTreeExpanding(int, LPNMHDR hdr, BOOL &) {
	auto tv = (NMTREEVIEW*)hdr;
	if (tv->action == TVE_EXPAND && tv->itemNew.lParam) {
		// job node
		
	}
	return LRESULT();
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	CMenuHandle menu = GetMenu();

	if (SecurityHelper::IsRunningElevated()) {
		// delete menu item
		menu.GetSubMenu(0).DeleteMenu(ID_FILE_RUNASADMINISTRATOR, MF_BYCOMMAND);
		menu.GetSubMenu(0).DeleteMenu(0, MF_BYPOSITION);	// delete separator

		CString text;
		GetWindowText(text);
		text += L" (Administrator)";
		SetWindowText(text);
	}
	else {
		m_CmdBar.AddIcon(SecurityHelper::GetShieldIcon(), ID_FILE_RUNASADMINISTRATOR);
	}

	m_CmdBar.AttachMenu(menu);
	m_CmdBar.LoadImages(IDR_MAINFRAME);

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
	m_view.SetFrame(this);

	m_splitter.SetSplitterPanes(m_Tree, m_view);
	UpdateLayout();
	m_splitter.SetSplitterPosPct(25);

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

	CReBarCtrl(m_hWndToolBar).LockBands(TRUE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	InitializeTree();
	m_Tree.SetFocus();

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

LRESULT CMainFrame::OnViewRefresh(WORD, WORD, HWND, BOOL& bHandled) {
	if (m_Tree.GetSelectedItem() == m_AllJobsNode)
		RefreshTree();
	else
		m_view.RefreshJob(nullptr);
	return 0;
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

LRESULT CMainFrame::OnViewJobList(WORD, WORD, HWND, BOOL &) {
	m_Tree.SelectItem(m_AllJobsNode.m_hTreeItem);
	return 0;
}

LRESULT CMainFrame::OnViewExpandAll(WORD, WORD, HWND, BOOL &) {
	ExpandAll(true);
	return 0;
}

LRESULT CMainFrame::OnViewCollapseAll(WORD, WORD, HWND, BOOL &) {
	ExpandAll(false);
	return 0;
}

LRESULT CMainFrame::OnEditCopy(WORD, WORD, HWND, BOOL &handled) {
	if (::GetFocus() == m_Tree) {
		CString text;
		m_Tree.GetItemText(m_Tree.GetSelectedItem(), text);
		ClipboardHelper::CopyText(*this, text);
	}
	else {
		handled = FALSE;
	}
	return 0;
}

LRESULT CMainFrame::OnRunAsAdmin(WORD, WORD, HWND, BOOL &) {
	WCHAR path[MAX_PATH];
	DWORD size = MAX_PATH;
	if (::QueryFullProcessImageName(::GetCurrentProcess(), 0, path, &size)) {
		SHELLEXECUTEINFO shex = { sizeof(shex) };
		shex.lpVerb = L"runas";
		shex.lpFile = path;
		shex.nShow = SW_SHOWDEFAULT;
		if (::ShellExecuteEx(&shex)) {
			PostMessage(WM_CLOSE);
			return 0;
		}
	}

	MessageBox(L"Elevation failed", L"Job Explorer");
	return 0;
}

