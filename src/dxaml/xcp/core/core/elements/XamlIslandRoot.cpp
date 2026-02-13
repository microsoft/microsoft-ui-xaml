// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <windows.ui.core.h>
#include <CoreWindow.h>

#include "DoubleUtil.h"
#include "WindowsGraphicsDeviceManager.h"
#include "WrlHelper.h"

#include "winpal.h"

#include "JupiterControl.h"
#include "JupiterWindow.h"
#include <DXamlServices.h>
#include <DXamlCore.h>
#include "FxCallbacks.h"
#include "PreTranslateHandler.h"

#include "TouchHitTestingHandler.h"
#include "DropOperationTarget.h"
#include "comInstantiation.h"

#include "FocusController.h"
#include "FrameworkTheming.h"

#include "RootScale.h"
#include "XamlIslandRootScale.h"
#include "FocusObserver.h"
#include "XamlTraceLogging.h"

#include <windowing.h>

#include <IHwndComponentHost.h>

#include <Theme.h>
#include "Value.h"

#include "PointerPointTransform.h"

#include "LoadLibraryAbs.h"

using namespace Microsoft::WRL;
using namespace WRLHelper;

CXamlIslandRoot::CXamlIslandRoot(_In_ CCoreServices *pCore)
    : CPanel(pCore)
{
    m_contentRoot = pCore->GetContentRootCoordinator()->CreateContentRoot(CContentRoot::Type::XamlIslandRoot, 0, this);
    m_contentRoot->SetXamlIslandRoot(this);
}

CXamlIslandRoot::~CXamlIslandRoot()
{
    Dispose();
}

// Order of cleanup should be :
//  1. Input state and resources
//  2. Content
//  3. Composition
void CXamlIslandRoot::Dispose()
{
    if (m_contentRoot)
    {
        const auto visualTree = m_contentRoot->GetVisualTreeNoRef();
        if (visualTree)
        {
            if (auto rootScale = visualTree->GetRootScale())
            {
                rootScale->SetContentIsland(nullptr);
            }
        }
    }

    m_rootVisual = nullptr;

    UnsubscribeToInputEvents();

    if (m_contentRoot)
    {
        IGNOREHR(m_contentRoot->Close());
        m_contentRoot = nullptr;
    }

    if (m_uiaWindow)
    {
        m_uiaWindow->UIADisconnectAllProviders();
        m_uiaWindow->Deinit();
        m_uiaWindow = nullptr;
    }

    // Let CCoreServices evaluate visibility again, in case this was the last visible content and it was just closed.
    m_isVisible = false;
    CCoreServices* coreServices = GetContext();
    IFCFAILFAST(coreServices->OnVisibilityChanged(false /* isStartingUp */, false /* freezeDWMSnapshotIfHidden */));

    // Clear out the IslandInputSite set on InputServices to facilitate a clean shutdown of DManip.
    if (nullptr != m_islandInputSite)
    {
        CInputServices* inputServices = coreServices->GetInputServices();
        inputServices->UnregisterIslandInputSite(m_islandInputSite.Get());
    }

    m_xamlFocusController = nullptr;

    if (m_dragDropManager && m_dropTargetRequestedToken.value != 0)
    {
        // We've seen remove_TargetRequested remove return CO_E_OBJNOTCONNECTED when we're tearing things down.
        // http://osgvsowi/19932392 There's not much we can do about this failure at this point, so we just ignore the HR.
        IGNOREHR(m_dragDropManager->remove_TargetRequested(m_dropTargetRequestedToken));
        m_dropTargetRequestedToken = {};
        m_dragDropManager = nullptr;
    }

    if (m_topLevelHost != nullptr)
    {
        if (m_topLevelHost_StateChanged.value != 0)
        {
            IGNOREHR(m_topLevelHost->remove_StateChanged(m_topLevelHost_StateChanged));
            m_topLevelHost_StateChanged.value = 0;
        }

        if (m_topLevelHost_ThemeChanged.value != 0)
        {
            wrl::ComPtr<ixp::IContentIslandEnvironmentExperimental> contentIslandEnvironmentExperimental;
            IFCFAILFAST(m_topLevelHost.As(&contentIslandEnvironmentExperimental));
            IGNOREHR(contentIslandEnvironmentExperimental->remove_ThemeChanged(m_topLevelHost_ThemeChanged));
            m_topLevelHost_ThemeChanged.value = 0;
        }

        if (m_topLevelHost_SettingChanged.value != 0)
        {
            IGNOREHR(m_topLevelHost->remove_SettingChanged(m_topLevelHost_SettingChanged));
            m_topLevelHost_SettingChanged.value = 0;
        }

        m_topLevelHost = nullptr;
    }

    m_contentBridgeDW = nullptr;
}

_Check_return_ HRESULT CXamlIslandRoot::Initialize(_In_ WUComp::Desktop::IDesktopWindowContentBridgeInterop* /*contentBridge*/)
{
    m_contentRoot->SetXamlIslandType(CContentRoot::IslandType::DesktopWindowContentBridge);
    IFC_RETURN(InitializeCommon());

    // Note: Don't connect here. The connect will happen as result of Calling DesktopWindowXamlSource::AttachToWindow

    return S_OK;
}

void CXamlIslandRoot::OnPostDesktopWindowContentBridgeInitialized(_In_ IUnknown* contentBridge)
{
    InitializeOwnerHWNDForWindowedPopups(contentBridge);
}

void CXamlIslandRoot::InitializeNonClientPointerSource(ABI::Microsoft::UI::WindowId parentWindowId)
{
    ixp::IInputNonClientPointerSourceStatics* inputNonClientPointerSourceStaticsNoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetInputNonClientPointerSourceStatics();

    IFCFAILFAST(inputNonClientPointerSourceStaticsNoRef->GetForWindowId(parentWindowId, &m_inputNonClientPointerSource));

    SubscribeToInputNonClientPointerSourceEvents();
}

//
//  TabShell XamlIslandRoot factory processing:
//
_Check_return_ HRESULT CXamlIslandRoot::Initialize()
{
    m_contentRoot->SetXamlIslandType(CContentRoot::IslandType::Raw);
    IFC_RETURN(InitializeCommon());
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::InitializeCommon()
{
    IFC_RETURN(SetRequiresComposition(CompositionRequirement::RootElement, IndependentAnimationType::None));

    auto core = GetContext();

    WindowsGraphicsDeviceManager* graphicsDeviceManager = core->GetBrowserHost()->GetGraphicsDeviceManager();
    FAIL_FAST_ASSERT(graphicsDeviceManager);

    IFC_RETURN(graphicsDeviceManager->EnsureDCompDevice());

    DCompTreeHost* pTreeHost = GetDCompTreeHost();
    WUComp::ICompositor* pCompositor = pTreeHost->GetCompositor();
    FAIL_FAST_ASSERT(pCompositor);

    // Create a Composition OrderedIsland for Xaml content:
    // - This enables getting feedback from the Compositor, such as scaling transforms
    //   needed for rasterizing text.
    // - It is also used by CoreInput to coordinate input / output.
    ctl::ComPtr<ixp::IContentIsland> compositionContent;

    ctl::ComPtr<ixp::IContainerVisual> containerRootVisual;
    IFCFAILFAST(pCompositor->CreateContainerVisual(&containerRootVisual));

    ctl::ComPtr<ixp::IVisual> rootVisual;
    IFCFAILFAST(containerRootVisual.As(&rootVisual));

    IFCFAILFAST(ActivationFactoryCache::GetActivationFactoryCache()->GetContentIslandStatics()->Create(rootVisual.Get(), &compositionContent));

    IFC_RETURN(m_contentRoot->SetContentIsland(compositionContent.Get()));

    FrameworkTheming* themingNoRef = core->GetFrameworkTheming();
    IFC_RETURN(NotifyThemeChanged(themingNoRef->GetTheme(), true /*fForceRefresh*/));

    // Initialize input

    // Deliverable 23283226: Lifted hwnd Xaml islands
    // TODO - Switch this to use the existing InputSiteAdapter instead.
    InitInputObjects(compositionContent.Get());
    SubscribeToInputKeyboardSourceEvents();
    SubscribeToInputPointerSourceEvents();

    xref::weakref_ptr<CXamlIslandRoot> weakXamlIslandRoot(this);

    auto inputActivationChangedCallback =
        [weakXamlIslandRoot](ixp::IInputActivationListener*, ixp::IInputActivationListenerActivationChangedEventArgs*)
        -> HRESULT
    {
        CXamlIslandRoot* that = weakXamlIslandRoot.lock();
        // Note: m_contentRoot could be null here if this is happening while the DesktopWindowXamlSource is closing.
        if (that && that->m_contentRoot)
        {
            ixp::InputActivationState state = {};
            IFC_RETURN(that->m_inputActivationListener->get_State(&state));

            that->m_contentRoot->SetIsInputActive(state == ixp::InputActivationState::InputActivationState_Activated);
            that->m_contentRoot->RaiseXamlRootInputActivationChanged();
        }
        return S_OK;
    };

    // InputActivationListener
    IFCFAILFAST(m_inputActivationListener->add_InputActivationChanged(
        WRLHelper::MakeAgileCallback<
            wf::ITypedEventHandler<ixp::InputActivationListener*,
            ixp::InputActivationListenerActivationChangedEventArgs*>>(inputActivationChangedCallback).Get(),
        &m_activationChangedToken));

    if (nullptr != m_contentRoot)
    {
        ixp::InputActivationState state = {};
        IFCFAILFAST(m_inputActivationListener->get_State(&state));
        m_contentRoot->SetIsInputActive(ixp::InputActivationState::InputActivationState_Activated == state);
    }

    //  Configure focus navigation.
    wrl::ComPtr<FocusController> focusController;
    IFCFAILFAST(FocusController::Create(m_inputFocusController.Get(), &focusController));
    IFC_RETURN(m_contentRoot->GetFocusManagerNoRef()->GetFocusObserverNoRef()->Init(focusController.Get()));
    m_xamlFocusController = focusController;

    // Store the IContentIslandEnvironment in order to subscribe to its events.
    IFCFAILFAST(GetContentIsland()->get_Environment(&m_topLevelHost));

    if (m_topLevelHost)
    {
        auto stateChangedCallback = [](ixp::IContentIslandEnvironment*, ixp::IContentEnvironmentStateChangedEventArgs*) -> HRESULT
        {
            if (auto dxamlCore = DirectUI::DXamlCore::GetCurrent())
            {
                dxamlCore->GetControl()->OnDisplayChanged();
            }
            return S_OK;
        };

        IFCFAILFAST(m_topLevelHost->add_StateChanged(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::ContentIslandEnvironment*, ixp::ContentEnvironmentStateChangedEventArgs*>>(stateChangedCallback).Get(),
            &m_topLevelHost_StateChanged));

        auto settingChangedCallback = [](ixp::IContentIslandEnvironment*, ixp::IContentEnvironmentSettingChangedEventArgs* args) -> HRESULT
        {
            if (auto dxamlCore = DirectUI::DXamlCore::GetCurrent())
            {
                HSTRING settingName;
                IFC_RETURN(args->get_SettingName(&settingName));

                dxamlCore->GetControl()->OnSettingChanged(settingName);
            }
            return S_OK;
        };

        IFCFAILFAST(m_topLevelHost->add_SettingChanged(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::ContentIslandEnvironment*, ixp::ContentEnvironmentSettingChangedEventArgs*>>(settingChangedCallback).Get(),
            &m_topLevelHost_SettingChanged));

        auto themeChangedCallback = [](ixp::IContentIslandEnvironment*, IInspectable*) -> HRESULT
        {
            if (auto dxamlCore = DirectUI::DXamlCore::GetCurrent())
            {
                dxamlCore->GetControl()->OnThemeChanged();
            }
            return S_OK;
        };

        wrl::ComPtr<ixp::IContentIslandEnvironmentExperimental> contentIslandEnvironmentExperimental;
        IFCFAILFAST(m_topLevelHost.As(&contentIslandEnvironmentExperimental));
        IFCFAILFAST(contentIslandEnvironmentExperimental->add_ThemeChanged(
            WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::ContentIslandEnvironment*, IInspectable*>>(themeChangedCallback).Get(),
            &m_topLevelHost_ThemeChanged));
    }

    return S_OK;
}

void CXamlIslandRoot::OnStateChanged()
{
    // In the scope if this handler, we may find the the XamlRoot has changed in various ways.  This RAII
    // helper ensures we call CContentRoot::RaisePendingXamlRootChangedEventIfNeeded before exiting this function.
    XamlRootStateChangedRaiiHelper helper(m_contentRoot);

    UpdateRasterizationScale();
    OnIslandActualSizeChanged();

    boolean isSiteVisible = false;
    ixp::ContentLayoutDirection layoutDirection = ixp::ContentLayoutDirection_LeftToRight;
    // Don't consider content visibility. That's a separate concept and that property is exposed to apps. The site
    // visibility is the equivalent of CoreWindow visibility in XamlIslandRoots mode.
    if (auto compContent = GetContentIsland())
    {
        IFCFAILFAST(compContent->get_IsSiteVisible(&isSiteVisible));
        IFCFAILFAST(compContent->get_LayoutDirection(&layoutDirection));
    }

    if (m_isVisible != !!isSiteVisible)
    {
        m_isVisible = !!isSiteVisible;

        if (m_contentRoot != nullptr)
        {
            m_contentRoot->AddPendingXamlRootChangedEvent(CContentRoot::ChangeType::IsVisible);

            CCoreServices* coreServices = GetContext();
            IFCFAILFAST(coreServices->OnVisibilityChanged(false /* isStartingUp */, false /* freezeDWMSnapshotIfHidden */));
        }
    }

    if (layoutDirection == ixp::ContentLayoutDirection_RightToLeft)
    {
        IFCFAILFAST(ForceLTRLayoutDirection());
    }
}

_Check_return_ HRESULT
CXamlIslandRoot::ForceLTRLayoutDirection()
{
    // https://task.ms/43100993: If we don't have a bridge, this is hosted in a XamlIsland instead of
    // a DesktopWindowXamlSource. In this case we have no power to enforce an LTR Layout Direction.
    // Until Xaml correctly handles the RTL coordinate space, this will lead to issues with input and
    // output, as the underlying HWND will be in RTL, so we FAILFAST here. Once RTL is properly
    // handled, we should be able to remove this method entirely and respond correctly to the
    // ContentIsland's LayoutDirection.

    if (m_contentBridgeDW == nullptr)
    {
        ::RoOriginateError(
            E_NOT_SUPPORTED,
            wrl_wrappers::HStringReference(
            L"RTL layout on the ContentIsland is not supported for XamlIsland. Use Xaml FlowDirection to set RTL on the content instead."
            ).Get());
        IFC_RETURN(E_NOT_SUPPORTED);
    }

    wrl::ComPtr<DirectUI::Reference<ixp::ContentLayoutDirection>> ltr;
    IFCFAILFAST(DirectUI::PropertyValue::CreateReference<ixp::ContentLayoutDirection>(ixp::ContentLayoutDirection_LeftToRight, &ltr));
    wrl::ComPtr<wf::IReference<ixp::ContentLayoutDirection>> ltrIReference;
    IFCFAILFAST(ltr.As(&ltrIReference));

    wrl::ComPtr<ixp::IContentSiteBridge> contentSiteBridge;
    IFCFAILFAST(m_contentBridgeDW.As(&contentSiteBridge));
    IFC_RETURN(contentSiteBridge->put_LayoutDirectionOverride(ltrIReference.Get()));

    return S_OK;
}

_Check_return_ HRESULT
CXamlIslandRoot::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    auto core = GetContext();

    UINT backgroundColor = (theme == Theming::Theme::Light ? 0xffffffff : 0xff000000);
    CValue backgroundColorValue;
    backgroundColorValue.SetColor(backgroundColor);

    xref_ptr<CBrush> newBackgroundBrush;
    CBrush* pBackgroundBrush = m_pBackground;
    if (pBackgroundBrush == nullptr)
    {
        CREATEPARAMETERS cp(core);
        IFC_RETURN(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(newBackgroundBrush.ReleaseAndGetAddressOf()), &cp));
        pBackgroundBrush = newBackgroundBrush.get();
    }
    IFC_RETURN(pBackgroundBrush->SetValueByKnownIndex(KnownPropertyIndex::SolidColorBrush_Color, backgroundColorValue));
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::Panel_Background, pBackgroundBrush));   // Note - sets m_pBackground via the DependencyProperty

    IFC_RETURN(__super::NotifyThemeChangedCore(theme, fForceRefresh));

    return S_OK;
}

bool CXamlIslandRoot::HasTransparentBackground() const
{
    return m_hasTransparentBackground;
}

void CXamlIslandRoot::SetHasTransparentBackground(bool hasTransparentBackground)
{
    if (m_hasTransparentBackground != hasTransparentBackground)
    {
        m_hasTransparentBackground = hasTransparentBackground;
        CPanel::NWSetBackgroundDirty(this, DirtyFlags::Render);
    }
}

#pragma region Input
_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandPointerCaptureLost(ixp::IPointerEventArgs* args)
{
    IFC_RETURN(InjectPointerMessage(CInputServices::GetMessageFromPointerCaptureLostArgs(args), args));
    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandPointerEntered(ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(WM_POINTERENTER, e));
    return S_OK;
}


_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandPointerExited(ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(WM_POINTERLEAVE, e));
    return S_OK;
}


_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandPointerMoved(ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(WM_POINTERUPDATE, e));
    return S_OK;
}


_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandPointerPressed(ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(WM_POINTERDOWN, e));
    return S_OK;
}


_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandPointerReleased(ixp::IPointerEventArgs* e)
{
    HRESULT hr = InjectPointerMessage(WM_POINTERUP, e);
    return hr;
}


_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandPointerWheelChanged(ixp::IPointerEventArgs* e)
{
    // ISLANDTODO: CoreInput doesn't distinguish between WM_POINTERWHEEL and WM_POINTERHWHEEL.  Does
    // this matter?
    IFC_RETURN(InjectPointerMessage(WM_POINTERWHEEL, e));
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandPointerRoutedAway(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(WM_POINTERROUTEDAWAY, e));
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandPointerRoutedReleased(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(WM_POINTERROUTEDRELEASED, e));
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandPointerRoutedTo(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(WM_POINTERROUTEDTO, e));
    return S_OK;
}


_Check_return_ HRESULT CXamlIslandRoot::OnIslandDirectManipulationHitTest(_In_ ixp::IPointerEventArgs* e)
{
    IFC_RETURN(InjectPointerMessage(DM_POINTERHITTEST, e));
    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::InjectPointerMessage(UINT msg, ixp::IPointerEventArgs* args)
{
    wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFCFAILFAST(args->get_CurrentPoint(&pointerPoint));

    UpdateLastPointerPointForReplay(msg, pointerPoint.Get(), args);

    // Take a strong ref before calling out to JupiterWindow event handlers, which calls out to app code.
    // The XamlIslandRoot may be Disposed before we get back, and the contentRoot will be gone.
    xref_ptr<CContentRoot> contentRootStrongRef { m_contentRoot };

    bool handled = false;
    IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandPointerMessage(
        msg,
        contentRootStrongRef,
        pointerPoint.Get(),
        args,
        false /* isReplayedMessage */,
        &handled));

    // Don't write false into the event args. This prevents overwriting the value if someone else already handled the event.
    if (handled)
    {
        IFC_RETURN(args->put_Handled(handled));
    }

    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandNonClientPointerEntered(ixp::IPointerPoint* pointerPoint)
{
    IFC_RETURN(InjectNonClientPointerMessage(WM_POINTERENTER, pointerPoint));
    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandNonClientPointerExited(ixp::IPointerPoint* pointerPoint)
{
    IFC_RETURN(InjectNonClientPointerMessage(WM_POINTERLEAVE, pointerPoint));
    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandNonClientPointerMoved(ixp::IPointerPoint* pointerPoint)
{
    IFC_RETURN(InjectNonClientPointerMessage(WM_POINTERUPDATE, pointerPoint));
    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandNonClientPointerPressed(ixp::IPointerPoint* pointerPoint)
{
    IFC_RETURN(InjectNonClientPointerMessage(WM_POINTERDOWN, pointerPoint));
    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::OnIslandNonClientPointerReleased(ixp::IPointerPoint* pointerPoint)
{
    HRESULT hr = InjectNonClientPointerMessage(WM_POINTERUP, pointerPoint);
    return hr;
}

// Note that these non-client pointer events are all ones that occur outside the context of a move-size loop.
// When inside a move-size loop, the InputNonClientPointerSource will receive all of the pointer messages
// and instead raise the move-size loop events - we don't receive any raw pointer events in that case.
_Use_decl_annotations_
HRESULT CXamlIslandRoot::InjectNonClientPointerMessage(UINT msg, ixp::IPointerPoint* pointerPoint)
{
    UpdateLastPointerPointForReplay(msg, pointerPoint, nullptr, true);

    // Take a strong ref before calling out to JupiterWindow event handlers, which calls out to app code.
    // The XamlIslandRoot may be Disposed before we get back, and the contentRoot will be gone.
    xref_ptr<CContentRoot> contentRootStrongRef{ m_contentRoot };

    IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandNonClientPointerMessage(
        msg,
        contentRootStrongRef,
        pointerPoint,
        false));

    return S_OK;
}

void CXamlIslandRoot::ReplayPointerUpdate()
{
    bool pointerUpdateReplayed = false;

    // Take a strong ref before calling out to JupiterWindow event handlers, which calls out to app code.
    // The XamlIslandRoot may be Disposed before we get back, and the contentRoot will be gone.
    xref_ptr<CContentRoot> contentRootStrongRef { m_contentRoot };

    // Replay the xaml island pointer update if it has one.
    ixp::IPointerPoint* previousPointerPoint = m_previousPointerPoint.Get();
    ixp::IPointerEventArgs* previousPointerEventArgs = m_previousPointerEventArgs.Get();
    if (previousPointerPoint != nullptr)
    {
        if (previousPointerEventArgs && !m_previousPointerPointIsNonClient)
        {
            bool handled_dontCare;

            IFCFAILFAST(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandPointerMessage(
                WM_POINTERUPDATE,
                contentRootStrongRef,
                previousPointerPoint,
                previousPointerEventArgs,
                true /* isReplayedMessage */,
                &handled_dontCare));

            pointerUpdateReplayed = true;
        }

        if (m_previousPointerPointIsNonClient)
        {
            IFCFAILFAST(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandNonClientPointerMessage(
                WM_POINTERUPDATE,
                contentRootStrongRef,
                previousPointerPoint,
                true /* isReplayedMessage */));

            pointerUpdateReplayed = true;
        }
    }

    // Replay any popup pointer updates if there are any.
    if (CPopupRoot* popupRoot = GetPopupRootNoRef())
    {
        bool popupPointerUpdateReplayed = popupRoot->ReplayPointerUpdate();
        if (popupPointerUpdateReplayed)
        {
            // Only one window should have valid pointer messages to replay.
            ASSERT(!pointerUpdateReplayed);
        }
    }
}

// Island focus changed
// Route this over to CJupiterWindow so that XAML input is processed in a unified way.
_Check_return_ HRESULT CXamlIslandRoot::OnIslandGotFocus()
{
    auto jupiterWindow = DirectUI::DXamlServices::GetCurrentJupiterWindow();
    xref_ptr<CContentRoot> contentRoot(m_contentRoot);
    if (jupiterWindow && contentRoot)
    {
        // The last-used input device may have changed since the last time this window had focus,
        // which would cause us to re-show the keyboard focus rect if it was previously shown,
        // even if the user used a mouse to give this window focus.  Normally that's okay since
        // a XAML app is self-contained, and it wouldn't seem odd for it to go back to the state it was in
        // before it lost focus. However, in the case of a XAML island, it's integrated into a larger
        // app, and it would be strange for the user to use the mouse to click on a XAML island
        // and have the keyboard focus rect suddenly appear.  So, to prevent the focus rect from
        // showing again if the user didn't use a keyboard to bring focus back to this window,
        // we'll update the focused element's focus state to the last-used input device.
        if (auto focusManager = contentRoot->GetFocusManagerNoRef())
        {
            if (auto focusedControl = do_pointer_cast<CControl>(focusManager->GetFocusedElementNoRef()))
            {
                // To avoid showing the focus rect in cases where the user e.g. just Alt+Tabs back into the app,
                // we'll only transition from keyboard -> not keyboard, rather than the reverse.
                auto currentFocusState = CFocusManager::GetFocusStateFromInputDeviceType(contentRoot->GetInputManager().GetLastInputDeviceType());

                if (focusedControl->IsKeyboardFocused() && currentFocusState != DirectUI::FocusState::Keyboard)
                {
                    IFC_RETURN(focusedControl->UpdateFocusState(currentFocusState));
                }
            }
        }

        IFC_RETURN(jupiterWindow->OnIslandGotFocus(contentRoot));
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandLostFocus()
{
    if (m_contentRoot)
    {
        xref_ptr<CContentRoot> contentRoot(m_contentRoot);
        CPopupRoot* popupRoot = GetPopupRootNoRef();

        // We lost focus on the XamlIslandRoot's content bridge, but there are several cases where we ignore the focus
        // loss. Check for them by comparing the currently focused hwnd against a list of allowed things.
        HWND focusedHwnd = ::GetFocus();
        if (focusedHwnd)
        {
            // In the case where we're losing focus to a hosted component, we don't want Xaml to think it lost focus
            CDependencyObject* focusedObject = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
            // For now, the only use of this is not wanting to lose focus to CoreWebView2
            // WebView2 derives from Panel, scope to QI only on Panel elements
            if (focusedObject && focusedObject->GetTypeIndex() == KnownTypeIndex::Panel)
            {
                ctl::ComPtr<DirectUI::DependencyObject> peer;
                IFC_RETURN(DirectUI::DXamlCore::GetCurrent()->TryGetPeer(focusedObject, &peer));
                if (peer)
                {
                    ctl::ComPtr<IHwndComponentHost> host;
                    HRESULT qiResult = ctl::iinspectable_cast(peer.Get())->QueryInterface(__uuidof(IHwndComponentHost), reinterpret_cast<void**>(host.ReleaseAndGetAddressOf()));
                    if (SUCCEEDED(qiResult) && host != nullptr)
                    {
                        // Check if focus actually went to the correct hosted hwnd
                        HWND hwndChild = host->GetComponentHwnd();
                        if (focusedHwnd == hwndChild)
                        {
                            return S_OK;
                        }
                    }
                }
            }

            // Like with WebView, we don't want Xaml to think it lost focus if focus shifted to a windowed popup.
            // If there are open popups, check that we didn't focus one of them either.
            if (popupRoot && popupRoot->HasOpenOrUnloadingPopups())
            {
                std::vector<CPopup*> openPopups = popupRoot->GetOpenPopupList(true /* includeUnloadingPopups */);
                for (CPopup* popup : openPopups)
                {
                    if (popup->IsWindowed() && popup->WindowedPopupHasFocus())
                    {
                        return S_OK;
                    }
                }
            }

            // There are cases in MUXControlsTestApp where Xaml seemingly loses focus to itself.
            // See CommandBarFlyoutTests.CanTapOnPrimaryItems, for example.
            if (HasFocus())
            {
                return S_OK;
            }
        }

        IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandLostFocus(contentRoot));

        if (popupRoot)
        {
            popupRoot->OnIslandLostFocus();
        }
    }

    return S_OK;
}

// Island text input
// Route this over to CJupiterWindow so that XAML input is processed in a unified way.
// Note that we don't actually track which content received the input-- it will go to the focused element,
// whichever content it happens to live in.
_Check_return_ HRESULT CXamlIslandRoot::OnIslandCharacterReceived(
    _In_ ixp::ICharacterReceivedEventArgs* e)
{
    if (m_contentRoot)
    {
        xref_ptr<CContentRoot> contentRoot(m_contentRoot);

        IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandCharacterReceived(e, contentRoot));
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandKeyDown(
    _In_ ixp::IKeyEventArgs* e)
{
    if (m_contentRoot)
    {
        xref_ptr<CContentRoot> contentRoot(m_contentRoot);

        IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandKeyDown(e, contentRoot));
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandKeyUp(
    _In_ ixp::IKeyEventArgs* e)
{
    if (m_contentRoot)
    {
        xref_ptr<CContentRoot> contentRoot(m_contentRoot);

        IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandKeyUp(e, contentRoot));
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandSysKeyDown(
    _In_ ixp::IKeyEventArgs* e)
{
    if (m_contentRoot)
    {
        xref_ptr<CContentRoot> contentRoot(m_contentRoot);

        IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandSysKeyDown(e, contentRoot));
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandSysKeyUp(
    _In_ ixp::IKeyEventArgs* e)
{
    if (m_contentRoot)
    {
        xref_ptr<CContentRoot> contentRoot(m_contentRoot);

        IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->OnIslandSysKeyUp(e, contentRoot));
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::OnIslandTouchHitTesting(
    _In_ ixp::ITouchHitTestingEventArgs* args)
{
    IFC_RETURN(DirectUI::TouchHitTestingHandler::TouchHitTesting(
        GetPublicRootVisual(),
        args));
    return S_OK;
}
#pragma endregion Input

#pragma region Drag Drop
_Check_return_ HRESULT CXamlIslandRoot::OnDropTargetRequested(_In_ mui::DragDrop::IDragDropManager* /*sender*/, _In_ mui::DragDrop::IDropOperationTargetRequestedEventArgs* args)
{
    ctl::ComPtr<DirectUI::DropOperationTarget> dropTarget;
    IFC_RETURN(ctl::make<DirectUI::DropOperationTarget>(this, &dropTarget));
    IFC_RETURN(args->SetTarget(dropTarget.Get()));
    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::RegisterIslandDropTarget()
{
    // Only one handler for this event should exist
    ASSERT(m_dropTargetRequestedToken.value == 0);

    IFC_RETURN(DirectUI::DXamlServices::GetCurrentJupiterWindow()->SetIslandDragDropMode(true));

    wrl::ComPtr<mui::DragDrop::IDragDropManagerStatics> dragDropManagerStatics;
    IFC_RETURN(ActivationFactoryCache::GetActivationFactoryCache()->GetDragDropManagerStatics(&dragDropManagerStatics));

    IFC_RETURN(dragDropManagerStatics->GetForIsland(GetContentIsland(), &m_dragDropManager));

    // Callback object is not agile; this is intended for XAML objects that are thread affine
    auto callback = wrl::Callback<wf::ITypedEventHandler<mui::DragDrop::DragDropManager*, mui::DragDrop::DropOperationTargetRequestedEventArgs*>>(this, &CXamlIslandRoot::OnDropTargetRequested);
    IFC_RETURN(m_dragDropManager->add_TargetRequested(callback.Get(), &m_dropTargetRequestedToken));

    return S_OK;
}

_Use_decl_annotations_
HRESULT CXamlIslandRoot::UnregisterIslandDropTarget()
{
    if (m_dragDropManager && m_dropTargetRequestedToken.value != 0)
    {
        IFC_RETURN(m_dragDropManager->remove_TargetRequested(m_dropTargetRequestedToken));

        m_dropTargetRequestedToken = {};
        m_dragDropManager = nullptr;
    }

    return S_OK;
}
#pragma endregion Drag Drop

_Check_return_ HRESULT CXamlIslandRoot::OnContentAutomationProviderRequested(
    _In_ ixp::IContentIsland* content,
    _In_ ixp::IContentIslandAutomationProviderRequestedEventArgs* e)
{
    if (!IsDisposed())
    {
        IFC_RETURN(e->put_AutomationProvider(GetUIAWindowNoRef()));
        IFC_RETURN(e->put_Handled(true));
    }
    return S_OK;
}

HRESULT CXamlIslandRoot::UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents automationEvent) const
{
    if (m_uiaWindow)
    {
        return m_uiaWindow->UIAClientsAreListening(automationEvent);
    }
    return S_FALSE;
}

CUIAWindow* CXamlIslandRoot::GetUIAWindowNoRef(bool onlyGet)
{
    if (!m_uiaWindow && !onlyGet)
    {
        IFCFAILFAST(CUIAHostWindow::Create(
            UIAHostEnvironmentInfo(this),
            GetContext()->GetHostSite(),
            nullptr,    // We don't need to pass in the public root visual here because CUIAHostWindow will query the island for it.
            m_uiaWindow.ReleaseAndGetAddressOf()));
    }

    return m_uiaWindow.get();
}

ixp::IContentIsland *
CXamlIslandRoot::GetContentIsland()
{
    if(m_contentRoot)
    {
        return m_contentRoot->GetCompositionContentNoRef();
    }

    return nullptr;
}

_Check_return_ HRESULT CXamlIslandRoot::MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    desiredSize = { 0, 0 };

    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 i = 0; i < childrenCount; i++)
    {
        IFC_RETURN(children[i]->Measure(availableSize));
        IFC_RETURN(children[i]->EnsureLayoutStorage());

        // The first child is the content, which is what we want the desired size to be
        if (i == 0)
        {
            desiredSize = children[i]->GetLayoutStorage()->m_desiredSize;
        }
    }

    // Notify the comp content of a new desired/requested size
    if (auto compContent = GetContentIsland())
    {
        IFC_RETURN(compContent->RequestSize({ desiredSize.width, desiredSize.height }));
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize)
{
    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 i = 0; i < childrenCount; i++)
    {
        IFC_RETURN(children[i]->EnsureLayoutStorage());
        XSIZEF& childDesiredSize = children[i]->GetLayoutStorage()->m_desiredSize;

        const XRECTF childRect = {
            0.0f,
            0.0f,
            std::max(childDesiredSize.width, finalSize.width),
            std::max(childDesiredSize.height, finalSize.height) };
        IFC_RETURN(children[i]->Arrange(childRect));
    }

    newFinalSize = finalSize;

    // Notify the XAML render walk the content has changed size
    GetContext()->UpdateXamlIslandRootTargetSize(this);
    return S_OK;
}

wf::Size CXamlIslandRoot::GetSize()
{
    wfn::Vector2 actualSize = {};
    if (auto compContent = GetContentIsland())
    {
        IFCFAILFAST(compContent->get_ActualSize(&actualSize));
    }

    return{ actualSize.X, actualSize.Y };
}

bool CXamlIslandRoot::IsVisible() const
{
    return m_isVisible;
}

void CXamlIslandRoot::UpdateRasterizationScale()
{
    // We may have been already closed, bail out.
    // m_contentRoot may be null if we reset the window content to null before getting a StateChanged notification.
    if (m_contentRoot)
    {
        const auto visualTree = m_contentRoot->GetVisualTreeNoRef();
        if (auto rootScale = visualTree->GetRootScale())
        {
            IFCFAILFAST(rootScale->UpdateSystemScale());
        }
    }
}

void CXamlIslandRoot::OnIslandActualSizeChanged()
{
    const auto newSize = GetSize();
    if (!IsCloseReal(m_previousIslandSize.Width, newSize.Width) || !IsCloseReal(m_previousIslandSize.Height, newSize.Height))
    {
        InvalidateMeasure();

        // ISLANDTODO 10919237: Why do we need to invalidate the parent manually here?
        CUIElement* parent = GetUIElementParentInternal(false /*publicOnly*/);
        if (parent)
        {
            parent->InvalidateMeasure();
        }

        FxCallbacks::XamlIslandRoot_OnSizeChanged(this);

        if (m_contentRoot != nullptr)
        {
            m_contentRoot->AddPendingXamlRootChangedEvent(CContentRoot::ChangeType::Size);
        }
        else
        {
            // The XamlIslandRoot itself will release its visual tree immediately when it leaves the live tree,
            // but will be kept alive until the end of the frame. If any CompositionContent::StateChanged events
            // are raised in the meantime, we will ignore them.
            ASSERT(!IsActive());
        }

        const float newArea = newSize.Width * newSize.Height;
        if (IsLessThanReal(m_maxIslandArea, newArea))
        {
            m_maxIslandArea = newArea;

            InstrumentationNewAreaMax(newSize.Width, newSize.Height);
        }

        m_previousIslandSize = newSize;
    }
}

void CXamlIslandRoot::InstrumentationNewAreaMax(_In_ float width, _In_ float height)
{
    UINT32  uType;

    //  While these currently map to the actual values, this guards against
    //  the enum values changing and changing the payload of the telemetry
    switch (m_contentRoot->GetIslandType())
    {
        case CContentRoot::IslandType::Raw:
            uType = 1;
            break;

        case CContentRoot::IslandType::DesktopWindowContentBridge:
            uType = 3;
            break;

        case CContentRoot::IslandType::Invalid:
        default:
            uType = 0;
            break;
    }

    //  Fire event that says we've established a new max area.
    TraceLoggingWrite(
        g_hTraceProvider,
        "XamlIslandRoot-NewMaxArea",
        TraceLoggingDescription("Set new max area for this content."),
        TraceLoggingPointer((this), "IslandInstance"),
        TraceLoggingUInt32(((UINT32)uType), "IslandType"),
        TraceLoggingFloat32(((float)width), "Width"),
        TraceLoggingFloat32(((float)height), "Height"),
        TraceLoggingBoolean(TRUE, "UTCReplace_AppSessionGuid"),
        TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
}

_Check_return_ HRESULT CXamlIslandRoot::SetPublicRootVisual(
    _In_opt_ CUIElement* pRoot,
    _In_opt_ CScrollContentControl *pRootScrollViewer,
    _In_opt_ CContentPresenter *pRootContentPresenter)
{
    IFC_RETURN(m_contentRoot->GetVisualTreeNoRef()->SetPublicRootVisual(pRoot, pRootScrollViewer, pRootContentPresenter));
    return S_OK;
}

VisualTree* CXamlIslandRoot::GetVisualTreeNoRef()
{
    if (m_contentRoot)
    {
        return m_contentRoot->GetVisualTreeNoRef();
    }
    return nullptr;
}

CUIElement* CXamlIslandRoot::GetPublicRootVisual()
{
    if (m_contentRoot)
    {
        return m_contentRoot->GetVisualTreeNoRef()->GetPublicRootVisual();
    }
    return nullptr;
}

// Because of the way we render XamlIslandRoots in RenderWalk, we need to treat each XamlIslandRoot as
// a root visual for rendering.  Here we do the same thing CRootVisual does.
void CXamlIslandRoot::NWPropagateDirtyFlag(DirtyFlags flags)
{
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *host = GetContext()->GetBrowserHost();
    if (host)
    {
        ITickableFrameScheduler *scheduler = host->GetFrameScheduler();

        // There's no need to schedule another tick for tree changes during ticking.
        // Rendering the frame is the last step of ticking, so any changes that occur
        // during the tick will be picked up in the current frame.
        if (scheduler && !scheduler->IsInTick())
        {
            VERIFYHR(scheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::RootVisualDirty));
        }
    }

    __super::NWPropagateDirtyFlag(flags);
}

void CXamlIslandRoot::OnContentIslandConnected()
{
    HandleContentIslandConnectionChange(true/*isConnected*/);
}

void CXamlIslandRoot::OnContentIslandDisconnected()
{
    HandleContentIslandConnectionChange(false/*isConnected*/);
}

void CXamlIslandRoot::HandleContentIslandConnectionChange(bool isConnected)
{
    wrl::ComPtr<ixp::IContentIsland> contentIsland = GetContentIsland();

    wrl::ComPtr<InputSiteHelper::IIslandInputSite> islandInputSite;
    if (isConnected)
    {
        IFCFAILFAST(InputSiteHelper::GetIslandInputSite(contentIsland.Get(), &islandInputSite));
    }
    
    CCoreServices* coreServices = GetContext();
    CInputServices* inputServices = coreServices->GetInputServices();

    // Future: Strongly consider refactoring the XamlIslandRoot to use the InputSiteAdapter.
    if (nullptr != m_islandInputSite)
    {
        if (nullptr != islandInputSite)
        {
            wrl::ComPtr<InputSiteHelper::IIslandInputSite> newIslandInputSite{ islandInputSite };
            wrl::ComPtr<IUnknown> currentIslandInputSiteAsIUnknown;
            FAIL_FAST_IF_FAILED(m_islandInputSite.As(&currentIslandInputSiteAsIUnknown));
            wrl::ComPtr<IUnknown> newIslandInputSiteAsIUnknown;
            FAIL_FAST_IF_FAILED(newIslandInputSite.As(&newIslandInputSiteAsIUnknown));
            if (currentIslandInputSiteAsIUnknown.Get() == newIslandInputSiteAsIUnknown.Get())
            {
                // Already have the same IslandInputSite set.
                return;
            }
        }

        // If we reach here, we have an old IslandInputSite that doesn't match the new one.
        inputServices->UnregisterIslandInputSite(m_islandInputSite.Get());
    }

    m_islandInputSite = islandInputSite;

    if (m_islandInputSite)
    {
        // Pass the IslandInputSite to CInputServices. This IslandInputSite eventually makes its way to its m_pDMCrossSlideService, where
        // we use it to call IDirectManupulationManager::Activate.
        inputServices->RegisterIslandInputSite(m_islandInputSite.Get());

        FAIL_FAST_IF_FAILED(RegisterIslandDropTarget());

        // Ask the system, via the IslandInputSite whether focus rectangles should be visible.
        // If not, this can be extrapolated to assume the last input device used according to the system was not the keyboard.
        // This, in turn, instructs XAML to not draw focus rectangles.
        wrl::ComPtr<ixp::IInputFocusController3> focusController3;
        IFCFAILFAST(m_inputFocusController.As(&focusController3));

        boolean showFocusRectangles{};
        IFCFAILFAST(focusController3->get_ShouldShowKeyboardCues(&showFocusRectangles));
        if (!showFocusRectangles)
        {
            m_contentRoot->GetInputManager().SetLastInputDeviceType(DirectUI::InputDeviceType::None);
        }
        else
        {
            m_contentRoot->GetInputManager().SetLastInputDeviceType(DirectUI::InputDeviceType::Keyboard);
        }
    }
    else
    {
        // This allows the InputSite to be set to null once the ContentIsland is disconnected. We
        // make sure to set the state back to how it was before the island was connected.
        FAIL_FAST_IF_FAILED(UnregisterIslandDropTarget());

        m_contentRoot->GetInputManager().SetLastInputDeviceType(DirectUI::InputDeviceType::None);
    }
}

wrl::ComPtr<ixp::IPointerPoint> CXamlIslandRoot::GetPreviousPointerPoint()
{
    return m_previousPointerPoint;
}

_Check_return_ HRESULT CXamlIslandRoot::SetScreenOffsetOverride(_In_ wf::Point screenOffset)
{
    m_screenOffsetOverrideIsValid = true;
    m_screenOffsetOverride = screenOffset;
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::TrySetFocus(_Out_ boolean* pHasFocusNow)
{
    // Note: Make sure we do a HasFocus() check first. CFocusManager::UpdateFocus calls synchronously to here, even if
    // the focused Xaml element didn't change. We want to avoid a loop where Xaml calls the InputFocusController to set
    // focus via ::SetFocus, which pumps messages including the WM_SETFOCUS that was just posted, then Xaml gets a
    // notification that the InputFocusController got focus, and Xaml goes to focus an element via
    // CFocusManager::UpdateFocus, which calls InputFocusController to set focus right back again.
    if (!HasFocus())
    {
        // Failing to set focus isn't a fatal condition, so we'll just pass the return value.
        IFC_RETURN(m_inputFocusController->TrySetFocus(pHasFocusNow));
    }
    else
    {
        *pHasFocusNow = true;
    }

    return S_OK;
}

POINT CXamlIslandRoot::GetScreenOffset()
{
    // By design, contents are always placed on pixel boundaries, so we can ignore fractional offsets.  Just round
    // to nearest.
    if (m_screenOffsetOverrideIsValid)
    {
        // If the override has been set, that takes precedent over get_DesktopOffset
        return{ XcpRound(m_screenOffsetOverride.X), XcpRound(m_screenOffsetOverride.Y) };
    }

    // Due to an IXP bug, we cannot cache the coordinate converter and need to retrieve it from the ContentIsland.
    // https://task.ms/48700073 ContentIsland final state does not propagate to its satellite objects in time.
    auto contentIsland = GetContentIsland();

    ctl::ComPtr<mu::IClosableNotifier> closableNotifier;
    IFCFAILFAST(ctl::do_query_interface(closableNotifier, contentIsland));

    BOOLEAN isClosed;
    IFCFAILFAST(closableNotifier->get_IsClosed(&isClosed));

    if (!isClosed)
    {
        wrl::ComPtr<ixp::IContentCoordinateConverter> coordinateConverter;
        IFCFAILFAST(contentIsland->get_CoordinateConverter(&coordinateConverter));
        wf::Point origin = { 0, 0 };

        ixp::ContentLayoutDirection layoutDirection;
        IFCFAILFAST(contentIsland->get_LayoutDirection(&layoutDirection));

        if (layoutDirection == ixp::ContentLayoutDirection_RightToLeft)
        {
            // In WS_EX_LAYOUTRTL windows, (0, 0) is at the top-right corner of the window, and increasing x values
            // advance to the left. We want the top left corner, which is at (actualSize.X, 0).

            wfn::Vector2 actualSize = {};
            IFCFAILFAST(contentIsland->get_ActualSize(&actualSize));

            origin.X = actualSize.X;
        }

        wgr::PointInt32 result;
        HRESULT hr = coordinateConverter->ConvertLocalToScreenWithPoint(origin, &result);

        if (SUCCEEDED(hr))
        {
            return { result.X, result.Y };
        }
        else
        {
            return { 0, 0 };
        }
    }
    else
    {
        return { 0, 0 };
    }
}

void CXamlIslandRoot::ClientLogicalToScreenPhysical(_Inout_ POINT& pt)
{
    const POINT screenOffset = GetScreenOffset();
    const float scale = RootScale::GetRasterizationScaleForContentRoot(m_contentRoot);

    pt.x = XcpRound(pt.x * scale) + screenOffset.x;
    pt.y = XcpRound(pt.y * scale) + screenOffset.y;
}

void CXamlIslandRoot::ScreenPhysicalToClientLogical(_Inout_ POINT& pt)
{
    const POINT screenOffset = GetScreenOffset();
    const float scale = RootScale::GetRasterizationScaleForContentRoot(m_contentRoot);

    pt.x = XcpRound((pt.x - screenOffset.x) / scale);
    pt.y = XcpRound((pt.y - screenOffset.y) / scale);
}

void CXamlIslandRoot::ClientPhysicalToScreenPhysical(_Inout_ POINT& pt)
{
    const POINT screenOffset = GetScreenOffset();

    pt.x = pt.x + screenOffset.x;
    pt.y = pt.y + screenOffset.y;
}

void CXamlIslandRoot::ScreenPhysicalToClientPhysical(_Inout_ POINT& pt)
{
    const POINT screenOffset = GetScreenOffset();

    pt.x = pt.x - screenOffset.x;
    pt.y = pt.y - screenOffset.y;
}

_Check_return_ HRESULT CXamlIslandRoot::get_FocusController(_Outptr_ IUnknown** ppValue)
{
    IFC_RETURN(m_xamlFocusController.CopyTo(ppValue));
    return S_OK;
}

// Retrieve and cache the appropriate HWND that should be used as the owner for windowed popups.
void CXamlIslandRoot::InitializeOwnerHWNDForWindowedPopups(_In_ IUnknown* host)
{
    IFCFAILFAST(host->QueryInterface(IID_PPV_ARGS(m_contentBridgeDW.ReleaseAndGetAddressOf())));

    wrl::ComPtr<ixp::IDesktopSiteBridge> desktopBridge;
    IFCFAILFAST(m_contentBridgeDW.As(&desktopBridge));
    desktopBridge->get_WindowId(&m_contentBridgeWindowId);
    IFCFAILFAST(Windowing_GetWindowFromWindowId(m_contentBridgeWindowId, &m_contentBridgeWindow));
}

void CXamlIslandRoot::InitInputObjects(_In_ ixp::IContentIsland* const contentIsland)
{
    // InputFocusController
    {
        ixp::IInputFocusControllerStatics* inputFocusControllerStaticsNoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetInputFocusControllerStatics();

        IFCFAILFAST(inputFocusControllerStaticsNoRef->GetForIsland(contentIsland, &m_inputFocusController));
    }

    // InputKeyboardSource
    {
        ixp::IInputKeyboardSourceStatics2* inputKeyboardSourceStatics2NoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetInputKeyboardSourceStatics2();

        ComPtr<ixp::IInputKeyboardSource> inputKeyboardSource;
        IFCFAILFAST(inputKeyboardSourceStatics2NoRef->GetForIsland(contentIsland, &inputKeyboardSource));
        IFCFAILFAST(inputKeyboardSource.As(&m_inputKeyboardSource2));
    }

    // InputPreTranslateKeyboardSource
    {
        ixp::IInputPreTranslateKeyboardSourceStatics* inputPreTranslateKeyboardSourceStaticsNoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetInputPreTranslateKeyboardSourceStatics();

        ComPtr<ixp::IInputPreTranslateKeyboardSource> inputPreTranslateKeyboardSource;
        IFCFAILFAST(inputPreTranslateKeyboardSourceStaticsNoRef->GetForIsland(contentIsland, &inputPreTranslateKeyboardSource));
        IFCFAILFAST(inputPreTranslateKeyboardSource.As(&m_inputPreTranslateKeyboardSourceInterop));
    }

    // PreTranslateHandler
    m_preTranslateHandler.Attach(new DirectUI::PreTranslateHandler<CXamlIslandRoot>(this));

    // InputPointerSource
    {
        ixp::IInputPointerSourceStatics* inputPointerSourceStaticsNoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetInputPointerSourceStatics();
        IFCFAILFAST(inputPointerSourceStaticsNoRef->GetForIsland(contentIsland, &m_inputPointerSource));
        IFCFAILFAST(m_inputPointerSource.As(&m_inputPointerSource2));
    }

    // InputActivationListener
    {
        ixp::IInputActivationListenerStatics2* inputActivationListenerStatics2NoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetInputActivationListenerStatics2();

        IFCFAILFAST(inputActivationListenerStatics2NoRef->GetForIsland(contentIsland, &m_inputActivationListener));
    }
}

HRESULT CXamlIslandRoot::PreTranslateMessage(
    mui::IInputPreTranslateKeyboardSourceInterop* source,
    const MSG* msg,
    UINT keyboardModifiers,
    bool focusPass,
    bool* handled)
{
    xref_ptr<CContentRoot> contentRootStrongRef { m_contentRoot };

    return DirectUI::DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow()->PreTranslateMessage(
        contentRootStrongRef.get(),
        source,
        m_inputKeyboardSource2.Get(),
        msg,
        keyboardModifiers,
        focusPass,
        handled);
}

void CXamlIslandRoot::SubscribeToInputKeyboardSourceEvents()
{
    xref::weakref_ptr<CXamlIslandRoot> weakXamlIslandRoot(this);

    // PreTranslateHandler for accelerator support
    {
        m_inputPreTranslateKeyboardSourceInterop->SetPreTranslateHandler(m_preTranslateHandler.Get());
    }

    IFCFAILFAST(m_inputKeyboardSource2->add_CharacterReceived(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::CharacterReceivedEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::ICharacterReceivedEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandCharacterReceived(args);
    }).Get(), &m_characterReceivedToken));

    IFCFAILFAST(m_inputKeyboardSource2->add_KeyDown(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandKeyDown(args);
    }).Get(), &m_keyDownToken));

    IFCFAILFAST(m_inputKeyboardSource2->add_KeyUp(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandKeyUp(args);
    }).Get(), &m_keyUpToken));

    IFCFAILFAST(m_inputKeyboardSource2->add_SystemKeyDown(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandSysKeyDown(args);
    }).Get(), &m_sysKeyDownToken));

    IFCFAILFAST(m_inputKeyboardSource2->add_SystemKeyUp(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputKeyboardSource*, ixp::KeyEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputKeyboardSource* inputKeyboardSource, ixp::IKeyEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandSysKeyUp(args);
    }).Get(), &m_sysKeyUpToken));

    IFCFAILFAST(m_inputFocusController->add_GotFocus(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputFocusController*, ixp::FocusChangedEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputFocusController* inputFocusController, ixp::IFocusChangedEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        args->put_Handled(true);
        return spXamlIslandRoot->OnIslandGotFocus();
    }).Get(), &m_gotFocusToken));

    IFCFAILFAST(m_inputFocusController->add_LostFocus(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputFocusController*, ixp::FocusChangedEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputFocusController* inputFocusController, ixp::IFocusChangedEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandLostFocus();
    }).Get(), &m_lostFocusToken));
}

void CXamlIslandRoot::SubscribeToInputPointerSourceEvents()
{
    xref::weakref_ptr<CXamlIslandRoot> weakXamlIslandRoot(this);

    IFCFAILFAST(m_inputPointerSource->add_PointerCaptureLost(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerCaptureLost(args);
    }).Get(), &m_pointerCaptureLostToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerEntered(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerEntered(args);
    }).Get(), &m_pointerEnteredToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerExited(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerExited(args);
    }).Get(), &m_pointerExitedToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerMoved(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerMoved(args);
    }).Get(), &m_pointerMovedToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerPressed(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerPressed(args);
    }).Get(), &m_pointerPressedToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerReleased(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerReleased(args);
    }).Get(), &m_pointerReleasedToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerWheelChanged(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerWheelChanged(args);
    }).Get(), &m_pointerWheelChangedToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerRoutedAway(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerRoutedAway(args);
    }).Get(), &m_pointerRoutedAwayToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerRoutedReleased(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerRoutedReleased(args);
    }).Get(), &m_pointerRoutedReleasedToken));

    IFCFAILFAST(m_inputPointerSource->add_PointerRoutedTo(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandPointerRoutedTo(args);
    }).Get(), &m_pointerRoutedToToken));

    IFCFAILFAST(m_inputPointerSource2->add_DirectManipulationHitTest(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::PointerEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::IPointerEventArgs* args) -> HRESULT
    {
        auto spXamlIslandRoot = weakXamlIslandRoot.lock();
        if(spXamlIslandRoot == nullptr) { return S_OK; }

        return spXamlIslandRoot->OnIslandDirectManipulationHitTest(args);
    }).Get(), &m_directManipulationHitTestToken));

    // Temporarily disable touch hit testing pending ADO 38140531.
    //
    // IFCFAILFAST(m_inputPointerSource2->add_TouchHitTesting(
    //     WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputPointerSource*, ixp::TouchHitTestingEventArgs*>>(
    //         [weakXamlIslandRoot](ixp::IInputPointerSource* inputPointerSource, ixp::ITouchHitTestingEventArgs* args) -> HRESULT
    // {
    //     auto spXamlIslandRoot = weakXamlIslandRoot.lock();
    //     if (spXamlIslandRoot == nullptr) { return S_OK; }

    //     return spXamlIslandRoot->OnIslandTouchHitTesting(args);
    // }).Get(), &m_touchHitTestRequestedToken));
}

wrl::ComPtr<wf::ITypedEventHandler<ixp::InputNonClientPointerSource*, ixp::NonClientPointerEventArgs*>> CXamlIslandRoot::GetNonClientPointerEventHandler(PointerHandlerFunction pointerHandler, UINT msg, std::optional<bool> newContactState)
{
    xref::weakref_ptr<CXamlIslandRoot> weakXamlIslandRoot(this);

    return WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputNonClientPointerSource*, ixp::NonClientPointerEventArgs*>>(
        [weakXamlIslandRoot, pointerHandler, newContactState, msg](ixp::IInputNonClientPointerSource* inputNonClientPointerSource, ixp::INonClientPointerEventArgs* args) -> HRESULT
        {
            auto xamlIslandRoot = weakXamlIslandRoot.lock();
            if (xamlIslandRoot == nullptr) { return S_OK; }

            BOOLEAN isPointInRegion;
            IFC_RETURN(args->get_IsPointInRegion(&isPointInRegion));

            // If the mouse pointer is no longer in the NC region, we should not process the pointer
            // event, however we still have to process WM_POINTERLEAVE because we need to clear internal states, particularly last pointer point.
            if (!isPointInRegion && (msg != WM_POINTERLEAVE))
            {
                mui::PointerDeviceType pointerDeviceType;
                args->get_PointerDeviceType(&pointerDeviceType);
                if (pointerDeviceType != mui::PointerDeviceType_Mouse)
                {
                    // Clean up any dangling replay pointer data if pointer device is not mouse.
                    xamlIslandRoot->ClearLastPointerPointForReplay();
                }
                return S_OK;
            }

            wf::Point point{};
            IFC_RETURN(args->get_Point(&point));
            IFC_RETURN(xamlIslandRoot->ConvertNonClientPointToXamlCoordinates(point));

            auto wasNonClientPointerDown = xamlIslandRoot->m_isNonClientPointerDown;

            if (newContactState.has_value())
            {
                xamlIslandRoot->m_isNonClientPointerDown = newContactState.value();
            }
            wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
            IFC_RETURN(xamlIslandRoot->m_pointerPointHelper.CreateSimulatedNonClientPointerPoint(point, xamlIslandRoot->m_isNonClientPointerDown, wasNonClientPointerDown, xamlIslandRoot->m_contentBridgeWindowId, pointerPoint.ReleaseAndGetAddressOf()));

            return (xamlIslandRoot->*pointerHandler)(pointerPoint.Get());
        });
}

void CXamlIslandRoot::SubscribeToInputNonClientPointerSourceEvents()
{
    xref::weakref_ptr<CXamlIslandRoot> weakXamlIslandRoot(this);

    IFCFAILFAST(m_inputNonClientPointerSource->add_CaptionTapped(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputNonClientPointerSource*, ixp::NonClientCaptionTappedEventArgs*>>(
            [weakXamlIslandRoot](ixp::IInputNonClientPointerSource* inputNonClientPointerSource, ixp::INonClientCaptionTappedEventArgs* args) -> HRESULT
    {
        auto xamlIslandRoot = weakXamlIslandRoot.lock();
        if(xamlIslandRoot == nullptr) { return S_OK; }

        wf::Point point{};
        IFC_RETURN(args->get_Point(&point));
        IFC_RETURN(xamlIslandRoot->ConvertNonClientPointToXamlCoordinates(point));

        ctl::ComPtr<ixp::IPointerPoint> pressedPointerPoint;
        ctl::ComPtr<ixp::IPointerPoint> releasedPointerPoint;
        IFC_RETURN(xamlIslandRoot->m_pointerPointHelper.CreateSimulatedNonClientPointerPoint(point, true /* isInContact */, false /* wasPreviouslyInContact */, xamlIslandRoot->m_contentBridgeWindowId, pressedPointerPoint.ReleaseAndGetAddressOf()));
        IFC_RETURN(xamlIslandRoot->m_pointerPointHelper.CreateSimulatedNonClientPointerPoint(point, false /* isInContact */, true /* wasPreviouslyInContact */, xamlIslandRoot->m_contentBridgeWindowId, releasedPointerPoint.ReleaseAndGetAddressOf()));

        IFC_RETURN(xamlIslandRoot->OnIslandNonClientPointerPressed(pressedPointerPoint.Get()));
        return xamlIslandRoot->OnIslandNonClientPointerReleased(releasedPointerPoint.Get());
    }).Get(), &m_nonClientCaptionTappedToken));

    IFCFAILFAST(m_inputNonClientPointerSource->add_PointerEntered(GetNonClientPointerEventHandler(&CXamlIslandRoot::OnIslandNonClientPointerEntered, WM_POINTERENTER).Get(), &m_nonClientPointerEnteredToken));
    IFCFAILFAST(m_inputNonClientPointerSource->add_PointerExited(GetNonClientPointerEventHandler(&CXamlIslandRoot::OnIslandNonClientPointerExited, WM_POINTERLEAVE).Get(), &m_nonClientPointerExitedToken));
    IFCFAILFAST(m_inputNonClientPointerSource->add_PointerMoved(GetNonClientPointerEventHandler(&CXamlIslandRoot::OnIslandNonClientPointerMoved, WM_POINTERUPDATE).Get(), &m_nonClientPointerMovedToken));
    IFCFAILFAST(m_inputNonClientPointerSource->add_PointerPressed(GetNonClientPointerEventHandler(&CXamlIslandRoot::OnIslandNonClientPointerPressed, WM_POINTERDOWN, std::make_optional<bool>(true)).Get(), &m_nonClientPointerPressedToken));
    IFCFAILFAST(m_inputNonClientPointerSource->add_PointerReleased(GetNonClientPointerEventHandler(&CXamlIslandRoot::OnIslandNonClientPointerReleased, WM_POINTERUP, std::make_optional<bool>(false)).Get(), &m_nonClientPointerReleasedToken));
}

void CXamlIslandRoot::UnsubscribeToInputEvents()
{
    if (m_preTranslateHandler != nullptr)
    {
        m_inputPreTranslateKeyboardSourceInterop->SetPreTranslateHandler(nullptr);
        m_preTranslateHandler = nullptr;
    }

    if (m_characterReceivedToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_CharacterReceived(m_characterReceivedToken));
        m_characterReceivedToken.value = 0;
    }

    if(m_keyDownToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_KeyDown(m_keyDownToken));
        m_keyDownToken.value = 0;
    }

    if (m_keyUpToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_KeyUp(m_keyUpToken));
        m_keyUpToken.value = 0;
    }

    if (m_sysKeyDownToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_SystemKeyDown(m_sysKeyDownToken));
        m_sysKeyDownToken.value = 0;
    }

    if (m_sysKeyUpToken.value != 0)
    {
        IGNOREHR(m_inputKeyboardSource2->remove_SystemKeyUp(m_sysKeyUpToken));
        m_sysKeyUpToken.value = 0;
    }

    if (m_gotFocusToken.value != 0)
    {
        IGNOREHR(m_inputFocusController->remove_GotFocus(m_gotFocusToken));
        m_gotFocusToken.value = 0;
    }

    if(m_lostFocusToken.value != 0)
    {
        IGNOREHR(m_inputFocusController->remove_LostFocus(m_lostFocusToken));
        m_lostFocusToken.value = 0;
    }

    if (m_pointerCaptureLostToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerCaptureLost(m_pointerCaptureLostToken));
        m_pointerCaptureLostToken.value = 0;
    }

    if (m_pointerEnteredToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerEntered(m_pointerEnteredToken));
        m_pointerEnteredToken.value = 0;
    }

    if(m_pointerExitedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerExited(m_pointerExitedToken));
        m_pointerExitedToken.value = 0;
    }

    if(m_pointerMovedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerMoved(m_pointerMovedToken));
        m_pointerMovedToken.value = 0;
    }

    if (m_pointerPressedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerPressed(m_pointerPressedToken));
        m_pointerPressedToken.value = 0;
    }

    if (m_pointerReleasedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerReleased(m_pointerReleasedToken));
        m_pointerReleasedToken.value = 0;
    }

    if (m_pointerRoutedAwayToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerRoutedAway(m_pointerRoutedAwayToken));
        m_pointerRoutedAwayToken.value = 0;
    }

    if (m_pointerRoutedReleasedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerRoutedReleased(m_pointerRoutedReleasedToken));
        m_pointerRoutedReleasedToken.value = 0;
    }

    if (m_pointerRoutedToToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerRoutedTo(m_pointerRoutedToToken));
        m_pointerRoutedToToken.value = 0;
    }

    if (m_pointerWheelChangedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource->remove_PointerWheelChanged(m_pointerWheelChangedToken));
        m_pointerWheelChangedToken.value = 0;
    }

    if (m_touchHitTestRequestedToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource2->remove_TouchHitTesting(m_touchHitTestRequestedToken));
        m_touchHitTestRequestedToken.value = 0;
    }

    if (m_nonClientCaptionTappedToken.value != 0)
    {
        IGNOREHR(m_inputNonClientPointerSource->remove_CaptionTapped(m_nonClientCaptionTappedToken));
        m_nonClientCaptionTappedToken.value = 0;
    }

    if (m_nonClientPointerEnteredToken.value != 0)
    {
        IGNOREHR(m_inputNonClientPointerSource->remove_PointerEntered(m_nonClientPointerEnteredToken));
        m_nonClientPointerEnteredToken.value = 0;
    }

    if (m_nonClientPointerExitedToken.value != 0)
    {
        IGNOREHR(m_inputNonClientPointerSource->remove_PointerExited(m_nonClientPointerExitedToken));
        m_nonClientPointerExitedToken.value = 0;
    }

    if (m_nonClientPointerMovedToken.value != 0)
    {
        IGNOREHR(m_inputNonClientPointerSource->remove_PointerMoved(m_nonClientPointerMovedToken));
        m_nonClientPointerMovedToken.value = 0;
    }

    if (m_nonClientPointerPressedToken.value != 0)
    {
        IGNOREHR(m_inputNonClientPointerSource->remove_PointerPressed(m_nonClientPointerPressedToken));
        m_nonClientPointerPressedToken.value = 0;
    }

    if (m_nonClientPointerReleasedToken.value != 0)
    {
        IGNOREHR(m_inputNonClientPointerSource->remove_PointerReleased(m_nonClientPointerReleasedToken));
        m_nonClientPointerReleasedToken.value = 0;
    }

    if (m_activationChangedToken.value != 0)
    {
        IGNOREHR(m_inputActivationListener->remove_InputActivationChanged(m_activationChangedToken));
        m_activationChangedToken.value = 0;
    }

    if (m_directManipulationHitTestToken.value != 0)
    {
        IGNOREHR(m_inputPointerSource2->remove_DirectManipulationHitTest(m_directManipulationHitTestToken));
        m_directManipulationHitTestToken.value = 0;
    }

    m_inputFocusController = nullptr;
    m_inputKeyboardSource2 = nullptr;
    m_inputPointerSource = nullptr;
    m_inputPointerSource2 = nullptr;
    m_inputActivationListener = nullptr;
    m_inputPreTranslateKeyboardSourceInterop = nullptr;
}

_Check_return_ HRESULT CXamlIslandRoot::ConvertNonClientPointToXamlCoordinates(wf::Point& point)
{
    ABI::Microsoft::UI::WindowId appWindowId;
    IFC_RETURN(m_topLevelHost->get_AppWindowId(&appWindowId));

    HWND appWindowHwnd = nullptr;
    IFC_RETURN(Windowing_GetWindowFromWindowId(appWindowId, &appWindowHwnd));
    IFCEXPECT_RETURN(appWindowHwnd);

    DWORD dwExStyle = GetWindowLong(appWindowHwnd, GWL_EXSTYLE);
    bool isRtl = (dwExStyle & WS_EX_LAYOUTRTL) == WS_EX_LAYOUTRTL;

    if (isRtl)
    {
        // If the AppWindow is RTL, then the x-coordinate we're receiving will be flipped.
        // XAML expects coordinates it gets to be in unflipped LTR coordinates, so we'll
        // flip it back before handing it off to the XAML input stack.
        RECT windowRect;
        IFCEXPECT_RETURN(GetWindowRect(appWindowHwnd, &windowRect));

        long windowWidth = windowRect.right - windowRect.left;
        point.X = windowWidth - point.X;
    }

    // This point is in scaled physical pixels. Convert it to the non-scaled relative pixels XAML expects.
    float rasterizationScale = RootScale::GetRasterizationScaleForContentRoot(m_contentRoot);
    point.X /= rasterizationScale;
    point.Y /= rasterizationScale;

    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::SetPointerCapture()
{   // TODO: Remove this dead function
    m_hasCapture = true;
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::ReleasePointerCapture()
{   // TODO: Remove this dead function
    m_hasCapture = false;
    return S_OK;
}

bool CXamlIslandRoot::HasPointerCapture()
{   // TODO: Remove this dead function
    return m_hasCapture;
}

CPopupRoot* CXamlIslandRoot::GetPopupRootNoRef()
{
    if (m_contentRoot)
    {
        return m_contentRoot->GetVisualTreeNoRef()->GetPopupRoot();
    }
    return nullptr;
}

CTransitionRoot* CXamlIslandRoot::GetTransitionRootNoRef()
{
    if (m_contentRoot)
    {
        return m_contentRoot->GetVisualTreeNoRef()->GetTransitionRoot();
    }
    return nullptr;
}

_Check_return_ HRESULT CXamlIslandRoot::EnterImpl(_In_ CDependencyObject *namescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(CPanel::EnterImpl(namescopeOwner, params))
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::LeaveImpl(_In_ CDependencyObject *namescopeOwner, _In_ LeaveParams params)
{
    IFC_RETURN(CPanel::LeaveImpl(namescopeOwner, params));
    if (m_contentRoot)
    {
        IFC_RETURN(m_contentRoot->Close());
        m_contentRoot = nullptr;
    }
    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::get_DragDropManager(_COM_Outptr_result_maybenull_ mui::DragDrop::IDragDropManager** manager)
{
    *manager = nullptr;
    if (m_dragDropManager)
    {
        IFC_RETURN(m_dragDropManager.CopyTo(manager));
    }
    return S_OK;
}

bool CXamlIslandRoot::HasFocus() const
{
    if (m_inputFocusController)
    {
        boolean hasFocus {FALSE};
        if (SUCCEEDED(m_inputFocusController->get_HasFocus(&hasFocus)))
        {
            return !!hasFocus;
        }
    }
    return false;
}

_Check_return_ HRESULT CXamlIslandRoot::GenerateChildOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* bounds)
{
    //
    // If there are open parented popups, then the element tree looks something like:
    //
    //      <XamlIslandRoot>
    //          <PopupRoot>
    //              <Grid ChildOfPopup />
    //          </PopupRoot>
    //          <Border PublicRoot>
    //              <Canvas>
    //                  <Popup Child="ChildOfPopup" />
    //              </Canvas>
    //          </Border>
    //      </XamlIslandRoot>
    //
    // If the Canvas gets a different render transform, it'll propagate bounds dirty up the tree to the Border
    // and the XamlIslandRoot. But it also moved the position of the Popup and the Grid, which means the
    // PopupRoot's bounds should also be marked dirty (since it contains the Grid).
    //
    // There's no mechanism for propagating dirty flags down the tree, so we'll miss this Popup scenario.
    // The workaround is to always assume that the PopupRoot's bounds are dirty whenever the XamlIslandRoot's
    // bounds are dirty. That guarantees that we'll recompute the PopupRoot's bounds, and that we'll correctly
    // pass a point to it for the next bounds pass.
    //
    CPopupRoot* popupRoot = GetPopupRootNoRef();
    if (popupRoot != nullptr)
    {
        popupRoot->InvalidateChildBounds();
    }

    IFC_RETURN(__super::GenerateChildOuterBounds(hitTestParams, bounds));

    return S_OK;
}

_Check_return_ HRESULT CXamlIslandRoot::EffectiveViewportWalkCore(
    _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
    _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports,
    _Out_ bool& addedViewports)
{
    wf::Size xamlIslandRootsize = GetSize();

    // XAML content roots are not viewports per se, but we still want to clip
    // any viewports to the size of the root.  So we'll add our bounds here.
    horizontalViewports.emplace_back(0.0f, static_cast<float>(xamlIslandRootsize.Width));
    verticalViewports.emplace_back(0.0f, static_cast<float>(xamlIslandRootsize.Height));

    // We set addedViewports to false since these aren't real viewports
    // and we don't have any transforms associated with them.
    // If we set it to true, then we'll try to remove an entry from
    // transformsToViewports when we remove these entries from the viewport vectors,
    // which will lead to us trying to free memory that we didn't allocate.
    addedViewports = false;

    return S_OK;
}

void CXamlIslandRoot::ClearLastPointerPointForReplay()
{
    m_previousPointerPoint = nullptr;
    m_previousPointerEventArgs = nullptr;
    m_previousPointerPointIsNonClient = false;
}

void CXamlIslandRoot::UpdateLastPointerPointForReplay(const UINT uMsg, _In_ ixp::IPointerPoint* pointerPoint, _In_opt_ ixp::IPointerEventArgs* pointerEventArgs, bool previousPointerPointIsNonClient)
{
    mui::PointerDeviceType pointerDeviceType;
    IFCFAILFAST(pointerPoint->get_PointerDeviceType(&pointerDeviceType));

    // Save off the last pointer message so we can replay it.
    // Note that we will replay pointer down/up as updates.  This allows us to handle the case where there is
    // slight movement between the last update and the release as well as if the down/up cause
    // changes in the scene (e.g. clicking a button deletes that button).
    // Also only certain types of input (e.g. mouse) can be replayed. All mouse messages not addressed here
    // should be considered as not invalidating the previously cached pointer point value.
    if ((uMsg == WM_POINTERUPDATE ||
        uMsg == WM_POINTERDOWN ||
        uMsg == WM_POINTERUP) &&
        pointerDeviceType == mui::PointerDeviceType_Mouse)
    {
        m_previousPointerPoint = pointerPoint;
        m_previousPointerEventArgs = pointerEventArgs;
        m_previousPointerPointIsNonClient = previousPointerPointIsNonClient;
    }
    else if (uMsg == WM_POINTERLEAVE ||
        uMsg == WM_POINTERCAPTURECHANGED ||
        pointerDeviceType != mui::PointerDeviceType_Mouse)
    {
        // If our pointer is leaving, then we should have no reason to ever replay the previous
        // update. In addition, we only play mouse messages, so if the current message is not a
        // mouse message then we should clear the previously cached mouse message because it is
        // now stale.
        m_previousPointerPoint = nullptr;
        m_previousPointerEventArgs = nullptr;
        m_previousPointerPointIsNonClient = false;
    }

    // Clear any input cached by any open windowed popups. Those are no longer the most recent - the most recent input
    // is the one that we just cached on this CXamlIslandRoot.
    if (CPopupRoot* popupRoot = GetPopupRootNoRef())
    {
        popupRoot->ClearLastPointerPointForReplay();
    }
}

