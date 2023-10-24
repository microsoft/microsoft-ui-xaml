// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Override for Enter to enforce only entering the tree on the first time.
_Check_return_ HRESULT CNoParentShareableDependencyObject::Enter(
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params)
{
    if (params.fIsLive)
    {
        if (m_cEnteredLive == 0)
        {
            // This is the first time the SDO is live, so tell its kids.
            // This would also set m_cEnteredLive to 1.
            IFC_RETURN(CDependencyObject::Enter(pNamescopeOwner, params));
        }
        else
        {
            // Increment the enter count.
            m_cEnteredLive++;
            if (GetWantsInheritanceContextChanged() && HasManagedPeer())
            {

                // InheritanceContext is disabled when we have multiple parents so if we
                // re-enter the live tree, we need to update it if there's a listener.
                // It looks like we should propagate this to child properties but since
                // we propagate all the custom properties on the managed side, we'd only
                // need to do so here if there is a NoParentShareableDO with a core property
                // that points to a DO that would have Bindings on it.  Currently, it doesn't
                // appear that that scenario exists so we don't need to propagate here.  If
                // we do come accross that in the future, we would need to propagate this.
                IFC_RETURN(FxCallbacks::JoltHelper_RaiseEvent(
                    this,
                    DirectUI::ManagedEvent::ManagedEventInheritanceContextChanged,
                    /* pArgs */ NULL));
            }
            OnSkippedLiveEnter();
        }
    }
    else if (ShouldSharedObjectRegisterName(params.pParentResourceDictionary))
    {
        IFC_RETURN(CDependencyObject::Enter(pNamescopeOwner, params));
    }

    return S_OK;
}

// Override for Leave to enforce only leaving the tree on the last time.
_Check_return_ HRESULT CNoParentShareableDependencyObject::Leave(
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    if (params.fIsLive && m_cEnteredLive > 0)
    {
        if (m_cEnteredLive == 1)
        {
            // This is the last time we're pulling the SDO off a live tree, so yank it for real.
            // This would also set m_cEnteredLive to 0.
            IFC(CDependencyObject::Leave(pNamescopeOwner, params));
        }
        else
        {
            m_cEnteredLive--;
            OnSkippedLiveLeave();
        }
    }
    else if (ShouldSharedObjectRegisterName(params.pParentResourceDictionary))
    {
        IFC(CDependencyObject::Leave(pNamescopeOwner, params ) );
    }

Cleanup:
    RRETURN(S_OK);
}
