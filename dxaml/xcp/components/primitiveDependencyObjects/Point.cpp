// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <Point.h>
#include <DOPointerCast.h>

_Check_return_ HRESULT CPoint::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    CPoint* theClone = new CPoint(pCreate->m_pCore);

    // We could be cloning a point or creating a default point.

    if (pCreate->m_value.GetType() == valueObject)
    {
        CPoint* point = checked_cast<CPoint>(pCreate->m_value);

        if (point)
        {
            theClone->m_pt = point->m_pt;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        // Call the type converter
        XUINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(ArrayFromString(cString, pString, &cString, &pString, 2, (XFLOAT *) &theClone->m_pt));
    }
    else if (pCreate->m_value.GetType() == valuePoint)
    {
        XPOINTF* point = pCreate->m_value.AsPoint();
        IFCEXPECT(point != nullptr);
        theClone->m_pt = *point;
    }
    else
    {
        // Create will work, but is not written with expectation of other valueTypes
        ASSERT(pCreate->m_value.GetType() == valueAny);
    }

    // Return the object to the caller

   *ppObject = static_cast<CDependencyObject *>(theClone);
    theClone = NULL;

Cleanup:
    ReleaseInterface(theClone);
    return hr;
}

KnownTypeIndex CPoint::GetTypeIndex() const
{
    return DependencyObjectTraits<CPoint>::Index;
}
