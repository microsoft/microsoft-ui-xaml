// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DiagnosticsInterop.h"
#include "vsm\inc\VisualState.h"
#include "valueBoxer\inc\CValueBoxer.h"
#include "ThemeResourceExtension.h"
#include "ThemeResourceExpression.h"
#include "DoPointerCast.h"
#include "resources\inc\ResourceResolver.h"
#include "dependencyLocator\inc\DependencyLocator.h"
#include "theming\inc\ThemeResource.h"
#include "inc\ResourceGraph.h"
#include "inc\ResourceDependency.h"
#include "inc\Unsealer.h"
#include "vsm\inc\VisualStateSetterHelper.h"
#include "deferral\inc\CustomWriterRuntimeContext.h"
#include "MetadataAPI.h"
#include "DependencyObject.h"
#include "DXamlServices.h"
#include "xcp_error.h"
#include "xamlDiagnostics\inc\XamlDiagnostics.h"
#include "xamlDiagnostics\inc\ElementStateChangedBuilder.h"
#include "collection\inc\docollection.h"
#include "style.h"
#include "NullKeyedResource.h"
#include "CreateParameters.h"
#include "UIElementCollection.h"
#include "FrameworkElement_Partial.h"
#include "corep.h"
#include "framework.h"
#include "application.h"

using namespace DirectUI;

namespace Diagnostics
{
    CVisualState* DiagnosticsInterop::TryFindVisualState(_In_ const CDependencyObject* potentialChildOfVisualState)
    {
        CVisualState* candidate = nullptr;
        CDependencyObject* parent = potentialChildOfVisualState->GetParentInternal(false);
        while (parent && !candidate)
        {
            candidate = do_pointer_cast<CVisualState>(parent);
            parent = parent->GetParentInternal(false);
        }

        return candidate;
    }

    _Check_return_ HRESULT DiagnosticsInterop::ResolveResource(
        _In_ xaml::IDependencyObject* resolveFor,
        _In_opt_ xaml::IResourceDictionary* dictionaryFoundIn,
        const xstring_ptr& resourceKey,
        ResourceType resourceType,
        KnownPropertyIndex propertyIndex,
        _In_opt_ xaml::IUIElement* resolutionContext,
        _Out_ wrl::ComPtr<IInspectable>& unboxedValue)
    {
        const auto resourceDependency = GetResourceGraph()->GetResourceDependency(resolveFor, resourceType, propertyIndex);

        xref_ptr<CResourceDictionary> coreDictionary;
        if (dictionaryFoundIn)
        {
            ctl::ComPtr<DirectUI::DependencyObject> dictionaryAsDO;
            IFC_RETURN(dictionaryFoundIn->QueryInterface<DirectUI::DependencyObject>(&dictionaryAsDO));
            coreDictionary = checked_cast<CResourceDictionary>(dictionaryAsDO->GetHandle());
        }

        IFC_RETURN(ResolveResource(resourceDependency, coreDictionary, resourceKey, resolutionContext, unboxedValue));
        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::ResolveResource(
        _In_ const std::shared_ptr<ResourceDependency>& resourceDependency,
        const xref_ptr<CResourceDictionary>& dictionaryFoundIn,
        const xstring_ptr& resourceKey,
        _In_opt_ xaml::IUIElement* resolutionContext,
        _Out_ wrl::ComPtr<IInspectable>& unboxedValue)
    {
        ASSERT(!resourceDependency->Expired());

        Resources::ResolvedResource resolvedResource;
        IFC_RETURN(resourceDependency->Resolve(dictionaryFoundIn.get(), resourceKey, ConvertToCore(resolutionContext).get(), resolvedResource));

        // Only set the resolved value if the resource dependency is valid.
        auto dependency = resourceDependency->GetDependency();
        unboxedValue = wrl::ComPtr<IInspectable>();
        if (resourceDependency->IsValid())
        {
            CValue value;
            value.SetObjectAddRef(resolvedResource.Value.get());

            if (resolvedResource.Value != nullptr)
            {
                IFC_RETURN(DirectUI::CValueBoxer::UnboxObjectValue(&value, resolvedResource.Value->GetClassInformation(), &unboxedValue));
            }

            if (resourceDependency->GetType() == ResourceTypeTheme)
            {
                CREATEPARAMETERS cp(resolvedResource.Value->GetContext());
                xref_ptr<CThemeResourceExtension> themeResourceExtension;
                IFC_RETURN(CThemeResourceExtension::Create(
                    reinterpret_cast<CDependencyObject **>(themeResourceExtension.ReleaseAndGetAddressOf()),
                    &cp));

                // Set key
                themeResourceExtension->m_strResourceKey = resourceKey;

                // Set inital value and target dictionary
                IFC_RETURN(themeResourceExtension->SetInitialValueAndTargetDictionary(
                    resolvedResource.Value.get(),
                    resolvedResource.DictionaryForThemeReference.get()));

                IFC_RETURN(DiagnosticsInterop::SetThemeResourceBinding(
                    dependency.get(),
                    themeResourceExtension.get(),
                    resourceDependency->GetPropertyIndex()));
            }
            else
            {
                ctl::ComPtr<IInspectable> dependencyPeer;
                IFC_RETURN(DirectUI::DXamlServices::GetPeer(dependency.get(), IID_PPV_ARGS(&dependencyPeer)));
                IFC_RETURN(DiagnosticsInterop::SetPropertyValue(
                    dependencyPeer.Get(),
                    static_cast<uint32_t>(resourceDependency->GetPropertyIndex()),
                    unboxedValue.Get(),
                    false /*unregisterResource*/));
            }
        }
        else
        {
            // If invalid, clear the locally set value
            ctl::ComPtr<IInspectable> dependencyPeer;
            IFC_RETURN(DirectUI::DXamlServices::GetPeer(dependency.get(), IID_PPV_ARGS(&dependencyPeer)));
            IFC_RETURN(DiagnosticsInterop::ClearPropertyValue(dependencyPeer.Get(), static_cast<uint32_t>(resourceDependency->GetPropertyIndex())));
        }

        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::SetThemeResourceBinding(
        _In_ CDependencyObject* depObj,
        _In_ CThemeResourceExtension* extension,
        KnownPropertyIndex propertyIndex)
    {
        auto reseal = ObjectUnsealer::UnsealIfRequired(depObj);
        IFC_RETURN(extension->SetThemeResourceBinding(depObj, DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyIndex)));

        IFC_RETURN(OnValueChanged(depObj, propertyIndex));
        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::UpdateThemeResourceValue(
            _In_ CDependencyObject* depObj,
            _In_ CDependencyObject* value,
            KnownPropertyIndex propertyIndex)
    {
        auto currentThemeRef = depObj->GetThemeResourceNoRef(propertyIndex);
        if (currentThemeRef)
        {
            IFC_RETURN(currentThemeRef->SetLastResolvedValue(value));
        }

        IFC_RETURN(depObj->UpdateThemeReference(propertyIndex));
        IFC_RETURN(OnValueChanged(depObj, propertyIndex));
        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::OnValueChanged(_In_ CDependencyObject* depObj, _In_ KnownPropertyIndex changedProperty)
    {
        auto prop = MetadataAPI::GetPropertyByIndex(changedProperty);

        // Ensure that the animated value is always applied. It's possible that a style or local property was changed, which
        // would override the animated value. Since animated values take precedence, we should ensure that it is applied.
        if (depObj->IsAnimatedProperty(prop))
        {
            CValue animatedValue;
            IFC_RETURN(depObj->GetAnimatedValue(prop, &animatedValue));
            IFC_RETURN(depObj->SetAnimatedValue(prop, animatedValue, depObj->TryGetAnimatedPropertySource(prop)));
        }

        auto setter = do_pointer_cast<CSetter>(depObj);
        if (setter)
        {
            return OnSetterChanged(setter);
        }

        auto markupExt = do_pointer_cast<CMarkupExtensionBase>(depObj);
        if (markupExt)
        {
            return OnMarkupExtensionChanged(markupExt);
        }

        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::OnSetterChanged(_In_ CSetter* setter, _In_ bool isSetOperation)
    {
        if (!IsSetterValid(setter))
        {
            // Don't do anything if Setter.Value is unset or we failed to get the value. This can
            // happen if the target property path is invalid, or if Setter.Value hasn't been set yet.
            // If Setter.Value was just cleared, then setterValue will still be set, it will just be null.
            return S_OK;
        }

        if (setter->IsStyleSetter())
        {
            CValue setterValue;
            IFC_RETURN(setter->GetSetterValue(&setterValue));
            if (setterValue.GetType() == ValueType::valueThemeResource)
            {
                // Setters preserve the original CThemeResource object, which means
                // that we need to manually invalidate the Setter's consumers if the source
                // theme resource was modified
                IFC_RETURN(setter->OnSetterValueChanged(
                            PropertyChangedParams(
                                 MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::Setter_Value),
                                 CValue::Empty(),
                                 setterValue)));
            }
        }
        else
        {
            // If this object is in a visual state, then re-apply the current properties
            auto visualState = TryFindVisualState(setter);
            const auto operation = isSetOperation ? VisualStateSetterHelper::SetterOperation::Set : VisualStateSetterHelper::SetterOperation::Unset;
            VisualStateSetterHelper::PerformSetterOperationIfStateActive(visualState, setter, operation);
        }

        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::OnMarkupExtensionChanged(
        _In_ CMarkupExtensionBase* extension)
    {
        const auto resourceGraph = GetResourceGraph();
        if (resourceGraph->HasTarget(extension))
        {
            auto targetDO = resourceGraph->GetTargetObject(extension);
            auto targetDP = MetadataAPI::GetPropertyByIndex(resourceGraph->GetTargetProperty(extension));
            if (targetDO && targetDP)
            {
                ctl::ComPtr<DependencyObject> targetPeer;
                IFC_RETURN(DirectUI::DXamlServices::GetPeer(targetDO.get(), &targetPeer));
                IFC_RETURN(targetPeer->RefreshExpression(targetDP));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::GetKeyFromIInspectable(
        _In_ IInspectable* keyInsp,
        _Out_ xstring_ptr& keyName,
        _Out_ bool* isImplicitStyle
        )
    {
        ctl::ComPtr<IInspectable> key(keyInsp);
        if (auto keyAsStyle = key.AsOrNull<xaml::IStyle>())
        {
            *isImplicitStyle = true;
            wxaml_interop::TypeName targetType = {};
            IFC_RETURN(keyAsStyle->get_TargetType(&targetType));
            IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(targetType.Name, &keyName));
            WindowsDeleteString(targetType.Name);
        }
        else if (auto keyAsType = key.AsOrNull<wf::IReference<wxaml_interop::TypeName>>())
        {
            *isImplicitStyle = true;
            wxaml_interop::TypeName typeName = {};
            IFC_RETURN(keyAsType->get_Value(&typeName));
            const CClassInfo* typeInfo = nullptr;
            IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByTypeName(typeName, &typeInfo));

            keyName = typeInfo->GetFullName();
            WindowsDeleteString(typeName.Name);
        }
        else
        {
            *isImplicitStyle = false;
            wrl_wrappers::HString buf;
            IFC_RETURN(ctl::do_get_value(*buf.GetAddressOf(), keyInsp));
            IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(buf.Get(), &keyName));
        }

        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::AddDictionaryItem(
        _In_ xaml::IResourceDictionary* dictionary,
        _In_ IInspectable* keyInsp,
        _In_ IInspectable* dictionaryItem,
        _Out_ ResourceGraphKey& resourceGraphKey)
    {
        ctl::ComPtr<DirectUI::DependencyObject> dictionaryAsDO;
        IFC_RETURN(dictionary->QueryInterface<DirectUI::DependencyObject>(&dictionaryAsDO));
        auto coreDictionary = checked_cast<CResourceDictionary>(dictionaryAsDO->GetHandle());

        // For implicit styles, the key and item should be the same
        bool isImplicitStyle = false;
        xstring_ptr key;
        IFC_RETURN(GetKeyFromIInspectable(keyInsp, key, &isImplicitStyle));

        // Lookup the key to find the current dictionary that this resource would resolve to. We do this so that
        // we can re-resolve any resource dependencies to this new value.
        xref_ptr<CResourceDictionary> dictionaryFoundIn;
        IFC_RETURN(Resources::ResourceResolver::FindCurrentDictionary(coreDictionary, key, isImplicitStyle, dictionaryFoundIn));

        // Items in a resource dictionary have to be DependencyObjects, so get the core representation of this object and
        // use SetObject rather than SetIInspectable
        bool wasPegged = false;
        auto coreValue = DiagnosticsInterop::ConvertToCore(dictionaryItem, &wasPegged);
        if (dictionaryItem == nullptr)
        {
            CREATEPARAMETERS cp(coreDictionary->GetContext());
            IFC_RETURN(CNullKeyedResource::Create(coreValue.ReleaseAndGetAddressOf(), &cp));
        }

        auto cleanup = wil::scope_exit([coreValue, wasPegged](){
            if (wasPegged)
            {
                coreValue->UnpegManagedPeer();
            }
        });

        IFCEXPECTRC_RETURN(coreValue, E_INVALIDARG);

        CValue keyValue;
        keyValue.WrapObjectNoRef(coreValue.get());

        const bool ignoreAllowItems = true; // We want this to be added no matter what
        const bool undeferringKey = false;
        IFC_RETURN(coreDictionary->Add(ResourceKey(key, isImplicitStyle), &keyValue, nullptr, false, ignoreAllowItems, undeferringKey));
        auto coreStyle = do_pointer_cast<CStyle>(coreValue);
        if (coreStyle)
        {
            const auto resourceGraph = GetResourceGraph();
            resourceGraph->AddStyleContext(coreStyle, coreDictionary, isImplicitStyle);
        }

        resourceGraphKey =  ResourceGraphKey(dictionaryFoundIn.get(), key);
        return S_OK;
    }

    bool DiagnosticsInterop::TryRemoveDictionaryItem(
        _In_ xaml::IResourceDictionary* dictionary,
        _In_ IInspectable* keyInsp,
        ResourceGraphKeyWithParent& graphKey)
    {
        ctl::ComPtr<DirectUI::DependencyObject> dictionaryAsDO;
        IFCFAILFAST(dictionary->QueryInterface<DirectUI::DependencyObject>(&dictionaryAsDO));
        auto coreDictionary = checked_cast<CResourceDictionary>(dictionaryAsDO->GetHandle());

        bool isImplicitStyle = false;
        xstring_ptr key;
        IFCFAILFAST(GetKeyFromIInspectable(keyInsp, key, &isImplicitStyle));
        CDependencyObject* coreValue = nullptr;
        if (isImplicitStyle)
        {
            IFCFAILFAST(coreDictionary->GetImplicitStyleKeyNoRef(key, Resources::LookupScope::SelfOnly, &coreValue));
        }
        else
        {
            IFCFAILFAST(coreDictionary->GetKeyNoRef(key, Resources::LookupScope::SelfOnly, &coreValue));
        }

        auto coreStyle = do_pointer_cast<CStyle>(coreValue);
        if (coreStyle)
        {
            const auto resourceGraph = GetResourceGraph();
            resourceGraph->RemoveStyleContext(coreStyle);
        }

        bool succeeded = SUCCEEDED(coreDictionary->Remove(key, isImplicitStyle));
        graphKey = ResourceGraphKeyWithParent(coreDictionary, key);

        return succeeded;
    }

    _Check_return_ HRESULT DiagnosticsInterop::FindIntersectingKeys(
        _In_ wfc::IVector<xaml::ResourceDictionary*>* dictionaryCollection,
        _In_ xaml::IResourceDictionary* resourceDictionary,
        _Out_ std::vector<ResourceGraphKey>& intesectingKeys)
    {
        auto parentDictionary = do_pointer_cast<CResourceDictionary>(ConvertToCore(dictionaryCollection)->GetParentInternal(false));
        intesectingKeys.clear();

        // If the parent of the collection is a dictionary itself, then we are either a theme or merged dictionary collection.
        // We'll want to grab all the current resolutions for keys that match the dictionary being added starting at the scope of the parent dictionary
        // before inserting the dictionary into the collecion. If the parent isn't a dictionary, then we'll just do the insertion and call it a day.
        if (parentDictionary)
        {
            // Only do this if the parent is ResourceDictionary, this ensures we are modifying either ResourceDictionary.MergedDictionaries
            // or ResourceDictionary.ThemeDictionaries and not some random, user defined collection of ResourceDictionaries.
            auto coreDictionary = checked_cast<CResourceDictionary>(ConvertToCore(resourceDictionary));
            IFC_RETURN(FindIntersectingKeys(coreDictionary, parentDictionary, intesectingKeys));
        }

        return S_OK;
    }

    std::vector<ResourceGraphKeyWithParent> DiagnosticsInterop::GetAllKeys(
        _In_ wfc::IVector<xaml::ResourceDictionary*>* dictionaryCollection)
    {
        std::vector<ResourceGraphKeyWithParent> allKeys; // holds all keys for every dictionary
        if (auto mergedCollection = do_pointer_cast<CResourceDictionaryCollection>(ConvertToCore(dictionaryCollection)))
        {
            // Only do this if the collection is a CResourceDictionaryCollection, this type is internal and ensures we are
            // modifying ResourceDictionary.MergedDictionaries and not some random, user defined collection of ResourceDictionaries.
            allKeys = GetAllKeys(mergedCollection);
        }

        return allKeys;
    }

    std::vector<ResourceGraphKeyWithParent> DiagnosticsInterop::GetAllKeys(
        _In_ xaml::IResourceDictionary* dictionary)
    {
        return GetAllKeys(checked_cast<CResourceDictionary>(ConvertToCore(dictionary)));
    }

    std::vector<ResourceGraphKeyWithParent> DiagnosticsInterop::GetAllKeys(
        _In_ CResourceDictionary* dictionary)
    {
        auto count = dictionary->GetCount();
        std::vector<ResourceGraphKeyWithParent> keys;
        keys.reserve(count);

        bool ignoreIsType;
        for (unsigned int i = 0; i < count; ++i)
        {
            xref_ptr<CDependencyObject> item;
            item.attach(dictionary->GetItemDOWithAddRef(i));
            auto dictionaryItem = do_pointer_cast<CResourceDictionary>(item);
            if (dictionaryItem)
            {
                // If the item in the dictionary is a dictionary itself, (such as in theme dictionaries) then use those keys instead.
                auto itemKeys = GetAllKeys(dictionaryItem);
                keys.reserve(keys.size() + itemKeys.size());
                keys.insert(keys.end(), std::make_move_iterator(itemKeys.begin()), std::make_move_iterator(itemKeys.end()));
            }
            else
            {
                CValue key;
                bool isImplicitStyle = false;
                // By definition, implicit styles can't be referenced with an x:Key
                if (SUCCEEDED(dictionary->GetKeyAtIndex(static_cast<int>(i), &key, &isImplicitStyle, &ignoreIsType)) && !isImplicitStyle)
                {
                    keys.emplace_back(ResourceGraphKeyWithParent(dictionary, key.AsString()));
                }
            }
        }

        // Get keys from merged dictionaries
        if (auto mergedCollection = dictionary->GetMergedDictionaries())
        {
            auto mergedKeys = GetAllKeys(mergedCollection);
            keys.reserve(keys.size() + mergedKeys.size());
            keys.insert(keys.end(), std::make_move_iterator(mergedKeys.begin()), std::make_move_iterator(mergedKeys.end()));
        }

        // Get keys from theme dictionaries
        if (auto themeDictionaries = dictionary->GetThemeDictionaries())
        {
            auto themeKeys = GetAllKeys(themeDictionaries);
            keys.reserve(keys.size() + themeKeys.size());
            keys.insert(keys.end(), std::make_move_iterator(themeKeys.begin()), std::make_move_iterator(themeKeys.end()));
        }

        return keys;
    }

    std::vector<ResourceGraphKeyWithParent> DiagnosticsInterop::GetAllKeys(
        _In_ CResourceDictionaryCollection* mergedDictionaryCollection)
    {
        std::vector<ResourceGraphKeyWithParent> allKeys;
        for (XINT32 i = mergedDictionaryCollection->GetCount() - 1; i >= 0; i--)
        {
            xref_ptr<CResourceDictionary> currentDictionary;
            currentDictionary.attach(static_cast<CResourceDictionary*>(mergedDictionaryCollection->GetItemWithAddRef(i)));
            auto currentKeys = GetAllKeys(currentDictionary.get());
            allKeys.reserve(allKeys.size() + currentKeys.size());
            allKeys.insert(allKeys.end(),std::make_move_iterator(currentKeys.begin()), std::make_move_iterator(currentKeys.end()));
        }
        return allKeys;
    }

    _Check_return_ HRESULT DiagnosticsInterop::FindIntersectingKeys(
        _In_ CResourceDictionary* modified,
        _In_ CResourceDictionary* parentDictionary,
        _Out_ std::vector<ResourceGraphKey>& intersectingKeys)
    {
        auto count = modified->GetCount();
        intersectingKeys.clear();
        intersectingKeys.reserve(count);

        bool ignoreIsType;
        for (int i = 0; i < static_cast<int>(count); ++i)
        {
            bool isImplicitStyle = false;
            CValue key;
            IFC_RETURN(modified->GetKeyAtIndex(i, &key, &isImplicitStyle, &ignoreIsType));
            IFCEXPECTRC_RETURN(key.GetType() == valueString, E_UNEXPECTED);
            // For add operations, we need to find where keys in the current scope intersect with ones that are in the dictionary being
            // added. We want to use the parent dictionary scope because it's possible that a sibling merged dictionary could have a resource
            // with the same key. If no current resolution is found, then we don't need to worry about updating it because it won't affect any objects.
            // Note that we don't care about the type of the resource, only about matching keys, as we'll allow the re-resolve handle any errors
            // that result from this add
            xref_ptr<CResourceDictionary> dictionaryFoundIn;
            IFC_RETURN(Resources::ResourceResolver::FindCurrentDictionary(parentDictionary, key.AsString(), isImplicitStyle, dictionaryFoundIn));
            if (dictionaryFoundIn)
            {
                ASSERT(dictionaryFoundIn != modified); // The modified dictionary shouldn't have been added to the parent yet
                intersectingKeys.emplace_back(ResourceGraphKey(dictionaryFoundIn, key.AsString()));
            }
        }

        intersectingKeys.shrink_to_fit();
        return S_OK;
    }

    void DiagnosticsInterop::OnElementStateChanged(
        VisualElementState state,
        _In_ CDependencyObject* element,
        _In_ const CDependencyProperty* prop)
    {
        m_diagnostics->OnElementStateChanged(state, element, prop);
    }

    xref_ptr<CResourceDictionary> DiagnosticsInterop::GetDictionaryIfResource(
        _In_ xaml::IResourceDictionary* resourceDictionary,
        _In_ xref_ptr<CDependencyObject> possibleKey)
    {
        auto coreDictionary = checked_cast<CResourceDictionary>(ConvertToCore(resourceDictionary));
        for (const auto& item : *coreDictionary)
        {
            if (item == possibleKey)
            {
                return xref_ptr<CResourceDictionary>(coreDictionary);
            }
        }

        return nullptr;
    }

    // If the grandparent of the passed in dictionary is a ResourceDictionary, then the one passed in
    // is part of a Theme or Merged dictionary. When removing the entire dictionary from these collections,
    // we need to get the grandparent to use as a starting point for the re-resolve logic.
    xref_ptr<CResourceDictionary> DiagnosticsInterop::GetImmediateParentDictionary(
        _In_ CResourceDictionary* resourceDictionary) noexcept
    {
        xref_ptr<CResourceDictionary> parentDictionary;
        auto parentCollection = do_pointer_cast<CDOCollection>(resourceDictionary->GetParentInternal(false));
        if (parentCollection)
        {
            parentDictionary = do_pointer_cast<CResourceDictionary>(parentCollection->GetParentInternal(false));
        }

        return parentDictionary;
    }

    bool DiagnosticsInterop::IsShareable(_In_ IInspectable* object)
    {
        if (object)
        {
            wrl::ComPtr<xaml::IStyle> style;
            wrl::ComPtr<xaml::IDependencyObject> depObj;
            if (SUCCEEDED(object->QueryInterface(style.ReleaseAndGetAddressOf())))
            {
                return true;
            }
            else if (SUCCEEDED(object->QueryInterface(depObj.ReleaseAndGetAddressOf())))
            {
                auto coreObj = static_cast<DirectUI::DependencyObject*>(depObj.Get())->GetHandle();
                return coreObj->DoesAllowMultipleParents() ||
                       // Objects that can be applied as property sources should be considered shareable. Setters
                       // are CMultiParentShareables, so we don't need to check for them.
                       coreObj->OfTypeByIndex<KnownTypeIndex::Timeline>() &&
                       !coreObj->OfTypeByIndex<KnownTypeIndex::Storyboard>();
            }
        }
        return false;
    }

    bool DiagnosticsInterop::CollectionIsParentToItems(_In_ IInspectable* collection)
    {
        ASSERT(DiagnosticsInterop::IsCollection(collection));

        ctl::ComPtr<DirectUI::DependencyObject> depObjCollection;
        if (SUCCEEDED(collection->QueryInterface(depObjCollection.GetAddressOf())))
        {
            if (auto elementCollection = do_pointer_cast<CUIElementCollection>(depObjCollection->GetHandle()))
            {
                return false;
            }
            else if (auto doCollection = do_pointer_cast<CDOCollection>(depObjCollection->GetHandle()))
            {
                // Some DOCollections specifically don't own their items
                return doCollection->CollectionShouldOwnItems();
            }
        }
        // Any other collection should be the parent
        return true;
    }

    std::vector<std::pair<xstring_ptr, bool>> DiagnosticsInterop::GetKeysOrderedByIndex(_In_ xaml::IResourceDictionary* dictionary)
    {
        auto coreDictionary = checked_cast<CResourceDictionary>(ConvertToCore(dictionary));

        auto count = coreDictionary->GetCount();
        std::vector<std::pair<xstring_ptr, bool>> keys;
        keys.reserve(count);

        bool ignoreIsType;
        for (unsigned int i = 0; i < count; ++i)
        {
            CValue key;
            bool isImplicitStyle = false;
            // Trying to get keys by index causes dictionaries to load all deferred resources. This can fail if any of them
            // are invalid.
            if (SUCCEEDED(coreDictionary->GetKeyAtIndex(static_cast<int>(i), &key, &isImplicitStyle, &ignoreIsType)))
            {
                keys.emplace_back(std::make_pair(key.AsString(), isImplicitStyle));
            }
        }
        return keys;
    }

    KnownPropertyIndex DiagnosticsInterop::GetSetterPropertyIndex(_In_ CSetter* setter)
    {
        auto typeIndex = KnownTypeIndex::UnknownType;
        if (setter->IsStyleSetter())
        {
            const auto graph = GetResourceGraph();
            if (auto style = graph->GetOwningStyle(checked_cast<CSetterBaseCollection>(setter->GetParentInternal(false /*publicOnly */))))
            {
                typeIndex = style->GetTargetType()->GetIndex();
            }
        }

        KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
        VERIFYHR(setter->GetProperty(typeIndex, &propertyIndex));
        return propertyIndex;
    }

    bool DiagnosticsInterop::IsSetterValid(_In_ CSetter* setter)
    {
        // Diagnostics needs to validate both VSM and Style setters.
        const bool isValueValid = IsSetterValueValid(setter);

        // For some reason we don't check Setter.Property
        const bool hasTargetOrProperty = true;

        return isValueValid && hasTargetOrProperty;
    }

    bool DiagnosticsInterop::IsSetterValueValid(_In_ CSetter* setter)
    {
        // This is the way Diagnostics refers to Setter.Value being valid. We can't just check if Setter.Value is default like CSetterBaseCollection,
        // because if it was set and cleared it won't be considered default.
        CValue setterValue;
        return SUCCEEDED(setter->GetSetterValue(&setterValue)) && !setterValue.IsUnset();
    }

    _Check_return_ HRESULT DiagnosticsInterop::UpdateBasedOnStyleListeners(_In_ CStyle* style, _In_opt_ CStyle* oldBasedOnStyle, _In_opt_ CStyle* newBasedOnStyle)
    {
        auto coreServices = GetCore();

        containers::vector_set<KnownPropertyIndex> seenProperties;
        {
            // Calculate how much space to reserve
            std::size_t sizeToReserve = 0;
            sizeToReserve += (oldBasedOnStyle != nullptr) ? oldBasedOnStyle->GetSetterCount() : 0;
            sizeToReserve += (newBasedOnStyle != nullptr) ? newBasedOnStyle->GetSetterCount() : 0;
            seenProperties.reserve(sizeToReserve);
        }

        if (oldBasedOnStyle != nullptr)
        {
            for (unsigned int i = 0; i < oldBasedOnStyle->GetSetterCount(); ++i)
            {
                auto propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
                IFC_RETURN(oldBasedOnStyle->GetPropertyAtSetterIndex(i, &propertyIndex));

                auto inserted = seenProperties.emplace(propertyIndex).second;
                if (inserted)
                {
                    coreServices->NotifyMutableStyleValueChangedListeners(style, propertyIndex);
                }
            }
        }
        if (newBasedOnStyle != nullptr)
        {
            for (unsigned int i = 0; i < newBasedOnStyle->GetSetterCount(); ++i)
            {
                auto propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
                IFC_RETURN(newBasedOnStyle->GetPropertyAtSetterIndex(i, &propertyIndex));

                auto inserted = seenProperties.emplace(propertyIndex).second;
                if (inserted)
                {
                    coreServices->NotifyMutableStyleValueChangedListeners(style, propertyIndex);
                }
            }
        }

        // Unregister the listeners from the old BasedOn Style and register them for the new one
        // By definition, this set of listeners is the set of all listeners registered for the child Style
        // whose BasedOn property is changing.
        for (auto listener : coreServices->GetMutableStyleValueChangedListeners(style))
        {
            if (oldBasedOnStyle != nullptr)
            {
                coreServices->RemoveMutableStyleValueChangedListener(oldBasedOnStyle, listener);
            }
            if (newBasedOnStyle != nullptr)
            {
                coreServices->AddMutableStyleValueChangedListener(newBasedOnStyle, listener);
            }
        }

        return S_OK;
    }

     _Check_return_ HRESULT DiagnosticsInterop::FindOtherDictionaryForResolution(
        _In_ CResourceDictionary* dictionary,
        const xstring_ptr& key,
        bool isImplicitStyle,
        _Out_ xref_ptr<CResourceDictionary>& dictionaryFoundIn)
    {
        CDependencyObject* keyNoRef = nullptr;

        // Get the resource owner of this dictionary and start at the outer most dictionary, we'll start there and
        // walk all merged and theme dictionaries
        auto owner = dictionary->GetResourceOwnerNoRef();
        CResourceDictionary* dictionaryToCheck = nullptr;

        if (auto ownerFe = do_pointer_cast<CFrameworkElement>(owner))
        {
            dictionaryToCheck = ownerFe->GetResourcesNoCreate();
        }
        else if (auto ownerApp = do_pointer_cast<CApplication>(owner))
        {
            dictionaryToCheck = ownerApp->m_pResources;
        }

        // Check dictionary and merged/theme dictionaries
        IFC_RETURN(FindOtherDictionaryForResolution(dictionaryToCheck, dictionary, key, isImplicitStyle, dictionaryFoundIn));

        // Nothing found at this scope, walk the tree and see if it can be found. Start at the parent of the owner so we don't
        // find anything else.
        if (!dictionaryFoundIn && owner)
        {
            if (isImplicitStyle)
            {
                if (auto ownerParent = owner->GetInheritanceParentInternal(TRUE/*bLogicalParent*/))
                {
                    IFC_RETURN(Resources::ResourceResolver::ResolveImplicitStyleKey(ownerParent, key, nullptr, &dictionaryFoundIn));
                }
            }
            else if (auto onwerParent = owner->GetParentInternal(false))
            {
                IFC_RETURN(Resources::ResourceResolver::FindResolvedValueNoRefImpl(onwerParent, key, true, &dictionaryFoundIn));
            }

            // Still not found, look in Application and Global resources
            if (!dictionaryFoundIn)
            {
                IFC_RETURN(Resources::ResourceResolver::FallbackGetKeyForResourceResolutionNoRef(dictionary->GetContext(), key, Resources::LookupScope::All, &keyNoRef, &dictionaryFoundIn));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT DiagnosticsInterop::FindOtherDictionaryForResolution(
        _In_opt_ CResourceDictionary* startDictionary,
        _In_ const CResourceDictionary* dictionaryToSkip,
        const xstring_ptr& key,
        bool isImplicitStyle,
        _Inout_ xref_ptr<CResourceDictionary>& dictionaryFoundIn)
    {
        // This algorithm is very similar to CResourceDictioanry::GetKeyNoRefImpl, the one problem with CResourceDictioanry::GetKeyNoRefImpl is that
        // we are unable to specify a dictionary to skip. We have a dictionary to skip because in the following scenario. If the ResourceDictionary
        // created from foo.xaml has a resource "foo", we
        // <ResourceDictionary>
        //   <ResourceDictionary.MergedDictionaries>
        //     <ResourceDictionary>
        //        <SolidColorBrush x:Key="foo">Red</SolidColorBrush>
        //     </ResourceDictionary>
        //     <ResourceDictionary Source="foo.xaml"/>
        //   </ResourceDictionary.MergedDictionaries>
        // </ResourceDictionary>
        // <Button Background="{StaticResource foo}" />    
        if (startDictionary && startDictionary != dictionaryToSkip)
        {
            if (startDictionary->HasKey(key, isImplicitStyle))
            {
                dictionaryFoundIn = startDictionary;
            }
            else
            {
                if (auto mergedDictionaries = startDictionary->GetMergedDictionaries())
                {
                    for (int32_t i = mergedDictionaries->GetCount() - 1; i >= 0 && dictionaryFoundIn == nullptr; i--)
                    {
                        xref_ptr<CResourceDictionary> dictionaryToCheck;
                        dictionaryToCheck.attach(static_cast<CResourceDictionary*>(mergedDictionaries->GetItemWithAddRef(i)));
                        IFC_RETURN(FindOtherDictionaryForResolution(dictionaryToCheck.get(), dictionaryToSkip, key, isImplicitStyle, dictionaryFoundIn));
                    }
                }

                auto themeDictionaries = startDictionary->GetThemeDictionaries();
                if (!dictionaryFoundIn && themeDictionaries)
                {
                    VERIFYHR(themeDictionaries->EnsureActiveThemeDictionary());
                    IFC_RETURN(FindOtherDictionaryForResolution(themeDictionaries->m_pActiveThemeDictionary, dictionaryToSkip, key, isImplicitStyle, dictionaryFoundIn));
                }
            }
        }
        return S_OK;
    }
}
