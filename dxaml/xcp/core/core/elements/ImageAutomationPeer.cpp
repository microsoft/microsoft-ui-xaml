// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//
// CImageAutomationPeer
//

//
// Summary:
//      ctor.
//
CImageAutomationPeer::CImageAutomationPeer(
    _In_ CCoreServices *pCore,  
    _In_ CValue        &value)
    : CFrameworkElementAutomationPeer(pCore, value)
{
    ASSERT(value.GetType() == valueObject);
    VERIFYHR(DoPointerCast(m_pImage, value.AsObject()));
    ASSERT(m_pImage);
}

//
// Summary:
//      dtor.
//
CImageAutomationPeer::~CImageAutomationPeer()
{
    m_pImage = NULL;
}

//
// Summary:
//      Creation method.
//
_Check_return_ 
HRESULT 
CImageAutomationPeer::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_        CREATEPARAMETERS   *pCreate
) 
{
    HRESULT hr = S_OK;
    CImageAutomationPeer *pPeer = NULL;
    
    IFCEXPECT_ASSERT(ppObject);
    *ppObject = NULL;

    IFCEXPECT_ASSERT(pCreate);

    if (pCreate->m_value.GetType() != valueObject)
    {
        IFC(E_NOTIMPL);
    }
    else
    {
        pPeer = new CImageAutomationPeer(pCreate->m_pCore, pCreate->m_value);
        IFC(ValidateAndInit(pPeer, ppObject));

    // On success we've transferred ownership

        pPeer = NULL;
    }
   
Cleanup:
    delete pPeer;
    RRETURN(hr);
}