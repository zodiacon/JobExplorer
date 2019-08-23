#pragma once

class FormatHelper {
public:
	static CString SizeToString(int64_t size);
	static PCWSTR BoolToString(bool value);

	template<typename T>
	static CString ToHex(const T& value) {
		CString text;
		text.Format(sizeof(T) > 4 ? L"0x%p\n" : L"0x%X", value);
		return text;
	}

	static CString ToTimeSpan(int64_t ts);
	static CString ToDateTime(int64_t dt, PCWSTR default = L"N/A");
};

