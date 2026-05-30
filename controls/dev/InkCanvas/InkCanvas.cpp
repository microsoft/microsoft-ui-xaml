// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "InkCanvas.h"
#include "InkCanvasAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "Microsoft.UI.Xaml.xamlroot.h"
#include "Microsoft.UI.Composition.h"
#include <pplawait.h>

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
// The only reason this data is scoped to the thread is because it saves us synchronization work, which,
// since the vast majority of application will only have one UI thread, would just be wasted and the
// cost here isn't that great.
struct ThreadData
{
    winrt::com_ptr<IInkDesktopHost> m_inkHost;
    winrt::com_ptr<IDCompositionDevice> m_compositionDevice;
    wil::unique_hmodule m_hmodDComp;
};

//
// Generic Ink Work Item Callback.  This allows us to easily submit work to the Ink thread using a lambda.
//
struct GenericInkCallback : winrt::implements<GenericInkCallback, IInkHostWorkItem>
{
    GenericInkCallback(const std::function<void()>& func)
        : m_func(func)
    {
    }

    IFACEMETHODIMP Invoke() try
    {
        m_func();
        return S_OK;
    }
    catch (...)
    {
        // REVIEW: Is this the way that we want to handle this?  IInkHostWorkItem::Invoke needs to return
        //         a HRESULT, but I can't find any information on what happens if it does.  There is no way
        //         to pass it back and raise it on the UI thread.
        return winrt::to_hresult();
    }

private:
    std::function<void()> m_func;
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

        // Create a desktop host which will create a ink thread.  Normally, I wouldn't want to do this until we actually needed
        // it, but unfortunately, we need the host to create the presenter and we need to create a presenter so that it can be 
        // accessed prior to entering the tree.
        winrt::check_hresult(CoCreateInstance(__uuidof(InkDesktopHost), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_threadData->m_inkHost.put())));
    }

    // Applications may want to access the ink presenter before they add the InkCanvas to the tree, so
    // make sure create one right away.
    CreateInkPresenter();
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

    // Hook up this ink canvas with the DComp tree.
    AttachToVisualLink();

 
    // Although the visual link will maintain position and clipping for for our visual, it won't update the
    // size of the system visual, which is ok, because its size does not clip its children (its clipping could,
    // but not its size).  However, the presenter won't see this size change either, so we need to explicitly
    // set the size of the presenter when the system rasterization scale, actual size or scale transform has changed.

    m_xamlRootChangedRevoker = XamlRoot().Changed(winrt::auto_revoke,
        [weakThis{ get_weak() }](auto const& /*sender*/, auto const& /*args*/)
        {
            if (auto strongThis = weakThis.get())
            {
                // Our Rasterization Scale may have changed.
                strongThis->UpdateInkPresenterSize();
            }
        });

    m_sizeChanged_revoker = SizeChanged(winrt::auto_revoke,
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

    if (UseSystemVisualLink())
    {
        // This is the first time we know the size of the ink canvas so update the presenter
        UpdateInkPresenterSize();
    }
    else
    {
        // This is the first time we have a position for the ink canvas in the scene so position the dcomp pieces.
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
    m_sizeChanged_revoker.revoke();

    DetachFromVisualLink();
}

void InkCanvas::OnIsEnabledPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto isEnabled = unbox_value<bool>(args.NewValue());

    QueueInkPresenterWorkItem([isEnabled](auto presenter)
        {
            presenter.IsInputEnabled(isEnabled);
        });
}

winrt::AutomationPeer InkCanvas::OnCreateAutomationPeer()
{
    return winrt::make<InkCanvasAutomationPeer>(*this);
}

winrt::IAsyncAction InkCanvas::QueueInkPresenterWorkItem(winrt::DoInkPresenterWork workItem)
{
    // Since the ink presenter is created on the ink thread, applications may want to request
    // presenter work before the presenter is created (e.g. setting rendering attributes).
    // This is Ok, because by the time the ink thread runs this work, the presenter will be there.
    //
    // Applications may also request presenter work as the InkCanvas is being shut down (e.g.
    // they want to save the ink strokes.  This is also OK, but we need to make sure that we
    // keep the presenter alive long enough for that work to occur.
    //
    // So we need to take a strong reference to the InkCanvas and pass that as part of the
    // work item so that we can retrieve the presenter if it isn't there yet and ensure that we
    // extend the life of the presenter until after the work is complete.
    
    concurrency::task_completion_event<void> taskComplete;

    auto workItemWrapper = [workItem, taskComplete, strongThis = get_strong()]()
        {
            try
            {
                // This shouldn't ever happen, since the first call to the ink thread should always be
                // to create the presenter.
                MUX_ASSERT(strongThis->m_inkPresenter);
                if (strongThis->m_inkPresenter)
                {
                    // Invoke the work item passing the presenter.
                    workItem(strongThis->m_inkPresenter);
                }
                taskComplete.set();
            }
            catch (...)
            {
                taskComplete.set_exception(std::current_exception);
            }
        };

    // Submit the work item to the ink thread
    winrt::check_hresult(m_threadData->m_inkHost->QueueWorkItem(winrt::make<GenericInkCallback>(workItemWrapper).get()));

    // create a task to wait for the work item to complete and await it.
    auto inktask = concurrency::create_task(taskComplete, concurrency::task_continuation_context::get_current_winrt_context());
    co_await inktask;

}

void InkCanvas::CreateInkPresenter()
{
    auto threadData = s_tlsThreadData.lock();
    auto inkHost = threadData->m_inkHost;
    auto weakThis = get_weak();

    auto callback = winrt::make<GenericInkCallback>([weakThis, inkHost]()
        {
            auto strongThis = weakThis.get();
            if (!strongThis)
            {
                return;
            }

            // Create ink presenter
            winrt::com_ptr inkPresenterDesktop = winrt::capture<IInkPresenterDesktop>(
                inkHost,
                &IInkDesktopHost::CreateInkPresenter);
            auto inkPresenter = inkPresenterDesktop.as<winrt::InkPresenter>();

            // Set up input devices
            winrt::CoreInputDeviceTypes types = winrt::CoreInputDeviceTypes::Mouse | winrt::CoreInputDeviceTypes::Pen | winrt::CoreInputDeviceTypes::Touch;
            inkPresenter.InputDeviceTypes(types);

            // Set the initial size.  This doesn't really mean anything and we can probably get away with out it,
            // but it helps in debugging, so for now we will leave it.
            winrt::check_hresult(inkPresenterDesktop->SetSize(400,400));

            // This m_inkPresetenr is only accessed on the ink thread, so we don't need to worry about contention.
            strongThis->m_inkPresenter = inkPresenter;
        });
    winrt::check_hresult(inkHost->QueueWorkItem(callback.get()));
}

void InkCanvas::UpdateInkPresenterSize()
{
    // Transform the width/height based on Xaml scaling
    auto transformer = TransformToVisual(nullptr);
    winrt::Rect rect{ 0, 0, static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight())};
    rect = transformer.TransformBounds(rect);

    // Get the system scale
    auto rootScale = XamlRoot().RasterizationScale();

    // Update the presenter
    QueueInkPresenterWorkItem([width = ActualWidth() * rootScale, height = ActualHeight() * rootScale](auto presenter)
        {
            auto inkPresenterDesktop = presenter.as<IInkPresenterDesktop>();
            inkPresenterDesktop->SetSize(static_cast<float>(width), static_cast<float>(height));
        });
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

    // Ensure we have composition device for this thread.
    if (!m_threadData->m_compositionDevice)
    {
        if (!m_threadData->m_hmodDComp)
        {
            m_threadData->m_hmodDComp.reset(::LoadLibraryExW(L"dcomp.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
            if (!m_threadData->m_hmodDComp)
            {
                throw winrt::hresult(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
        typedef HRESULT(__stdcall* DCompositionCreateDevice3fn)(IUnknown*, REFIID, void** dcompositionDevice);
        auto CompositionCreateDevice = reinterpret_cast<DCompositionCreateDevice3fn>(::GetProcAddress(m_threadData->m_hmodDComp.get(), "DCompositionCreateDevice3"));
        if (!CompositionCreateDevice)
        {
            throw winrt::hresult(HRESULT_FROM_WIN32(GetLastError()));
        }
        winrt::check_hresult(CompositionCreateDevice(nullptr, IID_PPV_ARGS(&m_threadData->m_compositionDevice)));
    }

    // Create our inking system visual
    winrt::check_hresult(m_threadData->m_compositionDevice->CreateVisual(m_inkRootVisual.put()));

    // Attach the visual to the presenter
    QueueInkPresenterWorkItem([rootVisual = m_inkRootVisual, compositionDevice = m_threadData->m_compositionDevice, useSystemVisualLink = UseSystemVisualLink()](auto presenter)
        {
            auto desktopPresenter = presenter.as<IInkPresenterDesktop>();
            winrt::check_hresult(desktopPresenter->SetRootVisual(rootVisual.get(), nullptr));
            // only request a commit here if we are using the visual link.  If we are using the composition
            // target method, it will be committed when we set position.
            if (useSystemVisualLink)
            {
                winrt::check_hresult(compositionDevice->Commit());
            }
        });

    // If we are using the composition target method then skip the visual link code
    if (AttachToCompositionTarget())
    {
        return;
    }

    // The visual link is created on the lifted side so we use the lifted compositor.
    auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();

    // Create the the visual link
    m_systemVisualLink = winrt::ContentExternalOutputLink::Create(compositor);
    m_systemVisualLink.IsAboveContent(true);

    // Set our ink visual into the visual link
    winrt::com_ptr<IDCompositionTarget> target = m_systemVisualLink.as<IDCompositionTarget>();
    winrt::check_hresult(target->SetRoot(m_inkRootVisual.get()));

    // Add the visual link's lifted visual to our tree
    winrt::ElementCompositionPreview::SetElementChildVisual(*this, m_systemVisualLink.PlacementVisual());
}

void InkCanvas::DetachFromVisualLink()
{
    // This will noop if we aren't using the composition target
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

// This section contains the code to handle the 'raw' composition target.  This is slightly confusing because the Visual link
// actually is a composition target as well, but this is the legacy composition target code. Everything below here can be
// deleted when the system visual link bug is fixed.
bool InkCanvas::UseSystemVisualLink()
{
    static bool useSystemVisualLink = [] {
        // We control whether we are using the system visual link by defining a boolean resource UseSystemVisualLink
        // in the application resources.  We initialize it upon first use so everything in the app gets the same
        // treatment.
        auto useSystemVisualKey = box_value(L"UseSystemVisualLink");
        if (winrt::Application::Current().Resources().HasKey(useSystemVisualKey))
        {
            return unbox_value<bool>(winrt::Application::Current().Resources().Lookup(useSystemVisualKey));
        }
        return false;
        }();
    return useSystemVisualLink;
}

bool InkCanvas::AttachToCompositionTarget()
{
    if (UseSystemVisualLink())
    {
        return false;
    }

    m_targetData = TargetData::Get(m_hostHwnd);

    // If we haven't created our composition target and root visual yet, do so
    if (!m_targetData->m_targetRootVisual)
    {
        auto threadData = s_tlsThreadData.lock();
        // Create a "top-most" target for this window
        winrt::check_hresult(threadData->m_compositionDevice->CreateTargetForHwnd(m_hostHwnd, TRUE /*topmost*/, m_targetData->m_compositionTarget.put()));

        // Attach a host visual.  This is different than the ink root visual although both of them use the term root.
        // One is the root of the composition target (we will call that hostVisual) and one is the root of the
        // ink presenter (we will call that rootVisual)
        winrt::check_hresult(threadData->m_compositionDevice->CreateVisual(m_targetData->m_targetRootVisual.put()));
        winrt::check_hresult(m_targetData->m_compositionTarget->SetRoot(m_targetData->m_targetRootVisual.get()));

        m_threadData->m_compositionDevice->Commit();
    }

    // Attach the visual to the target root
    winrt::check_hresult(m_targetData->m_targetRootVisual->AddVisual(m_inkRootVisual.get(), true, nullptr));

    // Register for the LayoutChanged event so we can move the system visual as the underlying control moves
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
    if (UseSystemVisualLink() || !m_targetData)
    {
        return;
    }

    // Quit listening for the layout changed event
    m_layoutUpdatedRevoker.revoke();

    // remove our system visual from the composition target tree
    winrt::check_hresult(m_targetData->m_targetRootVisual->RemoveVisual(m_inkRootVisual.get()));

    m_targetData.reset();
}

void InkCanvas::PositionInkVisual()
{
    // All of this is supposed be the functionality that we get from using the visual link
    if (UseSystemVisualLink()) return;

    // Get the transform from the root visual to the element
    auto transformer = TransformToVisual(nullptr);

    // Get the location of the Ink Canvas control in physical pixels.
    winrt::Rect rect { 0, 0, static_cast<float>(ActualWidth()), static_cast<float>(ActualHeight())};
    rect = transformer.TransformBounds(rect);

    // Use the same transform to get the current Xaml scale(s)
    winrt::Rect scaleRect{ 0, 0, 1, 1};
    scaleRect = transformer.TransformBounds(scaleRect);

    //  Set the offset position of the canvas.  It seems that dcomp doesn't account for
    //  the root scale for the offset so we need to apply it.
    const float rootScale = static_cast<float>(XamlRoot().RasterizationScale());
    m_inkRootVisual->SetOffsetX(rect.X * rootScale);
    m_inkRootVisual->SetOffsetY(rect.Y * rootScale);

    // Create the transform on on the system system visual.
    D2D_MATRIX_3X2_F visualTransform {
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

    m_threadData->m_compositionDevice->Commit();

    UpdateInkPresenterSize();
}

thread_local std::map<HWND, std::weak_ptr<InkCanvas::TargetData>> InkCanvas::TargetData::m_tlsMap;

