// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <Microsoft.UI.Xaml.coretypes.h>
#include <vector>
#include <ExternalDependency.h>

namespace Private {
    class XamlTypeInfoProvider;
}

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Phone_XamlTypeInfo {

    class XamlLibMetadataProvider : public wrl::RuntimeClass<xaml_markup::IXamlMetadataProvider>
    {
        InspectableClass(nullptr /* this class is internal */, BaseTrust);

        ~XamlLibMetadataProvider() override;

    public:
        XamlLibMetadataProvider();

        HRESULT RuntimeClassInitialize(
            _In_reads_(cModules) PCWSTR* const modules,
            unsigned int cModules,
            _In_ PFNTELEMETRYPROC pfnCacheExtensionsCallback,
            _In_opt_ IInspectable* hostMetadataProvider);

        IFACEMETHOD(GetXmlnsDefinitions)(
            _Out_ UINT* pnLength,
            _Outptr_result_buffer_maybenull_(*pnLength) xaml_markup::XmlnsDefinition** ppDefinitions);

        IFACEMETHOD(GetXamlType)(
            wxaml_interop::TypeName type,
            _COM_Outptr_ xaml_markup::IXamlType** ppXamlType);

        IFACEMETHOD(GetXamlTypeByFullName)(
            _In_ HSTRING fullName,
            _COM_Outptr_ xaml_markup::IXamlType** ppXamlType);

    private:
        HRESULT GetXamlTypeByFullNameWorker(
            _In_ HSTRING fullName,
            _COM_Outptr_ xaml_markup::IXamlType** ppXamlType);

        PCWSTR* m_modules;
        unsigned int m_moduleCount;
        PFNTELEMETRYPROC m_pfnCacheExtensionsTelemetry;
        std::vector<::Private::XamlTypeInfoProvider> m_providers;
        wrl::ComPtr<xaml_markup::IXamlMetadataProvider> m_hostMetadataProvider;
        bool m_providersInitialized = false;

        void InitializeProviders();
        void InitializeProviders(HSTRING fullTypeName);
        void ShutdownProviders();
        void ResetDependencyProperties();
    };

} } } } XAML_ABI_NAMESPACE_END
