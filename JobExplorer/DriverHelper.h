#pragma once

struct DriverHelper final {
	static bool LoadDriver();
	static bool InstallDriver();
	static HANDLE OpenJobHandle(HANDLE hSource, ULONG pid);
	static bool IsDriverLoaded();

	static HANDLE _hDevice;
};

