#pragma once
#include <windows.h>
#include <cstdint>
#include <string>

struct IXamlDiagnostics;

namespace WinUIProfiler::Tap
{
    struct TreeSnapshotResult
    {
        std::string Json;
    };

    // Called from the pipe's MTA worker; private memory diagnostics are optional.
    HRESULT CaptureTreeSnapshot(IXamlDiagnostics* diagnostics, HANDLE stop, GUID captureId,
        uint64_t targetCreationFileTime, TreeSnapshotResult& result, std::string& failure) noexcept;
}
