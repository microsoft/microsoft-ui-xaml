// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
class XbfCoreServices;
extern XbfCoreServices *g_pCore;

class CString
{
public:
    static long __stdcall CreateFromXStringPtr(class CCoreServices *,_Inout_ xstring_ptr&& strString,class CDependencyObject * *);
};

class CKeyTime
{
public:
    static long __stdcall Create(class CDependencyObject * *,class CREATEPARAMETERS *);
};

class XbfCoreServices : public CCoreServices
{
public:
    XbfCoreServices();

    static _Check_return_ HRESULT Create(_Out_ XbfCoreServices **ppCore);
};

class CDeferredMapping
{};

namespace CoreImports
{
_Check_return_ HRESULT XamlSchemaContext_AddAssemblyXmlnsDefinition(
    _In_ CCoreServices* pCore,
    _In_ XamlAssemblyToken tAssembly,
    _In_ const xstring_ptr& strXmlNamespace,
    _In_ XamlTypeNamespaceToken tTypeNamespace,
    _In_ const xstring_ptr& strTypeNamespace);
}

class CWinUriFactory final
{
public:
    static _Check_return_ HRESULT Create(_In_ XUINT32 cString, _In_reads_(cString) const WCHAR *pString, _Out_ IPALUri **ppUri);
};

namespace DirectUI
{
class BoxerBuffer
{
};
}

namespace ctl
{
    template <typename tobject>
    struct IsDependencyObject
    {
        static const bool value = std::is_base_of<DirectUI::DependencyObject, tobject>::value;
    };

    template <typename tobject>
    _Check_return_ typename std::enable_if<!IsDependencyObject<tobject>::value && IsComObject<tobject>::value, HRESULT>::type make(ctl::Internal::ComPtrRef<ComPtr<tobject>> ppNewInstance)
    {
        HRESULT hr = S_OK;
        tobject* pResult = NULL;

        IFC(ComObject<tobject>::CreateInstance(&pResult));
        auto ptr = ppNewInstance.ReleaseAndGetAddressOf();
        *ptr = pResult;
        pResult = NULL;

    Cleanup:

        release_interface(pResult);
        RRETURN(hr);
    }
}

namespace FxCallbacks
{
    _Check_return_ HRESULT FrameworkCallbacks_CheckPeerType(_In_ CDependencyObject* nativeRoot, _In_ const xstring_ptr& strPeerType, _In_ XINT32 bCheckExact)
    {
        return E_NOTIMPL;
    }

    void XamlRoot_RaiseChanged(_In_ IInspectable* xamlRootInsp) {}

    void XamlRoot_RaiseInputActivationChanged(_In_ IInspectable* xamlRootInsp) {}
}