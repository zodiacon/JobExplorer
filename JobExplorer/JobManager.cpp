#include "stdafx.h"
#include "JobManager.h"
#include "NtDll.h"

int JobManager::JobTypeIndex;

JobManager::JobManager() {
	if (JobTypeIndex == 0) {
		JobTypeIndex = GetJobObjectType();
	}
}

JobManager::~JobManager() = default;

bool JobManager::EnumJobObjects() {
	ULONG len = 1 << 23;
	auto buffer = std::make_unique<BYTE[]>(len);
	auto status = ::NtQuerySystemInformation(SystemExtendedHandleInformation, buffer.get(), len, &len);
	auto p = (SYSTEM_HANDLE_INFORMATION_EX*)buffer.get();
	
	_jobHandles.clear();
	_jobHandles.reserve(128);
	_jobMap.reserve(128);

	BYTE nameBuffer[1024];
	for (ULONG i = 0; i < p->NumberOfHandles; i++) {
		auto& hi = p->Handles[i];
		if (hi.ObjectTypeIndex == JobTypeIndex) {
			// job object handle

			if (_jobMap.find(hi.Object) != _jobMap.end())
				continue;

			auto entry = std::make_shared<JobHandleEntry>();
			entry->Handle = reinterpret_cast<HANDLE>(hi.HandleValue);
			entry->ProcessId = (DWORD)hi.UniqueProcessId;
			entry->Object = hi.Object;
			auto hDup = DuplicateJobHandle(entry->Handle, entry->ProcessId);
			if (hDup) {
				int count = 1024;
				JOBOBJECT_BASIC_PROCESS_ID_LIST list;
				DWORD size = sizeof(list) + (count - 1) * sizeof(ULONG_PTR);
				auto buffer = std::make_unique<BYTE[]>(size);
				if (::QueryInformationJobObject(hDup.get(), JobObjectBasicProcessIdList, &list, size, nullptr)) {
					entry->Processes.resize(list.NumberOfProcessIdsInList);
					::memcpy(entry->Processes.data(), list.ProcessIdList, list.NumberOfProcessIdsInList * sizeof(ULONG_PTR));
				}
				ULONG len;
				if (STATUS_SUCCESS == ::NtQueryObject(hDup.get(), ObjectNameInformation, nameBuffer, sizeof(nameBuffer), &len)) {
					auto info = (OBJECT_NAME_INFORMATION*)nameBuffer;
					entry->Name = std::wstring(info->Name.Buffer, info->Name.Length / sizeof(WCHAR));
				}
			}
			_jobMap[entry->Object] = entry;
			_jobHandles.push_back(std::move(entry));
		}
	}

	return false;
}

wil::unique_handle JobManager::DuplicateJobHandle(HANDLE h, DWORD pid) {
	wil::unique_handle hProcess(::OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid));
	if (!hProcess)
		return nullptr;

	wil::unique_handle hDup;
	::DuplicateHandle(hProcess.get(), h, ::GetCurrentProcess(), hDup.addressof(), JOB_OBJECT_QUERY, FALSE, 0);
	return hDup;
}

int JobManager::GetJobObjectType() {
	ULONG len = 1 << 17;
	auto buffer = std::make_unique<BYTE[]>(len);
	::NtQueryObject(nullptr, ObjectTypesInformation, buffer.get(), len, nullptr);
	auto types = (OBJECT_TYPES_INFORMATION*)buffer.get();
	auto type = (OBJECT_TYPE_INFORMATION*)(buffer.get() + sizeof(void*));
	
	for (ULONG i = 0; i < types->NumberOfTypes; i++) {
		if (::wcscmp(type->TypeName.Buffer, L"Job") == 0)
			return (int)type->TypeIndex;
		auto step = sizeof(OBJECT_TYPE_INFORMATION) + type->TypeName.MaximumLength;
		step = (step + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*);
		type = (OBJECT_TYPE_INFORMATION*)((BYTE*)type + step);
	}
	return 0;
}


