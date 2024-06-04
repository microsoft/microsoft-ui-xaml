// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>
#include <memory>

namespace wil {
    struct FailureInfo;
}

struct ErrorContext;

namespace ErrorHandling {

    struct XamlFailureInfo
    {
        wil::FailureInfo* pFailureInfo = nullptr;
        ErrorContext* pErrorContext = nullptr;
    };

    enum class LoggingLevel
    {
        Info,
        Warning,
        Error,
        Leak
    };
    typedef std::function<void(const wchar_t*, LoggingLevel)> LoggerCallback;

    class __declspec(uuid("b9152c6e-03b2-4d42-99d2-775bc8a779af")) ErrorHandlingSettings
    {
    public:
        ErrorHandlingSettings() = default;

        void SetErrorHandlingCallback(_In_ std::function<void(const ErrorHandling::XamlFailureInfo& failure)> func);
        void SetLoggerCallback(_In_ LoggerCallback callback);
        void ExecuteErrorHandlingCallbackIfSet(const XamlFailureInfo& failure) const;
        void LogMessage(const wchar_t* pMessage, LoggingLevel level) const;

    private:
        std::function<void(const ErrorHandling::XamlFailureInfo& failure)> m_errorHandlingCallback = nullptr;
        LoggerCallback m_loggerCallback = nullptr;
    };

    std::shared_ptr<ErrorHandlingSettings> GetErrorHandlingSettings();
};
