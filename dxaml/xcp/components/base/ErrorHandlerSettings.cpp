// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DependencyLocator.h>
#include <ErrorHandlerSettings.h>
#include <XcpAllocationDebug.h>

namespace ErrorHandling
{
    std::shared_ptr<ErrorHandlingSettings> GetErrorHandlingSettings()
    {
        static PROVIDE_DEPENDENCY(ErrorHandlingSettings, DependencyLocator::StoragePolicyFlags::None);
        static DependencyLocator::Dependency<ErrorHandling::ErrorHandlingSettings> s_errorHandlingSettings;
        
#if XCP_MONITOR
        auto leakIgnorer = XcpDebugStartIgnoringLeaks();
#endif
        return s_errorHandlingSettings.Get();
    }

    void ResultLoggingCallback(
        _Inout_ wil::FailureInfo *pFailure,
        _Inout_updates_opt_z_(cchDebugMessage) PWSTR pszDebugMessage,
        _Pre_satisfies_(cchDebugMessage > 0) size_t cchDebugMessage) WI_NOEXCEPT
    {
        // The new TAEF dev test suite registers for this callback to perform
        // its own stack capature and dump creation.
        static auto errorHandlerSettings = GetErrorHandlingSettings();
        if (errorHandlerSettings)
        {
            ErrorHandling::XamlFailureInfo failure = {};
            failure.pFailureInfo = pFailure;
            errorHandlerSettings->ExecuteErrorHandlingCallbackIfSet(failure);
        }
    }

    void ErrorHandlingSettings::SetErrorHandlingCallback(_In_ std::function<void(const ErrorHandling::XamlFailureInfo&)> func)
    {
        m_errorHandlingCallback = std::move(func);

        // We use this extensibility point in the Windows Internal Library's logging
        // mechanism to capture stacks at THROW_IF_FAILED blocks and CATCH_RETURN
        // blocks.
        if (!wil::g_pfnResultLoggingCallback)
        {
            wil::g_pfnResultLoggingCallback = ResultLoggingCallback;
        }
    }

    void ErrorHandlingSettings::SetLoggerCallback(_In_ LoggerCallback callback)
    {
        m_loggerCallback = std::move(callback);
    }

    void ErrorHandlingSettings::ExecuteErrorHandlingCallbackIfSet(const ErrorHandling::XamlFailureInfo& failure) const
    {
        if (m_errorHandlingCallback)
        {
            m_errorHandlingCallback(failure);
        }
    }

    void ErrorHandlingSettings::LogMessage(const wchar_t* pMessage, LoggingLevel level) const
    {
        if (m_loggerCallback)
        {
            m_loggerCallback(pMessage, level);
        }
    }
}
