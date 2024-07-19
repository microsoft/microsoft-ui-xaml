// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "inc\ResourceGraph.h"
#include "inc\ResourceDependency.h"
#include "inc\PropertyChainIterator.h"
#include "inc\StyleContext.h"
#include "Indexes.g.h"
#include "TypeTableStructs.h"
#include "MetadataAPI.h"
#include <DiagnosticsInterop.h>
#include "dependencyLocator\inc\DependencyLocator.h"
#include "DXamlServices.h"
#include <DependencyObject.h>
#include "DOPointerCast.h"
#include "framework.h"
#include "corep.h"
#include "resources.h"
#include "ThemeResourceExtension.h"
#include "resources\inc\ResourceResolver.h"
#include "CControl.h"
#include "Template.h"
#include "TemplateContent.h"
#include "MultiParentShareableDependencyObject.h"
#include "xamldiagnostics\inc\RuntimeObject.h"

namespace Diagnostics
{
    ResourceGraphKeyWithParent::ResourceGraphKeyWithParent(
        _In_ CResourceDictionary* dictionary,
        const xstring_ptr& key)
        : ResourceGraphKey(dictionary, key)
        , Parent(DiagnosticsInterop::GetImmediateParentDictionary(dictionary))
    {
    }

    //Removes a dependency from the StaticResource graph.  Called when Visual Studio overwrites a property
    //which may have been set to a StaticResource lookup.
    void ResourceGraph::UnregisterResourceDependency(
        _In_ CDependencyObject* pFrameworkDependency,
        _In_ KnownPropertyIndex propertyIndex)
    {
        //Check our deferred stack for any resources we haven't added to the graph yet, and add them
        ResolveAllResourceDependencies();

        //The resource graph maps static resources to lists of objects/properties that use it.
        //So to unregister a dependency, we have to loop through all the resources and remove the
        //input object/index from the list if they exist.

        ResourceGraphMap::iterator dependencyListIterator = m_pResourceMap.begin();
        ResourceDependency listItem(pFrameworkDependency, propertyIndex, ResourceTypeStatic);

        std::shared_ptr<ResourceDependency> removedDependency;
        // Iterate through all resources and get the lists of objects depending on them.
        while (dependencyListIterator != m_pResourceMap.end() && !removedDependency)
        {
            //Check if the dependency exists - if so, remove it
            //If deleting it makes the dependency list empty, also remove the static resource
            //from the graph
            auto it = std::find(dependencyListIterator->second.begin(), dependencyListIterator->second.end(), listItem);
            if (it != dependencyListIterator->second.end())
            {
                removedDependency = std::move(*it);
                dependencyListIterator->second.erase(it);
            }

            if (dependencyListIterator->second.empty())
            {
                //If this resource is no longer used by anything, delete it from the graph
                dependencyListIterator = m_pResourceMap.erase(dependencyListIterator);
            }
            else
            {
                ++dependencyListIterator;
            }
        }
    }

    // Registers that a dependency object and property reference an element in a resource dictionary.
    // This function does not actually construct the graph used by XamlDiagnostics, since we only need to build
    // it if XamlDiag is enabled.  However, we cannot only register these dependencies when XamlDiag is enabled
    // since it may not be enabled in the runtime feature detector by the time we need to register these dependencies.
    void ResourceGraph::RegisterResourceDependency(
        _In_ CDependencyObject* pFrameworkDependency,
        KnownPropertyIndex propertyIndex,
        _In_ CResourceDictionary* pResourceDictionary,
        const xstring_ptr& resourceKey,
        ResourceType dependencyType)
    {
        //If the property index we've gotten is a custom property, we need the custom dependency property instead.
        //GetUnderlyingDependencyProperty will give us the right custom dependency property (for custom dependency
        //properties or non-custom stuff, it will just return the correct input index).
        //Note the method/type name is misleading, as a CDependencyProperty can represent either
        //an actual dependency property or a non-dependency property.
        auto pProperty = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyIndex);

        {
            const CDependencyProperty* pDP = nullptr;
            if (SUCCEEDED(DirectUI::MetadataAPI::TryGetUnderlyingDependencyProperty(pProperty, &pDP)) &&
                pDP != nullptr)
            {
                pProperty = pDP;
            }
        }

        // Templates should be registered through RegisterControlTemplateDependency
        ASSERT(!pFrameworkDependency->OfTypeByIndex<KnownTypeIndex::ControlTemplate>() && !pFrameworkDependency->OfTypeByIndex<KnownTypeIndex::DataTemplate>());

        // If our property is valid (DP or non-DP), register the dependency.
        // IsInvalidProperty normally considers custom non-dependency properties to be invalid, but
        // for our resource dependency tracking they're OK, so allow them through
        if (!Diagnostics::PropertyChainIterator::IsInvalidProperty(pProperty, true /*allowCustomProperties*/))
        {
            ResourceGraphKey graphKey(pResourceDictionary, resourceKey);

            // Check if the DO is part of a template. If so, then this resource was pre-resolved and the
            // dictionary passed in here isn't the public facing dictionary. Let's fix that up real nice
            // and quick like!
            auto templatedParent = do_pointer_cast<CFrameworkElement>(pFrameworkDependency->GetTemplatedParent());
            if (templatedParent)
            {
                auto controlTemplate = templatedParent->GetTemplate();
                if (controlTemplate)
                {
                    // Find the matching key for this object
                    auto range = m_preResolveDictionaryMap.equal_range(xref::get_weakref(controlTemplate));
                    auto iter = std::find_if(range.first, range.second,[resourceKey](const auto& entry){
                        return resourceKey == entry.second.Key;
                    });

                    if (iter != range.second)
                    {
                        // Found the match, fix the dictionary
                        graphKey.Dictionary = iter->second.Dictionary;
                    }

                    // We keep the item in the m_preResolveDictionaryMap around because future template instantiations could run
                    // through this code again. Those objects will always resolve to the internal dictionary, so we must keep our
                    // wits about us
                }
            }

             m_resourceStack.emplace(std::move(graphKey),  ResourceDependency(pFrameworkDependency, pProperty->GetIndex(), dependencyType));
        }
    }

    void ResourceGraph::RegisterResourceDependency(
        const std::shared_ptr<ResourceDependency>& dependency,
        _In_ CResourceDictionary* pResourceDictionary,
        const xstring_ptr& resourceKey)
    {
        // No need to resolve all the dependencies at this point, we have an existing ResourceDependency,
        // so we already had to resolve the outstanding dependencies to get it.
        ASSERT(!dependency->Expired());

        // Unregister the current dependency if this property was previously registered
        UnregisterResourceDependency(dependency->GetDependency().get(), dependency->GetPropertyIndex());

        // Add this to the resolved map, rather than the pending one.
        ResourceGraphKey graphKey(pResourceDictionary, resourceKey);
        AddToResolvedMap(graphKey, dependency);
    }

    // The first time a template is created, it pre-resolves all the resources and caches it in a local, private
    // dictionary. This is done because we don't want future instantiations of the template to change if the developer
    // modified the dictionary in code-behind.
    void ResourceGraph::RegisterControlTemplateDependency(
            _In_ CControlTemplate* templateContent,
            _In_ CResourceDictionary* pResourceDictionary,
            const xstring_ptr& resourceKey)
    {
        m_preResolveDictionaryMap.emplace(xref::get_weakref(templateContent), ResourceGraphKey(pResourceDictionary, resourceKey));
    }

    // TODO: make this actually replace the resource in the dictionary and remove the DiagnosticsInterop method
    _Check_return_ HRESULT ResourceGraph::ReplaceResource(
        _In_ CResourceDictionary* pOldResourceDictionary,
        _In_ IInspectable* pKey,
        _In_ IInspectable* pNewResource)
    {
        wrl_wrappers::HString buf;
        xstring_ptr strKey;

        IFC_RETURN(ctl::do_get_value(*buf.GetAddressOf(), pKey));
        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(buf.Get(), &strKey));
        IFC_RETURN(UpdateResourceReferences(pOldResourceDictionary, strKey, pNewResource));
        return S_OK;
    }

    _Check_return_ HRESULT ResourceGraph::UpdateResourceReferences(
        _In_ CResourceDictionary* resourceDictionary,
        const xstring_ptr& strKey,
        _In_ IInspectable* pNewResource)
    {
        //Check our deferred stack for any resources we haven't added to the graph yet, and add them
        ResolveAllResourceDependencies();

        ResourceGraphKey graphKey(resourceDictionary, strKey);
        auto dependencyListIterator = m_pResourceMap.find(graphKey);
        //Look up the resource being replaced in the resource dictionary
        if (dependencyListIterator != m_pResourceMap.end())
        {
            //For each DO/property that depends on that resource, update its property value to the new resource
            auto it = dependencyListIterator->second.begin();

            while (it != dependencyListIterator->second.end())
            {
                std::shared_ptr<ResourceDependency> curItem = *it;

                //Make sure the old resource's dependencies still exist before manipulating them (they may have been garbage collected)
                if (!curItem->Expired())
                {
                    // Update value on the dependency's property.
                    IFC_RETURN(curItem->UpdateValue(pNewResource));

                    it++;
                }
                else
                {
                    //If a framework element dependent on this resource no longer exists, it isn't an error.
                    //However we should delete it from the dependency list.
                    it = dependencyListIterator->second.erase(it);
                }

            }

            if (dependencyListIterator->second.empty())
            {
                //If this resource is no longer used by anything, delete it from the graph
                m_pResourceMap.erase(dependencyListIterator);
            }
        }

        // If the resource Visual Studio is trying to replace wasn't registered as a StaticResource, then that's fine.
        // We don't give them an "IsResourceRegistered" API to know if a resource is registered.
        return S_OK;
    }

    void ResourceGraph::RegisterBasedOnStyleDependency(_In_ CStyle* pBase)
    {
        if (pBase->m_pBasedOn != nullptr)
        {
            xref::weakref_ptr<CStyle> weakBasedOnPointer = xref::get_weakref<CStyle>(pBase->m_pBasedOn);

            //Assume the set doesn't exist yet - make a new empty set and insert it.
            //Insert will return the iterator to the set we wanted to insert into, regardless of
            //if it's the empty one we just made or a pre-existing one.
            std::set<xref::weakref_ptr<CStyle>> newSet;
            auto iteratorFoundPair = m_dependentStyleMap.insert(std::make_pair(weakBasedOnPointer, newSet));

            //Get the iterator as the first member of the pair, and from that get the dependent Style set
            //which is the second member of the iterator.
            iteratorFoundPair.first->second.insert(xref::get_weakref<CStyle>(pBase));
        }
    }

    void ResourceGraph::UnregisterBasedOnStyleDependency(_In_ CStyle* pBase)
    {
        if (pBase->m_pBasedOn != nullptr)
        {
            xref::weakref_ptr<CStyle> weakBasedOnPointer = xref::get_weakref<CStyle>(pBase->m_pBasedOn);
            auto it = m_dependentStyleMap.find(weakBasedOnPointer);
            if (it != m_dependentStyleMap.end())
            {
                it->second.erase(xref::get_weakref<CStyle>(pBase));
            }
        }
    }

    void ResourceGraph::GetAllDependentStyles(_In_ CStyle* pStyle, _Inout_ std::vector<CStyle*>& dependentStyles)
    {
        dependentStyles.push_back(pStyle);

        xref::weakref_ptr<CStyle> weakStylePointer = xref::get_weakref<CStyle>(pStyle);
        auto it = m_dependentStyleMap.find(weakStylePointer);
        if (it != m_dependentStyleMap.end())
        {
            for (auto weakRef : it->second)
            {
                xref_ptr<CStyle> pStyleDependent = weakRef.lock();
                if (pStyleDependent.get() != nullptr)
                {
                    ASSERT(pStyleDependent.get()->m_pBasedOn == pStyle);
                    GetAllDependentStyles(pStyleDependent.get(), dependentStyles);
                }
            }
        }
    }

    _Check_return_ HRESULT ResourceGraph::RebuildMergedSetters(_In_ CStyle* pStyle, _Inout_ std::vector<CStyle*>& changedStyles)
    {
        changedStyles.push_back(pStyle);
        IFC_RETURN(pStyle->RefreshSetterCollection());

        xref::weakref_ptr<CStyle> weakStylePointer = xref::get_weakref<CStyle>(pStyle);
        auto it = m_dependentStyleMap.find(weakStylePointer);
        if (it != m_dependentStyleMap.end())
        {
            for (auto weakRef : it->second)
            {
                xref_ptr<CStyle> pStyleDependent = weakRef.lock();
                if (pStyleDependent.get() != nullptr)
                {
                    ASSERT(pStyleDependent.get()->m_pBasedOn == pStyle);
                    IFC_RETURN(RebuildMergedSetters(pStyleDependent.get(), changedStyles));
                }
            }
        }

        return S_OK;
    }

    void ResourceGraph::RegisterStyleAndSetterCollection(_In_ CStyle* pStyle, _In_ CSetterBaseCollection* pCollection)
    {
        m_setterCollectionDependentMap.insert(std::make_pair(xref::get_weakref<CSetterBaseCollection>(pCollection), xref::get_weakref<CStyle>(pStyle)));
    }

    void ResourceGraph::CacheRuntimeContext(_In_ CStyle* style, _In_ std::unique_ptr<CustomWriterRuntimeContext> context)
    {
        m_styleContext.emplace(xref::get_weakref<CStyle>(style), StyleContext(std::move(context)));
    }

    _Ret_maybenull_
    CustomWriterRuntimeContext* ResourceGraph::GetCachedRuntimeContext(_In_ CStyle* style)
    {
        auto iter = m_styleContext.find(xref::get_weakref<CStyle>(style));
        if (iter != m_styleContext.end())
        {
            return iter->second.GetCachedContext();
        }

        return nullptr;
    }

    void ResourceGraph::ClearCachedContext()
    {
        m_styleContext.clear();
    }

    CStyle* ResourceGraph::GetOwningStyle(_In_ CSetterBaseCollection* pCollection)
    {
        CStyle* pOwningStyle = nullptr;
        auto it = m_setterCollectionDependentMap.find(xref::get_weakref<CSetterBaseCollection>(pCollection));
        if (it != m_setterCollectionDependentMap.end())
        {
            xref_ptr<CStyle> pStyle = it->second.lock();
            pOwningStyle = pStyle.get();
        }

        return pOwningStyle;
    }

    //Gets all registered static resource dependencies so far and adds them to the resource graph used
    //by XamlDiagostics.  This will only be called when XamlDiagnostics is in-use.
    void ResourceGraph::ResolveAllResourceDependencies()
    {
        while (!m_resourceStack.empty())
        {
            std::tuple<ResourceGraphKey, ResourceDependency> stackElem = m_resourceStack.top();

            ResourceGraphKey curGraphKey = std::get<0>(stackElem);
            ResourceDependency curGraphValue = std::get<1>(stackElem);

            m_resourceStack.pop();
            // We store a shared_ptr so that we can check if resolving a reference makes it valid.
            auto resolvedValue = std::make_shared<ResourceDependency>(std::move(curGraphValue));
            AddToResolvedMap(curGraphKey, resolvedValue);
        }
    }

    ResourceDependencyList ResourceGraph::GetDependentItems(
        _In_ const ResourceGraphKey& graphKey)
    {
        ResolveAllResourceDependencies();

        auto dependentListIter = m_pResourceMap.find(graphKey);

        ResourceDependencyList dependentItems;
        if (dependentListIter != m_pResourceMap.end())
        {
            dependentItems.reserve(dependentListIter->second.size());

            auto it = dependentListIter->second.begin();
            while (it != dependentListIter->second.end())
            {
                auto item = *it;
                if (!item->Expired())
                {
                    dependentItems.emplace(item);
                    ++it;
                }
                else
                {
                    // If the weak reference can no longer be resolved, remove this from our map
                    it = dependentListIter->second.erase(it);
                }
            }

            dependentItems.shrink_to_fit();
        }

        return dependentItems;
    }

    std::shared_ptr<ResourceDependency> ResourceGraph::GetResourceDependency(
        _In_ xaml::IDependencyObject* resolutionContext,
        ResourceType resourceType,
        KnownPropertyIndex propertyIndex)
    {
        auto context = static_cast<DirectUI::DependencyObject*>(resolutionContext)->GetHandle();
        auto foundDependency = GetResourceDependencyImpl(context, propertyIndex, resourceType);

        // If this dependency hasn't been registered yet, then make a new one and return that.
        if (!foundDependency)
        {
            foundDependency = std::make_shared<ResourceDependency>(ResourceDependency(context, propertyIndex, resourceType));
        }

        return foundDependency;
    }

    std::shared_ptr<ResourceDependency> ResourceGraph::TryGetResourceDependency(
            _In_ CDependencyObject* dependentItem,
            KnownPropertyIndex propertyIndex)
    {
        auto dependency = GetResourceDependencyImpl(dependentItem, propertyIndex,ResourceTypeStatic);
        if (!dependency)
        {
            dependency = GetResourceDependencyImpl(dependentItem, propertyIndex, ResourceTypeTheme);
        }
        return dependency;
    }

    std::shared_ptr<ResourceDependency> ResourceGraph::GetResourceDependencyImpl(
            _In_ CDependencyObject* dependentItem,
            KnownPropertyIndex propertyIndex,
            ResourceType resoureType)
    {
        ResolveAllResourceDependencies();

        ResourceDependency dependency(dependentItem, propertyIndex, resoureType);
        auto dependencyListIterator = m_pResourceMap.begin();

        std::shared_ptr<ResourceDependency> foundDependency;

        // Iterate through all resources and get the lists of objects depending on them.
        while (dependencyListIterator != m_pResourceMap.end() && !foundDependency)
        {
            auto it = std::find(dependencyListIterator->second.begin(), dependencyListIterator->second.end(), dependency);
            if (it != dependencyListIterator->second.end())
            {
                foundDependency = *it;
            }
            ++dependencyListIterator;
        }

        return foundDependency;
    }

    void ResourceGraph::AddToResolvedMap(
        const ResourceGraphKey& key,
        const std::shared_ptr<ResourceDependency>& dependency)
    {
        auto dependencyListIterator = m_pResourceMap.find(key);
        //If the resource we depend on is already in the graph, add the framework element to its list of dependencies
        if (dependencyListIterator != m_pResourceMap.end())
        {
             dependencyListIterator->second.emplace(dependency);
        }
        else
        {
            //Create a new "node" (ResourceDependencyList) in the graph for the static resource if it didn't exist
            ResourceDependencyList dependencies;
            dependencies.emplace(dependency);
            m_pResourceMap.insert(std::make_pair(key, std::move(dependencies)));
        }
    }

    void ResourceGraph::CacheMarkupExtensionToTarget(_In_ CMarkupExtensionBase* ext, _In_ KnownPropertyIndex targetProp, _In_ CDependencyObject* targetObj)
    {
        // First try to remove current one
        RemoveMarkupExtensionToTarget(targetObj, targetProp);
        m_markupExtensionMap.emplace(xref::get_weakref(ext), std::make_pair(targetProp, xref::get_weakref(targetObj)));
    }

    void ResourceGraph::RemoveMarkupExtensionToTarget(_In_ CDependencyObject* targetObj, _In_ KnownPropertyIndex targetProp)
    {
        auto iter = std::find_if(m_markupExtensionMap.begin(), m_markupExtensionMap.end(), [targetObj, targetProp](const auto& pair) {
            return pair.second.second == xref::get_weakref(targetObj) && pair.second.first == targetProp;
        });

        if (iter != m_markupExtensionMap.end())
        {
            m_markupExtensionMap.erase(iter);
        }
    }

    KnownPropertyIndex ResourceGraph::GetTargetProperty(_In_ CMarkupExtensionBase* ext)
    {
        auto iter = m_markupExtensionMap.find(xref::get_weakref(ext));
        ASSERT(iter != m_markupExtensionMap.end());
        auto index = KnownPropertyIndex::UnknownType_UnknownProperty;
        if (iter != m_markupExtensionMap.end())
        {
            index = iter->second.first;
        }
        return index;
    }

    xref_ptr<CDependencyObject> ResourceGraph::GetTargetObject(_In_ CMarkupExtensionBase* ext)
    {
        auto iter = m_markupExtensionMap.find(xref::get_weakref(ext));
        ASSERT(iter != m_markupExtensionMap.end());
        xref_ptr<CDependencyObject> targetDO;
        if (iter != m_markupExtensionMap.end())
        {
            targetDO = iter->second.second.lock();
        }
        return targetDO;
    }

    bool ResourceGraph::HasTarget(_In_ CMarkupExtensionBase* ext)
    {
        auto iter = m_markupExtensionMap.find(xref::get_weakref(ext));
        return iter != m_markupExtensionMap.end();
    }

    void ResourceGraph::AddStyleContext(_In_ CStyle* style, _In_ CDependencyObject* owner, bool isImplicit)
    {
        auto result = m_styleContext.emplace(xref::get_weakref(style), StyleContext(owner, isImplicit));
        if (!result.second && isImplicit)
        {
            // This dictionary was already cached, just mark it as being implicit as well
            result.first->second.SetIsImplicit();
        }
    }

    _Check_return_ HRESULT ResourceGraph::TryRefreshImplicitStyle(_In_ CStyle* pStyle, _Out_ bool* wasImplicit)
    {
        *wasImplicit = false;
        auto it = m_styleContext.find(xref::get_weakref(pStyle));
        if (it != m_styleContext.end() && it->second.IsImplicit())
        {
            *wasImplicit = true;
            auto owningDict = do_pointer_cast<CResourceDictionary>(it->second.GetParent());
            if (owningDict != nullptr)
            {
                IFC_RETURN(owningDict->NotifyImplicitStyleChanged(pStyle, CResourceDictionary::StyleUpdateType::ForcedByDiagnotics));
            }
        }

        return S_OK;
    }

    xref_ptr<CDependencyObject> ResourceGraph::GetParent(_In_ CStyle* style, _Out_opt_ bool* wasImplicit)
    {
        auto it = m_styleContext.find(xref::get_weakref(style));
        if (it != m_styleContext.end())
        {
            if (wasImplicit) *wasImplicit = it->second.IsImplicit();
            return it->second.GetParent();
        }

        return nullptr;
    }

    void ResourceGraph::RemoveStyleContext(_In_ CStyle* style)
    {
        m_styleContext.erase(xref::get_weakref(style));
    }

    // GetParentForElementStateChanged is poorly named. Internally, MUX doesn't have a consistent parenting semantics
    // for who the parent is w.r.t. the markup tree. In the designer, it is important that we be able to associate elements
    // with how the users sees them when editing their .xaml file, and not only based on the visual tree that we have created.
    // This function essentially does that.
    CDependencyObject* GetParentForElementStateChanged(
        _In_ CDependencyObject* child)
   {
       auto parent = child->GetParentInternal(false);

       auto* childME = do_pointer_cast<CMarkupExtensionBase>(child);
       if (!parent && childME)
       {
           const auto resourceGraph = GetResourceGraph();
           parent = resourceGraph->GetTargetObject(childME);
       }

       if (!parent && child->DoesAllowMultipleParents())
       {
           // If this do has multiple parents, then grab the first one, as that will be the
           // original parent
           CMultiParentShareableDependencyObject* shareableChild = static_cast<CMultiParentShareableDependencyObject*>(child);
           if (shareableChild->GetParentCount() > 0)
           {
               parent = shareableChild->GetParentItem(0);
           }
       }

       if (!parent)
       {
           auto childStyle = do_pointer_cast<CStyle>(child);
           if (childStyle)
           {
               // Styles are CNoParentShareableDependencyObjects, so we don't have a way to get the parent.
               // Luckily, we have a cached context for styles that we can use in the resource graph to get
               // the resource dictionary it was contained in, or the element in the LVT if this style is declared
               // inline
               const auto resourceGraph = GetResourceGraph();
               parent = resourceGraph->GetParent(childStyle);
           }
           else
           {
               // Really, this should be about all we ever need to do to get the parent. The RuntimeObject cache
               // keeps the correct parent/child relation that we want for us.
               if (auto runtimeObject = TryGetRuntimeObject(child))
               {
                    std::shared_ptr<RuntimeObject> runtimeParent;
                    if (runtimeObject->TryGetParent(runtimeParent))
                    {
                        parent = DiagnosticsInterop::ConvertToCore(runtimeParent->GetBackingObject().Get());
                    }
               }
           }
       }

       if (!parent)
       {
           auto setterCollection = do_pointer_cast<CSetterBaseCollection>(child);
           if (setterCollection)
           {
               const auto resourceGraph = GetResourceGraph();
               parent = resourceGraph->GetOwningStyle(setterCollection);
           }
       }

       return parent;
   }
}
