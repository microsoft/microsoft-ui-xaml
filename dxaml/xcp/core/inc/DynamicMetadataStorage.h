// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Metadata storage for custom types and properties. 

#pragma once

#include <CustomClassInfo.h>
#include <CustomDependencyProperty.h>
#include <CStaticLock.h>
#include <vector_map.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Metadata {
        struct DynamicMetadataStorageMock;
    }
} } } }

namespace DirectUI
{
    class DynamicMetadataStorage
    {
        friend struct Microsoft::UI::Xaml::Tests::Metadata::DynamicMetadataStorageMock;
        friend class DynamicMetadataStorageInstanceWithLock;

        DynamicMetadataStorage() = default;

        // Not thread-safe.
        static DynamicMetadataStorage* GetInstance();

        // Not thread-safe.
        void ResetInstance();
        static void Reset();
        static void Destroy();

        void DestroyCustomPropertiesAndDPs()
        {
            // Manually dispatch, since there's are no virtuals in this class hierarchy
            for (auto property : m_customPropertiesCache)
            {
                if (auto customDependencyProperty = property->AsOrNull<CCustomDependencyProperty>())
                {
                    delete customDependencyProperty;
                }
                else if (auto customProperty = property->AsOrNull<CCustomProperty>())
                {
                    delete customProperty;
                }
                else
                {
                    ASSERT(false);
                }
            }
        }

    public:
        DynamicMetadataStorage(const DynamicMetadataStorage&) = delete;
        DynamicMetadataStorage& operator=(const DynamicMetadataStorage&) = delete;

        ~DynamicMetadataStorage()
        {
            DestroyCustomPropertiesAndDPs();
        }

        // Gets the next available namespace index for a custom namespace.
        KnownNamespaceIndex GetNextAvailableNamespaceIndex()
        {
            return static_cast<KnownNamespaceIndex>(KnownNamespaceCount + m_customNamespacesCache.size());
        }

        // Gets the next available property index for a custom property.
        KnownPropertyIndex GetNextAvailablePropertyIndex()
        {
            return static_cast<KnownPropertyIndex>(KnownPropertyCount + m_customPropertiesCache.size());
        }

        // Gets the next available type index for a custom type.
        KnownTypeIndex GetNextAvailableTypeIndex()
        {
            return static_cast<KnownTypeIndex>(KnownTypeCount + m_customTypesCache.size());
        }

        // Struct to hold deferred DP registration information.
        struct DPRegistrationInfo
        {
            DPRegistrationInfo(
                _In_ CCustomDependencyProperty* dp,
                _In_ wxaml_interop::TypeKind propertyTypeKind,
                _In_ const xstring_ptr& propertyTypeName,
                _In_ wxaml_interop::TypeKind ownerTypeKind,
                _In_ const xstring_ptr& ownerTypeName,
                _In_ xaml::IPropertyMetadata* defaultMetadata)
                : m_dp(dp)
                , m_propertyTypeKind(propertyTypeKind)
                , m_propertyTypeName(propertyTypeName)
                , m_declaringTypeKind(ownerTypeKind)
                , m_declaringTypeName(ownerTypeName)
                , m_defaultMetadata(defaultMetadata)
            {}

            CCustomDependencyProperty* m_dp;
            wxaml_interop::TypeKind m_propertyTypeKind;
            wxaml_interop::TypeKind m_declaringTypeKind;
            xstring_ptr m_propertyTypeName;
            xstring_ptr m_declaringTypeName;
            ctl::ComPtr<xaml::IPropertyMetadata> m_defaultMetadata;
        };

        struct GenerationBoundary
        {
            std::size_t m_namespaceFirstIndex  = 0;
            std::size_t m_typeFirstIndex       = 0;
            std::size_t m_propertyFirstIndex   = 0;
        };

        using PropertiesTable       = containers::vector_map<xstring_ptr, const CDependencyProperty*>;
        using PropertiesByTypeTable = std::unordered_map<KnownTypeIndex, std::unique_ptr<PropertiesTable>>;

        containers::vector_map<xstring_ptr, const CNamespaceInfo*>  m_customNamespacesByNameCache;
        std::vector<std::unique_ptr<CNamespaceInfo>>                m_customNamespacesCache;

        containers::vector_map<xstring_ptr, const CClassInfo*>      m_customTypesByNameCache;
        containers::vector_map<xstring_ptr, const CClassInfo*>      m_customTypesByCustomNameCache;
        std::vector<std::unique_ptr<CCustomClassInfo>>              m_customTypesCache;

        std::unique_ptr<PropertiesByTypeTable>                      m_customDPsByTypeAndNameCache;
        std::unique_ptr<PropertiesByTypeTable>                      m_customPropertiesByTypeAndNameCache;
        std::vector<CDependencyProperty*>                           m_customPropertiesCache; // Stores both DPs and regular properties.

        std::unique_ptr<std::vector<ctl::ComPtr<xaml::IDependencyProperty>>>   m_dpHandleCache;
        std::unique_ptr<std::queue<DPRegistrationInfo>>             m_queuedDPRegistrations;

        ctl::ComPtr<xaml_markup::IXamlMetadataProvider> m_metadataProvider;

        // Overridden metadata provider. This primarily exists to load and unload providers for unit tests.
        ctl::ComPtr<xaml_markup::IXamlMetadataProvider> m_overriddenMetadataProvider;

        GenerationBoundary m_valid = {};
    };

    // Automatically locks access to DynamicMetadataStorage members.
    class DynamicMetadataStorageInstanceWithLock
    {
    public:
        DynamicMetadataStorageInstanceWithLock() = default;

        DynamicMetadataStorage* operator->()
        {
            return DynamicMetadataStorage::GetInstance();
        }

        static void Reset()
        {
            // If the lock is not initialized there should be no metadata reset

            if (DirectUI::CStaticLock::IsInitialized())
            {
                DirectUI::CStaticLock lock;
                DynamicMetadataStorage::Reset();
            }
        }

        static void Destroy()
        {
            if (DirectUI::CStaticLock::IsInitialized())
            {
                DirectUI::CStaticLock lock;
                DynamicMetadataStorage::Destroy();
            }
        }

    private:
        DirectUI::CStaticLock m_lock;
    };

    // CLR interface for running class constructors.
    class DECLSPEC_UUID("60D27C8D-5F61-4CCE-B751-690FAE66AA53") IManagedActivationFactory : public IInspectable
    {
    public:
        STDMETHOD(RunClassConstructor)() = 0;
    };
}
