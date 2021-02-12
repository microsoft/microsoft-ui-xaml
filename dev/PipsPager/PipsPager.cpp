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

constexpr auto c_previousPageButtonName = L"PreviousPageButton"sv;
constexpr auto c_nextPageButtonName = L"NextPageButton"sv;

constexpr auto c_pipsPagerRepeaterName = L"PipsPagerItemsRepeater"sv;
constexpr auto c_pipsPagerScrollViewerName = L"PipsPagerScrollViewer"sv;

constexpr auto c_pipsPagerButtonWidthPropertyName = L"PipsPagerButtonWidth"sv;
constexpr auto c_pipsPagerButtonHeightPropertyName = L"PipsPagerButtonHeight"sv;

constexpr auto c_pipsPagerHorizontalOrientationVisualState = L"HorizontalOrientationView"sv;
constexpr auto c_pipsPagerVerticalOrientationVisualState = L"VerticalOrientationView"sv;

PipsPager::PipsPager()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PipsPager);
    m_pipsPagerItems = winrt::make<Vector<winrt::IInspectable>>().as<winrt::IObservableVector<winrt::IInspectable>>();
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

    m_defaultPipSize = GetDesiredPipSize(NormalPipStyle());
    m_selectedPipSize = GetDesiredPipSize(SelectedPipStyle());
    OnNavigationButtonVisibilityChanged(PreviousButtonVisibility(), c_previousPageButtonCollapsedVisualState, c_previousPageButtonDisabledVisualState);
    OnNavigationButtonVisibilityChanged(NextButtonVisibility(), c_nextPageButtonCollapsedVisualState, c_nextPageButtonDisabledVisualState);
    UpdatePipsItems(NumberOfPages(), MaxVisiblePips());
    OnOrientationChanged();
    OnSelectedPageIndexChanged(m_lastSelectedPageIndex);
}

void PipsPager::RaiseSelectedIndexChanged()
{
    const auto args = winrt::make_self<PipsPagerSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
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
    /* Extract default sizes and return in case the code above fails */
    auto pipHeight = unbox_value<double>(ResourceAccessor::ResourceLookup(*this, box_value(c_pipsPagerButtonHeightPropertyName)));
    auto pipWidth = unbox_value<double>(ResourceAccessor::ResourceLookup(*this, box_value(c_pipsPagerButtonWidthPropertyName)));
    return { static_cast<float>(pipWidth), static_cast<float>(pipHeight) };
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
    // We can get a spurious Exited and then Entered if the button
    // that is being clicked on hides itself. In order to avoid switching
    // visual states in this case, we check if the pointer is over the
    // control bounds when we get the exited event.
    if (IsOutOfControlBounds(args.GetCurrentPoint(*this).Position()))
    {
        m_isPointerOver = false;
        UpdateNavigationButtonVisualStates();
    }
    else
    {
    args.Handled(true);
    }
    __super::OnPointerExited(args);
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

    const auto ifGenerallyVisible = !hiddenOnEdgeCondition && NumberOfPages() != 0 && MaxVisiblePips() > 0;
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
            else
            {
                winrt::VisualStateManager::GoToState(*this, enabledStateName, false);
            }
            winrt::VisualStateManager::GoToState(*this, hiddenStateName, false);
        }
    }
}

void PipsPager::UpdateNavigationButtonVisualStates() {
    const int selectedPageIndex = SelectedPageIndex();
    const int numberOfPages = NumberOfPages();

    auto const ifPreviousButtonHiddenOnEdge = selectedPageIndex == 0;
    UpdateIndividualNavigationButtonVisualState(ifPreviousButtonHiddenOnEdge, PreviousButtonVisibility(),
        c_previousPageButtonVisibleVisualState, c_previousPageButtonHiddenVisualState,
        c_previousPageButtonEnabledVisualState, c_previousPageButtonDisabledVisualState);

    auto const ifNextButtonHiddenOnEdge = selectedPageIndex == numberOfPages - 1;
    UpdateIndividualNavigationButtonVisualState(ifNextButtonHiddenOnEdge, NextButtonVisibility(),
        c_nextPageButtonVisibleVisualState, c_nextPageButtonHiddenVisualState,
        c_nextPageButtonEnabledVisualState, c_nextPageButtonDisabledVisualState);
}

void PipsPager::ScrollToCenterOfViewport(const winrt::UIElement sender, const int index)
{
    /* Vertical and Horizontal AligmentsRatio are not available until Win Version 1803 (sdk version 17134) */
    if (SharedHelpers::IsBringIntoViewOptionsVerticalAlignmentRatioAvailable())
    {
        winrt::BringIntoViewOptions options;
        if (Orientation() == winrt::Orientation::Horizontal)
        {
            options.HorizontalAlignmentRatio(0.5);
        }
        else
        {
            options.VerticalAlignmentRatio(0.5);
        }
        options.AnimationDesired(true);
        sender.StartBringIntoView(options);
    }
    else if (const auto scrollViewer = m_pipsPagerScrollViewer.get())
    {
        double pipSize;
        std::function<void (const double&)> changeViewFunc;
        if (Orientation() == winrt::Orientation::Horizontal)
        {
            pipSize = m_defaultPipSize.Width;
            changeViewFunc = [&](const double& offset) {scrollViewer.ChangeView(offset, nullptr, nullptr);};
        }
        else
        {
            pipSize = m_defaultPipSize.Height;
            changeViewFunc = [&](const double& offset) {scrollViewer.ChangeView(nullptr, offset, nullptr);};
        }
        const int maxVisualIndicators = MaxVisiblePips();
        /* This line makes sure that while having even # of indicators the scrolling will be done correctly */
        const int offSetChangeForEvenSizeWindow = maxVisualIndicators % 2 == 0 && index > m_lastSelectedPageIndex ? 1 : 0;
        const int offSetNumOfElements = index + offSetChangeForEvenSizeWindow - maxVisualIndicators / 2;
        const double offset = std::max(0.0, offSetNumOfElements * pipSize);
        changeViewFunc(offset);
    }
}

void PipsPager::UpdateSelectedPip(const int index) {
    if (NumberOfPages() != 0 && MaxVisiblePips() > 0)
    {
        if (const auto repeater = m_pipsPagerRepeater.get())
        {
            repeater.UpdateLayout();
            if (const auto element = repeater.TryGetElement(m_lastSelectedPageIndex).try_as<winrt::Button>())
            {
                element.Style(NormalPipStyle());
            }
            if (const auto element = repeater.GetOrCreateElement(index).try_as<winrt::Button>())
            {
                element.Style(SelectedPipStyle());
                ScrollToCenterOfViewport(element, index);
            }
        }
    }
}

double PipsPager::CalculateScrollViewerSize(const double defaultPipSize, const double selectedPipSize, const int numberOfPages, int maxVisualIndicators) {

    auto numberOfPagesToDisplay = 0;
    maxVisualIndicators = std::max(0, maxVisualIndicators);
    if (maxVisualIndicators == 0 || numberOfPages == 0) {
        return 0;
    }
    else if (numberOfPages > 0)
    {
        numberOfPagesToDisplay = std::min(maxVisualIndicators, numberOfPages);
    }
    else
    {
        numberOfPagesToDisplay = maxVisualIndicators;
    }
    return defaultPipSize * (numberOfPagesToDisplay - 1) + selectedPipSize;
}

void PipsPager::SetScrollViewerMaxSize() {
    if (const auto scrollViewer = m_pipsPagerScrollViewer.get())
    {
        if (Orientation() == winrt::Orientation::Horizontal)
        {
            const auto scrollViewerWidth = CalculateScrollViewerSize(m_defaultPipSize.Width, m_selectedPipSize.Width, NumberOfPages(), MaxVisiblePips());
            scrollViewer.MaxWidth(scrollViewerWidth);
            scrollViewer.MaxHeight(std::max(m_defaultPipSize.Height, m_selectedPipSize.Height));
        }
        else
        {
            const auto scrollViewerHeight = CalculateScrollViewerSize(m_defaultPipSize.Height, m_selectedPipSize.Height, NumberOfPages(), MaxVisiblePips());
            scrollViewer.MaxHeight(scrollViewerHeight);
            scrollViewer.MaxWidth(std::max(m_defaultPipSize.Width, m_selectedPipSize.Width));
        }
    }
}

void PipsPager::UpdatePipsItems(const int numberOfPages, int maxVisualIndicators) {
    auto const pipsListSize = int(m_pipsPagerItems.Size());

    if (numberOfPages == 0 || maxVisualIndicators == 0)
    {
        m_pipsPagerItems.Clear();
    }
    /* Inifinite number of pages case */
    else if (numberOfPages < 0)
    {
        /* Treat negative max visual indicators as 0*/
        auto const minNumberOfElements = std::max(SelectedPageIndex() + 1, std::max(0, maxVisualIndicators));
        if (minNumberOfElements > pipsListSize)
        {
            for (int i = pipsListSize; i < minNumberOfElements; i++)
            {
                m_pipsPagerItems.Append(winrt::box_value(i + 1));
            }
        }
        else if (SelectedPageIndex() == pipsListSize - 1) {
            m_pipsPagerItems.Append(winrt::box_value(pipsListSize + 1));
        }
    }
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

void PipsPager::OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args)
{
    if (auto const element = args.Element())
    {
        if (const auto pip = element.try_as<winrt::Button>())
        {
            auto const index = args.Index();
            if (index != SelectedPageIndex())
            {
                pip.Style(NormalPipStyle());
            }

            // Narrator says: Page 5, Button 5 of 30. Is it expected behavior?
            winrt::AutomationProperties::SetName(pip, ResourceAccessor::GetLocalizedStringResource(SR_PipsPagerPageText) + L" " + winrt::to_hstring(index + 1));
            winrt::AutomationProperties::SetPositionInSet(pip, index + 1);
            winrt::AutomationProperties::SetSizeOfSet(pip, NumberOfPages());

            auto pciRevokers = winrt::make_self<PipsPagerViewItemRevokers>();
            pciRevokers->clickRevoker = pip.Click(winrt::auto_revoke,
                [this, index](auto const& sender, auto const& args)
                {
                    if (const auto repeater = m_pipsPagerRepeater.get()) {
                        if (const auto button = sender.try_as<winrt::Button>())
                        {
                            SelectedPageIndex(repeater.GetElementIndex(button));
                        }
                    }
                }
            );
            pip.SetValue(s_pipButtonHandlersProperty, pciRevokers.as<winrt::IInspectable>());
        }
    }
}

void PipsPager::OnElementIndexChanged(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args)
{
    if (auto const pip = args.Element())
    {
        auto const newIndex = args.NewIndex();
        winrt::AutomationProperties::SetName(pip, ResourceAccessor::GetLocalizedStringResource(SR_PipsPagerPageText) + L" " + winrt::to_hstring(newIndex + 1));
        winrt::AutomationProperties::SetPositionInSet(pip, newIndex + 1);
    }
}

void PipsPager::OnMaxVisiblePipsChanged()
{
    const auto numberOfPages = NumberOfPages();
    if (numberOfPages < 0) {
        UpdatePipsItems(numberOfPages, MaxVisiblePips());
    }
    SetScrollViewerMaxSize();
    UpdateSelectedPip(SelectedPageIndex());
    UpdateNavigationButtonVisualStates();
}

void PipsPager::OnNumberOfPagesChanged()
{
    const int numberOfPages = NumberOfPages();
    const int selectedPageIndex = SelectedPageIndex();
    UpdateSizeOfSetForElements(numberOfPages);
    UpdatePipsItems(numberOfPages, MaxVisiblePips());
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
        if (NumberOfPages() < 0) {
            UpdatePipsItems(NumberOfPages(), MaxVisiblePips());
        }
        UpdateSelectedPip(SelectedPageIndex());
        UpdateNavigationButtonVisualStates();
        RaiseSelectedIndexChanged();
    }
}

void PipsPager::OnOrientationChanged()
{
    if (Orientation() == winrt::Orientation::Horizontal)
    {
        winrt::VisualStateManager::GoToState(*this, c_pipsPagerHorizontalOrientationVisualState, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, c_pipsPagerVerticalOrientationVisualState, false);
    }
    SetScrollViewerMaxSize();
    UpdateSelectedPip(SelectedPageIndex());

}

void PipsPager::OnNavigationButtonVisibilityChanged(const ButtonVisibility visibility, const wstring_view& collapsedStateName, const wstring_view& disabledStateName) {
    if (visibility == ButtonVisibility::Collapsed)
    {
        winrt::VisualStateManager::GoToState(*this, collapsedStateName, false);
        winrt::VisualStateManager::GoToState(*this, disabledStateName, false);
    }
    else
    {
        UpdateNavigationButtonVisualStates();
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
        else if (property == MaxVisiblePipsProperty()) {
            OnMaxVisiblePipsChanged();
        }
        else if (property == PreviousButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(PreviousButtonVisibility(), c_previousPageButtonCollapsedVisualState, c_previousPageButtonDisabledVisualState);
        }
        else if (property == NextButtonVisibilityProperty())
        {
            OnNavigationButtonVisibilityChanged(NextButtonVisibility(), c_nextPageButtonCollapsedVisualState, c_nextPageButtonDisabledVisualState);
        }
        else if (property == NormalPipStyleProperty())
        {
            m_defaultPipSize = GetDesiredPipSize(NormalPipStyle());
            SetScrollViewerMaxSize();
            UpdateSelectedPip(SelectedPageIndex());
        }
        else if (property == SelectedPipStyleProperty())
        {
            m_selectedPipSize = GetDesiredPipSize(SelectedPipStyle());
            SetScrollViewerMaxSize();
            UpdateSelectedPip(SelectedPageIndex());
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

void PipsPager::UpdateSizeOfSetForElements(const int numberOfPages) {
    if(auto const repeater = m_pipsPagerRepeater.get())
    {
        for (int i = 0; i < numberOfPages; i++)
        {
            if (auto const pip = repeater.TryGetElement(i))
            {
                winrt::AutomationProperties::SetSizeOfSet(pip, numberOfPages);
            }
        }
    }
}
