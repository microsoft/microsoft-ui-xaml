#pragma once
#include <windows.h>
#include <combaseapi.h>
#include <string>

namespace WinUIProfiler::Tap::SnapshotJson
{
    // Shared by the collector and its native serialization regression tests.
    inline std::wstring CaptureIdText(const GUID& captureId)
    {
        wchar_t guid[40]{};
        if (StringFromGUID2(captureId, guid, ARRAYSIZE(guid)) != 39) return {};
        // StringFromGUID2 returns "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".
        // System.Text.Json's Guid converter requires the unbraced D-format.
        return std::wstring(guid + 1, 36);
    }
}
