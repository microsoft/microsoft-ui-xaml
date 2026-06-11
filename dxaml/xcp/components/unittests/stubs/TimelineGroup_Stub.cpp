// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "TimelineGroup.h"


CTimelineGroup::~CTimelineGroup()
{
}

bool CTimelineGroup::HasChildren()
{
   return false;
}

_Check_return_ HRESULT CTimelineGroup::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    return E_NOTIMPL;
}

void CTimelineGroup::DetachChildren()
{
}

_Check_return_ HRESULT CTimelineGroup::SetValue(_In_ const SetValueParams& args)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimelineGroup::AddChild(_In_ CTimeline *pChild)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimelineGroup::RemoveChild(_In_ CTimeline *pChild)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimelineGroup::ComputeStateImpl(
    _In_ const ComputeStateParams &parentParams,
    _Inout_ ComputeStateParams &myParams,
    _Inout_opt_ bool *pHasNoExternalReferences,
    bool hadIndependentAnimationLastTick,
    _Out_ bool *pHasIndependentAnimation
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimelineGroup::FinalizeIteration()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimelineGroup::OnAddToTimeManager()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTimelineGroup::OnRemoveFromTimeManager()
{
    return E_NOTIMPL;
}

bool CTimelineGroup::IsFinite()
{
    return false;
}
