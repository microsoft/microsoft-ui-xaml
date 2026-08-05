// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "InkCanvas.h"
#include "InkCanvasAutomationPeer.h"
#include "InkPresenter.h"
#include "RuntimeProfiler.h"
#include "Microsoft.UI.Xaml.xamlroot.h"
#include "Microsoft.UI.Composition.h"
#include <pplawait.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Microsoft.UI.Composition.Experimental.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <future>
#include <vector>

// There is a sal bug in this header file that causes a compile warning (which we fail on) due to a
// value type being identified with _In_opt_ (value types cannot be optional because there is no
// way to know the difference between null and zero).  Disable this warning for this header.
#pragma warning(push)
#pragma warning (disable : 6553)
#include <wil\resource.h>
#pragma warning(pop)

// IExpCompositorInterop2 (system-composition switcher splice) is declared in the InteractiveExperiences
// package's experimental interop header. That header (and the ABI IVisual it pulls in) is internal-only
// and absent from public build flavors, so including it unconditionally breaks the public PR pipeline.
// Use the package header when it is present; otherwise declare the single interface we call locally.
// The local declaration is binary-compatible with the package header (same IID and vtable slot), and
// the runtime InteractiveExperiences DLL still provides the implementation (resolved by IID via
// QueryInterface on the compositor). It takes the parent visual as IUnknown* so it does not depend on
// the internal ABI composition header; the caller passes the projected Visual's default (IVisual)
// interface pointer either way.
#if __has_include(<Microsoft.UI.Composition.Experimental.Interop.h>)
#include <Microsoft.UI.Composition.Experimental.Interop.h>
#else
struct IDCompositionDesktopDevice;
struct IDCompositionTarget;
namespace ABI::Microsoft::UI::Composition::Experimental
{
    MIDL_INTERFACE("033C5AC8-5D75-4B18-90AB-BE8EB8E1E633")
    IExpCompositorInterop2 : public ::IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE CreateDCompVisualUnderMUCVisual(
            _In_ ::IUnknown* parentMucVisual,
            _In_ ::IDCompositionDesktopDevice* externalDevice,
            _COM_Outptr_ ::IDCompositionTarget** ppTarget) = 0;
    };
}
#endif

// We use a weak pointer to track this so that it goes away when the last Ink control goes
// away, rather than living until the end of the thread.
thread_local std::weak_ptr<ThreadData> s_tlsThreadData;

//
// Thread Data
//
//
// This data is shared by every InkCanvas created on the same UI thread. The host and DComp
// device are per-thread singletons: the first InkCanvas allocates them, subsequent canvases
// reuse them, and they are released once the last InkCanvas on the thread is destroyed (the
// map holds a weak_ptr so lifetime tracks the controls, not the thread). Each InkCanvas still
// owns its own InkPresenter and its own ink root visual; only the underlying host/device (and,
// separately, the per-HWND composition target - see TargetData) are shared. Scoping to the
// thread avoids synchronization work that, for the common single-UI-thread app, would just be
// wasted, and the cost of the shared objects here isn't that great.
struct ThreadData
{
    winrt::com_ptr<IInkDesktopHost> m_inkHost;
    // System DirectComposition device (dcomp.dll) used by the CreateTargetForHwnd rendering path.
    winrt::com_ptr<IDCompositionDevice> m_compositionDevice;
    wil::unique_hmodule m_hmodDComp;
};

//
// IInkCommitRequestHandler implementation.
// InkPresenter calls OnCommitRequested() when ink transitions from wet to dry
// and needs the app to commit the DComposition device.
//
struct InkCommitRequestHandler : winrt::implements<InkCommitRequestHandler, IInkCommitRequestHandler>
{
    InkCommitRequestHandler(winrt::com_ptr<IDCompositionDevice> device)
        : m_device(device) {}

    IFACEMETHODIMP OnCommitRequested() override
    {
        if (m_device)
        {
            return m_device->Commit();
        }
        return S_OK;
    }

private:
    winrt::com_ptr<IDCompositionDevice> m_device;
};

//
//  InkCanvas
//

InkCanvas::InkCanvas()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InkCanvas);

    m_loadedRevoker = Loaded(winrt::auto_revoke, { this, &InkCanvas::OnLoaded });
    m_unloadedRevoker = Unloaded(winrt::auto_revoke, { this, &InkCanvas::OnUnloaded });

    // Ensure that we have allocated our thread data
    m_threadData = s_tlsThreadData.lock();
    if (!m_threadData)
    {
        // This is our first Ink Canvas on this thread so do a little bit of thread initialization.
        m_threadData = std::make_unique<ThreadData>();
        s_tlsThreadData = m_threadData;
    }

    // The presenter (proxy + OS presenter) is created lazily by EnsureInkPresenter() on first use
    // - either when the app touches InkPresenter() or when the control loads. We deliberately do
    // NOT create it in the constructor: EnsureInkPresenter() needs *this to build the proxy, and
    // taking a strong/weak self ref before construction finishes is unsafe.
}

InkCanvas::~InkCanvas()
{
    // Ensure that we have torn down our dcomp stuff
    DetachFromVisualLink();
}

void InkCanvas::OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    // Historically, there has been an issue with the firing of loaded/unloaded.  Although we are working
    // to fix this, we will implement the official workaround here to prevent any timing issues between these
    // changes;
    if (!IsLoaded())
    {
        return;
    }

    // Make sure the presenter (proxy + OS presenter) exists before we queue any ink-thread work
    // (SetRootVisual below runs against it). Safe here: we are past construction and on the UI thread.
    EnsureInkPresenter();

    // Hook up this ink canvas with the DComp tree.
    AttachToVisualLink();

    // The composition target maintains position/clipping for our visual, but the presenter
    // does not see size changes, so explicitly update the presenter size when the rasterization
    // scale, actual size or scale transform changes.

    // Previously we used get_weak() here, but we found the potential to hit a
    // C++/WinRT refcounting problem (cppwinrt #1431) where, for composed/aggregated
    // objects, the projected get_weak() can over-release the outer object. make_weak()
    // on the projected type routes the weak reference through the outer object and is safe.
    auto weakThis{ winrt::make_weak(static_cast<winrt::InkCanvas>(*this)) };
    m_xamlRootChangedRevoker = XamlRoot().Changed(winrt::auto_revoke,
        [weakThis](auto const& /*sender*/, auto const& /*args*/)
        {
            if (auto strongThis = weakThis.get())
            {
                // Our Rasterization Scale may have changed.
                winrt::get_self<InkCanvas>(strongThis)->UpdateInkPresenterSize();
            }
        });

    m_sizeChangedRevoker = SizeChanged(winrt::auto_revoke,
        [weakThis](auto const& sender, auto const& /*args*/)
        {
            if (auto strongThis = weakThis.get())
            {
                winrt::get_self<InkCanvas>(strongThis)->UpdateInkPresenterSize();
            }
        });

    // Bug 52084592: Need way to be notified of xaml scale factor changes above an element.
    //
    // There doesn't appear to be an event that we can listen to to know when the scale factor for the canvas has
    // changed.  This is required, because the Ink Canvas uses this scale factor to configure the Ink Presenter
    // size in physical pixels.  Note: this is the accumulated scale from the root, not just the scale on the Ink
    // canvas.
    //
    // When we know what event to be listening for, add it here.

    // Both compositor paths host the ink visual in the lifted XAML tree, which positions/clips/
    // scrolls it natively; the presenter still needs its size in physical pixels though.
    UpdateInkPresenterSize();
}

void InkCanvas::OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    // Historically, there has been an issue with the firing of loaded/unloaded.  Although we are working
    // to fix this, we will implement the official workaround here to prevent any timing issues between these
    // changes;
    if (this->IsLoaded())
    {
        return;
    }

    m_xamlRootChangedRevoker.revoke();
    m_sizeChangedRevoker.revoke();

    DetachFromVisualLink();
}

winrt::AutomationPeer InkCanvas::OnCreateAutomationPeer()
{
    return winrt::make<InkCanvasAutomationPeer>(*this);
}

muxc::InkPresenter InkCanvas::InkPresenter()
{
    EnsureInkPresenter();
    return m_inkPresenterProxy;
}

// Creates the marshaling proxy and kicks off creation of the OS presenter on the ink thread. The
// proxy owns the OS presenter and the ink-thread work queue; InkCanvas just hands it the shared ink
// host + this control's UI dispatcher. Idempotent and lazy: called from InkPresenter() (first app
// access) and from OnLoaded (before any ink-thread work is queued). Not called from the constructor
// because building the proxy needs *this, which is unsafe before construction completes.
void InkCanvas::EnsureInkPresenter()
{
    if (m_inkPresenterProxy)
    {
        return;
    }

    // Ensure the shared per-thread ink host (and its dedicated ink thread) exists; the proxy needs
    // it at construction to create and service the OS presenter.
    if (!m_threadData->m_inkHost)
    {
        winrt::check_hresult(CoCreateInstance(__uuidof(InkDesktopHost), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_threadData->m_inkHost.put())));
    }

    // Construct the proxy with the ink host and this control's UI-thread dispatcher (captured here,
    // on the UI thread), then start OS-presenter creation on the ink thread. Start() takes a self
    // weak-ref, which is only safe post-construction - hence it is not done in the proxy's ctor.
    m_inkPresenterProxy = winrt::make<::InkPresenter>(m_threadData->m_inkHost, DispatcherQueue());
    winrt::get_self<::InkPresenter>(m_inkPresenterProxy)->Start();
}

void InkCanvas::UpdateInkPresenterSize()
{
    // XamlRoot() can be null while unloading / reparenting. This runs from the SizeChanged
    // and XamlRoot().Changed handlers, so guard it the same way as PositionInkVisual /
    // AttachToVisualLink to avoid dereferencing a null XamlRoot for RasterizationScale().
    auto xamlRoot = XamlRoot();
    if (!xamlRoot)
    {
        return;
    }

    // Transform the width/height based on Xaml scaling
    auto transformer = TransformToVisual(nullptr);
    winrt::Rect rect{ 0, 0, static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight())};
    rect = transformer.TransformBounds(rect);

    // Get the system scale
    auto rootScale = xamlRoot.RasterizationScale();

    // Push the new physical-pixel size onto the OS presenter (on the ink thread) through the proxy.
    // The proxy's queue no-ops if the OS presenter has not been created yet.
    if (m_inkPresenterProxy)
    {
        winrt::get_self<::InkPresenter>(m_inkPresenterProxy)->QueueInkPresenterWorkItem(
            [width = ActualWidth() * rootScale, height = ActualHeight() * rootScale](inking::InkPresenter const& presenter)
            {
                presenter.as<IInkPresenterDesktop>()->SetSize(static_cast<float>(width), static_cast<float>(height));
            });
    }
}

void InkCanvas::AttachToVisualLink()
{
    // Verify that we are still attached to the same hwnd.  If the application moves an ink canvas from one
    // window to another, we will need to detach and reattach to a new visual link.
    HWND hostHwnd = NULL;
    auto xamlRoot = this->XamlRoot();
    if (!xamlRoot)
    {
        throw winrt::hresult_error(E_UNEXPECTED);
    }

    winrt::com_ptr<IXamlRootNative> xamlRootNative = xamlRoot.as<IXamlRootNative>();
    winrt::check_hresult(xamlRootNative->get_HostWindow(&hostHwnd));

    // Our target is good, we can continue to use it.
    if (hostHwnd == m_hostHwnd)
    {
        return;
    }

    // Our current target is stale so detach from it
    if (m_hostHwnd)
    {
        DetachFromVisualLink();
    }

    m_hostHwnd = hostHwnd;

    // Ensure the shared system DirectComposition device (both compositor paths render ink through
    // it). The ink visual is created, bound to the presenter, and rooted under the chosen
    // compositor's target inside the fork below - deliberately not before it, so nothing is attached
    // until the compositor engine has been decided.
    EnsureCompositionDevice();

    // Fork on the compositor engine (IsSystemCompositor detects it via GetForSystemEngine): a
    // system-backed process splices the ink visual under a lifted MUC visual; a lifted process
    // bridges it into the XAML tree via ContentExternalOutputLink. Each path binds the ink visual to
    // the presenter (AttachInkVisualToPresenter) first, then roots it under its own target.
    if (IsSystemCompositor())
    {
        AttachToSystemCompositor();
    }
    else
    {
        AttachToLiftedCompositor();
    }
}

// Ensures the per-thread system DirectComposition device used by the rendering paths.
void InkCanvas::EnsureCompositionDevice()
{
    if (m_threadData->m_compositionDevice)
    {
        return;
    }
    if (!m_threadData->m_hmodDComp)
    {
        m_threadData->m_hmodDComp.reset(::LoadLibraryExW(L"dcomp.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
        if (!m_threadData->m_hmodDComp)
        {
            winrt::throw_hresult(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    typedef HRESULT(__stdcall* DCompositionCreateDevice3fn)(IUnknown*, REFIID, void** dcompositionDevice);
    auto createDevice = reinterpret_cast<DCompositionCreateDevice3fn>(::GetProcAddress(m_threadData->m_hmodDComp.get(), "DCompositionCreateDevice3"));
    if (!createDevice)
    {
        winrt::throw_hresult(HRESULT_FROM_WIN32(GetLastError()));
    }
    winrt::check_hresult(createDevice(nullptr, IID_PPV_ARGS(&m_threadData->m_compositionDevice)));
}

// Creates this canvas's ink visual on the shared system DComp device and binds it to the OS
// presenter on the ink thread. Compositor-independent: the same ink visual is rooted under either
// compositor's target by the caller, so AttachToSystemCompositor and AttachToLiftedCompositor both
// call this first, before their compositor-specific rooting.
void InkCanvas::AttachInkVisualToPresenter()
{
    winrt::check_hresult(m_threadData->m_compositionDevice->CreateVisual(m_inkRootVisual.put()));

    // Clear the detach flag BEFORE queuing so the ink thread doesn't drop the SetRootVisual work.
    m_isDetached.store(false, std::memory_order_release);

    // Bind the visual to the presenter on the ink thread. SetRootVisual drives both rendering and
    // input routing. XAML positions/clips the ink visual for both compositor paths, so commit here.
    // No commit-request handler (conflicts with InkSynchronizer).
    winrt::get_self<::InkPresenter>(m_inkPresenterProxy)->QueueInkPresenterWorkItem([rootVisual = m_inkRootVisual, compositionDevice = m_threadData->m_compositionDevice](inking::InkPresenter const& presenter)
        {
            auto desktopPresenter = presenter.as<IInkPresenterDesktop>();
            winrt::check_hresult(desktopPresenter->SetRootVisual(rootVisual.get(), nullptr));
            winrt::check_hresult(compositionDevice->Commit());
        });
}

// System compositor path: splices the ink visual directly under a lifted MUC visual via
// IExpCompositorInterop2::CreateDCompVisualUnderMUCVisual, so lifted XAML natively clips/scrolls/
// z-orders it. Only reached when IsSystemCompositor() is true, so the interop must be present.
void InkCanvas::AttachToSystemCompositor()
{
    // Create the ink visual and bind it to the presenter before the compositor-specific splice.
    AttachInkVisualToPresenter();

    auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();

    winrt::com_ptr<ABI::Microsoft::UI::Composition::Experimental::IExpCompositorInterop2> interop;
    winrt::check_hresult(winrt::get_unknown(compositor)->QueryInterface(IID_PPV_ARGS(interop.put())));

    auto mucRootVisual = compositor.CreateContainerVisual();
    auto desktopDevice = m_threadData->m_compositionDevice.as<IDCompositionDesktopDevice>();

    // Get the MUC visual's IVisual interface pointer through the projection: up-cast to Visual (whose
    // default interface is IVisual) and take its ABI pointer. The projected ContainerVisual's own default
    // ABI interface is IContainerVisual, which is why we up-cast to Visual first. Doing it through the
    // projection avoids a compile-time dependency on the internal ABI composition header; the pointer is
    // forwarded unchanged to the interop (as ABI IVisual* with the package header, IUnknown* without it).
    auto parentVisual = mucRootVisual.as<winrt::Microsoft::UI::Composition::Visual>();
    auto parentAbi = winrt::get_abi(parentVisual);

    // m_systemDCompTarget roots the ink visual under the MUC visual and must outlive this call; it
    // is released in DetachFromVisualLink.
    winrt::check_hresult(interop->CreateDCompVisualUnderMUCVisual(
#if __has_include(<Microsoft.UI.Composition.Experimental.Interop.h>)
        reinterpret_cast<ABI::Microsoft::UI::Composition::IVisual*>(parentAbi),
#else
        reinterpret_cast<::IUnknown*>(parentAbi),
#endif
        desktopDevice.get(),
        m_systemDCompTarget.put()));
    winrt::check_hresult(m_systemDCompTarget->SetRoot(m_inkRootVisual.get()));

    winrt::ElementCompositionPreview::SetElementChildVisual(*this, mucRootVisual);
}

// Lifted compositor path: ContentExternalOutputLink produces a lifted PlacementVisual (backed by a
// system proxy visual) parented into the XAML tree, so lifted XAML clips/scrolls/z-orders the ink.
void InkCanvas::AttachToLiftedCompositor()
{
    // Create the ink visual and bind it to the presenter before the compositor-specific bridge.
    AttachInkVisualToPresenter();

    auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();

    m_systemVisualLink = winrt::ContentExternalOutputLink::Create(compositor);
    m_systemVisualLink.IsAboveContent(true);

    winrt::com_ptr<IDCompositionTarget> target = m_systemVisualLink.as<IDCompositionTarget>();
    winrt::check_hresult(target->SetRoot(m_inkRootVisual.get()));

    winrt::ElementCompositionPreview::SetElementChildVisual(*this, m_systemVisualLink.PlacementVisual());
}

void InkCanvas::DetachFromVisualLink()
{
    // Mark destruction-safety: flag detach BEFORE we tear down anything so concurrent
    // ink-thread lambdas observe the detached state and short-circuit instead of touching
    // the OS presenter / system-visual resources mid-teardown. Cheap acquire/release pair.
    m_isDetached.store(true, std::memory_order_release);

    winrt::ElementCompositionPreview::SetElementChildVisual(*this, nullptr);

    m_systemDCompTarget = nullptr;
    m_systemVisualLink = nullptr;
    m_inkRootVisual = nullptr;
    m_hostHwnd = NULL;

    // When we give up our last reference to the system visual, we need to let the
    // the system compositor know it has work to do.  Note that it is possible that
    // the application created, the canvas, but never used it (attached it to the tree),
    // so we may not have a compositor.
    if (m_threadData->m_compositionDevice)
    {
        m_threadData->m_compositionDevice->Commit();
    }
}

// Compositor-engine detection: true when the process exposes the system-compositor splice interop.
// QueryInterface is used as the gate so public builds that do not project newer CompositionEngine
// APIs still compile and naturally fall back to the lifted ContentExternalOutputLink path. Evaluated
// once per process on first use, so every InkCanvas on the thread agrees for the process lifetime.
bool InkCanvas::IsSystemCompositor()
{
    static bool isSystemCompositor = [] {
        auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
        winrt::com_ptr<ABI::Microsoft::UI::Composition::Experimental::IExpCompositorInterop2> interop;
        return SUCCEEDED(winrt::get_unknown(compositor)->QueryInterface(IID_PPV_ARGS(interop.put())));
    }();
    return isSystemCompositor;
}


