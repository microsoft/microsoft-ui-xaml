#pragma once
#include <Vector.h>
#include "NavigationViewItem.h"
#include "TopNavigationViewDataProvider.h"

class NavigationViewModel
{
public:
	NavigationViewModel(const ITrackerHandleManager* m_owner);
	~NavigationViewModel();

    void SetNavigationViewParent(winrt::NavigationView const& navigationView);

	void ToggleIsExpanded(winrt::IInspectable const& item);
	void IsExpandedPropertyChanged(winrt::NavigationViewItem const& sender, winrt::DependencyPropertyChangedEventArgs const& args);
	//void InsertAt(uint32_t index, winrt::IInspectable const& value);
	//winrt::IVector<winrt::IInspectable> GetActiveListView();

    winrt::IObservableVector<winrt::IInspectable> GetPrimaryItems();

    void SetDataSource(winrt::IInspectable rawDataSource);

    void RegisterItemExpandEventToSelf(winrt::NavigationViewItem const& item, winrt::NavigationViewList const& list);

    winrt::ListView GetActiveListView();

    // For TopNavigationView
	//void setTopNavigationViewDataProvider(const TopNavigationViewDataProvider& topNavigationViewDataProvider);

private:
    winrt::weak_ref<winrt::NavigationView> m_navigationView{ nullptr };

    // Vectors keeping track of displayed items
	winrt::IObservableVector<winrt::IInspectable> m_primaryItems{ nullptr };
	winrt::IObservableVector<winrt::IInspectable> m_popupItems{ nullptr };

    // Event Tokens
    std::vector<winrt::event_token> m_isExpandedChangedEventTokenVector;

    void UpdatePrimaryItems();
    void NavigationViewModel::InsertAt(uint32_t index, winrt::IInspectable const& value);
    void NavigationViewModel::RemoveItemAndDescendantsFromView(const winrt::NavigationViewItemBase& value, const int index);

    tracker_ref<winrt::ItemsSourceView> m_dataSource;
    // If the raw datasource is the same, we don't need to create new winrt::ItemsSourceView object.
    tracker_ref<winrt::IInspectable> m_rawDataSource;
    bool ShouldChangeDataSource(winrt::IInspectable const& rawData);
    void ChangeDataSource(winrt::ItemsSourceView newValue);


    // For TopNavigationView
	//winrt::weak_ref<TopNavigationViewDataProvider> m_topDataProvider{ nullptr };

};
