// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "PipsPager.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "PipsPagerTemplateSettings.h"
#include "PipsPagerSelectedIndexChangedEventArgs.h"
#include "PipsPagerAutomationPeer.h"

typedef winrt::PipsPagerButtonVisibility ButtonVisibility;

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

constexpr auto c_previousPageButtonName = L"PreviousPageButton"sv;
constexpr auto c_nextPageButtonName = L"NextPageButton"sv;

constexpr auto c_pipsPagerRepeaterName = L"PipsPagerItemsRepeater"sv;
constexpr auto c_pipsPagerScrollViewerName = L"PipsPagerScrollViewer"sv;

constexpr auto c_pipsPagerPipButtonWidthPropertyName = L"PipsPagerPipButtonWidth"sv;
constexpr auto c_pipsPagerPipButtonHeightPropertyName = L"PipsPagerPipButtonWidth"sv;

constexpr auto c_pipsPagerHorizontalOrientationVisualState = L"HorizontalOrientationView"sv;
constexpr auto c_pipsPagerVerticalOrientationVisualState = L"VerticalOrientationView"sv;

PipsPager::PipsPager()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PipsPager);

    m_pipsPagerItems = winrt::make<Vector<IInspectable>>().as<winrt::IObservableVector<IInspectable>>();
    const auto templateSettings = winrt::make<PipsPagerTemplateSettings>();
    templateSettings.SetValue(PipsPagerTemplateSettings::s_PipsPagerItemsProperty, m_pipsPagerItems);
    SetValue(s_TemplateSettingsProperty, templateSettings);

    s_pipButtonHandlersProperty =
        InitializeDependencyProperty(
            s_pipButtonHandlersPropertyName,
            winrt::name_of<PipsPagerViewItemRevokers>(),
            winrt::name_of<winrt::PipsPager>(),
            true,
            nullptr,
            nullptr);
    SetDefaultStyleKey(this);
}

void PipsPager::OnApplyTemplate()
{
    winrt::AutomationProperties::SetName(*this, ResourceAccessor::GetLocalizedStringResource(SR_PipsPagerNameText));

    m_previousPageButtonClickRevoker.revoke();
    [this](const winrt::Button button)
    {
        if (button)
        {
            winrt::AutomationProperties::SetName(button, ResourceAccessor::GetLocalizedStringResource(SR_PipsPagerPreviousPageButtonText));
            m_previousPageButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &PipsPager::OnPreviousButtonClicked });
        }
    }(GetTemplateChildT<winrt::Button>(c_previousPageButtonName, *this));

    m_nextPageButtonClickRevoker.revoke();
    [this](const winrt::Button button)
    {
        if (button)
        {
            winrt::AutomationProperties::SetName(button, ResourceAccessor::GetLocalizedStringResource(SR_PipsPagerNextPageButtonText));
            m_nextPageButtonClickRevoker = button.Click(winrt::auto_revoke, { this, &PipsPager::OnNextButtonClicked });
        }
    }(GetTemplateChildT<winrt::Button>(c_nextPageButtonName, *this));

    m_pipsPagerElementPreparedRevoker.revoke();
    [this](const winrt::ItemsRepeater repeater)
    {
        m_pipsPagerRepeater.set(repeater);
        if (repeater)
        {
            m_pipsPagerElementPreparedRevoker = repeater.ElementPrepared(winrt::auto_revoke, { this, &PipsPager::OnElementPrepared });
        }
    }(GetTemplateChildT<winrt::ItemsRepeater>(c_pipsPagerRepeaterName, *this));

    m_pipsPagerScrollViewer.set(GetTemplateChildT<winrt::FxScrollViewer>(c_pipsPagerScrollViewerName, *this));

    m_defaultPipSize = GetDesiredPipSize(DefaultIndicatorButtonStyle());
    m_selectedPipSize = GetDesiredPipSize(SelectedIndicatorButtonStyle());
    OnOrientationChanged();
    OnMaxVisualIndicatorsChanged();
    OnNumberOfPagesChanged();
    OnSelectedPageIndexChanged(m_lastSelectedPageIndex);
}

winrt::Size PipsPager::GetDesiredPipSize(const winrt::Style& style) {
    if (auto const repeater = m_pipsPagerRepeater.get())
    {
        if (auto const itemTemplate = repeater.ItemTemplate().try_as<winrt::DataTemplate>())
        {
            if (auto const element = itemTemplate.LoadContent().try_as<winrt::FrameworkElement>())
            {
                element.Style(style);
                element.Measure({ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() });
                return element.DesiredSize();
            }
        }
    }
    // TODO: Extract default sizes and return
    // Find a better way? not sure if this is a good solution
    /*auto pipHeight = unbox_value<double>(ResourceAccessor::ResourceLookup(*this, box_value(c_pipsPagerPipButtonHeightPropertyName)));
    auto pipWidth = unbox_value<double>(ResourceAccessor::ResourceLookup(*this, box_value(c_pipsPagerPipButtonWidthPropertyName)));
    return { static_cast<float>(pipWidth), static_cast<float>(pipHeight)}; */
    return { 0.0, 0.0 };
}

void PipsPager::OnKeyDown(const winrt::KeyRoutedEventArgs& args) {
    winrt::FocusNavigationDirection previousPipDirection;
    winrt::FocusNavigationDirection nextPipDirection;
    if (Orientation() == winrt::Orientation::Vertical)
    {
        previousPipDirection = winrt::FocusNavigationDirection::Up;
        nextPipDirection = winrt::FocusNavigationDirection::Down;
    }
    else
    {
        previousPipDirection = winrt::FocusNavigationDirection::Left;
        nextPipDirection = winrt::FocusNavigationDirection::Right;
    }

    if (args.Key() == winrt::VirtualKey::Left || args.Key() == winrt::VirtualKey::Up)
    {
        winrt::FocusManager::TryMoveFocus(previousPipDirection);
        args.Handled(true);
    }
    else if (args.Key() == winrt::VirtualKey::Right || args.Key() == winrt::VirtualKey::Down)
    {
        winrt::FocusManager::TryMoveFocus(nextPipDirection);
        args.Handled(true);
    }
    // Call for all other presses
    __super::OnKeyDown(args);
}

void PipsPager::OnPointerEntered(const winrt::PointerRoutedEventArgs& args) {
    __super::OnPointerEntered(args);
    m_isPointerOver = true;
    UpdateNavigationButtonVisualStates();
}
void PipsPager::OnPointerExited(const winrt::PointerRoutedEventArgs& args) {
    __super::OnPointerExited(args);
    // We can get a spurious Exited and then Entered if the button
    // that is being clicked on hides itself. In order to avoid switching
    // visual states in this case, we check if the pointer is over the
    // control bounds when we get the exited event.
    if (IsOutOfControlBounds(args.GetCurrentPoint(*this).Position()))
    {
        m_isPointerOver = false;
        UpdateNavigationButtonVisualStates();
        args.Handled(true);
    }
}

void PipsPager::OnPointerCanceled(const winrt::PointerRoutedEventArgs& args)
{
    __super::OnPointerCanceled(args);
    m_isPointerOver = false;
    UpdateNavigationButtonVisualStates();
}

bool PipsPager::IsOutOfControlBounds(const winrt::Point& point) {
    // This is a conservative check. It is okay to say we are
    // out of the bounds when close to the edge to account for rounding.
    const auto tolerance = 1.0;
    const auto actualWidth = ActualWidth();
    const auto actualHeight = ActualHeight();
    return point.X < tolerance ||
        point.X > actualWidth - tolerance ||
        point.Y < tolerance ||
        point.Y  > actualHeight - tolerance;
}

void PipsPager::UpdateIndividualNavigationButtonVisualState(
    const bool hiddenOnEdgeCondition,
    const ButtonVisibility visibility,
    const wstring_view& visibleStateName,
    const wstring_view& hiddenStateName,
    const wstring_view& enabledStateName,
    const wstring_view& disabledStateName) {

    const auto ifGenerallyVisible = !hiddenOnEdgeCondition && NumberOfPages() > 0 && MaxVisualIndicators() > 0;
    if (visibility != ButtonVisibility::Collapsed)
    {
        if ((visibility == ButtonVisibility::Visible || m_isPointerOver) && ifGenerallyVisible)
        {
            winrt::VisualStateManager::GoToState(*this, visibleStateName, false);
            winrt::VisualStateManager::GoToState(*this, enabledStateName, false);
        }
        else
        {
            if (!ifGenerallyVisible)
            {
                winrt::VisualStateManager::GoToState(*this, disabledStateName, false);
            }
            winrt::VisualStateManager::GoToState(*this, hiddenStateName, false);
        }
    }
}

void PipsPager::OnNavigationButtonVisibilityChanged(const ButtonVisibility visibility, const wstring_view& collapsedStateName) {
    if (visibility == ButtonVisibility::Collapsed)
    {
        winrt::VisualStateManager::GoToState(*this, collapsedStateName, false);
    }
    else
    {
        UpdateNavigationButtonVisualStates();
    }
}

void PipsPager::UpdateNavigationButtonVisualStates() {
    const int selectedPageIndex = SelectedPageIndex();
    const int numberOfPages = NumberOfPages();
    const int maxDisplayedPages = MaxVisualIndicators();

    auto const ifPreviousButtonHiddenOnEdge = selectedPageIndex == 0;
    UpdateIndividualNavigationButtonVisualState(ifPreviousButtonHiddenOnEdge, PreviousButtonVisibility(),
        c_previousPageButtonVisibleVisualState, c_previousPageButtonHiddenVisualState,
        c_previousPageButtonEnabledVisualState, c_previousPageButtonDisabledVisualState);

    auto const ifNextButtonHiddenOnEdge = selectedPageIndex == numberOfPages - 1;
    UpdateIndividualNavigationButtonVisualState(ifNextButtonHiddenOnEdge, NextButtonVisibility(),
        c_nextPageButtonVisibleVisualState, c_nextPageButtonHiddenVisualState,
        c_nextPageButtonEnabledVisualState, c_nextPageButtonDisabledVisualState);
}

void PipsPager::OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (auto const element = args.Element())
    {
        if (const auto pip = element.try_as<winrt::Button>())
        {
            auto const index = args.Index();
            if (index != SelectedPageIndex())
            {
                pip.Style(DefaultIndicatorButtonStyle());
            }

            // Narrator says: Page 5, Button 5 of 30. Is it expected behavior?
            winrt::AutomationProperties::SetName(pip, ResourceAccessor::GetLocalizedStringResource(SR_PipsPagerPageText) + L" " + winrt::to_hstring(index + 1));
            winrt::AutomationProperties::SetPositionInSet(pip, index + 1);
            winrt::AutomationProperties::SetSizeOfSet(pip, NumberOfPages());

            auto pciRevokers = winrt::make_self<PipsPagerViewItemRevokers>();
            pciRevokers->clickRevoker = pip.Click(winrt::auto_revoke,
                [this, index](auto const& sender, auto const& args)
                {
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

void PipsPager::ScrollToCenterOfViewport(const winrt::UIElement sender)
{
    winrt::BringIntoViewOptions options;
    options.VerticalAlignmentRatio(0.5);
    options.HorizontalAlignmentRatio(0.5);
    options.AnimationDesired(true);
    sender.StartBringIntoView(options);
}


void PipsPager::OnMaxVisualIndicatorsChanged()
{
    if (MaxVisualIndicators() >= 0)
    {
        SetScrollViewerMaxSize();
        UpdateSelectedPip(SelectedPageIndex());
        UpdateNavigationButtonVisualStates();
    }
    else
    {
        // TODO: Modify logic that it doesn't set it to 0
        MaxVisualIndicators(0);
    }
}

void PipsPager::OnNumberOfPagesChanged()
{
    const int numberOfPages = NumberOfPages();
    const int selectedPageIndex = SelectedPageIndex();
    UpdatePipsItems(numberOfPages, MaxVisualIndicators());
    SetScrollViewerMaxSize();
    if (SelectedPageIndex() > numberOfPages - 1 && numberOfPages > -1)
    {
        SelectedPageIndex(numberOfPages - 1);
    }
    else
    {
        UpdateSelectedPip(selectedPageIndex);
        UpdateNavigationButtonVisualStates();
    }
}

void PipsPager::OnSelectedPageIndexChanged(const int oldValue)
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
        if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::PipsPagerAutomationPeer>())
        {
            winrt::get_self<PipsPagerAutomationPeer>(peer)->RaiseSelectionChanged(m_lastSelectedPageIndex, SelectedPageIndex());
        }

        UpdateSelectedPip(SelectedPageIndex());
        UpdateNavigationButtonVisualStates();
        RaiseSelectedIndexChanged();
    }
}

void PipsPager::RaiseSelectedIndexChanged()
{
    const auto args = winrt::make_self<PipsPagerSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
}

void PipsPager::UpdateSelectedPip(const int index) {
    if (NumberOfPages() != 0)
    {
        if (const auto repeater = m_pipsPagerRepeater.get())
        {
            repeater.UpdateLayout();
            if (const auto element = repeater.TryGetElement(m_lastSelectedPageIndex).try_as<winrt::Button>())
            {
                element.Style(DefaultIndicatorButtonStyle());
            }
            if (const auto element = repeater.GetOrCreateElement(index).try_as<winrt::Button>())
            {
                element.Style(SelectedIndicatorButtonStyle());
                ScrollToCenterOfViewport(element);
            }
        }
    }
}

double PipsPager::CalculateScrollViewerSize(const double defaultPipSize, const double selectedPipSize, const int numberOfPages) {
    if (numberOfPages > 0)
    {
        return defaultPipSize * (numberOfPages - 1) + selectedPipSize;
    }
    return 0;
}

void PipsPager::SetScrollViewerMaxSize() {
    if (const auto scrollViewer = m_pipsPagerScrollViewer.get())
    {
        const auto numberOfPages = NumberOfPages() < 0 ? MaxVisualIndicators() : std::min(NumberOfPages(), MaxVisualIndicators());
        if (Orientation() == winrt::Orientation::Horizontal)
        {
            const auto scrollViewerWidth = CalculateScrollViewerSize(m_defaultPipSize.Width, m_selectedPipSize.Width, numberOfPages);
            scrollViewer.MaxWidth(scrollViewerWidth);
            scrollViewer.MaxHeight(std::max(m_defaultPipSize.Height, m_selectedPipSize.Height));
        }
        else
        {
            const auto scrollViewerHeight = CalculateScrollViewerSize(m_defaultPipSize.Height, m_selectedPipSize.Height, numberOfPages);
            scrollViewer.MaxHeight(scrollViewerHeight);
            scrollViewer.MaxWidth(std::max(m_defaultPipSize.Width, m_selectedPipSize.Width));
        }
    }
}

void PipsPager::UpdatePipsItems(const int numberOfPages, const int maxDisplayedPages) {
    auto const pipsListSize = int(m_pipsPagerItems.Size());
    if (numberOfPages == 0)
    {
        m_pipsPagerItems.Clear();
    }
    // TODO: Add infinite behaviour here
    // Waiting for Behaviour clarification from Gabby
    else if (pipsListSize < numberOfPages)
    {
        for (int i = pipsListSize; i < numberOfPages; i++)
        {
            m_pipsPagerItems.Append(winrt::box_value(i + 1));
        }
    }
    else {
        for (int i = numberOfPages; i < pipsListSize; i++)
        {
            m_pipsPagerItems.RemoveAtEnd();
        }
    }
}


void PipsPager::OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always greater than 0.
    SelectedPageIndex(SelectedPageIndex() - 1);
}

void PipsPager::OnNextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always less than maximum.
    SelectedPageIndex(SelectedPageIndex() + 1);
}


void PipsPager::OnOrientationChanged() {
    // TODO: Fix increase number of pages animation
    if (Orientation() == winrt::Orientation::Horizontal) {
        winrt::VisualStateManager::GoToState(*this, L"HorizontalOrientationView", false);
    }
    else {
        winrt::VisualStateManager::GoToState(*this, L"VerticalOrientationView", false);
    }
    SetScrollViewerMaxSize();
    UpdateSelectedPip(SelectedPageIndex());

}
void PipsPager::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
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
        else if (property == MaxVisualIndicatorsProperty()) {
            OnMaxVisualIndicatorsChanged();
        }
        else if (property == PreviousButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(PreviousButtonVisibility(), c_previousPageButtonCollapsedVisualState);
        }
        else if (property == NextButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(NextButtonVisibility(), c_nextPageButtonCollapsedVisualState);
        }
        else if (property == DefaultIndicatorButtonStyleProperty())
        {
            m_defaultPipSize = GetDesiredPipSize(DefaultIndicatorButtonStyle());
            SetScrollViewerMaxSize();
            // TODO: Do we need that?
            // UpdateSelectedPip(SelectedPageIndex());
        }
        else if (property == SelectedIndicatorButtonStyleProperty())
        {
            m_selectedPipSize = GetDesiredPipSize(SelectedIndicatorButtonStyle());
            SetScrollViewerMaxSize();
            // TODO: Do we need that?
            // UpdateSelectedPip(SelectedPageIndex());
        }
        else if (property == OrientationProperty())
        {
            OnOrientationChanged();
        }
    }
}

winrt::AutomationPeer PipsPager::OnCreateAutomationPeer()
{
    return winrt::make<PipsPagerAutomationPeer>(*this);
}
