// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif
#include "Animation.h"
#include "theming\inc\Theme.h"

CAnimation::~CAnimation()
{
}

_Check_return_ HRESULT CAnimation::SetValue(_In_ const SetValueParams& args)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::GetValue(_In_ const CDependencyProperty *pdp, _Inout_ CValue *pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::UpdateAnimation(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    XDOUBLE beginTime,
    DirectUI::DurationType durationType,
    XFLOAT durationValue,
    _In_ const COptionalDouble &expirationTime,
    _Out_ bool *pIsIndependentAnimation
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::OnBegin()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::FireCompletedEvent()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::FinalizeIteration()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::ResolveLocalTarget(_In_ CCoreServices *pCoreServices, _In_opt_ CTimeline *pParentTimeline)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::FindIndependentAnimationTargets(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::FindIndependentAnimationTargetsRecursive(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CAnimation::DoAnimationValueOperation(
    _In_ CValue& value,
    AnimationValueOperation operation,
    bool isValueChangeIndependent
    )
{
    return S_OK;
}