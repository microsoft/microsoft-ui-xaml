// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Window.g.h"
#include <DependencyLocator.h>
#include <RuntimeEnabledFeatures.h>
#include "ScrollViewer.g.h"
#include "FrameworkApplication.g.h"
#include "BackgroundTaskFrameworkContext.h"
#include "windows.ui.viewmanagement.h"
#include "CoreWindowWrapper.h"
#include <PixelFormat.h>
#include <windows.graphics.directx.h>
#include <windows.ui.input.h>
#include "IApplicationBarService.h"
#include <Grid.g.h>
#include <FeatureFlags.h>
#include <RootScale.h>
#include <FeatureFlags.h>
#include <windows.ui.core.corewindow-defs.h>
#include "WindowSizeChangedEventArgs.g.h"
#include "XamlRoot.g.h"
#include "host.h"
#include <WindowsGraphicsDeviceManager.h>
#include "DCompTreeHost.h"
#include "ElementSoundPlayerService_Partial.h"

#include "UWPWindowImpl.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace RuntimeFeatureBehavior;

// Work around disruptive max/min macros
#undef max
#undef min

// ----------------------------------------------------------------------
//                              UWPWindowImpl
// ----------------------------------------------------------------------

UWPWindowImpl::UWPWindowImpl(Window* parentWindow)
    : m_contentManager(parentWindow, true /*isUwpWindowContent*/)
    , m_pCoreWindowWrapper(NULL)
    , m_hasSizeOverrides(FALSE)
    , m_widthOverride(0)
    , m_heightOverride(0)
    , m_shrinkApplicationViewVisibleBounds(false)
    , m_coreWindowClosed(false)
    , m_parentWindow(parentWindow)
{
    m_tokAppViewVisibleBoundsChanged.value = 0;
    m_tokCoreWindowSizeChanged.value = 0;
    m_tokCoreWindowVisibleBoundsChanged.value = 0;
    m_tokCoreWindowClosed.value = 0;
}

UWPWindowImpl::~UWPWindowImpl()
{
    if (m_pCoreWindowWrapper)
    {
        if (m_tokCoreWindowSizeChanged.value)
        {
            if (auto const coreWindow = m_pCoreWindowWrapper->GetCoreWindow())
            {
                IGNOREHR(coreWindow->remove_SizeChanged(m_tokCoreWindowSizeChanged));
            }
        }
        if (m_tokAppViewVisibleBoundsChanged.value)
        {
            ctl::ComPtr<wuv::IApplicationView2> spAppView2;
            IGNOREHR(LayoutBoundsChangedHelper::GetApplicationView2(&spAppView2));
            if (spAppView2)
            {
                IGNOREHR(spAppView2->remove_VisibleBoundsChanged(m_tokAppViewVisibleBoundsChanged));
            }
        }
        if (m_tokCoreWindowClosed.value)
        {
            if (m_pCoreWindowWrapper->GetCoreWindow())
            {
                IGNOREHR(m_pCoreWindowWrapper->GetCoreWindow()->remove_Closed(m_tokCoreWindowClosed));
            }
        }

        delete m_pCoreWindowWrapper;
    }
}

// ----------------------------------------------------------------------
//                               IWindow
// ----------------------------------------------------------------------

IFACEMETHODIMP UWPWindowImpl::add_Activated(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowActivatedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    HRESULT hr = S_OK;

    *pToken = {0};

    // TODO: for now, no-op if we don't have a CoreWindow
    if (!m_pCoreWindowWrapper)
    {
        goto Cleanup;
    }

    IFC(m_pCoreWindowWrapper->AddActivatedHandler(pHandler, pToken));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP UWPWindowImpl::remove_Activated(_In_ EventRegistrationToken token)
{
    HRESULT hr = S_OK;

    // TODO: for now, no-op if we don't have a CoreWindow
    if (!m_pCoreWindowWrapper)
    {
        goto Cleanup;
    }

    IFC(m_pCoreWindowWrapper->RemoveActivatedHandler(token));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP UWPWindowImpl::add_Closed(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    HRESULT hr = S_OK;

    IFCCHECK(m_pCoreWindowWrapper);

    IFC(m_pCoreWindowWrapper->AddClosedHandler(pHandler, pToken));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP UWPWindowImpl::remove_Closed(_In_ EventRegistrationToken token)
{
    HRESULT hr = S_OK;

    IFCCHECK(m_pCoreWindowWrapper);

    IFC(m_pCoreWindowWrapper->RemoveClosedHandler(token));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP UWPWindowImpl::add_SizeChanged(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowSizeChangedEventArgs*> * pHandler, _Out_ EventRegistrationToken* pToken)
{
    HRESULT hr = S_OK;

    if (m_pCoreWindowWrapper)
    {
        IFC(m_pCoreWindowWrapper->AddSizeChangedHandler(pHandler, pToken));
    }
    else
    {
        IFC(m_SizeChangedEventSource.Add(pHandler, pToken));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP UWPWindowImpl::remove_SizeChanged(_In_ EventRegistrationToken token)
{
    HRESULT hr = S_OK;

    if (m_pCoreWindowWrapper)
    {
        IFC(m_pCoreWindowWrapper->RemoveSizeChangedHandler(token));
    }
    else
    {
        IFC(m_SizeChangedEventSource.Remove(token));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP UWPWindowImpl::add_VisibilityChanged(_In_ wf::ITypedEventHandler<IInspectable*, xaml::WindowVisibilityChangedEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken)
{
    HRESULT hr = S_OK;

    IFCCHECK(m_pCoreWindowWrapper);

    IFC(m_pCoreWindowWrapper->AddVisibilityChangedHandler(pHandler, pToken));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP UWPWindowImpl::remove_VisibilityChanged(_In_ EventRegistrationToken token)
{
    HRESULT hr = S_OK;

    IFCCHECK(m_pCoreWindowWrapper);

    IFC(m_pCoreWindowWrapper->RemoveVisibilityChangedHandler(token));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UWPWindowImpl::get_BoundsImpl(_Out_ wf::Rect* pValue)
{
    const auto contentRootCoordinator = DXamlCore::GetCurrent()->GetHandle()->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);

    if (m_hasSizeOverrides)
    {
        pValue->X = 0;
        pValue->Y = 0;
        pValue->Width = static_cast<float>(m_widthOverride);
        pValue->Height = static_cast<float>(m_heightOverride);
    }
    else if (m_pCoreWindowWrapper)
    {
        // Get the core window bounds.
        IFC_RETURN(m_pCoreWindowWrapper->GetCoreWindow()->get_Bounds(pValue));
    }
    else
    {
        // No window, return empty.
        *pValue = {};
    }

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::get_VisibleImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;


    IFCCHECK(m_pCoreWindowWrapper);

    IFC(m_pCoreWindowWrapper->GetCoreWindow()->get_Visible(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UWPWindowImpl::get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** ppValue)
{
    HRESULT hr = S_OK;

    if (nullptr == ppValue) { IFC(E_INVALIDARG); }
    *ppValue = nullptr;

    if (DXamlCore::GetCurrent()->GetHandle()->GetInitializationType() == InitializationType::IslandsOnly)
    {
        // In IslandsOnly mode, we have fake content on the Window that we don't expose to the app.
        // Just return nullptr.
    }
    else
    {
        *ppValue = m_contentManager.GetContent();
        AddRefInterface(*ppValue);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UWPWindowImpl::put_ContentImpl(_In_opt_ xaml::IUIElement* pContent)
{
    HRESULT hr = S_OK;


    IFC(m_contentManager.SetContent(pContent));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UWPWindowImpl::EnsureInitializedForIslands()
{
    if (m_contentManager.GetContent() == nullptr)
    {
        IFCFAILFAST(DXamlCore::GetCurrent()->GetHandle()->GetInitializationType() != InitializationType::IslandsOnly ? E_UNEXPECTED : S_OK);

        // We need to support islands, but the window doesn't have any content yet.
        // We currently handle this by setting a dummy grid in the tree to trigger normal
        // XAML startup.  Once we do this, we do not support setting Window.Content later.
        // http://osgvsowi/17002792 : Support a startup path without a dummy grid.
        ctl::ComPtr<Grid> dummy;
        ctl::make(&dummy);
        IFC_RETURN(m_contentManager.SetContent(dummy.Get() /*content*/));
    }

    return S_OK;
}


_Check_return_ HRESULT UWPWindowImpl::get_CoreWindowImpl(_Outptr_result_maybenull_ wuc::ICoreWindow** ppValue)
{
    HRESULT hr = S_OK;

    if (nullptr == ppValue) { IFC(E_INVALIDARG); }
    *ppValue = nullptr;

    if (m_pCoreWindowWrapper)
    {
        *ppValue = m_pCoreWindowWrapper->GetCoreWindow();
        AddRefInterface(*ppValue);
    }

Cleanup:
    RRETURN(hr);
}

//
// IMPORTANT: Don't use CoreDispatcher in XAML framework code.
//
// CoreDispatcher is exposed through our API for apps to use,
// but we need to avoid taking any internal dependencies on it,
// because it's not available in all environments we support.
//
// Instead, use DirectUI::IDispatcher. This is available through:
//     DXamlCore::GetXamlDispatcher()
//     DependencyObject::GetXamlDispatcher()
//
_Check_return_ HRESULT UWPWindowImpl::get_DispatcherImpl(_Outptr_result_maybenull_ wuc::ICoreDispatcher** ppValue)
{
    if (nullptr == ppValue) { IFC_RETURN(E_INVALIDARG); }
    *ppValue = nullptr;

    if (m_pCoreWindowWrapper)
    {
        *ppValue = m_pCoreWindowWrapper->GetCoreDispatcher();
        AddRefInterface(*ppValue);
    }

    RRETURN(S_OK);
}

_Check_return_ HRESULT UWPWindowImpl::get_CompositorImpl(_Outptr_result_maybenull_ WUComp::ICompositor** compositor)
{
    IFCPTR_RETURN(compositor);
    *compositor = nullptr;

    *compositor = DXamlCore::GetCurrent()->GetHandle()->GetCompositor();
    AddRefInterface(*compositor);

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::get_TitleImpl(_Out_ HSTRING* pValue)
{
    ctl::ComPtr<wuv::IApplicationViewStatics2> applicationViewStatics;
    IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
                                         RuntimeClass_Windows_UI_ViewManagement_ApplicationView)
                                         .Get(),
                                         &applicationViewStatics));

    ctl::ComPtr<wuv::IApplicationView> applicationView;
    IFC_RETURN(applicationViewStatics->GetForCurrentView(&applicationView));

    IFC_RETURN(applicationView->get_Title(pValue));

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::put_TitleImpl(_In_opt_ HSTRING value)
{
    ctl::ComPtr<wuv::IApplicationViewStatics2> applicationViewStatics;
    IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
                                         RuntimeClass_Windows_UI_ViewManagement_ApplicationView)
                                         .Get(),
                                         &applicationViewStatics));

    ctl::ComPtr<wuv::IApplicationView> applicationView;
    IFC_RETURN(applicationViewStatics->GetForCurrentView(&applicationView));

    IFC_RETURN(applicationView->put_Title(value));

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::ActivateImpl()
{
    HRESULT hr = S_OK;

    IFC(DXamlCore::GetCurrent()->ActivateWindow());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UWPWindowImpl::CloseImpl()
{
    HRESULT hr = S_OK;

    IFCCHECK(m_pCoreWindowWrapper);

    IFC(m_pCoreWindowWrapper->GetCoreWindow()->Close());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UWPWindowImpl::GetLayoutBounds(_Out_  wf::Rect* pLayoutBounds)
{
    if (m_pCoreWindowWrapper && !m_hasSizeOverrides)
    {
        if (m_coreWindowClosed)
        {
            *pLayoutBounds = m_layoutBounds;
        }
        else
        {
            wuv::ApplicationViewBoundsMode boundsMode = wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseCoreWindow;
            ctl::ComPtr<wuv::IApplicationView2> spAppView2;
            // We expect this call to fail in island scenarios, so don't IFC_RETURN it.
            IGNOREHR(LayoutBoundsChangedHelper::GetApplicationView2(&spAppView2));
            if(spAppView2)
            {
                IFC_RETURN(spAppView2->get_DesiredBoundsMode(&boundsMode));
            }

            if (boundsMode == wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseVisible)
            {
                IFC_RETURN(spAppView2->get_VisibleBounds(pLayoutBounds));
            }
            else if (boundsMode == wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseCoreWindow)
            {
                IFC_RETURN(m_pCoreWindowWrapper->GetCoreWindow()->get_Bounds(pLayoutBounds));
            }
        }
    }
    else if (m_hasSizeOverrides)
    {
        *pLayoutBounds = m_layoutBoundsOverrides;
    }
    else
    {
        // Return the core window bounds if the core window wrapper isn't available.
        IFC_RETURN(get_BoundsImpl(pLayoutBounds));
    }

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::GetLayoutBounds(_Out_  XRECTF* pLayoutBounds)
{
    HRESULT hr = S_OK;
    wf::Rect layoutBoundsRect = {};

    IFCPTR(pLayoutBounds);

    IFC(GetLayoutBounds(&layoutBoundsRect));

    pLayoutBounds->X = layoutBoundsRect.X;
    pLayoutBounds->Y = layoutBoundsRect.Y;
    pLayoutBounds->Width = layoutBoundsRect.Width;
    pLayoutBounds->Height = layoutBoundsRect.Height;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// HasBounds() checks whether it's currently valid to call get_Bounds().
//
// A XAML Window object can represent several OS-level window constructs
// (i.e. CoreWindow, HWND). It can also represent a state in which there is no
// backing window at all - e.g. when we've just started up and don't have
// a window yet, or when we're shutting down and have de-associated from a
// window.
//
// For these reasons, it's not always possible to obtain the size (bounds)
// of a Window object. If you want to handle these cases, you can call HasBounds()
// to check whether it's safe to call get_Bounds().
//
//-----------------------------------------------------------------------------
bool UWPWindowImpl::HasBounds()
{
    return m_pCoreWindowWrapper != nullptr;
}

_Check_return_ HRESULT UWPWindowImpl::SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow)
{
    HRESULT hr = S_OK;

    // SetCoreWindow should be called at most once - we should not already have created a CoreWindowWrapper.
    IFCEXPECT_ASSERT(!m_pCoreWindowWrapper);

    IFC(CoreWindowWrapper::Create(pCoreWindow, &m_pCoreWindowWrapper));

    // Initialize the layout bounds as the empty rect.
    m_layoutBounds = {};

    {
        // While running in win32 this call will fail to get an AppView. Thats okay, because we
        // shouldn't need to worry about the app view bounds on win32.
        ctl::ComPtr<wuv::IApplicationView2> spAppView2;
        IGNOREHR(LayoutBoundsChangedHelper::GetApplicationView2(&spAppView2));

        if (spAppView2)
        {
            IFC(spAppView2->add_VisibleBoundsChanged(
                Microsoft::WRL::Callback<wf::ITypedEventHandler<wuv::ApplicationView*, IInspectable*>>(
                    this,
                    &UWPWindowImpl::OnApplicationViewVisibleBoundsChanged).Get(),
                &m_tokAppViewVisibleBoundsChanged));
        }
    }

    IFC(m_pCoreWindowWrapper->GetCoreWindow()->add_SizeChanged(
        Microsoft::WRL::Callback<wf::ITypedEventHandler<wuc::CoreWindow*, wuc::WindowSizeChangedEventArgs*>>(
        this,
        &UWPWindowImpl::OnCoreWindowSizeChanged).Get(),
        &m_tokCoreWindowSizeChanged));

    IFC(m_pCoreWindowWrapper->GetCoreWindow()->add_Closed(
        Microsoft::WRL::Callback<wuc::IWindowClosedEventHandler>(
        this,
        &UWPWindowImpl::OnCoreWindowClosed).Get(),
        &m_tokCoreWindowClosed));

Cleanup:
    RRETURN(hr);
}

// Gets the client area rect of the window in window coordinates.  This method uses
// ApplicationView.VisibleBounds to account for OS chrome occlusion and RootScrollViewer
// height to detect an open SIP when ignoreIHM is False.
_Check_return_ HRESULT UWPWindowImpl::GetVisibleBounds(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _Out_ wf::Rect* pValue)
{
    // m_isVisibleBoundsOverrides flag is true for only test hook and not available on the regular apps.
    if (m_visibleBoundsOverrides.Width > 0 && m_visibleBoundsOverrides.Height > 0)
    {
        *pValue = m_visibleBoundsOverrides;
    }
    else
    {
        if (m_useApplicationView && nullptr == m_spApplicationView2)
        {
            // Try to prep our ApplicationView to see if we can and should be using ApplicationView.VisibleBounds
            ctl::ComPtr<wuv::IApplicationViewStatics2> spApplicationViewStatics;
            IFC_RETURN(ctl::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(),
                &spApplicationViewStatics));

            ctl::ComPtr<wuv::IApplicationView> spApplicationView;

            // Calling GetForCurrentView is expensive and sometimes fails
            if (FAILED(spApplicationViewStatics->GetForCurrentView(&spApplicationView)))
            {
                // Some hosting scenarios such as login UI don't implement ApplicationView
                // and can return a failure HRESULT for GetForCurrentView instead of only returning a null
                // output.
                spApplicationView = nullptr;
            }

            if (nullptr != spApplicationView)
            {
                IFC_RETURN(spApplicationView.As(&m_spApplicationView2));
            }

            if (nullptr == m_spApplicationView2)
            {
                // Since we didn't get a usable ApplicationView2 by this point, stop wasting time querying for
                // one because that shouldn't change for the lifetime of the Window
                m_useApplicationView = FALSE;
            }
        }

        wf::Rect visibleBounds = { 0 };

        // If available use the ApplicationViewVisibleBounds. Otherwise fall back to using CoreWindow.Bounds
        if (!m_hasSizeOverrides && nullptr != m_spApplicationView2)
        {
            // ApplicationView.VisibleBounds
            IFC_RETURN(m_spApplicationView2->get_VisibleBounds(&visibleBounds));

            if (!inDesktopCoordinates)
            {
                // Get CoreWindow.Bounds
                wf::Rect windowBounds = { 0 };
                IFC_RETURN(m_parentWindow->get_Bounds(&windowBounds));

                // CoreWindow.Bounds and ApplicationView.VisibleBounds are in Desktop coordinates so
                // they need to be translated to top left of the current window.  In the future if
                // VisibleBounds is updated to be in window coordinates the following logic should
                // be removed or it will return negative values for windows that aren't hosted at
                // the top-left hand side of the screen.
                visibleBounds.X -= windowBounds.X;
                visibleBounds.Y -= windowBounds.Y;
            }
        }
        else
        {
            // If there's no cached ApplicationView then it's impossible to get any chrome
            // dimension information. Assume the visible bounds is the entire window bounds.

            // Get CoreWindow.Bounds
            wf::Rect windowBounds = { 0 };
            IFC_RETURN(m_parentWindow->get_Bounds(&windowBounds));

            visibleBounds = { inDesktopCoordinates ? windowBounds.X : 0, inDesktopCoordinates ? windowBounds.Y : 0, windowBounds.Width, windowBounds.Height };
        }

        if (!ignoreIHM)
        {
            // determine if the IHM is open and factor in its consumed size.
            ctl::ComPtr<xaml_controls::IScrollViewer> spRootScrollViewer;
            IFC_RETURN(GetRootScrollViewer(&spRootScrollViewer));
            if (nullptr != spRootScrollViewer)
            {
                DOUBLE height;
                IFC_RETURN(static_cast<DirectUI::ScrollViewer*>(spRootScrollViewer.Get())->get_Height(&height));

                visibleBounds.Height = std::min(visibleBounds.Height, static_cast<float>(height)-visibleBounds.Y);
            }
        }

        *pValue = visibleBounds;
    }

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::GetRootScrollViewer(_Outptr_result_maybenull_ IScrollViewer** ppRootScrollViewer)
{
    *ppRootScrollViewer = m_contentManager.GetRootScrollViewer();
    AddRefInterface(*ppRootScrollViewer);

    RRETURN(S_OK);
}

_Check_return_ HRESULT UWPWindowImpl::OnWindowSizeChanged()
{
    auto pXamlCore = DXamlCore::GetCurrent();
    if (!pXamlCore)
    {
        // XamlCore is gone, do nothing.
        // This can happen during testing while the instance of Xaml Window is still alive
        // But the core has been de initialized.
        return S_OK;
    }

    // One of the implicit guarantees of the OnWindowSizeChanged() code path
    // is that functions we call from here are able to call Window::get_Bounds()
    // successfully.
    //
    // OnWindowSizeChanged() can be called at a time when we don't have
    // a valid window size. For example, during startup we'll call OnWindowSizeChanged()
    // as part of setting the initial UI scale. At this time we haven't yet associated
    // with a window, so we don't have a size available.
    //
    // Call HasBounds() and skip the OnWindowSizeChanged() code path if we're
    // in a state where no size is available.
    if (!HasBounds())
    {
        return S_OK;
    }

    wf::Rect bounds;
    IFC_RETURN(m_parentWindow->get_Bounds(&bounds));

    if (!m_pCoreWindowWrapper)
    {
        ctl::ComPtr<DirectUI::WindowSizeChangedEventArgs> sizeChangedEventArgs;

        IFC_RETURN(ctl::make(&sizeChangedEventArgs));

        IFC_RETURN(sizeChangedEventArgs->put_Size(wf::Size{ bounds.Width, bounds.Height }));
        IFC_RETURN(m_SizeChangedEventSource.Raise(m_parentWindow, sizeChangedEventArgs.Get()));
    }

    IFC_RETURN(m_contentManager.OnWindowSizeChanged());

    wf::Rect layoutBounds;
    GetLayoutBounds(&layoutBounds);

    // Save the current layout bounds.
    m_layoutBounds = layoutBounds;

    pXamlCore->GetHandle()->OnWindowSizeChanged(
        static_cast<XUINT32>(bounds.Width),
        static_cast<XUINT32>(bounds.Height),
        static_cast<XUINT32>(layoutBounds.Width),
        static_cast<XUINT32>(layoutBounds.Height));

    // On multi-mon setups, when the primary monitor scale factor is different than the secondary monitor scale factor,
    //   and an app with a Top/Bottom AppBar is opened on the secondary monitor, VisibleBoundsChanged reports bounds relative to the
    //   primary monitor, instead of the secondary monitor. VBC *should* fire twice, once for the primary and again when the window
    //   moves to the secondary, but since it doesn't because of 7330274, we also update bounds here, in SizeChanged.
    IFC_RETURN(UpdateApplicationBarServiceBounds());

    // Get the XamlRoot that is associated with this Window. This is safe because Window is essentially
    // a large wrapper around CoreWindow, so we know that whenever this Window needs a content root, it's the one
    // associated with a CoreWindow
    const auto contentRootCoordinator = pXamlCore->GetHandle()->GetContentRootCoordinator();
    const auto contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    if (contentRoot) // We may have already torn down the XamlRoot for the CoreWindow
    {
        IFC_RETURN(contentRoot->GetAKExport().ExitAccessKeyMode());
    }

    return S_OK;
}

DXamlCore* UWPWindowImpl::GetDXamlCore()
{
    return m_pDXamlCoreNoRef;
}

void UWPWindowImpl::SetDXamlCore(_In_opt_ DXamlCore* pDXamlCore)
{
    m_pDXamlCoreNoRef = pDXamlCore;
}

_Check_return_ HRESULT UWPWindowImpl::OnCoreWindowVisibleBoundsChanged(_In_ wuc::ICoreWindow* pSender, _In_ IInspectable* pArgs)
{
    return UpdateApplicationBarServiceBounds();
}

_Check_return_ HRESULT UWPWindowImpl::OnCoreWindowClosed(_In_ wuc::ICoreWindow* pSender, _In_ IInspectable* pArgs)
{
    m_coreWindowClosed = true;
    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::UpdateApplicationBarServiceBounds()
{
    ctl::ComPtr<xaml::IUIElement> contentRoot;
    IFC_RETURN(get_ContentImpl(&contentRoot));
    if (contentRoot)
    {
        ctl::ComPtr<DependencyObject> contentRootDO;
        IFC_RETURN(contentRoot.As(&contentRootDO));

        auto xamlRoot = XamlRoot::GetImplementationForElementStatic(contentRootDO.Get());
        if (xamlRoot)
        {
            ctl::ComPtr<IApplicationBarService> applicationBarService;
            IFC_RETURN(xamlRoot->TryGetApplicationBarService(applicationBarService));
            if (applicationBarService)
            {
                IFC_RETURN(applicationBarService->OnBoundsChanged());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::OnCoreWindowSizeChanged(_In_ wuc::ICoreWindow* pSender, _In_ wuc::IWindowSizeChangedEventArgs* pArgs)
{
    wuv::ApplicationViewBoundsMode boundsMode;
    IFC_RETURN(LayoutBoundsChangedHelper::GetDesiredBoundsMode(&boundsMode));

    if(boundsMode == wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseCoreWindow)
    {
        IFCCHECK_RETURN(m_pCoreWindowWrapper);

        wf::Rect layoutBounds;
        IFC_RETURN(m_pCoreWindowWrapper->GetCoreWindow()->get_Bounds(&layoutBounds));

        // Handle the LayoutBounds changed event when there is a real bound changed since the bogus change
        // notification will have a issue on SIP or IHM's edit control BringIntoView scenario.
        if (layoutBounds.X != m_layoutBounds.X || layoutBounds.Y != m_layoutBounds.Y ||
            layoutBounds.Width != m_layoutBounds.Width || layoutBounds.Height != m_layoutBounds.Height)
        {
            IFC_RETURN(OnWindowSizeChanged());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::OnApplicationViewVisibleBoundsChanged(_In_ wuv::IApplicationView* pSender, _In_ IInspectable* pArgs)
{
    ctl::ComPtr<wuv::IApplicationView2> spAppView2;
    IFC_RETURN(LayoutBoundsChangedHelper::GetApplicationView2(&spAppView2));

    wuv::ApplicationViewBoundsMode boundsMode;
    IFC_RETURN(spAppView2->get_DesiredBoundsMode(&boundsMode));

    if (boundsMode == wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseVisible)
    {
        wf::Rect layoutBounds {};
        IFC_RETURN(spAppView2->get_VisibleBounds(&layoutBounds));

        // Handle the LayoutBounds changed event when there is a real bound changed since the bogus change
        // notification will have a issue on SIP or IHM's edit control BringIntoView scenario.
        if (layoutBounds.X != m_layoutBounds.X || layoutBounds.Y != m_layoutBounds.Y ||
            layoutBounds.Width != m_layoutBounds.Width || layoutBounds.Height != m_layoutBounds.Height)
        {
            IFC_RETURN(OnWindowSizeChanged());
        }
    }

    return S_OK;
}

WindowType::Enum UWPWindowImpl::GetWindowType()
{
    if (m_pCoreWindowWrapper)
    {
        return WindowType::CoreWindow;
    }
    else
    {
        return WindowType::None;
    }
}

_Check_return_ HRESULT UWPWindowImpl::ShowImpl()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT UWPWindowImpl::HideImpl()
{
    ElementSoundPlayerService* soundPlayerService = DXamlCore::GetCurrent()->TryGetElementSoundPlayerServiceNoRef();

    if (soundPlayerService)
    {
        // This code path occurs for instance when an XBox native application goes into connected standby mode. In order to transition from 'connected standby active'
        // to 'connected standby sleep', we tear down the AudioGraph and the input/output AudioNodes. The next time we attempt to play a sound we will attempt to recreate them.
        IFC_RETURN(soundPlayerService->TearDownAudioGraph());
    }

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::MoveWindowImpl(_In_ INT x, _In_ INT y, _In_ INT width, _In_ INT height)
{
    HRESULT hr = S_OK;
    ASSERT(m_pCoreWindowWrapper);

    IFC(m_pCoreWindowWrapper->SetPosition(x, y, width, height));

Cleanup:
    return(hr);
}

_Check_return_ HRESULT UWPWindowImpl::get_TransparentBackgroundImpl(_Out_ BOOLEAN* pValue)
{
    bool backgroundTransparency = false;

    m_pDXamlCoreNoRef->GetTransparentBackground(&backgroundTransparency);

    *pValue = backgroundTransparency;

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::put_TransparentBackgroundImpl(_In_ BOOLEAN value)
{
    m_pDXamlCoreNoRef->SetTransparentBackground(!!value);

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::SetAtlasSizeHintImpl(UINT width, UINT height)
{
    m_pDXamlCoreNoRef->SetAtlasSizeHint(width, height);

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::ReleaseGraphicsDeviceOnSuspendImpl(BOOLEAN enable)
{
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    runtimeEnabledFeatureDetector->SetFeatureOverride(RuntimeEnabledFeature::ReleaseGraphicsDeviceOnSuspend, !!enable);
    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::SetAtlasRequestCallbackImpl(_In_opt_ xaml::IAtlasRequestCallback* callback)
{
    m_atlasRequestCallback = callback;

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::GetWindowContentBoundsForElementImpl(
    _In_ xaml::IDependencyObject* element,
    _Out_ wf::Rect* rect)
{
    ctl::ComPtr<xaml::IDependencyObject> elementAsDO(element);
    IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(elementAsDO.Cast<DependencyObject>()->GetHandle(), rect));

    return S_OK;
}

ctl::ComPtr<xaml::IAtlasRequestCallback> UWPWindowImpl::GetAtlasRequestCallback()
{
    return m_atlasRequestCallback;
}

// -------------------------------------------------------
//              DXamlTestHooks
// -------------------------------------------------------
UINT32 UWPWindowImpl::GetWidthOverride()
{
    return m_widthOverride;
}

void UWPWindowImpl::SetWidthOverride(UINT32 width)
{
    m_widthOverride = width;
}

UINT32 UWPWindowImpl::GetHeightOverride()
{
    return m_heightOverride;
}

void UWPWindowImpl::SetHeightOverride(UINT32 height)
{
    m_heightOverride = height;
}

wf::Rect UWPWindowImpl::GetLayoutBoundsOverrides()
{
    return m_layoutBoundsOverrides;
}

void UWPWindowImpl::SetLayoutBoundsOverrides(const wf::Rect& rect)
{
    m_layoutBoundsOverrides = rect;
}

wf::Rect& UWPWindowImpl::GetVisibleBoundsOverrides()
{
    return m_visibleBoundsOverrides;
}

void UWPWindowImpl::SetVisibleBoundsOverrides(const wf::Rect& rect)
{
    m_visibleBoundsOverrides = rect;
}

void UWPWindowImpl::SetHasSizeOverrides(bool hasSizeOverrides)
{
    m_hasSizeOverrides = hasSizeOverrides;
}

void UWPWindowImpl::SetShrinkApplicationViewVisibleBounds(bool enabled)
{
    m_shrinkApplicationViewVisibleBounds = enabled;
}

// TODO: Move these
bool UWPWindowImpl::ShouldShrinkApplicationViewVisibleBounds() const
{
    return m_shrinkApplicationViewVisibleBounds;
}

IFACEMETHODIMP UWPWindowImpl::get_SystemBackdrop(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush)
{
    *systemBackdropBrush = nullptr;

    auto xamlCore = GetDXamlCore();
    if (xamlCore == nullptr)
    {
        return S_OK;
    }

    auto const coreServices = xamlCore->GetHandle();
    IFCEXPECT_RETURN(coreServices);

    auto const graphicsDeviceManager = coreServices->GetBrowserHost()->GetGraphicsDeviceManager();

    if (graphicsDeviceManager) // Can be null during shutdown
    {
        auto const dcompTreeHost = graphicsDeviceManager->GetDCompTreeHost();

        if (dcompTreeHost != nullptr)
        {
            IFC_RETURN(dcompTreeHost->GetSystemBackdropBrush(systemBackdropBrush));
        }
    }

    return S_OK;
}

IFACEMETHODIMP UWPWindowImpl::put_SystemBackdrop(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush)
{
    auto xamlCore = GetDXamlCore();
    if (xamlCore == nullptr)
    {
        IFC_RETURN(E_INVALID_OPERATION);
    }

    auto const coreServices = xamlCore->GetHandle();
    IFCEXPECT_RETURN(coreServices);

    auto const graphicsDeviceManager = coreServices->GetBrowserHost()->GetGraphicsDeviceManager();
    IFCEXPECT_RETURN(graphicsDeviceManager);

    IFC_RETURN(graphicsDeviceManager->EnsureDCompDevice());

    auto const dcompTreeHost = graphicsDeviceManager->GetDCompTreeHost();
    IFCEXPECT_RETURN(dcompTreeHost);

    IFC_RETURN(dcompTreeHost->SetSystemBackdropBrush(systemBackdropBrush));

    IFC_RETURN(put_TransparentBackgroundImpl(systemBackdropBrush != nullptr));

    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::get_WindowHandle(_Out_ HWND* pValue)
{
    // The QI for INativeWindow containing this method will always fail
    // and return an error on UWP.  This method cannot be called from a
    // UWP Window.
    return E_NOTIMPL;
}

_Check_return_ HRESULT UWPWindowImpl::get_ExtendsContentIntoTitleBarImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::put_ExtendsContentIntoTitleBarImpl(_In_ BOOLEAN value)
{
    IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_API_NOT_IMPLEMENTED_UWP));
    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::SetTitleBarImpl(_In_ xaml::IUIElement* pTitleBar)
{
    IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_API_NOT_IMPLEMENTED_UWP));
    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::get_AppWindowImpl(_Outptr_result_maybenull_ ixp::IAppWindow** ppValue)
{
    IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_NOTIMPL, ERROR_API_NOT_IMPLEMENTED_UWP));
    return S_OK;
}

_Check_return_ HRESULT UWPWindowImpl::get_SystemBackdropImpl(_Outptr_result_maybenull_ xaml::Media::ISystemBackdrop** systemBackdrop)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT UWPWindowImpl::put_SystemBackdropImpl(_In_opt_ xaml::Media::ISystemBackdrop* systemBackdrop)
{
    return E_NOTIMPL;
}