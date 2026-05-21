// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "ParallelTimeline.h"


KnownTypeIndex CParallelTimeline::GetTypeIndex() const
{
    return KnownTypeIndex::ParallelTimeline;
}

HRESULT CParallelTimeline::FireCompletedEvent()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CParallelTimeline::UpdateAnimation(
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
