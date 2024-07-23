// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Popup.h"

CItemsControl::~CItemsControl()
{
    ReleaseInterface(m_pItemsHost);
    if (m_pItemCollection)
    {
        // TODO: MERGE: This should be using SetAndPropagateOwner to make the Clear unnecessary.
        // TODO: MERGE: SetOwner shouldn't even be exposed by the collection.
        // Must clear before resetting owner to clean parent references.
        IGNOREHR(m_pItemCollection->Clear());
        IGNOREHR(m_pItemCollection->SetOwner(nullptr));
    }
    ReleaseInterface(m_pItemCollection);
    ReleaseInterface(m_pItemTemplate);
    ReleaseInterface(m_pItemsPanelTemplate);
    ReleaseInterface(m_pItemsPresenter);
    ReleaseInterface(m_pItemContainerTransitions);
}

//
//  CItemsControl::GetItems
//
//  Get the ItemsControl.Items collection.
//
_Check_return_
HRESULT
CItemsControl::GetItems(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    CItemsControl *pItemsControl = (CItemsControl*)pObject;
    IFC_RETURN(pItemsControl->EnsureItemCollection());
    pResult->SetObjectAddRef(pItemsControl->m_pItemCollection);

    return S_OK;
}


//
//  CItemsControl::SetValue
//
//  This is called by the parser to set a property on
//  an ItemsControl.  If the value being set is the content
//  of the ItemsControl, we add it to the Items collection.
//
_Check_return_ HRESULT CItemsControl::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;
    auto core = GetContext();

    AddRef(); // Ensure that any actions on the managed side don't delete this object from underneath us.

    CControlTemplate * pOldTemplate = nullptr;

    if (args.m_pDP->GetIndex() != KnownPropertyIndex::ItemsControl_Items)
    {
        switch (args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::ItemsControl_ItemTemplate:
            {
                // ItemsControl.ItemTemplate being a field property and not a sparse one, SetPeerReferenceToProperty is not automatically called.
                // It is explicitly invoked here to keep the DataTemplate DXaml peer alive.
                IFC(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
                break;
            }
            case KnownPropertyIndex::ItemsControl_ItemsHost:
            {
                // This items host isn't yet in the visual tree, so we have to explicitly
                // track the lifetime of its managed peer.

                IFC(SetPeerReferenceToProperty(args.m_pDP, args.m_value));

                m_bItemsHostIsSetFromManaged = !!core->IsSettingValueFromManaged(this);

                if (m_pItemsHost != NULL)
                {
                    // invalidate old items host panel
                    IFC( InvalidateItemsHost(/*bHostIsReplaced*/ TRUE) );
                    // and set IsItemsHost property to false
                    IFC(m_pItemsHost->SetValueByKnownIndex(KnownPropertyIndex::Panel_IsItemsHost, FALSE));

                }
                break;
            }
            case KnownPropertyIndex::Control_Template:
            {
                // store old template
                pOldTemplate = GetTemplate();
                break;
            }
            case KnownPropertyIndex::ItemsControl_ItemContainerTransitions:
            {
                // when this elements receives a transition meant for its child,
                // proactively ensure storage on that child so it starts to
                // take snapshots.
                if (args.m_value.AsObject() || args.m_value.AsIInspectable())
                {
                    IFC(CPanel::EnsureTransitionStorageForChildren(m_pItemsHost));
                }
                // lifetime of transitioncollection is manually handled since it has no parent
                IFC(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
            }
            default: // Do nothing
                break;
        }

        // Store current value of ItemsHost invalidation
        bool bItemsHostInvalid = m_bItemsHostInvalid;

        // conditions of invalidation ItemsHost:
        // 1. Set new ItemTemplate
        // 2. Set new DisplayMemberPath
        // 3. Set IsItemsHostInvalid to TRUE
        bool shouldInvalidateItemsHost =
            args.m_pDP->GetIndex() == KnownPropertyIndex::ItemsControl_ItemTemplate ||
            args.m_pDP->GetIndex() == KnownPropertyIndex::ItemsControl_DisplayMemberPath ||
            (args.m_pDP->GetIndex() == KnownPropertyIndex::ItemsControl_IsItemsHostInvalid && args.m_value.AsBool());

        // Call the default SetValue implementation. It might change IsItemsHostInvalid property
        IFC(CControl::SetValue(args));

        if (shouldInvalidateItemsHost)
        {
            // if m_bItemsHostInvalid has been changed by CControl::SetValue
            // reset flag and call Invalidate
            // it allows us to perform a real Invalidation
            if (bItemsHostInvalid != m_bItemsHostInvalid)
            {
                m_bItemsHostInvalid = false;
            }
            IFC(InvalidateItemsHost(/*bHostIsReplaced*/ FALSE));
        }

        switch (args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::ItemsControl_DisplayMemberPath:
            {
                // The managed side caches a shared instance of DisplayMemberTemplate.  A change in the path
                // means that this cached object is invalid.
                IFC(FxCallbacks::ItemsControl_DisplayMemberPathChanged(this));
                break;
            }
            case KnownPropertyIndex::ItemsControl_ItemsHost:
            {
                if (m_pItemsHost != NULL)
                {
                    // set IsItemsHost property of the new panel to true
                    IFC(m_pItemsHost->SetValueByKnownIndex(KnownPropertyIndex::Panel_IsItemsHost, TRUE));

                    if (m_pItemsPresenter != NULL)
                    {
                        IFC(m_pItemsPresenter->AddChild(m_pItemsHost));
                    }
                    IFC(ValidateItemsHost());
                }
                else if (m_pItemsPresenter != NULL)
                {
                    CCollection * pChildren = m_pItemsPresenter->GetChildren();
                    if(pChildren != NULL)
                    {
                        IFC(pChildren->Clear());
                    }
                }
                break;
            }
            case KnownPropertyIndex::Control_Template:
            {
                CControlTemplate * pNewTemplate = GetTemplate();

                // If template has not changed, don't unapply current template, for
                // better perf.
                if ((m_pItemsHost != NULL)
                    && (pOldTemplate != pNewTemplate))
                {
                    CValue valueNull;
                    valueNull.SetNull();

                    // we have to throw old ItemsHost and ItemsPresenter
                    // to reuse ItemsControl for different template
                    IFC(SetValueByKnownIndex(KnownPropertyIndex::ItemsControl_ItemsHost, nullptr));
                    if (m_pItemsPresenter != nullptr)
                    {
                        IFC(FxCallbacks::ItemsPresenter_Dispose(m_pItemsPresenter));
                    }
                    ReleaseInterface(m_pItemsPresenter);
                }
                break;
            }
            default: // Do nothing
                break;
        }
    }
    else
    {
        // this is a child, put it in the Items collection
        if (args.m_value.GetType() != valueObject)
        {
            IFC(E_INVALIDARG);
        }

        IFC(EnsureItemCollection());
        IFC(m_pItemCollection->Append(args.m_value.AsObject()));
        IFC(m_pItemCollection->OnAddToCollection(args.m_value.AsObject()));
    }

Cleanup:
    Release();
    return hr;
}

//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CItemsControl::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    EnterParams newParams(params);

    // Work on the items
    if (!params.fSkipNameRegistration)
    {
        newParams.fIsLive = FALSE;
        IFC_RETURN(EnsureItemCollection(pNamescopeOwner, &params));
    }

    // Call the base class to enter all of this object's properties
    IFC_RETURN(CControl::EnterImpl(pNamescopeOwner, params));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   LeaveImpl
//
//  Synopsis:
//      Called when a DependencyObject leaves scope.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CItemsControl::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    LeaveParams newParams(params);

// Work on the children
    if (!params.fSkipNameRegistration && (m_pItemCollection != NULL))
    {
        newParams.fIsLive = FALSE;
        IFC_RETURN(m_pItemCollection->Leave(pNamescopeOwner, newParams));
    }

// Call the base class to leave all of this object's properties

    IFC_RETURN(CControl::LeaveImpl(pNamescopeOwner, params));

    return S_OK;
}

//
//  CItemsControl::MeasureOverride
//
//  Generate visual trees for the items if necessary, then forward measure to the
//  items host panel.
//
_Check_return_ HRESULT
CItemsControl::MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize)
{
    // See if we need to re-generate the item visual trees.
    if (m_bItemsHostInvalid && m_pItemsHost)
    {
        IFC_RETURN(ValidateItemsHost());
    }

    IFC_RETURN(CControl::MeasureOverride(availableSize, desiredSize));

    return S_OK;
}

//
//  CItemsControl::InvalidateItemsHost
//
//  Clear the items host panel and queue a layout pass.
//  We'll re-generate new visual trees on Measure.
//
_Check_return_
HRESULT
CItemsControl::InvalidateItemsHost(_In_ bool bHostIsReplaced)
{
    HRESULT hr = S_OK;
    CItemsControl * current = NULL;

    if( m_bItemsHostInvalid )
    {
        // This is a redundant call
        goto Cleanup;
    }

    // Flag that we need to do work in ValidateItemsHost
    m_bItemsHostInvalid = true;

    // Force layout (we re-generate the item visual trees during Measure).
    InvalidateMeasure();

    // Clear the children from the items host
    if( m_pItemsHost != NULL )
    {
        m_pItemsHost->InvalidateMeasure();

        // Get the children collection for the items host
        CCollection *pChildren = m_pItemsHost->GetChildren();

        // Be sure to check the count to prevent recursion from the managed ClearVisualChildren() function.
        if (pChildren && pChildren->GetCount() > 0)
        {
            AddRef(); // Ensure that any actions on the managed side don't delete this object from underneath us.
            current = this;
            IFC(FxCallbacks::ItemsControl_ClearVisualChildren(this, bHostIsReplaced) );
        }
    }

Cleanup:
    ReleaseInterface(current);
    RRETURN(hr);
}

//
//  CItemsControl::ValidateItemsHost
//
//  If necessary, generate item visual trees and put them
//  into the items host panel.
//
_Check_return_
HRESULT
CItemsControl::ValidateItemsHost(  )
{
    HRESULT hr = S_OK;
    AddRef(); // Ensure that any actions on the managed side don't delete this object from underneath us.

    // Even if recreating the children fails, don't call this again.
    m_bItemsHostInvalid = false;

    // Recreate all the visual children
    IFC(FxCallbacks::ItemsControl_RecreateVisualChildren(this) );

Cleanup:
    Release();
    RRETURN(hr);
}


_Check_return_ HRESULT CItemsControl::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr = S_OK;
    CItemsPresenter* pItemsPresenter = NULL;
    CREATEPARAMETERS cp(GetContext());

    IFC( CFrameworkElement::ApplyTemplate(fAddedVisuals) );
    if (!GetFirstChildNoAddRef())
    {
        // When the spec issue 29744 is ready bring the checking for
        // empty template thus we will fix bug#17615 and won't break SLC

        IFC( CItemsPresenter::Create((CDependencyObject**) &pItemsPresenter, &cp) );
        IFC( pItemsPresenter->SetTemplatedParent(this) );
        IFC( AddChild(pItemsPresenter) );
        fAddedVisuals = true;
    }

Cleanup:
    ReleaseInterface(pItemsPresenter);
    RRETURN(hr);
}

//
//  CItemsControl::EnsureItemCollection
//
//  Create the CItemCollection (for the Items property) if it's not
//  created already.
//
_Check_return_
HRESULT
CItemsControl::EnsureItemCollection( _In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams *pParams   )
{
    HRESULT hr = S_OK;
    CItemCollection *pItemCollection = nullptr;

    if( m_pItemCollection == nullptr )
    {
        CREATEPARAMETERS cp(GetContext());
        IFC(CItemCollection::Create( (CDependencyObject**)(&pItemCollection), &cp));

        IFC(pItemCollection->SetOwner(this));
        m_pItemCollection = pItemCollection;
        pItemCollection = nullptr;

        // Enter the new collection just like we do for other implicit collections
        EnterParams params;
        if( pParams != nullptr )
        {
            params = *pParams;
        }
        else
        {
            params = EnterParams (
                    /*isLive*/                IsActive(),
                    /*skipNameRegistration*/  FALSE,
                    /*coercedIsEnabled*/      GetCoercedIsEnabled(),
                    /*useLayoutRounding*/     GetUseLayoutRounding(),
                    /*visualTree*/            VisualTree::GetForElementNoRef(this, LookupOptions::NoFallback)
                );

            params.fCheckForResourceOverrides = ShouldCheckForResourceOverrides();
        }

        IFC(m_pItemCollection->Enter( GetStandardNameScopeOwner(), params));
    }

Cleanup:
    ReleaseInterface(pItemCollection);
    RRETURN(hr);
}

//
//  CItemsControl::CreationComplete
//
//  The ItemsControl is fully created at this point, if
//  there are items in the Items collection we need
//  the OnItemsChanged override to be called
//
_Check_return_
HRESULT
CItemsControl::CreationComplete()
{
    // Call the base first
    IFC_RETURN(CControl::CreationComplete());

    // Now call into managed to notify
    // of the new items if needed
    // we don't need to do reverse PInvoke if we have no items
    if (m_pItemCollection != NULL && m_pItemCollection->GetCount() > 0)
    {
        IFC_RETURN(FxCallbacks::ItemsControl_NotifyAllItemsAdded(this));
    }

    return S_OK;
}

//
//  CItemsControl::RegisterItemsPresenter
//
// When ItemsPresenter is created it should register itself on ItemsControl
// First presenter comming to ItemsControl is master presenter.
// Other presenters should be ignored.
//
_Check_return_
CItemsPresenter*
CItemsControl::RegisterItemsPresenter(_In_ CItemsPresenter* pItemsPresenter)
{
    // We only hook up the first ItemsPresenter.  It is possible, but not useful, for ItemsPresenter(s) to be created as elements
    // in the Items collection (or as a child of some other UIElement in the collection).  We do not want these spurious
    // ItemsPresenter(s) to be hooked up as the presenter.
    if (!m_pItemsPresenter)
    {
        m_pItemsPresenter = pItemsPresenter;
        AddRefInterface(m_pItemsPresenter);
    }

    return m_pItemsPresenter;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CItemsControl::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CControl::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    if (m_pItemCollection != nullptr)
    {
        m_pItemCollection->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
}

CMenuFlyoutPresenter::~CMenuFlyoutPresenter()
{
    //
    // Bug 18546414
    //
    // We have crashes in Microsoft Edge where a Popup is calling into a deleted m_pChild. CPopup::m_pChild is a ref-counted
    // pointer, so the child should never be deleted. It looks like in all cases the child is a CMenuFlyoutPresenter
    // that still has a back pointer to the popup, so catch the point where the CMenuFlyoutPresenter is being deleted
    // while still connected to the popup and crash there. It's possible that the extra release was not the final
    // release on the CMenuFlyoutPresenter, but this gets us closer to the point of the crash.
    //
    if (m_pLogicalParent != nullptr
        && m_pLogicalParent->OfTypeByIndex<KnownTypeIndex::Popup>())
    {
        CPopup* parentPopup = static_cast<CPopup*>(m_pLogicalParent);
        if (parentPopup->IsFlyout())
        {
            // There's still a logical parent, that parent is a popup, and that popup is a flyout. Fail fast now.
            // The popup should have cleaned up the parent pointer when it closed, before it released the popup
            // child (this CMenuFlyoutPresenter).
            FAIL_FAST_ASSERT(false);
        }
    }
}
