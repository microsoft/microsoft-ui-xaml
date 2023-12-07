// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <Rect.h>
#include <DOPointerCast.h>

_Check_return_ HRESULT CRect::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    CRect *_this = new CRect(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueObject)
    {
        CRect* rect = checked_cast<CRect>(pCreate->m_value);

        if (rect)
        {
            _this->m_rc = rect->m_rc;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        // Call the type converter
        XUINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(ArrayFromString(cString, pString, &cString, &pString, 4, (XFLOAT *) &_this->m_rc));
    }
    else if (pCreate->m_value.GetType() == valueRect)
    {
        _this->m_rc = *pCreate->m_value.AsRect();
    }
    else
    {
        // Create currently only supports valueObject, valueString and the default
        ASSERT(pCreate->m_value.GetType() == valueAny);
    }

    // Validate that the radii are non-negative (new parser only)
    // NB: MAKE SURE ISNANF COMES FIRST!  Normally, testing NaN < 0 is OK to do,
    // but DLLs can enable floating-point exceptions in their code in a way that
    // affects the whole process, even when the NaN is non-signaling. To avoid that,
    // we'll test for NaN first by doing a comparison here that doesn't raise
    // a floating-point exception.
    if ((!IsNanF(_this->m_rc.Height) && 0.0f > _this->m_rc.Height) ||
        (!IsNanF(_this->m_rc.Width) && 0.0f > _this->m_rc.Width))
    {
        // Negative values
        IFC(E_INVALIDARG);
    }

    // Return the object to the caller

   *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    return hr;
}

KnownTypeIndex CRect::GetTypeIndex() const
{
    return DependencyObjectTraits<CRect>::Index;
}
