// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Framework.h"
#include "Panel.h"
#include "UIAWindow.h"
#include <UIElementStructs.h>
#include <XYFocus.h>
#include <fwd/windows.ui.composition.h>
#include <Microsoft.UI.Content.h>
#include <Microsoft.UI.Input.Partner.h>

class HitTestPolygon;
class CKeyEventArgs;

namespace Theming {
    enum class Theme : uint8_t;
}

class IslandWindowedPopup;
class WindowedPopupInputSiteAdapter;
// Xaml implicitly closes popups in situations like changing the child or un-parenting from the tree.
// These close operations shouldn't cause UIE.Hidden to be raised, so they shouldn't request a KeepAlive count.
class CPopup;
struct ImplicitCloseGuard
{
public:
    ImplicitCloseGuard(_In_ CPopup* popup);
    ~ImplicitCloseGuard();

private:
    CPopup* m_popupNoRef;
};

//------------------------------------------------------------------------
//
//  Class:  CPopup
//
//  Used for UI that needs to appear on a separate "plane" on top of the
//  rest of the XAML content.
//
//  When the popup is opened, its content is added as a child of CPopupRoot.
//  CPopupRoot manages the popup in its list of open popups, and layout
//  and render of the content occurs as a child of CPopupRoot. When the
//  popup is closed, its content is removed as a child of CPopupRoot.
//
//  A popup can optionally be marked as windowed. A windowed popup displays
//  its content in a WS_POPUP window, instead of the Jupiter window, so is
//  not clipped to the Jupiter window. This is useful for context menus and
//  tooltips that don't want to be clipped to the Jupiter window. Windowed
//  popups are only supported on Desktop, because OneCore and Phone don't
//  support non-fullscreen windows. Controls that use windowed popups must
//  fallback to non-windowed placement on unsupported platforms.
//    - The popup window is positioned at the same location as the popup, so
//      can use standard hit testing code.
//    - The window does not take window activation or focus, so all keyboard
//      input goes directly to the Jupiter window, and can use the standard
//      focus manager and navigation code.
//    - Pointer input will go to the popup's window and is forwarded to the
//      Jupiter window. Pointer window messages provide the pointer position in
//      screen coordinates. Since the popup window is positioned at the
//      same screen location, the standard input manager code can be used in
//      most cases. Exceptions include PointerRoutedEventArgs.CurrentPoint,
//      which needs to be transformed from the pointer's target window to the
//      root window (see DxamlCore::GetTranslationFromTargetWindowToRootWindow),
//      and initialization of DirectManipulationContainer, which needs to
//      use the popup's island input site's window (see 
//      CInputServices::InitializeDirectManipulationContainer's usage of
//      CDependencyObject::GetElementIslandInputSite.)
//    - Windowed popups must be closed when the Jupiter window is moved, because
//      they will no longer be positioned at the location of the popup element.
//    - For rendering, a windowed popup uses HWWindowedPopupCompTreeNode, which
//      will provide the DComp visual to be set into the window's target (see
//      CPopup::SetRootVisualForWindowedPopupWindow.) HWWindowedPopupCompTreeNode
//      also inserts a placeholder in the main DComp tree, to maintain child
//      ordering with sibling visuals
//    - For UIA support, a windowed popup creates its own CUIAHostWindow, whose
//      root visual is the popup's content, and returns it through the window's
//      WM_GETOBJECT. CUIAHostWindow will use the Jupiter window for screen/client
//      transforms, because the layout of all elements is relative to the Jupiter
//      window -- see CUIAWindow::m_transformWindow.
//    - A windowed popup should be parentless. This is because transforms
//      between the popup and the visual tree root will not be applied while
//      positioning the popup window and setting the DComp visual to the window
//      target.
//    - The windowed popup cannot contain other popups, because nested popups
//      will still be attached to the main visual tree, so will be covered
//      by the windowed popup.
//    - A windowed popup cannot be made windowless, to simplify implementation.
//
//------------------------------------------------------------------------

class CPopup : public CFrameworkElement
{
protected:
    friend class CPopupRoot;
    friend class HWWalk;
    friend struct ImplicitCloseGuard;

    CPopup(_In_ CCoreServices *pCore);

private:
    void RaisePopupEvent(EventHandle hEvent);

    _Check_return_ HRESULT AddChildAsNamescopeOwner();

    _Check_return_ HRESULT RemoveChildAsNamescopeOwner();

    bool ShouldRemoveChildAsNamescopeOwner();

    _Check_return_ HRESULT Open();
    _Check_return_ HRESULT Close(bool forceCloseforTreeReset = false);
    void ForceCloseForTreeReset();

    _Check_return_ HRESULT SetChild(_In_ CUIElement* pChild);
    _Check_return_ HRESULT RemoveChild();

    _Check_return_ HRESULT ClearUCRemoveLogicalParentFlag(_In_ CUIElement* pChild);

    _Check_return_ HRESULT UpdateTranslationFromContentRoot(const wf::Point& offset, bool forceUpdate = false);

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;

    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    _Check_return_ HRESULT OnKeyDown(
        _In_ CEventArgs* pEventArgs) override;

     _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

    bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer) override;

public:
    // TODO: If the XcpList on PopupRoot goes away this shouldn't need to be public any longer
    ~CPopup() override;

    DECLARE_CREATE(CPopup);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPopup>::Index;
    }

    bool AllowsHandlerWhenNotLive(XINT32 iListenerType, KnownEventIndex eventIndex) const final
    {
        return true;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peer has state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    CPopupRoot* GetAssociatedPopupRootNoRef();

    CXamlIslandRoot* GetAssociatedXamlIslandNoRef();

    CDependencyObject* GetToolTipOwner();

    static _Check_return_ HRESULT Child(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    void ReleaseOverride() final;

    void AsyncRelease();

    bool IsOpen() const { return m_fIsOpen; }
    bool IsFlyout() const { return m_associatedFlyoutWeakRef.lock_noref() != nullptr; }
    bool IsApplicationBarService() const { return m_fIsApplicationBarService; }
    bool IsContentDialog() const { return m_fIsContentDialog; }
    bool IsSubMenu() const { return m_fIsSubMenu; }
    bool IsToolTip() const
    {
        if(!m_pChild) return false;
        return m_pChild->GetTypeIndex() == KnownTypeIndex::ToolTip;
    }
    bool WasOpenedDuringEngagement() const { return m_wasOpenedDuringEngagement; }
    void SetOpenedDuringEngagement(_In_ bool value) { m_wasOpenedDuringEngagement = value; }

    // We'll treat a popup associated with a sub-menu as light-dismiss
    // because we know that the parent menu of a sub-menu is always light-dismiss.
    bool IsSelfOrAncestorLightDismiss() { return m_fIsLightDismiss || m_fIsSubMenu; }

    CFlyoutBase* GetAssociatedFlyoutNoRef() { return m_associatedFlyoutWeakRef.lock_noref(); }

    _Check_return_ HRESULT UpdateImplicitStyle(
        _In_opt_ CStyle *pOldStyle,
        _In_opt_ CStyle *pNewStyle,
        bool bForceUpdate,
        bool bUpdateChildren = true,
        bool isLeavingParentStyle = false
        ) final;

    static CPopup *GetClosestPopupAncestor(_In_opt_ CUIElement *pUIElement);

    static CFlyoutBase *GetClosestFlyoutAncestor(_In_opt_ CUIElement *pUIElement);

    bool IsFocusable() final
    {
        // We don't check IsActive() because we want the focus logic to also work with unrooted popups,
        // which are never in the live tree.
        return m_fIsLightDismiss &&
            IsVisible() &&
            IsEnabled() &&
            AreAllAncestorsVisible();
    }

    void SetFocusStateAfterClosing(_In_ DirectUI::FocusState focusState)
    {
        m_focusStateAfterClosing = focusState;
    }

    // Whether this element should skip rendering when it is the target of a layout transition.
    bool SkipRenderForLayoutTransition() override;

    void FlushPendingKeepVisibleOperations() override;

    void CancelHideAnimationToPrepareForShow();

    bool ShouldPopupRootNotifyThemeChange();

    _Check_return_ HRESULT RecursiveInvalidateFontSize() override;

    void SetShouldTakeFocus(_In_ bool shouldTakeFocus)
    {
        m_shouldTakeFocus = shouldTakeFocus;
    }

    _Check_return_ HRESULT SetFocus(_In_ DirectUI::FocusState focusState)
    {
        bool focusChanged;
        IFC_RETURN(Focus(focusState, false /*animateIfBringIntoView*/, &focusChanged));
        return S_OK;
    }

    void SetPointerCapture();
    void ReleasePointerCapture();
    bool HasPointerCapture() const;

    _Check_return_ HRESULT SetIsWindowedIfNeeded();
    _Check_return_ HRESULT SetIsWindowed();

    bool WasEverOpened() const { return m_everOpened; }
    bool IsWindowed() const { return !!m_isWindowed; }
    bool WindowedPopupHasFocus() const;

    HWND GetPopupPositioningWindow() const;

    ixp::IVisual* GetWindowRootVisual_TestHook();

    _Check_return_ HRESULT SetRootVisualForWindowedPopupWindow(_In_ ixp::IVisual* popupRootVisual);
    void AddAdditionalVisualForWindowedPopupWindow(_In_ ixp::IVisual* popupVisual);
    void RemoveAdditionalVisualForWindowedPopupWindow(_In_ ixp::IVisual* popupVisual);

    void SetIsOpen(bool value) { m_fIsOpen = value; }

    static bool DoesPlatformSupportWindowedPopup(_In_ CCoreServices* pCore);

    _Check_return_ HRESULT SetOverlayThemeBrush(_In_ const xstring_ptr& brushKey);

    static _Check_return_ HRESULT AssociatedFlyout(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT GetOverlayInputPassThroughElementNoRef(_Outptr_ CDependencyObject **ppElementNoRef);

    bool IsUnloading() const;

    // Used when an unloading child no longer needs to be kept visible.
    static CPopup* GetPopupOfUnloadingChild(_In_ CUIElement* child);
    void RemoveUnloadingChild();

    void SetAssociatedVisualTree(_In_opt_ VisualTree* tree);
    void SetAssociatedIsland(_In_opt_ CDependencyObject* island);

    void EnsureUIAWindow();
    CUIAHostWindow* GetUIAWindow() const { return m_spUIAWindow.get(); }

    CUIElement* GetChildNoRef() const { return m_pChild; }

    _Check_return_ HRESULT Reposition();

    ixp::IPopupWindowSiteBridge* GetPopupWindowBridgeNoRef() { return m_popupWindowBridge.Get(); }
    ixp::IContentIsland* GetContentIslandNoRef() { return m_contentIsland.Get(); }
    ixp::IDesktopSiteBridge* GetDesktopBridgeNoRef() { return m_desktopBridge.Get(); }

    bool ReplayPointerUpdate();
    void ClearLastPointerPointForReplay();
    wrl::ComPtr<ixp::IPointerPoint> GetPreviousPointerPoint();

    wrl::ComPtr<ixp::IIslandInputSitePartner> GetIslandInputSite() const;

    wf::Point GetTranslationFromMainWindow() { return m_offsetFromMainWindow; }

    _Check_return_ HRESULT GetScreenOffsetFromOwner(_Out_ XPOINTF_COORDS* offset);

    // Used for MockDComp dumping
    wgr::RectInt32 GetWindowedPopupMoveAndResizeRect() const { return m_windowedPopupMoveAndResizeRect; }

    float GetWindowedPopupRasterizationScale() const;

    // Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop implementation
    _Check_return_ HRESULT GetSystemBackdrop(_Outptr_result_maybenull_ RealWUComp::ICompositionBrush** systemBackdropBrush);
    _Check_return_ HRESULT SetSystemBackdrop(_In_opt_ RealWUComp::ICompositionBrush* systemBackdropBrush);

    xref_ptr<CTransitionTarget> EnsureTransitionTarget();

protected:
    bool NWSkipRendering() override
    {
        // CPopup no-ops during the regular software render walk.
        // It is rendered when the CPopupRoot is walked, after all other elements have rendered.
        return true;
    }

    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams
        ) override;

    _Check_return_ HRESULT MarkInheritedPropertyDirty(
        _In_ const CDependencyProperty* pdp,
        _In_ const CValue* pValue) override;

    void NWPropagateDirtyFlag(DirtyFlags flags) override;

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
public:
    template <typename HitType>
    _Check_return_ HRESULT BoundsTestPopup(
        _In_ const HitType& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

protected:
    _Check_return_ HRESULT BoundsTestInternal(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT BoundsTestInternal(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT GenerateChildOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        ) override;

    bool IgnoresInheritedClips() override { return true; }

    virtual _Check_return_ HRESULT OnClosing(_Out_ bool* cancel);

private:
    void LeavePCSceneSubgraph() final;
    void LeavePCScene();

    _Check_return_ HRESULT StartComposition();
    _Check_return_ HRESULT StopComposition();

    bool HasFocusedElementAsChild();

    // This method is called once per lifetime of the CPopup. Resources created here aren't released until the CPopup is
    // deleted.
    _Check_return_ HRESULT EnsureWindowForWindowedPopup();

    // These methods are called once per open/close for the CPopup. Resources created here will be released when the
    // CPopup is closed.
    _Check_return_ HRESULT EnsureDCompResourcesForWindowedPopup();
    void ReleaseDCompResourcesForWindowedPopup();

    void EnsureContentExternalBackdropLink();
    void DiscardContentExternalBackdropLink();

    _Check_return_ HRESULT ShowWindowForWindowedPopup();
    _Check_return_ HRESULT HideWindowForWindowedPopup();
    _Check_return_ HRESULT PositionAndSizeWindowForWindowedPopup();
    bool MeetsRenderingRequirementsForWindowedPopup();
    _Check_return_ HRESULT GetPhysicalBounds(_In_ CUIElement* element, _Out_ XRECTF* physicalBounds);
    _Check_return_ HRESULT AdjustWindowedPopupBoundsForDropShadow(_In_ const XRECTF* popupWindowBounds);
    bool ShouldPopupRendersDropShadow() const;
    XTHICKNESS GetInsetsForDropShadow();
    float GetEffectiveRootScale();
    float DipsToPhysicalPixels(_In_ float dips);
    float PhysicalPixelsToDips(_In_ float physicalPixels);
    void DipsToPhysicalPixelsRect(_Inout_ XRECTF* rect);

    _Check_return_ HRESULT ClosePopupOnKeyEventIfNecessary(_In_ CKeyEventArgs* eventArgs);

    _Check_return_ HRESULT ReevaluateIsOverlayVisible();
    _Check_return_ HRESULT SetOverlayElement(_In_ CFrameworkElement* overlayElement);
    _Check_return_ HRESULT RemoveOverlayElement();
    _Check_return_ HRESULT AddOverlayElementToPopupRoot();
    _Check_return_ HRESULT RemoveOverlayElementFromPopupRoot();

    static bool IsElementOrParentEngaged(_In_ CDependencyObject* const element);

    void EnsureWindowedPopupRootVisualTree();

    _Check_return_ HRESULT OnContentAutomationProviderRequested(
        _In_ ixp::IContentIsland* content,
        _In_ ixp::IContentIslandAutomationProviderRequestedEventArgs* e);

    void ApplyRootRoundedCornerClipToSystemBackdrop();

public:
    // When the popup is open, its child is parented directly to the popup root.
    CUIElement*                     m_pChild;
    CTransitionCollection*          m_pChildTransitions;
    CFrameworkElement*              m_overlayElement;

    // When the previous child needs to be kept visible as a result of changing Popup.Child, we store the
    // old child here. This allows it to keep rendering while it animates.
    xref_ptr<CUIElement> m_unloadingChild;

    // Owner associated with this popup instance
    // Required for popups hosting ToolTips, which have no proper parent
    xref::weakref_ptr<CDependencyObject> m_toolTipOwnerWeakRef;

    // Storage for Popup.HorizontalOffset/VerticalOffset
    // Note carefully that these properties are NOT considered part of the Popup's layout offset or local transform.
    // These properties must be pushed into Popup.Child because of parentless popups.  In this scenario,
    // layout jumps directly to Popup.Child.  This means these properties are applied as layout properties
    // on Popup.Child, and subsequently get picked up as part of Popup.Child's local transform in the RenderWalk.
    XFLOAT                          m_eHOffset;
    XFLOAT                          m_eVOffset;

    // In order to accommodate relatively-positioned popups, we'll shift absolutely-positioned popups out of the way
    // in the case where the former type of popup can't fit on either side of its placement target.  We don't want to
    // muck with values the app can set, so instead of modifying the above values, we'll add separate offsets that we
    // exclusively own to the above.
    XFLOAT                          m_hAdjustment;
    XFLOAT                          m_vAdjustment;

    // When the popup is opened and both Placement and PlacementTarget have been set, we'll calculate the
    // horizontal offset and vertical offset needed to place the popup at the right location to honor those
    // values.  These will be added to the values of HorizontalOffset and VerticalOffset and the above adjustments
    // to give us the final xy-position at which we'll place the popup.
    XFLOAT                          m_calculatedHOffset;
    XFLOAT                          m_calculatedVOffset;

    // m_fIsLightDismiss is only set when opening the popup if m_fIsLightDismissEnabled is TRUE at this time,
    // and is cleared when closing the popup.  If a popup is opened as light-dismiss-enabled, it will remain
    // light-dismiss-enabled as long as it is shown.  We don't want to handle changing light-dismiss-enabled
    // state for open popups.
    bool                           m_fIsLightDismiss;

    DirectUI::LightDismissOverlayMode m_lightDismissOverlayMode;

    bool                            m_fAsyncQueueOnRelease;

    // Normally, the overlay is only shown for light-dismiss popups, but for controls that roll their own
    // light-dismiss logic (and therefore configure their popup's to not be light-dismiss) we still want
    // to re-use the popup's overlay code.  This flag provides a mechanism to disable the check for
    // IsLightDismissEnabled.
    bool                            m_disableOverlayIsLightDismissCheck;
    bool                            m_fIsOpen;
    bool                            m_fIsContentDialog;
    bool                            m_fIsSubMenu;

    // Is this the ApplicationBarService popup, which is used to host app bars?
    bool                            m_fIsApplicationBarService;

    // m_fIsLightDismissEnabled can be set at any time when the customer sets IsLightDismissEnabled.
    bool                            m_fIsLightDismissEnabled;

    // Holds a value indicating whether or not the popup has been placed in its own window.
    bool                            m_isWindowed;

    // Remember the last island that opened this popup. A popup can move between islands between closing and reopening.
    // Xaml uses a single shared context menu for all its TextBoxes, for example. If this popup is associated with a
    // different island then the content bridge and input site adapter needs to be re-created.
    UINT64 m_previousXamlIslandId = 0;

    // The Popup.SystemBackdrop set on this popup. We will call its OnTargetDisconnected when the SystemBackdrop
    // property changes. Note that by the time we get to the CPopup::SetValue override, the new value has already
    // been set, so we can't do a GetValue lookup for the previous value, and we have to cache it ourselves.
    xref_ptr<CSystemBackdrop> m_systemBackdrop;

    DirectUI::FocusState GetSavedFocusState();

    void SetCachedStandardNamescopeOwner(_In_ CDependencyObject* obj);
    CDependencyObject* GetCachedStandardNamescopeOwnerNoRef();

    void EnsureBridgeClosed();

private:
    struct ReentrancyGuard
    {
    public:
        ReentrancyGuard(_In_ CPopup* popup) : m_popup(popup)
        {
            m_popup->m_isClosing = true;
        }

        ~ReentrancyGuard()
        {
            m_popup->m_isClosing = false;
        }
    private:
        CPopup* m_popup;
    };

    bool m_fRemovingListeners                   : 1;
    bool m_fMadeChildNamescopeOwner             : 1;
    bool m_fIsPrintDirty                        : 1;
    bool m_wasMarkedAsRedirectionElementOnEnter : 1;
    bool m_wasMarkedAsRedirectionElementOnOpen  : 1;
    bool m_isRegisteredOnCore                   : 1;
    bool m_shouldTakeFocus                      : 1;
    // Was this popup ever opened?
    bool m_everOpened                           : 1;

    // If this popup is being kept visible, then there might be a pending close operation that needs to complete when
    // it's no longer being kept visible. It might also be kept visible because it has VIsibility="Collapsed", in which
    // case there's nothing to do when it's no longer being kept visible anymore.
    bool m_closeIsPending                       : 1;

    bool m_isClosing                            : 1;

    // Unloading popups are kept in the CPopupRoot's list of open popups, but aren't considered open. They're in that
    // list so they can continue rendering and play a close animation. They're marked by this flag.
    bool m_isUnloading                          : 1;

    // Xaml implicitly closes popups in situations like changing the child or un-parenting from the tree. These close
    // operations shouldn't cause UIE.Hidden to be raised and shouldn't call RequestKeepAlive expecting a UIE.Hidden
    // later. This flag guards against that.
    bool m_isImplicitClose                      : 1;

    // Debug flag. If someone opens a windowed popup after the main Xaml window has been destroyed, we'll no-op instead
    // of trying to create an HWND for it. In these cases Xaml is shutting down.
    bool m_skippedCreatingPopupHwnd             : 1;

    bool m_registeredWindowPositionChangedHandler : 1;

    XUINT32 m_cEventListenerCount;
    ITransformer *m_pTransformer;

    // Maintain a weak reference to the object that was focused before opening a light-dismiss-enabled popup.
    // When the light-dismiss-enabled popup is closed, return focus to that object.
    xref::weakref_ptr<CDependencyObject> m_pPreviousFocusWeakRef;

    // When opening light-dismiss popup, retain the type of focus that was on the last focused element.
    DirectUI::FocusState m_savedFocusState;

    // When opening a flyout/light-dismiss popup, retain the horizontal and vertical manifolds
    Focus::XYFocus::Manifolds m_savedXYFocusManifolds;

    // When closing a light-dismiss-enabled popup, remember which input mode caused us to close, and
    // set this as the new focus state for m_pPreviousFocusWeakRef.
    DirectUI::FocusState m_focusStateAfterClosing;

    // Windowed popup's HWND
    HWND m_windowedPopupWindow;

    // Class registration for Windowed popup's HWND
    static ATOM s_windowedPopupWindowClass;

    // See banner of CPopup::EnsureWindowedPopupRootVisualTree for the full tree.
    wrl::ComPtr<ixp::IVisual> m_contentIslandRootVisual;
    wrl::ComPtr<ixp::IVisual> m_windowedPopupDebugVisual;
    wrl::ComPtr<ixp::IVisual> m_animationRootVisual;
    wrl::ComPtr<ixp::IContentExternalBackdropLink> m_backdropLink;
    wrl::ComPtr<ixp::IVisual> m_systemBackdropPlacementVisual;
    wgr::RectInt32 m_windowedPopupMoveAndResizeRect = {};

    // The prepend visual from Popup.Child's comp node
    // This is the topmost Visual in the popup's tree that corresponds to a UIElement. Everything above this is an
    // internal Visual that handles some Popup feature. See banner of CPopup::EnsureWindowedPopupRootVisualTree for the
    // full tree. This corresponds to the "public root" concept in the main tree, so we name it the same.
    wrl::ComPtr<ixp::IVisual> m_publicRootVisual;

    // Windowed popup's UIA support
    xref_ptr<CUIAHostWindow> m_spUIAWindow;

    bool m_isOverlayVisible;

    bool m_wasOpenedDuringEngagement;

    // AssociatedFlyout uses the weak reference to prevent the reference cycle
    // between the associated FlyoutBase and Popup
    xref::weakref_ptr<CFlyoutBase> m_associatedFlyoutWeakRef;

    wrl::ComPtr<ixp::IPopupWindowSiteBridge> m_popupWindowBridge;
    wrl::ComPtr<ixp::IDesktopSiteBridge> m_desktopBridge;
    wrl::ComPtr<ixp::IContentIsland> m_contentIsland;
    std::unique_ptr<WindowedPopupInputSiteAdapter> m_inputSiteAdapter{nullptr};
    wf::Point m_offsetFromMainWindow{};
    EventRegistrationToken m_automationProviderRequestedToken = {};
    EventRegistrationToken m_bridgeClosedToken {};

    // True if the m_popupWindowBridge/m_desktopBridge has been closed unexpectedly.
    bool m_bridgeClosed {false};

    // This is temporary fix to ensure that a popup children leave the tree correctly.
    // http://osgvsowi/19548424 - Remove this field and find a better fix
    xref::weakref_ptr<CDependencyObject> m_cachedNamescopeOwner;

    // Note: We use a "secret" CTransitionTarget, rather than a real one connected through the
    // UIElement_TransitionTarget property. A real TransitionTarget will be detected and picked up by the render walk,
    // which then results in a TransitionClip Visual created in the comp node. We don't have a size set, so a
    // TransitionClip (with insets of 0) will just clip out the entire popup. Use a secret CTransitionTarget instead
    // and pick up its expression explicitly in popup code, rather than rely on the render walk.
    xref_ptr<CTransitionTarget> m_secretTransitionTarget;
    wrl::ComPtr<ixp::IExpressionAnimation> m_entranceAnimationExpression;
};

//------------------------------------------------------------------------
//
//  Class:  CPopupRoot
//
//  Synopsis: Used to host all of the popups and the relationships between
//                 the popup child and the popup element for data binding
//
//------------------------------------------------------------------------
class CPopupRoot final : public CPanel
{
    friend class HWWalk;
    friend class BaseContentRenderer;
    friend class ThemeShadowGlobalScene;

private:

    CPopupRoot(_In_ CCoreServices *pCore)
        : CPanel(pCore)
        , m_pOpenPopups(NULL)
        , m_pDeferredPopups(NULL)
        , m_hasThemeChanged(false)
        , m_isRootHitTestingSuppressed(false)
        , m_shouldDeferClosePopups(false)
    {
        m_availableSizeAtLastMeasure.width = 0;
        m_availableSizeAtLastMeasure.height = 0;
    }

    typedef xvector<CPopup*> CPopupVector;

protected:
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;

    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;

    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    ~CPopupRoot() override;

    _Check_return_ CTransitionCollection* GetTransitionsForChildElementNoAddRef(_In_ CUIElement* pChild) override;

    _Check_return_ HRESULT PrintChildren(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams
        ) override;

public:

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPopupRoot>::Index;
    }

    DECLARE_CREATE(CPopupRoot);

    bool GetIsLayoutElement() const final { return true; }

    bool SkipNameRegistrationForChildren() override {  return true;   }

    _Check_return_ HRESULT StartAdditionToOpenPopupList(_In_ CPopup* pPopup);
    _Check_return_ HRESULT UndoAdditionToOpenPopupList(_In_ CPopup* pPopup);
    _Check_return_ HRESULT CompleteAdditionToOpenPopupList(_Inout_ CPopup* pPopup);

    _Check_return_ HRESULT RemoveFromOpenPopupList(_Inout_ CPopup *pPopup, bool bAsyncRelease);

    _Check_return_ HRESULT AddToDeferredOpenPopupList(_Inout_ CPopup *pPopup);

    void CloseAllPopupsForTreeReset();

    void OnHostWindowPositionChanged();

    void OnIslandLostFocus();

    bool HasOpenOrUnloadingPopups() const;

    // Exposed publicly via VisualTreeHelper. Returns the most recently opened popup first. Does not include unloading popups.
    _Check_return_ HRESULT GetOpenPopups(_Out_ XINT32 *pnCount, _Outptr_result_buffer_(*pnCount) CPopup ***pppPopups);

    // CPopup pointers are not ref counted. The caller is expected to iterate this vector immediately then toss the vector.
    std::vector<CPopup*> GetOpenPopupList(bool includeUnloadingPopups);

    _Check_return_ HRESULT ClearPrintDirtyFlagOnOpenPopups();

    enum class PopupFilter
    {
        LightDismissOnly,
        LightDismissOrFlyout,
        All,
    };

    CPopup* GetTopmostPopup(_In_ PopupFilter filter);
    _Check_return_ HRESULT CloseTopmostPopup(_In_ DirectUI::FocusState focusStateAfterClosing, _In_ PopupFilter filter, _Out_opt_ bool* pDidCloseAPopup = nullptr);

    bool IsTopmostPopupInLightDismissChain();
    _Check_return_ HRESULT GetTopmostPopupInLightDismissChain(_Out_ CDependencyObject** ppPopup);

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    _Check_return_ HRESULT NotifyThemeChanged(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false);

    void ClearWasOpenedDuringEngagementOnAllOpenPopups() const;

    static std::vector<CDependencyObject*> GetPopupChildrenOpenedDuringEngagement(
        _In_ CDependencyObject* const element);

    CPopup* GetOpenPopupWithChild(_In_ const CUIElement* const child, bool checkUnloadingChildToo) const;

    bool ContainsOpenOrUnloadingPopup(_In_ CPopup* pPopup);

    bool GetIsRootHitTestingSuppressed() const { return m_isRootHitTestingSuppressed; }
    void SetIsRootHitTestingSuppressed(bool isRootHitTestingSuppressed) { m_isRootHitTestingSuppressed = isRootHitTestingSuppressed; }

    bool ReplayPointerUpdate();
    void ClearLastPointerPointForReplay();

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT HitTestPopups(
        _In_ const XPOINTF& target,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitMultipleElements,
        _In_opt_ CUIElement *pSubTreeRoot,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Inout_ CHitTestResults *pHitElements
        );

    _Check_return_ HRESULT HitTestPopups(
        _In_ const HitTestPolygon& target,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitMultipleElements,
        _In_opt_ CUIElement *pSubTreeRoot,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Inout_ CHitTestResults *pHitElements
        );

    static _Check_return_ HRESULT GetOpenPopupForElement(
        _In_ CUIElement *pElement,
        _Outptr_result_maybenull_ CPopup **ppPopup);

    bool ComputeDepthInOpenPopups();

    // Closing open popups in the order that they opened can close nested popups and cause reentrancy issues,
    // so find a safe order to close popups.
    std::vector<CPopup*> GetPopupsInSafeClosingOrder();

    bool ShouldDeferClosePopups() { return m_shouldDeferClosePopups; }
    void DeferClosePopup(_In_ CPopup *popup);

protected:
    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT GenerateChildOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

    // Handler for PointerPressed on the CPopupRoot, for light dismiss popups.
    _Check_return_ HRESULT OnPointerPressed(
        _In_ CEventArgs* pEventArgs) override;

    _Check_return_ HRESULT OnKeyDown(
        _In_ CEventArgs* pEventArgs) override;

private:
    void LeavePCSceneSubgraph() final;
    void CleanupAndLeaveSubgraphHelper(bool forLeave, _In_ bool cleanupDComp);

    template <typename HitType>
    _Check_return_  HRESULT BoundsTestChildrenImpl(
        _In_ const HitType& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_opt_ CUIElement *pSubTreeRoot,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    template <typename HitType>
    _Check_return_ HRESULT HitTestPopupsImpl(
        _In_ const HitType& target,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitMultipleElements,
        _In_opt_ CUIElement *pSubTreeRoot,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Inout_ CHitTestResults *pHitElements
        );

    template <typename HitType>
    _Check_return_ HRESULT HitTestLocalInternalImpl(
        _In_ const HitType& target,
        _Out_ bool* pHit
        );

private:
    // A list of open and unloading popups. The most recently opened is at the head.
    CXcpList<CPopup> *m_pOpenPopups;

    // A list of popups that were opened while running layout on open popups
    CPopupVector *m_pDeferredPopups;
    XSIZEF m_availableSizeAtLastMeasure;

    // Has theme ever changed from startup theme?
    bool m_hasThemeChanged : 1;

    // Should we suppress hit-testing of the popup root?
    bool m_isRootHitTestingSuppressed : 1;

    // If this popup root is currently calling base LeaveImpl, then popups should defer closing
    // with this defer close list processed by LeaveImpl at a safe time.
    bool m_shouldDeferClosePopups : 1;
    std::vector<xref_ptr<CPopup>> m_popupsToDeferClose;

public:
};
