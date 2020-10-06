// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "PagerControl.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "PagerControlSelectedIndexChangedEventArgs.h"
#include "PagerControlTemplateSettings.h"
#include <PagerControlAutomationPeer.h>

constexpr auto c_numberBoxVisibleVisualState = L"NumberBoxVisible"sv;
constexpr auto c_comboBoxVisibleVisualState = L"ComboBoxVisible"sv;
constexpr auto c_numberPanelVisibleVisualState = L"NumberPanelVisible"sv;

constexpr auto c_firstPageButtonVisibleVisualState = L"FirstPageButtonVisible"sv;
constexpr auto c_firstPageButtonNotVisibleVisualState = L"FirstPageButtonCollapsed"sv;
constexpr auto c_firstPageButtonHiddenVisualState = L"FirstPageButtonHidden"sv;
constexpr auto c_firstPageButtonEnabledVisualState = L"FirstPageButtonEnabled"sv;
constexpr auto c_firstPageButtonDisabledVisualState = L"FirstPageButtonDisabled"sv;

constexpr auto c_previousPageButtonVisibleVisualState = L"PreviousPageButtonVisible"sv;
constexpr auto c_previousPageButtonNotVisibleVisualState = L"PreviousPageButtonCollapsed"sv;
constexpr auto c_previousPageButtonHiddenVisualState = L"PreviousPageButtonHidden"sv;
constexpr auto c_previousPageButtonEnabledVisualState = L"PreviousPageButtonEnabled"sv;
constexpr auto c_previousPageButtonDisabledVisualState = L"PreviousPageButtonDisabled"sv;

constexpr auto c_nextPageButtonVisibleVisualState = L"NextPageButtonVisible"sv;
constexpr auto c_nextPageButtonNotVisibleVisualState = L"NextPageButtonCollapsed"sv;
constexpr auto c_nextPageButtonHiddenVisualState = L"NextPageButtonHidden"sv;
constexpr auto c_nextPageButtonEnabledVisualState = L"NextPageButtonEnabled"sv;
constexpr auto c_nextPageButtonDisabledVisualState = L"NextPageButtonDisabled"sv;

constexpr auto c_lastPageButtonVisibleVisualState = L"LastPageButtonVisible"sv;
constexpr auto c_lastPageButtonNotVisibleVisualState = L"LastPageButtonCollapsed"sv;
constexpr auto c_lastPageButtonHiddenVisualState = L"LastPageButtonHidden"sv;
constexpr auto c_lastPageButtonEnabledVisualState = L"LastPageButtonEnabled"sv;
constexpr auto c_lastPageButtonDisabledVisualState = L"LastPageButtonDisabled"sv;

constexpr auto c_finiteItemsModeState = L"FiniteItems"sv;
constexpr auto c_infiniteItemsModeState = L"InfiniteItems"sv;

constexpr auto c_rootGridName = L"RootGrid"sv;
constexpr auto c_comboBoxName = L"ComboBoxDisplay"sv;
constexpr auto c_numberBoxName = L"NumberBoxDisplay"sv;

constexpr auto c_numberPanelRepeaterName = L"NumberPanelItemsRepeater"sv;
constexpr auto c_numberPanelIndicatorName = L"NumberPanelCurrentPageIndicator"sv;
constexpr auto c_firstPageButtonName = L"FirstPageButton"sv;
constexpr auto c_previousPageButtonName = L"PreviousPageButton"sv;
constexpr auto c_nextPageButtonName = L"NextPageButton"sv;
constexpr auto c_lastPageButtonName = L"LastPageButton"sv;

static constexpr auto c_numberPanelButtonStyleName = L"PagerControlNumberPanelButtonStyle"sv; 
const int c_AutoDisplayModeNumberOfPagesThreshold = 10; // This integer determines when to switch between NumberBoxDisplayMode and ComboBoxDisplayMode 
const int c_infiniteModeComboBoxItemsIncrement = 100;

/* Common functions */
PagerControl::PagerControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PagerControl);

    m_comboBoxEntries = winrt::make<Vector<IInspectable>>().as<winrt::IObservableVector<IInspectable>>();
    m_numberPanelElements = winrt::make<Vector<IInspectable>>().as<winrt::IObservableVector<IInspectable>>();

    const auto templateSettings = winrt::make<PagerControlTemplateSettings>();
    templateSettings.SetValue(PagerControlTemplateSettingsProperties::s_PagesProperty, m_comboBoxEntries);
    templateSettings.SetValue(PagerControlTemplateSettingsProperties::s_NumberPanelItemsProperty, m_numberPanelElements);
    SetValue(s_TemplateSettingsProperty, templateSettings);

    SetDefaultStyleKey(this);
}

PagerControl::~PagerControl()
{
    m_rootGridKeyDownRevoker.revoke();
    m_comboBoxSelectionChangedRevoker.revoke();
    m_firstPageButtonClickRevoker.revoke();
    m_previousPageButtonClickRevoker.revoke();
    m_nextPageButtonClickRevoker.revoke();
    m_lastPageButtonClickRevoker.revoke();
}

void PagerControl::OnApplyTemplate()
{
    if (PrefixText().empty())
    {
        PrefixText(ResourceAccessor::GetLocalizedStringResource(SR_PagerControlPrefixTextName));
    }
    if (SuffixText().empty())
    {
        SuffixText(ResourceAccessor::GetLocalizedStringResource(SR_PagerControlSuffixTextName));
    }
    if (const auto rootGrid = GetTemplateChildT<winrt::FrameworkElement>(c_rootGridName, *this))
    {
        m_rootGridKeyDownRevoker = rootGrid.KeyDown(winrt::auto_revoke, { this, &PagerControl::OnRootGridKeyDown });
    }

    if (const auto firstPageButton = GetTemplateChildT<winrt::Button>(c_firstPageButtonName, *this))
    {
        winrt::AutomationProperties::SetName(firstPageButton, ResourceAccessor::GetLocalizedStringResource(SR_PagerControlFirstPageButtonTextName));
        m_firstPageButtonClickRevoker = firstPageButton.Click(winrt::auto_revoke, { this, &PagerControl::FirstButtonClicked });
    }
    if (const auto previousPageButton = GetTemplateChildT<winrt::Button>(c_previousPageButtonName, *this))
    {
        winrt::AutomationProperties::SetName(previousPageButton, ResourceAccessor::GetLocalizedStringResource(SR_PagerControlPreviousPageButtonTextName));
        m_previousPageButtonClickRevoker = previousPageButton.Click(winrt::auto_revoke, { this, &PagerControl::PreviousButtonClicked });
    }
    if (const auto nextPageButton = GetTemplateChildT<winrt::Button>(c_nextPageButtonName, *this))
    {
        winrt::AutomationProperties::SetName(nextPageButton, ResourceAccessor::GetLocalizedStringResource(SR_PagerControlNextPageButtonTextName));
        m_nextPageButtonClickRevoker = nextPageButton.Click(winrt::auto_revoke, { this, &PagerControl::NextButtonClicked });
    }
    if (const auto lastPageButton = GetTemplateChildT<winrt::Button>(c_lastPageButtonName, *this))
    {
        winrt::AutomationProperties::SetName(lastPageButton, ResourceAccessor::GetLocalizedStringResource(SR_PagerControlLastPageButtonTextName));
        m_lastPageButtonClickRevoker = lastPageButton.Click(winrt::auto_revoke, { this, &PagerControl::LastButtonClicked });
    }

    m_comboBoxSelectionChangedRevoker.revoke();
    [this](const winrt::ComboBox comboBox) {
        m_comboBox.set(comboBox);
        if (comboBox)
        {
            comboBox.SelectedIndex(SelectedPageIndex() - 1);
            winrt::AutomationProperties::SetName(comboBox, ResourceAccessor::GetLocalizedStringResource(SR_PagerControlPageTextName));
            m_comboBoxSelectionChangedRevoker = comboBox.SelectionChanged(winrt::auto_revoke, { this, &PagerControl::ComboBoxSelectionChanged });
        }
    }(GetTemplateChildT<winrt::ComboBox>(c_comboBoxName, *this));

    m_numberBoxValueChangedRevoker.revoke();
    [this](const winrt::NumberBox numberBox) {
        m_numberBox.set(numberBox);
        if (numberBox)
        {
            numberBox.Value(SelectedPageIndex() + 1);
            winrt::AutomationProperties::SetName(numberBox, ResourceAccessor::GetLocalizedStringResource(SR_PagerControlPageTextName));
            m_numberBoxValueChangedRevoker = numberBox.ValueChanged(winrt::auto_revoke, { this,&PagerControl::NumberBoxValueChanged });
        }
    }(GetTemplateChildT<winrt::NumberBox>(c_numberBoxName, *this));

    m_numberPanelRepeater.set(GetTemplateChildT<winrt::ItemsRepeater>(c_numberPanelRepeaterName, *this));
    m_selectedPageIndicator.set(GetTemplateChildT<winrt::FrameworkElement>(c_numberPanelIndicatorName, *this));

    // This is for the initial loading of the control
    OnDisplayModeChanged();
    UpdateOnEdgeButtonVisualStates();
    OnNumberOfPagesChanged(0);
    OnSelectedPageIndexChange(-1);
}

void PagerControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (this->Template() != nullptr)
    {
        if (property == FirstButtonVisibilityProperty())
        {
            OnButtonVisibilityChanged(FirstButtonVisibility(),
                c_firstPageButtonVisibleVisualState,
                c_firstPageButtonNotVisibleVisualState,
                c_firstPageButtonHiddenVisualState,
                0);
        }
        else if (property == PreviousButtonVisibilityProperty())
        {
            OnButtonVisibilityChanged(PreviousButtonVisibility(),
                c_previousPageButtonVisibleVisualState,
                c_previousPageButtonNotVisibleVisualState,
                c_previousPageButtonHiddenVisualState,
                0);
        }
        else if (property == NextButtonVisibilityProperty())
        {
            OnButtonVisibilityChanged(NextButtonVisibility(),
                c_nextPageButtonVisibleVisualState,
                c_nextPageButtonNotVisibleVisualState,
                c_nextPageButtonHiddenVisualState,
                NumberOfPages() - 1);
        }
        else if (property == LastButtonVisibilityProperty())
        {
            OnButtonVisibilityChanged(LastButtonVisibility(),
                c_lastPageButtonVisibleVisualState,
                c_lastPageButtonNotVisibleVisualState,
                c_lastPageButtonHiddenVisualState,
                NumberOfPages() - 1);
        }
        else if (property == DisplayModeProperty())
        {
            OnDisplayModeChanged();
            // Why are we calling this you might ask.
            // The reason is that that method only updates what it currently needs to update.
            // So when we switch to ComboBox from NumberPanel, the NumberPanel element list might be out of date.
            UpdateTemplateSettingElementLists();
        }
        else if (property == NumberOfPagesProperty())
        {
            OnNumberOfPagesChanged(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == SelectedPageIndexProperty())
        {
            OnSelectedPageIndexChange(winrt::unbox_value<int>(args.OldValue()));
        }
        else if (property == ButtonPanelAlwaysShowFirstLastPageIndexProperty())
        {
            UpdateNumberPanel(NumberOfPages());
        }
    }
}

winrt::AutomationPeer PagerControl::OnCreateAutomationPeer()
{
    return winrt::make<PagerControlAutomationPeer>(*this);
}

/* Property changed handlers */
void PagerControl::OnDisplayModeChanged()
{
    // Cache for performance reasons
    const auto displayMode = DisplayMode();

    if (displayMode == winrt::PagerControlDisplayMode::ButtonPanel)
    {
        winrt::VisualStateManager::GoToState(*this, c_numberPanelVisibleVisualState, false);
    }
    else if (displayMode == winrt::PagerControlDisplayMode::ComboBox)
    {
        winrt::VisualStateManager::GoToState(*this, c_comboBoxVisibleVisualState, false);
    }
    else if (displayMode == winrt::PagerControlDisplayMode::NumberBox)
    {
        winrt::VisualStateManager::GoToState(*this, c_numberBoxVisibleVisualState, false);
    }
    else
    {
        UpdateDisplayModeAutoState();
    }
}

void PagerControl::UpdateDisplayModeAutoState()
{
    const int numberOfPages = NumberOfPages();
    if (numberOfPages > -1)
    {
        winrt::VisualStateManager::GoToState(*this, numberOfPages < c_AutoDisplayModeNumberOfPagesThreshold ?
            c_comboBoxVisibleVisualState : c_numberBoxVisibleVisualState, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, c_numberBoxVisibleVisualState, false);
    }
}

void PagerControl::OnNumberOfPagesChanged(const int oldValue)
{
    m_lastNumberOfPagesCount = oldValue;
    const int numberOfPages = NumberOfPages();
    if (numberOfPages < SelectedPageIndex() && numberOfPages > -1)
    {
        SelectedPageIndex(numberOfPages - 1);
    }
    UpdateTemplateSettingElementLists();
    if (DisplayMode() == winrt::PagerControlDisplayMode::Auto)
    {
        UpdateDisplayModeAutoState();
    }
    if (numberOfPages > -1)
    {
        winrt::VisualStateManager::GoToState(*this, c_finiteItemsModeState, false);
        if (const auto numberBox = m_numberBox.get())
        {
            numberBox.Maximum(numberOfPages);
        }
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, c_infiniteItemsModeState, false);
        if (const auto numberBox = m_numberBox.get())
        {
            numberBox.Maximum(INFINITY);
        }
    }
    UpdateOnEdgeButtonVisualStates();
}

void PagerControl::OnSelectedPageIndexChange(const int oldValue)
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

    if (const auto comboBox = m_comboBox.get())
    {
        if (SelectedPageIndex() < (int32_t)m_comboBoxEntries.Size())
        {
            comboBox.SelectedIndex(SelectedPageIndex());
        }
    }
    if (const auto numBox = m_numberBox.get())
    {
        numBox.Value(SelectedPageIndex() + 1);
    }

    UpdateOnEdgeButtonVisualStates();
    UpdateTemplateSettingElementLists();

    if (DisplayMode() == winrt::PagerControlDisplayMode::ButtonPanel)
    {
        // NumberPanel needs to update based on multiple parameters.
        // SelectedPageIndex is one of them, so let's do that now!
        UpdateNumberPanel(NumberOfPages());
    }

    // Fire value property change for UIA
    if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::PagerControlAutomationPeer>())
    {
        winrt::get_self<PagerControlAutomationPeer>(peer)->RaiseSelectionChanged(m_lastSelectedPageIndex, SelectedPageIndex());
    }
    RaiseSelectedIndexChanged();
}

void PagerControl::RaiseSelectedIndexChanged()
{
    const auto args = winrt::make_self<PagerControlSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
}

void PagerControl::OnButtonVisibilityChanged(const winrt::PagerControlButtonVisibility visibility,
    const wstring_view visibleStateName,
    const wstring_view collapsedStateName,
    const wstring_view hiddenStateName,
    const int hiddenOnEdgePageCriteria)
{
    if (visibility == winrt::PagerControlButtonVisibility::Visible)
    {
        winrt::VisualStateManager::GoToState(*this, visibleStateName, false);
    }
    else if (visibility == winrt::PagerControlButtonVisibility::Hidden)
    {
        winrt::VisualStateManager::GoToState(*this, collapsedStateName, false);
    }
    else
    {
        if (SelectedPageIndex() != hiddenOnEdgePageCriteria)
        {
            winrt::VisualStateManager::GoToState(*this, visibleStateName, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, hiddenStateName, false);
        }
    }
}

void PagerControl::UpdateTemplateSettingElementLists()
{
    // Cache values for performance :)
    const auto displayMode = DisplayMode();
    const auto numberOfPages = NumberOfPages();

    if (displayMode == winrt::PagerControlDisplayMode::ComboBox ||
        displayMode == winrt::PagerControlDisplayMode::Auto)
    {
        if (numberOfPages > -1)
        {
            FillComboBoxCollectionToSize(numberOfPages);
        }
        else
        {
            if (m_comboBoxEntries.Size() < c_infiniteModeComboBoxItemsIncrement)
            {
                FillComboBoxCollectionToSize(c_infiniteModeComboBoxItemsIncrement);
            }
        }
    }
    else if (displayMode == winrt::PagerControlDisplayMode::ButtonPanel)
    {
        UpdateNumberPanel(numberOfPages);
    }
}

void PagerControl::FillComboBoxCollectionToSize(const int numberOfPages)
{
    const int currentComboBoxItemsCount = (int32_t)m_comboBoxEntries.Size();
    if (currentComboBoxItemsCount <= numberOfPages)
    {
        // We are increasing the number of pages, so add the missing numbers.
        for (int i = currentComboBoxItemsCount; i < numberOfPages; i++)
        {
            m_comboBoxEntries.Append(winrt::box_value(i + 1));
        }
    }
    else
    {
        // We are decreasing the number of pages, so remove numbers starting at the end.
        for (int i = currentComboBoxItemsCount; i > numberOfPages; i--)
        {
            m_comboBoxEntries.RemoveAtEnd();
        }
    }
}

void PagerControl::UpdateOnEdgeButtonVisualStates()
{
    // Cache those values as we need them often and accessing a DP is (relatively) expensive
    const int selectedPageIndex = SelectedPageIndex();
    const int numberOfPages = NumberOfPages();

    // Handle disabled/enabled status of buttons
    if (selectedPageIndex == 0)
    {
        winrt::VisualStateManager::GoToState(*this, c_firstPageButtonDisabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_previousPageButtonDisabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_nextPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_lastPageButtonEnabledVisualState, false);
    }
    else if (selectedPageIndex >= numberOfPages - 1)
    {
        winrt::VisualStateManager::GoToState(*this, c_firstPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_previousPageButtonEnabledVisualState, false);
        if (numberOfPages > -1)
        {
            winrt::VisualStateManager::GoToState(*this, c_nextPageButtonDisabledVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_nextPageButtonEnabledVisualState, false);
        }
        winrt::VisualStateManager::GoToState(*this, c_lastPageButtonDisabledVisualState, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, c_firstPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_previousPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_nextPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_lastPageButtonEnabledVisualState, false);
    }

    // Handle HiddenOnEdge states
    if (FirstButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != 0)
        {
            winrt::VisualStateManager::GoToState(*this, c_firstPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_firstPageButtonHiddenVisualState, false);
        }
    }
    if (PreviousButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != 0)
        {
            winrt::VisualStateManager::GoToState(*this, c_previousPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_previousPageButtonHiddenVisualState, false);
        }
    }
    if (NextButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != numberOfPages - 1)
        {
            winrt::VisualStateManager::GoToState(*this, c_nextPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_nextPageButtonHiddenVisualState, false);
        }
    }
    if (LastButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != numberOfPages - 1)
        {
            winrt::VisualStateManager::GoToState(*this, c_lastPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_lastPageButtonHiddenVisualState, false);
        }
    }
}

/* NumberPanel logic */
void PagerControl::UpdateNumberPanel(const int numberOfPages)
{
    if (numberOfPages < 0)
    {
        UpdateNumberOfPanelCollectionInfiniteItems();
    }
    else if (numberOfPages < 8)
    {
        UpdateNumberPanelCollectionAllItems(numberOfPages);
    }
    else
    {
        const auto selectedIndex = SelectedPageIndex();
        // Idea: Choose correct "template" based on SelectedPageIndex (x) and NumberOfPages (n)
        // 1 2 3 4 5 6 ... n <-- Items
        if (selectedIndex < 4)
        {
            // First four items selected, create following pattern:
            // 1 2 3 4 5... n
            UpdateNumberPanelCollectionStartWithEllipsis(numberOfPages, selectedIndex);
        }
        else if (selectedIndex >= numberOfPages - 4)
        {
            // Last four items selected, create following pattern:
            //1 [...] n-4 n-3 n-2 n-1 n
            UpdateNumberPanelCollectionEndWithEllipsis(numberOfPages, selectedIndex);
        }
        else
        {
            // Neither start or end, so lets do this:
            // 1 [...] x-2 x-1 x x+1 x+2 [...] n
            // where x > 4 and x < n - 4
            UpdateNumberPanelCollectionCenterWithEllipsis(numberOfPages, selectedIndex);
        }
    }
}

void PagerControl::UpdateNumberOfPanelCollectionInfiniteItems()
{
    const int selectedIndex = SelectedPageIndex();

    m_numberPanelElements.Clear();
    if (selectedIndex < 3)
    {
        AppendButtonToNumberPanelList(1, 0);
        AppendButtonToNumberPanelList(2, 0);
        AppendButtonToNumberPanelList(3, 0);
        AppendButtonToNumberPanelList(4, 0);
        AppendButtonToNumberPanelList(5, 0);
        MoveIdentifierToElement(selectedIndex);
    }
    else
    {
        AppendButtonToNumberPanelList(1, 0);
        AppendEllipsisIconToNumberPanelList();
        AppendButtonToNumberPanelList(selectedIndex, 0);
        AppendButtonToNumberPanelList(selectedIndex + 1, 0);
        AppendButtonToNumberPanelList(selectedIndex + 2, 0);
        MoveIdentifierToElement(3);
    }
}

void PagerControl::UpdateNumberPanelCollectionAllItems(int numberOfPages)
{
    // Check that the NumberOfPages did not change, so we can skip recreating collection
    if (m_lastNumberOfPagesCount != numberOfPages)
    {
        m_numberPanelElements.Clear();
        for (int i = 0; i < numberOfPages && i < 7; i++)
        {
            AppendButtonToNumberPanelList(i + 1, numberOfPages);
        }
    }
    MoveIdentifierToElement(SelectedPageIndex());
}

void PagerControl::UpdateNumberPanelCollectionStartWithEllipsis(int numberOfPages, int selectedIndex)
{
    if (m_lastNumberOfPagesCount != numberOfPages)
    {
        // Do a recalculation as the number of pages changed.
        m_numberPanelElements.Clear();
        AppendButtonToNumberPanelList(1, numberOfPages);
        AppendButtonToNumberPanelList(2, numberOfPages);
        AppendButtonToNumberPanelList(3, numberOfPages);
        AppendButtonToNumberPanelList(4, numberOfPages);
        AppendButtonToNumberPanelList(5, numberOfPages);
        if (ButtonPanelAlwaysShowFirstLastPageIndex())
        {
            AppendEllipsisIconToNumberPanelList();
            AppendButtonToNumberPanelList(numberOfPages, numberOfPages);
        }
    }
    // SelectedIndex is definitely the correct index here as we are counting from start
    MoveIdentifierToElement(selectedIndex);
}

void PagerControl::UpdateNumberPanelCollectionEndWithEllipsis(int numberOfPages, int selectedIndex)
{
    if (m_lastNumberOfPagesCount != numberOfPages)
    {
        // Do a recalculation as the number of pages changed.
        m_numberPanelElements.Clear();
        if (ButtonPanelAlwaysShowFirstLastPageIndex())
        {
            AppendButtonToNumberPanelList(1, numberOfPages);
            AppendEllipsisIconToNumberPanelList();
        }
        AppendButtonToNumberPanelList(numberOfPages - 4, numberOfPages);
        AppendButtonToNumberPanelList(numberOfPages - 3, numberOfPages);
        AppendButtonToNumberPanelList(numberOfPages - 2, numberOfPages);
        AppendButtonToNumberPanelList(numberOfPages - 1, numberOfPages);
        AppendButtonToNumberPanelList(numberOfPages, numberOfPages);
    }
    // We can only be either the last, the second from last or third from last
    // => SelectedIndex = NumberOfPages - y with y in {1,2,3}
    if (ButtonPanelAlwaysShowFirstLastPageIndex())
    {
        // Last item sits at index 4.
        // SelectedPageIndex for the last page is NumberOfPages - 1.
        // => SelectedItem = SelectedIndex - NumberOfPages + 7;
        MoveIdentifierToElement(selectedIndex - numberOfPages + 7);
    }
    else
    {
        // Last item sits at index 6.
        // SelectedPageIndex for the last page is NumberOfPages - 1.
        // => SelectedItem = SelectedIndex - NumberOfPages + 5;
        MoveIdentifierToElement(selectedIndex - numberOfPages + 5);
    }
}

void PagerControl::UpdateNumberPanelCollectionCenterWithEllipsis(int numberOfPages, int selectedIndex)
{
    const auto showFirstLastPageIndex = ButtonPanelAlwaysShowFirstLastPageIndex();
    if (m_lastNumberOfPagesCount != numberOfPages)
    {
        // Do a recalculation as the number of pages changed.
        m_numberPanelElements.Clear();
        if (showFirstLastPageIndex)
        {
            AppendButtonToNumberPanelList(1, numberOfPages);
            AppendEllipsisIconToNumberPanelList();
        }
        AppendButtonToNumberPanelList(selectedIndex, numberOfPages);
        AppendButtonToNumberPanelList(selectedIndex + 1, numberOfPages);
        AppendButtonToNumberPanelList(selectedIndex + 2, numberOfPages);
        if (showFirstLastPageIndex)
        {
            AppendEllipsisIconToNumberPanelList();
            AppendButtonToNumberPanelList(numberOfPages, numberOfPages);
        }
    }
    // "selectedIndex + 1" is our representation for SelectedIndex.
    if (showFirstLastPageIndex)
    {
        // SelectedIndex + 1 is the fifth element.
        // Collections are base zero, so the index to underline is 3.
        MoveIdentifierToElement(3);
    }
    else
    {
        // SelectedIndex + 1 is the third element.
        // Collections are base zero, so the index to underline is 1.
        MoveIdentifierToElement(1);
    }
}

void PagerControl::MoveIdentifierToElement(int index)
{
    if (const auto indicator = m_selectedPageIndicator.get())
    {
        if (const auto repeater = m_numberPanelRepeater.get())
        {
            repeater.UpdateLayout();
            if (const auto element = repeater.TryGetElement(index).try_as<winrt::FrameworkElement>())
            {
                const auto boundingRect = element.TransformToVisual(repeater).TransformBounds(winrt::Rect::Rect(0, 0, (float)repeater.ActualWidth(), (float)repeater.ActualHeight()));
                winrt::Thickness newMargin;
                newMargin.Left = boundingRect.X;
                newMargin.Top = 0;
                newMargin.Right = 0;
                newMargin.Bottom = 0;
                indicator.Margin(newMargin);
                indicator.Width(element.ActualWidth());
            }
        }
    }
}

void PagerControl::AppendButtonToNumberPanelList(const int pageNumber, const int numberOfPages)
{
    winrt::Button button;
    button.Content(winrt::box_value(pageNumber));
    const auto buttonClickedFunc = [this](auto const& sender, auto const& args) {
        if (const auto button = sender.try_as<winrt::Button>())
        {
            const int unboxedValue = winrt::unbox_value<int>(button.Content());
            SelectedPageIndex(unboxedValue - 1);
        }
    };
    button.Click(buttonClickedFunc);
    // Set the default style of buttons
    button.Style(unbox_value<winrt::Style>(ResourceAccessor::ResourceLookup(*this, box_value(c_numberPanelButtonStyleName))));
    winrt::AutomationProperties::SetName(button, ResourceAccessor::GetLocalizedStringResource(SR_PagerControlPageTextName) + L" " + winrt::to_hstring(pageNumber));
    winrt::AutomationProperties::SetPositionInSet(button, pageNumber);
    winrt::AutomationProperties::SetSizeOfSet(button, numberOfPages);
    m_numberPanelElements.Append(button);
}

void PagerControl::AppendEllipsisIconToNumberPanelList()
{
    winrt::SymbolIcon ellipsisIcon;
    ellipsisIcon.Symbol(winrt::Symbol::More);
    m_numberPanelElements.Append(ellipsisIcon);
}

/* Interaction event listeners */
void PagerControl::OnRootGridKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Left || args.Key() == winrt::VirtualKey::GamepadDPadLeft)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Left);
    }
    else if (args.Key() == winrt::VirtualKey::Right || args.Key() == winrt::VirtualKey::GamepadDPadRight)
    {
        winrt::FocusManager::TryMoveFocus(winrt::FocusNavigationDirection::Right);
    }
}

void PagerControl::ComboBoxSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args)
{
    if (const auto comboBox = m_comboBox.get())
    {
        SelectedPageIndex(comboBox.SelectedIndex());
    }
}

void PagerControl::NumberBoxValueChanged(const winrt::IInspectable& sender, const winrt::NumberBoxValueChangedEventArgs& args)
{
    SelectedPageIndex((int)(args.NewValue()) - 1);
}

void PagerControl::FirstButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    SelectedPageIndex(0);
    if (const auto command = FirstButtonCommand())
    {
        command.Execute(nullptr);
    }
}

void PagerControl::PreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always greater than 1.
    SelectedPageIndex(SelectedPageIndex() - 1);
    if (const auto command = PreviousButtonCommand())
    {
        command.Execute(nullptr);
    }
}

void PagerControl::NextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    // In this method, SelectedPageIndex is always less than maximum.
    SelectedPageIndex(SelectedPageIndex() + 1);
    if (const auto command = NextButtonCommand())
    {
        command.Execute(nullptr);
    }
}

void PagerControl::LastButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    SelectedPageIndex(NumberOfPages() - 1);
    if (const auto command = LastButtonCommand())
    {
        command.Execute(nullptr);
    }
}
