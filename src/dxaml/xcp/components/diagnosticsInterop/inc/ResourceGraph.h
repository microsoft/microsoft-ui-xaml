// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <map>
#include <set>
#include <stack>
#include <base\inc\weakref_ptr.h>
#include <Style.h>
#include <collection\inc\SetterBaseCollection.h>
#include <Resources.h>
#include "ResourceDependency.h"
#include "base\inc\vector_set.h"
#include "StyleContext.h"

enum class KnownPropertyIndex : UINT16;
enum ResourceType;
class xstring_ptr;
class CTemplateContent;
class CMarkupExtensionBase;

namespace DirectUI {
    class DependencyObject;
}

namespace Diagnostics
{
    CDependencyObject* GetParentForElementStateChanged(
        _In_ CDependencyObject* child);

    struct ResourceGraphKey
    {
        ResourceGraphKey(
            _In_ CResourceDictionary* dictionary,
            const xstring_ptr& key)
            : Dictionary(xref::get_weakref(dictionary))
            , Key(key)
        {
        }
        ResourceGraphKey() = default;
        xref::weakref_ptr<CResourceDictionary> Dictionary;
        xstring_ptr                            Key;
    };

    struct ResourceGraphKeyWithParent : public ResourceGraphKey
    {
        ResourceGraphKeyWithParent(
            _In_ CResourceDictionary* dictionary,
            const xstring_ptr& key);

        ResourceGraphKeyWithParent() = default;
        xref_ptr<CResourceDictionary>   Parent;
    };

    struct ResourceGraphKeyComparer
    {
        bool operator()(const ResourceGraphKey& left, const ResourceGraphKey& right) const
        {
            // If the same dictionary, then compare keys.
            if (left.Dictionary == right.Dictionary)
            {
                return left.Key.Compare(right.Key) < 0;
            }

            return left.Dictionary < right.Dictionary;
        }
    };

    using ResourceDependencyList = containers::vector_set<std::shared_ptr<ResourceDependency>>;

    class __declspec(uuid("f9d7d115-0726-4e92-a240-14b19b111d89")) ResourceGraph final
    {
    public:
        // Registers the dependency, doesn't add it the resolved map. This is used
        // during parse time where we don't want to pay the extra hit of building our
        // graph before it's needed.
        void RegisterResourceDependency(
            _In_ CDependencyObject* pFrameworkDependency,
            KnownPropertyIndex propertyIndex,
            _In_ CResourceDictionary* pResourceDictionary,
            const xstring_ptr& resourceKey,
            ResourceType dependencyType);

        // Registers an existing ResourceDependency with the dictionary and key. Will remove
        // the current dependency if found.
        void RegisterResourceDependency(
            const std::shared_ptr<ResourceDependency>& dependency,
            _In_ CResourceDictionary* pResourceDictionary,
            const xstring_ptr& resourceKey);

        void RegisterControlTemplateDependency(
            _In_ CControlTemplate* controlTemplate,
            _In_ CResourceDictionary* pResourceDictionary,
            const xstring_ptr& resourceKey);

        void UnregisterResourceDependency(
            _In_ CDependencyObject* pFrameworkDependency,
            KnownPropertyIndex propertyIndex);

        std::shared_ptr<ResourceDependency> GetResourceDependency(
            _In_ xaml::IDependencyObject* resolutionContext,
            ResourceType resourceType,
            KnownPropertyIndex propertyIndex);

        std::shared_ptr<ResourceDependency> TryGetResourceDependency(
            _In_ CDependencyObject* dependentItem,
            KnownPropertyIndex propertyIndex);

        _Check_return_ HRESULT ReplaceResource(_In_ CResourceDictionary* pOldResourceDictionary, _In_ IInspectable* pKey, _In_ IInspectable* pNewResource);

        ResourceDependencyList GetDependentItems(
            _In_ const ResourceGraphKey& key);

        void RegisterBasedOnStyleDependency(_In_ CStyle* pBase);
        void UnregisterBasedOnStyleDependency(_In_ CStyle* pBase);
        void GetAllDependentStyles(_In_ CStyle* pStyle, _Inout_ std::vector<CStyle*>& dependentStyles);
        _Check_return_ HRESULT RebuildMergedSetters(_In_ CStyle* pStyle, _Inout_ std::vector<CStyle*>& changedStyles);

        void RegisterStyleAndSetterCollection(_In_ CStyle* pStyle, _In_ CSetterBaseCollection* pCollection);
        CStyle* GetOwningStyle(_In_ CSetterBaseCollection* pCollection);

        // Clears the cached contexts for this ResourceGraph. Called from XamlDiagnostics when a core is shutdown.
        void ClearCachedContext();

        void CacheRuntimeContext(_In_ CStyle* style, _In_ std::unique_ptr<CustomWriterRuntimeContext> context);
        CustomWriterRuntimeContext* GetCachedRuntimeContext(_In_ CStyle* style);

        void CacheMarkupExtensionToTarget(_In_ CMarkupExtensionBase* ext, _In_ KnownPropertyIndex targetProp, _In_ CDependencyObject* targetObj);
        void RemoveMarkupExtensionToTarget(_In_ CDependencyObject* targetObj, _In_ KnownPropertyIndex targetProp);

        KnownPropertyIndex GetTargetProperty(_In_ CMarkupExtensionBase* ext);
        xref_ptr<CDependencyObject> GetTargetObject(_In_ CMarkupExtensionBase* ext);
        bool HasTarget(_In_ CMarkupExtensionBase* ext);

        void AddStyleContext(_In_ CStyle* pStyle, _In_ CDependencyObject* pDict, bool isImplicit);
        _Check_return_ HRESULT TryRefreshImplicitStyle(_In_ CStyle* pStyle, _Out_ bool* pWasImplicit);

        void AddStyleContext(_In_ CStyle* pStyle, _In_ CResourceDictionary* pDict);
        xref_ptr<CDependencyObject> GetParent(_In_ CStyle* styleParent, _Out_opt_ bool* wasImplicit = nullptr);
        void RemoveStyleContext(_In_ CStyle* pStyle);

    private:
#pragma region Private methods
        void ResolveAllResourceDependencies();

        _Check_return_ HRESULT UpdateResourceReferences(
            _In_ CResourceDictionary* resourceDictionary,
            const xstring_ptr& key,
            _In_ IInspectable* pNewResource);

        // Add's the dependencies to the resolved map.
        void AddToResolvedMap(
            const ResourceGraphKey& key,
            const std::shared_ptr<ResourceDependency>& dependency);

        std::shared_ptr<ResourceDependency> GetResourceDependencyImpl(
            _In_ CDependencyObject* dependentItem,
            KnownPropertyIndex propertyIndex,
            ResourceType resoureType);
#pragma endregion
    private:
#pragma region Private members
        // Visual Diagnostics resource referential graph
        // ResourceGraph: a mapping of static resources to objects/properties that use them
        // Visual Diagnostics resource graph typedefs

        using ResourceGraphMap = std::map<ResourceGraphKey, ResourceDependencyList, ResourceGraphKeyComparer>;
        ResourceGraphMap m_pResourceMap;
        //A stack of deferred resource key/values we'll resolve and add to the resource graph
        //when we're sure XamlDiagnostics is enabled
        std::stack<std::tuple<ResourceGraphKey, ResourceDependency>> m_resourceStack;

        //Mapping of Styles to all other Styles which are directly based on them
        std::map<xref::weakref_ptr<CStyle>, std::set<xref::weakref_ptr<CStyle>>> m_dependentStyleMap;

        //Mapping of setter collections to the Styles that use them as their Style.Setters property
        std::map<xref::weakref_ptr<CSetterBaseCollection>, xref::weakref_ptr<CStyle>> m_setterCollectionDependentMap;

        // Mapping of parser context to the style that it refers to.
        std::map<xref::weakref_ptr<CStyle>, StyleContext> m_styleContext;

        // Map of template to the pre-resolved dictionary and key. We use the ControlTemplate because
        // at pre-resolve time the CTemplateContent hasn't been assigned to the CControlTempalet yet
        std::multimap<xref::weakref_ptr<CControlTemplate>, ResourceGraphKey> m_preResolveDictionaryMap;

        using MarkupExtensionTargetMap = std::map<xref::weakref_ptr<CMarkupExtensionBase>, std::pair<KnownPropertyIndex, xref::weakref_ptr<CDependencyObject>>>;
        MarkupExtensionTargetMap m_markupExtensionMap;


#pragma endregion
    };

    std::shared_ptr<ResourceGraph> GetResourceGraph();
}
