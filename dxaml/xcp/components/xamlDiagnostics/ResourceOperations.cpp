// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ResourceOperations.h"
#include "DiagnosticsInterop.h"
#include "ResourceGraph.h"
#include "RuntimeObjectCache.h"
#include "RuntimeObject.h"
#include "RuntimeProperty.h"
#include "RuntimeElement.h"
#include "xstring_ptr.h"
#include "resources.h"
#include "DXamlServices.h"

namespace Diagnostics { namespace ResourceOperations {

    _Check_return_ HRESULT UpdateDependentItem(
        const std::shared_ptr<Diagnostics::ResourceDependency>& dependency,
        const xstring_ptr& key,
        const xref_ptr<CResourceDictionary>& dictionaryForResolution);

    _Check_return_ HRESULT UpdateDependentItemsOnAdd(const ResourceGraphKey& currentResolution, _In_ xaml::IResourceDictionary* newDictionary)
    {
        // Re-resolve items
        auto resourceGraph = GetResourceGraph();

        // Get items dependent on the dictionary that this would currently resolve to.
        for (const auto& item : resourceGraph->GetDependentItems(currentResolution))
        {
            // If the dependent item is a key in the dictionary, we want to pass in the dictionaries peer
            // so that the key will resolve correctly. Imagine we have a color defined in App.Xaml like this:
            //   <Application.Resources>
            //      <Color x:Key="MyColor">Red</Color>
            //   </Application.Resources>

            // If we update MainPage.xaml from:
            //   <Page.Resources>
            //     <SolidColorBrush x:Key="Foo" Color="{StaticResource MyColor"} />
            //   <Page.Resources>
            // to:
            //   <Page.Resources>
            //     <SolidColorBrush x:Key="Foo" Color="{StaticResource MyColor"} />
            //     <Color x:Key="MyColor">Blue</Color>
            //   <Page.Resources>

            // We want to look in the Page.Resources dictionary for MyColor. Other things such as Setters will
            // use the cached context for lookup, or for resolving an object in MainPage.xaml, we'll be able
            // to walk up the tree and resolve the value once it's found on Page.Resources.
            auto dictionary = DiagnosticsInterop::GetDictionaryIfResource(newDictionary, item->GetDependency());
            IFC_RETURN(UpdateDependentItem(item, currentResolution.Key, dictionary));
        }
        return S_OK;
    }

    _Check_return_ HRESULT UpdateDependentItemsOnRemove(
        const ResourceGraphKeyWithParent& removedGraphKey)
    {
        auto resourceGraph = GetResourceGraph();
        for (const auto& item : resourceGraph->GetDependentItems(removedGraphKey))
        {
            IFC_RETURN(UpdateDependentItem(item, removedGraphKey.Key, removedGraphKey.Parent));
        }
        return S_OK;
    }

    _Check_return_ HRESULT UpdateDependentItem(
        const std::shared_ptr<Diagnostics::ResourceDependency>& dependency,
        const xstring_ptr& key,
        const xref_ptr<CResourceDictionary>& dictionaryForResolution)
    {
        wrl::ComPtr<IInspectable> peer;
        IFC_RETURN(DirectUI::DXamlServices::GetPeer(dependency->GetDependency().get(), IID_PPV_ARGS(&peer)));

        wrl::ComPtr<xaml::IUIElement> resolutionContext;

        std::shared_ptr<RuntimeObject> dependentObject;
        if (GetRuntimeObjectCache()->TryFindInCache(peer.Get(), dependentObject))
        {
            std::shared_ptr<RuntimeElement> runtimeElementContext;

            // If the dependent item is in the tree, use that as resolution context, otherwise
            // try to use the parent since this item is something like a Brush
            if (!dependentObject->TryGetAsElement(runtimeElementContext))
            {
                std::shared_ptr<RuntimeObject> dependentItemOwner;
                if (dependentObject->TryGetParent(dependentItemOwner))
                {
                    dependentItemOwner->TryGetAsElement(runtimeElementContext);
                }
            }

            if (runtimeElementContext)
            {
                resolutionContext = runtimeElementContext->GetBackingElement();
            }
        }
        
        wrl::ComPtr<IInspectable> resolvedValue;
        IFC_RETURN(DiagnosticsInterop::ResolveResource(
            dependency,
            dictionaryForResolution,
            key,
            resolutionContext.Get(),
            resolvedValue));

        RuntimeProperty prop(dependency->GetPropertyIndex());

        // If we didn't find the object in our cache, then we don't need to worry about creating
        // it or keeping it around as the caller hasn't queried for it and/or it isn't in the LVT.
        if (dependency->IsValid() && dependentObject)
        {
            dependentObject->StoreValue(prop, GetRuntimeObject(resolvedValue.Get(), dependentObject));
        }
        else if (dependentObject)
        {
            dependentObject->RemoveValue(prop);
        }
        return S_OK;
    }
}}