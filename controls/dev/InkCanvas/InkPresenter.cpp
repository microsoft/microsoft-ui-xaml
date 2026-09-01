// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InkCanvas.h"
#include "InkPresenter.h"
#include <future>

// ---------------------------------------------------------------------------
// Generic ink work item callback: lets us submit a lambda to the ink host thread.
// ---------------------------------------------------------------------------
namespace
{
    struct GenericInkCallback : winrt::implements<GenericInkCallback, IInkHostWorkItem>
    {
        GenericInkCallback(std::function<void()> func)
            : m_func(std::move(func))
        {
        }

        IFACEMETHODIMP Invoke() try
        {
            m_func();
            return S_OK;
        }
        catch (...)
        {
            // IInkHostWorkItem::Invoke must return an HRESULT; there is no channel to raise this on
            // the UI thread, so surface it as the callback's result.
            return winrt::to_hresult();
        }

    private:
        std::function<void()> m_func;
    };
}

// ---------------------------------------------------------------------------
// Ink-thread marshaling helpers
//
// The child proxies (InkStrokeContainer / InkInputProcessingConfiguration / InkInputConfiguration)
// call these instead of reaching into the InkPresenter proxy's work queue directly. They forward to
// the queue on the owning InkPresenter proxy - the sole owner of the OS presenter and the ink host
// reference. Both no-op if the proxy has already been destroyed.
// ---------------------------------------------------------------------------

void RunOnInkHostThread(winrt::weak_ref<muxc::InkPresenter> const& presenter, std::function<void(inking::InkPresenter const&)> workItem)
{
    if (auto strong = presenter.get())
    {
        winrt::get_self<::InkPresenter>(strong)->QueueInkPresenterWorkItem(std::move(workItem));
    }
}

void RunOnInkHostThreadSync(winrt::weak_ref<muxc::InkPresenter> const& presenter, std::function<void(inking::InkPresenter const&)> workItem, bool propagateException, bool pumpMessages)
{
    if (auto strong = presenter.get())
    {
        winrt::get_self<::InkPresenter>(strong)->RunInkPresenterWorkItemSync(std::move(workItem), propagateException, pumpMessages);
    }
}

// ---------------------------------------------------------------------------
// InkStrokeContainer
// ---------------------------------------------------------------------------

InkStrokeContainer::InkStrokeContainer(muxc::InkPresenter const& owner)
{
    m_owner = owner;
}

void InkStrokeContainer::Clear()
{
    // Fire-and-forget: clearing does not need a result on the UI thread.
    RunOnInkHostThread(m_owner,
        [](inking::InkPresenter const& presenter)
        {
            presenter.StrokeContainer().Clear();
        });
}

winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> InkStrokeContainer::GetStrokes()
{
    // InkStroke is agile, so the strokes enumerated on the ink thread can be handed back
    // to the UI thread. RunOnInkHostThreadSync blocks until the ink thread has populated
    // the snapshot, so capturing it by reference is safe.
    std::vector<inking::InkStroke> snapshot;

    RunOnInkHostThreadSync(m_owner,
        [&snapshot](inking::InkPresenter const& presenter)
        {
            for (auto const& stroke : presenter.StrokeContainer().GetStrokes())
            {
                snapshot.push_back(stroke);
            }
        },
        /* propagateException */ false, /* pumpMessages */ false);

    return winrt::single_threaded_vector<inking::InkStroke>(std::move(snapshot)).GetView();
}

void InkStrokeContainer::AddStroke(inking::InkStroke const& stroke)
{
    // InkStroke is agile, so it can be captured into the ink-thread work item.
    RunOnInkHostThread(m_owner,
        [stroke](inking::InkPresenter const& presenter)
        {
            presenter.StrokeContainer().AddStroke(stroke);
        });
}

void InkStrokeContainer::AddStrokes(winrt::Windows::Foundation::Collections::IIterable<inking::InkStroke> const& strokes)
{
    // InkStroke is agile; build an agile vector on the UI thread (single_threaded_vector is
    // agile and copyable, so it can be captured into the fire-and-forget work item; the
    // incoming IIterable itself may not be agile).
    auto agileStrokes = winrt::single_threaded_vector<inking::InkStroke>();
    if (strokes)
    {
        for (auto const& stroke : strokes) { agileStrokes.Append(stroke); }
    }

    RunOnInkHostThread(m_owner,
        [agileStrokes](inking::InkPresenter const& presenter)
        {
            presenter.StrokeContainer().AddStrokes(agileStrokes);
        });
}

winrt::Windows::Foundation::IAsyncAction InkStrokeContainer::SaveAsync(winrt::Windows::Storage::Streams::IOutputStream outputStream)
{
    auto lifetime = get_strong();

    // Move off the UI thread so the synchronous ink-thread wait never blocks it.
    co_await winrt::resume_background();

    RunOnInkHostThreadSync(m_owner,
        [outputStream](inking::InkPresenter const& presenter)
        {
            // Run the OS async to completion on the ink thread that owns the container.
            presenter.StrokeContainer().SaveAsync(outputStream).get();
        },
        /* propagateException */ true, /* pumpMessages */ false);
}

winrt::Windows::Foundation::IAsyncAction InkStrokeContainer::SaveAsync(winrt::Windows::Storage::Streams::IOutputStream outputStream, inking::InkPersistenceFormat inkPersistenceFormat)
{
    auto lifetime = get_strong();

    // Move off the UI thread so the synchronous ink-thread wait never blocks it.
    co_await winrt::resume_background();

    RunOnInkHostThreadSync(m_owner,
        [outputStream, inkPersistenceFormat](inking::InkPresenter const& presenter)
        {
            // Run the OS async to completion on the ink thread that owns the container.
            presenter.StrokeContainer().SaveAsync(outputStream, inkPersistenceFormat).get();
        },
        /* propagateException */ true, /* pumpMessages */ false);
}

winrt::Windows::Foundation::IAsyncAction InkStrokeContainer::LoadAsync(winrt::Windows::Storage::Streams::IInputStream inputStream)
{
    auto lifetime = get_strong();

    co_await winrt::resume_background();

    RunOnInkHostThreadSync(m_owner,
        [inputStream](inking::InkPresenter const& presenter)
        {
            presenter.StrokeContainer().LoadAsync(inputStream).get();
        },
        /* propagateException */ true, /* pumpMessages */ false);
}

// Selection / query members. Each marshals a single call to the OS container on the ink
// thread and blocks for the result (InkStroke and the value-type Rect/Point are agile, so
// they cross the boundary safely). Clipboard copy/paste marshals to the ink thread too (the
// container is thread-affine); the OS presenter's ink thread is the apartment that services
// the ISF clipboard format, so the calls run there like every other container operation.
inking::InkStroke InkStrokeContainer::GetStrokeById(uint32_t id)
{
    inking::InkStroke result{ nullptr };
    RunOnInkHostThreadSync(m_owner,
        [&result, id](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().GetStrokeById(id);
        },
        /* propagateException */ false, /* pumpMessages */ false);
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::DeleteSelected()
{
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().DeleteSelected();
        },
        /* propagateException */ false, /* pumpMessages */ false);
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::MoveSelected(winrt::Windows::Foundation::Point const& translation)
{
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result, translation](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().MoveSelected(translation);
        },
        /* propagateException */ false, /* pumpMessages */ false);
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::SelectWithLine(winrt::Windows::Foundation::Point const& from, winrt::Windows::Foundation::Point const& to)
{
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result, from, to](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().SelectWithLine(from, to);
        },
        /* propagateException */ false, /* pumpMessages */ false);
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::SelectWithPolyLine(winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::Point> const& points)
{
    // Point is an agile value type; snapshot on the UI thread and rebuild an agile vector on
    // the ink thread (the IIterable itself may not be agile).
    std::vector<winrt::Windows::Foundation::Point> snapshot;
    if (points)
    {
        for (auto const& p : points) { snapshot.push_back(p); }
    }

    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result, &snapshot](inking::InkPresenter const& presenter)
        {
            auto vec = winrt::single_threaded_vector<winrt::Windows::Foundation::Point>();
            for (auto const& p : snapshot) { vec.Append(p); }
            result = presenter.StrokeContainer().SelectWithPolyLine(vec);
        },
        /* propagateException */ false, /* pumpMessages */ false);
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::BoundingRect()
{
    // Value-type Rect crosses the thread boundary safely; block for the result.
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().BoundingRect();
        },
        /* propagateException */ false, /* pumpMessages */ false);
    return result;
}

// Clipboard members. The OS container is thread-affine, so these run on the ink thread like
// the other container operations. Copy/Paste propagate failures so the app sees the HRESULT
// (e.g. an empty selection on copy, or an incompatible clipboard payload on paste) instead of
// a silent no-op; the returned Rect from paste is the invalidated region.
void InkStrokeContainer::CopySelectedToClipboard()
{
    RunOnInkHostThreadSync(m_owner,
        [](inking::InkPresenter const& presenter)
        {
            presenter.StrokeContainer().CopySelectedToClipboard();
        },
        /* propagateException */ true, /* pumpMessages */ true);
}

winrt::Windows::Foundation::Rect InkStrokeContainer::PasteFromClipboard(winrt::Windows::Foundation::Point const& position)
{
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result, position](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().PasteFromClipboard(position);
        },
        /* propagateException */ true, /* pumpMessages */ true);
    return result;
}

bool InkStrokeContainer::CanPasteFromClipboard()
{
    // Query: best-effort, so a failure just reports "cannot paste" rather than throwing.
    bool result = false;
    RunOnInkHostThreadSync(m_owner,
        [&result](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().CanPasteFromClipboard();
        },
        /* propagateException */ false, /* pumpMessages */ true);
    return result;
}

// ---------------------------------------------------------------------------
// InkInputProcessingConfiguration
// ---------------------------------------------------------------------------

InkInputProcessingConfiguration::InkInputProcessingConfiguration(muxc::InkPresenter const& owner)
{
    m_owner = owner;
}

muxc::InkInputProcessingMode InkInputProcessingConfiguration::Mode() const noexcept
{
    return m_mode;
}

void InkInputProcessingConfiguration::Mode(muxc::InkInputProcessingMode const& value)
{
    m_mode = value;

    // Map the MUXC mirror enum to the OS enum (identical values) for the ink thread.
    auto osMode = static_cast<inking::InkInputProcessingMode>(static_cast<int32_t>(value));
    RunOnInkHostThread(m_owner,
        [osMode](inking::InkPresenter const& presenter)
        {
            presenter.InputProcessingConfiguration().Mode(osMode);
        });
}

muxc::InkInputRightDragAction InkInputProcessingConfiguration::RightDragAction() const noexcept
{
    return m_rightDragAction;
}

void InkInputProcessingConfiguration::RightDragAction(muxc::InkInputRightDragAction const& value)
{
    m_rightDragAction = value;

    // Map the MUXC mirror enum to the OS enum (identical values) for the ink thread.
    auto osAction = static_cast<inking::InkInputRightDragAction>(static_cast<int32_t>(value));
    RunOnInkHostThread(m_owner,
        [osAction](inking::InkPresenter const& presenter)
        {
            presenter.InputProcessingConfiguration().RightDragAction(osAction);
        });
}

// ---------------------------------------------------------------------------
// InkInputConfiguration
// ---------------------------------------------------------------------------

InkInputConfiguration::InkInputConfiguration(muxc::InkPresenter const& owner)
{
    m_owner = owner;
}

bool InkInputConfiguration::IsPrimaryBarrelButtonInputEnabled() const noexcept
{
    return m_isPrimaryBarrelButtonInputEnabled;
}

void InkInputConfiguration::IsPrimaryBarrelButtonInputEnabled(bool value)
{
    m_isPrimaryBarrelButtonInputEnabled = value;

    RunOnInkHostThread(m_owner,
        [value](inking::InkPresenter const& presenter)
        {
            presenter.InputConfiguration().IsPrimaryBarrelButtonInputEnabled(value);
        });
}

bool InkInputConfiguration::IsEraserInputEnabled() const noexcept
{
    return m_isEraserInputEnabled;
}

void InkInputConfiguration::IsEraserInputEnabled(bool value)
{
    m_isEraserInputEnabled = value;

    RunOnInkHostThread(m_owner,
        [value](inking::InkPresenter const& presenter)
        {
            presenter.InputConfiguration().IsEraserInputEnabled(value);
        });
}

// ---------------------------------------------------------------------------
// InkPresenter
// ---------------------------------------------------------------------------

InkPresenter::InkPresenter(winrt::com_ptr<IInkDesktopHost> const& inkHost, winrt::Microsoft::UI::Dispatching::DispatcherQueue const& uiDispatcher)
    : m_inkHost(inkHost)
    , m_uiDispatcher(uiDispatcher)
{
    // A default InkDrawingAttributes is agile, so it is safe to cache here on the UI
    // thread and later hand into the ink-thread work item without marshaling issues.
    m_defaultDrawingAttributes = winrt::InkDrawingAttributes();

    // Create the input event proxies eagerly (stable instances) so InitializeOsPresenter can wire
    // the OS events to them from the ink thread without a lazy-create race. They hold no owner ref
    // and are agile, so they are safe to touch from either thread.
    m_strokeInput = winrt::make<::InkStrokeInput>();
    m_unprocessedInput = winrt::make<::InkUnprocessedInput>();
}

// Queue-only: submit the work item to the ink thread and return WITHOUT waiting for it to run. The
// fire-and-forget path for presenter mutations. The wrapper captures a strong ref to this proxy so
// the OS presenter stays alive until the item runs (e.g. a save-on-close after teardown), and reads
// m_osPresenter back on the ink thread (null until it has been created).
void InkPresenter::QueueInkPresenterWorkItem(std::function<void(inking::InkPresenter const&)> workItem)
{
    auto workItemWrapper = [workItem = std::move(workItem), strongThis = get_strong()]()
        {
            if (strongThis->m_osPresenter)
            {
                workItem(strongThis->m_osPresenter);
            }
        };

    winrt::check_hresult(m_inkHost->QueueWorkItem(winrt::make<GenericInkCallback>(std::move(workItemWrapper)).get()));
}

// Post the work item to the ink thread and block the calling thread until it runs. Used by the
// synchronous getters / Save / Load. If already on the ink thread, run inline to avoid a self
// deadlock. INVARIANT: a work item posted here must not synchronously call back to the UI thread
// (the wait is non-pumping); anything needing UI/app objects uses the async path (SaveAsync /
// LoadAsync resume_background() first, so the UI thread is never the one blocked).
void InkPresenter::RunInkPresenterWorkItemSync(std::function<void(inking::InkPresenter const&)> workItem, bool propagateException, bool pumpMessages)
{
    if (!m_inkHost)
    {
        return;
    }

    if (::GetCurrentThreadId() == m_inkThreadId)
    {
        try
        {
            if (m_osPresenter)
            {
                workItem(m_osPresenter);
            }
        }
        catch (...)
        {
            if (propagateException)
            {
                throw;
            }
        }
        return;
    }

    // Manual-reset event signaled by the ink thread when the work item has finished. Used instead of
    // std::future so the wait below can be a COM-pumping wait when pumpMessages is set.
    winrt::handle doneEvent{ ::CreateEventW(nullptr, /* bManualReset */ TRUE, /* bInitialState */ FALSE, nullptr) };
    winrt::check_bool(static_cast<bool>(doneEvent));
    std::exception_ptr workError;
    auto strongThis = get_strong();

    auto workItemWrapper = [&workItem, &doneEvent, &workError, strongThis]()
        {
            try
            {
                if (strongThis->m_osPresenter)
                {
                    workItem(strongThis->m_osPresenter);
                }
            }
            catch (...)
            {
                // Capture rather than swallow: SaveAsync/LoadAsync opt into re-propagation so a
                // failed save/load is reported to the app. Best-effort getters ignore this.
                workError = std::current_exception();
            }
            ::SetEvent(doneEvent.get());
        };

    winrt::check_hresult(m_inkHost->QueueWorkItem(winrt::make<GenericInkCallback>(workItemWrapper).get()));

    // Safe to capture workItem/doneEvent/workError by reference: we block here until the ink thread
    // has finished running workItemWrapper (it signals doneEvent last).
    if (pumpMessages)
    {
        // Clipboard path: the OS runs the OLE clipboard on the ink thread, which can SendMessage back
        // to the UI thread (the clipboard owner) while we wait here. A non-pumping wait would deadlock,
        // so pump COM calls - this services the incoming cross-apartment OLE call and lets the
        // ink-thread work finish. COWAIT_DISPATCH_CALLS limits reentrancy to COM (it does not pump
        // arbitrary input / window messages).
        HANDLE handle = doneEvent.get();
        DWORD signaledIndex = 0;
        winrt::check_hresult(::CoWaitForMultipleHandles(COWAIT_DISPATCH_CALLS, INFINITE, 1, &handle, &signaledIndex));
    }
    else
    {
        ::WaitForSingleObject(doneEvent.get(), INFINITE);
    }

    if (propagateException && workError)
    {
        std::rethrow_exception(workError);
    }
}

// Kicks off creation of the OS presenter on the ink host thread. Called once by InkCanvas right
// after construction (safe to take a self weak-ref now, unlike in the constructor).
void InkPresenter::Start()
{
    // Weak self-ref (typed as the projected type so get_self<::InkPresenter> resolves the impl) so
    // the OS-creation work does not keep this proxy alive on its own.
    winrt::weak_ref<muxc::InkPresenter> weakSelf{ *this };
    auto inkHost = m_inkHost;

    // Give the eagerly-created input mirrors their owner back-pointer now that a self weak-ref is
    // safe (never in the ctor). Powers InkStrokeInput/InkUnprocessedInput.InkPresenter (UWP parity).
    winrt::get_self<::InkStrokeInput>(m_strokeInput)->SetOwner(weakSelf);
    winrt::get_self<::InkUnprocessedInput>(m_unprocessedInput)->SetOwner(weakSelf);

    auto callback = winrt::make<GenericInkCallback>([weakSelf, inkHost]()
        {
            auto strong = weakSelf.get();
            if (!strong)
            {
                return;
            }
            auto self = winrt::get_self<::InkPresenter>(strong);

            // Record the ink thread id so RunInkPresenterWorkItemSync can detect a same-thread call.
            self->m_inkThreadId = ::GetCurrentThreadId();

            // Create the OS presenter on the ink thread and hand it to the proxy.
            winrt::com_ptr inkPresenterDesktop = winrt::capture<IInkPresenterDesktop>(
                inkHost,
                &IInkDesktopHost::CreateInkPresenter);
            self->InitializeOsPresenter(inkPresenterDesktop.as<inking::InkPresenter>());
        });
    winrt::check_hresult(m_inkHost->QueueWorkItem(callback.get()));
}

winrt::CoreInputDeviceTypes InkPresenter::InputDeviceTypes() const noexcept
{
    return m_inputDeviceTypes;
}

void InkPresenter::InputDeviceTypes(winrt::CoreInputDeviceTypes const& value)
{
    m_inputDeviceTypes = value;

    QueueInkPresenterWorkItem(
        [value](inking::InkPresenter const& presenter)
        {
            presenter.InputDeviceTypes(value);
        });
}

bool InkPresenter::IsInputEnabled() const noexcept
{
    return m_isInputEnabled;
}

void InkPresenter::IsInputEnabled(bool value)
{
    m_isInputEnabled = value;

    QueueInkPresenterWorkItem(
        [value](inking::InkPresenter const& presenter)
        {
            presenter.IsInputEnabled(value);
        });
}

void InkPresenter::UpdateDefaultDrawingAttributes(winrt::InkDrawingAttributes const& drawingAttributes)
{
    m_defaultDrawingAttributes = drawingAttributes;

    // InkDrawingAttributes is agile (verified), so it can cross into the ink thread.
    QueueInkPresenterWorkItem(
        [drawingAttributes](inking::InkPresenter const& presenter)
        {
            presenter.UpdateDefaultDrawingAttributes(drawingAttributes);
        });
}

winrt::InkDrawingAttributes InkPresenter::CopyDefaultDrawingAttributes()
{
    // Parity with UWP: return an independent copy from the OS presenter (InkDrawingAttributes is agile,
    // so it crosses back to the UI thread) so the caller can mutate it without touching the stored
    // default. Falls back to a fresh default if the ink thread has not created the OS presenter yet.
    winrt::InkDrawingAttributes result{ nullptr };
    RunInkPresenterWorkItemSync(
        [&result](inking::InkPresenter const& presenter)
        {
            result = presenter.CopyDefaultDrawingAttributes();
        },
        /* propagateException */ false, /* pumpMessages */ false);

    if (!result)
    {
        result = winrt::InkDrawingAttributes();
    }
    return result;
}

void InkPresenter::SetPredefinedConfiguration(inking::InkPresenterPredefinedConfiguration const& configuration)
{
    // Fire-and-forget: applies the OS-defined presenter configuration on the ink thread.
    QueueInkPresenterWorkItem(
        [configuration](inking::InkPresenter const& presenter)
        {
            presenter.SetPredefinedConfiguration(configuration);
        });
}

muxc::InkHighContrastAdjustment InkPresenter::GetHighContrastAdjustment() const noexcept
{
    return m_highContrastAdjustment;
}

void InkPresenter::SetHighContrastAdjustment(muxc::InkHighContrastAdjustment const& value)
{
    m_highContrastAdjustment = value;

    // Map the MUXC mirror enum to the OS enum (identical values) for the ink thread.
    auto osValue = static_cast<inking::InkHighContrastAdjustment>(static_cast<int32_t>(value));
    QueueInkPresenterWorkItem(
        [osValue](inking::InkPresenter const& presenter)
        {
            presenter.HighContrastAdjustment(osValue);
        });
}

muxc::InkStrokeContainer InkPresenter::StrokeContainer()
{
    if (!m_strokeContainer)
    {
        m_strokeContainer = winrt::make<InkStrokeContainer>(*this);
    }

    return m_strokeContainer;
}

muxc::InkInputProcessingConfiguration InkPresenter::InputProcessingConfiguration()
{
    if (!m_inputProcessingConfiguration)
    {
        m_inputProcessingConfiguration = winrt::make<InkInputProcessingConfiguration>(*this);
    }

    return m_inputProcessingConfiguration;
}

muxc::InkInputConfiguration InkPresenter::InputConfiguration()
{
    if (!m_inputConfiguration)
    {
        m_inputConfiguration = winrt::make<InkInputConfiguration>(*this);
    }

    return m_inputConfiguration;
}

muxc::InkStrokeInput InkPresenter::StrokeInput()
{
    return m_strokeInput;
}

muxc::InkUnprocessedInput InkPresenter::UnprocessedInput()
{
    return m_unprocessedInput;
}

muxc::InkSynchronizer InkPresenter::ActivateCustomDrying()
{
    // Snapshot the app-set input configuration on the UI thread (avoids a cross-thread read inside the
    // ink-thread work item); the rebuilt presenter restores it.
    auto mode = m_inputProcessingConfiguration ? m_inputProcessingConfiguration.Mode() : muxc::InkInputProcessingMode::Inking;
    auto rightDrag = m_inputProcessingConfiguration ? m_inputProcessingConfiguration.RightDragAction() : muxc::InkInputRightDragAction::LeaveUnprocessed;
    bool barrelButton = m_inputConfiguration ? m_inputConfiguration.IsPrimaryBarrelButtonInputEnabled() : true;
    bool eraserInput = m_inputConfiguration ? m_inputConfiguration.IsEraserInputEnabled() : true;

    // Create the projected synchronizer on the UI (calling) thread, where the app invokes it - a proxy
    // created on the ink thread crashes when called cross-apartment. Stable across repeated calls.
    if (!m_customDrySynchronizer)
    {
        m_customDrySynchronizer = winrt::make<InkSynchronizer>(winrt::weak_ref<muxc::InkPresenter>{ *this });
    }

    // Idempotent (UWP parity): the first call rebuilds + activates on the ink thread; repeated calls
    // return the same synchronizer.
    RunInkPresenterWorkItemSync(
        [this, mode, rightDrag, barrelButton, eraserInput](inking::InkPresenter const& startedPresenter)
        {
            if (!m_customDryActive)
            {
                RebuildOsPresenterForCustomDrying(startedPresenter, mode, rightDrag, barrelButton, eraserInput);
                m_customDryActive = true;
            }
        },
        /* propagateException */ true,
        /* pumpMessages */ false);

    return m_customDrySynchronizer;
}

void InkPresenter::RebuildOsPresenterForCustomDrying(inking::InkPresenter const& startedPresenter,
    muxc::InkInputProcessingMode mode, muxc::InkInputRightDragAction rightDrag, bool barrelButton, bool eraserInput)
{
    // Carry the size across, then stop the started presenter from presenting into the shared ink visual.
    float width = 0;
    float height = 0;
    startedPresenter.as<IInkPresenterDesktop>()->GetSize(&width, &height);
    startedPresenter.as<IInkPresenterDesktop>()->SetRootVisual(nullptr, nullptr);

    // SetRootVisual gets the composition DEVICE (not nullptr): custom drying needs it for the wet->dry
    // handoff; a null device makes BeginDry fail with E_UNEXPECTED. Size is still 0, so activation is legal.
    auto customDryDesktop = winrt::capture<IInkPresenterDesktop>(m_inkHost, &IInkDesktopHost::CreateInkPresenter);
    auto customDryPresenter = customDryDesktop.as<inking::InkPresenter>();
    if (m_rootVisual)
    {
        winrt::check_hresult(customDryDesktop->SetRootVisual(m_rootVisual.get(), m_compositionDevice.get()));
    }
    if (m_compositionDevice)
    {
        auto commitHandler = winrt::make_self<InkCommitRequestHandler>(m_compositionDevice);
        winrt::check_hresult(customDryDesktop->SetCommitRequestHandler(commitHandler.as<IInkCommitRequestHandler>().get()));
    }

    // Inject the OS synchronizer (from ActivateCustomDrying) into the UI-thread-created proxy.
    // m_customDrySync lets the in-context StrokesCollected callback drive its ink-thread BeginDry.
    auto syncImpl = winrt::get_self<::InkSynchronizer>(m_customDrySynchronizer);
    syncImpl->AdoptOsSynchronizer(customDryPresenter.ActivateCustomDrying());
    m_customDrySync = syncImpl;

    // Adopt the fresh presenter and restore the config the app had set (a fresh presenter = OS defaults).
    InitializeOsPresenter(customDryPresenter);
    customDryPresenter.InputDeviceTypes(m_inputDeviceTypes);
    customDryPresenter.IsInputEnabled(m_isInputEnabled);
    if (m_defaultDrawingAttributes)
    {
        customDryPresenter.UpdateDefaultDrawingAttributes(m_defaultDrawingAttributes);
    }
    customDryPresenter.HighContrastAdjustment(
        static_cast<inking::InkHighContrastAdjustment>(static_cast<int32_t>(m_highContrastAdjustment)));
    customDryPresenter.InputProcessingConfiguration().Mode(
        static_cast<inking::InkInputProcessingMode>(static_cast<int32_t>(mode)));
    customDryPresenter.InputProcessingConfiguration().RightDragAction(
        static_cast<inking::InkInputRightDragAction>(static_cast<int32_t>(rightDrag)));
    customDryPresenter.InputConfiguration().IsPrimaryBarrelButtonInputEnabled(barrelButton);
    customDryPresenter.InputConfiguration().IsEraserInputEnabled(eraserInput);

    // Start it (input begins) now that custom drying is engaged.
    winrt::check_hresult(customDryDesktop->SetSize(width, height));
    if (m_compositionDevice)
    {
        winrt::check_hresult(m_compositionDevice->Commit());
    }
}

void InkPresenter::SetCompositionDevice(winrt::com_ptr<IDCompositionDevice> const& device)
{
    // Store the shared composition device on the ink thread; the custom-drying path commits it after
    // clearing the wet container so the removed strokes leave the screen.
    QueueInkPresenterWorkItem([this, device](inking::InkPresenter const&) { m_compositionDevice = device; });
}

void InkPresenter::ReleaseCompositionDevice()
{
    QueueInkPresenterWorkItem([this](inking::InkPresenter const&) { m_compositionDevice = nullptr; });
}

// -- InkSynchronizer mirror -----------------------------------------------------------------------
// Owns the OS InkSynchronizer (adopted on the ink thread from the presenter's OS ActivateCustomDrying)
// and the dry-transaction state, and marshals its BeginDry/EndDry onto the ink thread through the owning
// InkPresenter proxy's work queue. Returns empty / no-ops once the owner has been torn down.

void InkSynchronizer::BeginDryInContext()
{
    // Runs on the ink thread inside the OS StrokesCollected callback - the only point BeginDry is valid.
    // A rapid burst can outrun the app's EndDry, and BeginDry is invalid while a dry is still open, so
    // close any still-open one first. Then BeginDry hands back the just-committed strokes and we HOLD the
    // wet layer up (no EndDry here) so the stroke stays on screen until the app has painted its dry ink
    // and calls EndDry. Append (don't overwrite) so a burst that outruns the app's BeginDry is not lost.
    // Capture any failure so it surfaces at the app's BeginDry rather than being swallowed.
    if (!m_osSynchronizer)
    {
        return;
    }
    m_lastDryHr = S_OK;
    try
    {
        if (m_dryInProgress)
        {
            m_osSynchronizer.EndDry();
            m_dryInProgress = false;
        }
        auto dry = m_osSynchronizer.BeginDry();
        m_dryInProgress = true;
        if (dry && dry.Size())
        {
            std::vector<inking::InkStroke> drySnapshot(dry.Size(), nullptr);
            dry.GetMany(0, drySnapshot);
            m_pendingDryStrokes.insert(m_pendingDryStrokes.end(), drySnapshot.begin(), drySnapshot.end());
        }
    }
    catch (winrt::hresult_error const& e)
    {
        m_lastDryHr = e.code();
    }
    catch (...)
    {
        m_lastDryHr = E_FAIL;
    }
}

winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> InkSynchronizer::BeginDry()
{
    // The in-context BeginDry already ran on the ink thread (inside the OS StrokesCollected callback) and
    // the wet layer is being held up. Drain the strokes it captured on the ink thread, then hand them
    // back; the app paints them as dry ink and then calls EndDry to release the wet layer.
    std::vector<inking::InkStroke> drained;
    RunOnInkHostThreadSync(m_owner,
        [this, &drained](inking::InkPresenter const&)
        {
            // Propagate a genuine in-context BeginDry failure to the app's BeginDry (UWP parity) rather
            // than returning an empty result that hides it.
            if (FAILED(m_lastDryHr))
            {
                winrt::throw_hresult(m_lastDryHr);
            }
            drained = std::move(m_pendingDryStrokes);
            m_pendingDryStrokes.clear();
        },
        /* propagateException */ true,
        /* pumpMessages */ false);
    // Build the returned collection on the calling (UI) thread - matching the StrokesCollected path and
    // avoiding an ink-thread-affine object being handed to the app.
    return winrt::single_threaded_vector<inking::InkStroke>(std::move(drained)).GetView();
}

void InkSynchronizer::EndDry()
{
    // Release the held wet layer now that the app has painted its dry ink; the commit-request handler
    // presents that removal. The wet was held since the in-context BeginDry, so the stroke stayed
    // continuously visible. No-op if a rapid burst already closed this dry in the StrokesCollected handler.
    RunOnInkHostThreadSync(m_owner,
        [this](inking::InkPresenter const&)
        {
            if (m_dryInProgress && m_osSynchronizer)
            {
                m_osSynchronizer.EndDry();
                m_dryInProgress = false;
            }
        },
        /* propagateException */ true,
        /* pumpMessages */ false);
}

winrt::event_token InkPresenter::StrokesCollected(winrt::Windows::Foundation::TypedEventHandler<muxc::InkPresenter, muxc::InkStrokesCollectedEventArgs> const& handler)
{
    return m_strokesCollectedEvent.add(handler);
}

void InkPresenter::StrokesCollected(winrt::event_token const& token)
{
    m_strokesCollectedEvent.remove(token);
}

winrt::event_token InkPresenter::StrokesErased(winrt::Windows::Foundation::TypedEventHandler<muxc::InkPresenter, muxc::InkStrokesErasedEventArgs> const& handler)
{
    return m_strokesErasedEvent.add(handler);
}

void InkPresenter::StrokesErased(winrt::event_token const& token)
{
    m_strokesErasedEvent.remove(token);
}

// Runs on the ink thread as soon as an OS presenter is created: once from Start's work item, and again
// from ActivateCustomDrying when it swaps in a freshly-created presenter to activate custom drying. The
// proxy adopts the given OS presenter and (re)establishes everything about it from here on: initial
// configuration and the stroke-event forwarding path. Safe to run more than once - it always targets
// the passed-in presenter, and the previously-adopted one is released when m_osPresenter is reassigned.
void InkPresenter::InitializeOsPresenter(inking::InkPresenter const& osPresenter)
{
    m_osPresenter = osPresenter;

    // Initial input devices. A local constant (not a cross-thread read of the UI-thread cache) so
    // there is no race; any app override queued via InputDeviceTypes() runs after this on the ink
    // thread and wins.
    osPresenter.InputDeviceTypes(
        winrt::CoreInputDeviceTypes::Mouse | winrt::CoreInputDeviceTypes::Pen | winrt::CoreInputDeviceTypes::Touch);

    // Weak ref to this proxy (as the projected type) for the UI-thread re-raise. A strong ref would
    // form a cycle (OS presenter -> handler -> proxy -> OS presenter). Typed as muxc::InkPresenter so
    // get_self<::InkPresenter> below resolves the implementation.
    winrt::weak_ref<muxc::InkPresenter> weakSelf{ *this };
    auto uiDispatcher = m_uiDispatcher;

    // Single UI-thread marshaling helper for every OS ink-thread event below: re-locks a strong
    // self on the UI thread and runs 'fn' with it (no-op if the dispatcher is gone or the proxy was
    // destroyed). Replaces the per-event weakSelf/TryEnqueue/get() boilerplate.
    auto marshalToUi = [weakSelf, uiDispatcher](auto&& fn)
    {
        if (!uiDispatcher)
        {
            return;
        }
        uiDispatcher.TryEnqueue([weakSelf, fn = std::forward<decltype(fn)>(fn)]() mutable
            {
                if (auto strong = weakSelf.get())
                {
                    fn(winrt::get_self<::InkPresenter>(strong));
                }
            });
    };

    // Subscribe to the OS presenter's stroke events on the ink thread. InkStroke is agile, so we
    // snapshot the strokes here, marshal to the UI thread, and re-raise our own events there.
    // Handlers are attached eagerly and are harmless when the app never subscribes to our events.
    osPresenter.StrokesCollected(
        [marshalToUi, weakSelf](inking::InkPresenter const&, inking::InkStrokesCollectedEventArgs const& args)
        {
            auto strokes = args.Strokes();
            std::vector<inking::InkStroke> snapshot(strokes.Size(), nullptr);
            strokes.GetMany(0, snapshot);

            // Custom drying: BeginDry is only valid synchronously inside THIS OS callback, on the ink
            // thread (the app's StrokesCollected handler runs later on the UI thread - too late). Drive it
            // through the InkSynchronizer mirror, which owns the OS synchronizer and the dry-transaction
            // state: it holds the wet layer up and captures the strokes for the app's BeginDry to drain.
            if (auto strong = weakSelf.get())
            {
                auto self = winrt::get_self<::InkPresenter>(strong);
                if (self->m_customDrySync)
                {
                    self->m_customDrySync->BeginDryInContext();
                }
            }

            marshalToUi([snapshot = std::move(snapshot)](::InkPresenter* self) mutable
                {
                    self->RaiseStrokesCollected(
                        winrt::single_threaded_vector<inking::InkStroke>(std::move(snapshot)).GetView());
                });
        });

    osPresenter.StrokesErased(
        [marshalToUi](inking::InkPresenter const&, inking::InkStrokesErasedEventArgs const& args)
        {
            auto strokes = args.Strokes();
            std::vector<inking::InkStroke> snapshot(strokes.Size(), nullptr);
            strokes.GetMany(0, snapshot);

            marshalToUi([snapshot = std::move(snapshot)](::InkPresenter* self) mutable
                {
                    self->RaiseStrokesErased(
                        winrt::single_threaded_vector<inking::InkStroke>(std::move(snapshot)).GetView());
                });
        });

    // Subscribe to the OS presenter's raw pointer-input events (InkStrokeInput / InkUnprocessedInput)
    // and re-raise them on the UI thread through our mirror proxies. The OS PointerEventArgs is
    // passed through unchanged. NOTE: because delivery is marshaled to the UI thread, the
    // args.Handled round-trip back to the OS is best-effort (the OS has already proceeded).
    auto osStrokeInput = osPresenter.StrokeInput();
    auto marshalStroke = [marshalToUi](void (::InkStrokeInput::* raise)(winrt::Windows::UI::Core::PointerEventArgs const&))
    {
        return [marshalToUi, raise](inking::InkStrokeInput const&, winrt::Windows::UI::Core::PointerEventArgs const& args)
        {
            // PointerEventArgs is not agile; capture an agile reference so the UI-thread read
            // marshals through a proxy instead of touching the ink-thread object directly (which
            // would risk RPC_E_WRONG_THREAD or a recycled args).
            winrt::agile_ref<winrt::Windows::UI::Core::PointerEventArgs> agileArgs{ args };
            marshalToUi([agileArgs, raise](::InkPresenter* self)
                {
                    if (self->m_strokeInput)
                    {
                        (winrt::get_self<::InkStrokeInput>(self->m_strokeInput)->*raise)(agileArgs.get());
                    }
                });
        };
    };
    osStrokeInput.StrokeStarted(marshalStroke(&::InkStrokeInput::RaiseStrokeStarted));
    osStrokeInput.StrokeContinued(marshalStroke(&::InkStrokeInput::RaiseStrokeContinued));
    osStrokeInput.StrokeEnded(marshalStroke(&::InkStrokeInput::RaiseStrokeEnded));
    osStrokeInput.StrokeCanceled(marshalStroke(&::InkStrokeInput::RaiseStrokeCanceled));

    auto osUnprocessed = osPresenter.UnprocessedInput();
    auto marshalUnproc = [marshalToUi](void (::InkUnprocessedInput::* raise)(winrt::Windows::UI::Core::PointerEventArgs const&))
    {
        return [marshalToUi, raise](inking::InkUnprocessedInput const&, winrt::Windows::UI::Core::PointerEventArgs const& args)
        {
            // PointerEventArgs is not agile; capture an agile reference so the UI-thread read
            // marshals through a proxy instead of touching the ink-thread object directly (which
            // would risk RPC_E_WRONG_THREAD or a recycled args).
            winrt::agile_ref<winrt::Windows::UI::Core::PointerEventArgs> agileArgs{ args };
            marshalToUi([agileArgs, raise](::InkPresenter* self)
                {
                    if (self->m_unprocessedInput)
                    {
                        (winrt::get_self<::InkUnprocessedInput>(self->m_unprocessedInput)->*raise)(agileArgs.get());
                    }
                });
        };
    };
    osUnprocessed.PointerEntered(marshalUnproc(&::InkUnprocessedInput::RaisePointerEntered));
    osUnprocessed.PointerHovered(marshalUnproc(&::InkUnprocessedInput::RaisePointerHovered));
    osUnprocessed.PointerExited(marshalUnproc(&::InkUnprocessedInput::RaisePointerExited));
    osUnprocessed.PointerPressed(marshalUnproc(&::InkUnprocessedInput::RaisePointerPressed));
    osUnprocessed.PointerMoved(marshalUnproc(&::InkUnprocessedInput::RaisePointerMoved));
    osUnprocessed.PointerReleased(marshalUnproc(&::InkUnprocessedInput::RaisePointerReleased));
    osUnprocessed.PointerLost(marshalUnproc(&::InkUnprocessedInput::RaisePointerLost));
}

void InkPresenter::RaiseStrokesCollected(winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> const& strokes)
{
    m_strokesCollectedEvent(*this, *winrt::make_self<InkStrokesCollectedEventArgs>(strokes));
}

void InkPresenter::RaiseStrokesErased(winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> const& strokes)
{
    m_strokesErasedEvent(*this, *winrt::make_self<InkStrokesErasedEventArgs>(strokes));
}

// Shows/hides the ruler stencil. The InkPresenterRuler is thread-affine to the OS presenter,
// so it is created on first enable and toggled entirely on the ink thread. m_inkRuler is only
// ever touched inside these serialized ink-thread work items.
void InkPresenter::SetRulerEnabled(bool enabled)
{
    // Capture a strong ref to this proxy (rather than raw 'this') so the m_inkRuler member
    // stays alive for the duration of the async ink-thread work item.
    QueueInkPresenterWorkItem(
        [strongThis = get_strong(), enabled](inking::InkPresenter const& presenter)
        {
            if (enabled && !strongThis->m_inkRuler)
            {
                strongThis->m_inkRuler = inking::InkPresenterRuler(presenter);
            }
            if (strongThis->m_inkRuler)
            {
                strongThis->m_inkRuler.IsVisible(enabled);
            }
        });
}

// Shows/hides the protractor stencil. Mirrors SetRulerEnabled: the InkPresenterProtractor is
// thread-affine to the OS presenter, so it is created on first enable and toggled entirely on
// the ink thread. m_inkProtractor is only ever touched inside these serialized ink-thread work
// items.
void InkPresenter::SetProtractorEnabled(bool enabled)
{
    QueueInkPresenterWorkItem(
        [strongThis = get_strong(), enabled](inking::InkPresenter const& presenter)
        {
            if (enabled && !strongThis->m_inkProtractor)
            {
                strongThis->m_inkProtractor = inking::InkPresenterProtractor(presenter);
            }
            if (strongThis->m_inkProtractor)
            {
                strongThis->m_inkProtractor.IsVisible(enabled);
            }
        });
}
