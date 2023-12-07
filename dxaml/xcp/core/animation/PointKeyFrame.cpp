// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PointKeyFrame.h"
#include "KeySpline.h"
#include "EasingFunctions.h"

CSplinePointKeyFrame::~CSplinePointKeyFrame()
{
    ReleaseInterface(m_pKeySpline);
}

_Check_return_ HRESULT CSplinePointKeyFrame::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    xref_ptr<CSplinePointKeyFrame> pPointKeyFrame;
    pPointKeyFrame.attach(new CSplinePointKeyFrame(pCreate->m_pCore));

    IFC_RETURN(CKeySpline::Create(reinterpret_cast<CDependencyObject **>(&pPointKeyFrame->m_pKeySpline), pCreate));

    *ppObject = pPointKeyFrame.detach();

    return S_OK;
}

// Computes effective progress of the current keyframe versus a prior time value.
// The keyframe must be initalized with percentage for this to work.
float CSplinePointKeyFrame::GetEffectiveProgress(float rCurrentProgress)
{
    if (m_pKeySpline != nullptr)
    {
        return m_pKeySpline->GetSplineProgress(rCurrentProgress);
    }
    else
    {
        return 0;
    }
}

CEasingPointKeyFrame::~CEasingPointKeyFrame()
{
    ReleaseInterface(m_pEasingFunction);
}

// Computes effective progress of the current keyframe versus a prior time value.
float CEasingPointKeyFrame::GetEffectiveProgress(float rCurrentProgress)
{
    return CEasingFunctionBase::EaseValue(m_pEasingFunction, rCurrentProgress);
}
