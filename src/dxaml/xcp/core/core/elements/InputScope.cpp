// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of an input scope name
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CInputScopeName::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT               hr              = S_OK;
    CInputScopeName      *pInputScopeName = NULL;
    CInputScopeNameValue *pNameValue      = NULL;
    
    pInputScopeName = new CInputScopeName(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueString)
    {
        // Need to create a CInputScopeNameValue to convert from the string to the enumerated value.
        IFC(CInputScopeNameValue::Create((CDependencyObject **)&pNameValue, pCreate));
        pInputScopeName->m_nameValue = (DirectUI::InputScopeNameValue)pNameValue->m_nValue;
    }
    else
    {
        pInputScopeName->m_nameValue = DirectUI::InputScopeNameValue::Default;
    }

   *ppObject = static_cast<CDependencyObject *>(pInputScopeName);
    pInputScopeName = NULL;

Cleanup:
    ReleaseInterface(pInputScopeName);
    ReleaseInterface(pNameValue);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of an input scope name collection
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CInputScopeNameCollection::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT                    hr                        = S_OK;
    CInputScopeNameCollection *pInputScopeNameCollection = NULL;
    CInputScopeName           *pInputScopeName           = NULL;

    pInputScopeNameCollection = new CInputScopeNameCollection(pCreate->m_pCore);

    if (pCreate->m_value.GetType() == valueString)
    {
        // If we've been given a string, parse it as an InputScopeName and add it to the collection.
        IFC(CInputScopeName::Create((CDependencyObject**)&pInputScopeName, pCreate));
        IFC(pInputScopeNameCollection->Append(pInputScopeName, NULL));
    }
    
   *ppObject = static_cast<CDependencyObject *>(pInputScopeNameCollection);
    pInputScopeNameCollection = NULL;
   
Cleanup:
    ReleaseInterface(pInputScopeNameCollection);
    ReleaseInterface(pInputScopeName);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates an instance of an input scope
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CInputScope::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
)
{
    HRESULT                    hr                        = S_OK;
    CInputScope               *pInputScope               = NULL;
    CInputScopeNameCollection *pInputScopeNameCollection = NULL;

    pInputScope = new CInputScope(pCreate->m_pCore);

    // Create the name collection from the CREATEPARAMETERS - if we've been given a string,
    // it will be parsed to the appropriate enumerated value and added to the list.
    IFC(CInputScopeNameCollection::Create((CDependencyObject**)&pInputScopeNameCollection, pCreate));
    
    pInputScope->m_pNames = pInputScopeNameCollection;
    pInputScopeNameCollection = NULL;

   *ppObject = static_cast<CDependencyObject *>(pInputScope);
    pInputScope = NULL;

Cleanup:
    ReleaseInterface(pInputScope);
    ReleaseInterface(pInputScopeNameCollection);
    RRETURN(hr);
}
