// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>
#include <CValue.h>
#include <CDependencyObject.h>
#include <DeferredAnimationOperation.h>
#include <MetadataAPI.h>

CDeferredAnimationOperation::CDeferredAnimationOperation(
    _In_ std::pair<xref_ptr<CDependencyObject>, const KnownPropertyIndex> target,
    _In_ CValue& value,
    _In_ bool targetHasPeggedPeer,
    _In_ DeferredOperation operation,
    _In_ xref_ptr<CDependencyObject> sourceSetter)
    : m_target(std::move(target))
    , m_targetHasPeggedPeer(targetHasPeggedPeer)
    , m_operation(operation)
    , m_sourceSetter(std::move(sourceSetter))
{
    if (!value.IsUnset())
    {
        VERIFYHR(m_vValue.CopyConverted(value));
    }
}

CDeferredAnimationOperation::~CDeferredAnimationOperation()
{
    if (m_target.first && m_targetHasPeggedPeer)
    {
        m_target.first->UnpegManagedPeer();
    }
}

_Check_return_ HRESULT CDeferredAnimationOperation::Execute()
{
    switch (m_operation)
    {
        case DeferredOperation::Set:
        {
            IFC_RETURN(SetAnimatedValue());
            break;
        }
        case DeferredOperation::Clear:
        {
            IFC_RETURN(ClearAnimatedValue());
            break;
        }
        default:
        {
            ASSERT(false);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDeferredAnimationOperation::SetAnimatedValue()
{
    // This can cause a synchronous callout to user code.
    IFC_RETURN(m_target.first->SetAnimatedValue(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(m_target.second), m_vValue, m_sourceSetter));

    return S_OK;
}

_Check_return_ HRESULT CDeferredAnimationOperation::ClearAnimatedValue()
{
    // This can cause a synchronous callout to user code.
    IFC_RETURN(m_target.first->ClearAnimatedValue(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(m_target.second), m_vValue));

    return S_OK;
}