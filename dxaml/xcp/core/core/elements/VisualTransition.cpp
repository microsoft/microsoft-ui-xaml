// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <Storyboard.h>
#include "DurationVO.h"
#include <VisualTransitionCompletedData.h>
#include <EnumDefs.g.h>

CVisualTransition::~CVisualTransition()
{
    ReleaseInterface(m_pStoryboard);
    ReleaseInterface(m_pEasingFunction);
}

_Check_return_ HRESULT CVisualTransition::InitInstance()
{
    auto core = GetContext();
    m_duration = DurationVOHelper::Create(core);
    return S_OK;
}

#pragma region Legacy VSM Code
// None of this code is used in XBFv2-based VSM implementations. It is considered dead code
// only kept for strict back-compat reasons. No VSM written with XBFv2 will use these code paths,
// only VSMs created using text parsing or from XBFv1 will use this code.
bool CVisualTransition::GetIsDefault()
{
    //the transition is default if it does not contain both To and From value
    return (m_strFrom.IsNullOrEmpty() && m_strTo.IsNullOrEmpty());
}

//-------------------------------------------------------------------------
//
//  Function:   RegisterName()
//
//  Synopsis:   Called when registering names.
//              SL3 did not register names on visualstates/transitions in all scenario's
//              because it could not walk up. We will be bug compat here.
//              The following code will only allow a register when the register
//              is triggered by an enter walk a few levels above this element
//              (to be exact, above the visualstategroupcollection).
//-------------------------------------------------------------------------
_Check_return_
    HRESULT
    CVisualTransition::RegisterName( _In_ CDependencyObject *pNamescopeOwner, _In_ XUINT32 bTemplateNamescope)
{
    CVisualTransitionCollection *pParentCollection = do_pointer_cast<CVisualTransitionCollection>(GetParentInternal(false));
    bool bParentCollectionInEnterOrLeave = pParentCollection && pParentCollection->IsProcessingEnterLeave();

    if (bParentCollectionInEnterOrLeave || bTemplateNamescope)
    {
        IFC_RETURN(CDependencyObject::RegisterName(pNamescopeOwner, bTemplateNamescope));
    }
    return S_OK;
}

bool CVisualTransition::IsZeroDuration()
{
    bool visualTransitionZeroDuration = !m_duration || m_duration->Value().GetTimeSpanInSec() == 0.0;
    bool storyboardNullOrZeroDuration = true;
    if (m_pStoryboard)
    {
        XFLOAT rDuration;
        DirectUI::DurationType durationType;
        // We use the GetDuration method to ensure that we take into account offshifted
        // start times.
        THROW_IF_FAILED(m_pStoryboard->GetDuration(&durationType, &rDuration));
        storyboardNullOrZeroDuration = (rDuration == 0.0f);
    }
    return visualTransitionZeroDuration && storyboardNullOrZeroDuration;
}
#pragma endregion

