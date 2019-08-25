// View.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "JobManager.h"
#include "VirtualListView.h"
#include "UndocListView.h"

struct IFrame;

class CView : 
	public CWindowImpl<CView>,
	public CVirtualListView<CView>,
	public IOwnerDataCallback {
public:
	DECLARE_WND_CLASS(nullptr)

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
		NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDoubleClick)
		CHAIN_MSG_MAP(CVirtualListView<CView>)

	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCopy)
	END_MSG_MAP()

	void RefreshJobList(JobManager& jm);
	void RefreshJob(const std::shared_ptr<JobObjectEntry>& job);
	void DoSort(const SortInfo* si);
	void SetFrame(IFrame*);

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnGetDispInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);

	void GetDispInfoJobList(NMLVDISPINFO* di);
	void GetDispInfoJob(NMLVDISPINFO* di);

	bool CompareJobs(const JobObjectEntry* j1, const JobObjectEntry* j2, const SortInfo* si);
	static PCWSTR GetSiloType(const JobObjectEntry* job);
	void GetGeneralJobInfo(PWSTR text, DWORD maxLen, int row, int col);
	void GetProcessesJobInfo(PWSTR text, DWORD maxLen, int row, int col);
	void GetJobLimitsInfo(PWSTR text, DWORD maxLen, int row, int col);
	void GetJobOpenHandlesInfo(PWSTR text, DWORD maxLen, int row, int col);

	static std::vector<std::pair<CString, CString>> GetJobLimits(JobObjectEntry* job);

private:
	enum class ViewType {
		None,
		JobList,
		SpecificJob
	};

	CListViewCtrl m_List;
	std::shared_ptr<JobObjectEntry> m_Job;
	ViewType m_ViewType;
	std::vector<std::shared_ptr<JobObjectEntry>> m_AllJobs;
	CImageListManaged m_ImagesLarge, m_ImagesSmall;
	CComPtr<IListView> m_spListView;
	std::vector<std::pair<CString, CString>> m_JobLimits;
	IFrame* m_pFrame{ nullptr };

	// Inherited via IOwnerDataCallback
	virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override;
	virtual ULONG __stdcall AddRef(void) override;
	virtual ULONG __stdcall Release(void) override;
	virtual HRESULT __stdcall GetItemPosition(int itemIndex, LPPOINT pPosition) override;
	virtual HRESULT __stdcall SetItemPosition(int itemIndex, POINT position) override;
	virtual HRESULT __stdcall GetItemInGroup(int groupIndex, int groupWideItemIndex, PINT pTotalItemIndex) override;
	virtual HRESULT __stdcall GetItemGroup(int itemIndex, int occurenceIndex, PINT pGroupIndex) override;
	virtual HRESULT __stdcall GetItemGroupCount(int itemIndex, PINT pOccurenceCount) override;
	virtual HRESULT __stdcall OnCacheHint(LVITEMINDEX firstItem, LVITEMINDEX lastItem) override;
};
