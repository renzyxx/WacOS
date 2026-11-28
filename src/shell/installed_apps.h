#pragma once
#include <string>

namespace installed_apps {
const wchar_t* MenuLabel();
void Open(const std::wstring& preferredName);
}
