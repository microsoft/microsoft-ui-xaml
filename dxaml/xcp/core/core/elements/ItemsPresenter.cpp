// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CItemsPresenter::~CItemsPresenter()
{
    ReleaseInterface(m_pItemsPanelTemplate);
}

//-------------------------------------------------------------------------
//
//  Synopsis:   override CUIElement::AddChild() to ensure there is only one child.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CItemsPresenter::AddChild(_In_ CUIElement *pChild)
{
    HRESULT hr = S_OK;
    CDependencyObject *pParent = NULL;
    IFCEXPECT_ASSERT(pChild);

    // The pChild is now m_pItemsHost.  We're about to set its parent to NULL.
    // In so doing, its managed peer will become GC-able.
    // To keep it from being collected during that time, strengthen the ref to it.
    pChild->PegManagedPeerNoRef();

    // Clear the associations so that we can insert into the visual tree.
    pChild->SetAssociated(false, nullptr);
    pParent = pChild->GetParentInternal(false);
    if (pParent)
    {
        IFC(pChild->RemoveParent(pParent));
    }

    if (CCollection* pChildren = GetChildren())
    {
        IFC(pChildren->Clear());
    }

    IFC(CFrameworkElement::AddChild(pChild));

Cleanup:
    // The ItemsHost (pPanel) is now back in the tree, and its lifetime
    // is being tracked again.  So we don't need a special strong reference.
    if(pChild)
    {
        pChild->UnpegManagedPeerNoRef();
    }
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CItemsPresenter::SetValue()
//
//  Synopsis:   override CUIElement::SetValue() to act on a changing
//              ItemsPanel.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CItemsPresenter::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::ItemsPresenter_ItemsPanel)
    {
        // we force the ApplyTemplate to run again, which has the logic necessary to
        // register this new panel and invalidates the itemshost.
        if (GetChildren())
        {
            IFC_RETURN(GetChildren()->Clear());
            InvalidateMeasure();
        }
    }

    IFC_RETURN(CFrameworkElement::SetValue(args));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CItemsPresenter::OnApplyTemplate()
//
//  Synopsis:   Called by CFrameworkElement::InvokeApplyTemplate once a
//              template has been applied
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CItemsPresenter::OnApplyTemplate()
{
    if (HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::FrameworkElement_OnApplyTemplate(this));
    }

    return S_OK;
}

_Check_return_ HRESULT CItemsPresenter::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    IFC_RETURN(CFrameworkElement::ApplyTemplate(fAddedVisuals));

    if (!HasTemplateChild())
    {
        // Find our parent pItemsControl and ask him to fill in the panel and elements.

        CDependencyObject* pItemsControlDO = this;
        CItemsControl* pItemsControl = NULL;
        pItemsControlDO = pItemsControlDO->GetInheritanceParentInternal();
        while (pItemsControlDO)
        {
            pItemsControl = do_pointer_cast<CItemsControl>(pItemsControlDO);
            if (pItemsControl)
                break;
            pItemsControlDO = pItemsControlDO->GetInheritanceParentInternal();
        }

        // If we coudn't find an items control then just return
        // this will make this element a noop element
        if (pItemsControl == NULL ||
            pItemsControl->RegisterItemsPresenter(this) != this)
        {
            return S_OK;
        }

        if (pItemsControl == do_pointer_cast<CItemsControl>(GetTemplatedParent()))
        {
            const CDependencyProperty* pdpItemsPanel = NULL;

            // By default ItemsPanel is template should bound.
            // If no template binding exists already then hook it up now
            pdpItemsPanel = GetPropertyByIndexInline(KnownPropertyIndex::ItemsPresenter_ItemsPanel);
            IFCEXPECT_RETURN(pdpItemsPanel);

            if (IsPropertyDefault(pdpItemsPanel) && !IsPropertyTemplateBound(pdpItemsPanel))
            {
                const CDependencyProperty* pdpSource = pItemsControl->GetPropertyByIndexInline(KnownPropertyIndex::ItemsControl_ItemsPanel);
                IFCEXPECT_RETURN(pdpSource);

                IFC_RETURN(SetTemplateBinding(pdpItemsPanel, pdpSource));

                IFC_RETURN(pItemsControl->RefreshTemplateBindings(TemplateBindingsRefreshType::All));
            }
        }

        // Create peer because it participates in Measure/Arrange
        IFC_RETURN(EnsurePeer());

        IFC_RETURN( AddItemsHost(pItemsControl) );

        // Since ItemsPresenter is not a Control, CFE::ApplyTemplate does nothing and hence returns
        // fAddedVisuals as false. However, we *do* add a visual under the ItemsPresenter in the AddItemsHost
        // function - hence set it to true. We need this because we need to have the OnApplyTemplate invoked
        // which is where we will validate which Panel can be inside which Control.
        fAddedVisuals = true;
    }
    return S_OK;
}

//
// CItemsPresenter::AddPanelForItemsControl
//
// Only called from ApplyTemplate to create in the ItemsHost panel.
//
//
_Check_return_
HRESULT
CItemsPresenter::AddItemsHost(_In_ CItemsControl* pItemsControl)
{
    HRESULT hr = S_OK;
    CPanel* pPanel = NULL;
    CREATEPARAMETERS cp(GetContext());

    IFCEXPECT(pItemsControl);

    IFC(pItemsControl->InvalidateItemsHost(/*bHostIsReplaced*/ TRUE));

    if (m_pItemsPanelTemplate)
    {
        // Create the user defined panel.
        IFC( m_pItemsPanelTemplate->LoadContent((CDependencyObject**)&pPanel, this) );
    }
    else
    {
        if (pItemsControl->m_pItemsHost == NULL)
        {
            IFC( CStackPanel::Create((CDependencyObject**) &pPanel, &cp) );

            // set the presenter as templated parent for host panel when we created it
            IFC( pPanel->SetTemplatedParent(this) );
        }
        else
        {
            // use m_pItemsHost which already have been set
            pPanel = pItemsControl->m_pItemsHost;
            pPanel->AddRef();

            if (pItemsControl->m_bItemsHostIsSetFromManaged)
            {
                IFC(pPanel->SetTemplatedParent(this));
            }
        }
    }

    IFCEXPECT_ASSERT(pPanel);

    IFC(pItemsControl->SetValueByKnownIndex(KnownPropertyIndex::ItemsControl_ItemsHost, pPanel));
    IFCEXPECT_ASSERT(pItemsControl->m_pItemsHost == pPanel);

Cleanup:
    ReleaseInterface(pPanel);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CItemsPresenter::MeasureOverride()
//
//  Synopsis:   This will return the desired size of the first (and only)
//              child.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CItemsPresenter::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    if (HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::FrameworkElement_MeasureOverride(
            this,
            availableSize.width,
            availableSize.height,
            &(desiredSize.width),
            &(desiredSize.height)));
    }
    else
    {
        CUIElement* pe = do_pointer_cast<CUIElement>(GetFirstChildNoAddRef());
        // The ItemsControl will only hook up the first ItemsPresenter.  Any other spurious ItemsPresenter(s) will not get children.
        if (pe)
        {
            IFC_RETURN(pe->Measure(availableSize));
            IFC_RETURN(pe->EnsureLayoutStorage());
            desiredSize = pe->DesiredSize;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CItemsPresenter::ArrangeOverride()
//
//  Synopsis:   This will arrange the first (and only) child.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CItemsPresenter::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;

    if (HasManagedPeer())
    {
        IFC(FxCallbacks::FrameworkElement_ArrangeOverride(this, finalSize.width,
            finalSize.height,
            &(newFinalSize.width),
            &(newFinalSize.height)));
    }
    else
    {
        CUIElement* pChild = (CUIElement*) GetFirstChildNoAddRef();
        if (pChild)
        {
            XRECTF arrangeRect = {0, 0, finalSize.width, finalSize.height};
            IFC(pChild->Arrange(arrangeRect));
        }
    }

Cleanup:
    newFinalSize = finalSize;
    RRETURN(hr);
}
