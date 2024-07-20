// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ColorKeyFrame.h"
#include "KeySpline.h"
#include "EasingFunctions.h"

CSplineColorKeyFrame::~CSplineColorKeyFrame()
{
    ReleaseInterface(m_pKeySpline);
}

_Check_return_ HRESULT CSplineColorKeyFrame::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    xref_ptr<CSplineColorKeyFrame> pColorKeyFrame;
    pColorKeyFrame.attach(new CSplineColorKeyFrame(pCreate->m_pCore));

    IFC_RETURN(CKeySpline::Create(reinterpret_cast<CDependencyObject **>(&pColorKeyFrame->m_pKeySpline), pCreate));

    *ppObject = pColorKeyFrame.detach();

    return S_OK;
}

CEasingColorKeyFrame::~CEasingColorKeyFrame()
{
    ReleaseInterface(m_pEasingFunction);
}
