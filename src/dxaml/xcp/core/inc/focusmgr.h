// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XYFocus.h>
#include <vector>
#include <FocusRectManager.h>
#include <FocusObserver.h>
#include <FocusMovement.h>
#include "CoreAsyncAction.h"

#include "CControl.h"

class CTextElement;

class CFocusedElementRemovedEventArgs;
class CContentRoot;

struct FindFocusOptions
{
    FindFocusOptions(DirectUI::FocusNavigationDirection direction, bool queryOnly)
    {
        this->direction = direction;
        this->queryOnly = queryOnly;
    }

    FindFocusOptions(DirectUI::FocusNavigationDirection direction)
    {
        this->direction = direction;
        this->queryOnly = true;
    }

    const DirectUI::FocusNavigationDirection GetDirection() const
    {
        return direction;
    }

    const bool IsQueryOnly() const
    {
        return queryOnly;
    }

private:
    DirectUI::FocusNavigationDirection direction;
    bool queryOnly;
};

class CFocusManager final
{
public:
    CFocusManager(_In_ CCoreServices *pCoreService, _In_ CContentRoot& contentRoot);
    ~CFocusManager();

    CFocusManager& operator =(const CFocusManager&);

    XUINT32 AddRef();
    XUINT32 Release();

    void SetFocusObserver(_In_ std::unique_ptr<FocusObserver> focusObserver);

    CDependencyObject* GetFocusedElementNoRef() const;
    CDependencyObject* GetFirstFocusableElement(_In_ CDependencyObject *pStartElement)
    {
        return GetFirstFocusableElement(pStartElement, NULL /* focusCandidate */);
    }
    CDependencyObject* GetLastFocusableElement(_In_ CDependencyObject *pStartElement)
    {
        return GetLastFocusableElement(pStartElement, NULL /* focusCandidate */);
    }

    xref_ptr<CAutomationPeer> GetFocusedAutomationPeer();
    void SetFocusedAutomationPeer(_In_opt_ CAutomationPeer* pAP)
    {
        m_pFocusedAutomationPeer = xref::get_weakref(pAP);
    }

    void SetPluginFocusStatus(_In_ bool pluginFocused);
    bool IsPluginFocused() const { return m_bPluginFocused; }

    _Check_return_ const Focus::FocusMovementResult SetFocusedElement(_In_ const Focus::FocusMovement& movement);


    _Check_return_ HRESULT SetFocusOnNextFocusableElement(DirectUI::FocusState focusState, bool shouldFireFocusedRemoved = false, InputActivationBehavior inputActivationBehavior = InputActivationBehavior::NoActivate);

    // Release resources held by FocusManager's FocusRectManager. These
    // elements are automatically created on CFrameworkManager::UpdateFocus()
    // and must be released before core releases its main render target on
    // shutdown. Exposed by fixing core leak RS1 bug #7300521.
    void ReleaseFocusRectManagerResources();
    void ClearFocus();
    void CleanupDeviceRelatedResources(bool cleanupDComp);

    CDependencyObject* GetFirstFocusableElementFromRoot(_In_ bool bReverse);

    bool IsMovingFocusToNextTabStop() const
    {
        return m_isMovingFocusToNextTabStop;
    }

    bool IsMovingFocusToPreviousTabStop() const
    {
        return m_isMovingFocusToPreviousTabStop;
    }

    bool CanProcessTabStop(_In_ bool isShiftPressed);
    bool CanTabOutOfPlugin() { return m_bCanTabOutOfPlugin; }

    void SetCanTabOutOfPlugin(_In_ bool bCanTabOutOfPlugin) { m_bCanTabOutOfPlugin = bCanTabOutOfPlugin; }
    _Check_return_ HRESULT ProcessTabStop(_In_ bool bPressedShift, _Out_ bool* bHandled);

    CUIElement* GetFirstFocusableElement();
    CUIElement* GetNextFocusableElement();

    CDependencyObject* GetNextTabStop(_In_opt_ CDependencyObject* pCurrentTabStop = NULL, _In_ bool bIgnoreCurrentTabStopScope = false);
    CDependencyObject* GetPreviousTabStop(_In_opt_ CDependencyObject* pCurrentTabStop = NULL);

    _Check_return_ HRESULT NotifyFocusChanged(_In_ bool bringIntoView, _In_ bool animateIfBringIntoView = false);

    _Check_return_ HRESULT TryMoveFocus(_In_ DirectUI::FocusNavigationDirection focusNavigationDirection, _Out_ bool* pbMoved);

    bool FindAndSetNextFocus(_In_ DirectUI::FocusNavigationDirection direction);
    _Check_return_ Focus::FocusMovementResult FindAndSetNextFocus(_In_ const Focus::FocusMovement& movement);

    bool NavigatedToByEngagingControl(_In_ CDependencyObject* pFocused);
    CDependencyObject* FindNextFocus(
        _In_ DirectUI::FocusNavigationDirection direction);
    CDependencyObject* FindNextFocus(_In_ const FindFocusOptions& findFocusOptions,
            _In_ Focus::XYFocusOptions& xyFocusOptions,
            _In_opt_ CDependencyObject* component = nullptr,
            _In_opt_ bool updateManifolds = true);

    CDependencyObject* GetTabStopCandidateElement(_In_ bool isShiftPressed, _In_ bool queryOnly, _Out_ bool& didCycleFocusAtRootVisualScope);

public:
    bool IsFocusedElementInPopup() { return m_pFocusedElement && GetRootOfPopupSubTree(m_pFocusedElement); }

    bool IsFocusable(_In_ CDependencyObject* pObject, bool ignoreOffScreenPosition = false);

    FocusObserver* GetFocusObserverNoRef();

    bool CanHaveFocusableChildren(_In_ CDependencyObject *pParent);

    xref::weakref_ptr<CUIElement> GetFocusRectangleUIElement() const
    {
        return m_focusRectangleUIElement;
    }

    void SetFocusRectangleUIElement(_In_ CUIElement* newFocusRectangle)
    {
        m_focusRectangleUIElement = xref::get_weakref(newFocusRectangle);
    }

    DirectUI::FocusState GetRealFocusStateForFocusedElement(){return m_realFocusStateForFocusedElement;}

    CControl* GetEngagedControlNoRef() const
    {
        return m_spEngagedControl.get();
    }

    void SetEngagedControl(_In_ CControl *pCntrl)
    {
        m_spEngagedControl = pCntrl;
    }

    CContentRoot* GetContentRootNoRef() const
    {
        return &m_contentRoot;
    }

    void SetFocusVisualDirty();

    Focus::XYFocus& GetXYFocus() { return m_xyFocus; }

    static CUIElement* GetFocusTargetDescendant(_In_ CUIElement* element);
    CDependencyObject* GetFocusTarget();

    void UpdateFocusRect(_In_ const DirectUI::FocusNavigationDirection focusNavigationDirection, _In_ bool cleanOnly = false);
    void OnFocusedElementKeyPressed() const;
    void OnFocusedElementKeyReleased() const;
    void RenderFocusRectForElementIfNeeded(_In_ CUIElement* element, _In_ IContentRenderer* renderer);

    void SetInitialFocus(_In_ bool status) { m_initialFocus = status; }
    bool IsInitialFocus() const { return m_initialFocus; }

    CTextElement* GetTextElementForFocusRectCandidate() const;

    _Check_return_ HRESULT OnAccessKeyDisplayModeChanged() const;

    void RaiseNoFocusCandidateFoundEvent(_In_ DirectUI::FocusNavigationDirection navigationDirection);

    bool RaiseAndProcessGettingAndLosingFocusEvents(
        _In_ CDependencyObject* const pOldFocus,
        _Inout_opt_ CDependencyObject** pFocusTarget,
        _In_ DirectUI::FocusState focusState,
        _In_ DirectUI::FocusNavigationDirection focusNavigationDirection,
        _In_ const bool focusChangeCancellable,
        _In_ const GUID correlationId);


    void FireAutomationFocusChanged();

    static DirectUI::FocusState GetFocusStateFromInputDeviceType(DirectUI::InputDeviceType inputDeviceType);

    void ClearXYFocusCache() { m_xyFocus.ClearCache(); }

    bool TrySetAsyncOperation(_In_ ICoreAsyncFocusOperation* asyncOperation);
    void CancelCurrentAsyncOperation(_In_ const Focus::FocusMovementResult& result);

    void SetIgnoreFocusLock(_In_ bool value) { m_ignoreFocusLock = value; }
    void SetCurrentFocusOperationCancellable(_In_ bool value) { m_currentFocusOperationCancellable = value; }

    _Check_return_ HRESULT SetWindowFocus(
        _In_ const bool isFocused,
        _In_ const bool isShiftDown);

private:
    // ------------------------------------------------------------------------
    // CFocusManager Private Methods
    // ------------------------------------------------------------------------
    _Check_return_ HRESULT ProcessTabStopInternal(_In_ bool bPressedShift, _In_ bool queryOnly, _Outptr_ CDependencyObject** ppNewTabStopElement);

    CDependencyObject* GetFirstFocusableElement(_In_ CDependencyObject* pSearchStart, _In_opt_ CDependencyObject *pFirstFocus);
    CDependencyObject* GetLastFocusableElement(_In_ CDependencyObject* pSearchStart, _In_opt_ CDependencyObject *pLastFocus);
    CDependencyObject* GetNextTabStopInternal(_In_ CDependencyObject *pParent, _In_ CDependencyObject *pCurrent, _In_ CDependencyObject *pCandidate, _Inout_ bool *bCurrentPassed, _Inout_ CDependencyObject **pCurrentCompare);
    CDependencyObject* GetPreviousTabStopInternal(_In_ CDependencyObject *pParent, _In_ CDependencyObject *pCurrent, _In_ CDependencyObject *pCandidate, _Inout_ bool *bCurrentPassed, _Inout_ CDependencyObject **pCurrentCompare);
    XINT32 CompareTabIndex(_In_ CDependencyObject *pObject1, _In_ CDependencyObject *pObject2);
    bool IsFocusOnFirstTabStop();
    bool IsFocusOnLastTabStop();
    CUIElement* GetParentElement(_In_ CDependencyObject *pCurrent);

    CDependencyObject* GetFirstFocusableElementInternal(_In_ CDependencyObject* pSearchStart, _In_opt_ CDependencyObject *pFirstFocus);
    CDependencyObject* GetLastFocusableElementInternal(_In_ CDependencyObject* pSearchStart, _In_opt_ CDependencyObject *pLastFocus);

    // The following allows general FocusManager functions to stay agnostic to
    // the specific element's inheritance hierarchy.
    bool IsVisible(_In_ CDependencyObject *pObject);
    bool CanHaveChildren(_In_ CDependencyObject *pObject);
    CDependencyObject* GetFocusParent(_In_ CDependencyObject *pObject);
    XINT32 GetTabIndex(_In_ CDependencyObject *pObject);
    CDependencyObject* GetRootOfPopupSubTree(_In_ CDependencyObject *pObject);
    DirectUI::KeyboardNavigationMode GetTabNavigation(_In_ CDependencyObject *pObject) const;
    bool IsPotentialTabStop(_In_ CDependencyObject *pObject) const;
    _Check_return_ HRESULT RaiseLostFocusEvent(CDependencyObject* pLostFocusElement, GUID correlationId);
    _Check_return_ HRESULT RaiseGotFocusEvent(CDependencyObject* pGotFocusElement, GUID correlationId);

    template <class ChangingFocusEventArgs>
    bool RaiseChangingFocusEvent(
        _In_ CDependencyObject* const pLosingFocusElement,
        _In_ CDependencyObject* pGettingFocusElement,
        _In_ DirectUI::FocusState newFocusState,
        _In_ DirectUI::FocusNavigationDirection navigationDirection,
        _In_ KnownEventIndex index,
        _In_ GUID correlationId,
        _Outptr_result_maybenull_ CDependencyObject** pFinalGettingFocusElement);

    CDependencyObject* RaiseFocusElementRemovedEvent(_In_ CDependencyObject* focusedElement);
    bool CanRaiseFocusEventChange();
    bool FocusedElementIsBehindFullWindowMediaRoot() const;

    bool ShouldUpdateFocus(
        _In_opt_ CDependencyObject *pNewFocus,
        _In_ DirectUI::FocusState focusState)  const;

    bool IsValidTabStopSearchCandidate(
        _In_ CDependencyObject* const element) const;

    _Check_return_ Focus::FocusMovementResult UpdateFocus(_In_ const Focus::FocusMovement& movement);

    DirectUI::FocusState CoerceFocusState(_In_ DirectUI::FocusState focusState) const;

    bool ShouldSetWindowFocus(const Focus::FocusMovement& movement) const;

    // ------------------------------------------------------------------------
    // CFocusManager Private Fields
    // ------------------------------------------------------------------------
private:
    CContentRoot& m_contentRoot;
    CDependencyObject *m_pFocusedElement;
    xref::weakref_ptr<CAutomationPeer> m_pFocusedAutomationPeer;
    xref_ptr<CDependencyObject> m_focusTarget;
    CCoreServices *m_pCoreService;
    xref::weakref_ptr<CUIElement> m_focusRectangleUIElement;
    bool m_bPluginFocused;
    DirectUI::FocusState m_realFocusStateForFocusedElement{};
    std::unique_ptr<FocusObserver> m_focusObserver;

    Focus::XYFocus m_xyFocus;

    // When running under platforms with support to tab out of plug-ins, we
    // want to NOT handle the "last" tab so the user can tab out of Silverlight.

    // When the platform cannot support tabbing out of plugin, we will act as if
    // there is an implicit top-level TabNavigation="Cycle"

    // So instead of tabbing out, we handle the "last" tab by cycling to the
    // "first" tab.  This is deemed better than getting stuck at the end.

    // (Reverse "first" and "last" for Shift-Tab.)
    bool m_bCanTabOutOfPlugin;

    bool m_isPrevFocusTextControl;

    bool m_isMovingFocusToNextTabStop = false;
    bool m_isMovingFocusToPreviousTabStop = false;

    xref_ptr<CControl> m_spEngagedControl;

    // focusRectManager is responsible for drawing the focus rectangle
    CFocusRectManager m_focusRectManager;

    bool m_focusLocked = false;

    // During Window Activation/Deactivation, we lock focus. However, focus can move internally via the focused element becoming
    // unfocusable (ie. leaving the tree or changing visibility). This member will force focus manager to bypass the m_focusLocked logic.
    // This should be used with care... if used irresponsibly, we can enable unsupported reentrancy scenarios.
    bool m_ignoreFocusLock = false;

    // It is possible to continue with a focus operation, even though focus is locked. In this case, we need to ensure that we persist the fact that focus should not be canceled.
    bool m_currentFocusOperationCancellable = true;

    bool m_initialFocus = false;

    // This represents the IAsyncOperation that can initiated through a public async FocusManager method, such as TryFocusAsync.
    // We store it as a memeber because the operation can continue to run even after the api has finished executing.
    ICoreAsyncFocusOperation* m_asyncOperation = nullptr;
}; // CFocusManger class


