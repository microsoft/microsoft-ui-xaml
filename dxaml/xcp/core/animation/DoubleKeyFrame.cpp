// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DoubleKeyFrame.h"
#include "KeySpline.h"
#include "EasingFunctions.h"

CSplineDoubleKeyFrame::~CSplineDoubleKeyFrame()
{
    ReleaseInterface(m_pKeySpline);
}

_Check_return_ HRESULT CSplineDoubleKeyFrame::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    xref_ptr<CSplineDoubleKeyFrame> pDoubleKeyFrame;
    pDoubleKeyFrame.attach(new CSplineDoubleKeyFrame(pCreate->m_pCore));

    IFC_RETURN(CKeySpline::Create(reinterpret_cast<CDependencyObject **>(&pDoubleKeyFrame->m_pKeySpline), pCreate));

    *ppObject = pDoubleKeyFrame.detach();

    return S_OK;
}

CEasingDoubleKeyFrame::~CEasingDoubleKeyFrame()
{
    ReleaseInterface(m_pEasingFunction);
}
