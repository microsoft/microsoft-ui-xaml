// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualStateManager.g.h"
#include "Control.g.h"
#include "VisualStateGroup.g.h"
#include "Timeline.g.h"
#include "VisualState.g.h"
#include <DependencyLocator.h>
#include <CVisualStateManager2.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT VisualStateManager::RaiseCurrentStateChangingImpl(
    _In_ xaml::IVisualStateGroup* pStateGroup,
    _In_ xaml::IVisualState* pOldState,
    _In_ xaml::IVisualState* pNewState,
    _In_ xaml_controls::IControl* pControl)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spRoot;

    IFCPTR(pStateGroup);
    IFCPTR(pNewState);
    IFCPTR(pControl);

    IFC(static_cast<Control*>(pControl)->get_ImplementationRoot(&spRoot));
    if (!spRoot)
    {
        // Ignore if a ControlTemplate hasn't been applied.
        goto Cleanup;
    }

    IFC(static_cast<VisualStateGroup*>(pStateGroup)->RaiseCurrentStateChanging(spRoot.Get(), pOldState, pNewState, pControl));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT VisualStateManager::RaiseCurrentStateChangedImpl(
    _In_ xaml::IVisualStateGroup* pStateGroup,
    _In_ xaml::IVisualState* pOldState,
    _In_ xaml::IVisualState* pNewState,
    _In_ xaml_controls::IControl* pControl)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spRoot;

    IFCPTR(pStateGroup);
    IFCPTR(pNewState);
    IFCPTR(pControl);

    IFC(static_cast<Control*>(pControl)->get_ImplementationRoot(&spRoot));
    if (!spRoot)
    {
        // Ignore if a ControlTemplate hasn't been applied.
        goto Cleanup;
    }

    IFC(static_cast<VisualStateGroup*>(pStateGroup)->RaiseCurrentStateChanged(spRoot.Get(), pOldState, pNewState, pControl));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualStateManager::GoToStateCoreImpl(
    _In_ IControl* pControl,
    _In_ IFrameworkElement* pTemplateRoot,
    _In_ HSTRING hStateName,
    _In_ IVisualStateGroup* pGroup,
    _In_ IVisualState* pState,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* pbReturnValue)
{
    ctl::ComPtr<IStoryboard> spStoryboard;
    BOOLEAN bRefreshInheritanceContext;
    BOOLEAN bWentToState;

    IFCPTR_RETURN(pControl);
    IFCPTR_RETURN(pTemplateRoot);

    IFC_RETURN(VisualStateManager::GoToStatePrivate(
        pControl,
        hStateName,
        static_cast<VisualState*>(pState),
        static_cast<VisualStateGroup*>(pGroup),
        bUseTransitions,
        &bRefreshInheritanceContext,
        &bWentToState));

    if (pState)
    {
        IFC_RETURN(pState->get_Storyboard(&spStoryboard));
        if (bRefreshInheritanceContext && pState != NULL && spStoryboard)
        {
            // We need to refresh the InheritanceContext before GoToState can succeed
            IFC_RETURN(VisualStateManager::RetryGoToStateAfterRefreshingInheritanceContext(pControl, pTemplateRoot, pGroup, pState, bUseTransitions, pbReturnValue));
            return S_OK;
        }
    }

    *pbReturnValue = bWentToState;
    return S_OK;
}

// This method handles a special case where the Storyboard has a 0 duration, and it contains a child with a DependencyObject Binding attached to it.
// In this case, we need to synchronously refresh the InheritanceContext of the child before Begin is called on the Storyboard.  VisualStateGroup
// automatically errors out in this scenario so the InheritanceContext can be refreshed synchronously at a safe time to avoid reentrancy issues.  Here
// we handle the refresh and retry GoToState which should succeed.
_Check_return_ HRESULT
VisualStateManager::RetryGoToStateAfterRefreshingInheritanceContext(
    _In_ xaml_controls::IControl* pControl,
    _In_ IFrameworkElement* pTemplateRoot,
    _In_ xaml::IVisualStateGroup* pGroup,
    _In_ xaml::IVisualState* pState,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* pbReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IStoryboard> spStoryboard;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;
    ctl::ComPtr<wfc::IIterable<xaml_animation::Timeline*>> spIterable;
    ctl::ComPtr<wfc::IIterator<xaml_animation::Timeline*>> spIterator;
    BOOLEAN bHasCurrent;

    IFC(pState->get_Storyboard(&spStoryboard));
    IFC(spStoryboard->get_Children(&spChildren));
    IFC(spChildren.As(&spIterable));
    IFC(spIterable->First(&spIterator));
    IFC(spIterator->get_HasCurrent(&bHasCurrent));

    while (bHasCurrent)
    {
        ctl::ComPtr<ITimeline> spTimeline;
        IFC(spIterator->get_Current(&spTimeline));

        // Animations are a child of a Storyboard. If the animation has a binding, it may be listening to the InheritanceContextChanged
        // event on the animation, and set the "wantsInheritanceContextChanged" flag on it. However, that flag does not bubble up, so
        // the Storyboard's "wantsInheritanceContextChanged" flag is never set. By calling NotifyInheritanceContextChanged with the
        // "ForceTopLevelEvent" flag, we make sure that we ignore the flag on the Storyboard.
        IFC(spTimeline.Cast<Timeline>()->NotifyInheritanceContextChanged(InheritanceContextChangeKind::ForceTopLevelEvent));
        IFC(spIterator->MoveNext(&bHasCurrent));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualStateManager::GoToStatePrivate(
    _In_ IControl* pControl,
    _In_ HSTRING hStateName,
    _In_ IVisualState* pState,
    _In_ IVisualStateGroup* pStateGroup,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* pbRefreshInheritanceContext,
    _Out_ BOOLEAN* pbReturnValue)
{
    IFCPTR_RETURN(pControl);
    IFCPTR_RETURN(pbRefreshInheritanceContext);
    IFCPTR_RETURN(pbReturnValue);

    LPCWSTR stateName = nullptr;
    if (hStateName)
    {
        stateName = WindowsGetStringRawBuffer(hStateName, nullptr);
    }

    CDependencyObject* pStateDO = nullptr;
    if(pState)
    {
        pStateDO = static_cast<VisualState*>(pState)->GetHandle();
    }

    CDependencyObject* pStateGroupDO = nullptr;
    if(pStateGroup)
    {
        pStateGroupDO = static_cast<VisualStateGroup*>(pStateGroup)->GetHandle();
    }

    bool succeeded = false;

    HRESULT hr = CVisualStateManager::GoToState(
        static_cast<Control*>(pControl)->GetHandle(),
        stateName,
        static_cast<CVisualState*>(pStateDO),
        static_cast<CVisualStateGroup*>(pStateGroupDO),
        !!bUseTransitions,
        &succeeded);

    if (hr == E_DO_INHERITANCE_CONTEXT_NEEDED)
    {
        *pbRefreshInheritanceContext = TRUE;
        *pbReturnValue = FALSE;
        hr = S_OK;
    }
    else
    {
        *pbRefreshInheritanceContext = FALSE;
        *pbReturnValue = succeeded;
        IFC_RETURN(hr);
    }

    return S_OK;
}

_Check_return_ HRESULT
VisualStateManager::GoToState(
    _In_ IControl* pControl,
    _In_ HSTRING hStateName,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* pbReturnValue)
{
    IFC_RETURN(VisualStateManager::GoToStateImpl(pControl, VisualStateToken(), hStateName, -1, bUseTransitions, pbReturnValue));
    return S_OK;
}

_Check_return_ HRESULT
VisualStateManager::TryGetState(
    _In_ xaml_controls::IControl* pControl,
    _In_ _Null_terminated_ const WCHAR* pszStateName,
    _Outptr_result_maybenull_ xaml::IVisualStateGroup** ppGroup,
    _Outptr_result_maybenull_ xaml::IVisualState** ppState,
    _Out_ BOOLEAN* pbReturnValue)
{
    HRESULT hr = S_OK;
    CVisualStateGroup *pGroup = nullptr;
    CVisualState *pState = nullptr;
    ctl::ComPtr<DependencyObject> spGroupDO;
    ctl::ComPtr<DependencyObject> spStateDO;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    bool found = false;

    if (ppGroup)
    {
        *ppGroup = nullptr;
    }
    if (ppState)
    {
        *ppState = nullptr;
    }

    if (pCore)
    {
        IFC(CVisualStateManager::FindVisualState(static_cast<CControl*>(static_cast<Control*>(pControl)->GetHandle()),
                               nullptr,
                               pszStateName,
                               &pGroup,
                               &pState,
                               &found));
        if (ppGroup && found && pGroup)
        {
            IFC(pCore->GetPeer(pGroup, KnownTypeIndex::VisualStateGroup, &spGroupDO));
            IFC(ctl::do_query_interface(*ppGroup, spGroupDO.Get()));
        }
        if (ppState && found && pState)
        {
            IFC(pCore->GetPeer(pState, KnownTypeIndex::VisualState, &spStateDO));
            IFC(ctl::do_query_interface(*ppState, spStateDO.Get()));
        }
    }

    *pbReturnValue = !!found;
Cleanup:
    ReleaseInterface(pState);
    ReleaseInterface(pGroup);
    RRETURN(hr);
}
// Use the default non-custom VSM implementation to process a state change on the given control
// and implementation root. It is called by the base implementation of the protected virtual
// method FrameworkElement.GoToElementStateCore on pImplementationRoot.
_Check_return_ HRESULT
VisualStateManager::GoToStateWithDefaultVSM(
    _In_ xaml_controls::IControl* pControl,
    _In_ xaml::IFrameworkElement* pImplementationRoot,
    _In_ HSTRING hStateName,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* pbReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bRefreshInheritanceContext;

    *pbReturnValue = FALSE;

    IFC(VisualStateManager::GoToStatePrivate(
        pControl,
        hStateName,
        nullptr,
        nullptr,
        bUseTransitions,
        &bRefreshInheritanceContext,
        pbReturnValue));

    if (bRefreshInheritanceContext)
    {
        // We need to refresh the InheritanceContext before GoToState can succeed.
        ctl::ComPtr<IVisualState> spState;
        ctl::ComPtr<IVisualStateGroup> spGroup;
        BOOLEAN bIgnored;

        IFC(TryGetState(pControl, WindowsGetStringRawBuffer(hStateName, nullptr), &spGroup, &spState, &bIgnored));
        if (spState)
        {
            ctl::ComPtr<IStoryboard> spStoryboard;
            IFC(spState->get_Storyboard(&spStoryboard));
            if (spStoryboard)
            {
                IFC(VisualStateManager::RetryGoToStateAfterRefreshingInheritanceContext(pControl, pImplementationRoot, spGroup.Get(), spState.Get(), bUseTransitions, pbReturnValue));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Callback from core that receives state changes initiated by the StateTrigger system.
// All calls must be routed through the GoToState call to support overrides
// from CustomVisualStateManager.
_Check_return_ HRESULT VisualStateManager::CustomVSMGoToState(
    _In_ CDependencyObject* pControl,
    _In_ VisualStateToken token,
    _In_ int groupIndex,
    _In_ bool bUseTransitions,
    _In_ bool* bSucceeded)
{
    ctl::ComPtr<DependencyObject> spPeer;
    IControl* pIControlNoRef = NULL;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pControl, &spPeer));
    pIControlNoRef = spPeer.Cast<Control>();

    BOOLEAN succeeded;
    IFC_RETURN(GoToStateImpl(pIControlNoRef, token, nullptr, groupIndex, bUseTransitions, &succeeded));
    *bSucceeded = !!succeeded;

    return S_OK;
}

// Main entry point for all VSM GoToState calls, with the exception
// of PasswordBox, TextBox, and TextBoxBase which incorrectly call the
// core CVisualStateManager's GoToState method, making it impossible to
// implement a custom VSM or override the FE's GoToElementState method.
_Check_return_ HRESULT
VisualStateManagerFactory::GoToStateImpl(
    _In_opt_ IControl* pControl,
    _In_opt_ HSTRING hStateName,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* pbReturnValue)
{
    return VisualStateManager::GoToStateImpl(
            pControl,
            VisualStateToken(),
            hStateName,
            -1,
            bUseTransitions,
            pbReturnValue);
}

_Check_return_ HRESULT VisualStateManager::GoToStateImpl(
    _In_ IControl* pControl,
    _In_ VisualStateToken visualStateToken,
    _In_ HSTRING hStateName,
    _In_ int groupIndex,
    _In_ BOOLEAN bUseTransitions,
    _Out_ BOOLEAN* pbReturnValue)
{
    ctl::ComPtr<IFrameworkElement> spRoot;
    ctl::ComPtr<IVisualStateManager> spCustomVsm;
    BOOLEAN bWentToState = false;
    wrl_wrappers::HString stateName;
    stateName.Set(hStateName);

    IFCPTR_RETURN(pControl);
    IFCPTR_RETURN(pbReturnValue);
    *pbReturnValue = FALSE;

    IFC_RETURN(static_cast<Control*>(pControl)->get_ImplementationRoot(&spRoot));
    if (!spRoot)
    {
        // Ignore state changes if a ControlTemplate hasn't been applied.
        return S_OK;
    }

    // Look for a custom VSM, and call it if it was found, regardless of whether the state was found or not.
    // This is because we don't know what the custom VSM will want to do. But for our default implementation,
    // we know that if we haven't found the state, we don't actually want to do anything.
    IFC_RETURN(VisualStateManagerFactory::GetCustomVisualStateManagerStatic(spRoot.Get(), &spCustomVsm));
    if (spCustomVsm)
    {
        ctl::ComPtr<IVisualStateGroup> spGroup;
        ctl::ComPtr<IVisualState> spState;
        BOOLEAN bIgnored;
        xref_ptr<CDependencyObject> visualState = nullptr;
        ctl::ComPtr<DependencyObject> spVisualStatePeer;

        // Get VisualStateGroup using the group index
        if(groupIndex != -1)
        {
            ctl::ComPtr<wfc::IVector<xaml::VisualStateGroup*>> spGroups;
            IFC_RETURN(VisualStateManagerFactory::GetVisualStateGroupsStatic(spRoot.Get(), &spGroups));
            IFC_RETURN(spGroups->GetAt(groupIndex, &spGroup));
        }

        // Get VisualState from name or VisualStateToken
        if(visualStateToken.IsEmpty() && !WindowsIsStringEmpty(stateName.Get()))
        {
            IFC_RETURN(VisualStateManager::TryGetState(pControl, WindowsGetStringRawBuffer(hStateName, nullptr), &spGroup, &spState, &bIgnored));
        }
        else
        {
            visualState = CVisualStateManager2::GetVisualState(static_cast<CControl*>(static_cast<Control*>(pControl)->GetHandle()), visualStateToken);
            if(visualState)
            {
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(visualState.get(), &spVisualStatePeer));
                spState = spVisualStatePeer.Cast<VisualState>();
                IFC_RETURN(spState->get_Name(stateName.ReleaseAndGetAddressOf()));
            }
        }

        // Call GoToStateCore on CustomVisualStateManager and return
        return (spCustomVsm.Cast<VisualStateManager>()->GoToStateCoreProtected(pControl, spRoot.Get(), stateName.Get(), spGroup.Get(), spState.Get(), bUseTransitions, pbReturnValue));
    }

    // Now, call GoToElementStateCore on our root. However, our default implementation of GoToElementStateCoreImpl
    // needs to know the control that GoToState is being called on, so it can eventually raise events on the VSGs.
    // So, we call InvokeGoToElementStateWithControl and pass in the IControl. The ref to this control is guaranteed
    // by InvokeGoToElementStateWithControl to not extend past the method call.
    if (WindowsIsStringEmpty(stateName.Get()))
    {
        auto name = CVisualStateManager2::GetVisualStateName(static_cast<CControl*>(static_cast<Control*>(pControl)->GetHandle()), visualStateToken);
        stateName.Set(name.GetBuffer());
    }

    if(!WindowsIsStringEmpty(stateName.Get()))
    {
        IFC_RETURN(spRoot.Cast<FrameworkElement>()->InvokeGoToElementStateWithControl(
            pControl,
            stateName.Get(),
            bUseTransitions,
            &bWentToState));
    }

    *pbReturnValue = bWentToState;
    return S_OK;
}

// Prepares the batch context to change VisualSates of a control
VisualStateManagerBatchContext::VisualStateManagerBatchContext(_In_ xaml_controls::IControl* pControl)
    : m_isContextInitialized(FALSE), m_spControl(pControl)
{
}

// Gets the required interfaces to change the visual state of the control
_Check_return_ HRESULT VisualStateManagerBatchContext::Initialize()
{
    HRESULT hr = S_OK;

    IFC(static_cast<Control*>(m_spControl.Get())->get_ImplementationRoot(&m_spRoot));
    if (m_spRoot)
    {
        IFC(VisualStateManagerFactory::GetCustomVisualStateManagerStatic(m_spRoot.Get(), &m_spCustomVsm));
        if (m_spCustomVsm)
        {
            // In the case of a Custom VSM, we also need the groups
            IFC(VisualStateManagerFactory::GetVisualStateGroupsStatic(m_spRoot.Get(), &m_spGroups));
        }
    }
    m_isContextInitialized = TRUE;

Cleanup:
    RRETURN(hr);
}

// Goes to a VisualState for the controles which was passed to the constructor
_Check_return_ HRESULT VisualStateManagerBatchContext::GoToState(
    _In_ BOOLEAN bUseTransitions,
    _In_reads_(cStateName + 1) _Null_terminated_ CONST WCHAR* pszStateName,
    _In_ size_t cStateName,
    _Out_opt_ BOOLEAN* pbReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN bWentToState = FALSE;

    if (pbReturnValue)
    {
        *pbReturnValue = FALSE;
    }

    if (!m_isContextInitialized)
    {
        IFC(Initialize());
    }

    if (m_spRoot)
    {
        wrl_wrappers::HStringReference strState(pszStateName, cStateName);
        if (m_spCustomVsm)
        {
            if (m_spGroups)
            {
                BOOLEAN found = FALSE;
                ctl::ComPtr<IVisualState> spState;
                ctl::ComPtr<IVisualStateGroup> spGroup;

                IFC(VisualStateManager::TryGetState(m_spControl.Get(), pszStateName, &spGroup, &spState, &found));
                if (found)
                {
                    IFC(m_spCustomVsm.Cast<VisualStateManager>()->GoToStateCoreProtected(m_spControl.Get(),
                        m_spRoot.Get(),
                        strState.Get(),
                        spGroup.Get(),
                        spState.Get(),
                        bUseTransitions,
                        &bWentToState));
                }
            }
        }
        else
        {
            // Now, call GoToElementStateCore on our root. However, our default implementation of GoToElementStateCoreImpl
            // needs to know the control that GoToState is being called on, so it can eventually raise events on the VSGs.
            // So, we call InvokeGoToElementStateWithControl and pass in the IControl. The ref to this control is guaranteed
            // by InvokeGoToElementStateWithControl to not extend past the method call.
            IFC(m_spRoot.Cast<FrameworkElement>()->InvokeGoToElementStateWithControl(
                m_spControl.Get(),
                strState.Get(),
                bUseTransitions,
                &bWentToState));
        }
    }

    if (pbReturnValue)
    {
        *pbReturnValue = bWentToState;
    }

Cleanup:
    return hr;
}

// Helper which will try to go to pszPrimaryState if bTryPrimaryState is true.
// If that cannot happen, it will attempt to go to pszFallbackState.
_Check_return_ HRESULT VisualStateManagerBatchContext::GoToStateWithFallback(
    _In_ bool useTransitions,
    _In_ BOOLEAN tryPrimaryState,
    _In_reads_(cPrimaryStateName + 1) _Null_terminated_ CONST WCHAR* pszPrimaryStateName,
    _In_ size_t cPrimaryStateName,
    _In_reads_(cFallbackStateName + 1) _Null_terminated_ CONST WCHAR* pszFallbackStateName,
    _In_ size_t cFallbackStateName,
    _Out_ BOOLEAN* used)
{
    HRESULT hr = S_OK;
    IFCPTR(used);
    *used = FALSE;

    if (tryPrimaryState)
    {
        IFC(GoToState(useTransitions, pszPrimaryStateName, cPrimaryStateName, used));
    }
    if (!*used)
    {
        IFC(GoToState(useTransitions, pszFallbackStateName, cFallbackStateName, used));
    }

Cleanup:
    RRETURN(hr);
}

