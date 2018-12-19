#include "pch.h"
#include "common.h"
#include "NavigationViewModel.h"
#include "NavigationView.h"

//CppWinRTActivatableClassWithBasicFactory(NavigationViewModel)

NavigationViewModel::NavigationViewModel(const ITrackerHandleManager* m_owner):
    m_rawDataSource(m_owner),
    m_dataSource(m_owner)
{
	// Initialize Menu Vectors
	m_primaryItems = winrt::make<Vector<winrt::IInspectable>>().as<winrt::IObservableVector<winrt::IInspectable>>();
	m_popupItems = winrt::make<Vector<winrt::IInspectable>>().as<winrt::IObservableVector<winrt::IInspectable>>();
}

NavigationViewModel::~NavigationViewModel()
{
	// Unregister from all events
}

void NavigationViewModel::RegisterItemExpandEventToSelf(winrt::NavigationViewItem const& item, winrt::NavigationViewList const& list)
{
    auto navItem = winrt::get_self<NavigationViewItem>(item);
    auto index = list.IndexFromContainer(item);
    m_isExpandedChangedEventTokenVector.insert(m_isExpandedChangedEventTokenVector.begin() + index, navItem->AddExpandedChanged({ this, &NavigationViewModel::IsExpandedPropertyChanged }));

    //navItem->AddExpandedChanged({ this, &NavigationViewModel::IsExpandedPropertyChanged });

    //auto activeListView = GetActiveListView();
    //if (auto navListView = activeListView.try_as<winrt::NavigationViewList>())
    //{
    //    auto index = navListView.IndexFromContainer(item);
    //    m_isExpandedChangedEventTokenVector.insert(m_isExpandedChangedEventTokenVector.begin() + index, navItem->AddExpandedChanged({ this, &NavigationViewModel::IsExpandedPropertyChanged }));
    //}
}

winrt::IObservableVector<winrt::IInspectable> NavigationViewModel::GetPrimaryItems()
{
    return m_primaryItems;
}

void NavigationViewModel::UpdatePrimaryItems()
{
    if (m_dataSource)
    {
        int size = m_dataSource.get().Count();
        for (int i = 0; i < size; i++)
        {
            auto item = m_dataSource.get().GetAt(i);
            m_primaryItems.Append(item);
        }
    }
}

void NavigationViewModel::SetDataSource(winrt::IInspectable rawDataSource)
{
    if (ShouldChangeDataSource(rawDataSource)) // avoid to create multiple of datasource for the same raw data
    {
        winrt::ItemsSourceView dataSource = nullptr;
        if (rawDataSource)
        {
            dataSource = winrt::ItemsSourceView(rawDataSource);
        }
        ChangeDataSource(dataSource);
        m_rawDataSource.set(rawDataSource);

        //TODO:: Upon the creation/update of the datasource object, make sure to empty popup vector
        //if (dataSource)
        //{
        //    MoveAllItemsToPrimaryList();
        //}
    }
}

bool NavigationViewModel::ShouldChangeDataSource(winrt::IInspectable const& rawData)
{
    return rawData != m_rawDataSource.get();
}

void NavigationViewModel::ChangeDataSource(winrt::ItemsSourceView newValue)
{
    auto oldValue = m_dataSource.get();
    if (oldValue != newValue)
    {
    m_dataSource.set(newValue);
    //TODO:
    //  1. Register for datasource changed event (and clear old one if exists)
    //  2. Clear all the vectors
    UpdatePrimaryItems();

    }
}


//void NavigationViewModel::setTopNavigationViewDataProvider(const TopNavigationViewDataProvider& topNavigationViewDataProvider) {
//	m_topDataProvider = winrt::make_weak(topNavigationViewDataProvider);
//}

void NavigationViewModel::ToggleIsExpanded(winrt::IInspectable const& item)
{
    if (auto navViewItem = item.try_as<winrt::NavigationViewItem>())
    {
        //RegisterItemExpandEventToSelf(navViewItem);
        //winrt::get_self<NavigationViewItem>(navViewItem)->RegisterSelfForViewModelExpandedEvent();
        navViewItem.IsExpanded(!navViewItem.IsExpanded());
    }
    else
    {
        // Should not be able toggle the 'IsExpanded' property for hidden items (hence, items that dont have a container).
    }
}

void NavigationViewModel::IsExpandedPropertyChanged(winrt::NavigationViewItem const& sender, winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto targetContainerItem = sender;
    auto targetListView = winrt::get_self<NavigationViewItem>(targetContainerItem)->GetNavigationViewList();
    unsigned int index = targetListView.IndexFromContainer(targetContainerItem);

    if (targetContainerItem)
    {
        if (targetContainerItem.IsExpanded())
        {
            auto dataSource = targetContainerItem.MenuItems();
            if (dataSource.Size() != 0)
            {
                //TODO: Implement Recursive Opening based on isChildSelected property when there is a need to expand down to the selected menuItem
                int openedDescendantOffset = 0;
                index = index + 1;
                for (unsigned int i = 0; i < dataSource.Size(); i++)
                {
                    InsertAt(index + i, dataSource.GetAt(i));

                    //winrt::TreeViewNode childNode{ nullptr };
                    //childNode = targetNode.Children().GetAt(i).as<winrt::TreeViewNode>();
                    //AddNodeToView(childNode, index + i + openedDescendantOffset);

                    //// IMPLEMENT OPEN RECURSIVELY IF A NODES ISEXPANED PROPERTY IS SET
                    //openedDescendantOffset = AddNodeDescendantsToView(childNode, index + i, openedDescendantOffset);
                }
                //Notify TreeView that a node is being expanded.
                //m_nodeExpandingEventSource(targetNode, nullptr);
            }
            else if (auto dataSource = targetContainerItem.MenuItemsSource())
            {
                //IMPLEMENT ADDING MENUITEMSSOURCE ITEMS TO VECTOR
            }
        }
        else
        {
            auto dataSource = targetContainerItem.MenuItems();
            if (dataSource.Size() != 0)
            {
                index = index + 1;
                for (unsigned int i = 0; i < dataSource.Size(); i++)
                {
                    auto container = targetListView.ContainerFromIndex(index);
                    if (auto itemBaseContainer = container.try_as<winrt::NavigationViewItemBase>())
                    {
                        RemoveItemAndDescendantsFromView(itemBaseContainer, index);
                    }
                }

                //Notife TreeView that a node is being collapsed
                //m_nodeCollapsedEventSource(targetNode, nullptr);
            }
            else if (auto dataSource = targetContainerItem.MenuItemsSource())
            {
                //IMPLEMENT ADDING MENUITEMSSOURCE ITEMS TO VECTOR
            }
		}
	}
}

void NavigationViewModel::RemoveItemAndDescendantsFromView(const winrt::NavigationViewItemBase& value, const int index)
{
    if (auto navigationViewItem = value.try_as<winrt::NavigationViewItem>())
    {
        // Remove all children from ListView
        if (navigationViewItem.IsExpanded())
        {
            unsigned int numberOfChildren = 0;
            if (navigationViewItem.MenuItems())
            {
                numberOfChildren = navigationViewItem.MenuItems().Size();
            }
            if (navigationViewItem.MenuItemsSource()) {
                //TODO: Get Children Count
            }

            for (unsigned int i = 0; i < numberOfChildren; i++)
            {
                //TODO: Update for MenuItemsSource case as well
                auto childNode = navigationViewItem.MenuItems().GetAt(i).as<winrt::NavigationViewItemBase>();
                RemoveItemAndDescendantsFromView(childNode, index + i);
            }
        }

        // Revoke isExpanded Event registration 
        if (navigationViewItem.MenuItems().Size() > 0 || navigationViewItem.MenuItemsSource())
        {
            winrt::get_self<NavigationViewItem>(navigationViewItem)->RemoveExpandedChanged(m_isExpandedChangedEventTokenVector[index]);
            m_isExpandedChangedEventTokenVector.erase(m_isExpandedChangedEventTokenVector.begin() + index);
            navigationViewItem.IsExpanded(false);
        }
    }

    //auto targetListView = winrt::get_self<NavigationViewItemBase>(value)->GetNavigationViewList();
    //unsigned int index = targetListView.IndexFromContainer(value);
    m_primaryItems.RemoveAt(index);
    //bool containsValue = IndexOfNode(value, valueIndex);
    //if (containsValue)
    //{
    //    RemoveAt(valueIndex);
    //}
}

void NavigationViewModel::InsertAt(uint32_t index, winrt::IInspectable const& value)
{

    m_primaryItems.InsertAt(index, value);


    //auto vector = GetActiveListView();
    //vector.
    //	GetVectorInnerImpl()->InsertAt(index, value);
    //winrt::TreeViewNode newNode = value.as<winrt::TreeViewNode>();

    ////Hook up events and save tokens
    //auto tvnNewNode = winrt::get_self<TreeViewNode>(newNode);
    //m_collectionChangedEventTokenVector.insert(m_collectionChangedEventTokenVector.begin() + index, tvnNewNode->ChildrenChanged({ this, &ViewModel::TreeViewNodeVectorChanged }));
    //m_IsExpandedChangedEventTokenVector.insert(m_IsExpandedChangedEventTokenVector.begin() + index, tvnNewNode->AddExpandedChanged({ this, &ViewModel::TreeViewNodePropertyChanged }));
}

winrt::ListView NavigationViewModel::GetActiveListView()
{
    return winrt::get_self<NavigationView>(m_navigationView.get())->LeftNavListView();
}

void NavigationViewModel::SetNavigationViewParent(winrt::NavigationView const& navigationView)
{
    m_navigationView = winrt::make_weak(navigationView);
}
