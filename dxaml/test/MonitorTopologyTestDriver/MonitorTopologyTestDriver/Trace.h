// SPDX-License-Identifier: MS-PL
// Copyright (c) Microsoft Corporation.
// Topology Test sample-derived driver. See LICENSE.
#pragma once
#include <strsafe.h>
#include <evntprov.h>

// Manifest-free diagnostics distinguish callback failures from denials earlier
// in the stack. Collect only in the isolated test machine.
inline constexpr GUID TopologyTraceProvider =
{ 0xe943ab91, 0x8655, 0x43ed, { 0xaa, 0x41, 0x77, 0x08, 0xa1, 0x5e, 0x43, 0x19 } };
inline REGHANDLE TopologyTraceHandle = 0;

inline void LogMessage(const wchar_t* message) noexcept
{
    OutputDebugStringW(message);
    if (TopologyTraceHandle)
    {
        EventWriteString(TopologyTraceHandle, 4, 0, message);
    }
}

inline void LogFailure(const wchar_t* operation, LONG status) noexcept
{
    wchar_t text[256] = {};
    if (SUCCEEDED(StringCchPrintfW(text, ARRAYSIZE(text),
        L"MonitorTopologyTestDriver: %s failed (0x%08lX).\n", operation,
        static_cast<unsigned long>(status))))
    {
        LogMessage(text);
    }
}
