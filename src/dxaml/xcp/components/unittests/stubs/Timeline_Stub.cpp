// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif
#include "Timeline.h"

interface ICompositionAnimationController : public IUnknown {};


CTimeline::~CTimeline()
{
}

KnownTypeIndex CTimeline::GetTypeIndex() const
{
    return KnownTypeIndex::Timeline;
}

_Check_return_ HRESULT CTimeline::InitInstance()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::SetValue(_In_ const SetValueParams& args)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::RemoveEventListener(_In_ EventHandle hEvent, _In_ CValue *pValue)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::OnManagedPeerCreated(XUINT32 fIsCustomDOType, XUINT32 fIsGCRoot)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::OnAddToTimeManager()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::OnRemoveFromTimeManager()
{
    return E_NOTIMPL;
}

bool CTimeline::IsFinite()
{
    return false;
}

_Check_return_ HRESULT CTimeline::ComputeState(
    _In_ const ComputeStateParams &parentParams,
    _Inout_opt_ bool *pHasNoExternalReferences
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::ComputeStateImpl(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    _Inout_opt_ bool *pHasNoExternalReferences,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pHasIndependentAnimation
    )
{
    return E_NOTIMPL;
}

void CTimeline::ComputeLocalProgressAndTime(
    XDOUBLE rBeginTime,
    XDOUBLE rParentTime,
    DirectUI::DurationType durationType,
    XFLOAT rDurationValue,
    _In_ COptionalDouble *poptExpirationTime,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pIsInReverseSegment
    )
{
}

void CTimeline::GetNaturalDuration(_Out_ DirectUI::DurationType *pDurationType, _Out_ XFLOAT *prDurationValue)
{
    *pDurationType = DirectUI::DurationType::Automatic;
    *prDurationValue = 0.0f;
}

_Check_return_ HRESULT CTimeline::FinalizeIteration()
{
    return E_NOTIMPL;
}

HRESULT CTimeline::FireCompletedEvent()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimeline::ResolveLocalTarget(_In_ CCoreServices *pCoreServices, _In_opt_ CTimeline *pParentTimeline)
{
    return E_NOTIMPL;
}

void CTimeline::RequestAdditionalFrame()
{
}
