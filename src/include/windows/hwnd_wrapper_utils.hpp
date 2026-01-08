#pragma once
#include <objbase.h>
#include <windows.h>

#include <iomanip>
#include <sstream>
#include <string>

namespace windows {

class hwnd_wrapper_utils {
   public:
    static std::wstring generate_class_name(const std::wstring& app_name) {
        // 生成 GUID
        GUID guid;
        CoCreateGuid(&guid);

        // 将 GUID 转换为字符串
        WCHAR guid_str[40];
        StringFromGUID2(guid, guid_str, sizeof(guid_str) / sizeof(WCHAR));

        // GUID 字符串通常包含花括号，这里去掉它们以获得类似 C# 的格式
        std::wstring random_name = guid_str;
        if (!random_name.empty() && random_name[0] == L'{') {
            random_name = random_name.substr(1, random_name.length() - 2);
        }

        auto tid = ::GetCurrentThreadId();

        // 构建类名字符串
        std::wstringstream ss;
        ss << L"hwnd_wrapper[" << app_name << L";" << tid << L";" << random_name
           << L"]";

        return ss.str();
    }
};
}  // namespace windows