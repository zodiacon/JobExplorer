// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "View.h"
#include "JobManager.h"
#include "FormatHelper.h"
#include <algorithm>

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

void CView::RefreshJobList(JobManager& jm) {
	m_ViewType = ViewType::JobList;
	m_List.DeleteAllItems();
	while (m_List.DeleteColumn(0))
		;

	m_List.EnableGroupView(FALSE);
	m_List.LockWindowUpdate();
	m_List.ModifyStyle(LVS_NOCOLUMNHEADER, 0);

	struct {
		PCWSTR Text;
		int Width;
		int Format = LVCFMT_LEFT;
	} columns[] = {
		{ L"Name", 250 },
		{ L"Address", 130, LVCFMT_RIGHT },
		{ L"Active Processes", 100, LVCFMT_RIGHT },
		{ L"CPU Time", 120, LVCFMT_RIGHT },
		{ L"Total Processes", 100, LVCFMT_RIGHT },
		{ L"Silo ID", 100, LVCFMT_RIGHT },
		{ L"Silo Type", 100 },
		//{ L"Child Job" , 80 }
	};

	int i = 0;
	for (auto& c : columns)
		m_List.InsertColumn(i++, c.Text, c.Format, c.Width);
	m_List.LockWindowUpdate(FALSE);

	if (m_AllJobs.empty())
		m_AllJobs = jm.GetJobObjects();
	m_List.SetItemCount(static_cast<int>(m_AllJobs.size()));
	SetSortMark(123);
}

void CView::RefreshJob(const std::shared_ptr<JobObjectEntry>& job) {
	m_Job = job;
	m_ViewType = ViewType::SpecificJob;

	m_List.DeleteAllItems();
	while (m_List.DeleteColumn(0))
		;
	m_List.RemoveAllGroups();
	m_List.ModifyStyle(0, LVS_NOCOLUMNHEADER);

	int count = 0;
	LVGROUP group = { sizeof(group) };
	group.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER | LVGF_ITEMS | LVGF_STATE;
	group.iGroupId = 1;
	group.uAlign = LVGA_HEADER_LEFT;
	group.cItems = count = 10;
	group.state = LVGS_COLLAPSIBLE;
	group.stateMask = LVGS_COLLAPSIBLE;

	group.pszHeader = L"General";
	m_List.InsertGroup(-1, &group);

	group.iGroupId = 2;
	group.cItems = (int)m_Job->BasicAccountInfo.ActiveProcesses;
	group.pszHeader = L"Processes";

	m_List.InsertGroup(-1, &group);
	count += group.cItems;

	group.iGroupId = 3;
	group.cItems = 10;
	group.pszHeader = L"Limits";
	m_List.InsertGroup(-1, &group);

	m_List.InsertColumn(0, L"Process Name", LVCFMT_LEFT, 270);
	m_List.InsertColumn(1, L"PID", LVCFMT_LEFT, 200);
	m_List.InsertColumn(2, L"Start Time", LVCFMT_LEFT, 200);
	//m_List.InsertColumn(3, L"Full Image Path", LVCFMT_LEFT, 700);

	m_List.EnableGroupView(TRUE);
	m_List.SetItemCount(30000);
}

void CView::DoSort(const SortInfo * si) {
	std::sort(m_AllJobs.begin(), m_AllJobs.end(), [this, si](const auto& j1, const auto& j2) {
		return CompareJobs(j1.get(), j2.get(), si);
		});
}

LRESULT CView::OnDestroy(UINT, WPARAM, LPARAM, BOOL &) {
	m_List.RemoveAllGroups();
	m_List.DeleteAllItems();

	return 0;
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL &) {
	m_List.Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDATA, 0, 123);
	m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_AUTOSIZECOLUMNS | LVS_EX_INFOTIP);

	m_ImagesSmall.Create(16, 16, ILC_COLOR32 | ILC_COLOR, 4, 4);
	m_ImagesSmall.AddIcon(AtlLoadIcon(IDI_PARENTJOB));
	m_ImagesSmall.AddIcon(AtlLoadIcon(IDI_CHILDJOB));
	m_List.SetImageList(m_ImagesSmall, LVSIL_SMALL);
	//::SetWindowTheme(m_List, L"Explorer", nullptr);

	CComPtr<IListView> spListView;
	m_List.SendMessage(LVM_QUERYINTERFACE, (WPARAM)&__uuidof(IListView), (LPARAM)&spListView);
	ATLASSERT(spListView);

	spListView->SetOwnerDataCallback(this);

	return 0;
}

LRESULT CView::OnSize(UINT, WPARAM, LPARAM lParam, BOOL&) {
	int cx = GET_X_LPARAM(lParam), cy = GET_Y_LPARAM(lParam);
	if (::IsWindow(m_List.m_hWnd))
		m_List.MoveWindow(0, 0, cx, cy);

	return 0;
}

LRESULT CView::OnGetDispInfo(int, LPNMHDR pnmh, BOOL &) {
	auto di = (NMLVDISPINFO*)pnmh;
	if (m_ViewType == ViewType::JobList)
		GetDispInfoJobList(di);
	else
		GetDispInfoJob(di);
	return 0;
}

void CView::GetDispInfoJobList(NMLVDISPINFO * di) {
	auto& item = di->item;
	auto& data = m_AllJobs[item.iItem];
	if (item.mask & LVIF_TEXT) {
		switch (item.iSubItem) {
			case 1:		// address
				::StringCchPrintf(item.pszText, item.cchTextMax, L"0x%p", data->Object);
				break;

			case 0:		// name
				item.pszText = (PWSTR)data->Name.c_str();
				break;

			case 2:		// active processes
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", (int)data->Processes.size());
				break;

			case 3:		// CPU time
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%s",
					FormatHelper::ToTimeSpan(data->BasicAccountInfo.TotalKernelTime.QuadPart + data->BasicAccountInfo.TotalUserTime.QuadPart));
				break;

			case 4:		// total processes
				::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data->BasicAccountInfo.TotalProcesses);
				break;

			case 5:		// Job ID
				if (data->JobId)
					::StringCchPrintf(item.pszText, item.cchTextMax, L"%d", data->JobId);
				break;

			case 6:		// silo type
				if (data->JobId)
					::StringCchCopy(item.pszText, item.cchTextMax, GetSiloType(data.get()));
				break;

			case 7:
				if (data->ParentJob)
					::StringCchCopy(item.pszText, item.cchTextMax, L"Yes");
				break;
		}
	}
	if (item.mask & LVIF_IMAGE) {
		item.iImage = data->ParentJob ? 1 : 0;
	}
}

void CView::GetDispInfoJob(NMLVDISPINFO * di) {
	auto& item = di->item;
	int group = item.iItem / 10000;
	int index = item.iItem % 10000;
	if (item.mask & LVIF_TEXT) {
		switch (group) {
			case 0:		// general
				GetGeneralJobInfo(item.pszText, item.cchTextMax, index, item.iSubItem);
				break;

			case 1:		// processes
				GetProcessesJobInfo(item.pszText, item.cchTextMax, index, item.iSubItem);
				break;
		}
	}
}

bool CView::CompareJobs(const JobObjectEntry * j1, const JobObjectEntry * j2, const SortInfo * si) {
	switch (si->SortColumn) {
		case 0:		// name
			return SortStrings(j1->Name, j2->Name, si->SortAscending);
		case 1:		// object
			return SortNumbers(j1->Object, j2->Object, si->SortAscending);
		case 2:		// active processes
			return SortNumbers(j1->Processes.size(), j2->Processes.size(), si->SortAscending);
		case 3:		// CPU time
			return SortNumbers(j1->BasicAccountInfo.TotalKernelTime.QuadPart + j1->BasicAccountInfo.TotalUserTime.QuadPart,
				j2->BasicAccountInfo.TotalKernelTime.QuadPart + j2->BasicAccountInfo.TotalUserTime.QuadPart, si->SortAscending);
		case 4:		// total processes
			return SortNumbers(j1->BasicAccountInfo.TotalProcesses, j2->BasicAccountInfo.TotalProcesses, si->SortAscending);
		case 5:		// job ID
			return SortNumbers(j1->JobId, j2->JobId, si->SortAscending);
		case 6:		// silo type
			return SortStrings(GetSiloType(j1), GetSiloType(j2), si->SortAscending);
	}
	return false;
}

PCWSTR CView::GetSiloType(const JobObjectEntry * job) {
	if (job->JobId == 0)
		return L"";
	return job->IsServerSilo ? L"Server Silo" : L"App Silo";
}

void CView::GetGeneralJobInfo(PWSTR text, DWORD maxLen, int row, int col) {
	static PCWSTR names[] = {
		L"Address:", L"Name:", L"Active Processes:", L"Total Processes:",
		L"User Time:", L"Kernel Time:", L"CPU Time:", L"Terminated Processes:",
		L"Page Faults:", L"Silo ID:"
	};

	if (col == 0) {
		ATLASSERT(row < _countof(names));
		::StringCchCopy(text, maxLen, names[row]);
		return;
	}

	if (col > 1)
		return;

	auto& ba = m_Job->BasicAccountInfo;

	switch (row) {
		case 0:
			::StringCchPrintf(text, maxLen, L"0x%p", m_Job->Object);
			break;

		case 1:
			::StringCchCopy(text, maxLen, m_Job->Name.c_str());
			break;

		case 2:
			::StringCchPrintf(text, maxLen, L"%u", ba.ActiveProcesses);
			break;

		case 3:
			::StringCchPrintf(text, maxLen, L"%u", ba.TotalProcesses);
			break;

		case 4:
			::StringCchPrintf(text, maxLen, L"%s", FormatHelper::ToTimeSpan(ba.TotalUserTime.QuadPart));
			break;

		case 5:
			::StringCchPrintf(text, maxLen, L"%s", FormatHelper::ToTimeSpan(ba.TotalKernelTime.QuadPart));
			break;

		case 6:
			::StringCchPrintf(text, maxLen, L"%s", FormatHelper::ToTimeSpan(ba.TotalKernelTime.QuadPart + ba.TotalUserTime.QuadPart));
			break;

		case 7:
			::StringCchPrintf(text, maxLen, L"%u", ba.TotalTerminatedProcesses);
			break;

		case 8:
			::StringCchPrintf(text, maxLen, L"%u", ba.TotalPageFaultCount);
			break;

		case 9:
			if (m_Job->JobId)
				::StringCchPrintf(text, maxLen, L"%d", m_Job->JobId);
			break;

	}
}

void CView::GetProcessesJobInfo(PWSTR text, DWORD maxLen, int row, int col) {
	//if (row == 0) {
	//	// headers
	//	static PCWSTR headers[] = {
	//		L"Name", L"PID", L"Start Time", L"Full Path"
	//	};
	//	::StringCchCopy(text, maxLen, headers[col]);
	//	return;
	//}
	auto pid = (DWORD)(DWORD)m_Job->Processes[row];
	if (col == 1) {
		::StringCchPrintf(text, maxLen, L"PID: %d (0x%X)", pid, pid);
		return;
	}

	wil::unique_handle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
	if (!hProcess)
		return;

	WCHAR path[MAX_PATH];
	DWORD size = MAX_PATH;
	switch (col) {
		case 0:
			if (::QueryFullProcessImageName(hProcess.get(), 0, path, &size)) {
				::StringCchCopy(text, maxLen, ::wcsrchr(path, L'\\') + 1);
			}
			break;

		case 2:
			FILETIME create, dummy;
			if (::GetProcessTimes(hProcess.get(), &create, &dummy, &dummy, &dummy)) {
				::StringCchPrintf(text, maxLen, L"Created: %s", FormatHelper::ToDateTime(*(int64_t*)&create));
			}
			break;

		case 3:
			::QueryFullProcessImageName(hProcess.get(), 0, text, &size);
			break;
	}
}

HRESULT __stdcall CView::QueryInterface(REFIID riid, void ** ppvObject) {
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == __uuidof(IOwnerDataCallback) || riid == __uuidof(IUnknown)) {
		*ppvObject = static_cast<IOwnerDataCallback*>(this);
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG __stdcall CView::AddRef(void) {
	return 2;
}

ULONG __stdcall CView::Release(void) {
	return 1;
}

HRESULT __stdcall CView::GetItemPosition(int itemIndex, LPPOINT pPosition) {
	return E_NOTIMPL;
}

HRESULT __stdcall CView::SetItemPosition(int itemIndex, POINT position) {
	return E_NOTIMPL;
}

HRESULT __stdcall CView::GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex) {
	*pTotalItemIndex = groupIndex * 10000 + groupWideItemIndex;
	return S_OK;
}

HRESULT __stdcall CView::GetItemGroup(int itemIndex, int occurenceIndex, PINT pGroupIndex) {
	*pGroupIndex = itemIndex / 10000;
	return S_OK;
}

HRESULT __stdcall CView::GetItemGroupCount(int itemIndex, PINT pOccurenceCount) {
	*pOccurenceCount = 1;

	return S_OK;
}

HRESULT __stdcall CView::OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem) {
	return E_NOTIMPL;
}

