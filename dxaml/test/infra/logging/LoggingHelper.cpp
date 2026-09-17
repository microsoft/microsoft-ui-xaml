// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LoggingHelper.h"
#include <string>
#include <utility>
#include <XamlLogging.h>

using namespace WEX::Common;
using namespace ErrorHandling;

namespace {
    std::function<void(CONTEXT)> s_stackLogger;

    enum class ExpectedLeakState
    {
        Inactive,
        Expecting,
        Detected
    };

    // Leaks on other threads must not satisfy or be hidden by the current scan.
    thread_local ExpectedLeakState s_expectedLeakState = ExpectedLeakState::Inactive;

    LPTOP_LEVEL_EXCEPTION_FILTER s_previousHandler = nullptr;
    LONG WINAPI WriteStackOnException(EXCEPTION_POINTERS* exception)
    {
        s_stackLogger(*(exception->ContextRecord));

        if (s_previousHandler != nullptr)
        {
            return s_previousHandler(exception);
        }
        else
        {
            return EXCEPTION_CONTINUE_SEARCH;
        }
    }
}

namespace Private { namespace Infrastructure {

LoggingHelper::LoggingHelper(_In_ IXamlLoggerTestHooks* testHooks)
{
    m_spStackLogger = std::make_unique<SmartStackLogger>();
    bool disableErrorHandling = false;
    WEX::Common::NoThrowString value;
    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"DisableErrorHandling", value)))
    {
        disableErrorHandling = true;
    }

    if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"RunAsBVT", value)) && !value.IsEmpty())
    {
        disableErrorHandling = false;
    }

    SetPrintStacksOnJupiterFailure(!disableErrorHandling);

    // We hook into Xaml's error handling here. This gives us an integration point
    // at which we can collect stack information and dumps and surface error conditions
    // in the test logs.
    testHooks->SetErrorHandlerCallback(std::make_shared<std::function<void(const ErrorHandling::XamlFailureInfo& failure)>>(
        std::bind(&SmartStackLogger::LogStackIfNovel, m_spStackLogger.get(), std::placeholders::_1)));

    // We also hook the final structured exception handler. This hook will get us all the
    // juicy details of typical AVs other structured exceptions outside of IFC spew and C++EH.
    WEX_ASSERT(!s_stackLogger, L"This should only be set once.");
    s_stackLogger = std::bind(&SmartStackLogger::LogStackFromContext,
        m_spStackLogger.get(), std::placeholders::_1);

    testHooks->SetLoggerCallback(std::make_shared<LoggerCallback>(
            std::bind(&LoggingHelper::LogMessage, this, std::placeholders::_1, std::placeholders::_2)));
}

void LoggingHelper::SetUnhandledExceptionFilter() const
{
    // The previous exception handler is the CRT's C++EH uncaught filter which will call
    // out to std::terminate to terminate the program.
    s_previousHandler = ::SetUnhandledExceptionFilter(WriteStackOnException);
}

bool LoggingHelper::GetPrintStacksOnJupiterFailure() const
{
    return m_spStackLogger->IsEnabled();
}

void LoggingHelper::SetPrintStacksOnJupiterFailure(bool enable)
{
    m_spStackLogger->SetIsEnabled(enable);
}

void LoggingHelper::LogMessage(const wchar_t* pMessage, ErrorHandling::LoggingLevel level)
{
    switch (level)
    {
        case LoggingLevel::Info:
            WEX::Logging::Log::Comment(pMessage);
            break;
        case LoggingLevel::Warning:
            WEX::Logging::Log::Warning(pMessage);
            break;
        case LoggingLevel::Error:
            WEX::Logging::Log::Error(pMessage);
            break;
        case LoggingLevel::Leak:
            if (s_expectedLeakState != ExpectedLeakState::Inactive)
            {
                s_expectedLeakState = ExpectedLeakState::Detected;
                WEX::Logging::Log::Comment(pMessage);
            }
            else if (m_ignoreLeaksForTest)
            {
                // If opted out of leak detection, then just display warning.
                WEX::Logging::Log::Warning(pMessage);
            }
            else
            {
                WEX::Logging::Log::Error(pMessage);
            }

            break;
    }
}

bool LoggingHelper::GetIgnoreLeaksForTest() const
{
    return m_ignoreLeaksForTest;
}

void LoggingHelper::SetIgnoreLeaksForTest(bool ignore)
{
    m_ignoreLeaksForTest = ignore;
}

void LoggingHelper::VerifyExpectedLeaks(const std::function<void()>& checkForLeaks)
{
    bool leakDetected = false;
    {
        auto previousExpectation = std::exchange(s_expectedLeakState, ExpectedLeakState::Expecting);
        auto restoreExpectation = wil::scope_exit([previousExpectation]() {
            s_expectedLeakState = previousExpectation;
        });
        checkForLeaks();
        leakDetected = s_expectedLeakState == ExpectedLeakState::Detected;
    }

    if (leakDetected)
    {
        WEX::Logging::Log::Comment(L"Detected the expected native leak.");
    }
    else
    {
        WEX::Logging::Log::Error(L"Expected a native leak, but the shutdown-time scan reported none.");
    }
}
} }