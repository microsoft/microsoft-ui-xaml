// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlTailored.h>
#include "TraceConsumer.h"
#include "Utilities.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;

namespace Private { namespace Infrastructure {
    HRESULT TraceConsumerStatics::StartProvider(GUID xamlProvider)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcTraceConsumerStartProvider(xamlProvider));
        }
        COM_END
    }
    HRESULT TraceConsumerStatics::Start()
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcTraceConsumerStart());
        }
        COM_END
    }

    HRESULT TraceConsumerStatics::Stop()
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcTraceConsumerStop());
        }
        COM_END
    }

    HRESULT TraceConsumerStatics::VerifyEventTraced(_In_ HSTRING _event, _In_ UINT count)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifyEventTraced(WindowsGetStringRawBuffer(_event, NULL), count));
        }
        COM_END
    }
    HRESULT TraceConsumerStatics::VerifyEventTracedById(_In_ int eventId, _In_ UINT count)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifyEventTracedById(eventId, count));
        }
        COM_END
    }

    HRESULT TraceConsumerStatics::VerifyEventTracedMoreThanOnce(_In_ int eventId)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcVerifyEventTracedMoreThanOnce(eventId));
        }
        COM_END
    }
    HRESULT TraceConsumerStatics::EnableTracingByEventId(_In_ int eventId)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcEnableTracingByEventId(eventId));
        }
        COM_END
    }
} }
