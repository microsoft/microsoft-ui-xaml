// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "corep.h"
#include "ThemeShadow.h"
#include <CValueBoxer.h>
#include <CDependencyObject.h>
#include <DOPointerCast.h>
#include <UIElement.h>
#include <MetadataAPI.h>
#include "UIElementWeakCollection.h"
#include "MetadataAPI.h"
#include "WindowRenderTarget.h"
#include "DCompTreeHost.h"
#include "Popup.h"
#include "ProjectedShadowManager.h"
#include "RuntimeEnabledFeatures.h"

CThemeShadow::CThemeShadow(_In_ CCoreServices *pCore)
    : CShadow(pCore)
{
}

CThemeShadow::CThemeShadow(
    _In_ const CThemeShadow& original,
    _Out_ HRESULT& hr)
    : CShadow(original, hr)
    , m_maskBrush(original.m_maskBrush)
{
}

CThemeShadow::~CThemeShadow()
{
    CUIElementWeakCollection* receivers = GetReceiversNoRef();
    if (receivers != nullptr)
    {
        receivers->SetCollectionChangeCallback(nullptr);
    }
}

_Check_return_ HRESULT CThemeShadow::GetMask(_Outptr_result_maybenull_ WUComp::ICompositionBrush** maskBrush)
{
    *maskBrush = nullptr;
    return m_maskBrush.CopyTo(maskBrush);
}

// Update mask and mark dirty for rendering
void CThemeShadow::SetMask(_In_opt_ WUComp::ICompositionBrush* maskBrush)
{
    m_maskBrush = maskBrush;
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
}

void CThemeShadow::SetDirtyFlagOnPopupChild(_In_opt_ CDependencyObject* element)
{
    // If parent is an opened Popup, propagate dirty flag to its child
    if (element && element->OfTypeByIndex<KnownTypeIndex::Popup>())
    {
        CPopup* popup = static_cast<CPopup*>(element);
        if (popup->IsOpen() && popup->m_pChild)
        {
            CUIElement::NWSetContentDirty(popup->m_pChild, DirtyFlags::Render);
        }
    }
}

// Just like CMultiParentShareableDependencyObject, but also dirty the child of any opened Popup.
// This is needed since Popup.Shadow affects Popup.Child and not the popup itself.
void CThemeShadow::NWPropagateDirtyFlag(DirtyFlags flags)
{
    __super::NWPropagateDirtyFlag(flags);

    if (flags_enum::is_set(flags, DirtyFlags::Render))
    {
        const auto& parentAssociation = GetParentAssociation();
        if (!parentAssociation.empty())
        {
            for (auto& assoc : parentAssociation)
            {
                SetDirtyFlagOnPopupChild(assoc.pParent);
            }
        }
        else
        {
            CDependencyObject* parent = GetParentInternal(false);
            SetDirtyFlagOnPopupChild(parent);
        }
    }
}

_Check_return_ HRESULT CThemeShadow::SetValue(_In_ const SetValueParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ThemeShadow_Receivers:
        {
            // The Receivers collection is read-only. There shouldn't be one already.
            // Note: We can't check GetValue. The collection is created on demand, which means GetValue will call back
            // into SetValue again. Check sparse storage directly.
            ASSERT(!IsEffectiveValueInSparseStorage(KnownPropertyIndex::ThemeShadow_Receivers));

            ASSERT(args.m_value.GetType() == ValueType::valueObject);
            CUIElementWeakCollection* weakCollection = checked_cast<CUIElementWeakCollection>(args.m_value.AsObject());
            ASSERT(weakCollection != nullptr);

            weakCollection->SetCollectionChangeCallback(this);
            weakCollection->SetOwner(this);
            weakCollection->DisallowAncestorsInCollection();
            break;
        }
    }

    IFC_RETURN(CShadow::SetValue(args));

    return S_OK;
}

_Check_return_ HRESULT CThemeShadow::ElementInserted(UINT32 indexInChildrenCollection)
{
    CUIElement* receiver = GetReceiversNoRef()->GetElementAt(indexInChildrenCollection).lock_noref();
    ASSERT(receiver != nullptr);
    receiver->AddedAsShadowReceiver();

    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);

    return S_OK;
}

_Check_return_ HRESULT CThemeShadow::ElementRemoved(UINT32 indexInChildrenCollection)
{
    CUIElement* receiver = GetReceiversNoRef()->GetElementAt(indexInChildrenCollection).lock_noref();
    if (receiver != nullptr)
    {
        receiver->RemovedAsShadowReceiver();
    }

    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);

    return S_OK;
}

CUIElementWeakCollection* CThemeShadow::GetReceiversNoRef()
{
    if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::ThemeShadow_Receivers))
    {
        CValue value;
        VERIFYHR(GetEffectiveValueInSparseStorage(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ThemeShadow_Receivers), &value));

        CUIElementWeakCollection* weakCollection = checked_cast<CUIElementWeakCollection>(value.AsObject());
        ASSERT(weakCollection != nullptr);
        return weakCollection;
    }
    else
    {
        return nullptr;
    }
}

_Check_return_ HRESULT CThemeShadow::CheckForAncestorReceivers(_In_opt_ CUIElement* newParent)
{
    CUIElementWeakCollection* receivers = GetReceiversNoRef();
    if (receivers != nullptr)
    {
        IFC_RETURN(receivers->CheckForAncestorElements(newParent));
    }
    return S_OK;
}

_Check_return_ HRESULT CThemeShadow::ElementMoved(UINT32, UINT32)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CThemeShadow::CollectionCleared()
{
    return E_NOTIMPL;
}

bool CThemeShadow::HasCustomReceivers()
{
    CUIElementWeakCollection* collection = GetReceiversNoRef();

    // Note that a call to ThemeShadowGenerated::get_Receivers on ThemeShadow with an unset receivers collection has
    // a side effect - it creates a default UIElementWeakCollection of size 0. Hence, make sure to check both the
    // existence and size of the collection here.
    return collection && collection->GetCount() > 0;
}

XTHICKNESS CThemeShadow::GetInsetsForWindowedPopup(DropShadowDepthClass depthClass)
{
    // Windowed Popups add their own insets to the HWND bounds.
    // These insets are kept to a minimal size which tightly bounds around the visible shadow.
    // These "tight" insets were generated by manual inspection and must be kept in
    // sync with any future changes we might make to the shadow recipe.
    XTHICKNESS insets;
    switch (depthClass)
    {
        case DropShadowDepthClass::Small:
            insets.left = 4;
            insets.top = 1;
            insets.right = 4;
            insets.bottom = 8;
        break;

        case DropShadowDepthClass::Medium:
            insets.left = 10;
            insets.top = 2;
            insets.right = 10;
            insets.bottom = 18;
        break;

        case DropShadowDepthClass::Large:
            IFCFAILFAST(E_UNEXPECTED);
        break;
    }

    return insets;
}

// Lifted Xaml had an old projected shadow code path that we're keeping around (disabled) as insurance. All shadows
// are going through the drop shadow code path, hence we just return true here.
// After WinUI 3.0 releases and drop shadows are proven to be good, we can delete the old projected shadow code path.
// See Task #35516811: Lifted drop shadows - delete old projected shadow code path
/* static */ bool CThemeShadow::IsDropShadowMode()
{
    return true;
}
