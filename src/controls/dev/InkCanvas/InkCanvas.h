// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InkCanvas.g.h"
#include "InkCanvas.properties.h"
#include "dcomp.h"
#include <windows.ui.input.inking.h>
#include <inkpresenterdesktop.h>

struct ThreadData;  

class InkCanvas :
    public ReferenceTracker<InkCanvas, winrt::implementation::InkCanvasT>,
    public InkCanvasProperties
{
public:
    InkCanvas();
    virtual ~InkCanvas();

   winrt::InkPresenter InkPresenter() {
        return m_inkPresenter;
    };
 
    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnIsEnabledPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnModePropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnAllowedInputTypesPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnDefaultDrawingAttributesPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    winrt::AutomationPeer OnCreateAutomationPeer();

    winrt::IAsyncAction QueueInkPresenterWorkItem(winrt::DoInkPresenterWork workItem);

    // New API surface - Mode, Input Types, Drawing Attributes
    winrt::InkStrokeContainer StrokeContainer();

    // Persistence
    winrt::IAsyncAction SaveAsync(winrt::Windows::Storage::Streams::IOutputStream stream);
    winrt::IAsyncAction LoadAsync(winrt::Windows::Storage::Streams::IInputStream stream);

    // Clear all strokes
    void ClearStrokes();

private:

    void CreateInkPresenter();
    void UpdateInkPresenterSize();
    void UpdateInkPresenterMode();
    void UpdateInkPresenterInputTypes();
    void SetupStrokeEvents();

    void AttachToVisualLink();
    void DetachFromVisualLink();

    std::shared_ptr<ThreadData> m_threadData;
 
    HWND m_hostHwnd = NULL;
    winrt::com_ptr<IDCompositionVisual> m_inkRootVisual;
    winrt::InkPresenter m_inkPresenter{ nullptr };

    winrt::IContentExternalOutputLink m_systemVisualLink{ nullptr };

    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};
    winrt::FrameworkElement::Unloaded_revoker m_unloadedRevoker{};
    winrt::XamlRoot::Changed_revoker m_xamlRootChangedRevoker{};
    winrt::FrameworkElement::SizeChanged_revoker m_sizeChanged_revoker;

    // Stroke event tokens for InkPresenter
    winrt::event_token m_strokesCollectedToken{};
    bool m_strokeEventsConnected{ false };

    // These methods (and struct) are all in support of the Composition Target method of
    // doing things.  They all can just go away and calls to them be removed when we
    // get the bug fixed for the visual link method and get that code path tested and enabled.
protected:
    bool UseSystemVisualLink();
    bool AttachToCompositionTarget();
    void DetachFromCompositionTarget();
    void PositionInkVisual();

    winrt::FrameworkElement::LayoutUpdated_revoker m_layoutUpdatedRevoker;

    struct TargetData
    {
        static thread_local std::map<HWND, std::weak_ptr<TargetData>> m_tlsMap;
        winrt::com_ptr<IDCompositionTarget> m_compositionTarget;
        winrt::com_ptr<IDCompositionVisual> m_targetRootVisual;
        HWND m_hwnd;

        static std::shared_ptr<TargetData> Get(HWND hwnd)
        {
            // Have we previously initialized target data for this hwnd.
            auto iter = m_tlsMap.find(hwnd);
            if (iter != m_tlsMap.end())
            {
                return iter->second.lock();
            }
            // No, so initialize it
            auto targetData = std::make_shared<TargetData>(hwnd);
            m_tlsMap[hwnd] = targetData;
            return targetData;
        }

        TargetData(HWND hwnd) : m_hwnd(hwnd)
        {
        }

        ~TargetData()
        {
            m_tlsMap.erase(m_hwnd);
        }
    };
    std::shared_ptr<TargetData> m_targetData;

 };
