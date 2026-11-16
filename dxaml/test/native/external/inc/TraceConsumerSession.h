// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "assert.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class TraceConsumerSession
    {
    public:
        TraceConsumerSession()
        {
            test_infra::TraceConsumer::Start();
            m_tracing = true;
        }

        TraceConsumerSession(GUID xamlProvider)
        {
            test_infra::TraceConsumer::Start(xamlProvider);
            m_tracing = true;
        }
        ~TraceConsumerSession()
        {
            if (m_tracing)
            {
                m_tracing = false;
                // A destructor mustn't throw, and this one can run while another exception unwinds the stack. Stop
                // logs its failures as errors, so the test still fails if it throws here.
                try
                {
                    test_infra::TraceConsumer::Stop();
                }
                catch (...)
                {
                }
            }
        }

        void Stop()
        {
            assert(m_tracing);
            // Clear the flag first, so the destructor doesn't run Stop again if this Stop throws.
            m_tracing = false;
            test_infra::TraceConsumer::Stop();
        }

        TraceConsumerSession(const TraceConsumerSession&) = delete;
        TraceConsumerSession& operator=(const TraceConsumerSession&) = delete;

    private:
        bool m_tracing;
    };

}}}}}