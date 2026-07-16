// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InkCanvas.g.h"
#include "dcomp.h"
#include <windows.ui.input.inking.h>
#include <inkpresenterdesktop.h>
#include <map>
#include <memory>

// Both the OS Windows.UI.Input.Inking types and the MUXC types are folded into winrt::,
// and the name InkPresenter collides. Alias the two namespaces so every use is
// unambiguous: 'inking' for the OS type, 'muxc' for our projected type.
namespace inking = winrt::Windows::UI::Input::Inking;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

// Shared per-UI-thread data (the ink host + its dedicated thread, and the DComp device).
// Defined in InkCanvas.cpp; InkCanvas holds it and hands the ink host to the InkPresenter proxy.
struct ThreadData;

class InkCanvas :
    public ReferenceTracker<InkCanvas, winrt::implementation::InkCanvasT>
{
public:
    InkCanvas();
    virtual ~InkCanvas();

    // Public API surface — mirrors Windows.UI.Xaml.Controls.InkCanvas: only the
    // InkPresenter is exposed and all ink configuration flows through it. Returns our
    // marshaling InkPresenter (see InkPresenter.h) rather than the sealed OS presenter,
    // which is only serviceable on the ink thread. Created lazily and cached so callers
    // observe a stable instance, just like WUXC.
    muxc::InkPresenter InkPresenter();
 
    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    winrt::AutomationPeer OnCreateAutomationPeer();

private:

    void EnsureInkPresenter();
    void UpdateInkPresenterSize();

    void AttachToVisualLink();
    void DetachFromVisualLink();

    // Ensures the per-thread system DComp device. TryAttachSwitcherVisual is the disabled
    // switcher fast path; AttachSystemVisualLink is the ContentExternalOutputLink fallback host.
    void EnsureCompositionDevice();
    bool TryAttachSwitcherVisual();
    void AttachSystemVisualLink();

    // Composition-target (CreateTargetForHwnd) rendering + input path: a topmost per-HWND
    // target that routes pointer input to the desktop ink presenter. Multiple InkCanvas
    // controls on one HWND share a single target (TargetData::Get) and each adds/positions
    // its own ink visual.
    bool AttachToCompositionTarget();
    void DetachFromCompositionTarget();
    void PositionInkVisual();

    // Returns true when the ink visual should be hosted in the lifted XAML tree via
    // ContentExternalOutputLink (opt-in "UseSystemVisualLink" boolean app resource) instead of
    // the topmost per-HWND composition target. Evaluated once per process on first use.
    bool UseSystemVisualLink();

    std::shared_ptr<ThreadData> m_threadData;
 
    HWND m_hostHwnd = NULL;
    winrt::com_ptr<IDCompositionVisual> m_inkRootVisual;

    // Lazily created marshaling presenter returned from InkPresenter(). This proxy is the sole
    // owner of the thread-affine OS InkPresenter (stored on it, created on the ink thread); the
    // InkCanvas never holds the OS presenter directly and reaches it only through this proxy.
    muxc::InkPresenter m_inkPresenterProxy{ nullptr };

    // ContentExternalOutputLink host for the system-visual-link (CEOL) path; null on the topmost path.
    winrt::IContentExternalOutputLink m_systemVisualLink{ nullptr };

    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    winrt::XamlRoot::Changed_revoker m_xamlRootChangedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChangedRevoker;
    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedRevoker;

    // Set during DetachFromVisualLink so queued ink-thread lambdas short-circuit instead of
    // touching torn-down visual resources. Data ops (see QueueInkPresenterWorkItem) do NOT gate on
    // this. AttachToVisualLink clears it for the rapid Loaded/Unloaded re-attach case.
    std::atomic<bool> m_isDetached{ false };

    // Per-HWND shared composition target used by the CreateTargetForHwnd input/render
    // path. All InkCanvas controls hosted in the same top-level HWND share one topmost
    // IDCompositionTarget (only one is allowed per HWND) and each parents its own ink
    // visual under the shared target root. Reference-counted via a thread_local weak map
    // so the target is torn down when the last canvas on that HWND detaches.
    struct TargetData
    {
        static thread_local std::map<HWND, std::weak_ptr<TargetData>> m_tlsMap;
        winrt::com_ptr<IDCompositionTarget> m_compositionTarget;
        winrt::com_ptr<IDCompositionVisual> m_targetRootVisual;
        HWND m_hwnd;

        static std::shared_ptr<TargetData> Get(HWND hwnd)
        {
            auto iter = m_tlsMap.find(hwnd);
            if (iter != m_tlsMap.end())
            {
                if (auto existing = iter->second.lock())
                {
                    return existing;
                }
            }
            auto targetData = std::make_shared<TargetData>(hwnd);
            m_tlsMap[hwnd] = targetData;
            return targetData;
        }

        TargetData(HWND hwnd) : m_hwnd(hwnd) {}

        ~TargetData()
        {
            m_tlsMap.erase(m_hwnd);
        }
    };
    std::shared_ptr<TargetData> m_targetData;
};
