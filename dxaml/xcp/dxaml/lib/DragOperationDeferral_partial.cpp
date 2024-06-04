// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragOperationDeferral.g.h"
#include "DragEventArgs_partial.h"

using namespace DirectUI;

DragOperationDeferral::~DragOperationDeferral()
{
    ASSERT(m_isComplete);
}


_Check_return_ HRESULT DragOperationDeferral::Initialize(
    _In_ IDragOperationDeferralTarget* pOwner)
{
    SetPtrValue(m_tpOwner, pOwner);
    IFC_RETURN(m_tpOwner->DeferralAdded());

    return S_OK;
}


_Check_return_ HRESULT DragOperationDeferral::CompleteImpl()
{
    if (m_isComplete)
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_DEFERRAL_COMPLETED));
    }

    m_isComplete = true;

    if (m_tpEventArgs)
    {
        wadt::DataPackageOperation acceptedOperation = wadt::DataPackageOperation::DataPackageOperation_None;
        ctl::ComPtr<IInspectable> spSource;
        IFC_RETURN(m_tpEventArgs.Cast<DragEventArgs>()->get_OriginalSource(spSource.GetAddressOf()));
        IFC_RETURN(m_tpEventArgs->get_AcceptedOperation(&acceptedOperation));
        IFC_RETURN(m_tpOwner->SetAcceptedOperation(spSource.Get(), acceptedOperation));
        // Let's free the event args
        m_tpEventArgs.Clear();
    }

    IFC_RETURN(m_tpOwner->DeferralCompleted());

    return S_OK;
}

void DragOperationDeferral::SetDragEventArgs(_In_ xaml::IDragEventArgs* const pEventArgs)
{
    SetPtrValue(m_tpEventArgs, pEventArgs);
}
