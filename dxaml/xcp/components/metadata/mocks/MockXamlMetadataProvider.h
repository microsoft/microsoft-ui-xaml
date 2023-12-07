// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ThreadLocalStorage.h>
#include <MockDynamicMetadataStorage.h>
#include <MockXamlType.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Metadata {

        class MockXamlMetadataProvider : public Microsoft::WRL::RuntimeClass<xaml_markup::IXamlMetadataProvider>
        {
        public:
            MockXamlMetadataProvider()
            {
                m_storageMock = TlsProvider<DynamicMetadataStorageMock>::CreateWrappedObject();
                m_storageMock->storage.m_metadataProvider = this;
            }

            MockXamlMetadataProvider(const std::shared_ptr<DynamicMetadataStorageMock>& mock)
            {
                // We only support a single IXamlMetadataProvider in storage at a time.
                ASSERT(mock->storage.m_metadataProvider == nullptr);
                mock->storage.m_metadataProvider = this;

                m_storageMock = mock;
            }

            IFACEMETHODIMP GetXamlType(_In_ wxaml_interop::TypeName type, _Out_ xaml_markup::IXamlType** ppXamlType)
            {
                if (GetXamlTypeCallback != nullptr)
                {
                    return GetXamlTypeCallback(type, ppXamlType);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP GetXamlTypeByFullName(_In_ HSTRING hFullName, _Out_ xaml_markup::IXamlType** ppXamlType)
            {
                if (GetXamlTypeByFullNameCallback != nullptr)
                {
                    return GetXamlTypeByFullNameCallback(hFullName, ppXamlType);
                }
                return E_NOTIMPL;
            }

            IFACEMETHODIMP GetXmlnsDefinitions(_Out_ XUINT32* pnLength, _Outptr_result_buffer_all_maybenull_(*pnLength) xaml_markup::XmlnsDefinition** ppDefinitions)
            {
                if (GetXmlnsDefinitionsCallback != nullptr)
                {
                    return GetXmlnsDefinitionsCallback(pnLength, ppDefinitions);
                }
                return E_NOTIMPL;
            }

            std::function<HRESULT(wxaml_interop::TypeName, xaml_markup::IXamlType**)> GetXamlTypeCallback;
            std::function<HRESULT(HSTRING, xaml_markup::IXamlType**)> GetXamlTypeByFullNameCallback;
            std::function<HRESULT(XUINT32*, xaml_markup::XmlnsDefinition**)> GetXmlnsDefinitionsCallback;

        private:
            std::shared_ptr<DynamicMetadataStorageMock> m_storageMock;
        };

    }

} } } }
