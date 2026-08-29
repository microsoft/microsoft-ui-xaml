// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{

    PARTIAL_CLASS(XamlShutdownCompletedOnThreadEventArgs)
    {
    public:

        _Check_return_ HRESULT GetDispatcherQueueDeferralImpl(_Outptr_ ABI::Windows::Foundation::IDeferral** ppReturnValue)
        {
            IFC_RETURN(m_dispatcherQueueShutdownStartingEventArgs->GetDeferral(ppReturnValue));
            return S_OK;
        }

        void SetDispatcherQueueShutdownStartingEventArgs(_In_ msy::IDispatcherQueueShutdownStartingEventArgs* args)
        {
            m_dispatcherQueueShutdownStartingEventArgs = args;
        }

    private:
        wrl::ComPtr<msy::IDispatcherQueueShutdownStartingEventArgs> m_dispatcherQueueShutdownStartingEventArgs;
    };
}
