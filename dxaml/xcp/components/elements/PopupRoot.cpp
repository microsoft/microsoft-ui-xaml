// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Popup.h"
#include "UIElementCollection.h"

CPopup* CPopupRoot::GetOpenPopupWithChild(_In_ const CUIElement* const child, bool checkUnloadingChildToo) const
{
    if (m_pOpenPopups)
    {
        // Include unloading popups too.
        for (CXcpList<CPopup>::XCPListNode *pNode = m_pOpenPopups->GetHead();
             pNode != nullptr;
             pNode = pNode->m_pNext)
        {
            CPopup* popup = pNode->m_pData;

            if (popup->m_pChild == child
                || checkUnloadingChildToo && popup->m_unloadingChild == child)
            {
                return popup;
            }
        }
    }

    return nullptr;
}

bool CPopupRoot::HasOpenOrUnloadingPopups() const
{
    return m_pOpenPopups != nullptr && m_pOpenPopups->GetHead() != nullptr;
}

// When closing all open popups, we can't just close them in the order that they were opened. With nested
// popups, calling Close on the outer popup will also close the inner popup, which calls back to the popup
// root and causes reentrancy since we're in the middle of closing the outer popup. So we need to close a
// nested popups before closing its parent popup. This method returns open popups in an order that's safe
// to iterate over and close.
std::vector<CPopup*> CPopupRoot::GetPopupsInSafeClosingOrder()
{
    std::vector<CPopup*> popupsInSafeClosingOrder;

    if (m_pOpenPopups != nullptr)
    {
        CXcpList<CPopup>::XCPListNode* openPopupNode = m_pOpenPopups->GetHead();

        while (openPopupNode)
        {
            CPopup* popup = openPopupNode->m_pData;
            openPopupNode = openPopupNode->m_pNext;

            // For each open popup, find a safe time to call Close on it. By default the popup is closed in natural
            // order (i.e. the order that it was opened in). But if the popup has popup ancestors that also need to
            // be closed, it must be closed before the ancestors get closed. So default to the index at end of the
            // list, then walk up the ancestor chain. For each popup ancestor, if that ancestor is already in the
            // close list, then we have to insert this popup before that ancestor.
            auto insertPopupIterator = popupsInSafeClosingOrder.end();

            for (CDependencyObject* ancestor = popup->GetParentFollowPopups();
                // We can early exit if we determine that this popup must be closed before all other popups.
                ancestor != nullptr && insertPopupIterator != popupsInSafeClosingOrder.begin();
                ancestor = ancestor->GetParentFollowPopups())
            {
                if (ancestor->OfTypeByIndex<KnownTypeIndex::Popup>())
                {
                    CPopup* ancestorPopup = static_cast<CPopup*>(ancestor);
                    if (ancestorPopup->IsOpen())
                    {
                        // We found an open ancestor popup. If it also needs to be closed, then make sure we're
                        // closed before that ancestor.
                        const auto& ancestorIterator = std::find(popupsInSafeClosingOrder.begin(), popupsInSafeClosingOrder.end(), ancestorPopup);
                        if (ancestorIterator != popupsInSafeClosingOrder.end()
                            && ancestorIterator < insertPopupIterator)
                        {
                            insertPopupIterator = ancestorIterator;
                        }
                    }
                }
            }

            popupsInSafeClosingOrder.insert(insertPopupIterator, popup);
        }
    }

    return popupsInSafeClosingOrder;
}
