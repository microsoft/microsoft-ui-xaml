// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    FlyoutMetadata - Storage class in DXamlcore to enforce one Flyout
//      at a time requirement.

#include "precomp.h"
#include "FlyoutMetadata.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

void
FlyoutMetadata::SetOpenFlyout(
    _In_ IFlyoutBase* pFlyout, 
    _In_ IFrameworkElement* pPlacementTarget
    )
{
    SetPtrValue(m_tpOpenFlyout, pFlyout);
    SetPtrValue(m_tpOpenFlyoutPlacementTarget, pPlacementTarget);
}

_Check_return_ HRESULT
FlyoutMetadata::GetOpenFlyout(
    _Out_ IFlyoutBase** ppFlyout, 
    _Out_opt_ IFrameworkElement** ppPlacementTarget
    )
{
    IFC_RETURN(m_tpOpenFlyout.CopyTo(ppFlyout));

    if (ppPlacementTarget)
    {
        IFC_RETURN(m_tpOpenFlyoutPlacementTarget.CopyTo(ppPlacementTarget));
    }

    return S_OK;
}

void
FlyoutMetadata::SetStagedFlyout(
    _In_ IFlyoutBase* pFlyout, 
    _In_ IFrameworkElement* pPlacementTarget
    )
{
    // We should never set a staged flyout without an already
    // open flyout.
    ASSERT(pFlyout == nullptr || m_tpOpenFlyout);

    SetPtrValue(m_tpStagedFlyout, pFlyout);
    SetPtrValue(m_tpStagedFlyoutTarget, pPlacementTarget);
}

_Check_return_ HRESULT
FlyoutMetadata::GetStagedFlyout(
    _Out_ IFlyoutBase** ppFlyout, 
    _Out_opt_ IFrameworkElement** ppPlacementTarget
    )
{
    IFC_RETURN(m_tpStagedFlyout.CopyTo(ppFlyout));

    if (ppPlacementTarget)
    {
        IFC_RETURN(m_tpStagedFlyoutTarget.CopyTo(ppPlacementTarget));
    }

    return S_OK;
}
