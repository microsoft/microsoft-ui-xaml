// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <VisualState.h>
#include <storyboard.h>
#include <TemplateContent.h>
#include <DOCollection.h>
#include <SetterBaseCollection.h>
#include <StateTriggerCollection.h>
#include <VisualTree.h>

#include "theming\inc\Theme.h"

CVisualState::CVisualState(CCoreServices *pCore)
    : CDependencyObject(pCore)
    , m_pStoryboard(nullptr)
    , m_setters(nullptr)
    , m_pDeferredStoryboard(nullptr)
    , m_pDeferredSetters(nullptr)
    , m_pTemporaryNamescopeParent(nullptr)
    , m_pStateTriggerCollection(nullptr)
{
}

CVisualState::~CVisualState()
{
    ReleaseInterface(m_pTemporaryNamescopeParent);

    // CVisualState is the parent of the storyboard it
    // owns, so clear that back pointer.
    // If we're using new logic is this needed?
    if (m_pStoryboard && m_pStoryboard->GetParentInternal())
    {
        IGNOREHR(m_pStoryboard->RemoveParent(this));
    }
    ReleaseInterface(m_pStoryboard);
    ReleaseInterface(m_pDeferredStoryboard);
    ReleaseInterface(m_pDeferredSetters);
    ReleaseInterface(m_setters);
    ReleaseInterface(m_pStateTriggerCollection);
}

void CVisualState::ParentAndEnterDO(_In_ CDependencyObject* object)
{
    ASSERT(!object->IsAssociated() || object->DoesAllowMultipleAssociation());
    object->SetAssociated(true, this);
    ASSERT(!object->GetParent() || object->DoesAllowMultipleParents());
    IFCFAILFAST(object->AddParent(this));
    EnterParams enterParams(IsActive() /* isLive */, false /* skipNameRegistration */,
        GetCoercedIsEnabled(), GetUseLayoutRounding(), VisualTree::GetForElementNoRef(this, LookupOptions::NoFallback));
    IFCFAILFAST(object->Enter(GetStandardNameScopeOwner(), enterParams));
}

void CVisualState::UnparentAndLeaveDO(_In_ CDependencyObject* object)
{
    LeaveParams leaveParams(IsActive(), false /* skipNameRegistration */,
        GetCoercedIsEnabled(), false /*fVisualTreeBeingReset*/ );
    IFCFAILFAST(object->Leave(GetStandardNameScopeOwner(), leaveParams));
    ASSERT(object->DoesAllowMultipleParents() || object->GetParent() == this);
    IFCFAILFAST(object->RemoveParent(this));
    ASSERT(object->IsAssociated());
    object->SetAssociated(false, nullptr);
}

_Check_return_ HRESULT CVisualState::EnterImpl(
    _In_ CDependencyObject* namescopeOwner, EnterParams params)
{
    IFC_RETURN(CDependencyObject::EnterImpl(namescopeOwner, params));
    if (m_pStoryboard)
    {
        IFC_RETURN(m_pStoryboard->Enter(namescopeOwner, params));
    }
    return S_OK;
}

_Check_return_ HRESULT CVisualState::LeaveImpl(
    _In_ CDependencyObject* namescopeOwner, LeaveParams params)
{
    IFC_RETURN(CDependencyObject::LeaveImpl(namescopeOwner, params));
    if (m_pStoryboard)
    {
        IFC_RETURN(m_pStoryboard->Leave(namescopeOwner, params));
    }
    return S_OK;
}

_Check_return_ HRESULT
CVisualState::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    IFC_RETURN(CDependencyObject::NotifyThemeChangedCore(theme, fForceRefresh));
    // Unlike Enter/Leave above this was always done for Storyboards, even in v1
    // of VSM.
    if (m_pStoryboard)
    {
        IFC_RETURN(m_pStoryboard->NotifyThemeChanged(theme, fForceRefresh));
    }
    return S_OK;
}

_Check_return_ VisualStateToken CVisualState::GetVisualStateToken()
{
    if(m_token.IsEmpty())
    {
        m_token = VisualStateToken(this);
    }

    return m_token;
}

void CVisualState::SetOptimizedIndex(_In_ int index)
{
    // The VisualStateToken for this VisualState never be set more than once
    ASSERT(m_token.IsEmpty());

    m_token = VisualStateToken(index);
}

CDependencyObject* CVisualState::GetTemplatedParent()
{
    if (!m_templatedParent.expired())
    {
        // Apparently GetTemplatedParent() isn't expected to addref its return value...
        return m_templatedParent.lock_noref();
    }
    return nullptr;
}
