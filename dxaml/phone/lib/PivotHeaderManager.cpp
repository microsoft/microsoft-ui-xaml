// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotHeaderManager.h"
#include "Pivot_Partial.h"
#include "PivotPanel_Partial.h"
#include "PivotHeaderPanel_Partial.h"
#include "PivotHeaderItem.h"
#include <DesignMode.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

//#define PVTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define PVTRACE(...)

PivotHeaderManager::PivotHeaderManager(IPivotHeaderManagerCallbacks* pCallbacks, Private::ReferenceTrackerHelper<Pivot> referenceTrackerHelper)
    : m_pCallbackPtr(pCallbacks)
    , m_currentIndex(0)
    , m_isLocked(FALSE)
    , m_useStaticHeaders(false)
    , m_shouldShowFocusStateOnSelectedItem(false)
    , m_referenceTrackerHelper(referenceTrackerHelper)
{}

_Check_return_ HRESULT
PivotHeaderManager::Initialize()
{
    HRESULT hr = S_OK;

    if (!m_tpGhostLteTranslateTransform)
    {
        wrl::ComPtr<xaml_media::ITranslateTransform> spGhostLteTranslateTransform;
        IFC(wf::ActivateInstance(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_TranslateTransform).Get(),
            &spGhostLteTranslateTransform));
        IFC(SetPtrValue(m_tpGhostLteTranslateTransform, spGhostLteTranslateTransform.Get()));
    }
    if (!m_tpPrimaryLteTranslateTransform)
    {
        wrl::ComPtr<xaml_media::ITranslateTransform> spPrimaryLteTranslateTransform;
        IFC(wf::ActivateInstance(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_TranslateTransform).Get(),
            &spPrimaryLteTranslateTransform));
        IFC(SetPtrValue(m_tpPrimaryLteTranslateTransform, spPrimaryLteTranslateTransform.Get()));
    }

    IFC(RegisterTrackerPtrVector(m_tpHeaderItems));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::ApplyTemplateEvent(_In_ xaml_controls::IPanel* pPanel, _In_ xaml_controls::IPanel* pStaticPanel)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    if (m_tpPanel)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> spPanelAsHeaderPanel;
        IFC(m_tpPanel.As(&spPanelAsHeaderPanel));

        IFC(static_cast<xaml_primitives::PivotHeaderPanel*>(spPanelAsHeaderPanel.Get())->SetHeaderManagerCallbacks(nullptr));

        IFC(m_tpPanel->get_Children(&spChildren));
        IFC(spChildren->Clear());
        spChildren.Reset();
    }

    if (m_tpStaticPanel)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> spStaticPanelAsHeaderPanel;
        IFC(m_tpStaticPanel.As(&spStaticPanelAsHeaderPanel));

        IFC(static_cast<xaml_primitives::PivotHeaderPanel*>(spStaticPanelAsHeaderPanel.Get())->SetHeaderManagerCallbacks(nullptr));

        IFC(m_tpStaticPanel->get_Children(&spChildren));
        IFC(spChildren->Clear());
        spChildren.Reset();
    }

    IFC(SetPtrValue(m_tpPanel, pPanel));
    IFC(SetPtrValue(m_tpStaticPanel, pStaticPanel));

    if (m_tpPanel)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> spPanelAsHeaderPanel;
        IFC(m_tpPanel.As(&spPanelAsHeaderPanel));

        IFC(static_cast<xaml_primitives::PivotHeaderPanel*>(spPanelAsHeaderPanel.Get())->SetHeaderManagerCallbacks(this));
    }

    if (m_tpStaticPanel)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> spStaticPanelAsHeaderPanel;
        IFC(m_tpStaticPanel.As(&spStaticPanelAsHeaderPanel));

        IFC(static_cast<xaml_primitives::PivotHeaderPanel*>(spStaticPanelAsHeaderPanel.Get())->SetHeaderManagerCallbacks(this));
    }

    if (m_tpPanel && !m_tpStaticPanel)
    {
        m_useStaticHeaders = false;
    }
    else if (!m_tpPanel && m_tpStaticPanel)
    {
        m_useStaticHeaders = true;
    }

    if (m_tpPanel && !m_useStaticHeaders)
    {
        IFC(m_tpPanel->get_Children(&spChildren));
        for (auto iter = m_tpHeaderItems.Begin(); iter != m_tpHeaderItems.End(); iter++)
        {
            IFC(spChildren->Append(
                (*iter).AsOrNull<xaml::IUIElement>().Get()));
        }

        IFC(UpdateGhostItem());
    }

    if (m_tpStaticPanel && m_useStaticHeaders)
    {
        IFC(m_tpStaticPanel->get_Children(&spChildren));
        for (auto iter = m_tpHeaderItems.Begin(); iter != m_tpHeaderItems.End(); iter++)
        {
            IFC(spChildren->Append(
                (*iter).AsOrNull<xaml::IUIElement>().Get()));
        }

        IFC(UpdateGhostItem());
    }

Cleanup:
    RRETURN(hr);
}


// NOTE: This function will indirectly cause PivotCurveGenerator::ItemsSizeOrOrderChanged.
// It invalidates the layout of the Panel which in turn causes a HeaderPanelArrange event to
// be fired into HeaderManager.
_Check_return_ HRESULT
PivotHeaderManager::ItemsCollectionChangedEvent(_In_ wfc::IVector<IInspectable*>* pItems,
    _In_ INT32 newItemCount, _In_ INT32 changeIdx, _In_ wfc::CollectionChange changeType)
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(newItemCount);

    IFC(SetPtrValue(m_tpItems, pItems));

    if (changeType == wfc::CollectionChange_ItemChanged ||
        changeType == wfc::CollectionChange_ItemRemoved)
    {
        IFC(RemoveFromPanel(changeIdx));
        m_tpHeaderItems.RemoveAt(changeIdx);
    }

    if (changeType == wfc::CollectionChange_ItemInserted ||
        changeType == wfc::CollectionChange_ItemChanged)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderItem> spHeaderItem;
        wrl::ComPtr<IInspectable> spItem;

        IFC(pItems->GetAt(static_cast<UINT>(changeIdx), &spItem));
        IFC(CreateHeaderItem(&spHeaderItem));
        IFC(SetBinding(spItem.Get(), spHeaderItem.Get()));

        IFC(InsertIntoPanel(spHeaderItem.Get(), changeIdx));

        IFC(m_tpHeaderItems.InsertAt(changeIdx, spHeaderItem.Get()));
    }
    // NOTE: CollectionChange_Reset occurs when databinding to a new list, we
    // must account for all the new items here as well.
    else if (changeType == wfc::CollectionChange_Reset)
    {
        IFC(ClearPanel());

        m_tpHeaderItems.Clear();

        for (INT32 idx = 0; idx < newItemCount; idx++)
        {
            wrl::ComPtr<xaml_primitives::IPivotHeaderItem> spHeaderItem;
            wrl::ComPtr<IInspectable> spItem;

            IFC(pItems->GetAt(static_cast<UINT>(idx), &spItem));
            IFC(CreateHeaderItem(&spHeaderItem));
            IFC(SetBinding(spItem.Get(), spHeaderItem.Get()));

            IFC(InsertIntoPanel(spHeaderItem.Get(), idx));

            IFC(m_tpHeaderItems.Append(spHeaderItem.Get()));
        }
    }

    IFC(UpdateGhostItem());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::HeaderTemplateChangedEvent()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IDataTemplate> spNewTemplate;
    IFC(m_pCallbackPtr->GetHeaderTemplate(&spNewTemplate));

    // This will cause a layout pass to occurr which will then cause
    // the state machine to reapply the secondary content relationship.
    for (auto iter = m_tpHeaderItems.Begin(); iter != m_tpHeaderItems.End(); iter++)
    {
        IFC((*iter).AsOrNull<xaml_controls::IContentControl>()->put_ContentTemplate(spNewTemplate.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::VsmUnlockedStateChangeCompleteEvent()
{
    RRETURN(UpdateGhostItem());
}

_Check_return_ HRESULT
PivotHeaderManager::HeaderPanelMeasureEvent(float viewportSize)
{
    return m_pCallbackPtr->OnHeaderPanelMeasure(viewportSize);
}

_Check_return_ HRESULT
PivotHeaderManager::HeaderPanelSetLteOffsetEvent(_In_ DOUBLE primaryHorizontalOffset, _In_ DOUBLE ghostHorizontalOffset, _In_ DOUBLE verticalOffset)
{
    HRESULT hr = S_OK;

    IFC(m_tpPrimaryLteTranslateTransform->put_Y(verticalOffset));
    IFC(m_tpGhostLteTranslateTransform->put_Y(verticalOffset));
    IFC(m_tpPrimaryLteTranslateTransform->put_X(primaryHorizontalOffset));
    IFC(m_tpGhostLteTranslateTransform->put_X(ghostHorizontalOffset));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::IsLockedChangedEvent(_In_ BOOLEAN isLocked)
{
    if (isLocked != m_isLocked)
    {
        m_isLocked = isLocked;
        for (UINT idx = 0; idx < m_tpHeaderItems.Size(); idx++)
        {
            IFC_RETURN(static_cast<xaml_primitives::PivotHeaderItem*>(m_tpHeaderItems.GetAt(idx))->UpdateVisualState(true));
        }

        if (isLocked)
        {
            IFC_RETURN(ClearExistingLtes());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::UnloadedEvent()
{
    HRESULT hr = S_OK;

    // The SecondaryContentRelationship is removed here to ensure
    // that upon reentering the visual tree it will be reapplied.
    // For reasons unknown InputManager doesn't have the correct logic
    // to reapply it.
    if (m_tpDynamicHeaderRelationship)
    {
        IFC(m_tpDynamicHeaderRelationship->Remove());
        m_tpDynamicHeaderRelationship.Clear();
    }

    if (m_tpStaticHeaderRelationship)
    {
        IFC(m_tpStaticHeaderRelationship->Remove());
        m_tpStaticHeaderRelationship.Clear();
    }

    if (m_tpStaticLayoutRelationship)
    {
        IFC(m_tpStaticLayoutRelationship->Remove());
        m_tpStaticLayoutRelationship.Clear();
    }

    if (m_tpInverseStaticLayoutRelationship)
    {
        IFC(m_tpInverseStaticLayoutRelationship->Remove());
        m_tpInverseStaticLayoutRelationship.Clear();
    }

    IFC(ClearExistingLtes());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::HeaderStateChangedEvent(_In_ bool useStaticHeaders)
{
    if (m_useStaticHeaders == useStaticHeaders)
    {
        return S_OK;
    }

    // We expect to have the panel associated with the header state we're in.
    // The Pivot should not call this if it doesn't have the panel for the desired header state.
    if (useStaticHeaders)
    {
        IFCEXPECT_RETURN(m_tpStaticPanel);
    }
    else
    {
        IFCEXPECT_RETURN(m_tpPanel);
    }

    // Clear the current header panel so we can add the header items to the other panel instead.
    IFC_RETURN(ClearPanel());

    m_useStaticHeaders = useStaticHeaders;
    int headerIndex = 0;

    // Now put the header items into the other panel.
    for (auto iter = m_tpHeaderItems.Begin(); iter != m_tpHeaderItems.End(); iter++)
    {
        IFC_RETURN(InsertIntoPanel((*iter).Get(), headerIndex++));
    }

    IFC_RETURN(UpdateGhostItem());

    return S_OK;
}

_Check_return_ HRESULT PivotHeaderManager::FocusStateChangedEvent(_In_ bool shouldShowFocusStateOnSelectedItem)
{
    m_shouldShowFocusStateOnSelectedItem = shouldShowFocusStateOnSelectedItem;
    return UpdateItemsStates();
}

_Check_return_ HRESULT
PivotHeaderManager::SetSelectedIndex(_In_ INT32 idx, _In_ PivotAnimationDirection animationHint)
{
    HRESULT hr = S_OK;

    IFC(m_curveGenerator.SelectedIndexChangedEvent(idx, animationHint));
    m_currentIndex = idx;

    // NOTE: Updating the ghost item invalidates the arrange of
    // PivotHeaderPanel by calling SyncParallax, which will in
    // turn cause a parametric curve update and
    // propogate the new index down.
    IFC(UpdateGhostItem());
    IFC(UpdateItemsStates());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::PivotHeaderItemTapped(_In_ xaml_primitives::IPivotHeaderItem* pItem)
{
    HRESULT hr = S_OK;

    INT itemIdx = -1;

    PVTRACE(L"[PHM]: Item tapped.");

    for (UINT idx = 0; idx < m_tpHeaderItems.Size(); idx++)
    {
        if (m_tpHeaderItems.GetAt(idx) == pItem)
        {
            itemIdx = static_cast<INT>(idx);
            break;
        }
    }

    // We ignore the ghost item, which isn't present in m_tpHeaderItems.
    if (itemIdx != -1)
    {
        IFC(m_pCallbackPtr->OnHeaderItemTapped(itemIdx, true /* shouldPlaySound */));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::ClearExistingLtes()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Internal::ILayoutTransitionElementUtilitiesStatics> spLteUtilities;

    // Fast path: if already cleared bail out early.
    if (!m_tpGhostLte)
    {
        ASSERT(!m_tpPrimaryLte && !m_tpLteSource && !m_tpLteParent);
        goto Cleanup;
    }

    // NOTE: This private API is not available on desktop windows.
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Internal_LayoutTransitionElementUtilities).Get(),
        &spLteUtilities));

    if (m_tpGhostLte)
    {
        IFC(spLteUtilities->DestroyLayoutTransitionElement(
            m_tpLteSource.Get(), m_tpLteParent.Get(), m_tpGhostLte.Get()));
    }

    if (m_tpPrimaryLte)
    {
        IFC(spLteUtilities->DestroyLayoutTransitionElement(
            m_tpLteSource.Get(), m_tpLteParent.Get(), m_tpPrimaryLte.Get()));
    }

    m_tpPrimaryLte.Clear();
    m_tpGhostLte.Clear();
    m_tpLteSource.Clear();
    m_tpLteParent.Clear();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::UpdateGhostItem()
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IUIElement> spLteSource;

    // We only want a ghost item when we're using dynamic headers,
    // so skip all of this if we're using static headers.
    if (m_tpHeaderItems.Size() > 0 && !m_useStaticHeaders)
    {
        UINT ghostItemIdx = PositiveMod(m_currentIndex - 1, static_cast<INT32>(m_tpHeaderItems.Size()));

        // Update all of our items such that only a single item is subscribed to
        // VSM state transition completion callbacks.
        for (UINT idx = 0; idx < m_tpHeaderItems.Size(); idx++)
        {
            IFC((static_cast<xaml_primitives::PivotHeaderItem*>(m_tpHeaderItems.GetAt(idx)))->
                SetSubscribeToStateChangeCallback(idx == ghostItemIdx));
        }

        IFC(m_tpHeaderItems.GetAt(ghostItemIdx)->QueryInterface<xaml::IUIElement>(&spLteSource));

        // Optimization: if they are identical do not go through the effort of tearing down and
        // rebuilding.
        if (spLteSource.Get() == m_tpLteSource.Get() && !m_isLocked)
        {
            goto Cleanup;
        }
    }

    IFC(ClearExistingLtes());

    // When we're locked we make sure not to display the ghost item, doing so
    // will cause the locked translation animation to apply to the ghost item too
    // which looks incorrect.
    if (!m_isLocked && spLteSource)
    {
        wrl::ComPtr<xaml::Internal::ILayoutTransitionElementUtilitiesStatics> spLteUtilities;

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Internal_LayoutTransitionElementUtilities).Get(),
            &spLteUtilities));

        if (m_tpPanel)
        {
            wrl::ComPtr<xaml::IUIElement> spLteParent;
            IFC(m_tpPanel.As(&spLteParent));
            IFC(SetPtrValue(m_tpLteParent, spLteParent.Get()));
            IFC(SetPtrValue(m_tpLteSource, spLteSource.Get()));

            if (m_tpLteParent)
            {
                wrl::ComPtr<xaml::IUIElement> spPrimaryLte;
                wrl::ComPtr<xaml::IUIElement> spGhostLte;

                IFC(spLteUtilities->CreateLayoutTransitionElement(
                    m_tpLteSource.Get(), m_tpLteParent.Get(), &spPrimaryLte));
                IFC(SetPtrValue(m_tpPrimaryLte, spPrimaryLte.Get()));

                IFC(spLteUtilities->CreateLayoutTransitionElement(
                    m_tpLteSource.Get(), m_tpLteParent.Get(), &spGhostLte));
                IFC(SetPtrValue(m_tpGhostLte, spGhostLte.Get()));

                IFC(m_tpPrimaryLte->put_RenderTransform(m_tpPrimaryLteTranslateTransform.AsOrNull<xaml_media::ITransform>().Get()));
                IFC(m_tpGhostLte->put_RenderTransform(m_tpGhostLteTranslateTransform.AsOrNull<xaml_media::ITransform>().Get()));
            }
        }
    }

    // Even if we didn't set a ghost item, we want to invalidate the measure of the header panel
    // to ensure PivotHeaderManager pushes a new header offset to the HeaderPanel.
    IFC(SyncParallax());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::InsertIntoPanel(_In_ xaml_primitives::IPivotHeaderItem* pItem, _In_ INT32 idx)
{
    HRESULT hr = S_OK;

    if (m_tpPanel && !m_useStaticHeaders)
    {
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spItems;
        wrl::ComPtr<xaml::IUIElement> spItemAsUIE;
        IFC(pItem->QueryInterface<xaml::IUIElement>(&spItemAsUIE));
        IFC(m_tpPanel->get_Children(&spItems));
        IFC(spItems->InsertAt(idx, spItemAsUIE.Get()));
    }
    else if (m_tpStaticPanel && m_useStaticHeaders)
    {
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spItems;
        wrl::ComPtr<xaml::IUIElement> spItemAsUIE;
        IFC(pItem->QueryInterface<xaml::IUIElement>(&spItemAsUIE));
        IFC(m_tpStaticPanel->get_Children(&spItems));
        IFC(spItems->InsertAt(idx, spItemAsUIE.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::RemoveFromPanel(_In_ INT32 idx)
{
    HRESULT hr = S_OK;

    if (m_tpPanel && !m_useStaticHeaders)
    {
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spItems;
        IFC(ClearExistingLtes());
        IFC(m_tpPanel->get_Children(&spItems));
        IFC(spItems->RemoveAt(idx));
    }
    else if (m_tpStaticPanel && m_useStaticHeaders)
    {
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spItems;
        IFC(ClearExistingLtes());
        IFC(m_tpStaticPanel->get_Children(&spItems));
        IFC(spItems->RemoveAt(idx));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::ClearPanel()
{
    HRESULT hr = S_OK;

    if (m_tpPanel && !m_useStaticHeaders)
    {
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spItems;
        IFC(ClearExistingLtes());
        IFC(m_tpPanel->get_Children(&spItems));
        IFC(spItems->Clear());
    }
    else if (m_tpStaticPanel && m_useStaticHeaders)
    {
        wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spItems;
        IFC(ClearExistingLtes());
        IFC(m_tpStaticPanel->get_Children(&spItems));
        IFC(spItems->Clear());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::UpdateItemsStates()
{
    for (UINT idx = 0; idx < m_tpHeaderItems.Size(); idx++)
    {
        auto pivotHeaderItem = static_cast<xaml_primitives::PivotHeaderItem*>(m_tpHeaderItems.GetAt(idx));

        // We reset the "hover" state for all items so that the selected item
        // appears in SelectedRest and NOT SelectedPointerOver visual state.
        pivotHeaderItem->ClearIsHovered();

        pivotHeaderItem->SetIsSelected(idx == static_cast<UINT>(m_currentIndex));
        pivotHeaderItem->SetShouldShowFocusWhenSelected(m_shouldShowFocusStateOnSelectedItem);

        // We should call UpdateVisualState() to have the pivotHeaderItem appear in
        // correct visual state.
        IFC_RETURN(pivotHeaderItem->UpdateVisualState(true));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::CreateHeaderItem(_Outptr_ xaml_primitives::IPivotHeaderItem** ppHeaderItem)
{
    wrl::ComPtr<xaml::IDataTemplate> spHeaderTemplate;
    wrl::ComPtr<xaml_primitives::PivotHeaderItem> spHeaderItem;
    wrl::ComPtr<xaml_controls::IContentControl> spHeaderItemAsContentControl;

    *ppHeaderItem = nullptr;

    IFC_RETURN(wrl::MakeAndInitialize<xaml_primitives::PivotHeaderItem>(&spHeaderItem));
    IFC_RETURN(spHeaderItem.As(&spHeaderItemAsContentControl));

    IFC_RETURN(m_pCallbackPtr->GetHeaderTemplate(&spHeaderTemplate));
    IFC_RETURN(spHeaderItemAsContentControl->put_ContentTemplate(spHeaderTemplate.Get()));

    IFC_RETURN(spHeaderItem->SetHeaderManagerCallbacks(this));

    IFC_RETURN(spHeaderItem->UpdateVisualState(false));

    *ppHeaderItem = spHeaderItem.Detach();

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::SetBinding(_In_ IInspectable* pItem, _In_ xaml_primitives::IPivotHeaderItem* pHeaderItem)
{
    wrl::ComPtr<xaml_primitives::IPivotHeaderItem> spHeaderItem(pHeaderItem);

    wrl::ComPtr<xaml_data::IBinding> spHeaderBinding;
    wrl::ComPtr<xaml_data::IBindingBase> spHeaderBindingBase;
    wrl::ComPtr<xaml::IPropertyPath> spPropertyPath;
    wrl::ComPtr<xaml::IDependencyProperty> spContentProperty;
    wrl::ComPtr<xaml::IFrameworkElement> spHeaderItemAsFE;
    wrl::ComPtr<IInspectable> spItem(pItem);
    wrl::ComPtr<IPivotItem> spPivotItem;

    IFC_RETURN(spHeaderItem.As(&spHeaderItemAsFE));

    // When the items are PivotItems we implicitly bind the headers
    // to the Header property. When they aren't PivotItems they have
    // access to the full item's data context.
    IGNOREHR(spItem.As(&spPivotItem));
    if (nullptr != spPivotItem)
    {
        wrl::ComPtr<xaml::IPropertyPathFactory> spPropertyPathFactory;
        IFC_RETURN(wf::GetActivationFactory(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_PropertyPath).Get(),
              &spPropertyPathFactory));

        IFC_RETURN(spPropertyPathFactory->CreateInstance(
              wrl_wrappers::HStringReference(L"Header").Get(),
              &spPropertyPath));

        wrl::ComPtr<xaml::IUIElement> pivotItemAsUI;
        IFC_RETURN(spPivotItem.As(&pivotItemAsUI));

        wrl::ComPtr<xaml::IDependencyObject> itemHeaderAsDO;
        IFC_RETURN(spHeaderItem.As(&itemHeaderAsDO));

        // Set the keytip target as an item header for the current pivot item.
        IFCFAILFAST(pivotItemAsUI->put_KeyTipTarget(itemHeaderAsDO.Get()));
        // Set the tooltip target as an item header for the current pivot item.
        IFCFAILFAST(pivotItemAsUI->put_KeyboardAcceleratorPlacementTarget(itemHeaderAsDO.Get()));
    }

    IFC_RETURN(wf::ActivateInstance(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Data_Binding).Get(),
          &spHeaderBinding));
    IFC_RETURN(spHeaderBinding->put_Source(pItem));
    IFC_RETURN(spHeaderBinding->put_Mode(xaml_data::BindingMode_OneWay));
    IFC_RETURN(spHeaderBinding->put_Path(spPropertyPath.Get()));

    {
        wrl::ComPtr<xaml_controls::IContentControlStatics> spContentControlStatics;
        IFC_RETURN(wf::GetActivationFactory(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_ContentControl).Get(),
              &spContentControlStatics));
        IFC_RETURN(spContentControlStatics->get_ContentProperty(&spContentProperty));
    }

    IFC_RETURN(spHeaderBinding.As(&spHeaderBindingBase));
    IFC_RETURN(spHeaderItemAsFE->SetBinding(spContentProperty.Get(), spHeaderBindingBase.Get()));

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::SyncParallax()
{
    HRESULT hr = S_OK;

    if (m_tpPanel)
    {
        IFC((Private::As<xaml::IUIElement, xaml_controls::IPanel>(m_tpPanel.Get()))->InvalidateArrange());
    }

    if (m_tpStaticPanel)
    {
        IFC((Private::As<xaml::IUIElement, xaml_controls::IPanel>(m_tpStaticPanel.Get()))->InvalidateArrange());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::UpdateCurveHeaderItemSizes()
{
    HRESULT hr = S_OK;

    std::vector<double> itemSizes;
    double totalSize = 0.0;

    IFC(GetHeaderWidths(&itemSizes, &totalSize));
    IFC(m_curveGenerator.SyncItemsSizeAndOrder(itemSizes, totalSize));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderManager::ApplyParallax(
    _In_ xaml_controls::IScrollViewer* pScrollViewer,
    _In_ xaml_controls::IPanel* pPanel,
    _In_ xaml_media::ICompositeTransform* pTransform,
    _In_ DOUBLE sectionOffset,
    _In_ DOUBLE sectionWidth,
    _In_ bool isHeaderPanelInsideLayoutElementTemplatePart,
    _In_ bool isDynamicHeader,
    _In_ float viewportSize)
{
    // NOTE: In regards to timing this function should ALWAYS be called after
    // the PivotHeaderPanel items have been measured and have a known desired
    // size, but BEFORE submission to the compositor occurs.
    // This function can be called DURING a manipulation and AFTER a manipulation
    // but will perform slightly different behaviors depending:
    // - If DURING a manipulation we jump through a lot of hoops to keep the
    //   Pivot header items from glitching/jumping. DManip is continuously applying
    //   the parametric curve.
    // - If AFTER a manipulation the parametric curve is evaluated and applied
    //   as a XAML transform.

    IFC_RETURN(m_curveGenerator.SyncSectionWidth(sectionWidth));
    IFC_RETURN(m_curveGenerator.SyncSelectedItemOffset(sectionOffset));
    IFC_RETURN(UpdateCurveHeaderItemSizes());

    auto& secondaryRelationship = isDynamicHeader ? m_tpDynamicHeaderRelationship : m_tpStaticHeaderRelationship;

    if (m_curveGenerator.AreCurvesDirty(isDynamicHeader) || !secondaryRelationship)
    {
        wrl::ComPtr<xaml::Internal::ISecondaryContentRelationship> spSCR;
        wrl::ComPtr<xaml::IUIElement> spScrollViewerAsUIE;
        wrl::ComPtr<xaml::IUIElement> spPanelAsUIE;
        wrl::ComPtr<xaml::IDependencyObject> spTransformAsDO;

        std::vector<DOUBLE> primaryOffsets;
        std::vector<DOUBLE> secondaryOffsets;

        IFC_RETURN(pScrollViewer->QueryInterface<xaml::IUIElement>(&spScrollViewerAsUIE));
        IFC_RETURN(pPanel->QueryInterface<xaml::IUIElement>(&spPanelAsUIE));
        IFC_RETURN(pTransform->QueryInterface<xaml::IDependencyObject>(&spTransformAsDO));

        BOOLEAN isScrollViewerInManipulation = FALSE;
        {
            wrl::ComPtr<xaml_controls::IScrollViewerPrivate> spPrivateScrollViewer;
            IFC_RETURN(pScrollViewer->QueryInterface<xaml_controls::IScrollViewerPrivate>(&spPrivateScrollViewer));
            IFC_RETURN(spPrivateScrollViewer->get_IsInDirectManipulation(&isScrollViewerInManipulation));
        }

        if (isDynamicHeader)
        {
            m_curveGenerator.GetDynamicCurveSegments(
                primaryOffsets,
                secondaryOffsets,
                m_pCallbackPtr->GetPivotPanelMultiplier());

            if (isHeaderPanelInsideLayoutElementTemplatePart)
            {
                // The layout element follows the viewport.
                // In case the layout element part is present, it means the header panel is inside it and we need
                // to adjust the secondary offsets.
                for (unsigned i = 1; i < static_cast<unsigned>(primaryOffsets.size()) - 1; ++i)
                {
                    secondaryOffsets[i] += primaryOffsets[i];
                }

                secondaryOffsets[0] = secondaryOffsets[1];
                secondaryOffsets[primaryOffsets.size() - 1] = secondaryOffsets[primaryOffsets.size() - 2];
            }
        }
        else
        {
            ASSERT(isHeaderPanelInsideLayoutElementTemplatePart);

            m_curveGenerator.GetStaticCurveSegments(
                primaryOffsets,
                secondaryOffsets,
                viewportSize);
        }

        PVTRACE(L"[PHM]: Parametric curve definitions:");
        for (UINT idx = 0; idx < static_cast<UINT>(primaryOffsets.size()); idx++)
        {
            PVTRACE(L"[PHM]: (%f) -> (%f)", primaryOffsets[idx], secondaryOffsets[idx]);
        }
        PVTRACE(L"[PHM]: Applying parametric curves.");

        wrl::ComPtr<xaml::Internal::ISecondaryContentRelationshipStatics> spSecondaryContentRelationshipStatics;
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Internal_SecondaryContentRelationship).Get(),
            &spSecondaryContentRelationshipStatics));
        IFC_RETURN(spSecondaryContentRelationshipStatics->CreateParallaxRelationship(
            spScrollViewerAsUIE.Get(),
            spPanelAsUIE.Get(),
            spTransformAsDO.Get(),
            static_cast<UINT>(primaryOffsets.size()),
            primaryOffsets.data(),
            static_cast<UINT>(secondaryOffsets.size()),
            secondaryOffsets.data(),
            &spSCR));

        IFC_RETURN(spSCR->Apply());

        if (!isScrollViewerInManipulation)
        {
            PVTRACE(L"[PHM]: ScrollViewer not in manipulation. Applying XAML transform");
            IFC_RETURN(spSCR->UpdateDependencyProperties());
        }
        IFC_RETURN(SetPtrValue(secondaryRelationship, spSCR.Get()));
    }

    // The dynamic header panel offset is so deeply tied to the parametric curve we ALWAYS and ONLY update
    // it here as well. The static header panel offset is always 0.
    if(isDynamicHeader)
    {
        double headerPanelOffset;

        headerPanelOffset = m_curveGenerator.GetHeaderPanelOffset();

        wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> spPanelAsHeader;
        IFC_RETURN(m_tpPanel.As(&spPanelAsHeader));
        IFC_RETURN((static_cast<xaml_primitives::PivotHeaderPanel*>(spPanelAsHeader.Get()))->SetCurrentOffset(headerPanelOffset));
        IFC_RETURN((static_cast<xaml_primitives::PivotHeaderPanel*>(spPanelAsHeader.Get()))->SetCurrentIndex(m_currentIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::ApplyStaticLayoutRelationship(
    _In_ xaml_controls::IScrollViewer* scrollViewer,
    _In_ xaml::IUIElement* element,
    _In_ xaml_media::ICompositeTransform* transform,
    _In_ double sectionWidth)
{
    RRETURN(ApplyStaticRelationship(
        scrollViewer,
        element,
        transform,
        false /* isInverted */,
        sectionWidth,
        m_tpStaticLayoutRelationship));
}

_Check_return_ HRESULT
PivotHeaderManager::ApplyInverseStaticLayoutRelationship(
    _In_ xaml_controls::IScrollViewer* scrollViewer,
    _In_ xaml::IUIElement* element,
    _In_ xaml_media::ICompositeTransform* transform,
    _In_ double sectionWidth)
{
    RRETURN(ApplyStaticRelationship(
        scrollViewer,
        element,
        transform,
        true /* isInverted */,
        sectionWidth,
        m_tpInverseStaticLayoutRelationship));
}

_Check_return_ HRESULT
PivotHeaderManager::ApplyStaticRelationship(
    _In_ xaml_controls::IScrollViewer* pScrollViewer,
    _In_ xaml::IUIElement* pElement,
    _In_ xaml_media::ICompositeTransform* pTransform,
    _In_ bool isInverted,
    _In_ double sectionWidth,
    _Inout_ Private::TrackerPtr<xaml::Internal::ISecondaryContentRelationship> &tpRelationship)
{
    // NOTE: In regards to timing this function should ALWAYS be called after
    // the PivotHeaderPanel items have been measured and have a known desired
    // size, but BEFORE submission to the compositor occurs.
    // This function can be called DURING a manipulation and AFTER a manipulation
    // but will perform slightly different behaviors depending:
    // - If DURING a manipulation we jump through a lot of hoops to keep the
    //   Pivot header items from glitching/jumping. DManip is continuously applying
    //   the parametric curve.
    // - If AFTER a manipulation the parametric curve is evaluated and applied
    //   as a XAML transform.

    PivotStaticContentCurve& curve = isInverted ? m_staticContentInverseCurve : m_staticContentCurve;

    const unsigned itemsCount = m_tpHeaderItems.Size();
    const bool isHeaderItemsCarouselEnabled = m_pCallbackPtr->IsHeaderItemsCarouselEnabled();

    if (!tpRelationship || curve.IsCurveDirty(itemsCount, sectionWidth, isHeaderItemsCarouselEnabled))
    {
        wrl::ComPtr<xaml::Internal::ISecondaryContentRelationshipStatics> spSecondaryContentRelationshipStatics;
        wrl::ComPtr<xaml::Internal::ISecondaryContentRelationship> spSCR;
        wrl::ComPtr<xaml::IUIElement> spScrollViewerAsUIE;
        wrl::ComPtr<xaml::IDependencyObject> spTransformAsDO;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Internal_SecondaryContentRelationship).Get(),
            &spSecondaryContentRelationshipStatics));

        IFC_RETURN(pScrollViewer->QueryInterface<xaml::IUIElement>(&spScrollViewerAsUIE));
        IFC_RETURN(pTransform->QueryInterface<xaml::IDependencyObject>(&spTransformAsDO));

        std::vector<double> primaryOffsets;
        std::vector<double> secondaryOffsets;
        IFC_RETURN(curve.GetCurveSegments(
            primaryOffsets,
            secondaryOffsets,
            itemsCount,
            sectionWidth,
            isHeaderItemsCarouselEnabled,
            isInverted));

        IFC_RETURN(spSecondaryContentRelationshipStatics->CreateParallaxRelationship(
            spScrollViewerAsUIE.Get(),
            pElement,
            spTransformAsDO.Get(),
            static_cast<UINT>(primaryOffsets.size()),
            primaryOffsets.data(),
            static_cast<UINT>(secondaryOffsets.size()),
            secondaryOffsets.data(),
            &spSCR));

        IFC_RETURN(spSCR->Apply());
        IFC_RETURN(SetPtrValue(tpRelationship, spSCR.Get()));
    }

    // We still want to update the dependency properties even if we don't need to create the relationship,
    // since circumstances could have changed if this is being called.
    BOOLEAN isScrollViewerInManipulation = FALSE;
    {
        wrl::ComPtr<xaml_controls::IScrollViewerPrivate> spPrivateScrollViewer;
        IFC_RETURN(pScrollViewer->QueryInterface<xaml_controls::IScrollViewerPrivate>(&spPrivateScrollViewer));
        IFC_RETURN(spPrivateScrollViewer->get_IsInDirectManipulation(&isScrollViewerInManipulation));
    }

    if (!isScrollViewerInManipulation)
    {
        PVTRACE(L"[PHM]: ScrollViewer not in manipulation. Applying XAML transform");
        IFC_RETURN(tpRelationship->UpdateDependencyProperties());
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::GetHeaderWidths(_Out_ std::vector<double> *pHeaderWidths, _Out_opt_ double *pTotalWidth)
{
    std::vector<DOUBLE> headerWidths;
    DOUBLE totalWidth = 0.0;

    headerWidths.reserve(m_tpHeaderItems.Size());
    for (UINT idx = 0; idx < m_tpHeaderItems.Size(); idx++)
    {
        wf::Size desiredSize = {};
        DOUBLE actualWidth = 0.0;
        IFC_RETURN((Private::As<xaml::IUIElement, xaml_primitives::IPivotHeaderItem>(m_tpHeaderItems.GetAt(idx)))->get_DesiredSize(&desiredSize));
        actualWidth = desiredSize.Width;
        totalWidth += actualWidth;
        headerWidths.push_back(actualWidth);
    }

    *pHeaderWidths = std::move(headerWidths);

    if (pTotalWidth != NULL)
    {
        *pTotalWidth = totalWidth;
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::Dispose()
{
    IFC_RETURN(DetachCallbackPointers());
    IFC_RETURN(ClearExistingLtes());
    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderManager::DetachCallbackPointers()
{
    // This method is called by the Pivot destructor, we we need to use TryGetSafeReference throughout to avoid
    // crashes where the TrackerPtr target is a collected object in a zombie state.

    for (UINT idx = 0; idx < m_tpHeaderItems.Size(); idx++)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderItem> pivotHeaderItemInterface;
        if (m_tpHeaderItems.TryGetSafeReferenceAt(idx, &pivotHeaderItemInterface) && pivotHeaderItemInterface)
        {
            IFC_RETURN(static_cast<xaml_primitives::PivotHeaderItem*>(pivotHeaderItemInterface.Get())->SetHeaderManagerCallbacks(nullptr));
        }

    }

    wrl::ComPtr<xaml_controls::IPanel> panel;
    if (m_tpPanel.TryGetSafeReference(&panel) && panel)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> panelAsHeaderPanel;
        IFC_RETURN(panel.As(&panelAsHeaderPanel));

        IFC_RETURN(static_cast<xaml_primitives::PivotHeaderPanel*>(panelAsHeaderPanel.Get())->SetHeaderManagerCallbacks(nullptr));
    }

    wrl::ComPtr<xaml_controls::IPanel> staticPanel;
    if (m_tpStaticPanel.TryGetSafeReference(&staticPanel) && staticPanel)
    {
        wrl::ComPtr<xaml_primitives::IPivotHeaderPanel> staticPanelAsHeaderPanel;
        IFC_RETURN(staticPanel.As(&staticPanelAsHeaderPanel));

        IFC_RETURN(static_cast<xaml_primitives::PivotHeaderPanel*>(staticPanelAsHeaderPanel.Get())->SetHeaderManagerCallbacks(nullptr));
    }

    return S_OK;
}


} } } } XAML_ABI_NAMESPACE_END
