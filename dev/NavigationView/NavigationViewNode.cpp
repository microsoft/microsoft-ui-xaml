#include "pch.h"
#include "common.h"
#include "NavigationViewNode.h"


NavigationViewNode::NavigationViewNode(const ITrackerHandleManager* m_owner) :
    m_menuItems(m_owner),
    m_menuItemsSource(m_owner)
{

}

NavigationViewNode::~NavigationViewNode()
{
}
