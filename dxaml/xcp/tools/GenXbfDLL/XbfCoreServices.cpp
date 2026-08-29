// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//
// XbfCoreServices
//
//   A CCoreServices implementation with selected Fx Callbacks and only type information initialization.
//

XbfCoreServices::XbfCoreServices()
{    
}

_Check_return_ HRESULT
XbfCoreServices::Create(
    _Out_ XbfCoreServices **ppCore)
{
    HRESULT hr;
    XbfCoreServices *pCore = nullptr;

    TraceCoreServicesCreateBegin();
    ENTERSECTION(Main);

    pCore = new XbfCoreServices;

    // Initialize the type store for this instance.
    IFC(XamlSchemaContext::Create(pCore, pCore->m_spXamlSchemaContext));

    // Now we can return the core services object to the caller

    *ppCore = pCore;
    pCore = NULL;
    hr = S_OK;

Cleanup:
    TraceCoreServicesCreateEnd();

    delete pCore;

    RRETURN(hr);
}
