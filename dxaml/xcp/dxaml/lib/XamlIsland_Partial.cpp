// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlIsland.g.h"
#include "WindowsXamlManager_Partial.h"

#include "DCompTreeHost.h"
#include "DiagnosticsInterop.h"
#include "FrameworkApplication_Partial.h"
#include "IFocusController.h"
#include "WindowsXamlManager_Partial.h"
#include "XAMLIslandRoot_Partial.h"
#include "DCompTreeHost.h"
#include "Microsoft.UI.Input.h"
#include "WrlHelper.h"
#include "SystemBackdrop.g.h"
#include <Microsoft.UI.Content.Private.h>

using namespace DirectUI;

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_WindowsXamlManager();
    _Check_return_ IActivationFactory* CreateActivationFactory_XamlSourceFocusNavigationRequest();
}

XamlIsland::XamlIsland()
{
}

XamlIsland::~XamlIsland()
{
    VERIFYHR(Close());
}

_Check_return_ HRESULT XamlIsland::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    return __super::QueryInterfaceImpl(iid, ppObject);
}

_Check_return_ HRESULT XamlIsland::Initialize()
{
    IFC_RETURN(WeakReferenceSourceNoThreadId::Initialize());

    ctl::ComPtr<IActivationFactory> activationFactory(CreateActivationFactory_WindowsXamlManager());
    ctl::ComPtr<xaml_hosting::IWindowsXamlManagerStatics> coreFactory;
    IFC_RETURN(activationFactory.As(&coreFactory));
    ctl::ComPtr<xaml_hosting::IWindowsXamlManager> core;
    IFC_RETURN(coreFactory->InitializeForCurrentThread(&core));
    IFC_RETURN(core.As(&m_spXamlCore));

    IFC_RETURN(CheckThread());

    // Used by the DesktopWindowXamlSource to ensure it doesn't get released during garbage
    // collection in the case that it isn't exposed to a C# desktop app. Since the XamlIsland isn't used in
    // a desktop app, it's possible that this can be removed.
    SetReferenceTrackerPeg();

    // This will make sure that it doesn't get cleared off thread in WeakReferenceSourceNoThreadId::OnFinalReleaseOffThread()
    // as it has thread-local variables and needs to be disposed off by the same thread
    AddToReferenceTrackingList();

    ctl::ComPtr<FrameworkApplication> frameworkApplication = FrameworkApplication::GetCurrentNoRef();
    IFC_RETURN(frameworkApplication->CreateIslandRootWithContentBridge(ctl::iinspectable_cast(this), nullptr, m_spXamlIsland.ReleaseAndGetAddressOf()));

    // Create and store the composition island
    m_xamlIsland = m_spXamlIsland.Cast<XamlIslandRoot>();

    // Get and store the XamlIslandRoot
    m_pXamlIslandCore = static_cast<CXamlIslandRoot *>(m_xamlIsland->GetHandle());

    // Set the background transparent so that the SystemBackdrop is not occluded.
    m_pXamlIslandCore->SetHasTransparentBackground(true);

    m_pXamlIslandCore->SetContentRequested(true);

    if (auto interop = Diagnostics::GetDiagnosticsInterop(false))
    {
        interop->SignalRootMutation(ctl::iinspectable_cast(this), VisualMutationType::Add);
    }

    // Configure the XamlIsland2 will take focus when requested
    // Get a FocusControllerStatics and get current FocusController for ContentIsland
    ctl::ComPtr<ixp::IInputFocusController> inputFocusController;
    IFCFAILFAST(ActivationFactoryCache::GetActivationFactoryCache()->GetInputFocusControllerStatics()->GetForIsland(m_pXamlIslandCore->GetContentIsland(), &inputFocusController));
    IFCFAILFAST(inputFocusController.As(&m_inputFocusController2));

    XamlIslandRoot* xamlIsland = m_xamlIsland;
    IFCFAILFAST(m_inputFocusController2->add_NavigateFocusRequested(
        WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<ixp::InputFocusController*, ixp::FocusNavigationRequestEventArgs*>>(
            [xamlIsland](ixp::IInputFocusController* sender, ixp::IFocusNavigationRequestEventArgs* args) -> HRESULT
            {
                // Convert IXP FocusNavigationRequest to Xaml FocusNavigationRequest
                wrl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequest> xamlSourceFocusNavigationRequest;
                wrl::ComPtr<IActivationFactory> requestActivationFactory;
                requestActivationFactory.Attach(DirectUI::CreateActivationFactory_XamlSourceFocusNavigationRequest());
                wrl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationRequestFactory> requestFactory;
                IFC_RETURN(requestActivationFactory.As(&requestFactory));

                wrl::ComPtr<ixp::IFocusNavigationRequest> ixpRequest;
                IFC_RETURN(args->get_Request(&ixpRequest));

                // Convert IXP FocusNavigationReason to Xaml FocusNavigationReason
                ixp::FocusNavigationReason ixpReason;
                xaml_hosting::XamlSourceFocusNavigationReason xamlReason;
                IFC_RETURN(ixpRequest->get_Reason(&ixpReason));
                switch (ixpReason)
                {
                case ixp::FocusNavigationReason::FocusNavigationReason_Programmatic:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Programmatic;
                    break;
                case ixp::FocusNavigationReason::FocusNavigationReason_Restore:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Restore;
                    break;
                case ixp::FocusNavigationReason::FocusNavigationReason_Left:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Left;
                    break;
                case ixp::FocusNavigationReason::FocusNavigationReason_Down:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Down;
                    break;
                case ixp::FocusNavigationReason::FocusNavigationReason_First:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_First;
                    break;
                case ixp::FocusNavigationReason::FocusNavigationReason_Right:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Right;
                    break;
                case ixp::FocusNavigationReason::FocusNavigationReason_Up:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Up;
                    break;
                case ixp::FocusNavigationReason::FocusNavigationReason_Last:
                    xamlReason = xaml_hosting::XamlSourceFocusNavigationReason::XamlSourceFocusNavigationReason_Last;
                    break;
                }

                // Get HintRect and CorrelationID from IXP FocusNavigationRequest
                ctl::ComPtr<ABI::Windows::Foundation::IReference<ABI::Windows::Foundation::Rect>> hintRectRef;
                IFC_RETURN(ixpRequest->get_HintRect(&hintRectRef));
                ABI::Windows::Foundation::Rect hintRect;
                if (hintRectRef != nullptr)
                {
                    IFC_RETURN(hintRectRef->get_Value(&hintRect));
                }
                GUID correlationId;
                IFC_RETURN(ixpRequest->get_CorrelationId(&correlationId));

                IFC_RETURN(requestFactory->CreateInstanceWithHintRectAndCorrelationId(
                    xamlReason,
                    hintRect,
                    correlationId,
                    &xamlSourceFocusNavigationRequest));

                // Get Xaml FocusController and FocusManager
                ctl::ComPtr<IInspectable> spInsp;
                ctl::ComPtr<xaml_hosting::IFocusController> spFocusController;
                IFC_RETURN(xamlIsland->get_FocusController(&spInsp));
                IFC_RETURN(spInsp.As(&spFocusController));
                CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(xamlIsland->GetHandle());

                // Pass IXP focus navigation on to Xaml focus navigation logic to set Xaml logical focus
                ctl::ComPtr<xaml_hosting::IXamlSourceFocusNavigationResult> pResult = nullptr;
                IFC_RETURN(spFocusController->NavigateFocus(xamlSourceFocusNavigationRequest.Get(), focusManager->GetFocusObserverNoRef(), pResult.GetAddressOf()));

                // Convert Xaml focus navigation result to IXP focus navigation result
                boolean wasFocusMoved;
                IFC_RETURN(pResult->get_WasFocusMoved(&wasFocusMoved));
                ixp::FocusNavigationResult ixpResult = wasFocusMoved ? ixp::FocusNavigationResult::FocusNavigationResult_Moved : ixp::FocusNavigationResult::FocusNavigationResult_NotMoved;
                IFC_RETURN(args->put_Result(ixpResult));

                return S_OK;
            }).Get(), &m_focusNavigationRequestedToken));

    // Note: This is needed for ScrollViewer initialization.
    // CUIElement::CanDMContainerInitialize looks for a valid input window, and CanDMContainerInitialize
    // is needed before CInputServices::InitializeDirectManipulationContainers can activate DM for a
    // ScrollViewer.
    wrl::ComPtr<ixp::IContentIsland> contentIsland = m_pXamlIslandCore->GetContentIsland();
    wrl::ComPtr<ixp::IContentIsland2> contentIsland2;
    IFCFAILFAST(contentIsland.As(&contentIsland2));

    // Right now, the InputSite only has a valid HWND once the island has connected. This means that
    // we wait to set it on the XamlIslandRoot until the connected event. Once velocity key
    // 45720437 has been flipped on, we can remove this and set the input site on initialization.
    // http://task.ms/48681310: Once we flip this InputSite velocity key in Xaml, we can set the InputSite as
    // soon as the XamlIsland is initialized and won't have to wait for connection.

    ctl::WeakRefPtr wrThis;
    IFC_RETURN(ctl::AsWeak(this, &wrThis));

    IFCFAILFAST(contentIsland2->add_Connected(
        WRLHelper::MakeAgileCallback<wf::IEventHandler<ixp::ContentIsland *>>(
            [wrThis](IInspectable *, ixp::IContentIsland * /* content */) mutable -> HRESULT
            {
                ctl::ComPtr<XamlIsland> spThis;
                IFC_RETURN(wrThis.As(&spThis));

                if (spThis)
                {
                    IFC_RETURN(spThis->SetInputSite());
                }

                return S_OK;
            })
            .Get(),
        &m_islandConnectedToken));

    IFCFAILFAST(contentIsland2->add_Disconnected(
        WRLHelper::MakeAgileCallback<wf::IEventHandler<ixp::ContentIsland *>>(
            [wrThis](IInspectable *, ixp::IContentIsland * /* content */) mutable -> HRESULT
            {
                ctl::ComPtr<XamlIsland> spThis;
                IFC_RETURN(wrThis.As(&spThis));

                if (spThis && spThis->m_pXamlIslandCore)
                {
                    spThis->m_pXamlIslandCore->SetIslandInputSite(nullptr);
                }

                return S_OK;
            })
            .Get(),
        &m_islandDisconnectedToken));

    return S_OK;
}

_Check_return_ HRESULT XamlIsland::get_ContentImpl(_Outptr_ xaml::IUIElement** ppValue)
{
    *ppValue = nullptr;
    IFC_RETURN(m_spXamlIsland->get_Content(ppValue));

    return S_OK;
}

_Check_return_ HRESULT XamlIsland::put_ContentImpl(_In_opt_ xaml::IUIElement* pValue)
{
    // Determine and set the content's layout direction.
    if (pValue)
    {
        ixp::ContentLayoutDirection layoutDirection;
        m_pXamlIslandCore->GetContentIsland()->get_LayoutDirection(&layoutDirection);

        // https://task.ms/43100993: In a Xaml island, we have no access to a bridge or HWND to
        // enforce the LTR layout that Xaml needs to perform layout correctly.
        // Until Xaml correctly handles the RTL coordinate space, this will lead to issues with input and
        // output, as the underlying HWND will be in RTL, so we FAILFAST here. Once RTL is properly
        // handled, we should be able to respond correctly to the ContentIsland's LayoutDirection.

        if (layoutDirection == ixp::ContentLayoutDirection_RightToLeft)
        {
            ::RoOriginateError(
                E_NOT_SUPPORTED,
                wrl_wrappers::HStringReference(
                L"RTL layout on the ContentIsland is not supported for XamlIsland. Use Xaml FlowDirection to set RTL on the content instead."
                ).Get());
            IFC_RETURN(E_NOT_SUPPORTED);        
        }
    }

    IFC_RETURN(m_spXamlIsland->put_Content(pValue));

    return S_OK;
}

_Check_return_ HRESULT XamlIsland::get_ContentIslandImpl(_Outptr_ ixp::IContentIsland **ppValue)
{
    *ppValue = m_pXamlIslandCore->GetContentIsland();

    return S_OK;
}

// Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop implementation
IFACEMETHODIMP XamlIsland::get_SystemBackdrop(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush **systemBackdropBrush)
{
    ARG_VALIDRETURNPOINTER(systemBackdropBrush);
    *systemBackdropBrush = nullptr;

    IFC_RETURN(CheckThread());

    ctl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop> compositionSupportsSystemBackdrop;
    ctl::ComPtr<ixp::IContentIsland> contentIsland = m_pXamlIslandCore->GetContentIsland();

    IFC_RETURN(contentIsland.As(&compositionSupportsSystemBackdrop));

    IFC_RETURN(compositionSupportsSystemBackdrop->get_SystemBackdrop(systemBackdropBrush));

    return S_OK;
}

IFACEMETHODIMP XamlIsland::put_SystemBackdrop(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush *systemBackdropBrush)
{
    IFC_RETURN(CheckThread());

    ctl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop> compositionSupportsSystemBackdrop;
    ctl::ComPtr<ixp::IContentIsland> contentIsland = m_pXamlIslandCore->GetContentIsland();

    IFC_RETURN(contentIsland.As(&compositionSupportsSystemBackdrop));

    IFC_RETURN(compositionSupportsSystemBackdrop->put_SystemBackdrop(systemBackdropBrush));

    return S_OK;
}

_Check_return_ HRESULT XamlIsland::get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop **iSystemBackdrop)
{
    return m_systemBackdrop.CopyTo(iSystemBackdrop);
}

_Check_return_ HRESULT XamlIsland::put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop *iSystemBackdrop)
{
    // If nothing changed then do nothing. Otherwise we'd call OnTargetDisconnected and OnTargetConnected
    // back-to-back on the same SystemBackdrop.
    if (m_systemBackdrop.Get() != iSystemBackdrop)
    {
        if (m_systemBackdrop.Get() != nullptr)
        {
            ctl::ComPtr<DirectUI::SystemBackdrop> systemBackdrop;
            IFC_RETURN(m_systemBackdrop.As(&systemBackdrop));
            IFC_RETURN(systemBackdrop->InvokeOnTargetDisconnected(this));
        }

        m_systemBackdrop = iSystemBackdrop;
        if (iSystemBackdrop != nullptr)
        {
            ctl::ComPtr<xaml::IUIElement> content;
            IFC_RETURN(get_ContentImpl(&content));
            ctl::ComPtr<xaml::IXamlRoot> xamlRoot;
            IFC_RETURN(content->get_XamlRoot(&xamlRoot));

            ctl::ComPtr<DirectUI::SystemBackdrop> systemBackdrop;
            IFC_RETURN(m_systemBackdrop.As(&systemBackdrop));
            IFC_RETURN(systemBackdrop->InvokeOnTargetConnected(this, xamlRoot.Get()));
        }
    }

    return S_OK;
}

_Check_return_ xaml_hosting::IXamlIslandRoot* XamlIsland::GetXamlIslandRootNoRef()
{
    return m_spXamlIsland.Get();
}

_Check_return_ HRESULT XamlIsland::SetInputSite()
{
    wrl::ComPtr<ixp::IContentIslandPartner> contentIslandPartner;
    wrl::ComPtr<ixp::IContentIsland> contentIsland = m_pXamlIslandCore->GetContentIsland();
    IFCFAILFAST(contentIsland.As(&contentIslandPartner));

    wrl::ComPtr<ixp::IIslandInputSitePartner> islandInputSitePartner;
    IFCFAILFAST(contentIslandPartner->get_IslandInputSite(&islandInputSitePartner));

    m_pXamlIslandCore->SetIslandInputSite(islandInputSitePartner.Get());

    return S_OK;
}

IFACEMETHODIMP XamlIsland::Close()
{
    if (m_bClosed)
    {
        return S_OK;
    }

    IFC_RETURN(CheckThread());

    m_bClosed = true;

    if (m_systemBackdrop.Get() != nullptr)
    {
        ctl::ComPtr<DirectUI::SystemBackdrop> systemBackdrop;
        IFC_RETURN(m_systemBackdrop.As(&systemBackdrop));
        IFC_RETURN(systemBackdrop->InvokeOnTargetDisconnected(this));
        systemBackdrop = nullptr;
        m_systemBackdrop = nullptr;
    }

    if (m_inputFocusController2 != nullptr)
    {
        m_inputFocusController2->remove_NavigateFocusRequested(m_focusNavigationRequestedToken);
        m_inputFocusController2 = nullptr;
        m_focusNavigationRequestedToken.value = 0;
    }

    // http://task.ms/48681310: Once velocity key 45720437 has been flipped on, we will no longer
    // have to set the InputSite on connection and this can be removed.
    if (m_islandConnectedToken.value != 0)
    {
        wrl::ComPtr<ixp::IContentIsland> contentIsland = m_pXamlIslandCore->GetContentIsland();
        wrl::ComPtr<ixp::IContentIsland2> contentIsland2;
        IFCFAILFAST(contentIsland.As(&contentIsland2));

        IFCFAILFAST(contentIsland2->remove_Connected(m_islandConnectedToken));
        m_islandConnectedToken.value = 0;
    }

    if (m_islandDisconnectedToken.value != 0)
    {
        wrl::ComPtr<ixp::IContentIsland> contentIsland = m_pXamlIslandCore->GetContentIsland();
        wrl::ComPtr<ixp::IContentIsland2> contentIsland2;
        IFCFAILFAST(contentIsland.As(&contentIsland2));

        IFCFAILFAST(contentIsland2->remove_Disconnected(m_islandDisconnectedToken));
        m_islandDisconnectedToken.value = 0;
    }

    if (m_spXamlIsland.Get() != nullptr)
    {
        auto frameworkApplication = FrameworkApplication::GetCurrentNoRef();
        IFCFAILFAST(frameworkApplication->RemoveIsland(m_spXamlIsland.Get()));

        // Turn off any frame counters (if they are on) for the same reason we remove the content.  Also inform
        // the core that we have done this in case it needs to re-evaluate whether to display on a future frame
        // (and a different island).
        if (m_pXamlIslandCore->GetDCompTreeHost())
        {
            IFC_RETURN(m_pXamlIslandCore->GetDCompTreeHost()->UpdateDebugSettings(false /* isFrameRateCounterEnabled */));
        }
        IFC_RETURN(m_pXamlIslandCore->GetContext()->OnDebugSettingsChanged());
    }

    // Signal to the interop tool of the closure after the XamlIslandRoot has been removed. This way
    // the RuntimeObjectCache stays connected.
    if (auto interop = Diagnostics::GetDiagnosticsInterop(false))
    {
        interop->SignalRootMutation(ctl::iinspectable_cast(this), VisualMutationType::Remove);
    }

    if (m_spXamlIsland.Get() != nullptr)
    {
        m_pXamlIslandCore->Dispose();
        m_spXamlIsland = nullptr;
    }

    if (m_spXamlCore.Get() != nullptr)
    {
        ctl::ComPtr<XamlIsland> spThis(this); // Avoid deleting this
        ctl::ComPtr<wf::IClosable> spClosable;

        IFC_RETURN(m_spXamlCore.As(&spClosable));
        IFC_RETURN(spClosable->Close());
        spClosable = nullptr;
        m_spXamlCore = nullptr;
    }

    return S_OK;
}
