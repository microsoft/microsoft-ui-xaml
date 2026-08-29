// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <base\inc\weakref_ptr.h>

enum class KnownPropertyIndex : UINT16;
enum class KnownTypeIndex : UINT16;
enum ResourceType;
enum VisualElementState;
class xstring_ptr;
class CResourceDictionary;
class CDependencyProperty;
class CThemeResourceExtension;
class CClassInfo;

namespace DirectUI {
    class DependencyObject;
}

namespace Resources {
    struct ResolvedResource;
}

namespace Diagnostics
{
    // ResourceDependency is a class that encapsulates a dependency on a resource at runtime. It contains
    // the element with the dependency, the index of the property, and the resource type. In this trivial example,
    // <Button Background="{StaticResource Foo}"/>, the ResourceDependency would contain a weak reference to 
    // the CButton, the PropertyIndex for Control_Background, and a resource type of ResourceTypeStatic.
    class ResourceDependency final : public std::enable_shared_from_this<ResourceDependency>
    {
    public:
        explicit ResourceDependency(
            _In_ CDependencyObject* dependency,
            KnownPropertyIndex index,
            ResourceType type);

        bool                        operator<(const ResourceDependency& rhs) const;
        bool                        operator==(const ResourceDependency& rhs) const;
        bool                        Expired() const;
        bool                        IsValid() const;
        xref_ptr<CDependencyObject> GetDependency() const;
        KnownPropertyIndex          GetPropertyIndex() const;
        ResourceType                GetType() const;

        // Resolves the value for this dependency with the given key, and an optional
        // dictionary if the dependency was found in one.
        _Check_return_ HRESULT Resolve(
                                        _In_opt_ CResourceDictionary* dictionary,
                                        const xstring_ptr& key,
                                        _In_opt_ CDependencyObject* resourceContext,
                                        _Out_ Resources::ResolvedResource& reolvedResource);

        _Check_return_ HRESULT      UpdateValue(_In_ IInspectable* newValue);

        void                        MarkInvalid(VisualElementState state);
    private:
        // Private helper methods for notifying of resource resolution errors and resolved errors.
        // compares the current state of the ResourceDependency against the stateToMatch and if matches,
        // notifies interop of the Error/Resolution with the passed in error and property name
        void                        NotifyInteropOfStateChange(
                                        VisualElementState stateToMatch);

        void                        NotifyInteropOfResolvedError(
                                        VisualElementState stateToMatch);

        _Check_return_ HRESULT      FixDependencyAndProperty();
        _Check_return_ HRESULT      GetActualResourceTypeIndex(_In_ const CDependencyObject* const resourceValue, _Out_ KnownTypeIndex* typeIndex) const;
        const CClassInfo*           GetActualTargetPropertyType() const;
        const CDependencyProperty*  GetActualTargetProperty() const;
        _Check_return_ HRESULT      IsValidPropertyType(_In_ const CDependencyObject* const resourceValue, _Out_ bool* isValid) const;
        _Check_return_ HRESULT      TryConvertValue(xref_ptr<CDependencyObject>& value, _Out_opt_ bool* succeeded) const;
    private:
        xref::weakref_ptr<CDependencyObject>    m_dependentItem;
        KnownPropertyIndex                      m_propertyIndex : 16;
        ResourceType                            m_resourceType  : 14;
        VisualElementState                      m_state         : 2;
    };

    // Forward declare comparison operators
    bool operator<(const std::shared_ptr<ResourceDependency>& lhs, const ResourceDependency& rhs);
    bool operator<(const ResourceDependency& lhs, const std::shared_ptr<ResourceDependency>& rhs);
    bool operator==(const std::shared_ptr<ResourceDependency>& lhs, const ResourceDependency& rhs);
    bool operator==(const ResourceDependency& lhs, const std::shared_ptr<ResourceDependency>& rhs);
}
