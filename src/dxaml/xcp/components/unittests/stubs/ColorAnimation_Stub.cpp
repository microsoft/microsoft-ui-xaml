// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

// Needed for private DComp interfaces
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINTHRESHOLD

#include "ColorAnimation.h"


void CColorAnimation::InterpolateCurrentValue(XFLOAT rPercentEnd)
{
}

void CColorAnimation::ComputeToUsingFromAndBy()
{
}

_Check_return_ HRESULT CColorAnimation::GetAnimationBaseValue()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CColorAnimation::FindIndependentAnimationTargetsRecursive(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    return E_NOTIMPL;
}
