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
#include "PipsControlAutomationPeer.h"

typedef winrt::IndicatorPagerButtonVisibility ButtonVisibility;

static constexpr wstring_view s_pipButtonHandlersPropertyName = L"PipButtonHandlers"sv;

constexpr auto c_previousPageButtonVisibleVisualState = L"PreviousPageButtonVisible"sv;
constexpr auto c_previousPageButtonHiddenVisualState = L"PreviousPageButtonHidden"sv;
constexpr auto c_previousPageButtonCollapsedVisualState = L"PreviousPageButtonCollapsed"sv;

constexpr auto c_previousPageButtonEnabledVisualState = L"PreviousPageButtonEnabled"sv;
constexpr auto c_previousPageButtonDisabledVisualState = L"PreviousPageButtonDisabled"sv;

constexpr auto c_nextPageButtonVisibleVisualState = L"NextPageButtonVisible"sv;
constexpr auto c_nextPageButtonHiddenVisualState = L"NextPageButtonHidden"sv;
constexpr auto c_nextPageButtonCollapsedVisualState = L"NextPageButtonCollapsed"sv;

constexpr auto c_nextPageButtonEnabledVisualState = L"NextPageButtonEnabled"sv;
constexpr auto c_nextPageButtonDisabledVisualState = L"NextPageButtonDisabled"sv;

constexpr auto c_infiniteItemsModeState = L"InfiniteItems"sv;

constexpr auto c_rootGridName = L"RootGrid"sv;

constexpr auto c_previousPageButtonName = L"PreviousPageButton"sv;
constexpr auto c_nextPageButtonName = L"NextPageButton"sv;

constexpr auto c_PipsControlPipsRepeaterName = L"VerticalPipsItemsRepeater"sv;
constexpr auto c_PipsControlScrollViewerName = L"VerticalPipsScrollViewer"sv;

constexpr auto c_PipsControlButtonHeightPropertyName = L"PipsControlVerticalPipButtonHeight"sv;

PipsControl::PipsControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PipsControl);

    m_verticalPipsElements = winrt::make<Vector<IInspectable>>().as<winrt::IObservableVector<IInspectable>>();
    const auto templateSettings = winrt::make<PipsControlTemplateSettings>();
    templateSettings.SetValue(PipsControlTemplateSettings::s_VerticalPipsItemsProperty, m_verticalPipsElements);
    SetValue(s_TemplateSettingsProperty, templateSettings);

    s_pipButtonHandlersProperty =
        InitializeDependencyProperty(
            s_pipButtonHandlersPropertyName,
            winrt::name_of<PipsControlViewItemRevokers>(),
            winrt::name_of<winrt::PipsControl>(),
            true,
            nullptr,
            nullptr);
    SetDefaultStyleKey(this);
}

void PipsControl::OnApplyTemplate()
{
    winrt::AutomationProperties::SetName(*this, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlNameText));

    m_previousPageButtonClickRevoker.revoke();
    [this](const winrt::Button button) {
        if (button) {
            winrt::AutomationProperties::SetName(button, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlPreviousPageButtonText));
            m_previousPageButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &PipsControl::OnPreviousButtonClicked });
        }
    }(GetTemplateChildT<winrt::Button>(c_previousPageButtonName, *this));

    m_nextPageButtonClickRevoker.revoke();
    [this](const winrt::Button button) {
        if (button) {
            winrt::AutomationProperties::SetName(button, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlNextPageButtonText));
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

    m_rootGridKeyDownRevoker.revoke();
    [this](const winrt::Grid grid) {
        if (grid) {
            m_rootGridKeyDownRevoker = grid.KeyDown(winrt::auto_revoke, { this, &PipsControl::OnRootGridKeyDown });

        }
    }(GetTemplateChildT<winrt::Grid>(c_rootGridName, *this));

    OnMaxDisplayedPagesChanged(m_lastMaxDisplayedPages);
    OnNumberOfPagesChanged();
    OnSelectedPageIndexChanged(m_lastSelectedPageIndex);
}

void PipsControl::OnPointerEntered(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerEntered(args);
    m_isPointerOver = true;
    UpdateNavigationButtonVisualStates();
}

void PipsControl::OnPointerExited(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerExited(args);

    // We can get a spurious Exited and then Entered if the button
    // that is being clicked on hides itself. In order to avoid switching
    // visual states in this case, we check if the pointer is over the
    // control bounds when we get the exited event.
    if (IsOutOfControlBounds(args.GetCurrentPoint(*this).Position()))
    {
        m_isPointerOver = false;
        UpdateNavigationButtonVisualStates();
    }
}

void PipsControl::OnPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCanceled(args);
    m_isPointerOver = false;
    UpdateNavigationButtonVisualStates();
}

bool PipsControl::IsOutOfControlBounds(winrt::Point point) {
    // This is a conservative check. It is okay to say we are within
    // bounds when close to the edge to account for rounding.
    const auto tolerance = 1.0;
    const auto actualWidth = ActualWidth();
    const auto actualHeight = ActualHeight();
    return point.X < tolerance ||
       point.X > actualWidth - tolerance ||
       point.Y < tolerance ||
       point.Y  > actualHeight - tolerance;
}

void PipsControl::UpdateIndividualNavigationButtonVisualState(
    bool hiddenOnEdgeCondition,
    ButtonVisibility visibility,
    const wstring_view visibleStateName,
    const wstring_view hiddenStateName,
    const wstring_view enabledStateName,
    const wstring_view disabledStateName) {

    const auto ifGenerallyVisible = !hiddenOnEdgeCondition && NumberOfPages() > 0 && MaxDisplayedPages() > 0;
    if (visibility != ButtonVisibility::Collapsed) {
        if ((visibility == ButtonVisibility::Visible || m_isPointerOver) && ifGenerallyVisible) {
            winrt::VisualStateManager::GoToState(*this, visibleStateName, false);
            winrt::VisualStateManager::GoToState(*this, enabledStateName, false);
        }
        else {
            if (!ifGenerallyVisible) {
                winrt::VisualStateManager::GoToState(*this, disabledStateName, false);
            }
            winrt::VisualStateManager::GoToState(*this, hiddenStateName, false);
        }
    }
}

void PipsControl::OnNavigationButtonVisibilityChanged(ButtonVisibility visibility, const wstring_view collapsedStateName) {
    if (visibility == ButtonVisibility::Collapsed) {
        winrt::VisualStateManager::GoToState(*this, collapsedStateName, false);
    }
    else {
        UpdateNavigationButtonVisualStates();
    }
}

void PipsControl::UpdateNavigationButtonVisualStates() {

    const int selectedPageIndex = SelectedPageIndex();
    const int numberOfPages = NumberOfPages();
    const int maxDisplayedPages = MaxDisplayedPages();

    // if (!numberOfPages == 0 && maxDisplayedPages > 0) {
    auto const ifPreviousButtonHiddenOnEdge = selectedPageIndex == 0;
    UpdateIndividualNavigationButtonVisualState(ifPreviousButtonHiddenOnEdge, PreviousButtonVisibility(),
        c_previousPageButtonVisibleVisualState, c_previousPageButtonHiddenVisualState,
        c_previousPageButtonEnabledVisualState, c_previousPageButtonDisabledVisualState);

    auto const ifNextButtonHiddenOnEdge = selectedPageIndex == numberOfPages - 1;
    UpdateIndividualNavigationButtonVisualState(ifNextButtonHiddenOnEdge, NextButtonVisibility(),
        c_nextPageButtonVisibleVisualState, c_nextPageButtonHiddenVisualState,
        c_nextPageButtonEnabledVisualState, c_nextPageButtonDisabledVisualState);
    // }
}

void PipsControl::OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (auto const element = args.Element())
    {
        if (const auto pip = element.try_as<winrt::Button>())
        {
            auto const index = args.Index();
            if (index != SelectedPageIndex())
            {
                pip.Style(DefaultIndicatorStyle());
            }

            // Narrator says: Page 5, Button 5 of 30. Is it expected behavior?
            winrt::AutomationProperties::SetName(pip, ResourceAccessor::GetLocalizedStringResource(SR_PipsControlPageText) + L" " + winrt::to_hstring(index + 1));
            winrt::AutomationProperties::SetPositionInSet(pip, index + 1);
            winrt::AutomationProperties::SetSizeOfSet(pip, NumberOfPages());

            auto pciRevokers = winrt::make_self<PipsControlViewItemRevokers>();
            pciRevokers->clickRevoker = pip.Click(winrt::auto_revoke,
                [this, index](auto const& sender, auto const& args) {
                    if (const auto button = sender.try_as<winrt::Button>())
                    {
                        SelectedPageIndex(index);
                    }
                }
            );
            pip.SetValue(s_pipButtonHandlersProperty, pciRevokers.as<winrt::IInspectable>());
        }
    }
}

void PipsControl::ScrollToCenterOfViewport(winrt::UIElement sender)
{
    winrt::BringIntoViewOptions options;
    options.VerticalAlignmentRatio(0.5);
    options.AnimationDesired(true);
    sender.StartBringIntoView(options);
}


void PipsControl::OnMaxDisplayedPagesChanged(const int oldValue)
{
    if (MaxDisplayedPages() >= 0) {
        m_lastMaxDisplayedPages = oldValue;
        UpdateVerticalPips(NumberOfPages(), MaxDisplayedPages());
        UpdateNavigationButtonVisualStates();
    }
    else {
        MaxDisplayedPages(0);
    }
}

void PipsControl::OnNumberOfPagesChanged()
{
    const int numberOfPages = NumberOfPages();
    if (SelectedPageIndex() > numberOfPages - 1 && numberOfPages > -1) {
        SelectedPageIndex(numberOfPages - 1);
    }
    else {
        UpdateVerticalPips(numberOfPages, MaxDisplayedPages());
        UpdateNavigationButtonVisualStates();
    }
}

void PipsControl::OnSelectedPageIndexChanged(const int oldValue)
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
    else {
        // Now handle the value changes
        m_lastSelectedPageIndex = oldValue;

        // Fire value property change for UIA
        if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::PipsControlAutomationPeer>())
        {
            winrt::get_self<PipsControlAutomationPeer>(peer)->RaiseSelectionChanged(m_lastSelectedPageIndex, SelectedPageIndex());
        }

        UpdateVerticalPips(NumberOfPages(), SelectedPageIndex());
        UpdateNavigationButtonVisualStates();
        RaiseSelectedIndexChanged();
    }
}

void PipsControl::RaiseSelectedIndexChanged()
{
    const auto args = winrt::make_self<PipsControlSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
}

void PipsControl::MovePipIdentifierToElement(int index) {

    if (NumberOfPages() != 0) {
        if (const auto repeater = m_verticalPipsRepeater.get())
        {
            if (const auto element = repeater.TryGetElement(m_lastSelectedPageIndex).try_as<winrt::Button>()) {
                element.Style(DefaultIndicatorStyle());
            }
            if (const auto element = repeater.GetOrCreateElement(index).try_as<winrt::Button>()) {
                element.Style(SelectedIndicatorStyle());
                ScrollToCenterOfViewport(element);
            }
        }
    }
}

void PipsControl::SetVerticalPipsSVMaxSize() {
    const auto pipHeight = unbox_value<double>(ResourceAccessor::ResourceLookup(*this, box_value(c_PipsControlButtonHeightPropertyName)));
    const auto numberOfPages = NumberOfPages() < 0 ? MaxDisplayedPages() : std::min(NumberOfPages(), MaxDisplayedPages());
    const auto scrollViewerHeight = pipHeight * numberOfPages;
    if (const auto scrollViewer = m_verticalPipsScrollViewer.get()) {
        m_verticalPipsScrollViewer.get().MaxHeight(scrollViewerHeight);
    }
}

void PipsControl::UpdateVerticalPips(const int numberOfPages, const int maxDisplayedPages) {

    auto pipsListSize = int(m_verticalPipsElements.Size());
    auto const selectedIndex = SelectedPageIndex();

    if (numberOfPages < pipsListSize) {
        m_verticalPipsElements.Clear();
        pipsListSize = 0;
    }

    auto const endIndex = std::min(numberOfPages, selectedIndex + maxDisplayedPages);
    for (int i = pipsListSize; i < endIndex; i++) {
        m_verticalPipsElements.Append(box_value(i + 1));
    }

    if (maxDisplayedPages != m_lastMaxDisplayedPages) {
        SetVerticalPipsSVMaxSize();
    }
    MovePipIdentifierToElement(SelectedPageIndex());

}

void PipsControl::OnRootGridKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args) {
    if (args.Key() == winrt::VirtualKey::Left || args.Key() == winrt::VirtualKey::Up)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Up);
    }
    else if (args.Key() == winrt::VirtualKey::Right || args.Key() == winrt::VirtualKey::Down)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Down);
    }
}


void PipsControl::OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always greater than 0.
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
            OnNumberOfPagesChanged();
        }
        else if (property == SelectedPageIndexProperty())
        {
            OnSelectedPageIndexChanged(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == MaxDisplayedPagesProperty()) {
            OnMaxDisplayedPagesChanged(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == PreviousButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(PreviousButtonVisibility(), c_previousPageButtonCollapsedVisualState);
        }
        else if (property == NextButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(NextButtonVisibility(), c_nextPageButtonCollapsedVisualState);
        }
    }
}

winrt::AutomationPeer PipsControl::OnCreateAutomationPeer()
{
    return winrt::make<PipsControlAutomationPeer>(*this);
}
