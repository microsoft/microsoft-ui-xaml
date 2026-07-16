// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShutdownOrderValidation.h"
#include <strsafe.h>
#include <vector>
#include <fstream>

struct ShutdownOrderValidationStaticInfo
{
    ~ShutdownOrderValidationStaticInfo()
    {
        if (ShouldValidateStateOnProcessExit)
        {
            // If we have a log at this point, it means that either:
            // 1. The test never called SaveLogToTempFile().
            // 2. The test logged something after calling SaveLogToTempFile().
            if (!ShutdownOrderLog.empty())
            {
                abort();
            }
        }
    }

    bool ShouldValidateStateOnProcessExit {false};
    std::vector<const wchar_t*> ShutdownOrderLog;
} g_shutdownOrderValidationStaticInfo;

void ShutdownOrderValidation::SaveLogToTempFile()
{
    OutputDebugStringW(L"Observed shutdown order:\n");
    for (auto str : g_shutdownOrderValidationStaticInfo.ShutdownOrderLog)
    {
        OutputDebugStringW(str);
        OutputDebugStringW(L"\n");
    }

    // Generate a filename for the log file: %temp%\shutdownlog_<pid>.txt
    wchar_t logFilePath[MAX_PATH];
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    StringCchPrintfW(logFilePath, MAX_PATH, L"%sShutdownOrderLog_%d.txt", tempPath, GetCurrentProcessId());

    // Write the log to the file
    std::wofstream logFile(logFilePath);
    if (logFile.is_open())
    {
        logFile << L"# Observed shutdown order:" << std::endl;
        for (auto str : g_shutdownOrderValidationStaticInfo.ShutdownOrderLog)
        {
            logFile << str << std::endl;
        }

        logFile.close();
    }    

    g_shutdownOrderValidationStaticInfo.ShutdownOrderLog.clear();
}

void ShutdownOrderValidation::Log(const wchar_t* message)
{
    wchar_t buffer[128]{};
    ::StringCchPrintf(buffer, ARRAYSIZE(buffer), L"[TestAppShutdownOrder] %s\n", message);
    ::OutputDebugStringW(buffer);

    g_shutdownOrderValidationStaticInfo.ShutdownOrderLog.push_back(message);
}

void ShutdownOrderValidation::ValidateStateOnProcessExit(bool enable)
{
    g_shutdownOrderValidationStaticInfo.ShouldValidateStateOnProcessExit = enable;
}