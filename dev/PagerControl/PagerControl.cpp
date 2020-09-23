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

const winrt::hstring c_numberBoxVisibleVisualState = L"NumberBoxVisible";
const winrt::hstring c_comboBoxVisibleVisualState = L"ComboBoxVisible";
const winrt::hstring c_numberPanelVisibleVisualState = L"NumberPanelVisible";

const winrt::hstring c_firstPageButtonVisibleVisualState = L"FirstPageButtonVisible";
const winrt::hstring c_firstPageButtonNotVisibleVisualState = L"FirstPageButtonCollapsed";
const winrt::hstring c_firstPageButtonEnabledVisualState = L"FirstPageButtonEnabled";
const winrt::hstring c_firstPageButtonDisabledVisualState = L"FirstPageButtonDisabled";

const winrt::hstring c_previousPageButtonVisibleVisualState = L"PreviousPageButtonVisible";
const winrt::hstring c_previousPageButtonNotVisibleVisualState = L"PreviousPageButtonCollapsed";
const winrt::hstring c_previousPageButtonEnabledVisualState = L"PreviousPageButtonEnabled";
const winrt::hstring c_previousPageButtonDisabledVisualState = L"PreviousPageButtonDisabled";

const winrt::hstring c_nextPageButtonVisibleVisualState = L"NextPageButtonVisible";
const winrt::hstring c_nextPageButtonNotVisibleVisualState = L"NextPageButtonCollapsed";
const winrt::hstring c_nextPageButtonEnabledVisualState = L"NextPageButtonEnabled";
const winrt::hstring c_nextPageButtonDisabledVisualState = L"NextPageButtonDisabled";

const winrt::hstring c_lastPageButtonVisibleVisualState = L"LastPageButtonVisible";
const winrt::hstring c_lastPageButtonNotVisibleVisualState = L"LastPageButtonCollapsed";
const winrt::hstring c_lastPageButtonEnabledVisualState = L"LastPageButtonEnabled";
const winrt::hstring c_lastPageButtonDisabledVisualState = L"LastPageButtonDisabled";

const winrt::hstring c_rootGridName = L"RootGrid";
const winrt::hstring c_prefixTextTextblockName = L"PrefixTextLabel";
const winrt::hstring c_suffixTextTextblockName = L"SuffixTextLabel";

const winrt::hstring c_comboBoxName = L"ComboBoxDisplay";
const winrt::hstring c_numberBoxName = L"NumberBoxDisplay";
const winrt::hstring c_numberPanelRepeaterName = L"NumberPanelItemsRepeater";
const winrt::hstring c_numberPanelIndicatorName = L"NumberPanelCurrentPageIndicator";
const winrt::hstring c_firstPageButtonName = L"FirstPageButton";
const winrt::hstring c_previousPageButtonName = L"PreviousPageButton";
const winrt::hstring c_nextPageButtonName = L"NextPageButton";
const winrt::hstring c_lastPageButtonName = L"LastPageButton";

const int c_AutoDisplayModeNumberOfPagesThreshold = 10; // This integer determines when to switch between NumberBoxDisplayMode and ComboBoxDisplayMode 

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
    if (const auto rootGrid = GetTemplateChildT<winrt::FrameworkElement>(c_rootGridName, *this))
    {
        m_rootGridKeyDownRevoker = rootGrid.KeyDown(winrt::auto_revoke, { this, &PagerControl::OnRootGridKeyDown });
    }
    if (const auto prefixTextBlock = GetTemplateChildT<winrt::TextBlock>(c_prefixTextTextblockName,*this))
    {
        prefixTextBlock.Text(ResourceAccessor::GetLocalizedStringResource(SR_PagerControlPrefixTextName));
    }
    if (const auto suffixTextBlock = GetTemplateChildT<winrt::TextBlock>(c_suffixTextTextblockName, *this))
    {
        suffixTextBlock.Text(ResourceAccessor::GetLocalizedStringResource(SR_PagerControlSuffixTextName));
    }

    if (const auto firstPageButton = GetTemplateChildT<winrt::Button>(c_firstPageButtonName, *this))
    {
        m_firstPageButtonClickRevoker = firstPageButton.Click(winrt::auto_revoke, { this, &PagerControl::FirstButtonClicked });
    }
    if (const auto previousPageButton = GetTemplateChildT<winrt::Button>(c_previousPageButtonName, *this))
    {
        m_previousPageButtonClickRevoker = previousPageButton.Click(winrt::auto_revoke, { this, &PagerControl::PreviousButtonClicked });
    }
    if (const auto nextPageButton = GetTemplateChildT<winrt::Button>(c_nextPageButtonName, *this))
    {
        m_nextPageButtonClickRevoker = nextPageButton.Click(winrt::auto_revoke, { this, &PagerControl::NextButtonClicked });
    }
    if (const auto lastPageButton = GetTemplateChildT<winrt::Button>(c_lastPageButtonName, *this))
    {
        m_lastPageButtonClickRevoker = lastPageButton.Click(winrt::auto_revoke, { this, &PagerControl::LastButtonClicked });
    }

    if (const auto comboBox = GetTemplateChildT<winrt::ComboBox>(c_comboBoxName,*this))
    {
        m_comboBox.set(comboBox);
        comboBox.SelectedIndex(SelectedPageIndex() - 1);
        m_comboBoxSelectionChangedRevoker = comboBox.SelectionChanged(winrt::auto_revoke, { this, &PagerControl::ComboBoxSelectionChanged });
    }
    if (const auto numberBox = GetTemplateChildT<winrt::NumberBox>(c_numberBoxName,*this))
    {
        m_numberBox.set(numberBox);
        numberBox.Value(SelectedPageIndex() + 1);
        m_numberBoxValueChangedRevoker = numberBox.ValueChanged(winrt::auto_revoke, { this,&PagerControl::NumberBoxValueChanged });
    }
    if (const auto repeater = GetTemplateChildT<winrt::ItemsRepeater>(c_numberPanelRepeaterName, *this))
    {
        m_numberPanelRepeater.set(repeater);
    }
    if (const auto numberPanelIndicator = GetTemplateChildT<winrt::FrameworkElement>(c_numberPanelIndicatorName,*this))
    {
        m_selectedPageIndicator.set(numberPanelIndicator);
    }

    // This is for the initial loading of the control
    RaiseSelectedIndexChanged();
    OnDisplayModeChanged();
    UpdateOnEdgeButtonVisualStates();
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
                0);
        }
        else if (property == PreviousButtonVisibilityProperty())
        {
            OnButtonVisibilityChanged(PreviousButtonVisibility(),
                c_previousPageButtonVisibleVisualState,
                c_previousPageButtonNotVisibleVisualState,
                0);
        }
        else if (property == NextButtonVisibilityProperty())
        {
            OnButtonVisibilityChanged(NextButtonVisibility(),
                c_nextPageButtonVisibleVisualState,
                c_nextPageButtonNotVisibleVisualState,
                NumberOfPages() - 1);
        }
        else if (property == LastButtonVisibilityProperty())
        {
            OnButtonVisibilityChanged(PreviousButtonVisibility(),
                c_lastPageButtonVisibleVisualState,
                c_lastPageButtonNotVisibleVisualState,
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
            m_lastNumberOfPagesCount = winrt::unbox_value<int>(args.OldValue());
            if (NumberOfPages() < SelectedPageIndex())
            {
                SelectedPageIndex(NumberOfPages() - 1);
            }
            UpdateTemplateSettingElementLists();
            if (DisplayMode() == winrt::PagerControlDisplayMode::Auto)
            {
                UpdateDisplayModeAutoState();
            }
            UpdateOnEdgeButtonVisualStates();
        }
        else if (property == SelectedPageIndexProperty())
        {
            // If we don't have any pages, there is nothing we should do.
            if (NumberOfPages() != 0)
            {
                // Ensure that SelectedPageIndex will end up in the valid range of values
                if (SelectedPageIndex() > NumberOfPages() - 1)
                {
                    SelectedPageIndex(NumberOfPages() - 1);
                }
                else if (SelectedPageIndex() < 0)
                {
                    SelectedPageIndex(0);
                }

                // Now handle the value changes
                OnSelectedIndexChanged(winrt::unbox_value<int>(args.OldValue()));
                if (DisplayMode() == winrt::PagerControlDisplayMode::ButtonPanel)
                {
                    // NumberPanel needs to update based on multiple parameters.
                    // SelectedPageIndex is one of them, so let's do that now!
                    UpdateNumberPanel(NumberOfPages());
                }
                UpdateOnEdgeButtonVisualStates();

                // Fire value property change for UIA
                if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::PagerControlAutomationPeer>())
                {
                    winrt::get_self<PagerControlAutomationPeer>(peer)->RaiseSelectionChanged(m_lastSelectedPageIndex, SelectedPageIndex());
                }
            }
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
    winrt::VisualStateManager::GoToState(*this, NumberOfPages() < c_AutoDisplayModeNumberOfPagesThreshold ?
        c_comboBoxVisibleVisualState : c_numberBoxVisibleVisualState, false);
}

void PagerControl::OnSelectedIndexChanged(const int oldIndex)
{
    m_lastSelectedPageIndex = oldIndex;

    if (const auto comboBox = m_comboBox.get())
    {
        comboBox.SelectedIndex(SelectedPageIndex());
    }

    if (const auto numBox = m_numberBox.get())
    {
        numBox.Value(SelectedPageIndex() + 1);
    }

    UpdateOnEdgeButtonVisualStates();

    RaiseSelectedIndexChanged();
}

void PagerControl::RaiseSelectedIndexChanged()
{
    auto args = winrt::make_self<PagerControlSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
}

void PagerControl::OnButtonVisibilityChanged(const winrt::PagerControlButtonVisibility visibility,
    const winrt::hstring visibleStateName,
    const winrt::hstring hiddenStateName,
    const int hiddenOnEdgePageCriteria)
{
    if (visibility == winrt::PagerControlButtonVisibility::Visible)
    {
        winrt::VisualStateManager::GoToState(*this, visibleStateName, false);
    }
    else if (visibility == winrt::PagerControlButtonVisibility::Hidden)
    {
        winrt::VisualStateManager::GoToState(*this, hiddenStateName, false);
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
        const int currenComboBoxItemsCount = (int32_t)m_comboBoxEntries.Size();
        if (currenComboBoxItemsCount <= numberOfPages)
        {
            // We are increasing the number of pages, so add the missing numbers.
            for (int i = currenComboBoxItemsCount; i < numberOfPages; i++)
            {
                m_comboBoxEntries.Append(winrt::box_value(i + 1));
            }
        }
        else
        {
            // We are decreasing the number of pages, so remove numbers starting at the end.
            for (int i = currenComboBoxItemsCount; i > numberOfPages; i--)
            {
                m_comboBoxEntries.RemoveAt(i - 1);
            }
        }
    }
    else if (displayMode == winrt::PagerControlDisplayMode::ButtonPanel)
    {
        UpdateNumberPanel(numberOfPages);
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
    else if (selectedPageIndex == numberOfPages - 1)
    {
        winrt::VisualStateManager::GoToState(*this, c_firstPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_previousPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_nextPageButtonDisabledVisualState, false);
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
            winrt::VisualStateManager::GoToState(*this, c_firstPageButtonNotVisibleVisualState, false);
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
            winrt::VisualStateManager::GoToState(*this, c_previousPageButtonNotVisibleVisualState, false);
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
            winrt::VisualStateManager::GoToState(*this, c_nextPageButtonNotVisibleVisualState, false);
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
            winrt::VisualStateManager::GoToState(*this, c_lastPageButtonNotVisibleVisualState, false);
        }
    }
}

/* NumberPanel logic */
void PagerControl::UpdateNumberPanel(const int numberOfPages)
{
    if (numberOfPages < 7)
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
            // First two items selected, create following pattern:
            // 1 2 3 4 5... n
            UpdateNumberPanelCollectionStartWithEllipsis(numberOfPages, selectedIndex);
        }
        else if (selectedIndex >= numberOfPages - 4)
        {
            // Last two items selected, create following pattern:
            //1 [...] n-4 n-3 n-2 n-1 n
            UpdateNumberPanelCollectionEndWithEllipsis(numberOfPages, selectedIndex);
        }
        else
        {
            // Neither start or end, so lets do this:
            // 1 [...] x-2 x-1 x x+1 x+2 [...] n
            // where x-2 > 1 and x+2 < n
            UpdateNumberPanelCollectionCenterWithEllipsis(numberOfPages, selectedIndex);
        }
    }
}

void PagerControl::UpdateNumberPanelCollectionAllItems(int numberOfPages)
{
    // Check that the NumberOfPages did not change, so we can skip recreating collection
    if (m_lastNumberOfPagesCount != numberOfPages)
    {
        m_numberPanelElements.Clear();
        for (int i = 0; i < numberOfPages && i < 6; i++)
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
            AppendButtonToNumberPanelList(1,numberOfPages);
            AppendEllipsisIconToNumberPanelList();
        }
        AppendButtonToNumberPanelList(selectedIndex - 1, numberOfPages);
        AppendButtonToNumberPanelList(selectedIndex, numberOfPages);
        AppendButtonToNumberPanelList(selectedIndex + 1, numberOfPages);
        AppendButtonToNumberPanelList(selectedIndex + 2, numberOfPages);
        AppendButtonToNumberPanelList(selectedIndex + 3, numberOfPages);
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
        // Collections are base zero, so the index to underline is 4.
        MoveIdentifierToElement(4);
    }
    else
    {
        // SelectedIndex + 1 is the third element.
        // Collections are base zero, so the index to underline is 2.
        MoveIdentifierToElement(2);
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
    winrt::AutomationProperties::SetPositionInSet(button, pageNumber);
    winrt::AutomationProperties::SetSizeOfSet(button, pageNumber);
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
