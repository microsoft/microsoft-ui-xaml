// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// GridLength, GridUnitType for specifying the row height and column width of a Grid

#include "precomp.h"
#include <StringConversions.h>

CGridLength::CGridLength(_In_ CCoreServices *pCore)
    : CDependencyObject(pCore)
{
    m_gridLength.type = DirectUI::GridUnitType::Auto;
    m_gridLength.value = XGRIDLENGTH::Default();
}


_Check_return_ HRESULT CGridLength::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;

    CGridLength* _this = new CGridLength(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueString) 
    {
        IFC(_this->FromString(pCreate->m_value.AsString()));
    }
    else if(pCreate->m_value.GetType() == valueGridLength)
    {
        IFC(_this->FromValueGridLength(pCreate));
    }
    IFC(Validate(_this->m_gridLength));

   *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

    
Cleanup:
    delete _this;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   FromValueGridLength
//  Synopsis: Parses the type and value 
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CGridLength::FromValueGridLength(
    _In_ CREATEPARAMETERS *pCreate)
{
    ASSERT(pCreate->m_value.GetType() == valueGridLength);
    m_gridLength = *pCreate->m_value.AsGridLength();
    if (IsNanF(m_gridLength.value))
    {
        m_gridLength.value = XGRIDLENGTH::Default();
        m_gridLength.type = DirectUI::GridUnitType::Auto;
    }
    return S_OK;
}

_Check_return_ HRESULT CGridLength::FromString(
    _In_ const xstring_ptr_view& inString)
{
    return GridLengthFromString(inString, &m_gridLength);
}
