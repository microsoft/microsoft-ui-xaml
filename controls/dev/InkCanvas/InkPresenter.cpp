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

void RunOnInkHostThreadSync(winrt::weak_ref<muxc::InkPresenter> const& presenter, std::function<void(inking::InkPresenter const&)> workItem, bool propagateException)
{
    if (auto strong = presenter.get())
    {
        winrt::get_self<::InkPresenter>(strong)->RunInkPresenterWorkItemSync(std::move(workItem), propagateException);
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
        });

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
        /* propagateException */ true);
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
        /* propagateException */ true);
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
        /* propagateException */ true);
}

// Selection / query members. Each marshals a single call to the OS container on the ink
// thread and blocks for the result (InkStroke and the value-type Rect/Point are agile, so
// they cross the boundary safely). Clipboard copy/paste is intentionally not exposed yet
// (OLE clipboard has apartment requirements the ink thread does not satisfy).
inking::InkStroke InkStrokeContainer::GetStrokeById(uint32_t id)
{
    inking::InkStroke result{ nullptr };
    RunOnInkHostThreadSync(m_owner,
        [&result, id](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().GetStrokeById(id);
        });
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::DeleteSelected()
{
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().DeleteSelected();
        });
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::MoveSelected(winrt::Windows::Foundation::Point const& translation)
{
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result, translation](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().MoveSelected(translation);
        });
    return result;
}

winrt::Windows::Foundation::Rect InkStrokeContainer::SelectWithLine(winrt::Windows::Foundation::Point const& from, winrt::Windows::Foundation::Point const& to)
{
    winrt::Windows::Foundation::Rect result{};
    RunOnInkHostThreadSync(m_owner,
        [&result, from, to](inking::InkPresenter const& presenter)
        {
            result = presenter.StrokeContainer().SelectWithLine(from, to);
        });
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
        });
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
        });
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
void InkPresenter::RunInkPresenterWorkItemSync(std::function<void(inking::InkPresenter const&)> workItem, bool propagateException)
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

    std::promise<void> done;
    std::future<void> ready = done.get_future();
    std::exception_ptr workError;
    auto strongThis = get_strong();

    auto workItemWrapper = [&workItem, &done, &workError, strongThis]()
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
            done.set_value();
        };

    winrt::check_hresult(m_inkHost->QueueWorkItem(winrt::make<GenericInkCallback>(workItemWrapper).get()));

    // Safe to capture workItem/done/workError by reference: we block here until the ink thread
    // has finished running workItemWrapper.
    ready.wait();

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
    if (!m_defaultDrawingAttributes)
    {
        m_defaultDrawingAttributes = winrt::InkDrawingAttributes();
    }

    return m_defaultDrawingAttributes;
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

// Runs ONCE on the ink thread (from Start's work item) as soon as the OS presenter is created. The
// proxy takes ownership of the OS presenter and owns everything about it from here on: initial
// configuration and the stroke-event forwarding path.
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

    // Subscribe to the OS presenter's stroke events on the ink thread. InkStroke is agile, so we
    // snapshot the strokes here, marshal to the UI thread, and re-raise our own events there.
    // Handlers are attached eagerly and are harmless when the app never subscribes to our events.
    osPresenter.StrokesCollected(
        [weakSelf, uiDispatcher](inking::InkPresenter const&, inking::InkStrokesCollectedEventArgs const& args)
        {
            std::vector<inking::InkStroke> snapshot;
            for (auto const& stroke : args.Strokes())
            {
                snapshot.push_back(stroke);
            }

            if (uiDispatcher)
            {
                uiDispatcher.TryEnqueue([weakSelf, snapshot = std::move(snapshot)]() mutable
                    {
                        if (auto strong = weakSelf.get())
                        {
                            winrt::get_self<::InkPresenter>(strong)->RaiseStrokesCollected(
                                winrt::single_threaded_vector<inking::InkStroke>(std::move(snapshot)).GetView());
                        }
                    });
            }
        });

    osPresenter.StrokesErased(
        [weakSelf, uiDispatcher](inking::InkPresenter const&, inking::InkStrokesErasedEventArgs const& args)
        {
            std::vector<inking::InkStroke> snapshot;
            for (auto const& stroke : args.Strokes())
            {
                snapshot.push_back(stroke);
            }

            if (uiDispatcher)
            {
                uiDispatcher.TryEnqueue([weakSelf, snapshot = std::move(snapshot)]() mutable
                    {
                        if (auto strong = weakSelf.get())
                        {
                            winrt::get_self<::InkPresenter>(strong)->RaiseStrokesErased(
                                winrt::single_threaded_vector<inking::InkStroke>(std::move(snapshot)).GetView());
                        }
                    });
            }
        });
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
