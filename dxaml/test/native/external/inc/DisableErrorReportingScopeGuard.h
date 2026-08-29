// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class DisableErrorReportingScopeGuard
        {
        public:
            DisableErrorReportingScopeGuard()
            {
                m_restoreState = test_infra::TestServices::ErrorHandlingHelper->PrintStacksOnJupiterFailure;
                if (m_restoreState)
                {
                    WEX::Logging::Log::Comment(L"Disabling error reporting.");
                    test_infra::TestServices::ErrorHandlingHelper->PrintStacksOnJupiterFailure = false;
                }
            }

            ~DisableErrorReportingScopeGuard()
            {
                if (m_restoreState)
                {
                    WEX::Logging::Log::Comment(L"Reenabling error reporting.");
                    test_infra::TestServices::ErrorHandlingHelper->PrintStacksOnJupiterFailure = m_restoreState;
                }
            }

            DisableErrorReportingScopeGuard(const DisableErrorReportingScopeGuard&) = delete;
            DisableErrorReportingScopeGuard& operator=(const DisableErrorReportingScopeGuard&) = delete;
        private:
            bool m_restoreState = false;
        };
    }
} } } }