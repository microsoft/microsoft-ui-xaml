// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "DragOperationDeferral.g.h"
#include "IDragOperationDeferralTarget.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DragOperationDeferral)
    {
    public:
        DragOperationDeferral() :
            m_isComplete(false)
        {}

        ~DragOperationDeferral() override;

        _Check_return_ HRESULT Initialize(_In_ IDragOperationDeferralTarget* pOwner);
        _Check_return_ HRESULT CompleteImpl();
    
        void SetDragEventArgs(_In_ xaml::IDragEventArgs* const pEventArgs);

    private:
        TrackerPtr<IDragOperationDeferralTarget> m_tpOwner;
        TrackerPtr<xaml::IDragEventArgs> m_tpEventArgs;
        bool m_isComplete;
    };
}
