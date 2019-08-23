#include "stdafx.h"
#include "FormatHelper.h"


CString FormatHelper::SizeToString(int64_t size) {
	CString text;
	if (size < 10 << 10)
		text.Format(L"%lld B", size);
	else if (size < 10 << 20)
		text.Format(L"%lld KB", (size + 512) >> 10);
	else if (size < 10ULL << 30)
		text.Format(L"%lld MB", (size + (1 << 19)) >> 20);
	else
		text.Format(L"%lld GB", (size + (1 << 29)) >> 30);

	return text;
}

PCWSTR FormatHelper::BoolToString(bool value) {
	return value ? L"True" : L"False";
}


CString FormatHelper::ToTimeSpan(int64_t ticks) {
	auto msec = ticks / 10000;
	auto seconds = msec / 1000;
	msec -= seconds * 1000;
	int minutes = (int)(seconds / 60);
	seconds -= minutes * 60;
	int hours = minutes / 60;
	minutes -= hours * 60;
	CString text;
	text.Format(L"%02d:%02d:%02d.%03d", hours, minutes, seconds, msec);
	return text;
}

CString FormatHelper::ToDateTime(int64_t dt, PCWSTR default) {
	SYSTEMTIME st;
	CString text;
	FileTimeToSystemTime((FILETIME*)&dt, &st);
	if (st.wYear < 1900)
		text = default;
	else {
		SystemTimeToTzSpecificLocalTime(nullptr, &st, &st);
		text.Format(L"%s.%03d", CTime(st).Format(L"%x %X"), st.wMilliseconds);
	}
	return text;
}

