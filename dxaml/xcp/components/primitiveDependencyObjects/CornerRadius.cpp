// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValue.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <dopointercast.h>
#include <CornerRadius.h>
#include <StringConversions.h>

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of a ConerRadius object
//      Corner radius is a struct that carries the info of
//      corners' radius on a 4 corners shape in the order
//      topLeft, topRight, bottomRight and bottomLeft
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCornerRadius::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    ASSERT(ppObject);
    ASSERT(pCreate);

    CCornerRadius *_this = new CCornerRadius(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueObject)
    {
        const CCornerRadius* pRadius = do_pointer_cast<CCornerRadius>(pCreate->m_value);

        if (pRadius)
        {
            _this->m_cornerRadius = pRadius->m_cornerRadius;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        // Call the type converter. Must go in descending order of number of parameters
        UINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(CornerRadiusFromString(cString, pString, &_this->m_cornerRadius));
    }
    else if (pCreate->m_value.GetType() == valueCornerRadius)
    {
        _this->m_cornerRadius = *pCreate->m_value.AsCornerRadius();
    }
    else
    {
        ASSERT(pCreate->m_value.GetType() == valueAny);
    }

    // Validate that the parameters are non-negative (new parser only)
    IFC(Validate(&_this->m_cornerRadius));

    // Return the object to the caller
    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CCornerRadius Validate method
//
//  Synopsis:
//       Validates that the corner radius parameters are non-negative.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCornerRadius::Validate(
    _In_ XCORNERRADIUS * pCornerRadius)
{
    IFCPTR_RETURN(pCornerRadius);

    // NB: MAKE SURE ISNANF COMES FIRST!  Normally, testing NaN < 0 is OK to do,
    // but DLLs can enable floating-point exceptions in their code in a way that
    // affects the whole process, even when the NaN is non-signaling. To avoid that,
    // we'll test for NaN first by doing a comparison here that doesn't raise
    // a floating-point exception.
    if ((!IsNanF(pCornerRadius->bottomLeft) && pCornerRadius->bottomLeft < 0.0f)
        || (!IsNanF(pCornerRadius->bottomRight) && pCornerRadius->bottomRight < 0.0f)
        || (!IsNanF(pCornerRadius->topLeft) && pCornerRadius->topLeft < 0.0f)
        || (!IsNanF(pCornerRadius->topRight) && pCornerRadius->topRight < 0.0f))
    {
        // Negative values
        IFC_RETURN(E_INVALIDARG);
    }
    return S_OK;
}

_Check_return_ HRESULT CCornerRadius::CornerRadiusFromString(
    _In_ UINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ XCORNERRADIUS *peValue)
{
    HRESULT  hr = S_OK;

    UINT32 cScratch;
    const WCHAR* pScratch;

    // The corner radius can be specified as topLeft, topRight, bottomRight, bottomLeft
    hr = ArrayFromString(cString, pString, &cScratch, &pScratch, 4, (XFLOAT*)peValue);
    if (FAILED(hr))
    {
        // ...or just one number, which is used all around.
        IFC(ArrayFromString(cString, pString, &cScratch, &pScratch, 1, (XFLOAT*)peValue));
        peValue->topRight = peValue->bottomLeft = peValue->bottomRight = peValue->topLeft;
    }

Cleanup:
    return hr;
}

KnownTypeIndex CCornerRadius::GetTypeIndex() const
{
    return DependencyObjectTraits<CCornerRadius>::Index;
}
