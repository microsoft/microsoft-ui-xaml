// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UIElementWeakCollection.h"
#include "UIElement.h"
#include "Weakref_ptr.h"
#include "ICollectionChangeCallback.h"
#include "ErrorHelper.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

_Check_return_ HRESULT CUIElementWeakCollection::Create(
    _Outptr_ CDependencyObject** object,
    _In_ CREATEPARAMETERS* create)
{
    *object = new CUIElementWeakCollection(create->m_pCore);
    return S_OK;
}

XUINT32 CUIElementWeakCollection::GetCount() const
{
    return static_cast<XUINT32>(m_items.size());
}

_Check_return_ HRESULT CUIElementWeakCollection::ValidateItem(const CValue& newItem)
{
    if (newItem.GetType() != valueObject)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    CDependencyObject* elementDO = newItem.AsObject();
    if (elementDO != nullptr && !elementDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (m_disallowAncestorsInCollection)
    {
        // The new item can't be an ancestor UIElement.
        CUIElement* element = static_cast<CUIElement*>(elementDO);

        IFC_RETURN(CheckForAncestor(element, nullptr /* newParent */));
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElementWeakCollection::Append(_In_ CValue& value, XUINT32* index)
{
    IFC_RETURN(ValidateItem(value));
    CUIElement* element = static_cast<CUIElement*>(value.AsObject());
    m_items.push_back(xref::get_weakref(element));
    *index = m_items.size() - 1;

    if (m_changeCallbackNoRef != nullptr)
    {
        IFCFAILFAST(m_changeCallbackNoRef->ElementInserted(*index));
    }

    return S_OK;
}

_Check_return_ HRESULT CUIElementWeakCollection::Insert(XUINT32 index, _In_ CValue& value)
{
    // DirectUI::PresentationFrameworkCollection::InsertAt should have validated the index.
    if (index > m_items.size())
    {
        IFCFAILFAST(E_BOUNDS);
    }

    IFC_RETURN(ValidateItem(value));
    CUIElement* element = static_cast<CUIElement*>(value.AsObject());
    m_items.insert(m_items.begin() + index, xref::get_weakref(element));

    if (m_changeCallbackNoRef != nullptr)
    {
        IFCFAILFAST(m_changeCallbackNoRef->ElementInserted(index));
    }

    return S_OK;
}

void* CUIElementWeakCollection::RemoveAt(XUINT32 index)
{
    if (m_changeCallbackNoRef != nullptr)
    {
        IFCFAILFAST(m_changeCallbackNoRef->ElementRemoved(index));
    }

    xref::weakref_ptr<CUIElement> removed = m_items[index];
    m_items.erase(m_items.begin() + index);
    return removed.lock_noref();    // RemoveAt returns a NoRef object
}

void* CUIElementWeakCollection::GetItemWithAddRef(XUINT32 index)
{
    xref::weakref_ptr<CUIElement> item = m_items[index];
    xref_ptr<CUIElement> element = item.lock();
    return element.detach();
}

_Check_return_ HRESULT CUIElementWeakCollection::Neat(bool /* processLeave */)
{
    // Ignore the processLeave flag. This collection doesn't do enter/leave, and it doesn't take references.
    m_items.clear();
    return S_OK;
}

// Be careful, position is supposed to be one past the destination if moving towards
// the end, but supposed to be exactly the destination if moving towards the beginning!
_Check_return_ HRESULT CUIElementWeakCollection::MoveInternal(XINT32 index, XINT32 position)
{
    // It looks like the only place that uses Move is UIElements moving their children their child collections.
    // This collection will not support Move.
    IFCFAILFAST(E_NOTIMPL);
    return S_OK;
}

_Check_return_ HRESULT CUIElementWeakCollection::CheckForAncestorElements(_In_opt_ CUIElement* newParent) const
{
    for (const xref::weakref_ptr<CUIElement>& item : m_items)
    {
        const CUIElement* element = item.lock_noref();
        if (element != nullptr)
        {
            IFC_RETURN(CheckForAncestor(element, newParent));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CUIElementWeakCollection::CheckForAncestor(const CUIElement* element, _In_opt_ CUIElement* newParent) const
{
    // There can be multi-parent shareable DOs along the way, so build a vector of all UIElements that use this collection.
    std::vector<CUIElement*> elementAncestors;

    // If we have a prospective owner of this collection, check it too.
    if (newParent != nullptr)
    {
        elementAncestors.push_back(newParent);
    }

    // Keep a list of all non-UIE ancestors that we've encountered so far. This list can contain DOs that are shareable, which
    // have multiple parents.
    std::vector<CDependencyObject*> nonElementAncestors;
    nonElementAncestors.push_back(GetOwner());

    while (!nonElementAncestors.empty())
    {
        CDependencyObject* ancestorDO = nonElementAncestors[nonElementAncestors.size() - 1];
        nonElementAncestors.pop_back();

        if (ancestorDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            // Found a UIElement. Add it to the list of ancestor UIElements.
            elementAncestors.push_back(static_cast<CUIElement*>(ancestorDO));
        }
        else if (ancestorDO->DoesAllowMultipleParents())
        {
            // Found a shareable DO. Check all its parents.
            CMultiParentShareableDependencyObject* shareableAncestorDO = static_cast<CMultiParentShareableDependencyObject*>(ancestorDO);

            const auto& parentCount = shareableAncestorDO->GetParentCount();
            for (std::size_t i = 0; i < parentCount; i++)
            {
                nonElementAncestors.push_back(shareableAncestorDO->GetParentItem(i));
            }
        }
        else
        {
            // Found a nonshareable DO. Follow its single parent pointer.
            nonElementAncestors.push_back(ancestorDO->GetParentInternal(false /* publicParentOnly */));
        }
    }

    // Now that we have a list of ancestor elements, check their ancestor chains.
    for (CUIElement* ancestorElement : elementAncestors)
    {
        while (ancestorElement != nullptr)
        {
            if (ancestorElement == element)
            {
                IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ANCESTOR_ELEMENT_CANNOT_BE_SHADOW_RECEIVER));
            }
            // The parent of a UIElement must be a UIElement.
            CDependencyObject* ancestorDO = ancestorElement->GetParentFollowPopups();
            FAIL_FAST_ASSERT(!ancestorDO || ancestorDO->OfTypeByIndex<KnownTypeIndex::UIElement>());
            ancestorElement = static_cast<CUIElement*>(ancestorDO);
        }
    }

    return S_OK;
}