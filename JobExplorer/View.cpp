// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "View.h"
#include "JobManager.h"

BOOL CView::PreTranslateMessage(MSG* pMsg) {
	pMsg;
	return FALSE;
}

void CView::RefreshJobList(JobManager& jm) {
	m_List.DeleteAllItems();
	while (m_List.DeleteColumn(0))
		;

	m_List.LockWindowUpdate();

	struct {
		PCWSTR Text;
		int Width;
		int Format = LVCFMT_LEFT;
	} columns[] = {
		{ L"Address", 100 },
		{ L"Name", 150 },
		{ L"Active Processes", 100, LVCFMT_RIGHT },
		{ L"Total Processes", 100, LVCFMT_RIGHT },
	};

	int i = 0;
	for (auto& c : columns)
		m_List.InsertColumn(i++, c.Text, c.Format, c.Width);

	m_List.LockWindowUpdate(FALSE);
}

LRESULT CView::OnCreate(UINT, WPARAM, LPARAM, BOOL &) {
	m_List.Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDATA, 0, 123);

	return LRESULT();
}

LRESULT CView::OnSize(UINT, WPARAM, LPARAM lParam, BOOL&) {
	int cx = GET_X_LPARAM(lParam), cy = GET_Y_LPARAM(lParam);
	if (m_List)
		m_List.MoveWindow(0, 0, cx, cy);

	return 0;
}

