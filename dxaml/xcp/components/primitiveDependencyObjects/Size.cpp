// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <CreateParameters.h>
#include <StringConversions.h>
#include <Size.h>
#include <DOPointerCast.h>

_Check_return_ HRESULT CSize::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    CSize *_this = new CSize(pCreate->m_pCore);

    // We could be cloning a size or creating a default size.
    if (pCreate->m_value.GetType() == valueObject)
    {
        CSize* size = checked_cast<CSize>(pCreate->m_value);

        if (size)
        {
            _this->m_size = size->m_size;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        // Call the type converter
        XUINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(ArrayFromString(cString, pString, &cString, &pString, 2, (XFLOAT *) &_this->m_size));
    }
    else if (pCreate->m_value.GetType() == valueSize)
    {
        _this->m_size = *pCreate->m_value.AsSize();
    }
    else
    {
        // Create will work, but is not written with expectation of other valueTypes
        ASSERT(pCreate->m_value.GetType() == valueAny);
    }

    IFC(Validate(&_this->m_size));

    // Return the object to the caller
    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    return hr;
}

KnownTypeIndex CSize::GetTypeIndex() const
{
    return DependencyObjectTraits<CSize>::Index;
}

_Check_return_ HRESULT CSize::Validate(
    _In_ XSIZEF * pSize)
{
    IFCPTR_RETURN(pSize);

    // NB: MAKE SURE ISNANF COMES FIRST!  Normally, testing NaN < 0 is OK to do,
    // but DLLs can enable floating-point exceptions in their code in a way that
    // affects the whole process, even when the NaN is non-signaling. To avoid that,
    // we'll test for NaN first by doing a comparison here that doesn't raise
    // a floating-point exception.
    if ((!IsNanF(pSize->width) && pSize->width < 0.0f) ||
        (!IsNanF(pSize->height) && pSize->height < 0.0f))
    {
        // Negative values
        IFC_RETURN(E_INVALIDARG);
    }
    return S_OK;
}
