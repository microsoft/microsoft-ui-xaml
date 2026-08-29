// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <TypeTableStructs.h>
#include <primitives.h>
#include <dopointercast.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <MarkupExtension.h>
#include <EnumValueTable.h>
#include <StringConversions.h>

// Constructor to the base enumerated type converter.
_Check_return_ HRESULT CEnumerated::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    *ppObject = NULL;

    CEnumerated *_this = new CEnumerated(pCreate->m_pCore);

    switch (pCreate->m_value.GetType())
    {
        case valueAny:
            // enum created without initialization value, as would be the case with
            // <Visibility />
            //
            // *n.b.: this does assume that the enum has a 0 default value.  if this
            // is not the case for a particular enum, then this would have to be
            // handled explicitly in that enum's Create() function.
            _this->m_nValue = 0;
            break;

        case valueBool:
            _this->m_nValue = pCreate->m_value.AsBool();
            break;

        case valueEnum:
        case valueEnum8:
            _this->m_nValue = pCreate->m_value.AsEnum();
            break;

        case valueObject:
            CDependencyObject* valueDO = pCreate->m_value.AsObject();

            if (valueDO &&
                valueDO->GetClassInformation()->IsEnum())
            {
                CEnumerated* pEnum = NULL;
                IFC(DoPointerCast(pEnum, valueDO));
                _this->m_nValue = pEnum->m_nValue;
                _this->m_enumTypeIndex = pEnum->m_enumTypeIndex;
            }
            break;
    }

    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;
    hr = S_OK;

Cleanup:
    delete _this;
    return hr;
}

KnownTypeIndex CEnumerated::GetTypeIndex() const
{
    return DependencyObjectTraits<CEnumerated>::Index;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of a boolean object
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CBoolean::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;
    CEnumerated *_this = NULL;

    *ppObject = NULL;

    IFC(CEnumerated::Create(reinterpret_cast<CDependencyObject**>(&_this), pCreate));

    if (pCreate->m_value.GetType() == valueBool)
    {
        _this->m_nValue = pCreate->m_value.AsBool();
    }
    else if (pCreate->m_value.GetType() != valueAny)
    {
        CValue value;
        IFC(CreateCValue(pCreate, value));
        _this->m_nValue = value.AsBool();
    }

    _this->m_enumTypeIndex = KnownTypeIndex::Boolean;

    // Return the object to the caller
    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of a int32 object
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CInt32::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    CInt32 *_this = new CInt32(pCreate->m_pCore);

    // If we're cloning a int32 then create a new one and copy its value.

    if (pCreate->m_value.GetType() == valueObject)
    {
        CInt32* intValue = checked_cast<CInt32>(pCreate->m_value);

        if (intValue)
        {
            _this->m_iValue = intValue->m_iValue;
        }
    }
    else if (pCreate->m_value.GetType() == valueString)
    {
        // Call the type converter
        UINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(SignedFromDecimalString(cString, pString, &cString, &pString, &_this->m_iValue));
    }
    else if (pCreate->m_value.GetType() == valueSigned)
    {
        _this->m_iValue = pCreate->m_value.AsSigned();
    }
    else
    {
        // Caller should only be sending in object, string, signed or default
        ASSERT(pCreate->m_value.GetType() == valueAny);
    }

    // Return the object to the caller

    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    return hr;
}

KnownTypeIndex CInt32::GetTypeIndex() const
{
    return DependencyObjectTraits<CInt32>::Index;
}

KnownTypeIndex CMarkupExtensionBase::GetTypeIndex() const
{
    return DependencyObjectTraits<CMarkupExtensionBase>::Index;
}
