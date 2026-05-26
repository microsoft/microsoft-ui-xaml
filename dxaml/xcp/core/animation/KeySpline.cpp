// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeySpline.h"
#include <TrimWhitespace.h>
#include <StringConversions.h>

_Check_return_ HRESULT CKeySpline::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;
    CKeySpline *pKeySpline = new CKeySpline(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueString)
    {
        XUINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(pKeySpline->InitFromString(
            cString,
            pString
            ));
    }

// validate range
    IFC(IsControlPointValid(&pKeySpline->m_ControlPoint1));
    IFC(IsControlPointValid(&pKeySpline->m_ControlPoint2));

    *ppObject = static_cast<CDependencyObject *>(pKeySpline);
    pKeySpline = NULL;

Cleanup:
    delete pKeySpline;
    RRETURN(hr);
}

_Check_return_ HRESULT CKeySpline::InitFromString(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR *pString
    )
{
    if (cString == 0 || pString == NULL)
    {
        return S_OK;
    }

// Skip whitespace at the start of the string
    while (cString && xisspace(*pString))
    {
        pString++;
        cString--;
    }

    // Parse first control point (passing FALSE to ArrayFromString cause we shouldn't consume
    // the full string)
    IFC_RETURN(ArrayFromString(cString, pString, &cString, &pString, 2, (XFLOAT *) &m_ControlPoint1, FALSE));

// Look for a potential comma between the two points
    while (cString && xisspace(*pString))
    {
        pString++;
        cString--;
    }
    if (cString && *pString == ',')
    {
        pString++;
        cString--;
    }

// Parse second control point
    IFC_RETURN(ArrayFromString(cString, pString, &cString, &pString, 2, (XFLOAT *) &m_ControlPoint2));

// Skip whitespace at the end of the string
    while (cString && xisspace(*pString))
    {
        pString++;
        cString--;
    }

// If we have any characters left, it's an error
    if (cString)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

// Activate side-effects of a given property.
_Check_return_ HRESULT CKeySpline::InvokeImpl(
    _In_ const CDependencyProperty *pdp,
    _In_opt_ CDependencyObject *pNamescopeOwner
    )
{
    IFC_RETURN(CDependencyObject::InvokeImpl(pdp, pNamescopeOwner));

    if (pdp)
    {
        switch (pdp->GetIndex())
        {
        case KnownPropertyIndex::KeySpline_ControlPoint1:
            IFC_RETURN(IsControlPointValid(&m_ControlPoint1));
            break;

        case KnownPropertyIndex::KeySpline_ControlPoint2:
            IFC_RETURN(IsControlPointValid(&m_ControlPoint2));
            break;

        default:
            break;
        }
    }

    return S_OK;
}

void CKeySpline::CopyKeySplineProperties(_Inout_ CKeySpline *pClone) const
{
    pClone->m_ControlPoint1.x = m_ControlPoint1.x;
    pClone->m_ControlPoint1.y = m_ControlPoint1.y;
    pClone->m_ControlPoint2.x = m_ControlPoint2.x;
    pClone->m_ControlPoint2.y = m_ControlPoint2.y;
    pClone->m_rLastT = m_rLastT;
}

