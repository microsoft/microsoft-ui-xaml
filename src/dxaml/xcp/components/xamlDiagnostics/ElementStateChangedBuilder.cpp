// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Microsoft.UI.Xaml.coretypes2.h"
#include "ElementStateChangedBuilder.h"
#include "HandleMap.h"
#include "metadata\inc\TypeTableStructs.h"
#include "xstring\inc\xstring_ptr.h"
#include "Resources.h"
#include "vsm\inc\VisualState.h"
#include "uielement.h"
#include "collection\inc\docollection.h"
#include "diagnosticsInterop\inc\propertychainevaluator.h"
#include "DOPointerCast.h"
#include "MetadataAPI.h"
#include "dependencyLocator\inc\DependencyLocator.h"
#include "diagnosticsInterop\inc\resourcegraph.h"
#include "ccontrol.h"
#include "CustomClassInfo.h"
#include "DiagnosticsInterop.h"
#include "vsm\inc\CVisualStateManager2.h"
#include "collection\inc\visualstategroupcollection.h"

using namespace DependencyLocator;
using namespace DirectUI;
namespace Diagnostics
{

    ElementStateChangedBuilder::ElementStateChangedBuilder(_In_ CDependencyObject* invalidObj, _In_ const CDependencyProperty* invalidProperty)
    {
        HRESULT hr = S_OK;
        // See if the initial object passed in is a UIElement, if so, then we are done.
        if (invalidObj->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            IFC(HandleMap::GetHandle(invalidObj, &m_context.RootHandle));
        }

        IFC(AddProperty(invalidProperty));
    Cleanup:
        // We can't return an error from a constructor, but we have already logged the stowed error which is
        // consistent with previous behavior where we caught an exception and ignored it (after logging);
        return;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddCollectionContext(
        _In_ CDOCollection* collection,
        _In_ CDependencyObject* collectionItem,
        _Out_ bool* isDictionaryItem)
    {
        unsigned int index = 0;
        *isDictionaryItem = TryFindIndexInCollection(collection, collectionItem, &index);

        if (*isDictionaryItem)
        {
            IFC_RETURN(AddAccessor(index));
        }

        return S_OK;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddResourceDictionaryContext(
        _In_ CResourceDictionary* dictionary,
        _In_ CDependencyObject* dictionaryItem,
        _Out_ bool* isDictionaryItem)
    {
        unsigned int index = 0;
        *isDictionaryItem = TryFindIndexInCollection(dictionary, dictionaryItem, &index);
        if (*isDictionaryItem)
        {
            CValue key;
            bool isImplicitStyle = false;
            bool ignoreIsType;
            IFC_RETURN(dictionary->GetKeyAtIndex(index, &key, &isImplicitStyle, &ignoreIsType));
            if (isImplicitStyle)
            {
                const CClassInfo* typeInfo = nullptr;
                IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByFullName(key.AsString(), &typeInfo));
                IFC_RETURN(AddAccessor(typeInfo));
            }
            else
            {
                IFC_RETURN(AddAccessor(key.AsString().GetBuffer()));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddParentContext(
        _In_ CDependencyObject* parent,
        _In_ CDependencyObject* child, 
        _Out_ bool* contextAdded)
    {
        const CDependencyProperty* prop = nullptr;
        IFC_RETURN(FindParentProperty(parent, child, &prop));
        if (prop)
        {
            IFC_RETURN(AddProperty(prop));
        }

        *contextAdded = prop != nullptr;
        return S_OK;
    }

    // Finds the parent property via searching the property chain. Some properties are reported via the chain, so we special case those. For efficiency, special cases
    // should be done first before going through the whole property chain. Comparison of the child object should be done with the CDependencyObject pointer, trying to use the
    // instance handle may not return the same result for non-stateful peers as they can be created and destroyed multiple times.
    _Check_return_ HRESULT ElementStateChangedBuilder::FindParentProperty(_In_ CDependencyObject* parent, _In_ CDependencyObject* child, _Outptr_result_maybenull_ const CDependencyProperty** parentProperty)
    {
        ASSERT(parent != child);
        *parentProperty = nullptr;

        auto childME = do_pointer_cast<CMarkupExtensionBase>(child);
        const CDependencyProperty* foundProperty = nullptr;
        if (childME)
        {
            const auto resourceGraph = GetResourceGraph();
            foundProperty = DirectUI::MetadataAPI::GetPropertyByIndex(resourceGraph->GetTargetProperty(childME));
        }

        if (!foundProperty)
        {
            auto childResources = do_pointer_cast<CResourceDictionary>(child);
            if (childResources)
            {
                // Start with resources because it is skipped during the property chain evaluation
                auto parentFE = do_pointer_cast<CFrameworkElement>(parent);
                if (parentFE)
                {
                    if (childResources == parentFE->GetResourcesNoCreate())
                    {
                        foundProperty = DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::FrameworkElement_Resources);
                    }
                }

                // If the parent wasn't a framework element, we won't have found the property yet.  We should also check if the parent is a CApplication object,
                // since the CApplication object doesn't have a peer and won't work with our final attempt at using the property chain.
                if (!parentFE)
                {
                    if (parent->OfTypeByIndex<KnownTypeIndex::Application>())
                    {
                        CValue cVal;
                        auto resProp = MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::Application_Resources);
                        IFC_RETURN(parent->GetValue(resProp, &cVal));
                        CResourceDictionary* appResources = do_pointer_cast<CResourceDictionary>(cVal);
                        if (childResources == appResources)
                        {
                            foundProperty = resProp;
                        }
                    }
                }
            }
        }

        auto control = do_pointer_cast<CControl>(parent);
        auto vsGroupCollection = do_pointer_cast<CVisualStateGroupCollection>(child);
        if (vsGroupCollection && control)
        {
            auto groupFromControl = CVisualStateManager2::GetGroupCollectionFromControl(control);
            if (groupFromControl == vsGroupCollection)
            {
                foundProperty = DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::VisualStateManager_VisualStateGroups);
            }
        }

        // Now evaluate the entire chain if we haven't found it yet
        if (!foundProperty)
        {
            PropertyChainEvaluator eval(parent);
            for (const auto& data : eval)
            {
                EvaluatedValue evaluatedValue;
                IFC_RETURN(eval.Evaluate(data, evaluatedValue));
                bool wasPegged = false;
                auto coreValue = DiagnosticsInterop::ConvertToCore(evaluatedValue.Value.Get(), &wasPegged);
                auto cleanup = wil::scope_exit([coreValue, wasPegged](){
                    if (wasPegged)
                    {
                        coreValue->UnpegManagedPeer();
                    }
                });
                if (child == coreValue)
                {
                    foundProperty = DirectUI::MetadataAPI::GetPropertyByIndex(data.Index);
                    break;
                }
            }
        }

        if (foundProperty && (parent->OfTypeByIndex<KnownTypeIndex::UIElement>() || parent->OfTypeByIndex<KnownTypeIndex::Application>()))
        {
            IFC_RETURN(HandleMap::GetHandle(parent, &m_context.RootHandle));
        }

        *parentProperty = foundProperty;

        return S_OK;
    }

    bool ElementStateChangedBuilder::TryFindIndexInCollection(_In_ CDOCollection* collection, _In_ CDependencyObject* collectionItem, _Out_ UINT32* index)
    {
        unsigned int i = 0;
        bool found = false;
        for (const auto& item : *collection)
        {
            if (item == collectionItem)
            {
                found = true;
                *index = i;
                break;
            }
            ++i;
        }
        return found;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddProperty(_In_ const CDependencyProperty* prop)
    {
        IFCEXPECTRC_RETURN(prop, E_INVALIDARG);
        if (!m_lastContextWasAccessor)
        {
            StartNewStep();
        }

        IFC_RETURN(AddPropertyInternal(prop));
        m_lastContextWasAccessor = false;
        return S_OK;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddPropertyInternal(_In_ const CDependencyProperty* prop)
    {
        IFC_RETURN(AddTypeInternal(prop->GetDeclaringType()));
        m_context.PathToError.insert(0, prop->GetName().GetBuffer());
        return S_OK;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddTypeInternal(_In_ const CClassInfo* type)
    {
        IFCEXPECTRC_RETURN(type, E_INVALIDARG);
        m_context.PathToError.insert(0, type->GetFullName().GetBuffer());
        m_context.PathToError.insert(0, c_propDelim);
        return S_OK;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddAccessor(_In_ unsigned int index)
    {
        if (!m_lastContextWasAccessor)
        {
            // If this is a step for a resource dictionary inside a collection, then the accessor path looks different and we
            // don't want to add a new step. it would look like this:
            //  Resources:FrameworkElement/MergedDictionaries:ResourceDictionary[0]['MyBrush']
            StartNewStep();
        }

        AddAccessorInternal(std::to_wstring(index), AccessorType::Array);
        return S_OK;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddAccessor(_In_z_ const wchar_t* key)
    {
        IFCEXPECTRC_RETURN(key, E_INVALIDARG);
        if (!m_lastContextWasAccessor)
        {

            StartNewStep();
        }

        AddAccessorInternal(key, AccessorType::Key);
        return S_OK;
    }

    _Check_return_ HRESULT ElementStateChangedBuilder::AddAccessor(_In_ const CClassInfo* type)
    {
        if (!m_lastContextWasAccessor)
        {
            StartNewStep();
        }

        AddAccessorInternal(type->GetFullName().GetBuffer(), AccessorType::ImplicityKey);
        return S_OK;
    }

    void ElementStateChangedBuilder::AddAccessorInternal(_In_ const std::wstring& accessor, _In_ AccessorType accessorType)
    {
        m_context.PathToError.reserve(m_context.PathToError.size() + accessor.size() + GetAccessorCount(accessorType));
        m_context.PathToError.insert(0, GetAccessorEnd(accessorType));
        m_context.PathToError.insert(0, accessor);
        m_context.PathToError.insert(0, GetAccessorStart(accessorType));
        m_lastContextWasAccessor = true;
    }

    size_t ElementStateChangedBuilder::GetAccessorCount(_In_ AccessorType accessorType)
    {
        return wcslen(c_accessorsStart[accessorType]) + wcslen(c_accessorsEnd[accessorType]);
    }

    const wchar_t* ElementStateChangedBuilder::GetAccessorStart(_In_ AccessorType accessorType)
    {
        return c_accessorsStart[accessorType];
    }

    const wchar_t* ElementStateChangedBuilder::GetAccessorEnd(_In_ AccessorType accessorType)
    {
        return c_accessorsEnd[accessorType];
    }

    ElementStateChangedContext ElementStateChangedBuilder::GetContext()
    {
        return m_context;
    }

    bool ElementStateChangedBuilder::IsContextReady()
    {
        return m_context.RootHandle != 0;
    }

    void ElementStateChangedBuilder::StartNewStep()
    {
        if (!m_context.PathToError.empty())
        {
            m_context.PathToError.insert(0, c_delim);
        }
    }
}
