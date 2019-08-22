#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <wil\resource.h>
#include <string>

struct JobObjectEntry {
	DWORD ProcessId;
	HANDLE Handle;
	PVOID Object;
	std::vector<ULONG_PTR> Processes;
	JobObjectEntry* ParentJob{ nullptr };
	std::unordered_set<JobObjectEntry*> ChildJobs;
	std::wstring Name;
	DWORD TotalProcesses{ 0 };
};

class JobManager {
public:
	JobManager();

	bool EnumJobObjects();
	const std::vector<std::shared_ptr<JobObjectEntry>>& GetJobObjects() const;

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

