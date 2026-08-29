// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlLibMetadataProvider.h>
#include <DependencyLocator.h>
#include <RuntimeEnabledFeatures.h>
#include "XamlTypeInfoProvider.h"
#include <DesignMode.h>
#include <XamlBehaviorMode.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Phone_XamlTypeInfo {

    XamlLibMetadataProvider::XamlLibMetadataProvider()
        : m_modules(nullptr)
        , m_moduleCount(0)
        , m_pfnCacheExtensionsTelemetry(nullptr)
        , m_hostMetadataProvider(nullptr)
    {
    }

    XamlLibMetadataProvider::~XamlLibMetadataProvider()
    {
        ResetDependencyProperties();
    }

    HRESULT
    XamlLibMetadataProvider::RuntimeClassInitialize(
        _In_reads_(cModules) PCWSTR* const modules,
        unsigned int cModules,
        _In_ PFNTELEMETRYPROC pfnCacheExtensionsCallback,
        _In_opt_ IInspectable* hostMetadataProvider)
    {
        m_modules = modules;
        m_moduleCount = cModules;
        m_pfnCacheExtensionsTelemetry = pfnCacheExtensionsCallback;

        if (hostMetadataProvider != NULL)
        {
            IGNOREHR(hostMetadataProvider->QueryInterface(
                __uuidof(xaml_markup::IXamlMetadataProvider),
                reinterpret_cast<void**>(m_hostMetadataProvider.GetAddressOf())));
        }

        return S_OK;
    }

    IFACEMETHODIMP
    XamlLibMetadataProvider::GetXmlnsDefinitions(
        _Out_ UINT* pnLength,
        _Outptr_result_buffer_maybenull_(*pnLength) xaml_markup::XmlnsDefinition** ppDefinitions)
    {
        HRESULT hr = (pnLength == nullptr || ppDefinitions == nullptr) ? E_INVALIDARG : S_OK;

        if (SUCCEEDED(hr))
        {
            if (m_hostMetadataProvider)
            {
                hr = m_hostMetadataProvider->GetXmlnsDefinitions(
                    pnLength,
                    ppDefinitions);
            }
            else
            {
                *pnLength = 0;
                *ppDefinitions = nullptr;
            }
        }

        return hr;
    }

    IFACEMETHODIMP
    XamlLibMetadataProvider::GetXamlType(
        wxaml_interop::TypeName type,
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_markup::IXamlType> xamlType;

        IFCPTRRC(value, E_INVALIDARG);

        IFC(GetXamlTypeByFullNameWorker(
            type.Name,
            &xamlType));

        if (!xamlType)
        {
            if (m_hostMetadataProvider)
            {
                hr = m_hostMetadataProvider->GetXamlType(
                    type,
                    &xamlType);
            }
        }

        IFC(hr);

        *value = xamlType.Detach();

    Cleanup:
        return hr;
    }

    IFACEMETHODIMP
    XamlLibMetadataProvider::GetXamlTypeByFullName(
        _In_ HSTRING fullName,
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_markup::IXamlType> xamlType;

        IFCPTRRC(value, E_INVALIDARG);

        IFC(GetXamlTypeByFullNameWorker(
            fullName,
            &xamlType));

        if (!xamlType)
        {
            if (m_hostMetadataProvider)
            {
                hr = m_hostMetadataProvider->GetXamlTypeByFullName(
                    fullName,
                    &xamlType);
            }
        }

        IFC(hr);

        *value = xamlType.Detach();

    Cleanup:
        return hr;
    }

    void
    XamlLibMetadataProvider::InitializeProviders(HSTRING fullTypeName)
    {
        if (!m_providersInitialized)
        {
            const wchar_t* pszTypeName = WindowsGetStringRawBuffer(fullTypeName, nullptr);
            // The modules metadata providers register not only types under the the XAML namespace
            // (Microsoft.UI.Xaml.) but may also register types under Windows namespaces (e.g.
            // Windows.Foundation.Collection.IVector<T>).
            static const wchar_t xamlNamespace[] = L"Microsoft.UI.Xaml";
            static const wchar_t windowsNamespace[] = L"Windows.";
            if (wcsncmp(pszTypeName, xamlNamespace, ARRAY_SIZE(xamlNamespace) - 1) == 0 ||
                wcsncmp(pszTypeName, windowsNamespace, ARRAY_SIZE(windowsNamespace) - 1) == 0)
            {
                InitializeProviders();
            }
        }
    }

    void
    XamlLibMetadataProvider::InitializeProviders()
    {
        if (!m_providersInitialized)
        {
            for (unsigned int i = 0; i < m_moduleCount; ++i)
            {
                PCWSTR name = m_modules[i];
                Private::XamlTypeInfoProvider provider(name);
                if ( SUCCEEDED(provider.Initialize(m_pfnCacheExtensionsTelemetry)) )
                {
                    m_providers.insert(m_providers.begin(), std::move(provider));
                }
            }

            m_providersInitialized = true;
        }
    }

    void
    XamlLibMetadataProvider::ResetDependencyProperties()
    {
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

        // Reset DPs in all cases except new Designer.

        if (m_providersInitialized &&
            (DesignerInterop::GetDesignerMode(DesignerMode::None) ||
             runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown)))
        {
            for (Private::XamlTypeInfoProvider& provider : m_providers)
            {
                provider.ResetDependencyProperties();
            }
        }
    }

    HRESULT
    XamlLibMetadataProvider::GetXamlTypeByFullNameWorker(
        _In_ HSTRING fullName,
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        wrl::ComPtr<xaml_markup::IXamlType> xamlType;

        ASSERT(value != NULL);

        InitializeProviders(fullName);

        for(Private::XamlTypeInfoProvider& provider: m_providers)
        {
            IFC_RETURN(provider.FindXamlType_FromHSTRING(fullName, &xamlType));
            if ( xamlType )
            {
                break;
            }
        }

        *value = xamlType.Detach();

        return S_OK;
    }

} } } } XAML_ABI_NAMESPACE_END

