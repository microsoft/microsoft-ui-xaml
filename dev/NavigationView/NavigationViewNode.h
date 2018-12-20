#pragma once
#include <Vector.h>
#include "NavigationViewItem.h"

class NavigationViewNode
{
public:
    NavigationViewNode(const ITrackerHandleManager* m_owner);
    ~NavigationViewNode();

    tracker_ref<winrt::IVector<winrt::IInspectable>> m_menuItems;
    tracker_ref<winrt::IInspectable> m_menuItemsSource;
    //tracker_ref<NavigationViewNode> m_parent;
    //tracker_ref<std::vector<NavigationViewNode>> m_children;

    int m_depth{ 0 };
    bool m_isExpanded{ false };
    bool m_isSelected{ false };
    bool m_isChildSelected{ false };
    bool m_hasUnrealizedChildren{ false };
};
