// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "PagerControl.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include <PagerControlSelectedIndexChangedEventArgs.h>

const winrt::hstring c_NumberBoxVisibleVisualState = L"NumberBoxVisible";
const winrt::hstring c_ComboBoxVisibleVisualState = L"ComboBoxVisible";
const winrt::hstring c_NumberPanelVisibleVisualState = L"NumberPanelVisible";

const winrt::hstring c_FirstPageButtonVisibleVisualState = L"FirstPageButtonVisible";
const winrt::hstring c_FirstPageButtonNotVisibleVisualState = L"FirstPageButtonCollapsed";
const winrt::hstring c_FirstPageButtonEnabledVisualState = L"FirstPageButtonEnabled";
const winrt::hstring c_FirstPageButtonDisabledVisualState = L"FirstPageButtonDisabled";

const winrt::hstring c_PreviousPageButtonVisibleVisualState = L"PreviousPageButtonVisible";
const winrt::hstring c_PreviousPageButtonNotVisibleVisualState = L"PreviousPageButtonCollapsed";
const winrt::hstring c_PreviousPageButtonEnabledVisualState = L"PreviousPageButtonEnabled";
const winrt::hstring c_PreviousPageButtonDisabledVisualState = L"PreviousPageButtonDisabled";

const winrt::hstring c_NextPageButtonVisibleVisualState = L"NextPageButtonVisible";
const winrt::hstring c_NextPageButtonNotVisibleVisualState = L"NextPageButtonCollapsed";
const winrt::hstring c_NextPageButtonEnabledVisualState = L"NextPageButtonEnabled";
const winrt::hstring c_NextPageButtonDisabledVisualState = L"NextPageButtonDisabled";

const winrt::hstring c_LastPageButtonVisibleVisualState = L"LastPageButtonVisible";
const winrt::hstring c_LastPageButtonNotVisibleVisualState = L"LastPageButtonCollapsed";
const winrt::hstring c_LastPageButtonEnabledVisualState = L"LastPageButtonEnabled";
const winrt::hstring c_LastPageButtonDisabledVisualState = L"LastPageButtonDisabled";

const winrt::hstring c_prefixTextTextblockName = L"PrefixTextLabel";
const winrt::hstring c_suffixTextTextblockName = L"SuffixTextLabel";

const winrt::hstring c_comboBoxName = L"ComboBoxDisplay";

const winrt::hstring c_firstPageButtonName = L"FirstPageButton";
const winrt::hstring c_previousPageButtonName = L"PreviousPageButton";
const winrt::hstring c_nextPageButtonName = L"NextPageButton";
const winrt::hstring c_lastPageButtonName = L"LastPageButton";

const int c_AutoDisplayModeNumberOfPagesThreshold = 10; // This integer determines when to switch between NumberBoxDisplayMode and ComboBoxDisplayMode 


PagerControl::PagerControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PagerControl);

    SetDefaultStyleKey(this);
}

PagerControl::~PagerControl()
{
    m_firstPageButtonClickRevoker.revoke();
    m_previousPageButtonClickRevoker.revoke();
    m_nextPageButtonClickRevoker.revoke();
    m_lastPageButtonClickRevoker.revoke();
}

void PagerControl::OnApplyTemplate()
{
    if (const auto prefixTextBlock = GetTemplateChildT<winrt::TextBlock>(c_prefixTextTextblockName,*this))
    {
        prefixTextBlock.Text(ResourceAccessor::GetLocalizedStringResource(SR_PagerControlPrefixTextName));
    }
    if (const auto suffixTextBlock = GetTemplateChildT<winrt::TextBlock>(c_prefixTextTextblockName, *this))
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

    //if (PagerComboBox != null)
    //{
    //    PagerComboBox.SelectedIndex = SelectedIndex - 1;
    //    PagerComboBox.SelectionChanged += (s, e) = > { OnComboBoxSelectionChanged(); };
    //}
    //if (PagerNumberPanel != null)
    //{
    //    UpdateNumberPanel();
    //    PagerNumberPanel.ElementPrepared += OnElementPrepared;
    //    PagerNumberPanel.ElementClearing += OnElementClearing;
    //}

    // This is for the initial loading of the control
    RaiseSelectedIndexChanged();
    OnDisplayModeChanged();
    UpdateOnEdgeButtonVisualStates();
}


void  PagerControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    if (this->Template() == nullptr)
    {
        return;
    }
    else if (property == FirstButtonVisibilityProperty())
    {
        OnButtonVisibilityChanged(FirstButtonVisibility(),
            c_FirstPageButtonVisibleVisualState,
            c_FirstPageButtonNotVisibleVisualState,
            1);
    }
    else if (property == PreviousButtonVisibilityProperty())
    {
        OnButtonVisibilityChanged(PreviousButtonVisibility(),
            c_PreviousPageButtonVisibleVisualState,
            c_PreviousPageButtonNotVisibleVisualState,
            1);
    }
    else if (property == NextButtonVisibilityProperty())
    {
        OnButtonVisibilityChanged(NextButtonVisibility(),
            c_NextPageButtonVisibleVisualState,
            c_NextPageButtonNotVisibleVisualState,
            NumberOfPages());
    }
    else if (property == LastButtonVisibilityProperty())
    {
        OnButtonVisibilityChanged(PreviousButtonVisibility(),
            c_LastPageButtonVisibleVisualState,
            c_LastPageButtonNotVisibleVisualState,
            NumberOfPages());
    }
    else if (property == DisplayModeProperty())
    {
        OnDisplayModeChanged();
    }
    else if (property == NumberOfPagesProperty())
    {
        //OnNumberOfPagesChanged();
        if (DisplayMode() == winrt::PagerControlDisplayMode::Auto)
        {
            UpdateDisplayModeAutoState();
        }
        UpdateOnEdgeButtonVisualStates();
    }
    else if (property == SelectedPageIndexProperty())
    {
        OnSelectedIndexChanged(winrt::unbox_value<int>(args.OldValue()) - 1);
        UpdateOnEdgeButtonVisualStates();
    }
}

void PagerControl::OnDisplayModeChanged()
{
    // Cache for performance reasons
    const auto displayMode = DisplayMode();

    if (displayMode == winrt::PagerControlDisplayMode::ButtonPanel)
    {
        winrt::VisualStateManager::GoToState(*this, c_NumberPanelVisibleVisualState, false);
    }
    else if (displayMode == winrt::PagerControlDisplayMode::ComboBox)
    {
        winrt::VisualStateManager::GoToState(*this, c_ComboBoxVisibleVisualState, false);
    }
    else if (displayMode == winrt::PagerControlDisplayMode::NumberBox)
    {
        winrt::VisualStateManager::GoToState(*this, c_NumberBoxVisibleVisualState, false);
    }
    else
    {
        UpdateDisplayModeAutoState();
    }
}

void PagerControl::UpdateDisplayModeAutoState()
{
    winrt::VisualStateManager::GoToState(*this, NumberOfPages() < c_AutoDisplayModeNumberOfPagesThreshold ?
        c_ComboBoxVisibleVisualState : c_NumberBoxVisibleVisualState, false);
}

void PagerControl::OnSelectedIndexChanged(const int oldIndex)
{
    m_lastSelectedPageIndex = oldIndex;

    UpdateOnEdgeButtonVisualStates();

    RaiseSelectedIndexChanged();
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

void PagerControl::UpdateOnEdgeButtonVisualStates()
{
    // Cache those values as we need them often and accessing a DP is (relatively) expensive
    const int selectedPageIndex = SelectedPageIndex();
    const int numberOfPages = NumberOfPages();

    // Handle disabled/enabled status of buttons
    if (selectedPageIndex == 1)
    {
        winrt::VisualStateManager::GoToState(*this, c_FirstPageButtonDisabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_PreviousPageButtonDisabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_NextPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_LastPageButtonEnabledVisualState, false);
    }
    else if (selectedPageIndex == numberOfPages)
    {
        winrt::VisualStateManager::GoToState(*this, c_FirstPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_PreviousPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_NextPageButtonDisabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_LastPageButtonDisabledVisualState, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, c_FirstPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_PreviousPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_NextPageButtonEnabledVisualState, false);
        winrt::VisualStateManager::GoToState(*this, c_LastPageButtonEnabledVisualState, false);
    }

    // Handle HiddenOnEdge states
    if (FirstButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != 1)
        {
            winrt::VisualStateManager::GoToState(*this, c_FirstPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_FirstPageButtonNotVisibleVisualState, false);
        }
    }
    if (PreviousButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != 1)
        {
            winrt::VisualStateManager::GoToState(*this, c_PreviousPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_PreviousPageButtonNotVisibleVisualState, false);
        }
    }
    if (NextButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != numberOfPages)
        {
            winrt::VisualStateManager::GoToState(*this, c_NextPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_NextPageButtonNotVisibleVisualState, false);
        }
    }
    if (LastButtonVisibility() == winrt::PagerControlButtonVisibility::HiddenOnEdge)
    {
        if (selectedPageIndex != numberOfPages)
        {
            winrt::VisualStateManager::GoToState(*this, c_LastPageButtonVisibleVisualState, false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, c_LastPageButtonNotVisibleVisualState, false);
        }
    }
}

void PagerControl::FirstButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e)
{
    SelectedPageIndex(1);
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
    SelectedPageIndex(NumberOfPages());
    if (const auto command = LastButtonCommand())
    {
        command.Execute(nullptr);
    }
}

void PagerControl::RaiseSelectedIndexChanged()
{
    auto args = winrt::make_self<PagerControlSelectedIndexChangedEventArgs>(m_lastSelectedPageIndex, SelectedPageIndex());
    m_selectedIndexChangedEventSource(*this, *args);
}
