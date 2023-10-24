// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method: AddParent
//
//  Synopsis:
//     Adds a parent to this multi-parent DO.
//------------------------------------------------------------------------
_Check_return_
HRESULT
CMultiParentShareableDependencyObject::AddParent(
    _In_ CDependencyObject *pNewParent,
    bool fPublic /*= true */,
    _In_opt_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler /*= nullptr */
    )
{
    CDependencyObject* pCurrParent = GetParentInternal(false);

    ASSERT(pNewParent);
    if (!pCurrParent && m_rgParentAssociation.empty())
    {
        SetParentInternal(pNewParent, true /*fFireChanged*/, pfnNewParentRenderChangedHandler);
    }
    else
    {
        if (m_rgParentAssociation.empty())
        {
            ASSERT(pCurrParent);
            m_rgParentAssociation.emplace_back(pCurrParent, NWGetRenderChangedHandlerInternal());

            // This block moves the parent from the base class storage into the multi-parent array, so there's
            // no need to propagate changes to this parent - it isn't being added or removed.
            SetParentInternal(nullptr, false /*fFireChanged*/);
        }

        // Duplicate parents will be added so that only a matching number of RemoveParent calls will
        // break the reference back to the parent.
        m_rgParentAssociation.emplace_back(pNewParent, pfnNewParentRenderChangedHandler);

        // Notify the new parent of the rendering change.
        if (pfnNewParentRenderChangedHandler)
        {
            NotifyParentChange(pNewParent, pfnNewParentRenderChangedHandler);
        }
    }

    // Add expected reference on peer if necessary, and notify peer of parent update.
    IFC_RETURN(OnParentChange(
        NULL,
        pNewParent,
        // TRUE if this DO has at least one parent
        GetParentInternal(false) || !m_rgParentAssociation.empty()));
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: RemoveParent
//
//  Synopsis:
//     Removes the given parent of the element.
//------------------------------------------------------------------------
_Check_return_
HRESULT
CMultiParentShareableDependencyObject::RemoveParent(
    _In_ CDependencyObject *pParentToRemove
    )
{
    if (!m_rgParentAssociation.empty())
    {
        auto it = std::find_if(m_rgParentAssociation.rbegin(), m_rgParentAssociation.rend(),
            [&](const ParentAssociation& item)
        {
            return item.pParent == pParentToRemove;
        });

        if (it != m_rgParentAssociation.rend())
        {
            if (it->pfnRenderChangedHandler)
            {
                NotifyParentChange(it->pParent, it->pfnRenderChangedHandler);
            }
            m_rgParentAssociation.erase(--it.base());
        }

        if (m_rgParentAssociation.empty())
        {
            SetIsValueOfInheritedProperty(FALSE);
        }
    }
    else
    {
        if (GetParentInternal(false) == pParentToRemove)
        {
            SetParentInternal(nullptr, true /*fFireChanged*/);
            SetIsValueOfInheritedProperty(false);
        }
    }

    // Remove expected reference from peer if necessary, and notify peer of parent update.
    IFC_RETURN(OnParentChange(
        pParentToRemove,
        NULL,
        // TRUE if this DO has at least one parent
        GetParentInternal(false) || !m_rgParentAssociation.empty()));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This method propagates dirty flags to the parent DOs via their respective stored
//      dirty flag functions.
//
//------------------------------------------------------------------------
void
CMultiParentShareableDependencyObject::NWPropagateDirtyFlag(DirtyFlags flags)
{
    NW_ASSERT_CAN_PROPAGATE_DIRTY_FLAGS

    if (!m_rgParentAssociation.empty())
    {
        for (auto& assoc : m_rgParentAssociation)
        {
            ASSERT(assoc.pParent);

            // Handles sub-property changes for inherited property values.
            // If this DO is the value of an inherited DP, dirtiness needs to be propagated
            // down through its parent UIElement's subgraph.
            if (IsValueOfInheritedProperty() && assoc.pParent->OfTypeByIndex<KnownTypeIndex::UIElement>())
            {
                CUIElement *pUIE = static_cast<CUIElement*>(assoc.pParent);
                NWPropagateInheritedDirtyFlag(pUIE, flags);
            }

            // Call dirty flag function for each parent.
            if (assoc.pfnRenderChangedHandler)
            {
                (assoc.pfnRenderChangedHandler)(assoc.pParent, flags);
            }
        }
    }
    else
    {
        CNoParentShareableDependencyObject::NWPropagateDirtyFlag(flags);
    }
}

