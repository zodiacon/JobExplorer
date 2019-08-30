#pragma once

struct DriverHelper final {
	static bool LoadDriver();
	static bool InstallDriver();
	static HANDLE OpenJobHandle(void* pObject);
	static bool IsDriverLoaded();

	static HANDLE _hDevice;
};

