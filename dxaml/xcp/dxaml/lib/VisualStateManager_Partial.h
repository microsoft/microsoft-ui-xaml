// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VisualStateManager.g.h"
class VisualStateToken;

namespace DirectUI
{
    PARTIAL_CLASS(VisualStateManager)
    {
        friend class VisualStateManagerFactory;
        friend class VisualStateManagerBatchContext;

    public:
        _Check_return_ HRESULT RaiseCurrentStateChangingImpl(
            _In_ xaml::IVisualStateGroup* pStateGroup,
            _In_ xaml::IVisualState* pOldState,
            _In_ xaml::IVisualState* pNewState,
            _In_ xaml_controls::IControl* pControl);

        _Check_return_ HRESULT RaiseCurrentStateChangedImpl(
            _In_ xaml::IVisualStateGroup* pStateGroup,
            _In_ xaml::IVisualState* pOldState,
            _In_ xaml::IVisualState* pNewState,
            _In_ xaml_controls::IControl* pControl);

        _Check_return_ HRESULT GoToStateCoreImpl(
            _In_ xaml_controls::IControl* pControl,
            _In_ xaml::IFrameworkElement* pTemplateRoot,
            _In_ HSTRING hStateName,
            _In_ xaml::IVisualStateGroup* pGroup,
            _In_ xaml::IVisualState* pState,
            _In_ BOOLEAN bUseTransitions,
            _Out_ BOOLEAN* pbReturnValue);

        static _Check_return_ HRESULT GoToStateImpl(
            _In_ xaml_controls::IControl* pControl,
            _In_ VisualStateToken visualStateToken,
            _In_ HSTRING hStateName,
            _In_ int groupIndex,
            _In_ BOOLEAN bUseTransitions,
            _Out_ BOOLEAN* pbReturnValue);

        static _Check_return_ HRESULT GoToState(
            _In_ xaml_controls::IControl* pControl,
            _In_ HSTRING hStateName,
            _In_ BOOLEAN bUseTransitions,
            _Out_ BOOLEAN* pbReturnValue);

        static _Check_return_ HRESULT TryGetState(
            _In_ xaml_controls::IControl* pControl,
            _In_ _Null_terminated_ const WCHAR* pszStateName,
            _Outptr_result_maybenull_ xaml::IVisualStateGroup** ppGroup,
            _Outptr_result_maybenull_ xaml::IVisualState** ppState,
            _Out_ BOOLEAN* pbReturnValue);

        // Use the default non-custom VSM implementation to process a state change on the given control
        // and implementation root. It is called by the base implementation of the protected virtual
        // method FrameworkElement.GoToElementStateCore on pImplementationRoot.
        static _Check_return_ HRESULT GoToStateWithDefaultVSM(
            _In_ xaml_controls::IControl* pControl,
            _In_ xaml::IFrameworkElement* pImplementationRoot,
            _In_ HSTRING hStateName,
            _In_ BOOLEAN bUseTransitions,
            _Out_ BOOLEAN* pbReturnValue);

        // Callback from core that receives state changes initiated by the StateTrigger system.
        // All calls must be routed through the GoToState call to support overrides
        // from CustomVisualStateManager.
        static _Check_return_ HRESULT CustomVSMGoToState(
            _In_ CDependencyObject* pControl,
            _In_ VisualStateToken token,
            _In_ int groupIndex,
            _In_ bool bUseTransitions,
            _In_ bool* bSucceeded);

    private:
        static _Check_return_ HRESULT GoToStatePrivate(
            _In_ xaml_controls::IControl* pControl,
            _In_ HSTRING hStateName,
            _In_ xaml::IVisualState* pState,
            _In_ xaml::IVisualStateGroup* pGroup,
            _In_ BOOLEAN bUseTransitions,
            _Out_ BOOLEAN* pbRefreshInheritanceContext,
            _Out_ BOOLEAN* pbReturnValue);

        // This method handles a special case where the Storyboard has a 0 duration, and it contains a child with a DependencyObject Binding attached to it.
        // In this case, we need to synchronously refresh the InheritanceContext of the child before Begin is called on the Storyboard.  VisualStateGroup
        // automatically errors out in this scenario so the InheritanceContext can be refreshed synchronously at a safe time to avoid reentrancy issues.  Here
        // we handle the refresh and retry GoToState which should succeed.
        static _Check_return_ HRESULT RetryGoToStateAfterRefreshingInheritanceContext(
            _In_ xaml_controls::IControl* pControl,
            _In_ xaml::IFrameworkElement* pTemplateRoot,
            _In_ xaml::IVisualStateGroup* pGroup,
            _In_ xaml::IVisualState* pState,
            _In_ BOOLEAN bUseTransitions,
            _Out_ BOOLEAN* pbReturnValue);
    };

    // Caches the interfaces common to successive calls to GoToState
    // In addition, defers their initialization to be compatible with NoOp detection in ChangeVisualStateOverrides
    class VisualStateManagerBatchContext
    {
    public:
        // Prepares the batch context to change VisualSates of a control
        VisualStateManagerBatchContext(_In_ xaml_controls::IControl* pControl);

        // Goes to a VisualState for the controles which was passed to the constructor
        _Check_return_ HRESULT GoToState(
            _In_ BOOLEAN bUseTransitions,
            _In_reads_(cStateName + 1) _Null_terminated_ CONST WCHAR* pszStateName,
            _In_ size_t cStateName,
            _Out_opt_ BOOLEAN* pbReturnValue = nullptr);

        // Helper which will try to go to pszPrimaryState if bTryPrimaryState is true.
        // If that cannot happen, it will attempt to go to pszFallbackState.
        _Check_return_ HRESULT GoToStateWithFallback(
            _In_ bool useTransitions,
            _In_ BOOLEAN tryPrimaryState,
            _In_reads_(cPrimaryStateName + 1) _Null_terminated_ CONST WCHAR* pszPrimaryStateName,
            _In_ size_t cPrimaryStateName,
            _In_reads_(cFallbackStateName + 1) _Null_terminated_ CONST WCHAR* pszFallbackStateName,
            _In_ size_t cFallbackStateName,
            _Out_ BOOLEAN* used);

    private:
        // Boolean used for deferred initialization
        BOOLEAN m_isContextInitialized;

        // Interfaces needed for state changes
        ctl::ComPtr<xaml_controls::IControl> m_spControl;
        ctl::ComPtr<xaml::IFrameworkElement> m_spRoot;
        ctl::ComPtr<xaml::IVisualStateManager> m_spCustomVsm;
        ctl::ComPtr<wfc::IVector<xaml::VisualStateGroup*>> m_spGroups;

        // Gets the required interfaces to change the visual state of the control
        _Check_return_ HRESULT Initialize();
    };

}
