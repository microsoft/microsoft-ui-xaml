// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FacadeStorage.h"
#include <depends.h>
#include <uielement.h>
#include <SimpleProperties.h>
#include <MetadataAPI.h>
#include <TypeTableStructs.h>

using namespace DirectUI;

void DOFacadeAnimationInfo::UnregisterCompletionHandler()
{
    // Composition internally takes a reference on ScopedBatch, so we must take special care
    // to always explicitly unregister the Completed handler, otherwise we run the risk of
    // being called back after the listener object (eg CUIElement) has been deleted.
    if (m_scopedBatch != nullptr)
    {
        IFCFAILFAST(m_scopedBatch->remove_Completed(m_token));
    }
}

wrl::ComPtr<WUComp::ICompositionObject> FacadeStorage::GetBackingCompositionObject(_In_ const CDependencyObject* object) const
{
    auto itFind = m_doFacadeMap.find(object);
    if (itFind != m_doFacadeMap.end())
    {
        return itFind->second.m_backingCO;
    }

    return wrl::ComPtr<WUComp::ICompositionObject>(nullptr);
}

void FacadeStorage::SetBackingCompositionObject(_In_ const CDependencyObject* object, _In_ WUComp::ICompositionObject* backingCO)
{
    DOFacadeStorage& storage = EnsureDOFacadeStorage(object);
    storage.m_backingCO = backingCO;
}

wrl::ComPtr<IFacadePropertyListener> FacadeStorage::GetBackingListener(_In_ const CDependencyObject* object) const
{
    auto itFind = m_doFacadeMap.find(object);
    if (itFind != m_doFacadeMap.end())
    {
        return itFind->second.m_listener;
    }

    return wrl::ComPtr<IFacadePropertyListener>(nullptr);
}

void FacadeStorage::SetBackingListener(_In_ const CDependencyObject* object, _In_ IFacadePropertyListener* listener)
{
    DOFacadeStorage& storage = EnsureDOFacadeStorage(object);
    storage.m_listener = listener;
}

wrl::ComPtr<FacadeReferenceWrapper> FacadeStorage::GetReferenceWrapper(_In_ const CDependencyObject* object) const
{
    auto itFind = m_doFacadeMap.find(object);
    if (itFind != m_doFacadeMap.end())
    {
        return itFind->second.m_referenceWrapper;
    }

    return wrl::ComPtr<FacadeReferenceWrapper>(nullptr);
}

void FacadeStorage::SetReferenceWrapper(_In_ const CDependencyObject* object, _In_ FacadeReferenceWrapper* referenceWrapper)
{
    DOFacadeStorage& storage = EnsureDOFacadeStorage(object);
    storage.m_referenceWrapper = referenceWrapper;
}

void FacadeStorage::SetIsDeferringReferences(_In_ const CDependencyObject* object, bool isDeferringReferences)
{
    DOFacadeStorage& storage = EnsureDOFacadeStorage(object);
    storage.m_isDeferringReferences = isDeferringReferences;
}

bool FacadeStorage::IsDeferringReferences(_In_ const CDependencyObject* object) const
{
    DOFacadeStorage* storage = TryGetDOFacadeStorage(object);
    if (storage)
    {
        return storage->m_isDeferringReferences;
    }

    return false;
}

DOFacadeAnimationInfo& FacadeStorage::EnsureFacadeAnimationInfo(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID)
{
    DOFacadeStorage* storage = TryGetDOFacadeStorage(object);
    ASSERT(storage != nullptr);
    DOFacadeAnimationMap& doFacadeAnimationMap = storage->m_doFacadeAnimationMap;

    // First explicitly erase any entry we already have for this facade. This will guarantee we perform all cleanup
    // associated with this entry before we create a new one, including releasing the old scoped batch and detaching
    // its completed handler. We don't want that handler staying around and running, because it will cause us to read
    // values from the backing composition object and write them back into Xaml. This also takes care of the hand off
    // scenario by unregistering the completed handler for the old animation that's about to hand off to the new one.
    auto itFind = doFacadeAnimationMap.find(facadeID);
    if (itFind != doFacadeAnimationMap.end())
    {
        itFind->second.UnregisterCompletionHandler();
        doFacadeAnimationMap.erase(itFind);
    }

    // Create an empty object and insert it into the map, then return this for the caller to fill in.
    DOFacadeAnimationInfo info;
    auto result = doFacadeAnimationMap.emplace(facadeID, info);
    return result.first->second;
}

// Return the animation info for this object and the given facade ID, else return nullptr
DOFacadeAnimationInfo* FacadeStorage::TryGetFacadeAnimationInfo(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID) const
{
    // This function could be called in the context of not having any animation running for this object, thus the "Try" prefix.
    // This function does not create anything, instead it queries for an existing entry and returns nullptr if none is found.
    DOFacadeStorage* storage = TryGetDOFacadeStorage(object);
    if (storage != nullptr)
    {
        DOFacadeAnimationMap& doFacadeAnimationMap = storage->m_doFacadeAnimationMap;
        auto itFind = doFacadeAnimationMap.find(facadeID);
        if (itFind != doFacadeAnimationMap.end())
        {
            return const_cast<DOFacadeAnimationInfo*>(&(itFind->second));
        }
    }

    return nullptr;
}

// Perform a reverse lookup for animation info for this object given a ScopedBatch
DOFacadeAnimationInfo* FacadeStorage::GetFacadeAnimationInfoForScopedBatch(_In_ const CDependencyObject* object, _In_ WUComp::ICompositionScopedBatch* scopedBatch)
{
    DOFacadeStorage* storage = TryGetDOFacadeStorage(object);

    // This function insists that there be an entry existing for this search.
    ASSERT(storage != nullptr);

    DOFacadeAnimationMap& doFacadeAnimationMap = storage->m_doFacadeAnimationMap;
    for (auto& pair : doFacadeAnimationMap)
    {
        if (pair.second.m_scopedBatch.Get() == scopedBatch)
        {
            return &(pair.second);
        }
    }

    ASSERT(FALSE);
    return nullptr;
}

// Clear out the animation info we have stored for this object, if none remain, clear out the backing CompositionObject as well.
void FacadeStorage::ClearFacadeAnimationInfo(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID)
{
    DOFacadeStorage* storage = TryGetDOFacadeStorage(object);
    // There is a possible timing issue where an element can be removed from the tree while the completion event is asynchronously
    // enroute.  When this occurs, the element's facade storage will have been cleared before we attempt to clear it during
    // the completion callback.
    if (!storage)
    {
        return;
    }
    DOFacadeAnimationMap& doFacadeAnimationMap = storage->m_doFacadeAnimationMap;
    auto itFind = doFacadeAnimationMap.find(facadeID);
    if (itFind != doFacadeAnimationMap.end())
    {
        itFind->second.UnregisterCompletionHandler();
        doFacadeAnimationMap.erase(itFind);
    }
    if (doFacadeAnimationMap.empty() && storage->m_referenceWrapper == nullptr)
    {
        // There are no more animations for this DO, and there are no references, so we don't need the backing object/storage anymore.
        ClearDOFacadeStorage(object);
    }
}

// Returns true if this object has any facade animations running on it, else false
bool FacadeStorage::HasAnyFacadeAnimations(_In_ const CDependencyObject* object)
{
    DOFacadeStorage* storage = TryGetDOFacadeStorage(object);
    if (storage != nullptr)
    {
        return !storage->m_doFacadeAnimationMap.empty();
    }

    return false;
}

// Returns the KnownPropertyIndex of the first strict facade found for this object that has an animation targeting it.
KnownPropertyIndex FacadeStorage::FindFirstStrictAnimatingFacade(_In_ const CDependencyObject* object)
{
    DOFacadeStorage* storage = TryGetDOFacadeStorage(object);
    if (storage != nullptr)
    {
        DOFacadeAnimationMap& doFacadeAnimationMap = storage->m_doFacadeAnimationMap;

        for (auto& pair : doFacadeAnimationMap)
        {
            if (MetadataAPI::GetPropertyBaseByIndex(pair.first)->IsStrictOnly())
            {
                return pair.first;
            }
        }
    }

    return KnownPropertyIndex::UnknownType_UnknownProperty;
}

bool FacadeStorage::HasAnimation(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID) const
{
    return TryGetFacadeAnimationInfo(object, facadeID) != nullptr;
}

bool FacadeStorage::HasExplicitAnimation(_In_ const CDependencyObject* object, KnownPropertyIndex facadeID) const
{
    const auto& facadeAnimationInfo = TryGetFacadeAnimationInfo(object, facadeID);
    return facadeAnimationInfo != nullptr && !facadeAnimationInfo->m_isImplicitAnimation;
}

void FacadeStorage::OnDODestroyed(_In_ const CDependencyObject* object)
{
    auto itFind = m_doFacadeMap.find(object);
    if (itFind != m_doFacadeMap.end())
    {
        m_doFacadeMap.erase(itFind);
    }
}

DOFacadeStorage& FacadeStorage::EnsureDOFacadeStorage(_In_ const CDependencyObject* object)
{
    auto itFind = m_doFacadeMap.find(object);
    if (itFind != m_doFacadeMap.end())
    {
        return itFind->second;
    }
    else
    {
        DOFacadeStorage newEntry;
        auto result = m_doFacadeMap.emplace(object, newEntry);
        return result.first->second;
    }
}

// Return the facade storage for this object if it exists, otherwise return nullptr.
DOFacadeStorage* FacadeStorage::TryGetDOFacadeStorage(_In_ const CDependencyObject* object) const
{
    auto itFind = m_doFacadeMap.find(object);
    if (itFind != m_doFacadeMap.end())
    {
        return const_cast<DOFacadeStorage*>(&(itFind->second));
    }

    return nullptr;
}

wrl::ComPtr<WUComp::IExpressionAnimation> FacadeStorage::GetFacadeGlueExpression(KnownPropertyIndex facadeID) const
{
    auto itFind = m_facadeGlueExpressionMap.find(facadeID);
    if (itFind != m_facadeGlueExpressionMap.end())
    {
        return itFind->second;
    }

    return wrl::ComPtr<WUComp::IExpressionAnimation>(nullptr);
}

void FacadeStorage::SetFacadeGlueExpression(KnownPropertyIndex facadeID, _In_ WUComp::IExpressionAnimation* glueExpression)
{
    // We expect to only be called when there isn't already an expression stored in the map
    ASSERT(m_facadeGlueExpressionMap.find(facadeID) == m_facadeGlueExpressionMap.end());
    m_facadeGlueExpressionMap.emplace(facadeID, wrl::ComPtr<WUComp::IExpressionAnimation>(glueExpression));
}

// Clear the facade storage for this object if it exists.
void FacadeStorage::ClearDOFacadeStorage(_In_ const CDependencyObject* object)
{
    auto itFind = m_doFacadeMap.find(object);
    if (itFind != m_doFacadeMap.end())
    {
        m_doFacadeMap.erase(itFind);
    }
}

void FacadeStorage::ShrinkToFit()
{
    m_doFacadeMap.shrink_to_fit();
    m_facadeGlueExpressionMap.shrink_to_fit();
}

void FacadeStorage::CleanupCompositorResources()
{
    m_doFacadeMap.clear();
    m_facadeGlueExpressionMap.clear();
}

