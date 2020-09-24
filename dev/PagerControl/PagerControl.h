// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "PagerControl.g.h"
#include "PagerControl.properties.h"

class PagerControl :
    public ReferenceTracker<PagerControl, winrt::implementation::PagerControlT>,
    public PagerControlProperties
{

public:
    /* Common functions */
    PagerControl();
    ~PagerControl();

    /* IFrameworkElement */
    void OnApplyTemplate();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    winrt::AutomationPeer OnCreateAutomationPeer();
    
    /* Property changed handlers */
    void OnDisplayModeChanged();
    void UpdateDisplayModeAutoState();
    void OnNumberOfPagesChanged(const int oldValue);
    void OnSelectedPageIndexChange(const int oldValue);
    void OnButtonVisibilityChanged(const winrt::PagerControlButtonVisibility visibility,
        const winrt::hstring visibleStateName,
        const winrt::hstring hiddenStateName,
        const int hiddenOnEdgeSelectedIndexCriteria);

private:

    /* UI updating */
    void UpdateTemplateSettingElementLists();
    void FillComboBoxCollectionToSize(const int numberOfPages);
    void UpdateOnEdgeButtonVisualStates();

    /* Eventing */
    void RaiseSelectedIndexChanged();

    /* NumberPanel logic */
    void UpdateNumberPanel(const int numberOfPages);
    void UpdateNumberOfPanelCollectionInfiniteItems();
    void UpdateNumberPanelCollectionAllItems(const int numberOfPages);
    void UpdateNumberPanelCollectionStartWithEllipsis(int numberOfPages, int selectedIndex);
    void UpdateNumberPanelCollectionEndWithEllipsis(int numberOfPages, int selectedIndex);
    void UpdateNumberPanelCollectionCenterWithEllipsis(int numberOfPages, int selectedIndex);
    void MoveIdentifierToElement(int index);
    void AppendButtonToNumberPanelList(const int pageNumber, const int numberOfPages);
    void AppendEllipsisIconToNumberPanelList();

    /* Interaction event listeners */
    void OnRootGridKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void NumberBoxValueChanged(const winrt::IInspectable& sender, const winrt::NumberBoxValueChangedEventArgs& args);
    void ComboBoxSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args);
    void FirstButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void PreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void NextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void LastButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);


    int m_lastSelectedPageIndex = -1;
    int m_lastNumberOfPagesCount = 0;

    tracker_ref<winrt::ComboBox> m_comboBox{ this };
    tracker_ref<winrt::NumberBox> m_numberBox{ this };
    tracker_ref<winrt::ItemsRepeater> m_numberPanelRepeater{ this };
    tracker_ref<winrt::FrameworkElement> m_selectedPageIndicator{ this };

    winrt::FrameworkElement::KeyDown_revoker m_rootGridKeyDownRevoker{};
    winrt::ComboBox::SelectionChanged_revoker m_comboBoxSelectionChangedRevoker{};
    winrt::NumberBox::ValueChanged_revoker m_numberBoxValueChangedRevoker{};
    winrt::Button::Click_revoker m_firstPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_previousPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_nextPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_lastPageButtonClickRevoker{};

    winrt::IObservableVector<IInspectable> m_comboBoxEntries{};
    winrt::IObservableVector<IInspectable> m_numberPanelElements{};

};

