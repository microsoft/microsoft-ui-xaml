// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "inc\ResourceDependency.h"
#include "inc\ResourceGraph.h"
#include "Indexes.g.h"
#include "TypeTableStructs.h"
#include "MetadataAPI.h"
#include <DiagnosticsInterop.h>
#include "dependencyLocator\inc\DependencyLocator.h"
#include "corep.h"
#include "resources.h"
#include "resources\inc\ResourceResolver.h"
#include <deferral\inc\CustomWriterRuntimeContext.h>
#include "DOPointerCast.h"
#include "Setter.h"
#include "primitiveDependencyObjects\inc\primitives.h"
#include "ManagedObjectReference.h"
#include "valueboxer\inc\CValueBoxer.h"
#include "DXamlServices.h"
#include <DependencyObject.h>
#include <DependencyObject\inc\ValueBuffer.h>

using namespace DirectUI;

namespace Diagnostics
{
    DEFINE_ENUM_FLAG_OPERATORS(VisualElementState);

    ResourceDependency::ResourceDependency(
        _In_ CDependencyObject* dependency,
        KnownPropertyIndex index,
        ResourceType type)
        : m_dependentItem(xref::get_weakref(dependency))
        , m_propertyIndex(index)
        , m_resourceType(type)
        , m_state(VisualElementState::ErrorResolved)
    {
    }

    bool ResourceDependency::operator<(const ResourceDependency& rhs) const
    {
        // Don't include the resourceType in the equality comparison.
        // If the property comparisons are the same, then compare the objects,
        // otherwise return the result of the property comparison
        if (m_propertyIndex == rhs.m_propertyIndex)
            return m_dependentItem < rhs.m_dependentItem;

        return m_propertyIndex < rhs.m_propertyIndex;
    }

    bool ResourceDependency::operator==(const ResourceDependency& rhs) const
    {
        // Don't include the resourceType in the equality comparison
        return m_dependentItem == rhs.m_dependentItem &&
               m_propertyIndex == rhs.m_propertyIndex;
    }

    bool ResourceDependency::IsValid() const
    {
        // We should be getting rid of expired dependencies before trying to party on them.
        ASSERT(!Expired());
        return m_state == VisualElementState::ErrorResolved;
    }

    bool ResourceDependency::Expired() const
    {
        return m_dependentItem.expired();
    }

    ResourceType ResourceDependency::GetType() const
    {
        return m_resourceType;
    }

    xref_ptr<CDependencyObject> ResourceDependency::GetDependency() const
    {
        ASSERT(!Expired());
        return m_dependentItem.lock();
    }

    KnownPropertyIndex ResourceDependency::GetPropertyIndex() const
    {
        return m_propertyIndex;
    }

    _Check_return_ HRESULT ResourceDependency::Resolve(
        _In_opt_ CResourceDictionary* dictionary,
        const xstring_ptr& key,
        _In_opt_ CDependencyObject* resourceContext,
        _Out_ Resources::ResolvedResource& resolvedResource)
    {
        ASSERT(!Expired());
        IFC_RETURN(FixDependencyAndProperty());

        IFC_RETURN(Resources::ResourceResolver::ResolveResourceRuntime(
            resourceContext ? resourceContext : GetDependency().get(),
            dictionary,
            key,
            m_resourceType,
            resolvedResource));

        auto previousState = m_state;
        m_state = VisualElementState::ErrorResolved;

        if (resolvedResource.Value)
        {
            // If resolved, make sure the resolved value is valid for this property
            bool isValid;
            IFC_RETURN(IsValidPropertyType(resolvedResource.Value.get(), &isValid));
            if (!isValid)
            {
                bool succeeded = false;
                IFC_RETURN(TryConvertValue(resolvedResource.Value, &succeeded));
                if (!succeeded)
                {
                    m_state = m_state | VisualElementState::ErrorInvalidResource;
                }
            }

        }
        else
        {
            m_state = VisualElementState::ErrorResourceNotFound;
        }

        // Even if it's invalid, add the dependency to the map. We keep track of the invalid state
        // so we can notify VS if it is reolved at a later time.
        const auto graph = GetResourceGraph();
        graph->RegisterResourceDependency(shared_from_this(), resolvedResource.DictionaryReadFrom.get(), key);

        // update the state and if invalid, notify the interop layer of our newly invalid errors.
        if (m_state != VisualElementState::ErrorResolved)
        {
            NotifyInteropOfStateChange(VisualElementState::ErrorResourceNotFound);
            NotifyInteropOfStateChange(VisualElementState::ErrorInvalidResource);
        }
        else if (previousState != VisualElementState::ErrorResolved)
        {
            // If we just succeeded, and we were previously invalid, notify the interop layer of the resolved error.
            NotifyInteropOfStateChange(VisualElementState::ErrorResolved);
        }

        return S_OK;
    }

    void ResourceDependency::MarkInvalid(VisualElementState state)
    {
        ASSERT(state != VisualElementState::ErrorResolved);
        m_state = state;
        NotifyInteropOfStateChange(state);
    }

    _Check_return_ HRESULT ResourceDependency::TryConvertValue(xref_ptr<CDependencyObject>& value, _Out_opt_ bool* succeeded) const
    {
        const auto propertyType = GetActualTargetPropertyType();

        KnownTypeIndex actualTypeIndex;
        IFC_RETURN(GetActualResourceTypeIndex(value.get(), &actualTypeIndex));
        const bool isDirectlyAssignable = DirectUI::MetadataAPI::IsAssignableFrom(propertyType->GetIndex(), actualTypeIndex);

        // If not directly assignable, see if this could be repackaged. Only do this for Setter.Value, as this is the only place where this would
        // happen. Other properties would be invalid

        bool canConvert = false;
        if (!isDirectlyAssignable && propertyType->HasTypeConverter() && propertyType->IsBuiltinType())
        {
            if (actualTypeIndex == KnownTypeIndex::NullKeyedResource)
            {
                canConvert = true;
                value.reset();
            }

            if (!canConvert)
            {
                CValue resource;
                resource.WrapObjectNoRef(value.get());
                ValueBuffer buffer(value->GetContext());

                CValue* repackagedValue = nullptr;
                const auto prop = GetActualTargetProperty();
                if (SUCCEEDED(buffer.RepackageValueAndSetPtr(prop, &resource, &repackagedValue)) && repackagedValue)
                {
                    canConvert = SUCCEEDED(CDependencyObject::ValidateCValue(prop, *repackagedValue, prop->GetStorageType()));
                }

                if (canConvert && repackagedValue->GetType() != valueObject)
                {
                    // We weren't able to repackage the value, see if we can invoke the type converter
                    CREATEPARAMETERS cp(value->GetContext(), resource);
                    const CREATEPFN pfnCreate = c_aTypeActivations[static_cast<uint32_t>(propertyType->GetIndex())].m_pfnCreate;
                    IFC_RETURN(pfnCreate(value.ReleaseAndGetAddressOf(), &cp));
                }
                else if (canConvert && repackagedValue->GetType() == valueObject)
                {
                    value.reset(repackagedValue->As<valueObject>());
                }
            }
        }
        if (succeeded) *succeeded = canConvert;
        return S_OK;
    }

    const CDependencyProperty*  ResourceDependency::GetActualTargetProperty() const
    {
        auto propertyIndex = m_propertyIndex;
        // Setter.Value can take anything, make sure that the propertythe setter applies to is valid for this resolved resource
        if (propertyIndex == KnownPropertyIndex::Setter_Value)
        {
            propertyIndex = DiagnosticsInterop::GetSetterPropertyIndex(checked_cast<CSetter>(GetDependency()));
        }

        return DirectUI::MetadataAPI::GetPropertyByIndex(propertyIndex);
    }

    const CClassInfo* ResourceDependency::GetActualTargetPropertyType() const
    {
        return GetActualTargetProperty()->GetPropertyType();
    }

    _Check_return_ HRESULT ResourceDependency::IsValidPropertyType(_In_ const CDependencyObject* const resourceValue, _Out_ bool* isValid) const
    {
        const auto propertyType = GetActualTargetPropertyType();
        KnownTypeIndex actualTypeIndex;
        IFC_RETURN(GetActualResourceTypeIndex(resourceValue, &actualTypeIndex));
        *isValid =  DirectUI::MetadataAPI::IsAssignableFrom(propertyType->GetIndex(), actualTypeIndex);
        return S_OK;
    }

    _Check_return_ HRESULT ResourceDependency::UpdateValue(_In_ IInspectable* newValue)
    {
        ASSERT(!Expired());
        IFC_RETURN(FixDependencyAndProperty());

        auto dependentItem = m_dependentItem.lock();
        bool succeeded = true;
        if (m_resourceType == ResourceTypeTheme)
        {
            succeeded = SUCCEEDED(DiagnosticsInterop::UpdateThemeResourceValue(
                m_dependentItem.lock().get(),
                DiagnosticsInterop::ConvertToCore(newValue).get(),
                m_propertyIndex));
        }
        else
        {
            ASSERT(m_resourceType == ResourceTypeStatic);
            ctl::ComPtr<IInspectable> dependencyPeer;
            if (SUCCEEDED(DXamlServices::GetPeer(dependentItem.get(), IID_PPV_ARGS(&dependencyPeer))) && dependencyPeer)
            {
                succeeded = SUCCEEDED(Diagnostics::DiagnosticsInterop::SetPropertyValue(
                    dependencyPeer.Get(),
                    static_cast<int>(m_propertyIndex),
                    newValue,
                    false));
            }
        }

        const auto previousState = m_state;
        m_state = succeeded ? VisualElementState::ErrorResolved : VisualElementState::ErrorInvalidResource;
        // update the state and if invalid, notify the interop layer of our newly invalid errors.
        if (m_state == VisualElementState::ErrorInvalidResource)
        {
            NotifyInteropOfStateChange(VisualElementState::ErrorResourceNotFound);
        }
        else if (previousState != VisualElementState::ErrorResolved)
        {
            // If we just succeeded, and we were previously invalid, notify the interop layer of the resolved error.
            NotifyInteropOfStateChange(VisualElementState::ErrorResolved);
        }
        return S_OK;
    }

    _Check_return_ HRESULT ResourceDependency::GetActualResourceTypeIndex(_In_ const CDependencyObject* const resourceValue, _Out_ KnownTypeIndex* typeIndex) const
    {
        ctl::ComPtr<IInspectable> value;
        CValue resource;

        resource.WrapObjectNoRef(resourceValue);
        IFC_RETURN(CValueBoxer::UnboxObjectValue(&resource, nullptr, &value));

        // Value Converters are a bit special, since they are custom supplied by the user,
        // their type index is custom and the MetadataAPI::IsAssignableFrom fails since the
        // Converter properties expect this to be KnownTypeIndex::IValueConverter
        auto valueConverter = value.AsOrNull<xaml_data::IValueConverter>();
        if (valueConverter)
        {
            *typeIndex = KnownTypeIndex::IValueConverter;
            return S_OK;
        }

        const CClassInfo* classInfo = nullptr;
        IFC_RETURN(MetadataAPI::GetClassInfoFromObject_ResolveWinRTPropertyOtherType(value.Get(), &classInfo));
        *typeIndex = classInfo->GetIndex();
        return S_OK;
    }

    void ResourceDependency::NotifyInteropOfStateChange(
        VisualElementState stateToMatch)
    {
        if ((m_state & stateToMatch) == stateToMatch)
        {
            auto prop = DirectUI::MetadataAPI::GetPropertyByIndex(m_propertyIndex);
            const auto diagInterop = Diagnostics::GetDiagnosticsInterop(true);
            diagInterop->OnElementStateChanged(stateToMatch, m_dependentItem.lock_noref(), prop);
        }
    }

    bool operator<(const std::shared_ptr<ResourceDependency>& lhs, const ResourceDependency& rhs)
    {
        ASSERT(lhs != nullptr);
        return *lhs < rhs;
    }
    bool operator<(const ResourceDependency& lhs, const std::shared_ptr<ResourceDependency>& rhs)
    {
        ASSERT(rhs != nullptr);
        return lhs < *rhs;
    }

    bool operator==(const std::shared_ptr<ResourceDependency>& lhs, const ResourceDependency& rhs)
    {
        ASSERT(lhs != nullptr);
        return *lhs == rhs;
    }

    bool operator==(const ResourceDependency& lhs, const std::shared_ptr<ResourceDependency>& rhs)
    {
        ASSERT(rhs != nullptr);
        return lhs == *rhs;
    }

    // When there is an optimized style, the style and property are stored in the resource dependency.
    // To avoid possible perf regressions we'll resolve the setter when we try to re-resolve the dependency
    _Check_return_ HRESULT ResourceDependency::FixDependencyAndProperty()
    {
        auto style = do_pointer_cast<CStyle>(m_dependentItem.lock());
        if (style)
        {
            bool hasProperty = false;
            IFC_RETURN(style->HasPropertySetter(m_propertyIndex, &hasProperty));
            if (hasProperty)
            {
                CValue setterCollectionValue;
                IFC_RETURN(style->GetValue(DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::Style_Setters), &setterCollectionValue));
                auto setterCollection = checked_cast<CSetterBaseCollection>(setterCollectionValue.AsObject());
                for (const auto& setter : *setterCollection)
                {
                    KnownPropertyIndex setterIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
                    IFC_RETURN(checked_cast<CSetter>(setter)->GetProperty(style->GetTargetType()->GetIndex(), &setterIndex));
                    if (setterIndex == m_propertyIndex)
                    {
                        m_dependentItem = xref::get_weakref(setter);
                        m_propertyIndex = KnownPropertyIndex::Setter_Value;
                    }
                }
            }
        }
        return S_OK;
    }
}


