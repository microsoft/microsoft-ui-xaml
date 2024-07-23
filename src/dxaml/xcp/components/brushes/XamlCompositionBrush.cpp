// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "corep.h"
#include "XamlCompositionBrush.h"
#include <vector>
#include <algorithm>
#include <FxCallbacks.h>

CXamlCompositionBrush::CXamlCompositionBrush(_In_ CCoreServices *pCore)
    : CBrush(pCore)
{
}

CXamlCompositionBrush::CXamlCompositionBrush(
    _In_ const CXamlCompositionBrush& original,
    _Out_ HRESULT& hr)
    : CBrush(original, hr)
{
}

_Check_return_ HRESULT CXamlCompositionBrush::GetCompositionBrush(_In_opt_ const CUIElement* element, _In_opt_ WUComp::ICompositor* compositor, _Outptr_result_maybenull_ WUComp::ICompositionBrush** compBrush)
{
    *compBrush = nullptr;
    WUComp::ICompositionBrush* brushNoRef = nullptr;
    if (element != nullptr)
    {
        // Check if the XCBB has an ICompositionBrush specifically for this island. If not, use the singleton brush if it exists.
        CContentRoot* root = VisualTree::GetContentRootForElement(const_cast<CUIElement*>(element));
        brushNoRef = GetBrushForContentRootNoRef(root);
    }
    
    if (brushNoRef != nullptr)
    {
        SetInterface(*compBrush, brushNoRef);
    }
    else
    {
        IFC_RETURN(m_compositionBrush.CopyTo(compBrush));
    }

    return S_OK;
}

// Update brush and mark dirty for rendering
void CXamlCompositionBrush::SetCompositionBrush(_In_opt_ WUComp::ICompositionBrush* compBrush)
{
    m_compositionBrush = compBrush;
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

_Ret_maybenull_ WUComp::ICompositionBrush* CXamlCompositionBrush::GetBrushForContentRootNoRef(_In_ CContentRoot* contentRoot)
{
    auto iter = m_brushMap.find(xref::get_weakref(contentRoot));
    if (iter != m_brushMap.end())
    {
        return (iter->second).Get();
    }

    return nullptr;
}

void CXamlCompositionBrush::SetBrushForContentRoot(_In_ CContentRoot* contentRoot, _In_ WUComp::ICompositionBrush* brush)
{
    xref::weakref_ptr<CContentRoot> contentRootWeak = xref::get_weakref(contentRoot);
    m_brushMap.emplace(contentRootWeak, brush);
}

void CXamlCompositionBrush::ClearCompositionBrushMap()
{
    m_brushMap.clear();
}

void CXamlCompositionBrush::ClearBrushForContentRoot(_In_ CContentRoot* contentRoot, _In_ WUComp::ICompositionBrush* brush)
{
    xref::weakref_ptr<CContentRoot> contentRootWeak = xref::get_weakref(contentRoot);
    auto entry = m_brushMap.find(contentRootWeak);
    if (entry != m_brushMap.end())
    {
        m_brushMap.erase(entry);
    }
}

bool CXamlCompositionBrush::CheckUIElementParent()
{
    const std::size_t parentCount = GetParentCount();
    for (std::size_t index = 0; index < parentCount; index++)
    {
        CDependencyObject* parent = GetParentItem(index);

        // It's possible for XCB to be marked live (due to entering a live ResourceDictionary),
        // and there to exist a non-live UI element parent association (see Bug 18005612).
        // Make sure we don't return true for such an element with IsActive() check.
        if (parent->OfTypeByIndex<KnownTypeIndex::UIElement>() && parent->IsActive())
        {
            return true;
        }
    }
    return false;
}

_Check_return_ HRESULT CXamlCompositionBrush::EnterImpl(_In_ CDependencyObject *namescopeOwner, EnterParams params)
{
    IFC_RETURN(CBrush::EnterImpl(namescopeOwner, params));

    if (params.fIsLive)
    {
        IFC_RETURN(FireOnConnected());
    }

    return S_OK;
}

_Check_return_ HRESULT CXamlCompositionBrush::LeaveImpl(_In_ CDependencyObject* namescopeOwner, LeaveParams params)
{
    IFC_RETURN(CBrush::LeaveImpl(namescopeOwner, params));

    if (params.fIsLive)
    {
        IFC_RETURN(FireOnDisconnected());
    }

    return S_OK;
}

_Check_return_ HRESULT CXamlCompositionBrush::OnSkippedLiveEnter()
{
    return FireOnConnected();
}

_Check_return_ HRESULT CXamlCompositionBrush::OnSkippedLiveLeave()
{
    return FireOnDisconnected();
}

_Check_return_ HRESULT CXamlCompositionBrush::FireOnConnected()
{

    if (FxCallbacks::XamlCompositionBrushBase_HasPrivateOverrides(this))
    {
        // Prune the live parents list by removing the expired weak pointers
        auto it = /*[nodiscard] forcing to assign return value*/
        std::remove_if(m_liveParents.begin(),
            m_liveParents.end(),
            [](const xref::weakref_ptr<CDependencyObject>& weakPtr) { return weakPtr.expired(); });

        HRESULT hrFromApp = S_OK;
        const std::size_t parentCount = GetParentCount();
        for (std::size_t index = 0; index < parentCount; index++)
        {
            CDependencyObject* parent = GetParentItem(index);

            // Ensure parent is not a ResourceDictionary
            const bool isParentUIElement = parent->OfTypeByIndex<KnownTypeIndex::UIElement>();
            if (isParentUIElement && parent->IsActive())
            {
                xref::weakref_ptr<CDependencyObject> parentWeakRef = xref::weakref_ptr<CDependencyObject>(parent);
                if (m_liveParents.end() == m_liveParents.find(parentWeakRef))
                {
                    // A parent has been attached that we haven't seen yet.  Notify the app and track it.
                    // Note that we don't remove elements from this list until the brush is disconnected from the live
                    // tree in FireOnDisconnected, so if a parent is added, removed, and re-added we will not fire
                    // another OnElementConnected.
                    m_liveParents.emplace(parentWeakRef);
                    const HRESULT hr = FxCallbacks::XamlCompositionBrushBase_OnElementConnected(this, parent);
                    if (FAILED(hr))
                    {
                        hrFromApp = hr;
                    }
                }
            }
        }
        IFC_RETURN(hrFromApp);
    }

    // Only call OnConnected() the first time we get a UIElement parent. We can get live enter walks when added to a
    // ResourceDictionary, and those should not call OnConnected.
    if (!m_isParentedToUIElement && CheckUIElementParent())
    {
        // Note: We trigger OnConnected() here in core EnterImpl since the DXaml peer may not exist yet
        IFC_RETURN(FxCallbacks::XamlCompositionBrushBase_OnConnected(static_cast<CDependencyObject*>(this)));
        m_isParentedToUIElement = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CXamlCompositionBrush::FireOnDisconnected()
{
    // Leave is called before we unset the old parent, and it does not pass the old parent down the tree, so we have no
    // way of knowing whether we're being removed from a UIElement parent. Instead, we'll wait and call OnDisconnected
    // on the leave after having no more UIElement parents. The exception is if this is the last live leave, in which case
    // we should always call OnDisconnected (provided a matching OnConnected is still waiting for OnDisconnected).
    // Notes:
    //    - Checking GetParentCount is not good enough. CNoParentShareableDependencyObject stores parent associations,
    //      in a vector that contains duplicates. We have to use the count of live enters, m_cEnteredLive.
    //    - In the case of the last release, we'll go through CNoParentShareableDependencyObject::DeactivateImpl before
    //      getting here, which releases the last count and marks the object as inactive, so checking IsActive is enough.
    if (m_isParentedToUIElement &&
        (!CheckUIElementParent() || !IsActive()))
    {
        IFC_RETURN(FxCallbacks::XamlCompositionBrushBase_OnDisconnected(static_cast<CDependencyObject*>(this)));
        m_isParentedToUIElement = false;

        // After disconnection, we'll fire new OnElementConnected events for each new element again
        m_liveParents.clear();
        m_liveParents.shrink_to_fit();
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlCompositionBrush::SetValue(_In_ const SetValueParams& args)
{
    KnownPropertyIndex propertyIndex = args.m_pDP->GetIndex();
    if (propertyIndex == KnownPropertyIndex::Brush_Transform ||
        propertyIndex == KnownPropertyIndex::Brush_RelativeTransform)
    {
        ::RoOriginateError(E_INVALIDARG, wrl_wrappers::HStringReference(L"Transform/RelativeTransform properties are not supported on XamlCompositionBrushBase.").Get());
        IFC_RETURN(E_INVALIDARG);
    }
    else if (propertyIndex == KnownPropertyIndex::XamlCompositionBrushBase_FallbackColor)
    {
        m_fallbackColorBrush = nullptr;
    }

    IFC_RETURN(__super::SetValue(args));

    return S_OK;
}

bool CXamlCompositionBrush::HasCompositionBrush()
{
    bool hasSingleton = !!m_compositionBrush;
    bool somethingInMap = !(m_brushMap.empty());
    return hasSingleton || somethingInMap;
}
