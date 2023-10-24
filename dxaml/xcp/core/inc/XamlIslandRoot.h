// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Panel.h>
#include <wil\winrt.h>
#include <fwd/windows.ui.composition.h>
#include <fwd/windows.ui.core.h>
#include <fwd/Microsoft.UI.Xaml.hosting.h>
#include <microsoft.ui.composition.private.h>
#include <microsoft.ui.composition.internal.h>
#include <microsoft.ui.input.h>
#include <microsoft.ui.input.inputkeyboardsource.interop.h>
#include <microsoft.ui.input.inputpretranslatesource.interop.h>
#include <Microsoft.UI.Content.h>
#include "microsoft.ui.input.dragdrop.h"

class CContentRoot;

class CUIAHostWindow;
class CUIAWindow;
class VisualTree;

class CXamlIslandRoot final : public CPanel
{
private:
    CXamlIslandRoot(_In_ CCoreServices *pCore);

public:
    ~CXamlIslandRoot() override;
    void Dispose();

    DECLARE_CREATE(CXamlIslandRoot);

    //
    //  Object initialization variants
    //
    //  IFrameworkApplicationPrivate::CreateIslandWithContentBridge():
    _Check_return_ HRESULT Initialize(_In_ WUComp::Desktop::IDesktopWindowContentBridgeInterop* contentBridge);

    void OnPostDesktopWindowContentBridgeInitialized(_In_ IUnknown* contentBridge);

    //  IFrameworkApplicationPrivate::CreateIsland():
    _Check_return_ HRESULT Initialize();

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CXamlIslandRoot>::Index;
    }

    _Check_return_ bool GetContentRequested() const { return m_isContentRequested; }
    void SetContentRequested(bool value) { m_isContentRequested = value; }

    ixp::IContentIsland * GetContentIsland();

    GUID GetCompositionIslandId();

    CPopupRoot* GetPopupRootNoRef();
    CTransitionRoot* GetTransitionRootNoRef();

    wf::Size GetSize();

    bool IsVisible() const;

    _Check_return_ HRESULT SetScreenOffsetOverride(_In_ wf::Point screenOffset);

    _Check_return_ HRESULT TrySetFocus(_Out_ boolean* pHasFocusNow);

    void NWPropagateDirtyFlag(DirtyFlags flags) override;

    HWND GetInputHWND();
    HWND GetPositioningHWND() const { return m_positioningWindow; }

    wrl::ComPtr<ixp::IPointerPoint> GetPreviousPointerPoint();

    POINT GetScreenOffset();
    void ClientLogicalToScreenPhysical(_Inout_ POINT& pt);
    void ScreenPhysicalToClientLogical(_Inout_ POINT& pt);
    void ClientPhysicalToScreenPhysical(_Inout_ POINT& pt);
    void ScreenPhysicalToClientPhysical(_Inout_ POINT& pt);

    CUIAWindow* GetUIAWindowNoRef();
    HRESULT UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents automationEvent) const;

    VisualTree* GetVisualTreeNoRef();

    _Check_return_ HRESULT get_FocusController(_Outptr_ IUnknown** ppValue);

    _Check_return_ HRESULT SetPublicRootVisual(
        _In_opt_ CUIElement* pRoot,
        _In_opt_ CScrollContentControl *pRootScrollViewer,
        _In_opt_ CContentPresenter *pRootContentPresenter);

    CUIElement* GetPublicRootVisual();

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    _Check_return_ HRESULT SetPointerCapture();
    _Check_return_ HRESULT ReleasePointerCapture();
    bool HasPointerCapture();

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) final;

    void SetInputWindow(HWND inputWindow);

    _Check_return_ HRESULT get_DragDropManager(_COM_Outptr_result_maybenull_ mui::DragDrop::IDragDropManager** manager);

    CContentRoot* GetContentRootNoRef() const
    {
        return m_contentRoot.get();
    }

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    UINT64 GetVisualIdentifier();
#endif

    bool HasFocus() const;

    wrl::ComPtr<ixp::IVisual> GetRootVisual() { return m_rootVisual; }
    void SetRootVisual(_In_ ixp::IVisual* visual) { m_rootVisual = visual;}

    wrl::ComPtr<ixp::IInputPointerSource> GetInputPointerSource() const { return m_inputPointerSource; }

    void ReplayPointerUpdate();
    void ClearLastPointerPointForReplay();

    // Event handlers are set on CompositionContent in contentroot.
    void OnStateChanged();
    // Automation
    _Check_return_ HRESULT OnContentAutomationProviderRequested(
        _In_ ixp::IContentIsland* content,
        _In_ ixp::IContentIslandAutomationProviderRequestedEventArgs* e);

    ixp::IDesktopChildSiteBridge* GetDesktopContentBridgeNoRef() { return m_contentBridgeDW.Get(); }

    void SetHasTransparentBackground(bool hasTransparentBackground);
    bool HasTransparentBackground() const;

    // PreTranslate and accelerator support, called back from PreTranslateHandler template class
    HRESULT PreTranslateMessage(
        mui::IInputPreTranslateKeyboardSourceInterop* source,
        const MSG* msg,
        UINT keyboardModifiers,
        bool focusPass,
        bool* handled);

    // ContentIsland has a LayoutDirection property that can report input in RTL coordinates (i.e. (0,0) at the
    // top-right, increasing X goes left). Xaml has always run in a world where input was always reported as LTR, and we
    // apply and RTL transforms within Xaml. Force the island to LTR mode so we can keep this old behavior. Note that
    // this will apply a window style to the desktop site bridge's hwnd, but that is a child hwnd of the app's top-level
    // hwnd and forcing the child hwnd to LTR isn't expected to affect things like the min/max/close buttons in the
    // nonclient area.
    void ForceLTRLayoutDirection();

    wrl::ComPtr<ixp::IContentIslandEnvironment> GetContentIslandEnvironment() const { return m_topLevelHost; }

protected:
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) final;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;

    _Check_return_ HRESULT GenerateChildOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* bounds) override;

    // NOTE: We don't want to override or confuse with the virtual OnPointerXXX methods from
    // UIElement, so these are given different names.
    _Check_return_ HRESULT OnIslandPointerCaptureLost(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerEntered(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerExited(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerMoved(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerPressed(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerReleased(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerWheelChanged(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerRoutedAway(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerRoutedReleased(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandPointerRoutedTo(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT OnIslandDirectManipulationHitTest(
        _In_ ixp::IPointerEventArgs* e);

    _Check_return_ HRESULT InjectPointerMessage(
        UINT msg,
        _In_ ixp::IPointerEventArgs* pArgs);

    void OnIslandActualSizeChanged();

    _Check_return_ HRESULT OnIslandGotFocus();

    _Check_return_ HRESULT OnIslandLostFocus();

    _Check_return_ HRESULT OnIslandCharacterReceived(
        _In_ ixp::ICharacterReceivedEventArgs* e);

    _Check_return_ HRESULT OnIslandKeyDown(
        _In_ ixp::IKeyEventArgs* e);

    _Check_return_ HRESULT OnIslandKeyUp(
        _In_ ixp::IKeyEventArgs* e);

    _Check_return_ HRESULT OnIslandSysKeyDown(
        _In_ ixp::IKeyEventArgs* e);

    _Check_return_ HRESULT OnIslandSysKeyUp(
        _In_ ixp::IKeyEventArgs* e);

    _Check_return_ HRESULT OnIslandTouchHitTesting(
        _In_ ixp::ITouchHitTestingEventArgs* args);

    // Drag Drop
    _Check_return_ HRESULT OnDropTargetRequested(
        _In_ mui::DragDrop::IDragDropManager* pSender,
        _In_ mui::DragDrop::IDropOperationTargetRequestedEventArgs* pArgs);

    _Check_return_ HRESULT EffectiveViewportWalkCore(
        _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports,
        _Out_ bool& addedViewports) override;

private:

    void UpdateRasterizationScale();

    bool HasContent() const { XAML_FAIL_FAST(); return true; }

    _Check_return_ HRESULT InitializeCommon();
    void InitializeOwnerHWNDForWindowedPopups(_In_ IUnknown* host);
    void InitInputObjects(_In_ ixp::IContentIsland* const contentIsland);
    void SubscribeToInputKeyboardSourceEvents();
    void SubscribeToInputPointerSourceEvents();
    void UnsubscribeToInputEvents();

    bool IsDisposed() const { return m_contentRoot == nullptr; }

    void UpdateLastPointerPointForReplay(const UINT uMsg, _In_ ixp::IPointerPoint* pointerPoint, _In_ ixp::IPointerEventArgs* pointerEventArgs);

    // For testing - normally we can get the visual out of m_spVisualTreeIsland, but for tests this returns a real
    // visual when we want the mock. So track the root visual separately.
    wrl::ComPtr<ixp::IVisual> m_rootVisual;

    bool m_isContentRequested = false;

    // Drag Drop
    _Check_return_ HRESULT RegisterIslandDropTarget();
    Microsoft::WRL::ComPtr<mui::DragDrop::IDragDropManager> m_dragDropManager;
    EventRegistrationToken m_dropTargetRequestedToken = {};

    // Accessibility
    xref_ptr<CUIAHostWindow> m_uiaWindow;

    bool m_screenOffsetOverrideIsValid = false;
    wf::Point m_screenOffsetOverride = {};

    Microsoft::WRL::ComPtr<xaml_hosting::IFocusController> m_xamlFocusController;

    //
    //  TODO: The input site should be retained by a composition component, ideally the
    //        CompositionIsland root visual, rather that by the UI framework or the application.
    //
    //  For the following XamlIsland factories, the InputSite is presently retained by the
    //  indicated components:
    //
    //  IFrameworkApplicationPrivate::CreateIslandWithContentBridge(): DesktopWindowContentBridge
    //  IFrameworkApplicationPrivate::CreateIsland() (TabShell suppport): XAML, as
    //      CXamlIslandRoot::m_inputSiteForTabShell.
    //
    //  Note: if the InputSite is retained by the CompositionIsland root visual, the three
    //  factories can be folded into one and CXamlIslandRoot need no longer
    //  be coupled to type DesktopWindowContentBridge
    //
    Microsoft::WRL::ComPtr<ixp::IExpInputSite> m_inputSiteForTabShell;
    Microsoft::WRL::ComPtr<ixp::IExpInputSite> m_inputSite;

    //  Send telemetry event when new area max is established.
    void InstrumentationNewAreaMax(_In_ float width, _In_ float height);

    bool m_hasCapture = false;

    //
    //  TODO: The InputSite child hwnd should not be known to framework. It's currently
    //        required to register a scrollpanel for DirectManipulation. Recommendation: a
    //        DirectManipulation InputObject that communicates directly with DM.
    //
    HWND m_inputWindow = nullptr;
    HWND m_positioningWindow = nullptr;

    wrl::ComPtr<ixp::IInputFocusController> m_inputFocusController;
    wrl::ComPtr<ixp::IInputKeyboardSource2> m_inputKeyboardSource2;
    wrl::ComPtr<ixp::IInputPointerSource> m_inputPointerSource;
    wrl::ComPtr<ixp::IInputPointerSource2> m_inputPointerSource2;
    wrl::ComPtr<ixp::IInputPreTranslateKeyboardSourceInterop> m_inputPreTranslateKeyboardSourceInterop;
    wrl::ComPtr<ixp::IInputActivationListener> m_inputActivationListener;
    wrl::ComPtr<ixp::IDesktopChildSiteBridge> m_contentBridgeDW;
    wrl::ComPtr<ixp::IContentIslandEnvironment> m_topLevelHost;
    wrl::ComPtr<ixp::IInputPreTranslateKeyboardSourceHandler> m_preTranslateHandler;

    EventRegistrationToken m_characterReceivedToken = {};
    EventRegistrationToken m_keyDownToken = {};
    EventRegistrationToken m_keyUpToken = {};
    EventRegistrationToken m_sysKeyDownToken = {};
    EventRegistrationToken m_sysKeyUpToken = {};

    EventRegistrationToken m_gotFocusToken = {};
    EventRegistrationToken m_lostFocusToken = {};

    EventRegistrationToken m_pointerCaptureLostToken = {};
    EventRegistrationToken m_pointerEnteredToken = {};
    EventRegistrationToken m_pointerExitedToken = {};
    EventRegistrationToken m_pointerMovedToken = {};
    EventRegistrationToken m_pointerPressedToken = {};
    EventRegistrationToken m_pointerReleasedToken = {};
    EventRegistrationToken m_pointerRoutedAwayToken = {};
    EventRegistrationToken m_pointerRoutedReleasedToken = {};
    EventRegistrationToken m_pointerRoutedToToken = {};
    EventRegistrationToken m_directManipulationHitTestToken{};
    EventRegistrationToken m_pointerWheelChangedToken = {};

    EventRegistrationToken m_touchHitTestRequestedToken = {};

    EventRegistrationToken m_activationChangedToken = {};

    EventRegistrationToken m_topLevelHost_StateChanged = {};
    EventRegistrationToken m_topLevelHost_ThemeChanged = {};
    EventRegistrationToken m_topLevelHost_SettingsChanged = {};

    xref_ptr<CContentRoot> m_contentRoot;

    // Cached site visibility flag
    // When this changes, we need to notify CCoreServices to reevaluate whether or not to suspend (the last visible island
    // became invisible) or resume (an island is visible again) rendering.
    bool m_isVisible = true;

    // Toggle switch for drawing a transparent background on the island. This is used by desktop apps to individually turn
    // on transparency for windows that have a system backdrop brush set (so the system acrylic or mica effect can show
    // through rather than be blocked by an opaque window background drawn by Xaml). We want to keep Xaml's opaque background
    // in other cases so that in-app acrylic can blend against it, rather than blend against complete transparency.
    bool m_hasTransparentBackground = false;

    wf::Size m_previousIslandSize = {};

    // Used for tracking max area for telemetry, whenever a new max is
    // is established for this island, we'll send an event.
    float m_maxIslandArea = 0.0;

    wrl::ComPtr<ixp::IPointerPoint> m_previousPointerPoint;
    wrl::ComPtr<ixp::IPointerEventArgs> m_previousPointerEventArgs;
};

