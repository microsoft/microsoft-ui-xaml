// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "PipsControl.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "PipsControlTemplateSettings.h"
#include "PipsControlSelectedIndexChangedEventArgs.h"



constexpr auto c_previousPageButtonVisibleVisualState = L"PreviousPageButtonVisible"sv;
constexpr auto c_previousPageButtonHiddenVisualState = L"PreviousPageButtonHidden"sv;

constexpr auto c_previousPageButtonEnabledVisualState = L"PreviousPageButtonEnabled"sv;
constexpr auto c_previousPageButtonDisabledVisualState = L"PreviousPageButtonDisabled"sv;

constexpr auto c_nextPageButtonVisibleVisualState = L"NextPageButtonVisible"sv;
constexpr auto c_nextPageButtonHiddenVisualState = L"NextPageButtonHidden"sv;

constexpr auto c_nextPageButtonEnabledVisualState = L"NextPageButtonEnabled"sv;
constexpr auto c_nextPageButtonDisabledVisualState = L"NextPageButtonDisabled"sv;

constexpr auto c_infiniteItemsModeState = L"InfiniteItems"sv;



constexpr auto c_previousPageButtonName = L"PreviousPageButton"sv;
constexpr auto c_nextPageButtonName = L"NextPageButton"sv;

constexpr auto c_PipsControlPipsRepeaterName = L"VerticalPipsItemsRepeater"sv;
constexpr auto c_PipsControlScrollViewerName = L"VerticalPipsScrollViewer"sv;

constexpr auto c_PipsControlButtonHeightPropertyName = L"PagerControlVerticalPipButtonHeight"sv;

PipsControl::PipsControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PipsControl);

    m_verticalPipsElements = winrt::make<Vector<IInspectable>>().as<winrt::IObservableVector<IInspectable>>();
    const auto templateSettings = winrt::make<PipsControlTemplateSettings>();
    templateSettings.SetValue(PipsControlTemplateSettings::s_VerticalPipsItemsProperty, m_verticalPipsElements);
    SetValue(s_TemplateSettingsProperty, templateSettings);

    SetDefaultStyleKey(this);

}

PipsControl::~PipsControl() {
    m_nextPageButtonClickRevoker.revoke();
    m_previousPageButtonClickRevoker.revoke();
    m_verticalPipsElementPreparedRevoker.revoke();
    m_gridPointerEnteredRevoker.revoke();
    m_gridPointerExitedRevoker.revoke();
}

void PipsControl::OnApplyTemplate()
{

    m_previousPageButtonClickRevoker.revoke();
    [this](const winrt::Button button) {
        if (button) {
            m_previousPageButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &PipsControl::OnPreviousButtonClicked });
        }
    }(GetTemplateChildT<winrt::Button>(c_previousPageButtonName, *this));

    m_nextPageButtonClickRevoker.revoke();
    [this](const winrt::Button button) {
        if (button) {
            m_nextPageButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &PipsControl::OnNextButtonClicked });
        }
    }(GetTemplateChildT<winrt::Button>(c_nextPageButtonName, *this));

    m_verticalPipsElementPreparedRevoker.revoke();
    [this](const winrt::ItemsRepeater repeater) {
        m_verticalPipsRepeater.set(repeater);
        if (repeater) {
            m_verticalPipsElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { this, &PipsControl::OnElementPrepared });
        }
    }(GetTemplateChildT<winrt::ItemsRepeater>(c_PipsControlPipsRepeaterName, *this));


    m_verticalPipsScrollViewer.set(GetTemplateChildT<winrt::FxScrollViewer>(c_PipsControlScrollViewerName, *this));

    m_gridPointerEnteredRevoker.revoke();
    m_gridPointerExitedRevoker.revoke();

    [this](const winrt::Grid grid) {
        if (grid) {
            m_gridPointerEnteredRevoker = grid.PointerEntered(winrt::auto_revoke, { this, &PipsControl::OnPipsControlPointerEntered });
            m_gridPointerExitedRevoker = grid.PointerExited(winrt::auto_revoke, { this, &PipsControl::OnPipsControlPointerExited });

        }
    }(GetTemplateChildT<winrt::Grid>(L"RootGrid", *this));

    OnNumberOfPagesChanged(0);
    OnSelectedPageIndexChange(-1);
}

void PipsControl::OnPipsControlPointerEntered(winrt::IInspectable sender, winrt::PointerRoutedEventArgs args) {
    UpdateNavigationButtonVisualStates();
}

void PipsControl::OnPipsControlPointerExited(winrt::IInspectable sender, winrt::PointerRoutedEventArgs args) {
    HideNavigationButtons();
}

void PipsControl::HideNavigationButtons() {
    winrt::VisualStateManager::GoToState(*this, c_previousPageButtonHiddenVisualState, false);
    winrt::VisualStateManager::GoToState(*this, c_nextPageButtonHiddenVisualState, false);
}
void PipsControl::UpdateNavigationButtonVisualStates() {

    
    const int selectedPageIndex = SelectedPageIndex();
    const int numberOfPages = NumberOfPages();

    if (!numberOfPages == 0) {
        if (selectedPageIndex != numberOfPages - 1) {
            winrt::VisualStateManager::GoToState(*this, c_nextPageButtonVisibleVisualState, false);
        }
        else {
            winrt::VisualStateManager::GoToState(*this, c_nextPageButtonHiddenVisualState, false);
        }

        if (selectedPageIndex != 0) {
            winrt::VisualStateManager::GoToState(*this, c_previousPageButtonVisibleVisualState, false);
        }
        else {
            winrt::VisualStateManager::GoToState(*this, c_previousPageButtonHiddenVisualState, false);
        }
    }

}

void PipsControl::OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (const auto pip = args.Element().try_as<winrt::Button>()) {
        if (unbox_value<int>(pip.Tag()) - 1 != SelectedPageIndex()) {
            winrt::VisualStateManager::GoToState(pip, L"Unselected", true);
        }

        const auto buttonClickedFunc = [this](auto const& sender, auto const& args) {
            if (const auto button = sender.try_as<winrt::Button>())
            {
                SelectedPageIndex(winrt::unbox_value<int>(button.Tag()) - 1);
            }
        };
        pip.Click(buttonClickedFunc);
    }
}

void PipsControl::ScrollToCenterOfViewport(winrt::UIElement sender)
{
    winrt::BringIntoViewOptions options;
    options.VerticalAlignmentRatio(0.5);
    options.AnimationDesired(true);
    sender.StartBringIntoView(options);
}


void PipsControl::OnMaxDisplayedPagesChanged(const int oldValue) {
    m_lastMaxDisplayedPages = oldValue;
    UpdateVerticalPips(NumberOfPages(), MaxDisplayedPages());
}

void PipsControl::OnNumberOfPagesChanged(const int oldValue)
{
    m_lastNumberOfPagesCount = oldValue;
    const int numberOfPages = NumberOfPages();
    if (SelectedPageIndex() + 1 >= numberOfPages && numberOfPages > -1)
    {
        SelectedPageIndex(numberOfPages - 1);
    }
    UpdateVerticalPips(NumberOfPages(), MaxDisplayedPages());
}

void PipsControl::OnSelectedPageIndexChange(const int oldValue)
{
    // If we don't have any pages, there is nothing we should do.
    // Ensure that SelectedPageIndex will end up in the valid range of values
    // Special case is NumberOfPages being 0, in that case, don't verify upperbound restrictions
    if (SelectedPageIndex() > NumberOfPages() - 1 && NumberOfPages() > 0)
    {
        SelectedPageIndex(NumberOfPages() - 1);
    }
    else if (SelectedPageIndex() < 0)
    {
        SelectedPageIndex(0);
    }
    // Now handle the value changes
    m_lastSelectedPageIndex = oldValue;

    UpdateVerticalPips(NumberOfPages(), MaxDisplayedPages());
      // Fire value property change for UIA
    /*if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::PipsControlAutomationPeer>())
    {
        winrt::get_self<PipsControlAutomationPeer>(peer)->RaiseSelectionChanged(m_lastSelectedPageIndex, SelectedPageIndex());
    }*/
    if (oldValue > -1) {

        // TODO: this will be called when the number of pages called in certain cases
        // -> it will update buttons visibility regardless of cursor being over them or not
        UpdateNavigationButtonVisualStates();
    }
    RaiseSelectedIndexChanged();
}

void PipsControl::RaiseSelectedIndexChanged()
{
    const auto args = winrt::make_self<PipsControlSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
}


void PipsControl::MovePipIdentifierToElement(int index) {

    if (const auto repeater = m_verticalPipsRepeater.get())
    {
        if (const auto element = repeater.TryGetElement(m_lastSelectedPageIndex).try_as<winrt::Button>()) {
            winrt::VisualStateManager::GoToState(element, L"Unselected", true);
        }
        if (const auto element = repeater.GetOrCreateElement(index).try_as<winrt::Button>()) {
            element.UpdateLayout();
            winrt::VisualStateManager::GoToState(element, L"Selected", true);
            ScrollToCenterOfViewport(element);
        }
    }
}

void PipsControl::setVerticalPipsSVMaxSize() {
    auto pipHeight = unbox_value<double>(ResourceAccessor::ResourceLookup(*this, box_value(c_PipsControlButtonHeightPropertyName)));
    auto numberOfPages = NumberOfPages() < 0 ? MaxDisplayedPages() : std::min(NumberOfPages(), MaxDisplayedPages());
    auto scrollViewerHeight = pipHeight * numberOfPages;
    m_verticalPipsScrollViewer.get().MaxHeight(scrollViewerHeight);
}


void PipsControl::UpdateVerticalPips(const int numberOfPages, const int maxDisplayedPages) {

    auto const pipsListSize = static_cast<int>(m_verticalPipsElements.Size());

    if (numberOfPages != pipsListSize) {
        // TODO: find a way to clean the pipsElements when switched to infinite mode from a numberOfPages >= 0
        // TODO: finalize the infinite behavior: do we start with 1 pin or MaxDisplayedPages
        if (numberOfPages < 0) {
            auto const startIndex = pipsListSize;
            auto const endIndex = pipsListSize < maxDisplayedPages ? maxDisplayedPages : pipsListSize + 1;
            for (int i = startIndex; i < endIndex; i++) {
                m_verticalPipsElements.Append(box_value(i + 1));
            }
        }
        else {
            m_verticalPipsElements.Clear();
            for (int i = 0; i < numberOfPages; i++) {
                m_verticalPipsElements.Append(box_value(i + 1));
            }
        }
    }
    if (maxDisplayedPages != m_lastMaxDisplayedPages) {
        setVerticalPipsSVMaxSize();
    }
    if (numberOfPages > 0) {
        MovePipIdentifierToElement(SelectedPageIndex());
    }
}


void PipsControl::OnRootGridKeyDown(const winrt::IInspectable & sender, const winrt::KeyRoutedEventArgs & args) {

    if (args.Key() == winrt::VirtualKey::Left || args.Key() == winrt::VirtualKey::GamepadDPadLeft)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left);
    }
    else if (args.Key() == winrt::VirtualKey::Right || args.Key() == winrt::VirtualKey::GamepadDPadRight)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right);
    }
}


void PipsControl::OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always greater than 1.
    SelectedPageIndex(SelectedPageIndex() - 1);
}

void PipsControl::OnNextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always less than maximum.
    SelectedPageIndex(SelectedPageIndex() + 1);
}




void PipsControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (this->Template() != nullptr)
    {
        if (property == NumberOfPagesProperty())
        {
            OnNumberOfPagesChanged(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == SelectedPageIndexProperty())
        {
            OnSelectedPageIndexChange(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == MaxDisplayedPagesProperty()) {
            OnMaxDisplayedPagesChanged(winrt::unbox_value<int>(args.OldValue()));
        }

        // TODO: decide if want to update navigation buttons visual states
        // after num of pages or max display pages change (we'll need to check if pointer is "inside" the control")

        // It will be updated once cursor falls in again, but if the cursor stays inside during the change
        // the button visuals won't be updated
    }
}
