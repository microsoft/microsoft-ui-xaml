// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CurrentChangingEventArgs_partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT CurrentChangingEventArgsFactory::CreateWithCancelableParameterImpl(
    _In_ BOOLEAN isCancelable, 
    _In_opt_ IInspectable* pOuter, 
    _Outptr_ IInspectable** ppInner, 
    _Outptr_ xaml_data::ICurrentChangingEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_data::ICurrentChangingEventArgs *pNewInstance = NULL;
    CurrentChangingEventArgs *pArgs = NULL;

    IFC(CreateInstance(pOuter, ppInner, &pNewInstance));
    pArgs = static_cast<CurrentChangingEventArgs *>(pNewInstance);

    pArgs->m_fIsCancelable = isCancelable;

    // Transfer the new instance to the caller
    *ppInstance = pNewInstance;
    pNewInstance = NULL;

Cleanup:

    ReleaseInterface(pNewInstance);

    RRETURN(hr);
}

// Creates simple non-aggregated instances for the CurrentChangingEventArgs class to be used
// internally, external instantiations will come through the above factory method
_Check_return_ 
HRESULT 
DirectUI::CurrentChangingEventArgs::CreateInstance(_In_ BOOLEAN isCancelable, _Outptr_ CurrentChangingEventArgs **ppArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CurrentChangingEventArgs> spEventArgs;
    
    IFC(ctl::make(&spEventArgs));
    spEventArgs->m_fIsCancelable = isCancelable;

    // Transfer the new instance to the caller
    *ppArgs = spEventArgs.Detach();

Cleanup:
    RRETURN(hr);
}

