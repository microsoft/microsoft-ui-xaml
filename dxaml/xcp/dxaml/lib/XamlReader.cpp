// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlReader.g.h"

using namespace DirectUI;

_Check_return_ HRESULT
XamlReaderFactory::LoadImpl(_In_ HSTRING xaml, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    CValue val;
    IInspectable* pObject = NULL;
    xaml::IDependencyObject* pDO = NULL;
    LPCWSTR pszStrXaml = NULL;
    XUINT32 cStrXaml = 0;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT(pCore);

    pszStrXaml = WindowsGetStringRawBuffer(xaml, &cStrXaml);

    hr = CoreImports::Host_CreateFromXaml(
        pCore->GetHandle(),
        cStrXaml,
        pszStrXaml,
        /* bCreateNameScope */ TRUE,
        /* bRequireDefaultNamespace */ TRUE,
        /* bExpandTemplatesDuringParse */ FALSE,
        &val);

    if (FAILED(hr))
    {
        // Translate to XamlParseFailed error. The CLR knows how to translate this to 
        // a XamlParseException.
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_XAMLPARSEFAILED);
    }
    IFC(hr);

    IFC(CValueBoxer::UnboxObjectValue(&val, /* pTargetType */ NULL, &pObject));

    // During the parse, the only thing keeping the peer for the root
    // object alive was the peer table. Now that we have another 
    // reference to it ("pDO"), we can release on behalf of the parser.
    pDO = ctl::query_interface<xaml::IDependencyObject>(pObject);
    if (pDO)
    {
        static_cast<DependencyObject *>(pDO)->UnpegNoRef();
    }

    *ppReturnValue = pObject;
    pObject = NULL;

Cleanup:
    ReleaseInterface(pObject);
    ReleaseInterface(pDO);
    RRETURN(hr);
}

_Check_return_ HRESULT
XamlReaderFactory::LoadWithInitialTemplateValidationImpl(_In_ HSTRING xaml, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    CValue val;
    IInspectable* pObject = NULL;
    xaml::IDependencyObject* pDO = NULL;
    LPCWSTR pszStrXaml = NULL;
    XUINT32 cStrXaml = 0;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCEXPECT(pCore);

    pszStrXaml = WindowsGetStringRawBuffer(xaml, &cStrXaml);

    hr = CoreImports::Host_CreateFromXaml(
        pCore->GetHandle(),
        cStrXaml,
        pszStrXaml,
        /* bCreateNameScope */ TRUE,
        /* bRequireDefaultNamespace */ TRUE,
        /* bExpandTemplatesDuringParse */ TRUE,
        &val);

    if (FAILED(hr))
    {
        // Translate to XamlParseFailed error. The CLR knows how to translate this to 
        // a XamlParseException.
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_XAML, E_XAMLPARSEFAILED);
        IFC(hr);
    }
    
    IFC(CValueBoxer::UnboxObjectValue(&val, /* pTargetType */ NULL, &pObject));

    // During the parse, the only thing keeping the peer for the root
    // object alive was the peer table. Now that we have another 
    // reference to it ("pDO"), we can release on behalf of the parser.
    pDO = ctl::query_interface<xaml::IDependencyObject>(pObject);
    // Note that the returned pDO is left pegged (noref), and will be unpegged when it's added to the tree.
   
    *ppReturnValue = pObject;
    pObject = NULL;

Cleanup:
    
    ReleaseInterface(pObject);
    ReleaseInterface(pDO);
    RRETURN(hr);
}