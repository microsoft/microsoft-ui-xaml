#pragma once
#include <windows.h>
#include <combaseapi.h>
#include <string>

namespace WinUIProfiler::Tap::SnapshotJson
{
    inline std::wstring CaptureIdText(const GUID& captureId)
    {
        wchar_t guid[40]{};
        if (StringFromGUID2(captureId, guid, ARRAYSIZE(guid)) != 39) return {};
        // System.Text.Json requires the unbraced D-format.
        return std::wstring(guid + 1, 36);
    }
}
