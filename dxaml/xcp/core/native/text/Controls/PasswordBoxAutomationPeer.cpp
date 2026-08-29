// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//  
//---------------------------------------------------------------------------

CPasswordBoxAutomationPeer::CPasswordBoxAutomationPeer(
    _In_ CCoreServices *pCore,  
    _In_ CValue        &value) : CTextBoxBaseAutomationPeer(pCore, value)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by this class.
//  
//---------------------------------------------------------------------------
CPasswordBoxAutomationPeer::~CPasswordBoxAutomationPeer()
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Allocates a new CPasswordBoxAutomationPeer.
//  
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBoxAutomationPeer::Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate) 
{
    HRESULT hr = S_OK;
        
    IFCEXPECT_ASSERT(pCreate);

    if (pCreate->m_value.GetType() != valueObject)
    {
        IFC(E_NOTIMPL);
    }
    else
    {
        CPasswordBoxAutomationPeer *pPeer = 
            new CPasswordBoxAutomationPeer(pCreate->m_pCore, pCreate->m_value);
        IFC(ValidateAndInit(pPeer, ppObject));
    }
       
Cleanup:
    RRETURN(hr);
}
