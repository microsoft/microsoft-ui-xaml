// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FlipView.g.h"
#include "ScrollViewer.g.h"
#include "ButtonBase.g.h"
#include "FlipViewItem.g.h"
#include "ItemCollection.g.h"
#include "VirtualizingStackPanel.g.h"
#include "ItemsPresenter.g.h"
#include "FlipViewAutomationPeer.g.h"
#include "Window.g.h"
#include "DispatcherTimer.g.h"
#include "ToolTipService.g.h"
#include "RoutedEventArgs.h"
#include "InputPointEventArgs.h"
#include "InputServices.h"
#include "AutomationProperties.h"
#include "localizedResource.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

int DirectUI::FlipView::s_scrollWheelDelayMS = FLIP_VIEW_DISTINCT_SCROLL_WHEEL_DELAY_MS;

// Initializes a new instance of the FlipView class.
FlipView::FlipView()
    : m_showNavigationButtons(FALSE)
    , m_ShouldShowFocusRect(FALSE)
    , m_animateNewIndex(FALSE)
    , m_verticalSnapPointsType(xaml_controls::SnapPointsType::SnapPointsType_None)
    , m_horizontalSnapPointsType(xaml_controls::SnapPointsType::SnapPointsType_None)
    , m_skipAnimationOnce(FALSE)
    , m_lastScrollWheelDelta(0)
    , m_lastScrollWheelTime(0)
    , m_keepNavigationButtonsVisible(false)
    , m_itemsAreSized(false)
{
    // Zero member structs
    ZeroMemory(&m_ScrollingHostPartSizeChangedEventToken, sizeof(m_ScrollingHostPartSizeChangedEventToken));
    ZeroMemory(&m_PreviousButtonHorizontalPartClickEventToken, sizeof(m_PreviousButtonHorizontalPartClickEventToken));
    ZeroMemory(&m_NextButtonHorizontalPartClickEventToken, sizeof(m_NextButtonHorizontalPartClickEventToken));
    ZeroMemory(&m_PreviousButtonVerticalPartClickEventToken, sizeof(m_PreviousButtonVerticalPartClickEventToken));
    ZeroMemory(&m_NextButtonVerticalPartClickEventToken, sizeof(m_NextButtonVerticalPartClickEventToken));
}

// Destroys an instance of the FlipView class.
FlipView::~FlipView()
{
    HRESULT hr = S_OK;

    // Release and unhook from template parts/events
    IFC(UnhookTemplate());

    // Makes sure the timers are stopped.
    {
        auto spButtonsFadeOutTimer = m_tpButtonsFadeOutTimer.GetSafeReference();
        if (spButtonsFadeOutTimer)
        {
            IGNOREHR(spButtonsFadeOutTimer->Stop());
        }

        auto spFixOffsetTimer = m_tpFixOffsetTimer.GetSafeReference();
        if (spFixOffsetTimer)
        {
            IGNOREHR(spFixOffsetTimer->Stop());
        }
    }

Cleanup:
    VERIFYHR(hr);
}

// Saves the Vertical/HorizontalSnapPointsTypes and clears them. This is necessary for us to
// be able to chain animate FlipViewItems.
_Check_return_ HRESULT
    FlipView::SaveAndClearSnapPointsTypes()
{
    HRESULT hr = S_OK;

    // Saved snap points will be restored asynchronously, so if we are still in the animation we do not want to
    // save the current values since they are barely the default values.
    if (m_tpScrollViewer.Get() != nullptr && !m_animateNewIndex)
    {
        IFC(m_tpScrollViewer->get_VerticalSnapPointsType(&m_verticalSnapPointsType));
        IFC(m_tpScrollViewer->get_HorizontalSnapPointsType(&m_horizontalSnapPointsType));

        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ScrollViewer_VerticalSnapPointsType)));
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->ClearValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ScrollViewer_HorizontalSnapPointsType)));
    }

Cleanup:
    RRETURN(hr);
}

// Restore the saved Vertical/HorizontalSnapPointsTypes
_Check_return_ HRESULT
    FlipView::RestoreSnapPointsTypes()
{
    HRESULT hr = S_OK;

    if (m_tpScrollViewer.Get() != nullptr)
    {
        IFC(m_tpScrollViewer->put_VerticalSnapPointsType(m_verticalSnapPointsType));
        IFC(m_tpScrollViewer->put_HorizontalSnapPointsType(m_horizontalSnapPointsType));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
FlipView::MoveNext(_Out_opt_ bool* pSuccessfullyMoved)
{
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableItems;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
    INT nSelectedIndex = 0;
    XUINT32 nCount = 0;
    bool successfullyMoved = false;

    // Select the next index
    IFC_RETURN(get_Items(&spObservableItems));
    IFC_RETURN(spObservableItems.As(&spItems));
    IFC_RETURN(spItems->get_Size(&nCount));
    IFC_RETURN(get_SelectedIndex(&nSelectedIndex));

    if (nSelectedIndex < static_cast<INT>(nCount - 1))
    {
        ++nSelectedIndex;
        successfullyMoved = true;
    }
    else
    {
        nSelectedIndex = nCount - 1;
    }

    IFC_RETURN(UpdateSelectedIndex(nSelectedIndex));

    if (successfullyMoved)
    {
        IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_MoveNext, this));
    }

    if (pSuccessfullyMoved)
    {
        *pSuccessfullyMoved = successfullyMoved;
    }

    return S_OK;
}

_Check_return_ HRESULT
FlipView::MovePrevious(_Out_opt_ bool* pSuccessfullyMoved)
{
    INT nSelectedIndex = 0;
    bool successfullyMoved = false;

    // Select the previous index
    IFC_RETURN(get_SelectedIndex(&nSelectedIndex));

    if (nSelectedIndex > 0)
    {
        --nSelectedIndex;
        successfullyMoved = true;
    }

    IFC_RETURN(UpdateSelectedIndex(nSelectedIndex));

    if (successfullyMoved)
    {
        // Play MovePrevious sound:
        IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_MovePrevious, this));
    }

    if (pSuccessfullyMoved)
    {
        *pSuccessfullyMoved = successfullyMoved;
    }

    return S_OK;
}


// Calculates the rectangle to be brought into view from the index.
_Check_return_ HRESULT
    FlipView::CalculateBounds(
    _In_ INT index,
    _Out_ XRECTF* bounds)
{
    HRESULT hr = S_OK;
    XRECTF emptyRect = {};
    BOOLEAN isVertical = FALSE;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    DOUBLE width = 0;
    DOUBLE height = 0;

    *bounds = emptyRect;

    IFC(GetDesiredItemHeight(&height));
    bounds->Height = static_cast<XFLOAT>(height);
    IFC(GetDesiredItemWidth(&width));
    bounds->Width = static_cast<XFLOAT>(width);

    IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
    isVertical = physicalOrientation == xaml_controls::Orientation_Vertical;

    if (isVertical)
    {
        bounds->Y = bounds->Height * index;
        ASSERT(bounds->X == 0);
    }
    else
    {
        bounds->X = bounds->Width * index;
        ASSERT(bounds->Y == 0);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::UpdateSelectedIndex(UINT index)
{
    HRESULT hr = S_OK;

    //The following statement will trigger OnSelectionChanged
    IFC(put_SelectedIndex(index));

    //Focus state needs to be keyboard when using navigation keys
    IFC(SetFocusedItem(index, FALSE, TRUE, xaml::FocusState_Pointer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::UnhookTemplate()
{
    HRESULT hr = S_OK;

    if (auto peg = m_tpScrollViewer.TryMakeAutoPeg())
    {
        VERIFYHR(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(NULL));
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->remove_SizeChanged(m_ScrollingHostPartSizeChangedEventToken));
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_ArePointerWheelEventsIgnored(FALSE));
        ZeroMemory(&m_ScrollingHostPartSizeChangedEventToken, sizeof(m_ScrollingHostPartSizeChangedEventToken));

        IFC(m_epScrollViewerViewChangedHandler.DetachEventHandler(m_tpScrollViewer.Cast<IInspectable>()));
    }

    if (auto peg = m_tpPreviousButtonHorizontalPart.TryMakeAutoPeg())
    {
        IFC(m_tpPreviousButtonHorizontalPart.Cast<ButtonBase>()->remove_Click(m_PreviousButtonHorizontalPartClickEventToken));
        ZeroMemory(&m_PreviousButtonHorizontalPartClickEventToken, sizeof(m_PreviousButtonHorizontalPartClickEventToken));

        IFC(m_epPreviousButtonHorizontalPartPointerEnteredHandler.DetachEventHandler(m_tpPreviousButtonHorizontalPart.Cast<IInspectable>()));
        IFC(m_epPreviousButtonHorizontalPartPointerExitedHandler.DetachEventHandler(m_tpPreviousButtonHorizontalPart.Cast<IInspectable>()));
    }

    if (auto peg = m_tpNextButtonHorizontalPart.TryMakeAutoPeg())
    {
        IFC(m_tpNextButtonHorizontalPart.Cast<ButtonBase>()->remove_Click(m_NextButtonHorizontalPartClickEventToken));
        ZeroMemory(&m_NextButtonHorizontalPartClickEventToken, sizeof(m_NextButtonHorizontalPartClickEventToken));

        IFC(m_epNextButtonHorizontalPartPointerEnteredHandler.DetachEventHandler(m_tpNextButtonHorizontalPart.Cast<IInspectable>()));
        IFC(m_epNextButtonHorizontalPartPointerExitedHandler.DetachEventHandler(m_tpNextButtonHorizontalPart.Cast<IInspectable>()));
    }

    if (auto peg = m_tpPreviousButtonVerticalPart.TryMakeAutoPeg())
    {
        IFC(m_tpPreviousButtonVerticalPart.Cast<ButtonBase>()->remove_Click(m_PreviousButtonVerticalPartClickEventToken));
        ZeroMemory(&m_PreviousButtonVerticalPartClickEventToken, sizeof(m_PreviousButtonVerticalPartClickEventToken));

        IFC(m_epPreviousButtonVerticalPartPointerEnteredHandler.DetachEventHandler(m_tpPreviousButtonVerticalPart.Cast<IInspectable>()));
        IFC(m_epPreviousButtonVerticalPartPointerExitedHandler.DetachEventHandler(m_tpPreviousButtonVerticalPart.Cast<IInspectable>()));
    }

    if (auto peg = m_tpNextButtonVerticalPart.TryMakeAutoPeg())
    {
        IFC(m_tpNextButtonVerticalPart.Cast<ButtonBase>()->remove_Click(m_NextButtonVerticalPartClickEventToken));
        ZeroMemory(&m_NextButtonVerticalPartClickEventToken, sizeof(m_NextButtonVerticalPartClickEventToken));

        IFC(m_epNextButtonVerticalPartPointerEnteredHandler.DetachEventHandler(m_tpNextButtonVerticalPart.Cast<IInspectable>()));
        IFC(m_epNextButtonVerticalPartPointerExitedHandler.DetachEventHandler(m_tpNextButtonVerticalPart.Cast<IInspectable>()));
    }

Cleanup:
    m_tpScrollViewer.Clear();
    m_tpPreviousButtonHorizontalPart.Clear();
    m_tpNextButtonHorizontalPart.Clear();
    m_tpPreviousButtonVerticalPart.Clear();
    m_tpNextButtonVerticalPart.Clear();
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::CreateButtonClickEventHandler(
    _In_reads_(pcButton + 1) WCHAR* psButton,
    _In_ size_t pcButton,
    _In_ HRESULT (DirectUI::FlipView::*pfnEventHandler)(IInspectable*, xaml::IRoutedEventArgs*),
    _Outptr_ xaml_primitives::IButtonBase **ppButton,
    _Out_ EventRegistrationToken *pEventToken)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<xaml_primitives::IButtonBase> spButtonBasePart;

    *ppButton = nullptr;

    IFC(GetTemplatePart<xaml_primitives::IButtonBase>(psButton, pcButton, spButtonBasePart.ReleaseAndGetAddressOf()));

    if (spButtonBasePart)
    {
        ctl::ComPtr<xaml::IRoutedEventHandler> spEventHandler;

        spEventHandler.Attach(new ClassMemberEventHandler<
            FlipView,
            xaml_controls::IFlipView,
            xaml::IRoutedEventHandler,
            IInspectable,
            xaml::IRoutedEventArgs>(this, pfnEventHandler));

        IFC(spButtonBasePart->add_Click(spEventHandler.Get(), pEventToken));

        IFC(spButtonBasePart.MoveTo(ppButton));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::CreateButtonPointerEnteredEventHandler(
    _In_ xaml_primitives::IButtonBase *pButton,
    _In_ ctl::EventPtr<UIElementPointerEnteredEventCallback>& eventPtr)
{
    HRESULT hr = S_OK;

    if (pButton)
    {
        IFC(eventPtr.AttachEventHandler(static_cast<ButtonBase*>(pButton),
            [this](IInspectable* pSender, IPointerRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnPointerEnteredNavigationButtons(pSender, pArgs));

            return S_OK;
        }));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
    FlipView::CreateButtonPointerExitedEventHandler(
    _In_ xaml_primitives::IButtonBase *pButton,
    _In_ ctl::EventPtr<UIElementPointerExitedEventCallback>& eventPtr)
{
    HRESULT hr = S_OK;

    if (pButton)
    {
        IFC(eventPtr.AttachEventHandler(static_cast<ButtonBase*>(pButton),
            [this](IInspectable* pSender, IPointerRoutedEventArgs* pArgs)
        {
            IFC_RETURN(OnPointerExitedNavigationButtons(pSender, pArgs));

            return S_OK;
        }));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
    FlipView::InitializeScrollViewer()
{
    HRESULT hr = S_OK;

    if (!m_tpScrollViewer)
    {
        ctl::ComPtr<xaml_controls::IScrollViewer> spScrollViewer;

        IFC(GetTemplatePart<xaml_controls::IScrollViewer>(STR_LEN_PAIR(L"ScrollingHost"), spScrollViewer.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpScrollViewer, spScrollViewer);

        if (m_tpScrollViewer)
        {
            ctl::ComPtr<xaml::ISizeChangedEventHandler> spSizeChangedHandler;
            xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;

            // Ignore mouse wheel scroll events to route them to the parent
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_ArePointerWheelEventsIgnored(TRUE));

            // Set ScrollViewer's properties based o ItemsHost orientation
            IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));

            if (physicalOrientation == xaml_controls::Orientation_Vertical)
            {
                // Disable horizontal scrolling and enable vertical scrolling
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_HorizontalScrollMode(xaml_controls::ScrollMode_Disabled));
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_VerticalScrollMode(xaml_controls::ScrollMode_Enabled));
            }
            else
            {
                // Disable vertical scrolling and enable horizontal scrolling
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_VerticalScrollMode(xaml_controls::ScrollMode_Disabled));
                IFC(m_tpScrollViewer.Cast<ScrollViewer>()->put_HorizontalScrollMode(xaml_controls::ScrollMode_Enabled));
            }

            //Create SizeChange handler
            spSizeChangedHandler.Attach(new ClassMemberEventHandler<
                FlipView,
                xaml_controls::IFlipView,
                xaml::ISizeChangedEventHandler,
                IInspectable,
                xaml::ISizeChangedEventArgs>(this, &FlipView::OnScrollingHostPartSizeChanged));

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->add_SizeChanged(spSizeChangedHandler.Get(), &m_ScrollingHostPartSizeChangedEventToken));

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->SetDirectManipulationStateChangeHandler(static_cast<DirectManipulationStateChangeHandler*>(this)));

            IFC(m_epScrollViewerViewChangedHandler.AttachEventHandler(m_tpScrollViewer.Cast<ScrollViewer>(),
                [this](IInspectable* pSender, IScrollViewerViewChangedEventArgs* pArgs)
            {
                return OnScrollViewerViewChanged(pSender, pArgs);
            }));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlipView::OnScrollViewerViewChanged(_In_ IInspectable* pSender, _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pArgs)
{
    BOOLEAN isIntermediate = TRUE;
    IFC_RETURN(pArgs->get_IsIntermediate(&isIntermediate));

    if (!isIntermediate && m_itemsAreSized && !m_inMeasure && !m_inArrange)
    {
        INT nSelectedIndex = 0;
        DOUBLE offset = 0;
        DOUBLE viewportSize = 0;
        XUINT32 nCount = 0;

        ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableItems;

        IFC_RETURN(get_Items(&spObservableItems));
        IFC_RETURN(spObservableItems.As(&spItems));
        IFC_RETURN(spItems->get_Size(&nCount));

        if (nCount > 0)
        {
            // FlipView contains at least 1 FlipViewItem
            // Update SelectedIndex

            xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;

            IFC_RETURN(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));

            if (physicalOrientation == xaml_controls::Orientation_Vertical)
            {
                IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->get_VerticalOffset(&offset));
                IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->get_ViewportHeight(&viewportSize));
            }
            else
            {
                IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->get_HorizontalOffset(&offset));
                IFC_RETURN(m_tpScrollViewer.Cast<ScrollViewer>()->get_ViewportWidth(&viewportSize));
            }

            if (viewportSize > 0)
            {
                BOOLEAN bIsVirtualizing = FALSE;
                IFC_RETURN(VirtualizingStackPanelFactory::GetIsVirtualizingStatic(this, &bIsVirtualizing));

                if (!m_animateNewIndex)
                {
                    boolean hasPendingSelectionChange = false;

                    // When the m_tpFixOffsetTimer was started and thus there is a pending view change to sync
                    // up with the current SelectedIndex, do not change that SelectedIndex. This situation occurs
                    // when the SelectedIndex is changed during a view animation caused by a prior SelectedIndex change.
                    if (m_tpFixOffsetTimer)
                    {
                        IFC_RETURN(m_tpFixOffsetTimer->get_IsEnabled(&hasPendingSelectionChange));
                    }
                    if (!hasPendingSelectionChange)
                    {
                        // Round to the nearest FlipViewItem
                        m_skipAnimationOnce = TRUE;
                        nSelectedIndex = static_cast<INT>(DoubleUtil::Round(bIsVirtualizing ? ItemsPresenter::OffsetToIndex(offset) : (offset / viewportSize), 0));
                        IFC_RETURN(put_SelectedIndex(nSelectedIndex));
                        m_skipAnimationOnce = FALSE;
                    }
                }
                else
                {
                    // No need to update the selected index since it is already at the correct value.
                    m_animateNewIndex = FALSE;
                    IFC_RETURN(RestoreSnapPointsTypes());
                }

                if (m_moveFocusToSelectedItem)
                {
                    INT selectedIndex = 0;
                    IFC_RETURN(get_SelectedIndex(&selectedIndex));
                    if (selectedIndex >= 0)
                    {
                        ctl::ComPtr<IDependencyObject> spContainer;
                        IFC_RETURN(ContainerFromIndex(selectedIndex, &spContainer));
                        if (spContainer)
                        {
                            // We previously received focus on a non-selected item at a time when the container for the selected item
                            // was not realized. We move focus to the selected item now that it is available.
                            // We do this in the ScrollViewerChanged event since there is no other callback that we use to determine when a
                            // particular container has been realized by the virtualizing panel.
                            m_moveFocusToSelectedItem = false;
                            IFC_RETURN(SetFocusedItem(selectedIndex, FALSE /*shouldScrollIntoView*/, FALSE /*forceFocus*/, xaml::FocusState_Programmatic));
                        }
                    }
                }
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP
    FlipView::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strAutomationName;
    ctl::ComPtr<IButtonBase> spPreviousButtonHorizontalPart;
    ctl::ComPtr<IButtonBase> spNextButtonHorizontalPart;
    ctl::ComPtr<IButtonBase> spPreviousButtonVerticalPart;
    ctl::ComPtr<IButtonBase> spNextButtonVerticalPart;

    // Unhook from old Template
    IFC(UnhookTemplate());

    // Call base class implementation
    IFC(FlipViewGenerated::OnApplyTemplate());

    //Set event handlers for the 4 FlipView buttons
    IFC(CreateButtonClickEventHandler(STR_LEN_PAIR(L"PreviousButtonHorizontal"),
        &FlipView::OnPreviousButtonPartClick,
        &spPreviousButtonHorizontalPart,
        &m_PreviousButtonHorizontalPartClickEventToken));
    SetPtrValue(m_tpPreviousButtonHorizontalPart, spPreviousButtonHorizontalPart);
    if(m_tpPreviousButtonHorizontalPart)
    {
        IFC(CreateButtonPointerEnteredEventHandler(m_tpPreviousButtonHorizontalPart.Get(), m_epPreviousButtonHorizontalPartPointerEnteredHandler));

        IFC(CreateButtonPointerExitedEventHandler(m_tpPreviousButtonHorizontalPart.Get(), m_epPreviousButtonHorizontalPartPointerExitedHandler));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpPreviousButtonHorizontalPart.Cast<ButtonBase>(), strAutomationName.ReleaseAndGetAddressOf()))
            if(strAutomationName.Get() == NULL)
            {
                IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_FLIPVIEW_PREVIOUS, strAutomationName.ReleaseAndGetAddressOf()));
                IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpPreviousButtonHorizontalPart.Cast<ButtonBase>(), strAutomationName.Get()));
            }
    }

    IFC(CreateButtonClickEventHandler(STR_LEN_PAIR(L"NextButtonHorizontal"),
        &FlipView::OnNextButtonPartClick,
        &spNextButtonHorizontalPart,
        &m_NextButtonHorizontalPartClickEventToken));
    SetPtrValue(m_tpNextButtonHorizontalPart, spNextButtonHorizontalPart);
    if(m_tpNextButtonHorizontalPart)
    {
        IFC(CreateButtonPointerEnteredEventHandler(m_tpNextButtonHorizontalPart.Get(), m_epNextButtonHorizontalPartPointerEnteredHandler));

        IFC(CreateButtonPointerExitedEventHandler(m_tpNextButtonHorizontalPart.Get(), m_epNextButtonHorizontalPartPointerExitedHandler));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpNextButtonHorizontalPart.Cast<ButtonBase>(), strAutomationName.ReleaseAndGetAddressOf()))
            if(strAutomationName.Get() == NULL)
            {
                IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_FLIPVIEW_NEXT, strAutomationName.ReleaseAndGetAddressOf()));
                IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpNextButtonHorizontalPart.Cast<ButtonBase>(), strAutomationName.Get()))
            }
    }

    IFC(CreateButtonClickEventHandler(STR_LEN_PAIR(L"PreviousButtonVertical"),
        &FlipView::OnPreviousButtonPartClick,
        &spPreviousButtonVerticalPart,
        &m_PreviousButtonVerticalPartClickEventToken));
    SetPtrValue(m_tpPreviousButtonVerticalPart, spPreviousButtonVerticalPart);
    if(m_tpPreviousButtonVerticalPart)
    {
        IFC(CreateButtonPointerEnteredEventHandler(m_tpPreviousButtonVerticalPart.Get(), m_epPreviousButtonVerticalPartPointerEnteredHandler));

        IFC(CreateButtonPointerExitedEventHandler(m_tpPreviousButtonVerticalPart.Get(), m_epPreviousButtonVerticalPartPointerExitedHandler));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpPreviousButtonVerticalPart.Cast<ButtonBase>(), strAutomationName.ReleaseAndGetAddressOf()))
            if(strAutomationName.Get() == NULL)
            {
                IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_FLIPVIEW_PREVIOUS, strAutomationName.ReleaseAndGetAddressOf()));
                IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpPreviousButtonVerticalPart.Cast<ButtonBase>(), strAutomationName.Get()))
            }
    }

    IFC(CreateButtonClickEventHandler(STR_LEN_PAIR(L"NextButtonVertical"),
        &FlipView::OnNextButtonPartClick,
        &spNextButtonVerticalPart,
        &m_NextButtonVerticalPartClickEventToken));
    SetPtrValue(m_tpNextButtonVerticalPart, spNextButtonVerticalPart);
    if(m_tpNextButtonVerticalPart)
    {
        IFC(CreateButtonPointerEnteredEventHandler(m_tpNextButtonVerticalPart.Get(), m_epNextButtonVerticalPartPointerEnteredHandler));

        IFC(CreateButtonPointerExitedEventHandler(m_tpNextButtonVerticalPart.Get(), m_epNextButtonVerticalPartPointerExitedHandler));

        IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpNextButtonVerticalPart.Cast<ButtonBase>(), strAutomationName.ReleaseAndGetAddressOf()))
            if(strAutomationName.Get() == NULL)
            {
                IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_FLIPVIEW_NEXT, strAutomationName.ReleaseAndGetAddressOf()));
                IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpNextButtonVerticalPart.Cast<ButtonBase>(), strAutomationName.Get()))
            }
    }

    // Sync the logical and visual states of the control

    IFC(UpdateVisualState(FALSE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::OnItemsHostAvailable()
{
    HRESULT hr = S_OK;

    IFC(InitializeScrollViewer());

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
    FlipView::OnPointerWheelChanged(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN handled = FALSE;

    IFC(FlipViewGenerated::OnPointerWheelChanged(pArgs));

    IFC(pArgs->get_Handled(&handled));
    if (!handled)
    {
        wsy::VirtualKeyModifiers keyModifiers = wsy::VirtualKeyModifiers_None;
        BOOLEAN isCtrlPressed = FALSE;

        IFC(GetKeyboardModifiers(&keyModifiers));
        isCtrlPressed = IsFlagSet(keyModifiers, wsy::VirtualKeyModifiers_Control);

        if (!isCtrlPressed)
        {
            LARGE_INTEGER lTimeCurrent = {};
            BOOLEAN canFlip = FALSE;
            BOOL queryCounterSuccess = FALSE;

            queryCounterSuccess = QueryPerformanceCounter(&lTimeCurrent);

            if (queryCounterSuccess)
            {
                ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
                ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
                INT mouseWheelDelta = 0;

                IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
                IFCPTR(spPointerPoint);
                IFC(spPointerPoint->get_Properties(&spPointerProperties));
                IFCPTR(spPointerProperties);
                IFC(spPointerProperties->get_MouseWheelDelta(&mouseWheelDelta));

                if ((mouseWheelDelta < 0 && m_lastScrollWheelDelta >= 0) ||
                    (mouseWheelDelta > 0 && m_lastScrollWheelDelta <= 0))
                {
                    // Direction change so we can flip.
                    canFlip = TRUE;
                }
                else
                {
                    LARGE_INTEGER frequency;
                    BOOL queryFrequencySuccess;

                    queryFrequencySuccess = QueryPerformanceFrequency(&frequency);

                    if (queryFrequencySuccess && ((lTimeCurrent.QuadPart - m_lastScrollWheelTime) / static_cast<DOUBLE>(frequency.QuadPart) * 1000) > s_scrollWheelDelayMS)
                    {
                        // Enough time has passed so we can flip.
                        canFlip = TRUE;
                    }
                }

                // Whether a flip is performed or not, the time of this mouse wheel delta is being recorded. This is to avoid a single touch pad
                // flick to trigger multiple flips in a row, every 200ms. Touch pad flicks are indeed translated into pointer wheel deltas and spread
                // over multiple seconds, which is much larger than s_scrollWheelDelayMS==200ms. So a pause of 200ms since the last
                // wheel delta, or a change in direction, is required to trigger a new flip. Unfortunately that may require the user to wait a few seconds
                // before being able to trigger a new flip with the touch pad.
                m_lastScrollWheelTime = lTimeCurrent.QuadPart;

                if (canFlip)
                {
                    bool successfullyMoved = false;

                    if (mouseWheelDelta < 0)
                    {
                        IFC(MoveNext(&successfullyMoved));
                    }
                    else
                    {
                        IFC(MovePrevious(&successfullyMoved));
                    }

                    if (successfullyMoved)
                    {
                        // An actual flip occurred.
                        m_lastScrollWheelDelta = mouseWheelDelta;

                        // Since we flipped we ignore mouse wheel in current and parent scrollers.
                        IFC(pArgs->put_Handled(TRUE));
                    }
                    // Do not set handled in the else so we can scroll chain since we were already at the first/last item.
                }
            }

            if (!canFlip)
            {
                // Ignore mouse wheel in current and parent scrollers.
                IFC(pArgs->put_Handled(TRUE));
            }
        }
    }
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
    FlipView::OnPointerEntered(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFC(FlipViewGenerated::OnPointerEntered(pArgs));

    IFCPTR(pArgs);

    IFC(pArgs->get_Pointer(&spPointer));
    IFCPTR(spPointer);
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));

    if (pointerDeviceType == mui::PointerDeviceType_Touch)
    {
        IFC(HideButtonsImmediately());
    }
    else
    {
        IFC(ResetButtonsFadeOutTimer());
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
    FlipView::OnPointerMoved(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    IFC(FlipViewGenerated::OnPointerMoved(pArgs));

    IFCPTR(pArgs);

    IFC(pArgs->get_Pointer(&spPointer));
    IFCPTR(spPointer);
    IFC(spPointer->get_PointerDeviceType(&pointerDeviceType));

    if (mui::PointerDeviceType_Touch != pointerDeviceType)
    {
        IFC(ResetButtonsFadeOutTimer());
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
    FlipView::IsItemItsOwnContainerOverride(
    _In_ IInspectable* item,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    // Require containers be of type IFlipViewItem
    IFCEXPECT(returnValue);
    *returnValue = !!ctl::value_is<xaml_controls::IFlipViewItem>(item);

Cleanup:
    RRETURN(hr);
}

// Creates or identifies the element that is used to display the given item.
IFACEMETHODIMP
    FlipView::GetContainerForItemOverride(
    _Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<FlipViewItem> spFlipViewItem;

    IFCEXPECT(returnValue);
    IFC(ctl::make<FlipViewItem>(&spFlipViewItem));

    IFC(spFlipViewItem.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::OnItemsChanged(
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    INT currentSelectedIndex = 0;
    INT previousSelectedIndex = 0;
    BOOLEAN savedSkipAnimationOnce = m_skipAnimationOnce;

    IFC(get_SelectedIndex(&previousSelectedIndex));

    IFC(FlipViewGenerated::OnItemsChanged(e));

    IFC(get_SelectedIndex(&currentSelectedIndex));

    if (previousSelectedIndex < 0 ||
        previousSelectedIndex != currentSelectedIndex)
    {
        ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableItems;
        XUINT32 nCount = 0;
        INT newSelectedIndex = -1;

        IFC(get_Items(&spObservableItems));
        IFC(spObservableItems.As(&spItems));
        IFC(spItems->get_Size(&nCount));

        if (nCount > 0)
        {
            if (previousSelectedIndex < 0 &&
                currentSelectedIndex < 0)
            {
                // Initial case is special to be in parity with old behavior
                // Default to selecting the first element.
                newSelectedIndex = 0;
            }
            else if (previousSelectedIndex < 0 ||
                (currentSelectedIndex >= 0 && currentSelectedIndex < previousSelectedIndex))
            {
                // If no item was selected previously, or an item got removed before the previously selected index
                // ensure selected index and offset is correctly set to new selected index.
                newSelectedIndex = currentSelectedIndex;
            }
            else
            {
                newSelectedIndex = previousSelectedIndex;
            }

            // Upper bound check to select new last element after last one was removed.
            if (newSelectedIndex >= static_cast<INT>(nCount))
            {
                newSelectedIndex = nCount - 1;
            }
        }

        m_skipAnimationOnce = TRUE;
        IFC(put_SelectedIndex(newSelectedIndex));
    }

    // Set the correct offset to selected item so that during Arrange items are laid out correctly.
    if (!m_animateNewIndex)
    {
        IFC(SetOffsetToSelectedIndex());
    }

Cleanup:
    m_skipAnimationOnce = savedSkipAnimationOnce;
    RRETURN(hr);
}

_Check_return_ HRESULT FlipView::OnItemsSourceChanged(
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    INT nSelectedIndex = 0;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spItems;
    UINT nCount = 0;

    IFC(FlipViewGenerated::OnItemsSourceChanged(pNewValue));

    IFC(get_Items(&spItems));
    IFC(spItems.Cast<ItemCollection>()->get_Size(&nCount));

    if (pNewValue && nCount > 0)
    {
        IFC(get_SelectedIndex(&nSelectedIndex));
        if (nSelectedIndex < 0)
        {
            // Reset selected index to 0 if nothing is selected.
            IFC(put_SelectedIndex(0));
        }

        IFC(SetOffsetToSelectedIndex());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    FlipView::NotifyOfSourceChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e) noexcept
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action = wfc::CollectionChange_Reset;
    INT nSelectedIndex = 0;
    INT nPreviousSelectedIndex = 0;

    // If we animate due to change in items, we may end up showing a wrong index due to entering and leaving items.
    // So we skip animations for index changes resulting from source changes.
    m_skipAnimationOnce = TRUE;

    IFC(get_SelectedIndex(&nPreviousSelectedIndex));
    if (nPreviousSelectedIndex < 0)
    {
        nPreviousSelectedIndex = 0;
    }

    IFC(FlipViewGenerated::NotifyOfSourceChanged(pSender, e));
    IFC(e->get_CollectionChange(&action));
    if (action == wfc::CollectionChange_ItemChanged)
    {
        IFC(get_SelectedIndex(&nSelectedIndex));
        if (nSelectedIndex < 0)
        {
            // Reset selected index to 0 if nothing is selected.
            IFC(put_SelectedIndex(nPreviousSelectedIndex));
        }

        IFC(SetOffsetToSelectedIndex());
    }

Cleanup:
    m_skipAnimationOnce = FALSE;
    RRETURN(hr);
}

// Sets vertical/horizontal offset corresponding to the selected item.
// If the panel inside is a virtualizing panel selected index is used since virtualizing panels use item based scrolling
// otherwise pixel offset is set calculated based on flipview's size since all items are of the same size.
_Check_return_ HRESULT FlipView::SetOffsetToSelectedIndex()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IScrollInfo> spScrollInfo;
    INT selectedIndex = 0;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    BOOLEAN bIsVertical = FALSE;
    BOOLEAN bIsVirtualizing = FALSE;
    DOUBLE offset = 0.0;

    IFC(get_SelectedIndex(&selectedIndex));

    if (selectedIndex < 0)
    {
        goto Cleanup;
    }

    IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
    bIsVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);
    IFC(VirtualizingStackPanelFactory::GetIsVirtualizingStatic(this, &bIsVirtualizing));

    if (bIsVirtualizing)
    {
        // Item based scrolling
        offset = ItemsPresenter::IndexToOffset(selectedIndex);
    }
    else if (bIsVertical) // pixel based scrolling
    {
        IFC(GetDesiredItemHeight(&offset));
        offset *= selectedIndex;
    }
    else
    {
        IFC(GetDesiredItemWidth(&offset));
        offset *= selectedIndex;
    }

    if (m_tpScrollViewer)
    {
        IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ScrollInfo(&spScrollInfo));
        if (spScrollInfo)
        {
            if (bIsVertical)
            {
                IFC(spScrollInfo->SetVerticalOffset(offset));
            }
            else
            {
                IFC(spScrollInfo->SetHorizontalOffset(offset));
            }
            IFC(m_tpScrollViewer->InvalidateScrollInfo());
        }
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
    FlipView::PrepareContainerForItemOverride(
    _In_ xaml::IDependencyObject* element,
    _In_ IInspectable* item)
{
    HRESULT hr = S_OK;
    DependencyObject *pElement = static_cast<DependencyObject*>(element);
    FlipViewItem *pFlipViewItem = NULL;
    DOUBLE value = 0.0;
    xaml::Thickness flipViewItemMargin = {};

    IFC(FlipViewGenerated::PrepareContainerForItemOverride(element, item));

    // Cast container to known type
    pFlipViewItem = static_cast<FlipViewItem*>(pElement);
    IFCEXPECT(pFlipViewItem);

    IFC(pFlipViewItem->get_Margin(&flipViewItemMargin));

    IFC(GetDesiredItemWidth(&value));
    value -= (flipViewItemMargin.Left + flipViewItemMargin.Right);
    IFC(pFlipViewItem->put_Width(value));

    IFC(GetDesiredItemHeight(&value));
    value -= (flipViewItemMargin.Top + flipViewItemMargin.Bottom);
    IFC(pFlipViewItem->put_Height(value));

Cleanup:
    RRETURN(hr);
}

// Create FlipViewAutomationPeer to represent the FlipView.
IFACEMETHODIMP FlipView::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IFlipViewAutomationPeer> spFlipViewAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IFlipViewAutomationPeerFactory> spFlipViewAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::FlipViewAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spFlipViewAPFactory));

    IFC(spFlipViewAPFactory.Cast<FlipViewAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spFlipViewAutomationPeer));
    IFC(spFlipViewAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::OnScrollingHostPartSizeChanged(
    _In_ IInspectable* pSender,
    _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableItems;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
    DOUBLE width = 0.0;
    DOUBLE height = 0.0;
    DOUBLE value = 0.0;
    XUINT32 nCount = 0;
    xaml::Thickness flipViewItemMargin = {};

    // Get desired container width/height
    IFC(GetDesiredItemWidth(&width));
    IFC(GetDesiredItemHeight(&height));

    // Iterate through all indices
    IFC(get_Items(&spObservableItems));
    IFC(spObservableItems.As(&spItems));
    IFC(spItems->get_Size(&nCount));
    for (XUINT32 i = 0; i < nCount; i++)
    {
        ctl::ComPtr<IDependencyObject> spContainer;

        IFC(ContainerFromIndex(i, &spContainer));

        if (spContainer)
        {
            FlipViewItem* pFlipViewItemNoRef = NULL;
            // Update the container's width/height to match
            pFlipViewItemNoRef = spContainer.Cast<FlipViewItem>();
            IFC(pFlipViewItemNoRef->get_Margin(&flipViewItemMargin));
            // NOTE: The following two assignments don't currently seem to have an effect (known layout bug)
            value = width - (flipViewItemMargin.Left + flipViewItemMargin.Right);
            IFC(pFlipViewItemNoRef->put_Width(value));
            value =  height - (flipViewItemMargin.Top + flipViewItemMargin.Bottom);
            IFC(pFlipViewItemNoRef->put_Height(value));
        }
    }

    m_itemsAreSized = true;

    IFC(SetFixOffsetTimer());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::OnPreviousButtonPartClick(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(MovePrevious());
    IFC(ResetButtonsFadeOutTimer());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::OnNextButtonPartClick(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(MoveNext());
    IFC(ResetButtonsFadeOutTimer());

Cleanup:
    RRETURN(hr);
}

// Invoked when the PointerEntered for the navigation buttons event is raised.
_Check_return_ HRESULT
    FlipView::OnPointerEnteredNavigationButtons(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    m_showNavigationButtons = TRUE;
    m_keepNavigationButtonsVisible = true;

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

// Invoked when the PointerExited for the navigation buttons event is raised.
_Check_return_ HRESULT
    FlipView::OnPointerExitedNavigationButtons(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    m_keepNavigationButtonsVisible = false;

    IFC_RETURN(ResetButtonsFadeOutTimer());

    return S_OK;
}

// DirectManipulationStateChangeHandler implementation
_Check_return_ HRESULT
    FlipView::NotifyStateChange(
    _In_ DMManipulationState /*state*/,
    _In_ FLOAT /*xCumulativeTranslation*/,
    _In_ FLOAT /*yCumulativeTranslation*/,
    _In_ FLOAT /*zCumulativeFactor*/,
    _In_ FLOAT /*xCenter*/,
    _In_ FLOAT /*yCenter*/,
    _In_ BOOLEAN /*isInertial*/,
    _In_ BOOLEAN isTouchConfigurationActivated,
    _In_ BOOLEAN /*isBringIntoViewportConfigurationActivated*/)
{
    // If while we are using ScrollViewer to animate the selected FlipViewItem into view the user begins a manipulation, this effectively
    // forces us to cancel the ScrollIntoView.
    if (isTouchConfigurationActivated && m_animateNewIndex)
    {
        IFC_RETURN(RestoreSnapPointsTypes());
        m_animateNewIndex = FALSE;
    }

    return S_OK;
}

_Check_return_ HRESULT
    FlipView::ChangeVisualState(
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<IInspectable*>> spItems;
    ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spObservableItems;
    INT nSelectedIndex = 0;
    XUINT32 nCount = 0;
    BOOLEAN nothingPrevious = FALSE;
    BOOLEAN nothingNext = FALSE;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
    BOOLEAN isVertical = FALSE;
    BOOLEAN bIgnore = FALSE;
    BOOLEAN bHasFocus = FALSE;

    // Determine the correct button/previous/next visibility

    IFC(get_Items(&spObservableItems));
    IFC(spObservableItems.As(&spItems));
    IFC(spItems->get_Size(&nCount));
    IFC(get_SelectedIndex(&nSelectedIndex));

    if ((0 == nSelectedIndex) || (nCount <= 1))
    {
        nothingPrevious = TRUE;
    }
    if ((nCount - 1 == nSelectedIndex) || (nCount <= 1))
    {
        nothingNext = TRUE;
    }

    IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
    isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);

    // Apply the new visibilities
    if (m_tpPreviousButtonHorizontalPart)
    {
        IFC(m_tpPreviousButtonHorizontalPart.Cast<ButtonBase>()->put_Visibility(
            (!m_showNavigationButtons || nothingPrevious || isVertical) ?
            xaml::Visibility_Collapsed : xaml::Visibility_Visible));
    }
    if (m_tpPreviousButtonVerticalPart)
    {
        IFC(m_tpPreviousButtonVerticalPart.Cast<ButtonBase>()->put_Visibility(
            (!m_showNavigationButtons || nothingPrevious || !isVertical) ?
            xaml::Visibility_Collapsed : xaml::Visibility_Visible));
    }
    if (m_tpNextButtonHorizontalPart)
    {
        IFC(m_tpNextButtonHorizontalPart.Cast<ButtonBase>()->put_Visibility(
            (!m_showNavigationButtons || nothingNext || isVertical) ?
            xaml::Visibility_Collapsed : xaml::Visibility_Visible));
    }
    if (m_tpNextButtonVerticalPart)
    {
        IFC(m_tpNextButtonVerticalPart.Cast<ButtonBase>()->put_Visibility(
            (!m_showNavigationButtons || nothingNext || !isVertical) ?
            xaml::Visibility_Collapsed : xaml::Visibility_Visible));
    }

    IFC(HasFocus(&bHasFocus));
    if (bHasFocus)
    {
        if (m_ShouldShowFocusRect)
        {

            IFC(GoToState(bUseTransitions, L"Focused", &bIgnore));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"PointerFocused", &bIgnore));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bIgnore));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::GetDesiredItemWidth(
    _Out_ DOUBLE* pWidth)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IVirtualizingPanel> spVirtualizingPanel;
    DOUBLE width = 0.0;

    IFC(get_ItemsHost(&spPanel));
    spVirtualizingPanel = spPanel.AsOrNull<IVirtualizingPanel>();

    if (spVirtualizingPanel)
    {
        width = spVirtualizingPanel.Cast<VirtualizingPanel>()->GetMeasureAvailableSize().Width;
    }

    if (DoubleUtil::IsInfinity(width) || width <= 0)
    {
        // Desired container width matches the width of the ScrollingHost part (or FlipView)
        IFC(m_tpScrollViewer ? m_tpScrollViewer.Cast<ScrollViewer>()->get_ActualWidth(pWidth) : get_ActualWidth(pWidth));
    }
    else
    {
        *pWidth = width;
    }

    // If flipview has never been measured yet - scroll viewer will not have its size set.
    // Use flipview size set by developer in that case.
    if (*pWidth <= 0)
    {
        IFC(get_Width(pWidth));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::GetDesiredItemHeight(
    _Out_ DOUBLE* pHeight)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPanel> spPanel;
    ctl::ComPtr<IVirtualizingPanel> spVirtualizingPanel;
    DOUBLE height = 0.0;

    IFC(get_ItemsHost(&spPanel));
    spVirtualizingPanel = spPanel.AsOrNull<IVirtualizingPanel>();

    if (spVirtualizingPanel)
    {
        height = spVirtualizingPanel.Cast<VirtualizingPanel>()->GetMeasureAvailableSize().Height;
    }

    if (DoubleUtil::IsInfinity(height) || height <= 0)
    {
        // Desired container height matches the height of the ScrollingHost part (or FlipView)
        IFC(m_tpScrollViewer ? m_tpScrollViewer.Cast<ScrollViewer>()->get_ActualHeight(pHeight) : get_ActualHeight(pHeight));
    }
    else
    {
        *pHeight = height;
    }

    // If flipview has never been measured yet - scroll viewer will not have its size set.
    // Use flipview size set by developer in that case.
    if (*pHeight <= 0)
    {
        IFC(get_Height(pHeight));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
    FlipView::MeasureOverride(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* returnValue)
{
    DOUBLE height = 0;
    DOUBLE width = 0;
    wf::Rect rootRect = {};

    m_inMeasure = true;
    auto scopeExit = wil::scope_exit([&]
    {
        m_inMeasure = false;
    });

    // FlipView requires to set its size on the FlipViewItems in order to show each item taking the whole space.
    // If no size is defined on the FlipView we get into a measure cycle which causes the application to not respond.
    // Fix is to use min of availableSize and window size as available size for measuring FlipView.
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &rootRect));
    IFC_RETURN(get_Height(&height));
    if (DoubleUtil::IsNaN(height))
    {
        availableSize.Height = MIN(rootRect.Height, availableSize.Height);
    }

    IFC_RETURN(get_Width(&width));
    if (DoubleUtil::IsNaN(width))
    {
        availableSize.Width = MIN(rootRect.Width, availableSize.Width);
    }

    IFC_RETURN(FlipViewGenerated::MeasureOverride(availableSize, returnValue));

    return S_OK;
}

IFACEMETHODIMP
FlipView::ArrangeOverride(_In_ wf::Size arrangeSize, _Out_ wf::Size* returnValue)
{
    m_inArrange = true;
    auto scopeExit = wil::scope_exit([&]
    {
        m_inArrange = false;
    });

    IFC_RETURN(FlipViewGenerated::ArrangeOverride(arrangeSize, returnValue));

    return S_OK;
}

// GotFocus event handler.
IFACEMETHODIMP FlipView::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;
    CControl *pControl = do_pointer_cast<CControl>(GetHandle());
    IFC(FlipViewGenerated::OnGotFocus(pArgs));

    m_moveFocusToSelectedItem = false;

    if (!pControl->IsFocusEngagementEnabled() || pControl->IsFocusEngaged())
    {
        IFC(HasFocus(&hasFocus));

        if (hasFocus)
        {
            ctl::ComPtr<IInspectable> spOriginalSource;

            IFC(pArgs->get_OriginalSource(&spOriginalSource));

            //Need to show Focus visual for FlipView whenever an item in a FlipView item has focus.
            if (spOriginalSource)
            {
                ctl::ComPtr<IUIElement> spFocusedElement;

                //Need focus state to show focus rect only when keyboard focus.
                spFocusedElement = spOriginalSource.AsOrNull<IUIElement>();
                if (spFocusedElement)
                {
                    IFC(spFocusedElement->get_FocusState(&focusState));
                }

                if (xaml::FocusState_Keyboard == focusState)
                {
                    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
                    if (contentRoot->GetInputManager().GetLastInputDeviceType() == DirectUI::InputDeviceType::GamepadOrRemote)
                    {
                        m_keepNavigationButtonsVisible = FALSE;
                    }

                    // Keyboard should show buttons
                    m_ShouldShowFocusRect = TRUE;
                    IFC(ResetButtonsFadeOutTimer());
                }
                else
                {
                    m_ShouldShowFocusRect = FALSE;
                }

                IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

                //If the FlipView is the item getting the focus then focus needs to be set to one of its items.
                if (isOriginalSource)
                {
                    IFC(put_IsSelectionActive(hasFocus));
                    IFC(SetFocusedItem(GetFocusedIndex() < 0 ? 0 : GetFocusedIndex(), TRUE));
                }
                else
                {
                    INT selectedIndex = 0;
                    IFC(get_SelectedIndex(&selectedIndex));
                    if (selectedIndex >= 0)
                    {
                        ctl::ComPtr<IDependencyObject> spContainer;
                        IFC(ContainerFromIndex(selectedIndex, &spContainer));
                        if (!spContainer)
                        {
                            // If we get focus when there is no container for the current item, we want to move focus
                            // to the selected item once it is available.
                            m_moveFocusToSelectedItem = true;
                        }
                    }
                }
                IFC(UpdateVisualState(TRUE));
            }
        }
    }
Cleanup:
    RRETURN(hr);
}

// GotFocus event handler.
IFACEMETHODIMP FlipView::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;
    BOOLEAN hasFocus = FALSE;
    bool isOriginalSource = false;

    IFC(FlipViewGenerated::OnLostFocus(pArgs));
    IFC(HasFocus(&hasFocus));

    IFCPTR(pArgs);
    IFC(pArgs->get_OriginalSource(&spOriginalSource));
    IFC(ctl::are_equal(spOriginalSource.Get(), ctl::as_iinspectable(this), &isOriginalSource));

    if (isOriginalSource)
    {
        IFC(put_IsSelectionActive(hasFocus));
        if (hasFocus)
        {
            IFC(SetFocusedItem(GetFocusedIndex() < 0 ? 0 : GetFocusedIndex(), TRUE));
        }
    }

    if (!hasFocus)
    {
        m_moveFocusToSelectedItem = false;
        m_ShouldShowFocusRect = FALSE;
        IFC(UpdateVisualState(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FlipView::OnPointerCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(FlipViewGenerated::OnPointerCaptureLost(pArgs));
    IFC(HandlePointerLostOrCanceled(pArgs));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FlipView::OnPointerCanceled(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(FlipViewGenerated::OnPointerCanceled(pArgs));
    IFC(HandlePointerLostOrCanceled(pArgs));
    IFC(HideButtonsImmediately());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlipView::HandlePointerLostOrCanceled(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    mui::PointerDeviceType nPointerDeviceType = mui::PointerDeviceType_Touch;

    IFCPTR(pArgs);
    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCPTR(spPointerPoint);
    IFC(spPointerPoint->get_PointerDeviceType(&nPointerDeviceType));
    if (nPointerDeviceType == mui::PointerDeviceType_Touch)
    {
        m_ShouldShowFocusRect = FALSE;
        IFC(UpdateVisualState(TRUE));
    }

Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the OnPropertyChanged2 methods.
_Check_return_
    HRESULT
    FlipView::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    switch(args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::UIElement_Visibility:
        IFC(OnVisibilityChanged());
        break;

    case KnownPropertyIndex::Selector_SelectedIndex:
        {
            ctl::ComPtr<IInspectable> spOldValue, spNewValue;
            IFC(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &spOldValue));
            IFC(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &spNewValue));
            IFC(OnSelectedIndexChanged(spOldValue.Get(), spNewValue.Get()));
        }
        break;
    }

    IFC(FlipViewGenerated::OnPropertyChanged2(args));

Cleanup:
    RRETURN(hr);
}

_Check_return_
    HRESULT
    FlipView::OnSelectedIndexChanged(
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    INT32 newIndex = 0;
    INT32 oldIndex = 0;
    BOOLEAN shouldUseAnimation = FALSE;

    if (IsInit())
    {
        goto Cleanup;
    }

    IFC(get_UseTouchAnimationsForAllNavigation(&shouldUseAnimation));
    IFC(ctl::do_get_value(oldIndex, pOldValue));
    IFC(ctl::do_get_value(newIndex, pNewValue));

    if ((oldIndex == newIndex + 1 || oldIndex == newIndex - 1) && m_tpScrollViewer.Get() && m_tpScrollViewer.Cast<ScrollViewer>()->IsManipulationHandlerReady()
        && shouldUseAnimation && !m_skipAnimationOnce && oldIndex != -1 && newIndex != -1)
    {
        XRECTF bounds = {};
        BOOLEAN handled = FALSE;
        ctl::ComPtr<IScrollInfo> spScrollInfo;

        IFC(CalculateBounds(newIndex, &bounds));
        IFC(SaveAndClearSnapPointsTypes());
        m_skipScrollIntoView = true;
        m_animateNewIndex = TRUE;

        if (m_tpScrollViewer.Get() != nullptr)
        {
            DOUBLE svOffset = 0;
            DOUBLE siOffset = 0;
            xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;
            BOOLEAN isVertical = FALSE;

            IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));
            isVertical = (physicalOrientation == xaml_controls::Orientation_Vertical);

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_ScrollInfo(&spScrollInfo));
            if (spScrollInfo)
            {
                // We do not want the measure pass and DManip try to set contradicting offsets on the ScrollViewer in the next tick.
                //So, if the ScrollViewer is waiting to be updated for the changes we made at this pass, update its layout.
                if (isVertical)
                {
                    IFC(spScrollInfo->get_VerticalOffset(&siOffset));
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_VerticalOffset(&svOffset));
                }
                else
                {
                    IFC(spScrollInfo->get_HorizontalOffset(&siOffset));
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->get_HorizontalOffset(&svOffset));
                }

                if (svOffset != siOffset)
                {
                    IFC(m_tpScrollViewer.Cast<ScrollViewer>()->UpdateLayout());
                }
            }

            if (m_tpFixOffsetTimer)
            {
                IFC(m_tpFixOffsetTimer->Stop());
            }
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->InvalidateScrollInfo());
            // When the application is not being rendered, there is no need to animate
            // the view change. Instead, a view jump is performed to minimize the CPU cost.
            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->BringIntoViewport(
                bounds,
                TRUE  /*skipDuringTouchContact*/,
                FALSE /*skipAnimationWhileRunning*/,
                DXamlCore::GetCurrent()->IsRenderingFrames() /*animate*/,
                &handled));
        }
    }
    else
    {
        // During an animated view change, cancel the user input if present, and do not change
        // the view according to the new SelectedIndex immediately, with SetOffsetToSelectedIndex.
        // Instead, wait for the animation to complete and then only sync the view to the new index
        // with SetFixOffsetTimer.
        if (m_animateNewIndex)
        {
            BOOLEAN succeeded = FALSE;

            IFC(m_tpScrollViewer.Cast<ScrollViewer>()->CancelDirectManipulations(&succeeded));
            IFC(RestoreSnapPointsTypes());
            m_animateNewIndex = FALSE;
            IFC(SetFixOffsetTimer());
        }
        else
        {
            IFC(SetOffsetToSelectedIndex());
        }
        m_skipScrollIntoView = false;
    }

    m_skipAnimationOnce = FALSE;

Cleanup:
    RRETURN(hr);
}

// Called when the IsEnabled property changes.
_Check_return_ HRESULT FlipView::OnIsEnabledChanged(_In_ IsEnabledChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;

    IFC(FlipViewGenerated::OnIsEnabledChanged(pArgs));

    IFC(get_IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        m_ShouldShowFocusRect = FALSE;
        IFC(HideButtonsImmediately());
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Update the visual states when the Visibility property is changed.
_Check_return_ HRESULT
    FlipView::OnVisibilityChanged()
{
    HRESULT hr = S_OK;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(get_Visibility(&visibility));
    if (xaml::Visibility_Visible != visibility)
    {
        m_ShouldShowFocusRect = FALSE;
        IFC(HideButtonsImmediately());
    }

    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// When size changes we need to reset the offsets on next tick.
// In current tick the IScrollInfo might not have the correct extent because it hasn't been measured yet.
_Check_return_ HRESULT FlipView::SetFixOffsetTimer()
{
    HRESULT hr = S_OK;

    if (m_tpFixOffsetTimer)
    {
        IFC(m_tpFixOffsetTimer->Stop());
    }
    else
    {
        ctl::ComPtr<DispatcherTimer> spNewDispatcherTimer;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spFixOffsetTimerTickEventHandler;
        EventRegistrationToken fixOffsetTimerTickToken;

        IFC(ctl::make<DispatcherTimer>(&spNewDispatcherTimer));

        spFixOffsetTimerTickEventHandler.Attach(
            new ClassMemberEventHandler<
            FlipView,
            xaml_controls::IFlipView,
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable>(this, &FlipView::FixOffset));

        IFC(spNewDispatcherTimer->add_Tick(spFixOffsetTimerTickEventHandler.Get(), &fixOffsetTimerTickToken));

        wf::TimeSpan showDurationTimeSpan = { 0 };
        IFC(spNewDispatcherTimer->put_Interval(showDurationTimeSpan));

        SetPtrValue(m_tpFixOffsetTimer, spNewDispatcherTimer.Get());
        IFCPTR(m_tpFixOffsetTimer.Get());
    }

    IFC(m_tpFixOffsetTimer->Start());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlipView::FixOffset(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    IFC_RETURN(m_tpFixOffsetTimer->Stop());

    if (m_tpScrollViewer && !m_tpScrollViewer.Cast<ScrollViewer>()->IsInManipulation())
    {
        IFC_RETURN(SetOffsetToSelectedIndex());
    }
    else
    {
        // If we are in the middle of a manipulation (during the animation from one selected item to another) we
        // don't want to modify the offset. Postpone the fix via timer.
        IFC_RETURN(SetFixOffsetTimer());
    }

    return S_OK;
}

// Creates a DispatcherTimer for fading out the FlipView's buttons.
_Check_return_ HRESULT
    FlipView::EnsureButtonsFadeOutTimer()
{
    HRESULT hr = S_OK;

    if (!m_tpButtonsFadeOutTimer.Get())
    {
        ctl::ComPtr<DispatcherTimer> spNewDispatcherTimer;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spButtonsFadeOutTimerTickHandler;
        EventRegistrationToken buttonsFadeOutTimerTickToken;
        wf::TimeSpan showDurationTimeSpan = {};

        IFC(ctl::make<DispatcherTimer>(&spNewDispatcherTimer));

        spButtonsFadeOutTimerTickHandler.Attach(
            new ClassMemberEventHandler<
            FlipView,
            IFlipView,
            wf::IEventHandler<IInspectable*>,
            IInspectable,
            IInspectable>(this, &FlipView::ButtonsFadeOutTimerTickHandler));
        IFC(spNewDispatcherTimer->add_Tick(spButtonsFadeOutTimerTickHandler.Get(), &buttonsFadeOutTimerTickToken));

        showDurationTimeSpan.Duration = FLIP_VIEW_BUTTONS_SHOW_DURATION_MS * TICKS_PER_MILLISECOND;
        IFC(spNewDispatcherTimer->put_Interval(showDurationTimeSpan));

        SetPtrValue(m_tpButtonsFadeOutTimer, spNewDispatcherTimer.Get());
    }

Cleanup:
    RRETURN(hr);
}

// Starts the fade out timer if not yet started.  Otherwise, resets it back to the original duration.
_Check_return_ HRESULT
    FlipView::ResetButtonsFadeOutTimer()
{
    HRESULT hr = S_OK;

    if (!m_showNavigationButtons)
    {
        IFC(EnsureButtonsFadeOutTimer());
        m_showNavigationButtons = TRUE;
        IFC(UpdateVisualState());
    }

    // Bug# 1292039: in the CanFlipWithMouse test, the mouse enters the the flipper button
    // directly which will set m_showNavigationButtons to true
    // this will not initialize m_tpButtonsFadeOutTimer
    if (m_tpButtonsFadeOutTimer)
    {
        IFC(m_tpButtonsFadeOutTimer->Start());
    }

Cleanup:
    RRETURN(hr);
}

// Hides the navigation buttons immediately.
_Check_return_ HRESULT
    FlipView::HideButtonsImmediately()
{
    HRESULT hr = S_OK;

    if (m_showNavigationButtons)
    {
        if (m_tpButtonsFadeOutTimer)
        {
            IFC(m_tpButtonsFadeOutTimer->Stop());
        }

        if (!m_keepNavigationButtonsVisible)
        {
            m_showNavigationButtons = FALSE;
            IFC(UpdateVisualState());
        }
    }

Cleanup:
    RRETURN(hr);
}

// Handler for the Tick event on m_tpButtonsFadeOutTimer.
_Check_return_ HRESULT
    FlipView::ButtonsFadeOutTimerTickHandler(
    _In_opt_ IInspectable* pUnused1,
    _In_opt_ IInspectable* pUnused2)
{
    HRESULT hr = S_OK;

    IFC(HideButtonsImmediately());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
    FlipView::GetPreviousAndNextButtons(
    _Outptr_ ButtonBase** ppPreviousButton,
    _Outptr_ ButtonBase** ppNextButton)
{
    HRESULT hr = S_OK;
    xaml_controls::Orientation physicalOrientation = xaml_controls::Orientation_Vertical;

    IFCPTR(ppPreviousButton);
    IFCPTR(ppPreviousButton);

    // Determine the correct button/previous/next
    IFC(GetItemsHostOrientations(&physicalOrientation, NULL /*pLogicalOrientation*/));

    if(physicalOrientation == xaml_controls::Orientation_Vertical)
    {
        *ppPreviousButton = m_tpPreviousButtonVerticalPart.Cast<ButtonBase>();
        *ppNextButton = m_tpNextButtonVerticalPart.Cast<ButtonBase>();
        ctl::addref_interface(*ppPreviousButton);
        ctl::addref_interface(*ppNextButton);
    }
    else
    {
        *ppPreviousButton = m_tpPreviousButtonHorizontalPart.Cast<ButtonBase>();
        *ppNextButton = m_tpNextButtonHorizontalPart.Cast<ButtonBase>();
        ctl::addref_interface(*ppPreviousButton);
        ctl::addref_interface(*ppNextButton);
    }

Cleanup:
    RRETURN(hr);
}



