#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <wil\resource.h>
#include <string>

struct JobHandleEntry {
	DWORD ProcessId;
	HANDLE Handle;
	PVOID Object;
	std::vector<ULONG_PTR> Processes;
	JobHandleEntry* ParentJob{ nullptr };
	std::vector<JobHandleEntry*> ChildJobs;
	std::wstring Name;
};

class JobManager {
public:
	JobManager();
	~JobManager();

	bool EnumJobObjects();

private:
	static wil::unique_handle DuplicateJobHandle(HANDLE h, DWORD pid);

	static int GetJobObjectType();

private:
	std::vector<std::shared_ptr<JobHandleEntry>> _jobHandles;
	std::unordered_map<PVOID, std::shared_ptr<JobHandleEntry>> _jobMap;

	static int JobTypeIndex;
};

