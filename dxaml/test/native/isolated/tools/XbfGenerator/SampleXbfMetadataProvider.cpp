// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Microsoft.UI.Xaml.coretypes2.h"
#include <XbfMetadataProvider.h>
#include "SampleXbfMetadataProvider.h"
#include <iostream>
#include "XamlLogging.h"
#include <XamlLibMetadataProvider.h>

static const wchar_t* s_pWUXExtensions[] = {
    L"Microsoft.UI.Xaml.Phone.dll",
};

void STDAPICALLTYPE LoadStub(_In_opt_ const HMODULE hModule, _In_ const wchar_t * modulename)
{
}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace XbfGenerator {

    HRESULT SampleXbfMetadataProvider::RuntimeClassInitialize()
    {
        wrl::ComPtr<xaml_phone_xti::XamlLibMetadataProvider> metadataProvider;
        IFC_RETURN(wrl::MakeAndInitialize<xaml_phone_xti::XamlLibMetadataProvider>(&metadataProvider, s_pWUXExtensions, static_cast<unsigned int>(ARRAYSIZE(s_pWUXExtensions)), LoadStub, nullptr));
        m_xmp = metadataProvider;
        
        // This isn't a WinRT application, so we need to do this the old-fashioned way.
        wrl::ComPtr<::IActivationFactory> xamlControlsXamlMetadataProviderFactory;
        
        typedef void (WINAPI * PfnDllGetActivationFactory)(_In_ HSTRING, _Out_ ::IActivationFactory**);
        PfnDllGetActivationFactory pfnDllGetActivationFactory;
        m_hModuleMuxc.reset(LoadLibrary(L"Microsoft.UI.Xaml.Controls.dll"));
        pfnDllGetActivationFactory = reinterpret_cast<PfnDllGetActivationFactory>(GetProcAddress(m_hModuleMuxc.get(), "DllGetActivationFactory"));
        pfnDllGetActivationFactory(
            wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider").Get(),
            xamlControlsXamlMetadataProviderFactory.ReleaseAndGetAddressOf());
        
        wrl::ComPtr<::IInspectable> xamlControlsXamlMetadataProviderAsI;
        IFC_RETURN(xamlControlsXamlMetadataProviderFactory->ActivateInstance(&xamlControlsXamlMetadataProviderAsI));
        IFC_RETURN(xamlControlsXamlMetadataProviderAsI.As(&m_muxcXmp));
        
        return S_OK;
    }

    SampleXbfMetadataProvider::~SampleXbfMetadataProvider()
    {
        // m_muxcXmp must be deleted before we free m_hModuleMuxc because the destructor
        // exists in the DLL, meaning that if we free m_hModuleMuxc first, we'll crash
        // when we attempt to retrieve the destructor.  For that reason, we'll explicitly
        // clear these smart pointers rather than letting it happen automatically in order
        // to ensure proper ordering.
        m_muxcXmp.Reset();
        m_hModuleMuxc.reset();
    }

    STDMETHODIMP SampleXbfMetadataProvider::GetXamlType(_In_ wxaml_interop::TypeName typeName, _Out_ IXbfType ** xbfType)
    {
        wrl::ComPtr<xaml_markup::IXamlType> xamlType;
        IFC_RETURN((m_xmp->GetXamlType(typeName, &xamlType)));

        if (!xamlType)
        {
            IFC_RETURN(m_muxcXmp->GetXamlType(typeName, &xamlType));
        }
        
        if (xamlType)
        {
            wrl::ComPtr<IXbfType> type = wrl::Make<XbfType>(xamlType);
            *xbfType = type.Detach();
        }
        
        return S_OK;
    }

    STDMETHODIMP SampleXbfMetadataProvider::GetXamlTypeByFullName(_In_ BSTR hFullName, _Out_ IXbfType **xbfType)
    {
        *xbfType = nullptr;

        wrl::ComPtr<xaml_markup::IXamlType> xamlType;
        IFC_RETURN(m_xmp->GetXamlTypeByFullName(wrl::Wrappers::HStringReference(hFullName).Get(), &xamlType));
        
        if (!xamlType)
        {
            IFC_RETURN(m_muxcXmp->GetXamlTypeByFullName(wrl::Wrappers::HStringReference(hFullName).Get(), &xamlType));
        }
        
        if (xamlType)
        {
            wrl::ComPtr<IXbfType> type = wrl::Make<XbfType>(xamlType);
            *xbfType = type.Detach();
        }

        return S_OK;
    }

    STDMETHODIMP SampleXbfMetadataProvider::GetXmlnsDefinitions(_In_ UINT * lenght, _Out_ xaml_markup::XmlnsDefinition ** def)
    {
        return m_xmp->GetXmlnsDefinitions(lenght, def);
    }

    STDMETHODIMP XbfType::get_BaseType(_Out_ IXbfType **xbfType)
    {
        *xbfType = nullptr;

        if (m_xamlType)
        {
            wrl::ComPtr<xaml_markup::IXamlType> xamlType;
            IFC_RETURN(m_xamlType->get_BaseType(&xamlType));
            *xbfType = wrl::Make<XbfType>(xamlType).Detach();
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_ContentProperty(_Out_ IXbfMember **xbfMember)
    {
        *xbfMember = nullptr;

        if (m_xamlType)
        {
            wrl::ComPtr<xaml_markup::IXamlMember> xamlMember;
            IFC_RETURN(m_xamlType->get_ContentProperty(&xamlMember));
            *xbfMember = wrl::Make<XbfMember>(xamlMember).Detach();
        }


        return S_OK;
    }

    STDMETHODIMP XbfType::get_FullName(_Out_ BSTR *hFullName)
    {
        *hFullName = nullptr;

        if (m_xamlType)
        {
            wrl::Wrappers::HString fullName;
            IFC_RETURN(m_xamlType->get_FullName(fullName.GetAddressOf()));
            *hFullName = SysAllocString(fullName.GetRawBuffer(nullptr));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_IsArray(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->get_IsArray(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_IsCollection(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->get_IsCollection(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_IsConstructible(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->get_IsConstructible(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_IsDictionary(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->get_IsDictionary(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_IsMarkupExtension(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->get_IsMarkupExtension(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_IsBindable(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->get_IsBindable(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_ItemType(_Out_ IXbfType ** itemType)
    {
        *itemType = nullptr;
        if (m_xamlType)
        {
            wrl::ComPtr<xaml_markup::IXamlType> xamlType;
            IFC_RETURN(m_xamlType->get_ItemType(&xamlType));
            *itemType = wrl::Make<XbfType>(xamlType).Detach();
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_KeyType(_Out_ IXbfType **keyType)
    {
        *keyType = nullptr;
        if (m_xamlType)
        {
            wrl::ComPtr<xaml_markup::IXamlType> xamlType;
            IFC_RETURN(m_xamlType->get_ItemType(&xamlType));
            *keyType = wrl::Make<XbfType>(xamlType).Detach();
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::get_BoxedType(_Out_ IXbfType** keyType)
    {
        *keyType = nullptr;

        return S_OK;
    }

    STDMETHODIMP XbfType::get_UnderlyingType(_Out_ wxaml_interop::TypeName *type)
    {
        *type = {};

        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->get_UnderlyingType(type));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::ActivateInstance(_Out_ IInspectable **)
    {
        return E_FAIL;
    }

    STDMETHODIMP XbfType::CreateFromString(_In_ BSTR, _Out_ IInspectable **)
    {
        return E_FAIL;
    }

    STDMETHODIMP XbfType::GetMember(_In_ BSTR hFullName, _Out_ IXbfMember **xbfMember)
    {
        *xbfMember = nullptr;
        if (m_xamlType)
        {
            wrl::ComPtr<xaml_markup::IXamlMember> xamlMember;
            IFC_RETURN(m_xamlType->GetMember(wrl::Wrappers::HStringReference(hFullName).Get(), &xamlMember));
            *xbfMember = wrl::Make<XbfMember>(xamlMember).Detach();
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::AddToVector(_In_ IInspectable * instance, _In_ IInspectable *result)
    {
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->AddToVector(instance, result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::AddToMap(_In_ IInspectable *instance, _In_ IInspectable *key, _In_ IInspectable *result)
    {
        if (m_xamlType)
        {
            IFC_RETURN(m_xamlType->AddToMap(instance, key, result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfType::RunInitializer(void)
    {
        return E_FAIL;
    }

    STDMETHODIMP XbfMember::get_IsAttachable(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlMember)
        {
            IFC_RETURN(m_xamlMember->get_IsAttachable(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfMember::get_IsDependencyProperty(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlMember)
        {
            IFC_RETURN(m_xamlMember->get_IsDependencyProperty(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfMember::get_IsReadOnly(_Out_ boolean *result)
    {
        *result = FALSE;
        if (m_xamlMember)
        {
            IFC_RETURN(m_xamlMember->get_IsReadOnly(result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfMember::get_Name(_Out_ BSTR *hFullName)
    {
        *hFullName = nullptr;
        if (m_xamlMember)
        {
            wrl::Wrappers::HString name;
            IFC_RETURN(m_xamlMember->get_Name(name.GetAddressOf()));
            *hFullName = SysAllocString(name.GetRawBuffer(nullptr));
        }

        return S_OK;
    }

    STDMETHODIMP XbfMember::get_TargetType(_Out_ IXbfType **xbfType)
    {
        *xbfType = nullptr;

        if (m_xamlMember)
        {
            wrl::ComPtr<xaml_markup::IXamlType> xamlType;
            IFC_RETURN(m_xamlMember->get_TargetType(&xamlType));

            *xbfType = wrl::Make<XbfType>(xamlType).Detach();
        }

        return S_OK;
    }

    STDMETHODIMP XbfMember::get_Type(_Out_ IXbfType **xbfType)
    {
        *xbfType = nullptr;

        if (m_xamlMember)
        {
            wrl::ComPtr<xaml_markup::IXamlType> xamlType;
            IFC_RETURN(m_xamlMember->get_Type(&xamlType));
            *xbfType = wrl::Make<XbfType>(xamlType).Detach();
        }

        return S_OK;
    }

    STDMETHODIMP XbfMember::GetValue(_In_ IInspectable * instance, _Out_ IInspectable **result)
    {
        *result = nullptr;
        if (m_xamlMember)
        {
            IFC_RETURN(m_xamlMember->GetValue(instance, result));
        }

        return S_OK;
    }

    STDMETHODIMP XbfMember::SetValue(_In_ IInspectable *instance, _In_ IInspectable *result)
    {
        if (m_xamlMember)
        {
            IFC_RETURN(m_xamlMember->SetValue(instance, result));
        }

        return S_OK;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Tools::XbfGenerator
