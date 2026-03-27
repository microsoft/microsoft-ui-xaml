// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SmartStackLogger.h"
#include "IXamlTestHooks-log.h"

namespace Private { namespace Infrastructure {

    class LoggingHelper sealed
    {

    public:
        LoggingHelper(_In_ IXamlLoggerTestHooks* loggerHooks);

        void SetUnhandledExceptionFilter() const;
        bool GetPrintStacksOnJupiterFailure() const;
        void SetPrintStacksOnJupiterFailure(bool enable);
        bool GetIgnoreLeaksForTest() const;
        void SetIgnoreLeaksForTest(bool ignore);
    private:
        void LogMessage(const wchar_t* pMessage, ErrorHandling::LoggingLevel level);

    private:
        std::unique_ptr<SmartStackLogger> m_spStackLogger;
        bool m_ignoreLeaksForTest   = false;
        IXamlLoggerTestHooks* m_testHooks = nullptr;
    };

} }