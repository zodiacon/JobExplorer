// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CView : public CWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};
