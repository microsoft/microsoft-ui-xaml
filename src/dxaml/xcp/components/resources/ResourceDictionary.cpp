// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Resources.h"
#include "xcp_error.h"
#include "DOPointerCast.h"
#include "corep.h"
#include "uielement.h"
#include "application.h"

void CResourceDictionary::SetResourceOwner(_In_opt_ CDependencyObject* const pResourceOwner)
{
    m_pResourceOwner = pResourceOwner;
    if (m_pMergedDictionaries)
    {
        m_pMergedDictionaries->SetResourceOwner(pResourceOwner);
    }
}

//---------------------------------------------------
//
// Method: CycleCheckInternal
//
// Check if adding pOriginalItem to this ResourceDictionary's
// MergedDictionaries will create a cycle
//
//---------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::CycleCheckInternal(_In_ CResourceDictionary * pOriginalItem)
{
    HRESULT hr = S_OK;
    CResourceDictionary* pCurrent = NULL;

    if (m_pMergedDictionaries)
    {
        for (XUINT32 i = 0; i < m_pMergedDictionaries->GetCount(); i++)
        {
            IFCPTR(pCurrent = (CResourceDictionary*)m_pMergedDictionaries->GetItemWithAddRef(i));
            if (pCurrent == pOriginalItem)
            {
                IFC(SetAndOriginateError(E_NER_INVALID_OPERATION, UnknownError, AG_E_RESOURCE_CYCLE_DETECTED));
            }

            IFC(pCurrent->CycleCheckInternal(pOriginalItem));

            ReleaseInterface(pCurrent);
        }
    }

Cleanup:
    ReleaseInterface(pCurrent);
    return hr;
}

_Check_return_ bool
CResourceDictionary::HasImplicitStyle()
{
    // This ResourceDictionary contains at least one implicit style if:
    // 1) its main contents has an implicit style
    // 2) one of its MergedDictionaries has an implicit style
    // 3) one of its ThemeDictionaries has an implicit style
    if (   m_nImplicitStylesCount > 0
        || (m_pMergedDictionaries && m_pMergedDictionaries->HasImplicitStyle()))
    {
        return true;
    }

    if (m_pThemeDictionaries)
    {
        for (const auto& item : *m_pThemeDictionaries)
        {
            auto dictionary = do_pointer_cast<CResourceDictionary>(item);
            if (dictionary && dictionary->HasImplicitStyle())
            {
                return true;
            }
        }
    }

    return false;
}

_Check_return_ void*
CResourceDictionary::GetItemWithAddRef(UINT32 index)
{
    // When querying for the count of a resource dictionary, we will return
    // the size of the deferred + undeferred resources. This makes it possible for
    // someone to try and get something from the dictionary that is out of the range so
    // we'll just fault in all deferred resources at this point.
    IFCFAILFAST(LoadAllDeferredResources());
    return CDOCollection::GetItemWithAddRef(index);
}


_Check_return_ HRESULT
CResourceDictionary::GetKeyAtIndex(_In_ XINT32 index, _Out_ CValue * pKey, _Out_ bool *keyIsImplicitStyle, _Out_ bool *keyIsType)
{
    IFCEXPECT_RETURN(pKey);

    if (m_pDeferredResources)
    {
        IFC_RETURN(LoadAllDeferredResources());
    }

    IFCEXPECTRC_RETURN(index >= 0 && static_cast<XUINT32>(index) < m_keyByIndex.size(), E_FAIL);
    ASSERT(static_cast<XUINT32>(index) < CDOCollection::GetCount());

    {
        const auto& keyValuePair = m_keyByIndex[index];

        pKey->SetString(keyValuePair.first);

        CDependencyObject* pValue = (*this)[index];

        if (keyValuePair.second.m_isType
            && pValue)
        {
#if DBG
            ASSERT(FindResourceByKey(ResourceKey(keyValuePair.first, true)) == pValue);
#endif
            *keyIsType = true;

            if (pValue->OfTypeByIndex<KnownTypeIndex::Style>())
            {
                *keyIsImplicitStyle = true;
            }
        }
    }

    return S_OK;
}

CDependencyObject* CResourceDictionary::FindResourceByKey(_In_ const ResourceKey& key) const
{
    const auto hashValue = std::hash<ResourceKey>()(key);
    const auto bucketIndex = hashValue % m_resourceMap.bucket_count();

    auto pos = std::find_if(
        m_resourceMap.begin(bucketIndex),
        m_resourceMap.end(bucketIndex),
        [&key](const auto& entry)
        {
            return key == entry.first;
        });

    return pos == m_resourceMap.end(bucketIndex) ? nullptr : pos->second;
}

//------------------------------------------------------------------------
//
//  Method:   NotifyImplicitStyleChanged
//
//  Synopsis: When dictionary element which is marked as implicit style
//            got removed from or added to dictionary we have to report
//            about it to the dictionary owner.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CResourceDictionary::NotifyImplicitStyleChanged(
    _In_ CStyle *style,
    StyleUpdateType styleType)
{
    CUIElement * pVisualOwner = do_pointer_cast<CUIElement>(m_pResourceOwner);

    bool forceUpdate = false;
    CStyle* newStyle = nullptr;
    CStyle* oldStyle = nullptr;
    switch (styleType)
    {
    case StyleUpdateType::Added:
        newStyle = style;
        break;
    case StyleUpdateType::Removed:
        oldStyle = style;
        break;
    case StyleUpdateType::ForcedByDiagnotics:
        // This is how xaml diagnostics forces updates to implicit styles. We set the new and old implicit
        // style to be the same. When we want to unapply the implicit style, we pass in nullptr. CFrameworkElement::UpdateImplicitStyle
        // won't update the implicit styles if nullptr is passed in without forceUpdate being true.
        forceUpdate = true;
        oldStyle = style;
        newStyle = style;
        break;
    default:
        ASSERT(false);
    }

    if (pVisualOwner)
    {
        IFC_RETURN(pVisualOwner->UpdateImplicitStyle(oldStyle, newStyle, forceUpdate));
    }
    else
    {
        // if owner is not CUIElement try to see whether is an Application
        CApplication * pApplication = do_pointer_cast<CApplication>(m_pResourceOwner);
        auto core = GetContext();

        if (pApplication && core)
        {
            // If the owner is the Application, update all root visual implicit styles
            IFC_RETURN(core->UpdateImplicitStylesOnRoots(oldStyle, newStyle, forceUpdate));
        }
    }

    return S_OK;
}

CustomWriterRuntimeContext* CResourceDictionary::GetDeferredResourcesRuntimeContext() const
{
    return m_pDeferredResources ? m_pDeferredResources->GetSavedRuntimeContext() : nullptr;
}

CDependencyObject* CResourceDictionary::GetResourceOwnerNoRef() const
{
    auto parent = GetParentInternal(false);
    CDependencyObject* resourceOwnerNoRef = m_pResourceOwner;
    while (parent && !resourceOwnerNoRef)
    {
        if (auto parentDictionary = do_pointer_cast<CResourceDictionary>(parent))
        {
            resourceOwnerNoRef = parentDictionary->GetResourceOwnerNoRef();
        }
        else if (auto parentCollection = do_pointer_cast<CResourceDictionaryCollection>(parent))
        {
            resourceOwnerNoRef = parentCollection->GetResourceOwnerNoRef();
        }
        parent = parent->GetParentInternal(false);
    }

    return resourceOwnerNoRef;
}