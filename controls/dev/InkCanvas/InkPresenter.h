// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "InkStrokesCollectedEventArgs.g.h"
#include "InkStrokesErasedEventArgs.g.h"
#include "InkStrokeContainer.g.h"
#include "InkInputProcessingConfiguration.g.h"
#include "InkInputConfiguration.g.h"
#include "InkPresenter.g.h"
#include <windows.ui.input.inking.h>
#include <inkpresenterdesktop.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <functional>
#include <vector>

// The OS Windows.UI.Input.Inking types and the MUXC types are both folded into winrt::,
// and several names collide (InkPresenter, InkStrokesCollectedEventArgs,
// InkStrokesErasedEventArgs exist in both). Alias the two namespaces so every use is
// unambiguous: 'inking' for the OS types, 'muxc' for our projected types.
namespace inking = winrt::Windows::UI::Input::Inking;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

// Marshals a work item onto the ink host thread through the owning InkPresenter proxy, passing the
// OS presenter to the lambda. No-op if the proxy has already been destroyed. The child proxies
// (InkStrokeContainer / InkInputProcessingConfiguration / InkInputConfiguration) call these instead
// of reaching into the proxy's work queue directly; the queue lives on the InkPresenter proxy, which
// is the sole owner of the thread-affine OS presenter and the ink host reference.
void RunOnInkHostThread(winrt::weak_ref<muxc::InkPresenter> const& presenter, std::function<void(inking::InkPresenter const&)> workItem);
void RunOnInkHostThreadSync(winrt::weak_ref<muxc::InkPresenter> const& presenter, std::function<void(inking::InkPresenter const&)> workItem, bool propagateException = false);

// Snapshot of the InkStroke objects handed to InkPresenter.StrokesCollected. InkStroke is
// agile, so the strokes are copied on the ink thread and this args object is re-raised on
// the UI thread.
class InkStrokesCollectedEventArgs :
    public winrt::implementation::InkStrokesCollectedEventArgsT<InkStrokesCollectedEventArgs>
{
public:
    InkStrokesCollectedEventArgs(winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> const& strokes)
        : m_strokes(strokes) {}

    winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> Strokes() const noexcept { return m_strokes; }

private:
    winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> m_strokes{ nullptr };
};

// Snapshot of the InkStroke objects handed to InkPresenter.StrokesErased.
class InkStrokesErasedEventArgs :
    public winrt::implementation::InkStrokesErasedEventArgsT<InkStrokesErasedEventArgs>
{
public:
    InkStrokesErasedEventArgs(winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> const& strokes)
        : m_strokes(strokes) {}

    winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> Strokes() const noexcept { return m_strokes; }

private:
    winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> m_strokes{ nullptr };
};

// Manipulable subset of Windows.UI.Input.Inking.InkStrokeContainer. The OS container is
// thread-affine (does not QI IAgileObject), so each operation marshals to the ink thread
// through the owning InkPresenter proxy. InkStroke is agile, so strokes returned by GetStrokes()
// are safe to hand back to the caller.
class InkStrokeContainer :
    public winrt::implementation::InkStrokeContainerT<InkStrokeContainer>
{
public:
    InkStrokeContainer(muxc::InkPresenter const& owner);

    void Clear();
    winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> GetStrokes();
    void AddStroke(inking::InkStroke const& stroke);
    void AddStrokes(winrt::Windows::Foundation::Collections::IIterable<inking::InkStroke> const& strokes);
    winrt::Windows::Foundation::IAsyncAction SaveAsync(winrt::Windows::Storage::Streams::IOutputStream outputStream);
    winrt::Windows::Foundation::IAsyncAction SaveAsync(winrt::Windows::Storage::Streams::IOutputStream outputStream, inking::InkPersistenceFormat inkPersistenceFormat);
    winrt::Windows::Foundation::IAsyncAction LoadAsync(winrt::Windows::Storage::Streams::IInputStream inputStream);
    inking::InkStroke GetStrokeById(uint32_t id);
    winrt::Windows::Foundation::Rect DeleteSelected();
    winrt::Windows::Foundation::Rect MoveSelected(winrt::Windows::Foundation::Point const& translation);
    winrt::Windows::Foundation::Rect SelectWithLine(winrt::Windows::Foundation::Point const& from, winrt::Windows::Foundation::Point const& to);
    winrt::Windows::Foundation::Rect SelectWithPolyLine(winrt::Windows::Foundation::Collections::IIterable<winrt::Windows::Foundation::Point> const& points);
    winrt::Windows::Foundation::Rect BoundingRect();

private:
    winrt::weak_ref<muxc::InkPresenter> m_owner{ nullptr };
};

// Manipulable subset of Windows.UI.Input.Inking.InkInputProcessingConfiguration. The mode
// is cached on the UI thread and marshaled to the ink thread on set.
class InkInputProcessingConfiguration :
    public winrt::implementation::InkInputProcessingConfigurationT<InkInputProcessingConfiguration>
{
public:
    InkInputProcessingConfiguration(muxc::InkPresenter const& owner);

    muxc::InkInputProcessingMode Mode() const noexcept;
    void Mode(muxc::InkInputProcessingMode const& value);

    muxc::InkInputRightDragAction RightDragAction() const noexcept;
    void RightDragAction(muxc::InkInputRightDragAction const& value);

private:
    winrt::weak_ref<muxc::InkPresenter> m_owner{ nullptr };
    muxc::InkInputProcessingMode m_mode{ muxc::InkInputProcessingMode::Inking };
    muxc::InkInputRightDragAction m_rightDragAction{ muxc::InkInputRightDragAction::LeaveUnprocessed };
};

// Manipulable subset of Windows.UI.Input.Inking.InkInputConfiguration. The flags are cached
// on the UI thread and marshaled to the ink thread on set.
class InkInputConfiguration :
    public winrt::implementation::InkInputConfigurationT<InkInputConfiguration>
{
public:
    InkInputConfiguration(muxc::InkPresenter const& owner);

    bool IsPrimaryBarrelButtonInputEnabled() const noexcept;
    void IsPrimaryBarrelButtonInputEnabled(bool value);

    bool IsEraserInputEnabled() const noexcept;
    void IsEraserInputEnabled(bool value);

private:
    winrt::weak_ref<muxc::InkPresenter> m_owner{ nullptr };
    bool m_isPrimaryBarrelButtonInputEnabled{ true };
    bool m_isEraserInputEnabled{ true };
};

// Manipulable subset of Windows.UI.Input.Inking.InkPresenter. The OS presenter can only be created
// and called on the IInkDesktopHost ink thread, so this proxy is the single owner of it: it holds
// the ink host reference (handed in at construction by the owning InkCanvas), creates the OS
// presenter on the ink thread, and dispatches every operation there through its own work queue.
// InkCanvas reaches the OS presenter only by calling this proxy; nothing here reaches back into
// InkCanvas. Getters return the value cached on the UI thread so they never block.
class InkPresenter :
    public winrt::implementation::InkPresenterT<InkPresenter>
{
public:
    InkPresenter(winrt::com_ptr<IInkDesktopHost> const& inkHost, winrt::Microsoft::UI::Dispatching::DispatcherQueue const& uiDispatcher);

    winrt::CoreInputDeviceTypes InputDeviceTypes() const noexcept;
    void InputDeviceTypes(winrt::CoreInputDeviceTypes const& value);

    bool IsInputEnabled() const noexcept;
    void IsInputEnabled(bool value);

    void UpdateDefaultDrawingAttributes(winrt::InkDrawingAttributes const& drawingAttributes);
    winrt::InkDrawingAttributes CopyDefaultDrawingAttributes();

    void SetPredefinedConfiguration(inking::InkPresenterPredefinedConfiguration const& configuration);

    muxc::InkStrokeContainer StrokeContainer();
    muxc::InkInputProcessingConfiguration InputProcessingConfiguration();
    muxc::InkInputConfiguration InputConfiguration();

    winrt::event_token StrokesCollected(winrt::Windows::Foundation::TypedEventHandler<muxc::InkPresenter, muxc::InkStrokesCollectedEventArgs> const& handler);
    void StrokesCollected(winrt::event_token const& token);
    winrt::event_token StrokesErased(winrt::Windows::Foundation::TypedEventHandler<muxc::InkPresenter, muxc::InkStrokesErasedEventArgs> const& handler);
    void StrokesErased(winrt::event_token const& token);

    // Internal (not on the winmd surface). Kicks off creation of the OS presenter on the ink host
    // thread. Called once by the owning InkCanvas right after construction (never from the ctor,
    // which cannot safely take a self weak-ref). Idempotent-by-contract: called exactly once.
    void Start();

    // Internal (not on the winmd surface). The ink-thread work queue. QueueInkPresenterWorkItem is
    // fire-and-forget; RunInkPresenterWorkItemSync blocks the caller for a result. Both dispatch to
    // the ink host thread and hand the work item this proxy's OS presenter (null-safe until it has
    // been created). Called by the child proxies (via RunOnInkHostThread) and by InkCanvas (for
    // SetRootVisual / SetSize).
    void QueueInkPresenterWorkItem(std::function<void(inking::InkPresenter const&)> workItem);
    void RunInkPresenterWorkItemSync(std::function<void(inking::InkPresenter const&)> workItem, bool propagateException = false);

    // Internal (not on the winmd surface). Shows/hides the ruler / protractor stencils on the OS
    // presenter. InkPresenterRuler / InkPresenterProtractor are thread-affine, so they are created
    // and toggled entirely on the ink thread; InkToolBar (same DLL) drives these via winrt::get_self
    // when its ruler toggle / stencil menu button changes.
    void SetRulerEnabled(bool enabled);
    void SetProtractorEnabled(bool enabled);

    // Internal (not on the winmd surface). Fire our StrokesCollected/StrokesErased events on the UI
    // thread. Called from the stroke-forwarding lambdas after the (agile) strokes have been
    // snapshotted off the ink thread and marshaled back to the UI thread.
    void RaiseStrokesCollected(winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> const& strokes);
    void RaiseStrokesErased(winrt::Windows::Foundation::Collections::IVectorView<inking::InkStroke> const& strokes);

private:
    // Runs on the ink thread (from Start's work item) once the OS presenter has been created. Takes
    // ownership of it, applies initial configuration, and wires its StrokesCollected/StrokesErased
    // events into our own event forwarding (snapshot on the ink thread -> marshal to the UI thread
    // via m_uiDispatcher -> raise our events).
    void InitializeOsPresenter(inking::InkPresenter const& osPresenter);

    // The shared ink host (owns the ink thread), handed in by InkCanvas at construction. Used to
    // queue work items; the OS presenter is created and serviced on that thread.
    winrt::com_ptr<IInkDesktopHost> m_inkHost{ nullptr };

    // The ink host thread id, recorded on that thread when the OS presenter is created, so
    // RunInkPresenterWorkItemSync can detect a same-thread call and run inline instead of
    // self-deadlocking.
    DWORD m_inkThreadId{ 0 };

    // The OS presenter this proxy fronts. Thread-affine to the ink host thread; only ever touched
    // inside ink-thread work items dispatched through the queue above. NEVER touch it on the UI thread.
    inking::InkPresenter m_osPresenter{ nullptr };

    // The owning InkCanvas's UI-thread dispatcher, captured on the UI thread at construction. Used
    // to marshal StrokesCollected/StrokesErased from the ink thread back to the UI thread.
    winrt::Microsoft::UI::Dispatching::DispatcherQueue m_uiDispatcher{ nullptr };

    // UI-thread cache of the last configured values. Initialized to the same defaults
    // InitializeOsPresenter applies on the ink thread.
    winrt::CoreInputDeviceTypes m_inputDeviceTypes{
        winrt::CoreInputDeviceTypes::Mouse | winrt::CoreInputDeviceTypes::Pen | winrt::CoreInputDeviceTypes::Touch };
    bool m_isInputEnabled{ true };
    winrt::InkDrawingAttributes m_defaultDrawingAttributes{ nullptr };

    // Lazily created, cached child objects (stable instances, like the OS presenter).
    muxc::InkStrokeContainer m_strokeContainer{ nullptr };
    muxc::InkInputProcessingConfiguration m_inputProcessingConfiguration{ nullptr };
    muxc::InkInputConfiguration m_inputConfiguration{ nullptr };

    // Ruler / protractor stencils bound to the OS presenter. Thread-affine to the ink
    // thread, so they are only ever constructed and touched inside serialized ink-thread
    // work items (never accessed from the UI thread).
    inking::InkPresenterRuler m_inkRuler{ nullptr };
    inking::InkPresenterProtractor m_inkProtractor{ nullptr };

    winrt::event<winrt::Windows::Foundation::TypedEventHandler<muxc::InkPresenter, muxc::InkStrokesCollectedEventArgs>> m_strokesCollectedEvent;
    winrt::event<winrt::Windows::Foundation::TypedEventHandler<muxc::InkPresenter, muxc::InkStrokesErasedEventArgs>> m_strokesErasedEvent;
};
