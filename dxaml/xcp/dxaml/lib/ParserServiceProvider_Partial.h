// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ParserServiceProvider.g.h"

class XamlServiceProviderContext;

namespace DirectUI
{
    // It is not recommended that instances of this class be instantiated without use of ctl::make
    PARTIAL_CLASS(ParserServiceProvider)
    {
    public:
#pragma region IXamlServiceProvider services

        // IUriContext
        _Check_return_ HRESULT get_BaseUriImpl(_Outptr_result_maybenull_ wf::IUriRuntimeClass** ppValue);

        // IRootObjectProvider
        _Check_return_ HRESULT get_RootObjectImpl(_Outptr_result_maybenull_ IInspectable** ppValue);

        // IProvideValueTarget
        _Check_return_ HRESULT get_TargetObjectImpl(_Outptr_result_maybenull_ IInspectable** ppValue);
        _Check_return_ HRESULT get_TargetPropertyImpl(_Outptr_result_maybenull_ IInspectable** ppValue);

        // IXamlTypeResolver
        _Check_return_ HRESULT ResolveImpl(_In_ HSTRING qualifiedTypeName, _Out_ wxaml_interop::TypeName* pResult);

        // IXamlServiceProvider
        _Check_return_ HRESULT GetServiceImpl(_In_ wxaml_interop::TypeName type, _Outptr_ IInspectable** ppResult);

#pragma endregion

        // Primarily used to support instantiation and initialization via ctl::make
        _Check_return_ HRESULT Initialize(std::shared_ptr<XamlServiceProviderContext> serviceProviderContext);

    private:
        std::shared_ptr<XamlServiceProviderContext> m_serviceProviderContext;
    };
}