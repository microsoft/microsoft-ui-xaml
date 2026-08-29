// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ParserServiceProvider.g.h"
#include "ProvideValueTargetProperty.g.h"
#include <XamlServiceProviderContext.h>
#include <MetadataApi.h>
#include <CValueBoxer.h>
#include <XamlQualifiedObject.h>
#include <XamlType.h>
#include <comInstantiation.h>

using namespace DirectUI;

_Check_return_ HRESULT
ParserServiceProvider::get_BaseUriImpl(_Outptr_result_maybenull_ wf::IUriRuntimeClass** ppValue)
{
    xstring_ptr strUri;

    auto baseUri = m_serviceProviderContext->GetBaseUri();

    if (baseUri)
    {
        CValue boxedUriString;
        IFC_RETURN(baseUri->GetCanonical(&strUri));
        boxedUriString.Set<valueString>(std::move(strUri));
        IFC_RETURN(CValueBoxer::UnboxValue(&boxedUriString, ppValue));
    }
    else
    {
        *ppValue = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT
ParserServiceProvider::get_RootObjectImpl(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    auto rootObjectQO = m_serviceProviderContext->GetRootObject();

    auto pRootObjectNative = rootObjectQO->GetDependencyObject();
    DependencyObject* pRootObjectPeer = nullptr;

    if (pRootObjectNative)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pRootObjectNative, &pRootObjectPeer));
    }

    *ppValue = static_cast<IInspectable*>(static_cast<xaml::IDependencyObject*>(pRootObjectPeer));

    return S_OK;
}

_Check_return_ HRESULT
ParserServiceProvider::get_TargetObjectImpl(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    std::shared_ptr<::XamlQualifiedObject> targetObjectQO;
    m_serviceProviderContext->GetMarkupExtensionTargetObject(targetObjectQO);

    auto pTargetObjectNative = targetObjectQO->GetDependencyObject();
    DependencyObject* pTargetObjectPeer = nullptr;

    if (pTargetObjectNative)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pTargetObjectNative, &pTargetObjectPeer));
    }

    *ppValue = static_cast<IInspectable*>(static_cast<xaml::IDependencyObject*>(pTargetObjectPeer));

    return S_OK;
}

_Check_return_ HRESULT
ParserServiceProvider::get_TargetPropertyImpl(_Outptr_result_maybenull_ IInspectable** ppValue)
{
    std::shared_ptr<XamlProperty> targetXamlProperty;
    m_serviceProviderContext->GetMarkupExtensionTargetProperty(targetXamlProperty);

    ctl::ComPtr<ProvideValueTargetProperty> provideValueTargetProperty;
    IFC_RETURN(ctl::make<ProvideValueTargetProperty>(targetXamlProperty, &provideValueTargetProperty));
    IFC_RETURN(provideValueTargetProperty.CopyTo(ppValue));

    return S_OK;
}

_Check_return_ HRESULT
ParserServiceProvider::ResolveImpl(_In_ HSTRING qualifiedTypeName, _Out_ wxaml_interop::TypeName* pResult)
{
    std::shared_ptr<XamlType> resolvedType;
    xstring_ptr typeName;

    IFC_RETURN(m_serviceProviderContext->ResolveXamlType(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(qualifiedTypeName), resolvedType));
    if (resolvedType)
    {
        auto classInfo = DirectUI::MetadataAPI::GetClassInfoByIndex(resolvedType->get_TypeToken().GetHandle());
        IFC_RETURN(DirectUI::MetadataAPI::GetTypeNameByClassInfo(classInfo, pResult));
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT
ParserServiceProvider::GetServiceImpl(_In_ wxaml_interop::TypeName type, _Outptr_ IInspectable** ppResult)
{
    const CClassInfo* pType = nullptr;

    IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByTypeName(type, &pType));

    if (pType)
    {
        switch (pType->GetIndex())
        {
            case KnownTypeIndex::IProvideValueTarget:
            {
                *ppResult = static_cast<xaml_markup::IProvideValueTarget*>(this);
            }
            break;

            case KnownTypeIndex::IXamlTypeResolver:
            {
                *ppResult = static_cast<xaml_markup::IXamlTypeResolver*>(this);
            }
            break;

            case KnownTypeIndex::IUriContext:
            {
                *ppResult = static_cast<xaml_markup::IUriContext*>(this);
            }
            break;

            case KnownTypeIndex::IRootObjectProvider:
            {
                *ppResult = static_cast<xaml_markup::IRootObjectProvider*>(this);
            }
            break;

            default:
            {
                *ppResult = nullptr;
            }
        }

        if (*ppResult)
        {
            AddRefOuter();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
ParserServiceProvider::Initialize(std::shared_ptr<XamlServiceProviderContext> serviceProviderContext)
{
    m_serviceProviderContext = std::move(serviceProviderContext);

    return S_OK;
}