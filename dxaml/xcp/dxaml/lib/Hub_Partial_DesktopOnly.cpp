// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Hub.g.h"
#include "HubSection.g.h"
#include "HubSectionCollection.g.h"
#include "ScrollViewer.g.h"
#include "SemanticZoom.g.h"
#include "SemanticZoomLocation.g.h"
#include "Panel.g.h"
#include "IItemLookupPanel.g.h"
#include "focusmgr.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


_Check_return_ IFACEMETHODIMP
Hub::get_View(
    _Outptr_ wfc::IVector<IInspectable*>** ppView)
{
    HRESULT hr = S_OK;

    IFC(m_tpSections.CopyTo(ppView));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
Hub::get_CollectionView(
    _Outptr_ xaml_data::ICollectionView** ppCollectionView)
{
    HRESULT hr = S_OK;

    IFCPTR(ppCollectionView);
    *ppCollectionView = nullptr;

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
Hub::IsItemItsOwnContainer(
    _In_ IInspectable* pItem,
    _Out_ BOOLEAN* pIsOwnContainer)
{
    HRESULT hr = S_OK;

    IFCPTR(pIsOwnContainer);
    *pIsOwnContainer = !!ctl::is<IHubSection>(pItem);

    // We are not expecting IGeneratorHost::IsItemItsOwnContainer() to get called on Hub with anything but HubSections
    // that are members of Hub.Sections.  However, we still QI for IHubSection on the item since that is the correct
    // implementation of IsItemItsOwnContainer().
    // Since IGeneratorHost is an internal interface, we control when Hub::IsItemItsOwnContainer() is called, and we
    // should never call it with any items that are not members of Hub.Sections.  Thus, we never expect it to return
    // false (and thus pItem is never expected to be NULL).
    ASSERT(*pIsOwnContainer, L"Unsupported case: called Hub::IsItemItsOwnContainer with an element that was not in Hub.Sections");

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
Hub::GetContainerForItem(
    _In_ IInspectable* pItem,
    _In_opt_ xaml::IDependencyObject* pRecycledContainer,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ IFACEMETHODIMP
Hub::PrepareItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spHubSection;

    IFC(ctl::do_query_interface(spHubSection, pContainer));
    IFC(spHubSection.Cast<HubSection>()->SetParentHub(this));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
Hub::ClearContainerForItem(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ IInspectable* pItem)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spHubSection;

    IFC(ctl::do_query_interface(spHubSection, pContainer));
    IFC(spHubSection.Cast<HubSection>()->SetParentHub(nullptr));

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
Hub::IsHostForItemContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pIsHost)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ IFACEMETHODIMP
Hub::GetGroupStyle(
    _In_opt_ xaml_data::ICollectionViewGroup* pGroup,
    _In_ UINT level,
    _Out_ xaml_controls::IGroupStyle** ppGroupStyle)
{
    // The modern panel is always going to ask for a GroupStyle.
    // Fortunately, it's perfectly valid to return null
    *ppGroupStyle = nullptr;
    RRETURN(S_OK);
}

_Check_return_ IFACEMETHODIMP
Hub::SetIsGrouping(
    _In_ BOOLEAN isGrouping)
{
    ASSERT(!isGrouping);
    RRETURN(S_OK);
}

// we don't expose this publicly, there is an override for our own controls
// to mirror the public api
_Check_return_ IFACEMETHODIMP
Hub::GetHeaderForGroup(
    _In_ IInspectable* pGroup,
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ IFACEMETHODIMP
Hub::PrepareGroupContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _In_ xaml_data::ICollectionViewGroup* pGroup)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ IFACEMETHODIMP
Hub::ClearGroupContainerForGroup(
    _In_ xaml::IDependencyObject* pContainer,
    _In_opt_ xaml_data::ICollectionViewGroup* pItem)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ IFACEMETHODIMP
Hub::CanRecycleContainer(
    _In_ xaml::IDependencyObject* pContainer,
    _Out_ BOOLEAN* pCanRecycleContainer)
{
    *pCanRecycleContainer = TRUE;
    RRETURN(S_OK);
}

_Check_return_ IFACEMETHODIMP
Hub::SuggestContainerForContainerFromItemLookup(
    _Outptr_ xaml::IDependencyObject** ppContainer)
{
    // hub has no clue
    *ppContainer = nullptr;
    RRETURN(S_OK);
}

// Supports the IGeneratorHost interface.
_Check_return_ HRESULT
    Hub::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (iid == __uuidof(IGeneratorHost))
    {
        *ppObject = static_cast<IGeneratorHost*>(this);
    }
    else
    {
        RRETURN(HubGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Keyboard navigation
_Check_return_ HRESULT
    Hub::HandleNavigationKey(
    _In_ HubSection* pSection,
    _In_ wsy::VirtualKey key,
    _Out_ BOOLEAN* pWasHandled)
{
    HRESULT hr = S_OK;

    xaml_controls::Orientation orientation = xaml_controls::Orientation_Horizontal;
    xaml::FlowDirection flowDirection = xaml::FlowDirection_LeftToRight;

    IFC(get_Orientation(&orientation));
    IFC(get_FlowDirection(&flowDirection));

    switch (key)
    {
    case wsy::VirtualKey::VirtualKey_Down:
        if (orientation == xaml_controls::Orientation_Vertical)
        {
            IFC(FocusNextSection(pSection, TRUE, pWasHandled));
        }
        break;
    case wsy::VirtualKey::VirtualKey_Right:
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            IFC(FocusNextSection(pSection, flowDirection == xaml::FlowDirection_LeftToRight, pWasHandled));
        }
        break;
    case wsy::VirtualKey::VirtualKey_PageDown:
        IFC(FocusNextSection(pSection, TRUE, pWasHandled));
        break;
    case wsy::VirtualKey::VirtualKey_Up:
        if (orientation == xaml_controls::Orientation_Vertical)
        {
            IFC(FocusNextSection(pSection, FALSE, pWasHandled));
        }
        break;
    case wsy::VirtualKey::VirtualKey_Left:
        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            IFC(FocusNextSection(pSection, flowDirection != xaml::FlowDirection_LeftToRight, pWasHandled));
        }
        break;
    case wsy::VirtualKey::VirtualKey_PageUp:
        IFC(FocusNextSection(pSection, FALSE, pWasHandled));
        break;
    default:
        ASSERT(FALSE);
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

// Helper method for keyboard navigation to move focus to an adjacent section.
_Check_return_ HRESULT
    Hub::FocusNextSection(
    _In_ HubSection* pSection,
    _In_ BOOLEAN forwardDirection,
    _Out_ BOOLEAN* pWasHandled)
{
    HRESULT hr = S_OK;
    BOOLEAN wasFocused = FALSE;
    UINT currentSectionIndex = 0;
    BOOLEAN wasFound = FALSE;
    // cast to IHubSection to avoid ambiguous cast
    IFC(m_tpSections.Cast<HubSectionCollection>()->IndexOf(static_cast<IHubSection*>(pSection), &currentSectionIndex, &wasFound));
    if (wasFound)
    {
        UINT size = 0;

        IFC(m_tpSections.Cast<HubSectionCollection>()->get_Size(&size));

        // We found the current section in m_tpSections.  Now try to focus the header of the next section.
        if (forwardDirection)
        {
            for (INT i = currentSectionIndex + 1; i < static_cast<INT>(size); ++i)
            {
                ctl::ComPtr<IHubSection> spNextSection;

                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(i, &spNextSection));
                IFC(spNextSection.Cast<HubSection>()->FocusHeaderButton(xaml::FocusState::FocusState_Keyboard, &wasFocused));
                if (wasFocused)
                {
                    break;
                }
            }
        }
        else
        {
            for (INT i = currentSectionIndex - 1; i >= 0; --i)
            {
                ctl::ComPtr<IHubSection> spNextSection;

                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(i, &spNextSection));
                IFC(spNextSection.Cast<HubSection>()->FocusHeaderButton(xaml::FocusState::FocusState_Keyboard, &wasFocused));
                if (wasFocused)
                {
                    break;
                }
            }
        }
    }

    *pWasHandled = wasFocused;

Cleanup:
    RRETURN(hr);
}


// Prepare the view for a zoom transition.
_Check_return_ HRESULT
    Hub::InitializeViewChangeImpl()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISemanticZoom> spSemanticZoomOwner;

    // block scrollbars from showing during sezo operation
    if (m_tpScrollViewer.Get())
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->BlockIndicatorsFromShowing());
    }

    // need to precalculate the focusstate our item is going to be in
    // since the sezo will lose the trigger state soon
    IFC(get_SemanticZoomOwner(&spSemanticZoomOwner));

    if (spSemanticZoomOwner)
    {
        ctl::ComPtr<SemanticZoom> spSemanticZoomOwnerConcrete = spSemanticZoomOwner.Cast<SemanticZoom>();
        m_semanticZoomCompletedFocusState = xaml::FocusState_Programmatic;

        if (spSemanticZoomOwnerConcrete->GetIsProcessingKeyboardInput() ||
            spSemanticZoomOwnerConcrete->GetIsProcessingPointerInput())
        {
            // will set focus to the destination element using either keyboard or pointer focus depending on what
            // triggered the statechange.
            m_semanticZoomCompletedFocusState =
                spSemanticZoomOwnerConcrete->GetIsProcessingKeyboardInput() ?
                xaml::FocusState_Keyboard :
            xaml::FocusState_Pointer;
        }
    }

Cleanup:
    RRETURN(hr);
}

// Cleanup the view after a zoom transition.
_Check_return_ HRESULT
    Hub::CompleteViewChangeImpl()
{
    HRESULT hr = S_OK;

    // unblock scrollbars from showing during sezo operation
    if (m_tpScrollViewer.Get())
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ResetBlockIndicatorsFromShowing());
    }

Cleanup:
    RRETURN(hr);
}

// Forces content to scroll until the coordinate space of the
// SemanticZoomItem is visible.
_Check_return_ HRESULT
    Hub::MakeVisibleImpl(
    _In_ xaml_controls::ISemanticZoomLocation* pItem)
{
    RRETURN(S_OK);
}

// When this Hub is the active view and we're changing to
// the other view, optionally provide the source and destination
// items.
_Check_return_ HRESULT
    Hub::StartViewChangeFromImpl(
    _In_ xaml_controls::ISemanticZoomLocation* pSource,
    _In_ xaml_controls::ISemanticZoomLocation* pDestination)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spSourceItem;
    ctl::ComPtr<IInspectable> spDestinationItem;

    IFC(pSource->get_Item(&spSourceItem));
    IFC(pDestination->get_Item(&spDestinationItem));
    if (!spSourceItem && !spDestinationItem)
    {
        wf::Point zoomPoint = { };
        UINT itemsCount = 0;
        BOOLEAN validZoomPoint = FALSE;

        IFC(m_tpSections.Cast<HubSectionCollection>()->get_Size(&itemsCount));

        IFC(static_cast<SemanticZoomLocation*>(pSource)->get_ZoomPoint(&zoomPoint));

        validZoomPoint = zoomPoint.X != 0 || zoomPoint.Y != 0;

        // only use zoompoint if there was a gesture that got us to a zoompoint
        // and there are actual items to work against.
        if (validZoomPoint && itemsCount > 0)
        {
            // The user has zoomed with DM or used ctrl-mousewheel and that is why we are doing this viewchange

            ctl::ComPtr<IItemLookupPanel> spItemLookupPanel;

            spItemLookupPanel = m_tpPanel.AsOrNull<IItemLookupPanel>();
            if (spItemLookupPanel)
            {
                ctl::ComPtr<IGeneralTransform> spTransformFromHubtoPanel;
                ctl::ComPtr<IHubSection> spSection;
                ctl::ComPtr<IInspectable> spHeader;
                wf::Point zoomPointToFirstPanel = zoomPoint;
                xaml_primitives::ElementInfo closestElementInfo = {-1, FALSE};

                IFC(TransformToVisual(m_tpPanel.Cast<Panel>(), &spTransformFromHubtoPanel));
                IFC(spTransformFromHubtoPanel->TransformPoint(zoomPoint, &zoomPointToFirstPanel));

                IFC(spItemLookupPanel->GetClosestElementInfo(zoomPointToFirstPanel, &closestElementInfo));
                ASSERT(closestElementInfo.m_childIndex < static_cast<INT>(itemsCount));

                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(closestElementInfo.m_childIndex, &spSection));
                IFC(spSection->get_Header(&spHeader));
                IFC(pDestination->put_Item(spHeader.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// When this Hub is the inactive view and we're changing to
// it, optionally provide the source and destination items.
_Check_return_ HRESULT
    Hub::StartViewChangeToImpl(
    _In_ xaml_controls::ISemanticZoomLocation* pSource,
    _In_ xaml_controls::ISemanticZoomLocation* pDestination)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spDestinationItem;

    m_destinationIndexForSemanticZoom = -1;

    // If the destination item had not yet been set, we'll set it ourselves
    // to the corresponding HubSection we wish to scroll into view.
    IFC(pDestination->get_Item(&spDestinationItem));
    if (!spDestinationItem)
    {
        ctl::ComPtr<IInspectable> spSourceItem;

        IFC(pSource->get_Item(&spSourceItem));
        if (spSourceItem)
        {
            ctl::ComPtr<wfc::IIterator<xaml_controls::HubSection*>> spIterator;
            BOOLEAN hasCurrent = FALSE;

            IFC(m_tpSections.Cast<HubSectionCollection>()->First(&spIterator));
            IFC(spIterator->get_HasCurrent(&hasCurrent));
            while (hasCurrent)
            {
                ctl::ComPtr<IHubSection> spSection;
                ctl::ComPtr<IInspectable> spHeader;
                bool areEqual = false;

                IFC(spIterator->get_Current(&spSection));
                IFC(spSection->get_Header(&spHeader));

                IFC(PropertyValue::AreEqual(spSourceItem.Get(), spHeader.Get(), &areEqual));
                if (areEqual)
                {
                    IFC(pDestination->put_Item(spSection.Get()));
                    break;
                }

                IFC(spIterator->MoveNext(&hasCurrent));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Complete the change to the other view when this Hub was
// the active view.
_Check_return_ HRESULT
    Hub::CompleteViewChangeFromImpl(
    _In_ xaml_controls::ISemanticZoomLocation* pSource,
    _In_ xaml_controls::ISemanticZoomLocation* pDestination)
{
    RRETURN(S_OK);
}

// Complete the change to make this Hub the active view.
_Check_return_ HRESULT
    Hub::CompleteViewChangeToImpl(
    _In_ xaml_controls::ISemanticZoomLocation* pSource,
    _In_ xaml_controls::ISemanticZoomLocation* pDestination)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ISemanticZoom> spSemanticZoomOwner;

    IFC(get_SemanticZoomOwner(&spSemanticZoomOwner));
    if (spSemanticZoomOwner)
    {
        BOOLEAN shouldTakeFocusFromSeZoOwner = TRUE;
        ctl::ComPtr<IInspectable> spDestinationItem;
        ctl::ComPtr<IHubSection> spSection;

        IFC(spSemanticZoomOwner.Cast<SemanticZoom>()->HasFocus(&shouldTakeFocusFromSeZoOwner));

        // two scenarios that the SeZo owner doesn't have focus:
        // 1. on Phone. the SeZo owner is popup and gets dismissed when code runs to here,
        // 2. SemanticZoom.ToogleActiveView() gets called from outside of the SeZo, e.g. from a button.
        if (shouldTakeFocusFromSeZoOwner)
        {
            // Make sure that the focus is cleared from the ZoomedOutView.
            // If focus stays in the ZoomedOutView, it can cause problems.  Navigation will route to
            // the ZoomedOutView, which in turn may route to the SeZo's ScrollViewer and affect the positioning
            // of the Hub (the ZoomedInView).  Clearing focus avoids this.

            CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
            focusManager->ClearFocus();
        }

        IFC(pDestination->get_Item(&spDestinationItem));
        if (spDestinationItem)
        {
            spSection = spDestinationItem.AsOrNull<IHubSection>();
        }

        // TODO: (desktop) decouple focus transfer from ScrollIntoView().
        // Since there is no destination item here, we should not scroll.
        // We should still transfer focus into the Hub to enable keyboard navigation in the Hub.
        // We may need to add private API to allow us to set focus inside the Hub without causing
        // its ScrollViewer to scroll (and without affecting its public "bring into view on focus change" behavior).
        if (shouldTakeFocusFromSeZoOwner && nullptr == spSection)
        {
            // There was no destination item (e.g. the target was a HubSection with a null header),
            // or the destination item was not a HubSection.
            // We need to move focus into the Hub, but we should keep the Hub scrolled to the beginning
            // (since we don't know which HubSection the semantic zoom should correspond to).

            UINT size = 0;

            IFC(m_tpSections.Cast<HubSectionCollection>()->get_Size(&size));
            if (size > 0)
            {
                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(0, &spSection));
            }
        }

        if (spSection)
        {
            if (shouldTakeFocusFromSeZoOwner)
            {
                IFC(TransferSemanticZoomFocus(spSection.Get()));
            }
            else
            {
                IFC(ScrollToSection(spSection.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Helper method transferring focus to a HubSection after a semantic zoom into the Hub.
_Check_return_ HRESULT
Hub::TransferSemanticZoomFocus(
    _In_ IHubSection* pCurrentSection)
{
    HRESULT hr = S_OK;
    UINT currentSectionIndex = 0;
    BOOLEAN found = FALSE;

    IFC(m_tpSections.Cast<HubSectionCollection>()->IndexOf(pCurrentSection, &currentSectionIndex, &found));

    ASSERT(found);
    if (found)
    {
        UINT size = 0;
        INT newDestinationIndex = currentSectionIndex;

        IFC(m_tpSections.Cast<HubSectionCollection>()->get_Size(&size));

        // If m_destinationIndexForSemanticZoom has not been specified yet, it means TransferSemanticZoomFocus()
        // is getting called for the first time after a semantic zoom operation (it will be called again until
        // focus is successfully transferred to a HubSection or we run out of HubSections to try).
        //
        // 1. Try to focus the HubSection that is the destination of the semantic zoom.
        // 2. Try to focus the first focusable HubSection beyond m_destinationIndexForSemanticZoom.
        // 3. Try to focus the first focusable HubSection before m_destinationIndexForSemanticZoom.
        // 4. Try to focus the Hub itself.
        if (-1 == m_destinationIndexForSemanticZoom)
        {
            m_destinationIndexForSemanticZoom = newDestinationIndex;
            IFC(static_cast<HubSection*>(pCurrentSection)->TakeFocusOnLoaded(m_semanticZoomCompletedFocusState, TRUE /* semanticZoomMode */));
        }
        else if (newDestinationIndex >= m_destinationIndexForSemanticZoom && newDestinationIndex + 1 < static_cast<INT>(size))
        {
            ctl::ComPtr<IHubSection> spNextSection;

            ++newDestinationIndex;

            IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(newDestinationIndex, &spNextSection));

            IFC(spNextSection.Cast<HubSection>()->TakeFocusOnLoaded(m_semanticZoomCompletedFocusState, TRUE /* semanticZoomMode */));
        }
        else
        {
            if (newDestinationIndex >= m_destinationIndexForSemanticZoom)
            {
                newDestinationIndex = m_destinationIndexForSemanticZoom - 1;
            }
            else
            {
                --newDestinationIndex;
            }

            if (newDestinationIndex >= 0)
            {
                ctl::ComPtr<IHubSection> spPreviousSection;

                IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(newDestinationIndex, &spPreviousSection));

                IFC(spPreviousSection.Cast<HubSection>()->TakeFocusOnLoaded(m_semanticZoomCompletedFocusState, TRUE /* semanticZoomMode */));
            }
            else
            {
                BOOLEAN wasFocused = FALSE;

                IFC(Focus(m_semanticZoomCompletedFocusState, &wasFocused));

                IFC(m_tpSemanticZoomScrollIntoViewTimer->Start());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Callback for m_tpSemanticZoomScrollIntoViewTimer's Tick event.
_Check_return_ HRESULT
Hub::OnSemanticZoomScrollIntoViewTimerTick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IHubSection> spSection;
    UINT itemsCount = 0;

    IFC(m_tpSemanticZoomScrollIntoViewTimer->Stop());

    IFC(m_tpSections.Cast<HubSectionCollection>()->get_Size(&itemsCount));
    if (m_destinationIndexForSemanticZoom > -1 && static_cast<INT>(itemsCount) > m_destinationIndexForSemanticZoom)
    {
        IFC(m_tpSections.Cast<HubSectionCollection>()->GetAt(m_destinationIndexForSemanticZoom, &spSection));
        IFC(ScrollToSection(spSection.Get()));
    }

    // We should have found a Section if we started the timer, but fail gracefully and Assert instead of a hard failure.
    ASSERT(spSection);

Cleanup:
    RRETURN(hr);
}

// Starts the timer that scrolls to the HubSection that is the destination of the semantic zoom.
_Check_return_ HRESULT
Hub::DelayScrollToSeZoDestination()
{
    RRETURN(m_tpSemanticZoomScrollIntoViewTimer->Start());
}

// Checks whether a HubSection comes before m_destinationIndexForSemanticZoom.
_Check_return_ HRESULT
Hub::SectionComesBeforeDestinationForSemanticZoom(
    _In_ xaml_controls::IHubSection* pCurrentSection,
    _Out_ BOOLEAN* pComesBeforeDestinationSection)
{
    HRESULT hr = S_OK;
    UINT index = 0;
    BOOLEAN found = FALSE;

    // m_destinationIndexForSemanticZoom should be valid whenever we're calling this method.
    ASSERT(m_destinationIndexForSemanticZoom >= 0);

    *pComesBeforeDestinationSection = FALSE;

    IFC(m_tpSections.Cast<HubSectionCollection>()->IndexOf(pCurrentSection, &index, &found));
    if (found && static_cast<INT>(index) < m_destinationIndexForSemanticZoom)
    {
        *pComesBeforeDestinationSection = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

// Callback for when the HubSection.Header property changes for one of the constituent HubSections.
_Check_return_ HRESULT
    Hub::OnSectionHeaderChanged(
    _In_ xaml_controls::IHubSection* pSection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spHeader;
    UINT index;
    BOOLEAN found = FALSE;

    IFC(m_tpSections.Cast<HubSectionCollection>()->IndexOf(pSection, &index, &found));

    if (found)
    {
        IFC(m_tpSectionHeaders->InternalRemoveAt(index));

        IFC(pSection->get_Header(&spHeader));
        IFC(m_tpSectionHeaders->InternalInsertAt(index, spHeader.Get()));
    }
    else
    {
        ASSERT(FALSE, L"Could not find the HubSection in its Parent Hub's Sections!");
    }

Cleanup:
    RRETURN(hr);
}
