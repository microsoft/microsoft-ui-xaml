// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <d3d11device.h>
#include <ImageReloadManager.h>
#include <InputServices.h>
#include <isapipresent.h>
#include "JupiterControl.h"
#include "JupiterWindow.h"
#include <KeyboardUtility.h>
#include <ResourceManager.h>
#include <RootScale.h>
#include "UIAWrapper.h"
#include <WindowsGraphicsDeviceManager.h>
#include "winpal.h"
#include <XamlOneCoreTransforms.h>
#include "xcpwindow.h"

extern HINSTANCE g_hInstance;

extern "C" ULONG APIENTRY GdiEntry13(VOID);
#define DdQueryDisplaySettingsUniqueness    GdiEntry13

#define LOCALE_CHANGE_MSG L"intl"
#define IMMERSIVE_COLORSET_CHANGE_MSG L"ImmersiveColorSet"

_Check_return_ HRESULT CJupiterControl::Create(_Outptr_ CJupiterControl** ppControl)
{
    HRESULT hr = S_OK;
    CJupiterControl* pControl = nullptr;

    pControl = new CJupiterControl();

    IFC(pControl->Init());

    *ppControl = pControl;
    pControl = nullptr;

Cleanup:
    ReleaseInterface(pControl);
    return hr;
}

CJupiterControl::CJupiterControl() :
    m_pWindow(nullptr),
    m_pUIAWindow(nullptr),
    m_isEnabledMockUIAClientsListening(false)
{
}

CJupiterControl::~CJupiterControl()
{
    if (IsInit())
    {
        Deinitialize();
    }
}

_Check_return_ XHANDLE CJupiterControl::GetXcpControlWindow()
{
    if (m_pWindow)
    {
        return m_pWindow->GetWindowHandle();
    }
    else
    {
        return nullptr;
    }
}

_Check_return_ HRESULT CJupiterControl::OnFirstFrameDrawn()
{
    return m_pWindow->NotifyFirstFrameDrawn();
}

bool CJupiterControl::IsWindowDestroyed()
{
    return m_pWindow && m_pWindow->IsWindowDestroyed();
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//      Override for Jupiter-specific control initialization.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterControl::Init()
{
    HRESULT hr = S_OK;
    IPlatformUtilities* pUtilities = nullptr;
    IErrorService* pErrorService = nullptr;
    CJupiterErrorServiceListener* pErrorServiceListener = nullptr;

    if (IsInit())
    {
        goto Cleanup;
    }

    IFC(gps->GetPlatformUtilities(&pUtilities));
    CControlBase::setPlatformUtilities(pUtilities);

    ReleaseInterface(m_pXcpHostCallback);
    m_pXcpHostCallback = new CJupiterXcpHostCallback();

    // its ok to not have a URI, in which case do nothing
    IFC(CControlBase::GetClientURI());

    ReleaseInterface(m_pDispatcher);
    IFC(CXcpDispatcher::Create(this, &m_pDispatcher));
    IFC(m_pDispatcher->Start());

    IFC(CControlBase::Init());

    IFC(m_pBH->GetErrorService(&pErrorService));

    pErrorServiceListener = new CJupiterErrorServiceListener();

    IFC(pErrorService->AddListener(pErrorServiceListener));

    m_fInitialized = true;
Cleanup:
    ReleaseInterface(pErrorServiceListener);

    if (FAILED(hr))
    {
        Deinitialize();
    }

    return hr;
}

void CJupiterControl::Deinitialize()
{
    if (IsInit())
    {
        // Reset the visual tree.  This will ensure that media playback will be
        // stopped, regardless of when IE finally releases the control.
        if (m_pBH)
        {
            IGNOREHR(ResetVisualTree());
        }

        DisconnectUIA();

        CControlBase::Deinit();

        m_fInitialized = false;
    }

    ReleaseInterface(m_pWindow);
}

void CJupiterControl::DisconnectUIA()
{
    if (m_pUIAWindow)
    {
        m_pUIAWindow->UIADisconnectAllProviders();
        m_pUIAWindow->Deinit();
        m_pUIAWindow = nullptr;
    }
}

_Check_return_ HRESULT CJupiterControl::ResetVisualTree()
{
    m_pBH->SetShuttingDown(TRUE);
    HRESULT hr = m_pBH->ResetVisualTree();
    m_pBH->SetShuttingDown(FALSE);

    return hr;
}

HRESULT CJupiterXcpHostCallback::GetBASEUrlFromDocument(
    _Out_ IPALUri** ppBaseUri,
    _Out_ IPALUri** ppDocUri)
{
    HRESULT hr = S_OK;
    IPALUri *pBaseUri = nullptr;
    IPALUri *pDocUri = nullptr;
    const WCHAR pApplicationBaseUri[] = L"ms-resource:///Files/";
    const size_t applicationBaseUriLength = ARRAY_SIZE(pApplicationBaseUri);

    IFC(gps->UriCreate(applicationBaseUriLength, const_cast<WCHAR*>(pApplicationBaseUri), &pDocUri));

    IFC(pDocUri->CreateBaseURI(&pBaseUri));

    *ppBaseUri = pBaseUri;
    *ppDocUri = pDocUri;

    pBaseUri = nullptr;
    pDocUri = nullptr;

Cleanup:
    ReleaseInterface(pBaseUri);
    ReleaseInterface(pDocUri);
    return hr;
}

bool CJupiterControl::HandleWindowMessage(UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot)
{
    bool fHandled = false;

    if (!IsInit())
    {
        goto Cleanup;
    }

    switch (uMsg)
    {
        case WM_PAINT:
            IFCFAILFAST(UpdateHdr());
            Paint();
            fHandled = TRUE;
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            TraceKeyDownBegin(InputUtility::Keyboard::GetVirtualKeyForETWLogging(uMsg, wParam, lParam), InputUtility::Keyboard::GetRepeatCount(uMsg, wParam, lParam), contentRoot->GetInputManager().GetKeyDownCountBeforeSubmitFrame());
            fHandled = HandleKeyMessage(uMsg, wParam, lParam, contentRoot);
            int count = contentRoot->GetInputManager().GetKeyDownCountBeforeSubmitFrame();
            TraceKeyDownEnd(InputUtility::Keyboard::GetVirtualKeyForETWLogging(uMsg, wParam, lParam), InputUtility::Keyboard::GetRepeatCount(uMsg, wParam, lParam), count);
            contentRoot->GetInputManager().SetKeyDownCountBeforeSubmitFrame(count+1);
            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
            TraceKeyUpBegin(InputUtility::Keyboard::GetVirtualKeyForETWLogging(uMsg, wParam, lParam), InputUtility::Keyboard::GetRepeatCount(uMsg, wParam, lParam));
            fHandled = HandleKeyMessage(uMsg, wParam, lParam, contentRoot);
            TraceKeyUpEnd(InputUtility::Keyboard::GetVirtualKeyForETWLogging(uMsg, wParam, lParam), InputUtility::Keyboard::GetRepeatCount(uMsg, wParam, lParam));
            break;
        case WM_CHAR:
        case WM_DEADCHAR:
            fHandled = HandleKeyMessage(uMsg, wParam, lParam, contentRoot);
            break;
        case WM_ACTIVATE:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_CONTEXTMENU:
        case WM_INPUTLANGCHANGE:
        case WM_MOVE:
            fHandled = HandleGenericMessage(uMsg, wParam, lParam, contentRoot);
            break;

        case WM_UPDATEUISTATE:
            fHandled = HandleUpdateUIStateMessage(uMsg, wParam, lParam, contentRoot);
            break;

        case WM_POINTERUPDATE:
            TracePointerUpdateBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerUpdateEnd();
            break;

        case WM_POINTERDOWN:
            TracePointerDownBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerDownEnd();
            break;

        case WM_POINTERUP:
            TracePointerUpBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerUpEnd();
            break;

        case WM_POINTERENTER:
            TracePointerEnterBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerEnterEnd();
            break;

        case WM_POINTERLEAVE:
            TracePointerLeaveBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerLeaveEnd();
            break;

        case WM_POINTERWHEEL:
            TracePointerWheelBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerWheelEnd();
            break;

        case WM_POINTERHWHEEL:
            TracePointerHWheelBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerHWheelEnd();
            break;

        case WM_POINTERCAPTURECHANGED:
            TracePointerCaptureChangedBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TracePointerCaptureChangedEnd();
            break;

        case DM_POINTERHITTEST:
            TraceDmPointerHitTestBegin();
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            TraceDmPointerHitTestEnd();
            break;

        case WM_CAPTURECHANGED:
            fHandled = HandlePointerMessage(uMsg, wParam, lParam, contentRoot);
            break;
        case WM_POINTERROUTEDAWAY:
            HandlePointerMessage(uMsg, wParam, lParam, contentRoot);

            // RS5 Bug #7653165:  Marking WM_POINTERROUTEDAWAY as handled sometimes causes confusion in CoreWindow book-keeping,
            // causing random crashes.  We are not letting CoreWindow process WM_POINTERROUTEDAWAY, but then we are letting
            // it process WM_POINTERROUTEDRELEASED since we don't handle it.
            // The fix is to always allow CoreWindow a chance to process both messages, to keep their book-keeping consistent.
            fHandled = FALSE;
            break;
    }

Cleanup:
    return fHandled;
}

void CJupiterControl::OnDisplayChanged()
{
    IXcpBrowserHost* pbh = GetBrowserHost();
    if (pbh)
    {
        pbh->ForceRedraw();
    }
}

bool CJupiterControl::HandleGenericMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot)
{
    bool bHandled = false;

    if (m_pBH)
    {
        MsgPacket msgPack;
        // Package wParam and lParam
        msgPack.m_wParam = wParam;
        msgPack.m_lParam = lParam;

        msgPack.m_hwnd = static_cast<HWND>(m_pWindow->GetWindowHandle());
        msgPack.m_pCoreWindow = m_pWindow->GetCoreWindowNoRef();

        // pass complete message to browser host
        m_pBH->HandleInputMessage(uMsg, &msgPack, contentRoot, false /* isReplayedMessage */, bHandled);
    }

    return bHandled;
}

bool CJupiterControl::HandleKeyMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot)
{
    bool bHandled = false;

    if (m_pBH)
    {
        MsgPacket msgPack;
        // Package wParam and lParam
        msgPack.m_wParam = wParam;
        msgPack.m_lParam = lParam;

        msgPack.m_hwnd = static_cast<HWND>(m_pWindow->GetWindowHandle());
        msgPack.m_pCoreWindow = m_pWindow->GetCoreWindowNoRef();

        // Shim around browser weirdness

        // Jolt #5049
        if( WM_KEYDOWN == uMsg && 0 == wParam )
        {
            // Workaround for bug in Windows Firefox: Occasionally we would
            //  get a KEYDOWN event when no key has been pressed.  Without Firefox
            //  source it's difficult to speculate on why this event shows up.
            // What we do know is that this KEYDOWN event doesn't have a valid key
            //  associated with it.  The Virtual Key (VK_*) #define in winuser.h
            //  starts with 0x01.  Zero doesn't mean anything.
            // When a KEYDOWN comes in with VK zero, we pretend nothing happened.  (Jolt #5049)
            bHandled = FALSE;
            goto Cleanup;
        }

        // pass complete message to browser host
        m_pBH->HandleInputMessage(uMsg, &msgPack, contentRoot, false /* isReplayedMessage */, bHandled);
    }
Cleanup:
    return bHandled;
}

_Check_return_ HRESULT CJupiterControl::OnJupiterWindowSizeChanged(CJupiterWindow &jupiterWindow, HWND hwndContext)
{
    // TODO: As part of 11236439, change this from physical size to logical size
    XSIZE size = jupiterWindow.GetJupiterWindowPhysicalSize(hwndContext);
    if (size.Width > 0 && size.Height > 0)
    {
        IFC_RETURN(m_pBH->SetWindowSize(size, hwndContext));
    }
    return S_OK;
}

_Check_return_ HRESULT CJupiterControl::SetWindow(_In_ CJupiterWindow* pWindow)
{
    IFCEXPECT_ASSERT_RETURN(!m_pWindow);
    m_pWindow = pWindow;

    // Only try to Show in the CoreWindow case. For desktop each DesktopWindow manages itself.
    if (pWindow->GetWindowHandle())
    {
        IFC_RETURN(m_pWindow->ShowWindow());
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterControl::ActivateWindow()
{
    return m_pWindow->Activate();
}

_Check_return_ HRESULT CJupiterControl::NotifyFirstFramePending()
{
    return m_pWindow->NotifyFirstFramePending();
}

_Check_return_ HRESULT CJupiterControl::UpdateHdr() const
{
    // If we don't have a window then there is nothing we can do
    if (m_pWindow == nullptr)
    {
        return S_OK;
    }

    // First determine whether we need to look for new Hdr info or not.
    ULONG displaySettingsUniqueness = DdQueryDisplaySettingsUniqueness();
    const bool displaySettingsUniquenessChanged = (m_lastDisplaySettingsUniqueness != displaySettingsUniqueness);

    // When window rect changes the monitor may change as well. Recompute HDR flag when monitor changes.
    // Note: windowRect is either logical units (under onecore transforms) or physical units (all other cases).
    // This doesn't really matter because all we are doing is just seeing if the rectangle has changed
    // since the previous call.
    RECT windowRect = {};
    if (XamlOneCoreTransforms::IsEnabled())
    {
        wf::Rect bounds;
        IFC_RETURN(m_pWindow->GetCoreWindowNoRef()->get_Bounds(&bounds));
        windowRect.left = static_cast<LONG>(bounds.X);
        windowRect.top = static_cast<LONG>(bounds.Y);
        windowRect.right = static_cast<LONG>(bounds.X + bounds.Width);
        windowRect.bottom = static_cast<LONG>(bounds.Y + bounds.Height);
    }
    else
    {
        GetWindowRect(m_pWindow->GetWindowHandle(), &windowRect);
    }

    const bool windowRectChanged = (memcmp(&m_lastWindowRect, &windowRect, sizeof(RECT)) != 0);

    // Nothing has changed so we can bail.
    if (!displaySettingsUniquenessChanged && !windowRectChanged)
    {
        return S_OK;
    }

    // See if the Hdr state has changed.
    bool newHdr = m_cachedIsHdr;

    if (XamlOneCoreTransforms::IsEnabled())
    {
        // Use display information to determine Hdr capability
        ctl::ComPtr<wgrd::IDisplayInformation> displayInformation(DirectUI::DXamlCore::GetCurrent()->GetDisplayInformationNoRef());
        IFCPTR_RETURN(displayInformation.Get());

        ctl::ComPtr<wgrd::IDisplayInformation5> displayInformation5;
        IFCFAILFAST(displayInformation.As(&displayInformation5));
        ctl::ComPtr<wgrd::IAdvancedColorInfo> colorInfo;
        IFC_RETURN(displayInformation5->GetAdvancedColorInfo(&colorInfo));
        boolean isHdr10Supported = false;
        IFC_RETURN(colorInfo->IsHdrMetadataFormatCurrentlySupported(wgrd::HdrMetadataFormat_Hdr10, &isHdr10Supported));
        if (!isHdr10Supported)
        {
            IFC_RETURN(colorInfo->IsHdrMetadataFormatCurrentlySupported(wgrd::HdrMetadataFormat_Hdr10Plus, &isHdr10Supported));
        }
        newHdr = !!isHdr10Supported;
    }
    else
    {
        // Use HMONITOR and the display outputs to deterine Hdr capability
        HMONITOR monitor = MonitorFromWindow(m_pWindow->GetWindowHandle(), MONITOR_DEFAULTTONEAREST);
        if (displaySettingsUniquenessChanged || monitor != m_lastMonitor)
        {
            auto deviceManager = GetBrowserHost()->GetGraphicsDeviceManager();
            CD3D11Device* device = deviceManager ? deviceManager->GetGraphicsDevice() : nullptr;
            if (device != nullptr)
            {
                HRESULT hr = device->IsHdrOutput(monitor, &newHdr);
                if (GraphicsUtility::IsDeviceLostError(hr))
                {
                    // Keep cached values unchanged so we may check them next time
                    return S_OK;
                }
                IFC_RETURN(hr);

                m_lastMonitor = monitor;
            }
            else
            {
                newHdr = false;
            }
        }
    }

    // Now that we are past the point where a device lost would prematurely exit, save off the
    // values that we retrieved
    m_lastDisplaySettingsUniqueness = displaySettingsUniqueness;
    m_lastWindowRect = windowRect;

    // Reload images only when the cached HDR flag was valid
    bool needReload = m_isCachedHdrValid && (newHdr != m_cachedIsHdr);

    m_cachedIsHdr = newHdr;
    m_isCachedHdrValid = true;

    if (needReload)
    {
        const auto coreServices = GetCoreServices();
        const auto& roots = coreServices->GetContentRootCoordinator()->GetContentRoots();
        for (const auto& root : roots)
        {
            if (const auto rootScale = RootScale::GetRootScaleForContentRoot(root))
            {
                CImageReloadManager& imageReloadManager = rootScale->GetImageReloadManager();
                IFC_RETURN(imageReloadManager.ReloadImages(ResourceInvalidationReason::HdrChanged));
            }
        }
    }

    return S_OK;
}

void CJupiterControl::Paint()
{
    PAINTSTRUCT ps = {};

    if (!m_pWindow)
    {
        return;
    }

    //
    // Note: the painting and drawing NTUSER APIs are not present on all OS
    // images. Instead of performing a target SKU check, we will instead rely
    // on the ext-ms-win-ntuser-draw-l1 API set extension to preserve the
    // client core behavior while enabling code sharing with non-client SKUs.
    //
    if (IsBeginPaintPresent())
    {
        BeginPaint(m_pWindow->GetWindowHandle(), &ps);
    }
    else
    {
        // Need to tell USER that we've handled the WM_PAINT request.
        ::RedrawWindow(m_pWindow->GetWindowHandle(), nullptr, nullptr, RDW_NOINTERNALPAINT);
    }

    IGNOREHR(OnPaint());

    if (IsEndPaintPresent())
    {
        EndPaint(m_pWindow->GetWindowHandle(), &ps);
    }
}

bool CJupiterControl::HandlePointerMessage(
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_opt_ CContentRoot* contentRoot,
    bool isReplayedMessage,
    _In_opt_ ixp::IPointerPoint* pointerPoint,
    _In_opt_ ixp::IPointerEventArgs* pointerEventArgs)
{
    if (uMsg == WM_CAPTURECHANGED)
    {
        // XAML doesn't do anything with WM_CAPTURECHANGED.  Early-out here to avoid unnecessary work.
        return false;
    }

    bool bHandled = false;

    if (m_pBH)
    {
        MsgPacket msgPack;
        // Package wParam and lParam
        msgPack.m_wParam = wParam;
        msgPack.m_lParam = lParam;

        // package window handle and corewindow pointer
        msgPack.m_hwnd        = m_pWindow->GetWindowHandle();
        msgPack.m_pCoreWindow = m_pWindow->GetCoreWindowNoRef();

        msgPack.m_pPointerPointNoRef = pointerPoint;
        msgPack.m_pPointerEventArgsNoRef = pointerEventArgs;

        // pass complete message to browser host
        m_pBH->HandleInputMessage(uMsg, &msgPack, contentRoot, isReplayedMessage, bHandled);
    }

    return bHandled;
}

void CJupiterControl::HandleNonClientPointerMessage(
    _In_ UINT uMsg,
    _In_ UINT32 pointerId,
    _In_opt_ CContentRoot* contentRoot,
    bool isGeneratedMessage,
    _In_opt_ ixp::IPointerPoint* pointerPoint)
{
    if (m_pBH)
    {
        MsgPacket msgPack;
        msgPack.m_wParam = pointerId;
        msgPack.m_hwnd = m_pWindow->GetWindowHandle();
        msgPack.m_pPointerPointNoRef = pointerPoint;
        msgPack.m_isNonClientPointerMessage = true;

        bool bHandled = false;
        m_pBH->HandleInputMessage(uMsg, &msgPack, contentRoot, isGeneratedMessage, bHandled);
    }
}

LRESULT CJupiterControl::HandleGetObjectMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    LRESULT lResult = 0;

    if (m_pBH)
    {
        HWND hWnd = nullptr;

        hWnd = static_cast<HWND>(m_pWindow->GetWindowHandle());

        if (m_pUIAWindow == nullptr)
        {
            IFC(CreateUIAHostWindowForHwnd(hWnd));
        }

        IFCPTR(m_pUIAWindow);

        wrl::ComPtr<::IRawElementProviderSimple> uiaWindowAsRawElementProviderSimple;
        IFC(m_pUIAWindow.As(&uiaWindowAsRawElementProviderSimple));
        lResult = ::UiaReturnRawElementProvider(hWnd, wParam, lParam, uiaWindowAsRawElementProviderSimple.Get());

        m_pUIAWindow->SetMockUIAClientsListening(m_isEnabledMockUIAClientsListening);
    }

Cleanup:
    return lResult;
}

HRESULT CJupiterControl::CreateUIAHostWindowForHwnd(_In_ HWND hwnd)
{
    RETURN_IF_FAILED(CUIAHostWindow::Create(
                        UIAHostEnvironmentInfo(hwnd, hwnd),
                        this,
                        nullptr,  // pRootVisual
                        &m_pUIAWindow));
    return S_OK;
}

_Check_return_ HRESULT CJupiterControl::CreateProviderForAP(_In_ CAutomationPeer* pAP, _Outptr_result_maybenull_ CUIAWrapper** ppRet)
{
    *ppRet = nullptr;

    if (m_pUIAWindow)
    {
        IFC_RETURN(m_pUIAWindow->CreateProviderForAP(pAP, ppRet));
    }

    return S_OK;
}

HRESULT CJupiterControl::NotifyImmersiveColorSetChanged()
{
    DirectUI::DXamlCore* pCore = DirectUI::DXamlCore::GetCurrentNoCreate();
    if (pCore)
    {
        IFC_RETURN(pCore->NotifyImmersiveColorsChanged());
    }

    return S_OK;
}

HRESULT CJupiterControl::OnThemeChanged()
{
    DirectUI::DXamlCore* pCore = DirectUI::DXamlCore::GetCurrentNoCreate();
    if (pCore)
    {
        IFC_RETURN(pCore->OnThemeChanged());
    }

    return S_OK;
}

HRESULT CJupiterControl::UpdateFontScale(_In_ XFLOAT newFontScale)
{
    DirectUI::DXamlCore* pCore = DirectUI::DXamlCore::GetCurrentNoCreate();
    if (pCore)
    {
        IFC_RETURN(pCore->UpdateFontScale(newFontScale));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CJupiterControl::UIAClientsAreListening
//
//  Synopsis:
//      Checks if clients are listening to a specific event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterControl::UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent)
{
    HRESULT hr = S_FALSE;

    if (m_pUIAWindow)
    {
        // Default here is S_FALSE, not S_OK, so check the HRESULT like this instead of IFC_RETURN
        hr = m_pUIAWindow->UIAClientsAreListening(eAutomationEvent);
    }

    return hr;
}

//-------------------------------------------------------------------------
//
//  Function:   CJupiterControl::UIARaiseAutomationEvent
//
//  Synopsis:
//      Raises a UIA Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterControl::UIARaiseAutomationEvent(
    _In_ CAutomationPeer *pAP,
    _In_ UIAXcp::APAutomationEvents eAutomationEvent)
{
    if (m_pUIAWindow)
    {
        IFC_RETURN(m_pUIAWindow->UIARaiseAutomationEvent(pAP, eAutomationEvent));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CJupiterControl::UIARaiseAutomationPropertyChangedEvent
//
//  Synopsis:
//      Raises a UIA Property Changed Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterControl::UIARaiseAutomationPropertyChangedEvent(
    _In_ CAutomationPeer *pAP,
    _In_ UIAXcp::APAutomationProperties eAutomationProperty,
    _In_ const CValue& oldValue,
    _In_ const CValue& newValue)
{
    if (m_pUIAWindow)
    {
        IFC_RETURN(m_pUIAWindow->UIARaiseAutomationPropertyChangedEvent(pAP, eAutomationProperty, oldValue, newValue));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CJupiterControl::UIARaiseFocusChangedEventOnUIAWindow
//
//  Synopsis:
//      Raises a UIA Focus Changed Event on UIAWindow
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterControl::UIARaiseFocusChangedEventOnUIAWindow()
{
    if (m_pUIAWindow)
    {
        IFC_RETURN(m_pUIAWindow->UIARaiseFocusChangedEventOnUIAWindow());
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CJupiterControl::UIARaiseTextEditTextChangedEvent
//
//  Synopsis:
//      Raises a UIA TextEdit Text Changed Event
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterControl::UIARaiseTextEditTextChangedEvent(
    _In_ CAutomationPeer *pAP,
    _In_ UIAXcp::AutomationTextEditChangeType eAutomationProperty,
    _In_ CValue *cValue)
{
    if (m_pUIAWindow)
    {
        IFC_RETURN(m_pUIAWindow->UIARaiseTextEditTextChangedEvent(pAP, eAutomationProperty, cValue));
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterControl::UIARaiseNotificationEvent(
    _In_ CAutomationPeer* ap,
    UIAXcp::AutomationNotificationKind notificationKind,
    UIAXcp::AutomationNotificationProcessing notificationProcessing,
    _In_opt_ xstring_ptr displayString,
    _In_ xstring_ptr activityId)
{
    if (m_pUIAWindow)
    {
        IFC_RETURN(m_pUIAWindow->UIARaiseNotificationEvent(ap, notificationKind, notificationProcessing, displayString, activityId));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CJupiterControl::GetUIAWindow
//
//  Synopsis:
//      Get the top-level CUIAWindow instance
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CJupiterControl::GetUIAWindow(
    _In_ CDependencyObject *pElement,
    _In_ XHANDLE hWnd,
    _In_ bool onlyGet,
    _Outptr_ CUIAWindow** uiaWindowNoRef)
{
    CContentRoot* root = VisualTree::GetContentRootForElement(pElement);
    IFCEXPECT_RETURN(root);

    if (CXamlIslandRoot* island = root->GetXamlIslandRootNoRef())
    {
        *uiaWindowNoRef = island->GetUIAWindowNoRef();
        return S_OK;
    }

    // Create/Get UI Automation provider object for plugin window
    if (!m_pUIAWindow && !onlyGet)
    {
        IFC_RETURN(CreateUIAHostWindowForHwnd(static_cast<HWND>(hWnd)));
    }
    *uiaWindowNoRef = m_pUIAWindow.Get();

    return S_OK;
}

_Check_return_ HRESULT CJupiterControl::ConfigureJupiterWindow(_In_opt_ wuc::ICoreWindow* pCoreWindow)
{
    ctl::ComPtr<CJupiterWindow> spWindow;
    IFC_RETURN(CJupiterWindow::ConfigureJupiterWindow(pCoreWindow, this, &spWindow));
    IFC_RETURN(SetWindow(spWindow.Get()));
    spWindow.Detach();

    return S_OK;
}

_Check_return_ HRESULT CJupiterControl::RunCoreWindowMessageLoop()
{
    MSG msg;
    BOOL bRet;

    if (m_pWindow && WindowType::CoreWindow == m_pWindow->GetType())
    {
        IFC_RETURN(m_pWindow->RunCoreWindowMessageLoop());
        return S_OK;
    }

    while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0)
    {
        if (-1 == bRet)
        {
            IFC_RETURN(E_FAIL);
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return S_OK;
}

void CJupiterControl::ScreenToClient(_Inout_ POINT* pPixelPoint)
{
    XamlOneCoreTransforms::FailFastIfEnabled(); // Due to ScreenToClient call
    ::ScreenToClient(m_pWindow->GetWindowHandle(), pPixelPoint);
}

void CJupiterControl::ClientToScreen(_Inout_ POINT* pPixelPoint)
{
    XamlOneCoreTransforms::FailFastIfEnabled(); // Due to ClientToScreen call
    ::ClientToScreen(m_pWindow->GetWindowHandle(), pPixelPoint);
}

_Check_return_ HRESULT CJupiterControl::SetTicksEnabled(bool fTicksEnabled)
{
    if (m_pDispatcher)
    {
        IFC_RETURN((static_cast<CXcpDispatcher*>(m_pDispatcher))->SetTicksEnabled(fTicksEnabled));
    }

    return S_OK;
}

_Check_return_ HRESULT CJupiterControl::CreateResourceManager(_Outptr_ IPALResourceManager** ppResourceManager)
{
    HRESULT hr = S_OK;
    IPALResourceManager* pResourceManager = nullptr;

    TraceCreateResourceManagerStart();

    IFC(ResourceManager::Create(m_pBH->GetContextInterface(), &pResourceManager));

    *ppResourceManager = pResourceManager;
    pResourceManager = nullptr;

Cleanup:
    ReleaseInterface(pResourceManager);

    TraceCreateResourceManagerStop();
    return hr;
}

void CJupiterControl::OnReentrancyDetected()
{
    IGNOREHR(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, AG_E_REENTRANCY_DETECTED));
}

bool CJupiterControl::IsHdrOutput() const
{
    if (m_isHdrOutputOverride)
    {
        return true;
    }
    else
    {
        if (!m_isCachedHdrValid)
        {
            IFCFAILFAST(UpdateHdr());
        }
        return m_cachedIsHdr;
    }
}

LRESULT CJupiterControl::ForwardWindowedPopupMessageToJupiterWindow(
    _In_ HWND window,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_opt_ CContentRoot* contentRoot)
{
    bool handled = false;

    switch(message)
    {
        case WM_CONTEXTMENU:
            handled = HandleGenericMessage(message, wParam, lParam, contentRoot);
            break;

        case WM_POINTERUPDATE:
        case WM_POINTERDOWN:
        case WM_POINTERUP:
        case WM_POINTERENTER:
        case WM_POINTERLEAVE:
        case WM_POINTERWHEEL:
        case WM_POINTERHWHEEL:
        case WM_POINTERCAPTURECHANGED:
        case DM_POINTERHITTEST:
        case WM_POINTERROUTEDAWAY:
            handled = HandlePointerMessage(message, wParam, lParam, contentRoot);
            break;
    }

    if (!handled)
    {
        return ::DefWindowProc(window, message, wParam, lParam);
    }
    else
    {
        return 0;
    }
}

void CJupiterErrorServiceListener::NotifyErrorAdded(HRESULT hrToOriginate, _In_ IErrorService* pErrorService)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    IError* pError = nullptr;
    xstring_ptr strMessage;
    WCHAR* pszFormattedMessage = nullptr;
    XUINT32 cLineNumber = 0;
    XUINT32 cCharPosition = 0;

    IFCPTR(pErrorService);

    IFC_NOTRACE(pErrorService->GetLastReportedError(&pError));
    IFC_NOTRACE(pError->GetErrorMessage(&strMessage));

    if (pError->GetErrorType() == ParserError)
    {
        const WCHAR pszParseErrorMessage[] = L"%s [Line: %d Position: %d]";

        cLineNumber = pError->GetLineNumber();
        cCharPosition = pError->GetCharPosition();

        const XUINT32 cMessage = _scwprintf(pszParseErrorMessage, strMessage.GetBuffer(), cLineNumber, cCharPosition);

        pszFormattedMessage = new WCHAR[cMessage + 1];

        IFCEXPECT(swprintf_s(pszFormattedMessage, cMessage + 1, pszParseErrorMessage, strMessage.GetBuffer(), cLineNumber, cCharPosition) == cMessage);

        IFC(xstring_ptr::CloneBuffer(pszFormattedMessage, cMessage, &strMessage));
    }

    TRACE(TraceAlways, strMessage.GetBuffer());

    IFC_NOTRACE(DirectUI::ErrorHelper::OriginateError(hrToOriginate, strMessage));

Cleanup:
    delete[] pszFormattedMessage;
}

// Handle WM_UPDATEUISTATE, which is sent when we have new information about the last-used input device
bool CJupiterControl::HandleUpdateUIStateMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_opt_ CContentRoot* contentRoot)
{
    if (contentRoot)
    {
        if ((HIWORD(wParam) & UISF_HIDEFOCUS) != 0)
        {
            if (LOWORD(wParam) == UIS_CLEAR)
            {
                // Clearing the "hide focus" flag means we'll want to show the keyboard focus rectangle
                contentRoot->GetInputManager().SetLastInputDeviceType(DirectUI::InputDeviceType::Keyboard);
            }
            else
            {
                // Setting the "hide focus" flag means we'll want to hide the keyboard focus rectangle
                contentRoot->GetInputManager().SetLastInputDeviceType(DirectUI::InputDeviceType::None);
            }
        }
    }
    return false;
}

void CJupiterControl::SetMockUIAClientsListening(bool isEnabledMockUIAClientsListening)
{
    m_isEnabledMockUIAClientsListening = isEnabledMockUIAClientsListening;

    if (m_pUIAWindow)
    {
        m_pUIAWindow->SetMockUIAClientsListening(isEnabledMockUIAClientsListening);
    }
}

void CJupiterControl::SetHdrOutputOverride(bool isHdrOutputOverride)
{
    bool prevValue = IsHdrOutput();
    m_isHdrOutputOverride = isHdrOutputOverride;
    if (IsHdrOutput() != prevValue)
    {
        const auto coreServices = GetCoreServices();
        const auto& roots = coreServices->GetContentRootCoordinator()->GetContentRoots();
        for (const auto& root : roots)
        {
            if (const auto rootScale = RootScale::GetRootScaleForContentRoot(root))
            {
                CImageReloadManager& imageReloadManager = rootScale->GetImageReloadManager();
                IFCFAILFAST(imageReloadManager.ReloadImages(ResourceInvalidationReason::HdrChanged));
            }
        }

    }
}

HRESULT CJupiterControl::OnSettingChanged(HSTRING settingName)
{
    auto settingNameXstring = XSTRING_PTR_EPHEMERAL_FROM_HSTRING(settingName);

    if (CompareStringOrdinal(settingNameXstring.GetBuffer(), -1, LOCALE_CHANGE_MSG, -1, FALSE) == CSTR_EQUAL)
    {
        if (IXcpBrowserHost *pBH = GetBrowserHost())
        {
            if (CCoreServices *pcs = pBH->GetContextInterface())
            {
                IFC_RETURN(pcs->ConfigureNumberSubstitution());
            }
        }
    }
    else if (CompareStringOrdinal(settingNameXstring.GetBuffer(), -1, IMMERSIVE_COLORSET_CHANGE_MSG, -1, FALSE) == CSTR_EQUAL)
    {
        IFC_RETURN(OnThemeChanged());
        IFC_RETURN(NotifyImmersiveColorSetChanged());
    }

    return S_OK;
}