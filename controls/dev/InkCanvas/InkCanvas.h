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

    // Compositor fork: IsSystemCompositor() detects (via CompositionEngine::GetForSystemEngine)
    // whether the process runs on the system composition engine. A system-backed process splices the
    // ink visual under a lifted MUC visual (AttachToSystemCompositor); a lifted process bridges it
    // into the XAML tree via ContentExternalOutputLink (AttachToLiftedCompositor).
    void EnsureCompositionDevice();
    bool IsSystemCompositor();
    void AttachToSystemCompositor();
    void AttachToLiftedCompositor();
    // Shared by both compositor paths: creates the ink visual on the shared DComp device and binds
    // it to the presenter. Called first by AttachToSystemCompositor/AttachToLiftedCompositor, before
    // their compositor-specific rooting.
    void AttachInkVisualToPresenter();

    std::shared_ptr<ThreadData> m_threadData;
 
    HWND m_hostHwnd = NULL;
    winrt::com_ptr<IDCompositionVisual> m_inkRootVisual;

    // Lazily created marshaling presenter returned from InkPresenter(). This proxy is the sole
    // owner of the thread-affine OS InkPresenter (stored on it, created on the ink thread); the
    // InkCanvas never holds the OS presenter directly and reaches it only through this proxy.
    muxc::InkPresenter m_inkPresenterProxy{ nullptr };

    // ContentExternalOutputLink host for the lifted compositor path; null on the system path.
    winrt::IContentExternalOutputLink m_systemVisualLink{ nullptr };

    // Writer-side DComp target from the system splice; roots the ink visual under the MUC visual and
    // is released in DetachFromVisualLink. Null on the lifted path.
    winrt::com_ptr<IDCompositionTarget> m_systemDCompTarget;

    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    winrt::XamlRoot::Changed_revoker m_xamlRootChangedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChangedRevoker;

    // Set during DetachFromVisualLink so queued ink-thread lambdas short-circuit instead of
    // touching torn-down visual resources. Data ops (see QueueInkPresenterWorkItem) do NOT gate on
    // this. AttachToVisualLink clears it for the rapid Loaded/Unloaded re-attach case.
    std::atomic<bool> m_isDetached{ false };

};
