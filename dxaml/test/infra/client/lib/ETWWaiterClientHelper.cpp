// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlTailored.h>
#include "ETWWaiterClientHelper.h"
#include "Utilities.h"

using namespace WEX::Common;
using namespace WEX::TestExecution;

namespace Private { namespace Infrastructure {

// Simple test ETW utility class using TAEF ETWWaiter functionality

    HRESULT ETWWaiterHelperStatics::Start(GUID providerGuid, unsigned long eventId)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterStart(providerGuid, eventId));
        }
        COM_END
    }
    HRESULT ETWWaiterHelperStatics::StartWithTaskName(GUID providerGuid, HSTRING taskName)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterStartWithTaskName(providerGuid, WindowsGetStringRawBuffer(taskName, NULL)));
        }
        COM_END
    }

    HRESULT ETWWaiterHelperStatics::StartWithPayload(GUID providerGuid, unsigned long eventId, HSTRING payloadCriteria)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterStartWithPayLoad(providerGuid, eventId, WindowsGetStringRawBuffer(payloadCriteria, NULL)));
        }
        COM_END
    }

    HRESULT ETWWaiterHelperStatics::Wait(GUID providerGuid, unsigned long eventId, unsigned int timeoutMs)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterWait(providerGuid, eventId, timeoutMs));
        }
        COM_END
    }

    HRESULT ETWWaiterHelperStatics::WaitForTaskName(GUID providerGuid, HSTRING taskName, unsigned int timeout)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterWaitForTaskName(providerGuid, WindowsGetStringRawBuffer(taskName, NULL), timeout));
        }
        COM_END
    }

    HRESULT ETWWaiterHelperStatics::Stop(GUID providerGuid, unsigned long eventId)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterStop(providerGuid, eventId));
        }
        COM_END
    }

    HRESULT ETWWaiterHelperStatics::StopTaskName(GUID providerGuid, HSTRING taskName)
    {
        COM_START
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterStopTaskName(providerGuid, WindowsGetStringRawBuffer(taskName, NULL)));
        COM_END
    }

    HRESULT ETWWaiterHelperStatics::GetActiveWaiterCount(unsigned int *waiterCount)
    {
        return (ETWWaiterHelperStatics::GetActiveWaiterCountStatic(waiterCount));
    }

    HRESULT ETWWaiterHelperStatics::GetActiveWaiterCountStatic(unsigned int *waiterCount)
    {
        COM_START
        {
            RpcClientEnsureConnected();
            LogThrow_IfFailed(RpcETWWaiterCount(waiterCount));
        }
        COM_END
    }
} }
