// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "EtwEventRecord.h"
#include "EventProcessor.h"
#include "EtwRule.h"
#include <TestEvent.h>
#include "NamespaceAliases.h"
#include "wil\resource.h"
#include "RuleTester.h"
#include "pathcch.h"

static EVENT_RECORD CreateEventRecord(LONGLONG time, GUID provider, USHORT id, UCHAR version)
{
    EVENT_RECORD eventRecord = { 0 };
    eventRecord.EventHeader.TimeStamp.QuadPart = time;
    eventRecord.EventHeader.ProviderId = provider;
    eventRecord.EventHeader.EventDescriptor.Id = id;
    eventRecord.EventHeader.EventDescriptor.Version = version;
    eventRecord.EventHeader.Size = sizeof(EVENT_RECORD);
    return eventRecord;
}

static AppAnalysis::Test::MockEtwEvent CreateMockEvent(USHORT ID, GUID provider, ULONGLONG time, BYTE version)
{
    AppAnalysis::Test::MockEtwEvent mockEvent = { 0 };
    mockEvent.Id = ID;
    mockEvent.Version = version;
    mockEvent.Provider = provider;
    mockEvent.Time = time;
    return mockEvent;
}

static std::wstring GetPathToEtl(_In_ PCWSTR relativePathToEtl)
{
    WEX::Common::String deploymentDir;
    LogThrow_IfFailed(WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDir));
    size_t dirLength = static_cast<size_t>(deploymentDir.GetLength());
    std::wstring path;
    path.reserve(dirLength + wcslen(relativePathToEtl) + 1);
    LogThrow_IfFailed(::PathCchCombine(&path[0], path.capacity(), deploymentDir, relativePathToEtl));
    return path;
}

static void WaitForDebugger()
{
    WEX::Common::String value;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"WaitForDebugger", value)))
    {
        while (!IsDebuggerPresent())
        {
            Sleep(1000);
            WEX::Logging::Log::Comment(L"Waiting for a debugger to attach.");
        }

        DebugBreak();
    }
}
