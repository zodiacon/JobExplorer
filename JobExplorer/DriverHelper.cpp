#include "stdafx.h"
#include "DriverHelper.h"
#include "SecurityHelper.h"
#include "resource.h"

#define IOCTL_KOBJEXP_DUP_HANDLE	CTL_CODE(0x8000, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct DupHandleData {
	ULONG Handle;
	ULONG SourcePid;
	ACCESS_MASK AccessMask;
};

HANDLE DriverHelper::_hDevice;

bool DriverHelper::LoadDriver() {
	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::OpenService(hScm.get(), L"KObjExp", SERVICE_ALL_ACCESS));
	if (!hService)
		return false;

	return ::StartService(hService.get(), 0, nullptr) ? true : false;
}

bool DriverHelper::InstallDriver() {
	if (!SecurityHelper::IsRunningElevated())
		return false;

	// locate the driver binary resource, extract to temp folder and install

	auto hRes = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_DRIVER), L"BIN");
	if (!hRes)
		return false;

	auto hGlobal = ::LoadResource(nullptr, hRes);
	if (!hGlobal)
		return false;

	auto size = ::SizeofResource(nullptr, hRes);
	void* pBuffer = ::LockResource(hGlobal);

	WCHAR path[MAX_PATH];
	::GetSystemDirectory(path, MAX_PATH);
	::wcscat_s(path, L"\\Drivers\\KObjExp.sys");
	wil::unique_hfile hFile(::CreateFile(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM, nullptr));
	if (!hFile)
		return false;

	DWORD bytes = 0;
	::WriteFile(hFile.get(), pBuffer, size, &bytes, nullptr);
	if (bytes != size)
		return false;

	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::CreateService(hScm.get(), L"KObjExp", nullptr, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, 
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, path, nullptr, nullptr, nullptr, nullptr, nullptr));
	if (!hService)
		return false;

	return ::StartService(hService.get(), 0, nullptr) ? true : false;
}

HANDLE DriverHelper::OpenJobHandle(HANDLE hSource, ULONG pid) {
	if (!_hDevice) {
		_hDevice = ::CreateFile(L"\\\\.\\KObjExp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
			nullptr, OPEN_EXISTING, 0, nullptr);
		if (_hDevice == INVALID_HANDLE_VALUE)
			return nullptr;
	}

	DupHandleData data;
	data.AccessMask = JOB_OBJECT_ALL_ACCESS;
	data.Handle = HandleToULong(hSource);
	data.SourcePid = pid;

	DWORD bytes;
	HANDLE hJob;
	return ::DeviceIoControl(_hDevice, IOCTL_KOBJEXP_DUP_HANDLE, &data, sizeof(data), &hJob, sizeof(hJob), &bytes, nullptr)
		? hJob : nullptr;
}

bool DriverHelper::IsDriverLoaded() {
	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::OpenService(hScm.get(), L"KObjExp", SERVICE_QUERY_STATUS));
	if (!hService)
		return false;

	SERVICE_STATUS status;
	if (!::QueryServiceStatus(hService.get(), &status))
		return false;

	return status.dwCurrentState == SERVICE_RUNNING;
}
