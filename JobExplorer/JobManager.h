#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>

struct OpenHandle {
	DWORD hJob;
	DWORD ProcessId;

	OpenHandle(DWORD h, DWORD pid);
};

struct JobObjectEntry {
	DWORD ProcessId;
	HANDLE Handle;
	PVOID Object;
	std::vector<ULONG_PTR> Processes;
	JobObjectEntry* ParentJob{ nullptr };
	std::unordered_set<JobObjectEntry*> ChildJobs;
	std::vector<OpenHandle> OpenHandles;
	CString Name;
	wil::unique_handle hDup;
	DWORD JobId{ 0 };
	JOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicAccountInfo{};
	bool IsServerSilo;
};

class JobManager {
public:
	JobManager();

	bool EnumJobObjects();
	const std::vector<std::shared_ptr<JobObjectEntry>>& GetJobObjects() const;
	std::shared_ptr<JobObjectEntry> GetJobByObject(void* pObject) const;

private:
	static wil::unique_handle DuplicateJobHandle(HANDLE h, DWORD pid);
	static bool EnableDebugPrivilege();
	static int GetJobObjectType();
	void AnalyzeJobs();

private:
	std::vector<std::shared_ptr<JobObjectEntry>> _jobObjects;
	std::unordered_map<PVOID, std::shared_ptr<JobObjectEntry>> _jobMap;
	std::unordered_map<DWORD, std::vector<std::shared_ptr<JobObjectEntry>>> _processesInJobs;
	static int JobTypeIndex;
};

