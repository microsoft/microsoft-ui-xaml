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
                test_infra::TraceConsumer::Stop();
            }
        }

        void Stop()
        {
            assert(m_tracing);
            test_infra::TraceConsumer::Stop();
            m_tracing = false;
        }

        TraceConsumerSession(const TraceConsumerSession&) = delete;
        TraceConsumerSession& operator=(const TraceConsumerSession&) = delete;

    private:
        bool m_tracing;
    };

}}}}}