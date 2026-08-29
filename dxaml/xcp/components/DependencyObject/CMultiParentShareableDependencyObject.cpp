// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <MultiParentShareableDependencyObject.h>
#include <corep.h>
#include <Resources.h>

//------------------------------------------------------------------------
//
//  Method:  CMultiParentShareableDependencyObject::GetMentor
//
//  Synopsis:
//     Gets the mentor by walking up the InheritanceContext (parent) tree.
//     Returns null if no mentor exists.  Overridden from base DO method to handle
//     multiple parents.
//
//------------------------------------------------------------------------
_Ret_maybenull_ CDependencyObject*
CMultiParentShareableDependencyObject::GetMentor()
{
    if (fInheritanceContextEnabled)
    {
        if (m_rgParentAssociation.empty())
        {
            // We only have one parent so use the base method which has access to m_pParent.  We
            // need m_pParent since we can't use GetParent
            return CDependencyObject::GetMentor();
        }
        else
        {
            // If we have multiple parents, then InheritanceContext is turned off permanently except in the cases
            // of ResourceDictionary and ContentControl.  It is ok to have multiple parents as long as
            // the first parent is a ResourceDictionary, or if the first parent is a ContentControl and there are at most
            // two parents.
            auto pParentDO = m_rgParentAssociation[0].pParent;
            ASSERT(pParentDO);

            if (pParentDO->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>() ||
                (pParentDO->OfTypeByIndex<KnownTypeIndex::ContentControl>() && m_rgParentAssociation.size() <= 2) ||
                (pParentDO->OfTypeByIndex<KnownTypeIndex::ScrollContentControl>() && m_rgParentAssociation.size() <= 2))
            {
                return pParentDO->GetMentor();
            }

            // Permanently disable InheritanceContext once we have multiple parents
            fInheritanceContextEnabled = false;
        }
    }

    return nullptr;
}

//------------------------------------------------------------------------
//
//  Method:  CMultiParentShareableDependencyObject::ShouldSharedObjectRegisterName
//
//  Synopsis:
//     Determines whether an object should be (un)registering its name
//     during the enter/leave walk.
//     In the case of a multiple parents, we only do this if the parent
//     currently entering/leaving is a resource dictionary.
//
//------------------------------------------------------------------------
bool CMultiParentShareableDependencyObject::ShouldSharedObjectRegisterName(_In_opt_ CResourceDictionary *pParentResourceDictionary) const
{
    if (GetSharingCount() <= 1)
    {
        return true;
    }
    else
    {
        auto core = GetContext();

        if (!core->IsShuttingDown() &&
            !core->IsDestroyingCoreServices() &&
            pParentResourceDictionary != nullptr &&
            pParentResourceDictionary->IsProcessingEnterLeave())
        {
            // we will only wish to register if this namescope walk is started by
            // a resourcedictionary
            return true;
        }
        return false;
    }
}

std::size_t CMultiParentShareableDependencyObject::GetParentCount() const
{
    if (m_rgParentAssociation.empty())
    {
        return GetParentInternal(false) ? 1 : 0;
    }
    else
    {
        return m_rgParentAssociation.size();
    }
}

_Ret_notnull_ CDependencyObject*
CMultiParentShareableDependencyObject::GetParentItem(
    std::size_t index
    ) const
{
    CDependencyObject* result = nullptr;
    if (m_rgParentAssociation.empty())
    {
        ASSERT(index == 0);
        result = GetParentInternal(false);
    }
    else
    {
        ASSERT(index < m_rgParentAssociation.size());
        result = m_rgParentAssociation[index].pParent;
    }
    ASSERT(result);
    return result;
}

#if DBG
bool CMultiParentShareableDependencyObject::HasParent(_In_ CDependencyObject *pCandidate) const
{
    auto cParents = GetParentCount();

    for (size_t i = 0; i < cParents; i++)
    {
        if (GetParentItem(i) == pCandidate)
        {
            return true;
        }
    }
    return false;
}
#endif

//------------------------------------------------------------------------
//
//  Method:   SetParentForInheritanceContextOnly
//
//  Synopsis:
//     Sets the Parent but only for use with InheritanceContext and not for GetParent.
//     Overridden here to handle the multi parent case.
//
//------------------------------------------------------------------------
void CMultiParentShareableDependencyObject::SetParentForInheritanceContextOnly(_In_opt_ CDependencyObject *pDO)
{
    if (m_rgParentAssociation.empty())
    {
        CDependencyObject::SetParentForInheritanceContextOnly(pDO);
    }
}

