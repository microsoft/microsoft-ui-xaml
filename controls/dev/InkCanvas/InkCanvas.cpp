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

    m_xamlRootChangedRevoker = XamlRoot().Changed(winrt::auto_revoke,
        [weakThis{ get_weak() }](auto const& /*sender*/, auto const& /*args*/)
        {
            if (auto strongThis = weakThis.get())
            {
                // Our Rasterization Scale may have changed.
                strongThis->UpdateInkPresenterSize();
            }
        });

    m_sizeChangedRevoker = SizeChanged(winrt::auto_revoke,
        [weakThis{ get_weak() }](auto const& sender, auto const& /*args*/)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->UpdateInkPresenterSize();
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

    // Position the ink visual at the control's on-screen location (also updates the presenter
    // size). Required for both correct rendering placement AND input hit-testing.
    if (UseSystemVisualLink())
    {
        // ContentExternalOutputLink places/clips/scrolls the ink visual natively through the
        // lifted XAML tree, so there is no manual positioning to do here; the presenter still
        // needs its size in physical pixels though.
        UpdateInkPresenterSize();
    }
    else
    {
        PositionInkVisual();
    }
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

    // Disabled switcher fast path short-circuits when the system compositor is available.
    if (TryAttachSwitcherVisual())
    {
        return;
    }

    // Default rendering uses a topmost per-HWND DComp target. Ensure the shared device, then
    // create this canvas's ink visual.
    EnsureCompositionDevice();
    winrt::check_hresult(m_threadData->m_compositionDevice->CreateVisual(m_inkRootVisual.put()));

    // Clear the detach flag BEFORE queuing so the ink thread doesn't drop the SetRootVisual work.
    m_isDetached.store(false, std::memory_order_release);

    // Attach the visual to the presenter on the ink thread. SetRootVisual drives both rendering and
    // input routing. The CEOL path has no positioning step, so commit here; the topmost path commits
    // when it positions the visual. No commit-request handler (conflicts with InkSynchronizer).
    winrt::get_self<::InkPresenter>(m_inkPresenterProxy)->QueueInkPresenterWorkItem([rootVisual = m_inkRootVisual, compositionDevice = m_threadData->m_compositionDevice, useSystemVisualLink = UseSystemVisualLink()](inking::InkPresenter const& presenter)
        {
            auto desktopPresenter = presenter.as<IInkPresenterDesktop>();
            winrt::check_hresult(desktopPresenter->SetRootVisual(rootVisual.get(), nullptr));
            if (useSystemVisualLink)
            {
                winrt::check_hresult(compositionDevice->Commit());
            }
        });

    // Host the ink visual: topmost per-HWND target (default) or the system visual link (CEOL).
    if (AttachToCompositionTarget())
    {
        return;
    }

    AttachSystemVisualLink();
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

// Switcher fast path (disabled; enable with the CompositionEngine API from IXP 1893+). On the
// system engine, hosts the ink visual in the app's own tree instead of a topmost overlay.
// Returns true if it attached.
bool InkCanvas::TryAttachSwitcherVisual()
{
#if 0
    namespace muce = winrt::Microsoft::UI::Composition::Experimental;
    namespace wuc = winrt::Windows::UI::Composition;

    auto mucCompositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
    if (auto wucCompositor = muce::CompositionEngine::GetForSystemEngine(mucCompositor).try_as<wuc::Compositor>())
    {
        auto desktopInterop = wucCompositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();
        winrt::com_ptr<ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget> desktopTarget;
        winrt::check_hresult(desktopInterop->CreateDesktopWindowTarget(m_hostHwnd, TRUE /*topmost*/, desktopTarget.put()));

        auto root = wucCompositor.CreateContainerVisual();
        desktopTarget.as<wuc::Desktop::DesktopWindowTarget>().Root(root);

        EnsureCompositionDevice();
        winrt::check_hresult(m_threadData->m_compositionDevice->CreateVisual(m_inkRootVisual.put()));
        m_isDetached.store(false, std::memory_order_release);
        QueueInkPresenterWorkItem([rootVisual = m_inkRootVisual, device = m_threadData->m_compositionDevice](auto presenter)
            {
                auto desktop = presenter.as<IInkPresenterDesktop>();
                winrt::check_hresult(desktop->SetRootVisual(rootVisual.get(), device.get()));
            });
        return true;
    }
#endif
    return false;
}

// Fallback host: places the ink DComp visual in the lifted XAML tree via ContentExternalOutputLink
// so XAML clips/scrolls/z-orders it (used when the topmost composition-target path is off).
void InkCanvas::AttachSystemVisualLink()
{
#if 0
    // Approach B (disabled): bridge under a lifted MUC visual via IExpCompositorInterop2. Needs the
    // vendored Microsoft.UI.Composition.Experimental.Interop.h (absent from consumed transport pkgs).
    {
        auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
        auto mucRootVisual = compositor.CreateContainerVisual();

        winrt::com_ptr<ABI::Microsoft::UI::Composition::Experimental::IExpCompositorInterop2> interop;
        winrt::check_hresult(winrt::get_unknown(compositor)->QueryInterface(IID_PPV_ARGS(interop.put())));

        auto desktopDevice = m_threadData->m_compositionDevice.as<IDCompositionDesktopDevice>();

        winrt::com_ptr<IDCompositionTarget> target;
        winrt::check_hresult(interop->CreateDCompVisualUnderMUCVisual(
            reinterpret_cast<ABI::Microsoft::UI::Composition::IVisual*>(winrt::get_abi(mucRootVisual)),
            desktopDevice.get(),
            target.put()));
        winrt::check_hresult(target->SetRoot(m_inkRootVisual.get()));

        winrt::ElementCompositionPreview::SetElementChildVisual(*this, mucRootVisual);
        return;
    }
#endif

    // Approach A (active): ContentExternalOutputLink produces a lifted PlacementVisual (backed by a
    // system proxy visual + shared handle). SetRoot parents the ink visual under it; the
    // PlacementVisual goes into the XAML tree so XAML clips/scrolls/z-orders the ink.
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

    // Remove our ink visual from the shared per-HWND composition target.
    DetachFromCompositionTarget();

    winrt::ElementCompositionPreview::SetElementChildVisual(*this, nullptr);

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

// ---------------------------------------------------------------------------
// Composition-target (CreateTargetForHwnd) rendering + input path.
//
// A topmost DComp target is created for the top-level HWND and the ink visual is parented
// under it. Because the target is tied to the window, the desktop ink presenter receives
// pointer input for the region covered by its visual -> strokes are collected. Only ONE
// topmost target is permitted per HWND, so all InkCanvas controls in a window share one
// target (TargetData) and each parents/positions its own ink visual under the shared root.
// ---------------------------------------------------------------------------

thread_local std::map<HWND, std::weak_ptr<InkCanvas::TargetData>> InkCanvas::TargetData::m_tlsMap;

// The system-visual-link (ContentExternalOutputLink) path is opt-in via a boolean app resource
// named "UseSystemVisualLink". It is evaluated once, on first use, so every InkCanvas in the app
// gets the same treatment for the process lifetime. When true, the ink DComp visual is spliced
// into the lifted XAML tree (which clips/scrolls/z-orders it) instead of being parented under a
// topmost per-HWND composition target.
bool InkCanvas::UseSystemVisualLink()
{
    static bool useSystemVisualLink = [] {
        auto useSystemVisualKey = winrt::box_value(L"UseSystemVisualLink");
        if (winrt::Application::Current().Resources().HasKey(useSystemVisualKey))
        {
            return winrt::unbox_value<bool>(winrt::Application::Current().Resources().Lookup(useSystemVisualKey));
        }
        return false;
    }();
    return useSystemVisualLink;
}

bool InkCanvas::AttachToCompositionTarget()
{
    // When the system-visual-link (CEOL) path is enabled, skip the topmost composition target
    // entirely and report that we did nothing; AttachToVisualLink() then wires up CEOL instead.
    if (UseSystemVisualLink())
    {
        return false;
    }

    m_targetData = TargetData::Get(m_hostHwnd);

    // If we haven't created the shared composition target + root visual for this HWND yet,
    // do so now. Subsequent canvases on the same HWND reuse it.
    if (!m_targetData->m_targetRootVisual)
    {
        // Create a "top-most" target for this window.
        winrt::check_hresult(m_threadData->m_compositionDevice->CreateTargetForHwnd(m_hostHwnd, TRUE /*topmost*/, m_targetData->m_compositionTarget.put()));

        // Attach a host visual. This is the root of the composition target (distinct from
        // each canvas's own ink root visual).
        winrt::check_hresult(m_threadData->m_compositionDevice->CreateVisual(m_targetData->m_targetRootVisual.put()));
        winrt::check_hresult(m_targetData->m_compositionTarget->SetRoot(m_targetData->m_targetRootVisual.get()));

        m_threadData->m_compositionDevice->Commit();
    }

    // Parent this canvas's ink visual under the shared target root.
    winrt::check_hresult(m_targetData->m_targetRootVisual->AddVisual(m_inkRootVisual.get(), TRUE, nullptr));

    // Reposition the ink visual whenever layout changes (control moves/resizes).
    m_layoutUpdatedRevoker = LayoutUpdated(winrt::auto_revoke,
        [weakThis{ get_weak() }](auto const& /*sender*/, auto const& /*args*/)
        {
            if (auto strongThis = weakThis.get())
            {
                strongThis->PositionInkVisual();
            }
        });

    return true;
}

void InkCanvas::DetachFromCompositionTarget()
{
    // No topmost composition target exists in the system-visual-link path.
    if (UseSystemVisualLink() || !m_targetData)
    {
        return;
    }

    m_layoutUpdatedRevoker.revoke();

    // Remove our ink visual from the shared composition target tree.
    if (m_targetData->m_targetRootVisual && m_inkRootVisual)
    {
        m_targetData->m_targetRootVisual->RemoveVisual(m_inkRootVisual.get());
    }

    m_targetData.reset();
}

void InkCanvas::PositionInkVisual()
{
    // In the system-visual-link path XAML positions/clips/scrolls the ink visual for us, so there
    // is nothing to do here.
    if (UseSystemVisualLink())
    {
        return;
    }

    if (!m_inkRootVisual || !m_threadData->m_compositionDevice)
    {
        return;
    }

    auto xamlRoot = XamlRoot();
    if (!xamlRoot)
    {
        return;
    }

    // Get the transform from the root visual to the element.
    auto transformer = TransformToVisual(nullptr);

    // Location of the InkCanvas control in DIPs.
    winrt::Rect rect{ 0, 0, static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight()) };
    rect = transformer.TransformBounds(rect);

    // Same transform to recover the current Xaml scale(s).
    winrt::Rect scaleRect{ 0, 0, 1, 1 };
    scaleRect = transformer.TransformBounds(scaleRect);

    // DComp does not account for the root scale on the offset, so apply it here.
    const float rootScale = static_cast<float>(xamlRoot.RasterizationScale());
    m_inkRootVisual->SetOffsetX(rect.X * rootScale);
    m_inkRootVisual->SetOffsetY(rect.Y * rootScale);

    D2D_MATRIX_3X2_F visualTransform{
        scaleRect.Width, 0,
        0, scaleRect.Height,
        0, 0
    };
    if (FlowDirection() == winrt::FlowDirection::RightToLeft)
    {
        visualTransform.m11 *= -1;
        visualTransform.dx = rect.Width * rootScale;
    }
    m_inkRootVisual->SetTransform(visualTransform);

    // The composition target is a *topmost* overlay that is NOT clipped by ancestor
    // ScrollViewers, so without an explicit clip the ink would paint outside the control
    // box (over sticky headers / neighboring content) whenever the control is scrolled or a
    // stroke runs past the control bounds. Clip the ink visual to the intersection of the
    // control rect with the nearest scrolling viewport (falling back to the XamlRoot),
    // expressed in the visual's local, physical-pixel content space.
    {
        // Visible region in root DIPs, starting from the XamlRoot bounds.
        auto rootSize = xamlRoot.Size();
        float viewLeft = 0.0f, viewTop = 0.0f;
        float viewRight = rootSize.Width, viewBottom = rootSize.Height;

        // Narrow to the nearest scrolling ancestor's on-screen rect, if any.
        winrt::DependencyObject node = *this;
        while (auto parent = winrt::VisualTreeHelper::GetParent(node))
        {
            if (auto fe = parent.try_as<winrt::FrameworkElement>())
            {
                std::wstring cn{ winrt::get_class_name(parent).c_str() };
                if (cn.find(L"ScrollViewer") != std::wstring::npos ||
                    cn.find(L"ScrollView") != std::wstring::npos ||
                    cn.find(L"ScrollContentPresenter") != std::wstring::npos ||
                    cn.find(L"ScrollPresenter") != std::wstring::npos)
                {
                    auto svRect = fe.TransformToVisual(nullptr).TransformBounds(
                        winrt::Rect{ 0, 0, static_cast<float>(fe.ActualWidth()), static_cast<float>(fe.ActualHeight()) });
                    if (svRect.X > viewLeft) viewLeft = svRect.X;
                    if (svRect.Y > viewTop) viewTop = svRect.Y;
                    if (svRect.X + svRect.Width < viewRight) viewRight = svRect.X + svRect.Width;
                    if (svRect.Y + svRect.Height < viewBottom) viewBottom = svRect.Y + svRect.Height;
                    break;
                }
            }
            node = parent;
        }

        // Intersect the control rect with the viewport (root DIPs).
        float visL = rect.X > viewLeft ? rect.X : viewLeft;
        float visT = rect.Y > viewTop ? rect.Y : viewTop;
        float visR = (rect.X + rect.Width) < viewRight ? (rect.X + rect.Width) : viewRight;
        float visB = (rect.Y + rect.Height) < viewBottom ? (rect.Y + rect.Height) : viewBottom;

        D2D_RECT_F clip;
        if (visR <= visL || visB <= visT)
        {
            // Fully outside the viewport: clip to empty so nothing paints.
            clip = D2D_RECT_F{ 0, 0, 0, 0 };
        }
        else
        {
            // Convert to the visual's local content space (physical pixels, relative to the
            // control's top-left).
            clip = D2D_RECT_F{
                (visL - rect.X) * rootScale,
                (visT - rect.Y) * rootScale,
                (visR - rect.X) * rootScale,
                (visB - rect.Y) * rootScale
            };
        }
        m_inkRootVisual->SetClip(clip);
    }

    m_threadData->m_compositionDevice->Commit();

    UpdateInkPresenterSize();
}


