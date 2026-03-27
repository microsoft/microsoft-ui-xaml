// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LoggingHelper.h"
#include "IXamlTestHooks-win.h"

namespace Private { namespace Infrastructure {

    class ErrorHandlingHelper : public Microsoft::WRL::RuntimeClass<test_infra::IErrorHandlingHelper>
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_ErrorHandlingHelper, TrustLevel::BaseTrust);

    public:
        ErrorHandlingHelper() = default;

        IFACEMETHOD(RuntimeClassInitialize)();
        
        IFACEMETHOD(get_PrintStacksOnJupiterFailure)(BOOLEAN* pEnable);
        IFACEMETHOD(put_PrintStacksOnJupiterFailure)(BOOLEAN enable);
        IFACEMETHOD(IgnoreLeaksForTest)();

        // Starts the leak detection algorithm, Logs an Error to TAEF if a leak is detected.
        static void PerformLeakDetection();
        static bool ShouldIgnoreLeaks();
        static void TrackLeaksForTest();

    private:
        void LogMessage(const wchar_t* pMessage, ErrorHandling::LoggingLevel level);

    private:
        std::unique_ptr<LoggingHelper> m_spStackLogger;
        bool m_printStacksOnFailure = true;
        bool m_ignoreLeaksForTest  = false;
    };

} }