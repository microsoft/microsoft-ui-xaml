// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlDiagnostics.h"
#include "HandleMap.h"
#include "dependencyLocator\inc\DependencyLocator.h"
#include "diagnosticsInterop\inc\ResourceGraph.h"
#include "resources\inc\ResourceResolver.h"
#include "deferral\inc\CustomWriterRuntimeContext.h"
#include "DiagnosticsInterop.h"
#include "DependencyObject.h"
#include "DXamlServices.h"
#include "ThemeResourceExtension.h"
#include "MetadataAPI.h"
#include "DOPointerCast.h"
#include "RuntimeDictionary.h"
#include "RuntimeElement.h"

using namespace Diagnostics;
using namespace DependencyLocator;

IFACEMETHODIMP
XamlDiagnostics::ReplaceResource(
    _In_ InstanceHandle resourceDictionaryHandle,
    _In_ InstanceHandle keyHandle,
    _In_ InstanceHandle newValueHandle)
{
    SuspendFailFastOnStowedException suspender;

    std::shared_ptr<RuntimeDictionary> runtimeDictionary;
    RETURN_HR_IF(E_NOTFOUND, !TryFindDictionaryFromHandle(resourceDictionaryHandle, runtimeDictionary));
    std::shared_ptr<RuntimeObject> runtimeKey, runtimeValue;
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(keyHandle, runtimeKey));
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(newValueHandle, runtimeValue));

    IFC_RETURN(runtimeDictionary->TryReplaceItem(runtimeKey, std::move(runtimeValue)));
    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::GetDictionaryItem(
    _In_ InstanceHandle dictionaryHandle,
    _In_z_ LPCWSTR resourceName,
    _In_ BOOL resourceIsImplicitStyle,
    _Out_ InstanceHandle* resourceHandle)
{
    SuspendFailFastOnStowedException suspender;
    *resourceHandle = 0;
    std::shared_ptr<RuntimeDictionary> runtimeDictionary;
    if (TryFindDictionaryFromHandle(dictionaryHandle, runtimeDictionary))
    {
        std::shared_ptr<RuntimeObject> item;
        if (!runtimeDictionary->TryGetItem(resourceName, !!resourceIsImplicitStyle, item))
        {
            IFC_RETURN(E_NOTFOUND);
        }
        *resourceHandle = item->GetHandle();
    }

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::ResolveResource(
    _In_ InstanceHandle resolveForHandle,
    _In_z_ LPCWSTR resourceName,
    _In_ ResourceType resourceType,
    _In_ unsigned int propertyIndex)
{
    SuspendFailFastOnStowedException suspender;
    std::shared_ptr<RuntimeObject> resolveFor;
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(resolveForHandle, resolveFor));
    RETURN_HR_IF(E_INVALIDARG, !resolveFor->IsDependencyObject());

    wrl::ComPtr<xaml::IUIElement> resolutionContext;
    wrl::ComPtr<xaml::IResourceDictionary> dictionary;

    std::shared_ptr<RuntimeElement> resolveForElement;
    std::shared_ptr<RuntimeObject> resolveForOwner;

    if (resolveFor->TryGetAsElement(resolveForElement))
    {
        resolutionContext = resolveForElement->GetBackingElement();
    }
    else if (resolveFor->TryGetParent(resolveForOwner))
    {
        std::shared_ptr<RuntimeDictionary> resolveForOwnerDictionary;
        if (resolveForOwner->TryGetAsDictionary(resolveForOwnerDictionary))
        {
            dictionary = resolveForOwnerDictionary->GetBackingDictionary();
        }
        else if (resolveForOwner->TryGetAsElement(resolveForElement))
        {
            // If the object **isn't** a UIElement and the parent **is**, then this is something
            // like a brush being used like:
            //    <Button.Foreground>
            //      <SolidColorBrush Color='{StaticResource MyColor}' />
            resolutionContext = resolveForElement->GetBackingElement();
        }
    }

    xstring_ptr resourceKey;
    RETURN_IF_FAILED(xstring_ptr::CloneBuffer(resourceName, &resourceKey));
    RuntimeProperty resolveForProperty(static_cast<KnownPropertyIndex>(propertyIndex));
    wrl::ComPtr<xaml::IDependencyObject> resolveForDO;
    IFCFAILFAST(resolveFor->GetBackingObject().As(&resolveForDO));
    wrl::ComPtr<IInspectable> resolvedValue;
    IFC_RETURN(DiagnosticsInterop::ResolveResource(
        resolveForDO.Get(),
        dictionary.Get(),
        resourceKey,
        resourceType,
        static_cast<KnownPropertyIndex>(resolveForProperty.GetIndex()),
        resolutionContext.Get(),
        resolvedValue));

    resolveFor->StoreValue(resolveForProperty, GetRuntimeObject(resolvedValue.Get(), resolveFor));

    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::AddDictionaryItem(
    _In_ InstanceHandle dictionaryHandle,
    _In_ InstanceHandle resourceKey,
    _In_ InstanceHandle resourceHandle)
{
    SuspendFailFastOnStowedException suspender;
    std::shared_ptr<RuntimeDictionary> runtimeDictionary;
    RETURN_HR_IF(E_NOTFOUND, !TryFindDictionaryFromHandle(dictionaryHandle, runtimeDictionary));

    std::shared_ptr<RuntimeObject> runtimeKey, runtimeValue;
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(resourceKey, runtimeKey));
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(resourceHandle, runtimeValue));
    
    ResourceGraphKey graphKey;
    IFC_RETURN(runtimeDictionary->InsertItem(runtimeKey, runtimeValue, &graphKey));
    IFC_RETURN(UpdateDependentItemsOnAdd(graphKey, runtimeDictionary->GetBackingDictionary().Get()));
    return S_OK;
}

IFACEMETHODIMP
XamlDiagnostics::RemoveDictionaryItem(
    _In_ InstanceHandle dictionaryHandle,
    _In_ InstanceHandle resourceKey)
{
    SuspendFailFastOnStowedException suspender;
    std::shared_ptr<RuntimeDictionary> runtimeDictionary;
    RETURN_HR_IF(E_NOTFOUND, !TryFindDictionaryFromHandle(dictionaryHandle, runtimeDictionary));

    std::shared_ptr<RuntimeObject> runtimeKey;
    RETURN_HR_IF(E_NOTFOUND, !TryFindObjectFromHandle(resourceKey, runtimeKey));

    auto graphKey = runtimeDictionary->RemoveItem(runtimeKey);
    IFC_RETURN(UpdateDependentItemsOnRemove(graphKey));

    return S_OK;
}

HRESULT XamlDiagnostics::UpdateDependentItemsOnAdd(
    const ResourceGraphKey& currentResolution,
    _In_ xaml::IResourceDictionary* newDictionary)
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

HRESULT XamlDiagnostics::UpdateDependentItemsOnRemove(
    const ResourceGraphKeyWithParent& removedGraphKey)
{
    auto resourceGraph = GetResourceGraph();
    for (const auto& item : resourceGraph->GetDependentItems(removedGraphKey))
    {
        IFC_RETURN(UpdateDependentItem(item, removedGraphKey.Key, removedGraphKey.Parent));
    }
    return S_OK;
}

HRESULT
XamlDiagnostics::UpdateDependentItem(
    const std::shared_ptr<Diagnostics::ResourceDependency>& dependency,
    const xstring_ptr& key,
    const xref_ptr<CResourceDictionary>& dictionaryForResolution)
{
    ctl::ComPtr<DirectUI::DependencyObject> peer;
    IFC_RETURN(DirectUI::DXamlServices::GetPeer(dependency->GetDependency().get(), &peer));
    auto peerHandle = HandleMap::GetHandle(peer.Cast<xaml::IDependencyObject>());

    wrl::ComPtr<xaml::IUIElement> resolutionContext;

    std::shared_ptr<RuntimeObject> dependentObject;
    if (TryFindObjectFromHandle(peerHandle, dependentObject))
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
