// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlIsland.g.h"
#include "PopupRoot.g.h"
#include "ContentManager.h"

// XamlIsland adds multiple UI tree support to Xaml
//
// Layout:
//
//     Single layout pass for all XamlIslands
//
// Rendering:
//
//     Per-Island render pass, using separate render target
//
// Contraints:
//
//     Layout (Measure and Arrange)
//         - Size contraint on each CXamlIslandRoot
//     Render
//         - Per-island 'Windowless' WindowsPresentTarget, sized per island
//
// New DCompTreeHost composition tree associations:
//
//     XamlIslandRenderData : Maps CRootVisual to per-Island render data
//     - Shared target created from host's interop compositor (Root comp node is rooted here)
//     - HANDLE to shared target
//     - WindowsPresentTarget: dimensions, rotation, DComp synchronized commit handle
//
// Composition Trees:
//
//     One tree per XamlIsland, rooted to shared target created from host's
//     interop compositor. See DCompTreeHost's XamlIslandRenderData.
//
//
// Shared WUC Visual Tree (per XamlIsland):
//
//           Host's Visual
//                |
//           Shared Target
//                |
//            XamlIslandRoot- - - - - - - - - - - -
//           /    |      \                        |
//          /     |       \                       |
//         /      |        \                      |
// MainVisual PopupVisual  TransitionVisuals    Other Visual roots

namespace DirectUI
{
    PARTIAL_CLASS(XamlIsland)
    {
    public:
        XamlIsland();
        ~XamlIsland() override;

        _Check_return_ HRESULT Initialize(_In_ std::nullptr_t);
        _Check_return_ HRESULT Initialize(_In_ WUComp::Desktop::IDesktopWindowContentBridgeInterop* contentBridge);

        // IXamlIsland
        _Check_return_ HRESULT get_FocusControllerImpl(_Outptr_ IInspectable** value);

        _Check_return_ HRESULT get_ContentImpl(_Outptr_result_maybenull_ xaml::IUIElement** pValue);
        _Check_return_ HRESULT put_ContentImpl(_In_opt_ xaml::IUIElement* value);

        xaml_controls::IScrollViewer* GetRootScrollViewer();

        void SetOwner(_In_opt_ IInspectable* owner);
        bool TryGetOwner(_COM_Outptr_opt_result_maybenull_ IInspectable** owner);
        static void OnSizeChangedStatic(_In_ CXamlIslandRoot* xamlIsland);

    private:
        ContentManager m_contentManager;
        Microsoft::WRL::WeakRef m_owner;
    };
}
