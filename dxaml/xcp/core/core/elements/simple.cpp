// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EnumValueTable.h"

//------------------------------------------------------------------------
//
//  Method:   CreateEnumerateHelper
//
//  Synopsis:
//      Called by all the enumeration Create methods to share code/implementation
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEnumerated::CreateEnumerateHelper(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate,
    _In_ XUINT32 cTable,
    _In_reads_(cTable) const XTABLE *pTable,
    _In_ KnownTypeIndex enumTypeIndex,
    _In_ bool IsFlagsEnumeration)
{
    HRESULT hr = S_OK;
    CEnumerated *_this = NULL;

    *ppObject = NULL;
    IFC(CEnumerated::Create(reinterpret_cast<CDependencyObject**>(&_this), pCreate));

    // Call the type converter

    if (pCreate->m_value.GetType() != valueAny)
    {
        if (pCreate->m_value.IsEnum())
        {
            uint32_t incomingValue = 0;
            KnownTypeIndex incomingTypeIndex = KnownTypeIndex::UnknownType;
            pCreate->m_value.GetEnum(incomingValue, incomingTypeIndex);
            IFCEXPECT(incomingTypeIndex == enumTypeIndex);
            _this->m_nValue = incomingValue;
        }
        else
        {
            IFCEXPECT(pCreate->m_value.GetType() == valueString);

            XUINT32 cString = 0;
            const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);

            if (IsFlagsEnumeration)
            {
                IFC(FlagsEnumerateFromString(cTable, pTable, cString, pString, &_this->m_nValue));
            }
            else
            {
                IFC(EnumerateFromString(cTable, pTable, cString, pString, &_this->m_nValue));
            }
        }
    }

    _this->m_enumTypeIndex = enumTypeIndex;

    // Return the object to the caller
   *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    delete _this;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:  CFontWeight::Create
//
//  Synopsis:
//      Creates an instance of a CFontWeight object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFontWeight::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    return CreateEnumerateHelper(ppObject, pCreate, ARRAY_SIZE(satFontWeight), satFontWeight, KnownTypeIndex::FontWeight);
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates a Uri.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CUri::Create(
_Outptr_ CDependencyObject **ppObject,
_In_ CREATEPARAMETERS *pCreate
)
{
    return CString::Create(ppObject, pCreate);
}

