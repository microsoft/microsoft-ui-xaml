// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <CreateParameters.h>
#include <Double.h>
#include <DOPointerCast.h>

_Check_return_ HRESULT CDouble::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    CDouble *_this = new CDouble(pCreate->m_pCore);

    // If we're cloning a double then create a new one and copy its value.

    if (pCreate->m_value.GetType() == valueObject)
    {
        CDouble* doubleValue = checked_cast<CDouble>(pCreate->m_value);

        if (doubleValue)
        {
            _this->m_eValue = doubleValue->m_eValue;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        CValue value;

        IFC(CreateCValue(pCreate, value));
        _this->m_eValue = value.AsFloat();
    }
    else if (pCreate->m_value.GetType() == valueFloat)
    {
        _this->m_eValue = pCreate->m_value.AsFloat();
    }
    else if (pCreate->m_value.GetType() == valueDouble)
    {
        _this->m_eValue = static_cast<float>(pCreate->m_value.AsDouble());
    }

    // Return the object to the caller

   *ppObject = static_cast<CDependencyObject *>(_this);
    _this = nullptr;

Cleanup:
    ReleaseInterface(_this);
    return hr;
}

KnownTypeIndex CDouble::GetTypeIndex() const
{
    return DependencyObjectTraits<CDouble>::Index;
}

_Check_return_ HRESULT CLengthConverter::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    if (pCreate->m_value.GetType() == valueString)
    {
        CValue value;
        IFC_RETURN(CLengthConverter::CreateCValue(pCreate, value));
        CREATEPARAMETERS cp(pCreate->m_pCore, value);
        return CDouble::Create(ppObject, &cp);
    }
    else
    {
        return CDouble::Create(ppObject, pCreate);
    }

}
